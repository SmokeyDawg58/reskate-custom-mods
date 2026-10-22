#pragma once
#include "Engine/Game/Build/20260929/race_effects.h"
#include <array>
#include <bit>
#include <cstdint>
#include <limits>

namespace dingosdk::modes::race_world::detail {
enum class CheckpointEntityKind { unsupported, spatial, effect };
template<class ReadPointer>
CheckpointEntityKind checkpoint_entity_kind(std::uintptr_t base, std::uintptr_t entity, ReadPointer read) {
    namespace fx = game::build::v20260929::race_effects;
    if (!base || entity < 0x10000 || entity > std::numeric_limits<std::uintptr_t>::max() - 0x78)
        return CheckpointEntityKind::unsupported;
    // EffectEntity::+0x70 contains unrelated small state (0x60 in the crash),
    // rather than the transform collection SpatialEntity::setTransform requires.
    if (read(entity) == base + fx::effect_entity_vtable) return CheckpointEntityKind::effect;
    const auto collection = read(entity + 0x70);
    if (collection >= 0x10000 && read(collection) == entity) return CheckpointEntityKind::spatial;
    return CheckpointEntityKind::unsupported;
}
inline std::array<float,16> absolute_effect_transform(std::array<float,16> matrix) {
    // Frostbite stores transform metadata in the otherwise unused final lane.
    // The native effect message copies an absolute transform when these bits are
    // present; without them it composes the parent bus transform a second time.
    matrix[15] = std::bit_cast<float>(std::bit_cast<std::uint32_t>(matrix[15]) |
        game::build::v20260929::race_effects::absolute_transform_bits);
    return matrix;
}
}
