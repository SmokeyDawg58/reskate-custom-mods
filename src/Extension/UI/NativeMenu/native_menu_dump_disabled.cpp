#include "native_menu_dump.h"
namespace dingosdk::multiplayer {
std::string request_ui_dump(std::string_view, unsigned) { return "UI model dumps are not included in this playtest build."; }
void run_pending_ui_dump(std::uintptr_t) noexcept {}
}
