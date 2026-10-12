#pragma once
#include "throwdown_relay.h"

namespace dingosdk::multiplayer::one_up {
// Local practice uses the game's real player and level generation. Keep this
// input separate from the multiplayer relay: it must never advertise a peer.
inline ThrowdownRelayInput local_input(const ThrowdownRelayInput& source,
    std::uint32_t native_player, std::uint64_t level_generation,
    std::optional<std::array<float,3>> position) {
    auto result=source;
    result.peers.clear();
    result.local_only=true;
    result.local=native_player;
    result.world=native_player?level_generation:0;
    result.position=source.in_world?position:std::nullopt;
    // The map and respawn can temporarily omit the captured skater pose.
    // World lifetime comes from the loaded level, not that optional sample.
    result.in_world=source.in_world && native_player && level_generation;
    return result;
}
}
