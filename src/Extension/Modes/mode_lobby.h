#pragma once
#include "mode_rules.h"
#include <algorithm>

namespace dingosdk::modes {
inline bool lobby_departure_blocks_rejoin(Phase phase) noexcept {
    return phase!=Phase::setup;
}
inline Settings advertised_lobby_settings(const Settings& settings,bool editing) {
    auto advertised=settings;
    // An unsaved Deathrace is a draft, even after its red finish is placed.
    // The existing route-validity check withdraws its flag on every peer.
    if(advertised.mode==Mode::race && editing) {
        advertised.race_finish=false;advertised.has_spawn=false;advertised.spawn={};
        advertised.points.clear();advertised.yaws.clear();advertised.widths.clear();advertised.gate_kinds.clear();
        advertised.arrow_rolls.clear();advertised.gate_pitches.clear();advertised.corners.clear();advertised.area_radius=0;
    }
    return advertised;
}
// Enrollment is the complete guest action. The wire's historical `up` bit
// is a gameplay indicator and must not gate lobby start or the player count.
inline std::size_t lobby_enrolled_count(const Message& state) {
    if(state.phase!=Phase::setup)return 0;
    return std::count_if(state.standings.begin(),state.standings.end(),[](const auto& p){return !p.out;});
}
} // namespace dingosdk::modes
