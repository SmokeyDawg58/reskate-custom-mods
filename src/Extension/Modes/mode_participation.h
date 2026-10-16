#pragma once
#include "mode_rules.h"
#include <algorithm>

namespace dingosdk::modes {
inline Message::Kind local_quit_kind(bool leader) {return leader?Message::Kind::end:Message::Kind::leave;}
// Restrict tools only while this player can still affect the result. Their
// saved settings/bindings are never edited by a temporary competition lease.
inline bool local_competition_active(const Settings& settings,const Message& state,std::uint64_t local) {
    if(state.phase!=Phase::countdown && state.phase!=Phase::playing)return false;
    const auto player=std::find_if(state.standings.begin(),state.standings.end(),
        [&](const Standing& row){return row.player==local;});
    if(player==state.standings.end() || player->out)return false;
    return settings.mode!=Mode::race || player->score<static_cast<std::int32_t>(settings.points.size());
}
}
