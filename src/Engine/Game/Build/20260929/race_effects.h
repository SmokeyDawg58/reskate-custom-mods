#pragma once
#include <array>
#include <cstdint>

// Verified offline against the 2026-09-29 Skate.exe and the 2026-10-21 builder crash.
// EffectEntity derives from Entity, not SpatialEntity. Never use a SpatialEntity
// transform collection (+0x70) or its placement vtable slots for an effect.
namespace dingosdk::game::build::v20260929::race_effects {
inline constexpr std::uintptr_t effect_entity_vtable = 0x6372388;
inline constexpr std::uintptr_t entity_init_message_vtable = 0x6082250;
inline constexpr std::uintptr_t effect_manager = 0x74deb10;
inline constexpr std::uintptr_t effect_handle_manager = 0x74deb20;
inline constexpr std::uintptr_t particle_manager = 0x74e2328;
inline constexpr std::uintptr_t particle_manager_vtable = 0x637a078;
inline constexpr std::uintptr_t mesh_emitter_helper_vtable = 0x6371c30;
inline constexpr std::uintptr_t entity_initialize = 0x2b761d0;
inline constexpr std::array<unsigned char,16> entity_initialize_prefix{0x48,0x89,0x5c,0x24,0x08,0x57,0x48,0x83,0xec,0x20,0x48,0x8b,0x41,0x28,0x48,0x8b};
inline constexpr std::uintptr_t context_world_id = 0x18b0ee0;
inline constexpr std::array<unsigned char,16> context_world_id_prefix{0x8b,0x15,0xaa,0x15,0x71,0x05,0x48,0x8b,0x01,0x8b,0x04,0x02,0xc3,0xcc,0xcc,0xcc};
inline constexpr std::uintptr_t effect_start = 0x29d0b40;
inline constexpr std::array<unsigned char,16> effect_start_prefix{0x48,0x89,0x5c,0x24,0x10,0x48,0x89,0x6c,0x24,0x18,0x56,0x57,0x41,0x56,0x48,0x81};
inline constexpr std::uintptr_t effect_transform_changed = 0x29cd4e0;
inline constexpr std::array<unsigned char,16> effect_transform_changed_prefix{0x48,0x89,0x5c,0x24,0x08,0x48,0x89,0x74,0x24,0x10,0x57,0x48,0x83,0xec,0x70,0x8b};
// EntityInitInfo is the same 0x38-byte value built by initialize_placement,
// but dispatched through Entity::init only. SpatialEntity::setTransform is omitted.
inline constexpr std::size_t effect_transform = 0x90;
inline constexpr std::uint32_t transform_message = 0x87525469;
inline constexpr std::uint32_t absolute_transform_bits = 0x0f0f0f0f;
}
