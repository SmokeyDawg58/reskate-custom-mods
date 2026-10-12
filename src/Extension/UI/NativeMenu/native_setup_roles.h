#pragma once
#include <string_view>

namespace dingosdk::multiplayer::menu_data {
enum class NativeSetupRole { other, players, timer, hide_timer, rounds };

// Native setup labels can already be localized when the page becomes ready;
// the private timer caption is also changed after the first adaptation tick.
// Identify both the authored localization keys and observed English captions.
inline NativeSetupRole native_setup_role(std::string_view text) noexcept {
    if(text=="ID_SETTINGS_PLAYERS" || text=="Players")return NativeSetupRole::players;
    if(text=="ID_LABEL_TURNTIMER" || text=="ID_LABEL_DURATION" || text=="Turn Timer" ||
       text=="Timer" || text=="Duration")return NativeSetupRole::timer;
    if(text=="Hide Timer")return NativeSetupRole::hide_timer;
    if(text=="ID_LABEL_ROUNDS" || text=="Rounds")return NativeSetupRole::rounds;
    return NativeSetupRole::other;
}
} // namespace dingosdk::multiplayer::menu_data
