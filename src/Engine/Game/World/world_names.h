#pragma once
#include <algorithm>
#include <string>
#include <string_view>

namespace dingosdk {
inline std::string world_level_short_name(std::string_view asset) {
    auto name = std::string(asset.substr(asset.find_last_of("/\\") + 1));
    auto folded = name;
    for (auto& c : folded) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    if (folded.starts_with("dingolevel_")) { name.erase(0, 11); folded.erase(0, 11); }
    // A custom map's level is dingolevel_reskate_<map> (ReSkate Studio): the map's own name is
    // what is shown and typed.
    if (folded.starts_with("reskate_") && folded.size() > 8) { name.erase(0, 8); folded.erase(0, 8); }
    if (folded.ends_with("_levelroot")) name.resize(name.size() - 10);
    std::replace(name.begin(), name.end(), '_', ' ');
    return name;
}
// A map's name as a player typed it. Custom maps used to be listed as "reskate <map>": typed
// that way it still means the map.
inline std::string_view world_level_typed_name(std::string_view typed) {
    constexpr std::string_view old = "reskate ";
    if (typed.size() <= old.size()) return typed;
    for (std::size_t i = 0; i < old.size(); ++i)
        if ((typed[i] | 0x20) != old[i] && typed[i] != old[i]) return typed;
    return typed.substr(old.size());
}
inline std::string world_level_name(std::string_view asset) {
    auto name = world_level_short_name(asset);
    auto folded = name;
    for (auto& c : folded) if (c >= 'A' && c <= 'Z') c = static_cast<char>(c + ('a' - 'A'));
    if (folded == "bam") return "San Vansterdam";
    if (folded == "ftue island") return "Tutorial Island";
    if (folded == "mpr") return "Super Ultra Mega Resort";
    if (folded == "sdm int 001") return "Stadium 1";
    if (folded == "sdm int 002") return "Stadium 2";
    if (folded == "isle of grom") return "Isle of Grom";
    if (folded == "level dev activitysandbox") return "Activity Sandbox (Dev)";
    return name;
}
// Multiplayer destinations contain a root and an optional detached level.
// Show the detached map when present, or the root for a root-only session.
inline std::string_view world_destination_asset(std::string_view destination) {
    const auto split = destination.find('|');
    if (split == std::string_view::npos) return destination;
    return split + 1 < destination.size() ? destination.substr(split + 1) : destination.substr(0, split);
}
template<class Levels>
inline std::string world_destination_name(std::string_view destination, const Levels& levels, bool match_short_name=false) {
    const auto asset = world_destination_asset(destination);
    const auto normalized = [](char c) {
        if (c == '\\') return '/';
        return c >= 'A' && c <= 'Z' ? static_cast<char>(c + ('a' - 'A')) : c;
    };
    const auto equal=[&](std::string_view a,std::string_view b) {
        return std::equal(a.begin(),a.end(),b.begin(),b.end(),[&](char x,char y){return normalized(x)==normalized(y);});
    };
    for (const auto& level : levels)
        if (!level.display_name.empty() && equal(asset,level.asset))return level.display_name;
    // Saved routes historically carry the persistence map key rather than a
    // full level asset. Resolve that presentation alias only when requested.
    if(match_short_name) {
        const auto key=world_level_short_name(asset);
        for(const auto& level:levels)if(!level.display_name.empty() && equal(key,world_level_short_name(level.asset)))return level.display_name;
    }
    return asset.empty() ? "Load a map" : world_level_name(asset);
}
}
