#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <span>
#include <string_view>

namespace dingosdk::mods::detail {
inline constexpr std::string_view race_core_mod = "ReSkate Core 1-Up";
inline constexpr std::string_view race_core_bundle = "win32/levels/game/bam_levelroot/bam_coregameassets";
inline constexpr std::string_view race_core_arrow_variations = "world/reskate/deathrace/arrowmeshvariationdb_win32";
// The chevron's compiled material names this texture without an EBX import.
// It must be resident beside both mesh layers when a map replaces the core.
inline constexpr std::array<std::string_view,1> race_core_arrow_textures{
    "effects/gameplay/textures/t_fx_directionalarrow_l03_v02"};
inline constexpr std::array<std::string_view,11> race_core_roots{
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_gate",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_gate",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_gate",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_finish",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_finish",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_finish",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_guide",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_guide",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_guide",
    "world/reskate/deathrace/floatingarrow_at",
    "world/reskate/deathrace/floatingarrow_a"};
inline constexpr std::array<std::string_view,3> race_core_shaders{
    "world/reskate/deathrace/checkpoints_win32_shaderstate/shaderblockdepot_12937733901070835605",
    "world/reskate/deathrace/arrows_win32_shaderstate/shaderblockdepot_12937733901070835605",
    "world/reskate/deathrace/straightarrows_win32_shaderstate/shaderblockdepot_5371451861984644426"};
inline std::span<const std::string_view> race_core_shader_roots(std::string_view name) {
    const auto roots=std::span(race_core_roots);
    if(name==race_core_shaders[0])return roots.first(9);
    if(name==race_core_shaders[1] || name==race_core_shaders[2])return roots.last(2);
    return {};
}
// The asset pack under either name: the core 1-Up bundle the DLL extracts, or the lean
// playtest's Throwdown asset mod (its folder name, possibly versioned). Without the second, a
// custom map that replaces the core bundle drops the pack's runtime roots in the lean build, and
// joining that map's server freezes in a lookup that never finds them (2026-10-17).
inline bool race_core_owner(std::string_view mod) {
    return mod == race_core_mod || mod.starts_with("SmokeyDawg58-ReSkate_Throwdown_Assets");
}
inline bool race_core_root(std::string_view mod,std::string_view bundle,std::string_view name) {
    return race_core_owner(mod) && bundle == race_core_bundle &&
        std::find(race_core_roots.begin(),race_core_roots.end(),name) != race_core_roots.end();
}
inline bool race_core_resident_root(std::string_view mod,std::string_view bundle,std::string_view name) {
    // Measured HUD variants, captions and clocks are selected by the DLL at
    // runtime and have no authored import from the picker. Retain them when
    // a custom map replaces the core bundle, just like the checkpoint roots.
    return race_core_owner(mod) && bundle == race_core_bundle &&
        (race_core_root(mod,bundle,name) || name==race_core_arrow_variations ||
         std::find(race_core_arrow_textures.begin(),race_core_arrow_textures.end(),name)!=race_core_arrow_textures.end() ||
         name.starts_with("ui/reskate/throwdowns/"));
}
inline bool race_core_shader(std::string_view mod,std::string_view bundle,std::string_view name,std::uint32_t type) {
    return race_core_owner(mod) && bundle == race_core_bundle && type == 0x73312045 &&
        !race_core_shader_roots(name).empty();
}
}
