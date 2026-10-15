#pragma once
#include "mod_list.h"
namespace dingosdk::mods {
// Validates every required file before mounting the visible Thunderstore mod.
// Missing or altered packs fail clearly; a disabled duplicate cannot hide the
// runtime's own required dependency and is never mounted a second time.
Mod required_throwdown_asset_mod(const std::filesystem::path& game);
}
