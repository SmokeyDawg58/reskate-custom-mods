#include "modding/materials/shader_depot.h"

#include "sdk/ebx/ebx_document.h"
#include "sdk/hash/hash.h"

#include <algorithm>
#include <array>
#include <atomic>
#include <cctype>
#include <cstring>
#include <limits>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <thread>
#include <tuple>
#include <unordered_set>

namespace reskate::modding::materials {

namespace {

constexpr std::uint32_t textureParameterType = 0x14C5FC30;
constexpr std::uint32_t floatParameterType = 0x2BB65C2D;
constexpr std::uint32_t vector2ParameterType = 0x2A36E9D5;
constexpr std::uint32_t vector3ParameterType = 0x885EFF59;
constexpr std::uint32_t vector4ParameterType = 0x7F999829;

template <typename Type>
Type read(const std::span<const std::byte> bytes, const std::size_t offset) {
    if (offset > bytes.size() || sizeof(Type) > bytes.size() - offset) {
        throw std::out_of_range("shader depot read exceeds its payload");
    }
    Type result;
    std::memcpy(&result, bytes.data() + offset, sizeof(result));
    return result;
}

// One compiled parameter of a block: a 16-byte header (8-byte name hash, type, element
// flags), then a 33-byte texture reference or an element count followed by the values.
struct ParameterSite {
    std::uint64_t hash{};
    std::uint32_t type{};
    std::size_t values{};
    std::uint32_t elements{};
    std::size_t width{};
};

std::size_t parameter_width(const std::uint32_t type) {
    switch (type) {
    case floatParameterType:
    case 0x36ED3444:
    case 0x8E4A817E:
    case 0x7826E998:
    case 0x57F970CA: return 4;
    case vector2ParameterType: return 8;
    case vector3ParameterType: return 12;
    case vector4ParameterType: return 16;
    default: return 0;
    }
}

std::vector<ParameterSite> parameter_sites(const std::span<const std::byte> block) {
    const auto count = read<std::uint32_t>(block, 0);
    std::vector<ParameterSite> result;
    result.reserve(count);
    std::size_t cursor = 4;
    for (std::uint32_t index = 0; index < count; ++index) {
        ParameterSite site;
        site.hash = read<std::uint64_t>(block, cursor);
        site.type = read<std::uint32_t>(block, cursor + 8);
        cursor += 16;
        if (site.type == textureParameterType) {
            if (cursor > block.size() || block.size() - cursor < 33) {
                throw std::runtime_error("shader texture parameter is truncated");
            }
            cursor += 33;
            result.push_back(site);
            continue;
        }
        site.width = parameter_width(site.type);
        if (!site.width) throw std::runtime_error("shader parameter has an unsupported type");
        site.elements = read<std::uint32_t>(block, cursor);
        site.values = cursor + 4;
        if (site.elements > (block.size() - site.values) / site.width) {
            throw std::runtime_error("shader parameter values are truncated");
        }
        cursor = site.values + site.elements * site.width;
        result.push_back(site);
    }
    return result;
}

void apply_overrides(std::vector<std::byte>& block, const ParameterOverrides& overrides) {
    if (overrides.empty()) return;
    for (const auto& site : parameter_sites(block)) {
        const auto found = overrides.find(site.hash);
        if (found == overrides.end()) continue;
        if (site.type != floatParameterType && site.type != vector2ParameterType &&
            site.type != vector3ParameterType && site.type != vector4ParameterType) continue;
        const auto floats = site.elements * site.width / sizeof(float);
        if (found->second.size() != floats) continue;
        std::memcpy(block.data() + site.values, found->second.data(), floats * sizeof(float));
    }
}

template <typename Type>
void append(std::vector<std::byte>& output, const Type value) {
    const auto offset = output.size();
    output.resize(offset + sizeof(value));
    std::memcpy(output.data() + offset, &value, sizeof(value));
}

template <typename Type>
void write(std::vector<std::byte>& output, const std::size_t offset, const Type value) {
    if (offset > output.size() || sizeof(value) > output.size() - offset) {
        throw std::out_of_range("shader depot write exceeds its payload");
    }
    std::memcpy(output.data() + offset, &value, sizeof(value));
}

void align(std::vector<std::byte>& output, const std::size_t amount) {
    output.resize((output.size() + amount - 1) / amount * amount);
}

std::string lower(std::string value) {
    std::ranges::transform(value, value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

std::optional<TextureRole> role(std::string name) {
    name = lower(std::move(name));
    const auto slash = name.find_last_of('/');
    if (slash != std::string::npos) name.erase(0, slash + 1);
    const auto suffix = [&](const std::string_view value) { return name.ends_with(value); };
    if (suffix("_c") || suffix("_d") || suffix("_alb") || suffix("_basecolor") || suffix("_bc")) {
        return TextureRole::albedo;
    }
    if (suffix("_ny")) return TextureRole::normal;
    if (suffix("_msk")) return TextureRole::mask;
    if (suffix("_osk")) return TextureRole::opacity;
    return std::nullopt;
}

struct TextureIdentity {
    sdk::io::Guid instance;
    TextureRole role{};
    std::string name;
};

struct ParsedDepot {
    std::uint32_t version{};
    std::vector<ShaderDepotAuthor::Row> rows;
    std::vector<ShaderDepotAuthor::Block> blocks;
    std::vector<std::size_t> blockStarts;
};

ParsedDepot parse(const std::span<const std::byte> payload,
                  const std::span<const std::byte> metadata) {
    if (metadata.size() != 16) throw std::runtime_error("shader depot metadata is not 16 bytes");
    const auto dataSize = read<std::uint32_t>(metadata, 0);
    const auto fixupSize = read<std::uint32_t>(metadata, 4);
    const auto version = read<std::uint32_t>(metadata, 8);
    const auto count = read<std::uint32_t>(metadata, 12);
    if (static_cast<std::uint64_t>(dataSize) + fixupSize != payload.size() ||
        static_cast<std::uint64_t>(count) * 16 + 16 > dataSize) {
        throw std::runtime_error("shader depot section table is invalid");
    }
    std::vector<std::pair<std::uint64_t, std::uint64_t>> rawRows;
    std::vector<std::uint64_t> heads;
    rawRows.reserve(count);
    for (std::size_t index = 0; index < count; ++index) {
        const auto offset = 0x10 + index * 16;
        const auto key = read<std::uint64_t>(payload, offset);
        const auto head = read<std::uint64_t>(payload, offset + 8);
        if (head == 0 || head > dataSize || 24 > dataSize - head) {
            throw std::runtime_error("shader depot row points outside its data section");
        }
        rawRows.emplace_back(key, head);
        heads.push_back(head);
    }
    std::ranges::sort(heads);
    heads.erase(std::unique(heads.begin(), heads.end()), heads.end());

    ParsedDepot result;
    result.version = version;
    result.blockStarts.reserve(heads.size());
    for (const auto head : heads) {
        const auto start = read<std::uint64_t>(payload, static_cast<std::size_t>(head));
        const auto identity = read<std::uint64_t>(payload, static_cast<std::size_t>(head) + 8);
        const auto size = read<std::uint64_t>(payload, static_cast<std::size_t>(head) + 16);
        if (size == 0 || start > dataSize || size > dataSize - start) {
            throw std::runtime_error("shader depot block extent is invalid");
        }
        result.blockStarts.push_back(static_cast<std::size_t>(start));
        result.blocks.push_back({identity,
            {payload.begin() + static_cast<std::ptrdiff_t>(start),
             payload.begin() + static_cast<std::ptrdiff_t>(start + size)}});
    }
    for (const auto [key, head] : rawRows) {
        const auto found = std::ranges::lower_bound(heads, head);
        result.rows.push_back({key, static_cast<std::size_t>(found - heads.begin())});
    }
    return result;
}

using TextureIndex = std::unordered_map<sdk::io::Guid, TextureIdentity>;

std::shared_ptr<const TextureIndex> build_texture_identities(const sdk::assets::AssetDatabase& database) {
    std::vector<std::pair<const sdk::assets::NativeAsset*, TextureRole>> candidates;
    for (const auto& asset : database.assets) {
        if (asset.kind != sdk::assets::AssetKind::ebx) continue;
        // Neutral defaults fill layers a shipped material deliberately leaves blank; binding
        // authored textures there would apply them twice.
        if (lower(asset.name).starts_with("common/textures/default/")) continue;
        if (const auto guessed = role(asset.name)) candidates.emplace_back(&asset, *guessed);
    }
    std::vector<std::optional<std::pair<sdk::io::Guid, sdk::io::Guid>>> found(candidates.size());
    std::atomic<std::size_t> next{};
    {
        const auto workers = std::clamp<std::size_t>(std::thread::hardware_concurrency(), 1,
                                                     std::max<std::size_t>(1, candidates.size()));
        std::vector<std::jthread> pool;
        pool.reserve(workers);
        for (std::size_t worker = 0; worker < workers; ++worker) {
            pool.emplace_back([&] {
                while (true) {
                    const auto index = next.fetch_add(1, std::memory_order_relaxed);
                    if (index >= candidates.size()) return;
                    try {
                        const auto document = sdk::ebx::read_document(database.read(*candidates[index].first));
                        if (document.rootType != "TextureAsset" || document.instances.empty()) continue;
                        found[index] = std::pair{document.fileGuid, document.instances.front().instanceGuid};
                    } catch (const std::exception&) {
                    }
                }
            });
        }
    }
    auto result = std::make_shared<TextureIndex>();
    for (std::size_t index = 0; index < candidates.size(); ++index) {
        if (!found[index]) continue;
        result->try_emplace(found[index]->first, TextureIdentity{
            found[index]->second, candidates[index].second, candidates[index].first->name});
    }
    return result;
}

// Every donor lookup needs this index, and building it reads thousands of TextureAssets, so
// it is built once per loaded game database and shared by later lookups and compiles.
std::shared_ptr<const TextureIndex> texture_identities(const sdk::assets::AssetDatabase& database) {
    static std::mutex mutex;
    static const sdk::assets::AssetDatabase* owner{};
    static std::size_t ownerAssets{};
    static std::shared_ptr<const TextureIndex> cached;
    std::scoped_lock lock(mutex);
    if (!cached || owner != &database || ownerAssets != database.assets.size()) {
        cached = build_texture_identities(database);
        owner = &database;
        ownerAssets = database.assets.size();
    }
    return cached;
}

std::uint64_t unique_key(const std::string_view text,
                         const std::unordered_set<std::uint64_t>& occupied) {
    auto value = sdk::hash::fnv1a_64(text);
    if (value == 0) value = 1;
    while (occupied.contains(value)) value = value * 0x9E3779B185EBCA87ULL + 0xD6E8FEB86659FD93ULL;
    return value;
}

struct BlockBindings {
    std::vector<ShaderDepotAuthor::Site> sites;
    std::vector<std::string> textureNames;
};

BlockBindings block_bindings(
    const std::span<const std::byte> bytes,
    const std::unordered_map<sdk::io::Guid, TextureIdentity>& textures) {
    BlockBindings result;
    for (std::size_t offset = 16; offset + 16 <= bytes.size(); ++offset) {
        sdk::io::Guid partition;
        std::memcpy(partition.bytes.data(), bytes.data() + offset, 16);
        const auto found = textures.find(partition);
        if (found == textures.end()) continue;
        sdk::io::Guid instance;
        std::memcpy(instance.bytes.data(), bytes.data() + offset - 16, 16);
        if (instance != found->second.instance) continue;
        result.sites.push_back({offset, found->second.role});
        result.textureNames.push_back(found->second.name);
    }
    return result;
}

bool safe_surface(const std::string& materialName,
                  const std::span<const std::string> textureNames) {
    static constexpr std::array materialAvoid{
        "_shadow", "_zonly", "m_fx_", "fx_", "_decal", "_proxy",
        "sgn_", "sign", "atlas", "vtar_", "cards", "leaf", "flower",
        "foliage", "graffiti", "graphic", "pedestrian", "canopy", "trunk"};
    static constexpr std::array textureAvoid{
        "decal_", "graffiti_", "t_fx_", "background_", "_proxy",
        "wheatpaste", "face_", "tattoo_", "acnescars", "neutral",
        "deck_gen_", "blythe_", "svlocs_poster"};
    const auto material = lower(materialName);
    if (material.empty() || std::ranges::any_of(materialAvoid, [&](const std::string_view token) {
        return material.find(token) != std::string::npos;
    })) return false;
    return std::ranges::none_of(textureNames, [&](const std::string& value) {
        const auto texture = lower(value);
        return std::ranges::any_of(textureAvoid, [&](const std::string_view token) {
            return texture.find(token) != std::string::npos;
        });
    });
}

std::size_t preferred_surface_rank(const std::string& materialName) {
    static constexpr std::array preferred{
        "synthetic_paint_", "skatepark_", "concrete", "asphalt", "wood",
        "metal", "brick", "synthetic_", "plaster", "stone", "tile"};
    const auto material = lower(materialName);
    for (std::size_t index = 0; index < preferred.size(); ++index) {
        if (material.find(preferred[index]) != std::string::npos) return index;
    }
    return preferred.size();
}

}

ShaderDepotAuthor ShaderDepotAuthor::find(const sdk::assets::AssetDatabase& database,
                                          const std::uint64_t sourceKey,
                                          const std::span<const std::size_t> allowedBundles,
                                          const TextureBindingMode mode) {
    const auto textureIndex = mode == TextureBindingMode::authored ? texture_identities(database) : nullptr;
    if (textureIndex && textureIndex->empty()) throw std::runtime_error("no native TextureAsset identities were indexed");
    const auto foundType = database.resourceTypeIndex.find(shaderBlockDepotResourceType);
    if (foundType == database.resourceTypeIndex.end()) throw std::runtime_error("game has no ShaderBlockDepot resources");

    std::optional<ShaderDepotAuthor> best;
    std::size_t bestSites{};
    for (const auto assetIndex : foundType->second) {
        const auto& asset = database.assets[assetIndex];
        if (!allowedBundles.empty() && std::ranges::none_of(asset.bundles, [&](const std::size_t bundle) {
            return std::ranges::find(allowedBundles, bundle) != allowedBundles.end();
        })) continue;
        try {
            auto parsed = parse(database.read(asset), asset.resourceMeta);
            const auto row = std::ranges::find(parsed.rows, sourceKey, &Row::key);
            if (row == parsed.rows.end()) continue;
            const auto blockIndex = row->block;
            const auto& bytes = parsed.blocks[blockIndex].data;
            auto sites = textureIndex ? block_bindings(bytes, *textureIndex).sites : std::vector<Site>{};
            if (textureIndex && sites.empty()) continue;
            if (!best || sites.size() > bestSites) {
                ShaderDepotAuthor candidate;
                candidate.donorName_ = asset.name;
                candidate.version_ = parsed.version;
                candidate.sourceKey_ = sourceKey;
                candidate.sourceBlock_ = blockIndex;
                candidate.rows_ = std::move(parsed.rows);
                candidate.blocks_ = std::move(parsed.blocks);
                candidate.sites_ = std::move(sites);
                bestSites = candidate.sites_.size();
                best = std::move(candidate);
            }
        } catch (const std::exception&) {
        }
    }
    if (!best) throw std::runtime_error("no ShaderBlockDepot contains the mesh template material key");
    return std::move(*best);
}

namespace {

std::vector<SurfaceCandidate> collect_surfaces(
    const sdk::assets::AssetDatabase& database,
    const std::unordered_map<sdk::io::Guid, TextureIdentity>& textures,
    const std::span<const SurfacePrototype> prototypes,
    const std::span<const std::size_t> allowedBundles) {
    const auto foundType = database.resourceTypeIndex.find(shaderBlockDepotResourceType);
    if (foundType == database.resourceTypeIndex.end()) throw std::runtime_error("game has no ShaderBlockDepot resources");
    std::vector<SurfaceCandidate> result;
    for (const auto assetIndex : foundType->second) {
        const auto& asset = database.assets[assetIndex];
        if (!allowedBundles.empty() && std::ranges::none_of(asset.bundles, [&](const std::size_t bundle) {
            return std::ranges::find(allowedBundles, bundle) != allowedBundles.end();
        })) continue;
        try {
            const auto parsed = parse(database.read(asset), asset.resourceMeta);
            for (const auto& prototype : prototypes) {
                const auto row = std::ranges::find(parsed.rows, prototype.key, &ShaderDepotAuthor::Row::key);
                if (row == parsed.rows.end() || row->block >= parsed.blocks.size()) continue;
                const auto& block = parsed.blocks[row->block].data;
                auto bindings = block_bindings(block, textures);
                SurfaceCandidate candidate;
                candidate.key = prototype.key;
                candidate.materialName = prototype.materialName;
                candidate.depot = asset.name;
                for (const auto& site : bindings.sites) candidate.roles.push_back(site.role);
                candidate.roleTextures = std::move(bindings.textureNames);
                for (const auto& site : parameter_sites(block)) {
                    if (site.type == textureParameterType) {
                        ++candidate.textureParameters;
                    } else if (site.type == vector2ParameterType && site.elements == 1) {
                        std::array<float, 2> value{};
                        std::memcpy(value.data(), block.data() + site.values, sizeof(value));
                        candidate.tiledVectors += value[0] != 1.0F || value[1] != 1.0F;
                    }
                }
                result.push_back(std::move(candidate));
            }
        } catch (const std::exception&) {
        }
    }
    return result;
}

}

std::vector<ParameterDump> dump_block_parameters(const sdk::assets::AssetDatabase& database,
                                                const std::uint64_t key,
                                                const sdk::assets::NativeAsset** depot) {
    const auto textureIndex = texture_identities(database);
    const auto foundType = database.resourceTypeIndex.find(shaderBlockDepotResourceType);
    if (foundType == database.resourceTypeIndex.end()) throw std::runtime_error("game has no ShaderBlockDepot resources");
    for (const auto assetIndex : foundType->second) {
        const auto& asset = database.assets[assetIndex];
        try {
            const auto parsed = parse(database.read(asset), asset.resourceMeta);
            const auto row = std::ranges::find(parsed.rows, key, &ShaderDepotAuthor::Row::key);
            if (row == parsed.rows.end() || row->block >= parsed.blocks.size()) continue;
            if (depot) *depot = &asset;
            const auto& block = parsed.blocks[row->block].data;
            const auto bindings = block_bindings(block, *textureIndex);
            std::vector<ParameterDump> result;
            std::size_t textureOrdinal{};
            for (const auto& site : parameter_sites(block)) {
                ParameterDump dump{site.hash, site.type, {}, {}};
                if (site.type == textureParameterType) {
                    if (textureOrdinal < bindings.textureNames.size()) dump.texture = bindings.textureNames[textureOrdinal];
                    ++textureOrdinal;
                } else {
                    dump.values.resize(site.elements * site.width / sizeof(float));
                    std::memcpy(dump.values.data(), block.data() + site.values, dump.values.size() * sizeof(float));
                }
                result.push_back(std::move(dump));
            }
            return result;
        } catch (const std::exception&) {
        }
    }
    throw std::runtime_error("no ShaderBlockDepot holds the material key");
}

std::vector<SurfaceCandidate> ShaderDepotAuthor::survey_surfaces(
    const sdk::assets::AssetDatabase& database,
    const std::span<const SurfacePrototype> prototypes,
    const std::span<const std::size_t> allowedBundles) {
    const auto textureIndex = texture_identities(database);
    const auto& textures = *textureIndex;
    if (textures.empty()) throw std::runtime_error("no native TextureAsset identities were indexed");
    return collect_surfaces(database, textures, prototypes, allowedBundles);
}

ShaderDepotAuthor ShaderDepotAuthor::find_surface(
    const sdk::assets::AssetDatabase& database,
    const std::span<const SurfacePrototype> prototypes,
    const std::span<const TextureRole> requiredRoles,
    const std::span<const std::size_t> allowedBundles,
    const bool requirePlainSurface) {
    if (prototypes.empty()) throw std::runtime_error("no declaration-compatible world material keys were found");
    if (requiredRoles.empty()) throw std::runtime_error("world material prototype selection has no required texture roles");
    const auto textureIndex = texture_identities(database);
    const auto& textures = *textureIndex;
    if (textures.empty()) throw std::runtime_error("no native TextureAsset identities were indexed");

    // Authored textures replace only the role-named sites of the donor block; every other
    // texture and vector keeps the donor's value. Layered donors therefore stamp their panel
    // layouts, grime and layer tiling onto the map, so prefer the plainest block: fewest
    // textures left untouched, no duplicated roles, and no non-unit tiling.
    struct Selection {
        std::string depot;
        std::uint64_t key{};
        std::string materialName;
        std::tuple<std::size_t, std::size_t, std::size_t, std::size_t, std::string, std::uint64_t> score;
    };
    std::optional<Selection> best;
    for (const auto& candidate : collect_surfaces(database, textures, prototypes, allowedBundles)) {
        const auto count = [&](const TextureRole role) {
            return static_cast<std::size_t>(std::ranges::count(candidate.roles, role));
        };
        if (std::ranges::any_of(requiredRoles, [&](const TextureRole role) { return count(role) == 0; }) ||
            !safe_surface(candidate.materialName, candidate.roleTextures)) continue;
        auto extraSites = std::size_t{};
        for (const auto role : requiredRoles) extraSites += count(role) - 1;
        const auto untouched = candidate.textureParameters - candidate.roles.size();
        if (requirePlainSurface && (untouched != 0 || extraSites != 0 || candidate.tiledVectors != 0 ||
            candidate.textureParameters != requiredRoles.size())) continue;
        const auto score = std::tuple{untouched, extraSites, candidate.tiledVectors,
                                      preferred_surface_rank(candidate.materialName),
                                      lower(candidate.materialName), candidate.key};
        if (!best || score < best->score) best = Selection{candidate.depot, candidate.key, candidate.materialName, score};
    }
    if (!best) {
        throw std::runtime_error(
            "native shader depots have no safe world-surface block for the authored declaration and texture roles");
    }

    const auto* chosen = database.find(best->depot, sdk::assets::AssetKind::resource);
    if (!chosen) throw std::runtime_error("selected world-surface shader depot disappeared");
    const auto& asset = *chosen;
    auto parsed = parse(database.read(asset), asset.resourceMeta);
    const auto row = std::ranges::find(parsed.rows, best->key, &Row::key);
    if (row == parsed.rows.end() || row->block >= parsed.blocks.size()) {
        throw std::runtime_error("selected world-surface material disappeared from its shader depot");
    }
    auto sites = block_bindings(parsed.blocks[row->block].data, textures).sites;
    ShaderDepotAuthor result;
    result.donorName_ = asset.name;
    result.sourceMaterial_ = best->materialName;
    result.version_ = parsed.version;
    result.sourceKey_ = best->key;
    result.sourceBlock_ = row->block;
    result.rows_ = std::move(parsed.rows);
    result.blocks_ = std::move(parsed.blocks);
    result.sites_ = std::move(sites);
    return result;
}

std::uint64_t ShaderDepotAuthor::add_material(
    const std::string_view identity,
    const std::unordered_map<TextureRole, TextureBinding>& textures,
    const ParameterOverrides& overrides) {
    if (sourceBlock_ >= blocks_.size()) throw std::logic_error("shader depot source block is missing");
    return add_material_impl(identity, textures, blocks_[sourceBlock_].data, sites_, overrides);
}

std::uint64_t ShaderDepotAuthor::add_material_from(
    const std::string_view identity,
    const std::unordered_map<TextureRole, TextureBinding>& textures,
    const ShaderDepotAuthor& prototype,
    const ParameterOverrides& overrides) {
    if (prototype.sourceBlock_ >= prototype.blocks_.size()) {
        throw std::logic_error("shader depot prototype block is missing");
    }
    return add_material_impl(identity, textures, prototype.blocks_[prototype.sourceBlock_].data,
                             prototype.sites_, overrides);
}

std::uint64_t ShaderDepotAuthor::add_material_replacing(
    const std::string_view identity, const std::uint64_t destinationKey,
    const TextureBinding original, const TextureBinding replacement) {
    if (!destinationKey || sourceBlock_ >= blocks_.size() ||
        std::ranges::any_of(rows_, [&](const Row& row) { return row.key == destinationKey; }))
        throw std::runtime_error("native variation material key is unavailable");
    auto clone = blocks_[sourceBlock_].data;
    std::size_t replaced{};
    for (std::size_t at = 0; at + 32 <= clone.size(); ++at) {
        if (std::memcmp(clone.data() + at, original.instance.bytes.data(), 16) ||
            std::memcmp(clone.data() + at + 16, original.partition.bytes.data(), 16)) continue;
        std::memcpy(clone.data() + at, replacement.instance.bytes.data(), 16);
        std::memcpy(clone.data() + at + 16, replacement.partition.bytes.data(), 16);
        ++replaced; at += 31;
    }
    if (replaced != 1) throw std::runtime_error("native flag material must contain exactly one badge texture binding");
    (void)add_material_impl(identity, {}, clone, {}, {});
    rows_.back().key = destinationKey;
    return destinationKey;
}

std::uint64_t ShaderDepotAuthor::add_material_impl(
    const std::string_view identity,
    const std::unordered_map<TextureRole, TextureBinding>& textures,
    const std::span<const std::byte> source,
    const std::span<const Site> sites,
    const ParameterOverrides& overrides) {
    std::unordered_set<std::uint64_t> occupied;
    for (const auto& row : rows_) occupied.insert(row.key);
    for (const auto& block : blocks_) occupied.insert(block.identity);
    const auto key = unique_key(std::string("reskate-material-key:") + std::string(identity), occupied);
    occupied.insert(key);

    std::vector<std::byte> clone(source.begin(), source.end());
    for (const auto& site : sites) {
        auto found = textures.find(site.role);
        if (found == textures.end() && site.role == TextureRole::opacity) {
            found = textures.find(TextureRole::albedo);
        }
        if (found == textures.end()) continue;
        if (site.offset < 16 || site.offset + 16 > clone.size()) {
            throw std::runtime_error("shader depot texture binding site is invalid");
        }
        std::memcpy(clone.data() + site.offset - 16, found->second.instance.bytes.data(), 16);
        std::memcpy(clone.data() + site.offset, found->second.partition.bytes.data(), 16);
    }
    apply_overrides(clone, overrides);
    std::vector<std::byte> cacheInput;
    cacheInput.insert(cacheInput.end(), reinterpret_cast<const std::byte*>(identity.data()),
                      reinterpret_cast<const std::byte*>(identity.data() + identity.size()));
    cacheInput.insert(cacheInput.end(), clone.begin(), clone.end());
    auto cacheIdentity = sdk::hash::murmur2_64(cacheInput, key);
    if (cacheIdentity == 0) cacheIdentity = 1;
    while (occupied.contains(cacheIdentity)) ++cacheIdentity;
    blocks_.push_back({cacheIdentity, std::move(clone)});
    rows_.push_back({key, blocks_.size() - 1});
    return key;
}

AuthoredDepot ShaderDepotAuthor::build_subset(const std::string_view bundleStem,
                                             const std::span<const std::uint64_t> keys) const {
    if (keys.empty()) throw std::invalid_argument("shader depot subset has no material keys");
    auto subset = *this;
    subset.rows_.clear();
    subset.blocks_.clear();
    std::unordered_map<std::size_t, std::size_t> remapped;
    std::unordered_set<std::uint64_t> emitted;
    for (const auto key : keys) {
        if (!emitted.insert(key).second) continue;
        const auto row = std::ranges::find(rows_, key, &Row::key);
        if (row == rows_.end() || row->block >= blocks_.size())
            throw std::runtime_error("shader depot subset material is missing");
        const auto [mapped, fresh] = remapped.try_emplace(row->block, subset.blocks_.size());
        if (fresh) subset.blocks_.push_back(blocks_[row->block]);
        subset.rows_.push_back({key, mapped->second});
    }
    return subset.build(bundleStem);
}

AuthoredDepot ShaderDepotAuthor::build(const std::string_view bundleStem) const {
    if (rows_.empty() || blocks_.empty()) throw std::logic_error("shader depot has no rows or blocks");
    std::vector<std::byte> data(16 + rows_.size() * 16);
    write(data, 0, std::uint64_t{16});
    std::vector<std::uint64_t> starts;
    starts.reserve(blocks_.size());
    for (const auto& block : blocks_) {
        starts.push_back(data.size());
        data.insert(data.end(), block.data.begin(), block.data.end());
    }
    align(data, 16);
    std::vector<std::uint64_t> heads;
    heads.reserve(blocks_.size());
    for (std::size_t index = 0; index < blocks_.size(); ++index) {
        heads.push_back(data.size());
        append(data, starts[index]);
        append(data, blocks_[index].identity);
        append(data, static_cast<std::uint64_t>(blocks_[index].data.size()));
    }
    align(data, 16);
    const auto dataSize = data.size();
    std::vector<std::uint32_t> relocations;
    std::unordered_set<std::size_t> emitted;
    for (std::size_t index = 0; index < rows_.size(); ++index) {
        const auto& row = rows_[index];
        if (row.block >= heads.size()) throw std::runtime_error("shader depot row names a missing block");
        const auto offset = 0x10 + index * 16;
        write(data, offset, row.key);
        write(data, offset + 8, heads[row.block]);
        if (emitted.insert(row.block).second) relocations.push_back(static_cast<std::uint32_t>(heads[row.block]));
        relocations.push_back(static_cast<std::uint32_t>(offset + 8));
    }
    relocations.push_back(0);
    for (const auto relocation : relocations) append(data, relocation);

    std::vector<std::byte> metadata;
    append(metadata, static_cast<std::uint32_t>(dataSize));
    append(metadata, static_cast<std::uint32_t>(relocations.size() * sizeof(std::uint32_t)));
    append(metadata, version_);
    append(metadata, static_cast<std::uint32_t>(rows_.size()));
    const auto suffix = donorName_.substr(donorName_.find_last_of('/') + 1);
    return {std::string(bundleStem) + "_win32_shaderstate/" + suffix,
            std::move(data), std::move(metadata)};
}

std::string_view ShaderDepotAuthor::donor_name() const noexcept { return donorName_; }
std::string_view ShaderDepotAuthor::source_material() const noexcept { return sourceMaterial_; }
std::uint64_t ShaderDepotAuthor::source_key() const noexcept { return sourceKey_; }
std::size_t ShaderDepotAuthor::binding_site_count() const noexcept { return sites_.size(); }

}
