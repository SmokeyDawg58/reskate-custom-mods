#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <format>
#include <optional>
#include <string>

namespace dingosdk::modes {
inline std::string match_time(std::uint64_t ms) {
    return std::format("{:02}:{:02}.{:03}",ms/60000,(ms/1000)%60,ms%1000);
}
struct MatchAnnouncements {
    bool started{},ended{};
    std::uint64_t playing_at{};
    std::optional<std::string> observe(std::string_view host,const Settings& settings,const Message& state,std::uint64_t now) {
        if(!started && (state.phase==Phase::countdown || state.phase==Phase::playing)) {
            started=true;playing_at=now+(state.phase==Phase::countdown?state.remaining_ms:0);
            const auto timer=timed(settings.mode)?settings.duration_s:settings.turn_s;
            return std::format("{} has started {} ({}).",host,mode_name(settings.mode),match_time(timer*1000ULL));
        }
        if(started && !ended && state.phase==Phase::results) {
            ended=true;
            auto elapsed=now>playing_at?now-playing_at:0;
            if(timed(settings.mode))elapsed=std::min<std::uint64_t>(elapsed,settings.duration_s*1000ULL);
            if(settings.mode==Mode::race && !state.standings.empty()) {
                const auto& first=state.standings.front();
                if(!first.out && first.aux>=0 && first.score>=static_cast<std::int32_t>(settings.points.size()))elapsed=first.aux;
            }
            return std::format("{} has ended {} ({}).",host,mode_name(settings.mode),match_time(elapsed));
        }
        return {};
    }
};
}
