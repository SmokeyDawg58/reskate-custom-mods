#pragma once
#include "mode_rules.h"
#include <optional>

namespace dingosdk::modes {
enum class RaceCountdownCue { count,go };
struct RaceCountdownAudio {
    unsigned last_second{};
    bool counted{},started{};
    std::optional<RaceCountdownCue> update(Phase phase,std::uint64_t remaining) noexcept {
        if(phase==Phase::setup){*this={};return {};}
        if(phase==Phase::countdown) {
            const auto seconds=static_cast<unsigned>((remaining+999)/1000);
            if(seconds>=1 && seconds<=3 && (!last_second || seconds<last_second)) {
                last_second=seconds;counted=true;return RaceCountdownCue::count;
            }
        }
        if(phase==Phase::playing && !started) {
            started=true;if(counted)return RaceCountdownCue::go;
        }
        return {};
    }
};
enum class RaceGateCue { none,required,finish };
inline RaceGateCue race_gate_cue(const Settings& route,int before,int after) noexcept {
    if(before<0 || after<=before || route.points.empty())return RaceGateCue::none;
    if(static_cast<std::size_t>(after)>=route.points.size())return RaceGateCue::finish;
    return mandatory_gates(route,after)>mandatory_gates(route,before)?RaceGateCue::required:RaceGateCue::none;
}
// Observe authoritative progress even after the final gate changes Playing to
// Results. Gameplay-only checkpoint polling cannot see a solo race's finish.
struct RaceProgressAudio {
    int heard{};
    RaceGateCue update(const Settings& route,Phase phase,int score,bool out) noexcept {
        if(phase==Phase::setup || phase==Phase::countdown){heard=0;return RaceGateCue::none;}
        if(out || score<=heard)return RaceGateCue::none;
        const auto cue=race_gate_cue(route,heard,score);heard=score;return cue;
    }
};
}
