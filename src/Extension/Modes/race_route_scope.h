#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <string_view>
#include <utility>

namespace dingosdk::modes {
// Placement persistence identifies the donor world (for example BAM). A
// Deathrace belongs to the actual detached level, which can share that donor
// with many unrelated custom maps. Never use a display name as its identity.
inline std::string race_route_map_key(std::string_view destination) {
    const auto split=destination.find('|');
    auto asset=split==std::string_view::npos?destination:
        split+1<destination.size()?destination.substr(split+1):destination.substr(0,split);
    std::string key(asset);
    for(auto& c:key) {
        if(c=='\\')c='/';
        else if(c>='A' && c<='Z')c=static_cast<char>(c+('a'-'A'));
    }
    // Retain the original retail route keys; custom levels retain their full
    // path so two mods with similar names cannot share a saved-route list.
    constexpr std::array<std::pair<std::string_view,std::string_view>,7> retail{{
        {"levels/game/bam_levelroot/bam_levelroot","bam"},
        {"levels/game/dingolevel_root/dingolevel_root","bam"},
        {"levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom","grom"},
        {"levels/game/dingolevel_ftue_island/dingolevel_ftue_island","ftue"},
        {"levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001","stadium_1"},
        {"levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002","stadium_2"},
        {"levels/game/dingolevel_mpr/dingolevel_mpr","mpr"}
    }};
    for(const auto& [path,legacy]:retail)if(key==path)return std::string(legacy);
    return key;
}
struct RaceRouteWorldScope {
    std::uint64_t local{},world{};
    std::string map;
    bool active{};
    bool update(std::uint64_t player,std::uint64_t generation,bool in_world,std::string_view destination) {
        auto next=in_world?race_route_map_key(destination):std::string{};
        const bool ready=in_world && generation && !next.empty();
        const bool changed=local!=player || world!=generation || active!=ready || map!=next;
        local=player;world=generation;active=ready;map=std::move(next);
        return changed;
    }
};
}
