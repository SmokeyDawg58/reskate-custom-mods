#pragma once
#include "mod_list.h"
namespace dingosdk::mods {
// Manual ZIP extraction and mod managers append a numeric x.y.z version.
// These names are reserved for the required dependency, including old copies.
bool throwdown_asset_folder_name(std::wstring_view name);
// Validates every required file before mounting the visible Thunderstore mod.
// Missing or altered packs fail clearly; a disabled duplicate cannot hide the
// runtime's own required dependency and is never mounted a second time.
Mod required_throwdown_asset_mod(const std::filesystem::path& game);
}
