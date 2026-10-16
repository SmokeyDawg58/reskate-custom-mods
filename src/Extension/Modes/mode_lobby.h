#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <set>
#include <span>

namespace dingosdk::modes {
// The leader is ready by owning the draft. Joining reserves a place; only
// that joined player's explicit Ready action can mark their place ready.
class LobbyReadiness {
    std::set<std::uint64_t> ready_;
public:
    bool mark(std::uint64_t player,std::span<const std::uint64_t> joined) {
        if(!player || std::find(joined.begin(),joined.end(),player)==joined.end())return false;
        return ready_.insert(player).second;
    }
    bool ready(std::uint64_t player) const {return ready_.contains(player);}
    bool all(std::span<const std::uint64_t> joined) const {
        return std::all_of(joined.begin(),joined.end(),[&](auto id){return ready(id);});
    }
    void remove(std::uint64_t player) {ready_.erase(player);}
    void reset() {ready_.clear();}
    void retain(std::span<const std::uint64_t> joined) {
        std::erase_if(ready_,[&](auto id){return std::find(joined.begin(),joined.end(),id)==joined.end();});
    }
};
inline std::size_t lobby_ready_count(const Message& state) {
    if(state.phase!=Phase::setup)return 0;
    return std::count_if(state.standings.begin(),state.standings.end(),[](const auto& p){return !p.out && p.up;});
}
} // namespace dingosdk::modes
