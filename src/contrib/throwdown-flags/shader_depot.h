#pragma once

#include "sdk/assets/frostbite_asset_manager.h"

#include <cstdint>
#include <optional>
#include <span>
#include <string>
#include <unordered_map>
#include <vector>

namespace reskate::modding::materials {

inline constexpr std::uint32_t shaderBlockDepotResourceType = 0x73312045;

enum class TextureRole {
    albedo,
    normal,
    mask,
    opacity
};

enum class TextureBindingMode {
    authored,
    preserveNative
};

struct TextureBinding {
    sdk::io::Guid partition;
    sdk::io::Guid instance;
};

struct AuthoredDepot {
    std::string name;
    std::vector<std::byte> resource;
    std::vector<std::byte> resourceMeta;
};

struct SurfacePrototype {
    std::uint64_t key{};
    std::string materialName;
};

// A shipped world-surface block that could host authored textures.
struct SurfaceCandidate {
    std::uint64_t key{};
    std::string materialName;
    std::string depot;
    std::vector<TextureRole> roles;
    std::vector<std::string> roleTextures;
    std::size_t textureParameters{};
    // Vector2 parameters other than (1, 1): layer tiling the authored textures would inherit.
    std::size_t tiledVectors{};
};

// Float values written over a cloned block's parameters, keyed by parameter hash. A value
// list must match the parameter's element count; parameters the donor lacks are skipped.
using ParameterOverrides = std::unordered_map<std::uint64_t, std::vector<float>>;

// Shipped world blocks precompute these from their own textures. The renderer shades
// distant surfaces with them instead of sampling, so a clone must carry its own values.
inline constexpr std::uint64_t averageAlbedoParameter = 0xFD3D7B2C922F3A8AULL;     // linear RGB
inline constexpr std::uint64_t averageSmoothnessParameter = 0xC12DD8BE6B17463AULL; // mask green
inline constexpr std::uint64_t averageMetalnessParameter = 0xF6B3B4A0183ED809ULL;  // mask red

// Research: one compiled parameter of a shipped block, values as floats (texture parameters
// carry the bound texture's name instead).
struct ParameterDump {
    std::uint64_t hash{};
    std::uint32_t type{};
    std::vector<float> values;
    std::string texture;
};
[[nodiscard]] std::vector<ParameterDump> dump_block_parameters(const sdk::assets::AssetDatabase& database,
                                                               std::uint64_t key,
                                                               const sdk::assets::NativeAsset** depot = nullptr);

class ShaderDepotAuthor final {
public:
    [[nodiscard]] static ShaderDepotAuthor find(
        const sdk::assets::AssetDatabase& database,
        std::uint64_t sourceKey,
        std::span<const std::size_t> allowedBundles = {},
        TextureBindingMode mode = TextureBindingMode::authored);
    [[nodiscard]] static ShaderDepotAuthor find_surface(
        const sdk::assets::AssetDatabase& database,
        std::span<const SurfacePrototype> prototypes,
        std::span<const TextureRole> requiredRoles,
        std::span<const std::size_t> allowedBundles = {},
        bool requirePlainSurface = false);
    [[nodiscard]] static std::vector<SurfaceCandidate> survey_surfaces(
        const sdk::assets::AssetDatabase& database,
        std::span<const SurfacePrototype> prototypes,
        std::span<const std::size_t> allowedBundles = {});

    [[nodiscard]] std::uint64_t add_material(
        std::string_view identity,
        const std::unordered_map<TextureRole, TextureBinding>& textures,
        const ParameterOverrides& overrides = {});
    [[nodiscard]] std::uint64_t add_material_from(
        std::string_view identity,
        const std::unordered_map<TextureRole, TextureBinding>& textures,
        const ShaderDepotAuthor& prototype,
        const ParameterOverrides& overrides = {});
    // A native mesh variation fixes the material key to baseKey + NameHash.
    // Replace one exact texture identity, preserving the pole's other albedos.
    [[nodiscard]] std::uint64_t add_material_replacing(
        std::string_view identity, std::uint64_t destinationKey,
        TextureBinding original, TextureBinding replacement);
    [[nodiscard]] AuthoredDepot build(std::string_view bundleStem) const;
    // Export only the requested native material keys. Loading an entire world
    // depot in a UI bundle would register unrelated materials before their
    // world texture dependencies are resident.
    [[nodiscard]] AuthoredDepot build_subset(std::string_view bundleStem,
                                            std::span<const std::uint64_t> keys) const;
    [[nodiscard]] std::string_view donor_name() const noexcept;
    [[nodiscard]] std::string_view source_material() const noexcept;
    [[nodiscard]] std::uint64_t source_key() const noexcept;
    [[nodiscard]] std::size_t binding_site_count() const noexcept;

public:
    struct Row {
        std::uint64_t key{};
        std::size_t block{};
    };
    struct Block {
        std::uint64_t identity{};
        std::vector<std::byte> data;
    };
    struct Site {
        std::size_t offset{};
        TextureRole role{};
    };

private:
    [[nodiscard]] std::uint64_t add_material_impl(
        std::string_view identity,
        const std::unordered_map<TextureRole, TextureBinding>& textures,
        std::span<const std::byte> source,
        std::span<const Site> sites,
        const ParameterOverrides& overrides);

    std::string donorName_;
    std::string sourceMaterial_;
    std::uint32_t version_{};
    std::uint64_t sourceKey_{};
    std::size_t sourceBlock_{};
    std::vector<Row> rows_;
    std::vector<Block> blocks_;
    std::vector<Site> sites_;
};

}
