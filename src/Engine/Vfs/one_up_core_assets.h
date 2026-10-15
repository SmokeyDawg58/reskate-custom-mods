#pragma once
#include "mod_list.h"
#include <optional>

namespace dingosdk::mods {
// Native game data, independent of Mods settings. Lean playtest builds load the
// required Throwdown asset mod folder; ordinary builds embed the same bytes.
std::optional<Mod> one_up_core_assets(const std::filesystem::path& data_root);
}
