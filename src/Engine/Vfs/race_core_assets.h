#pragma once
#include <array>
#include <algorithm>
#include <cstdint>
#include <string_view>

namespace dingosdk::mods::detail {
inline constexpr std::string_view race_core_mod = "ReSkate Core 1-Up";
inline constexpr std::string_view race_core_bundle = "win32/levels/game/bam_levelroot/bam_coregameassets";
inline constexpr std::array<std::string_view,9> race_core_roots{
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_gate",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_gate",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_gate",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_finish",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_finish",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_finish",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_at_guide",
    "world/reskate/deathrace/sm_fx_gp_checkpoint_a_al_guide",
    "world/reskate/deathrace/sm_fx_gp_lightningbolt_a_guide"};
inline bool race_core_root(std::string_view mod,std::string_view bundle,std::string_view name) {
    return mod == race_core_mod && bundle == race_core_bundle &&
        std::find(race_core_roots.begin(),race_core_roots.end(),name) != race_core_roots.end();
}
inline bool race_core_shader(std::string_view mod,std::string_view bundle,std::string_view name,std::uint32_t type) {
    return mod == race_core_mod && bundle == race_core_bundle && type == 0x73312045 &&
        name == "world/reskate/deathrace/checkpoints_win32_shaderstate/shaderblockdepot_12937733901070835605";
}
}
