#pragma once
#include "mode_rules.h"
#include <cmath>
#include <optional>
#include <vector>

namespace dingosdk::modes {
inline constexpr float race_compass_icon_size=96.f;
inline constexpr float race_compass_center_y=209.f;
inline constexpr float race_compass_max_offset=(1184.f-race_compass_icon_size)*.5f;
// The compass is local presentation; it never advances the referee or changes
// a player's map waypoint. Guide gates and decorative arrows are not targets.
inline std::optional<std::size_t> race_compass_gate(const Settings& route,const Message& state,std::uint64_t local) {
    if(route.mode!=Mode::race || state.phase!=Phase::playing)return {};
    for(const auto& row:state.standings)if(row.player==local && !row.out) {
        if(row.score<0)return {};
        for(auto i=static_cast<std::size_t>(row.score);i<route.points.size();++i)
            if(i>0 && (i>=route.gate_kinds.size() || route.gate_kinds[i]==RaceGateKind::mandatory))return i;
    }
    return {};
}
// Guide gates and decorative Arrows have no compass pointer.
inline std::vector<std::size_t> race_compass_gates(const Settings& route,const Message& state,std::uint64_t local,bool enabled) {
    if(enabled)if(const auto next=race_compass_gate(route,state,local))return {*next};
    return {};
}
inline std::string_view race_compass_art(const Settings& route,std::size_t index) noexcept {
    if(route.race_finish && index+1==route.points.size())return "UI/ReSkate/Throwdowns/img_FinishGateCompass";
    return "UI/ReSkate/Throwdowns/img_GateCompass";
}
// Native compass projection: ahead is the center; +/-90 degrees reaches
// either end of its 1184-unit strip. A target behind stays at the nearer end.
inline std::optional<float> race_compass_offset(const Vec3& target,const std::array<float,16>& camera) {
    const float dx=target[0]-camera[12],dz=target[2]-camera[14];
    const float right=dx*camera[0]+dz*camera[2],forward=-dx*camera[8]-dz*camera[10];
    if(!std::isfinite(right) || !std::isfinite(forward) || std::hypot(camera[8],camera[10])<.001f)return {};
    return std::clamp(std::atan2(right,forward)/(3.14159265f*.5f),-1.f,1.f)*race_compass_max_offset;
}
} // namespace dingosdk::modes
