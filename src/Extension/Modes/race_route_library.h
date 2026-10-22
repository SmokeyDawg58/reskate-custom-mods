#pragma once
#include "race_route.h"
#include <charconv>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <tuple>


namespace dingosdk::modes {
struct NamedRaceRoute {
    std::uint64_t id{};
    std::string name,map;
    Settings settings;
};
inline std::string race_route_name(std::string name) {
    const auto first=name.find_first_not_of(" \t\r\n"),last=name.find_last_not_of(" \t\r\n");
    if(first==std::string::npos)return {};
    name=name.substr(first,last-first+1);
    if(name.size()>64 || std::any_of(name.begin(),name.end(),[](unsigned char ch){return ch<32 || ch==127;}))return {};
    return name;
}
inline std::vector<std::uint8_t> encode_named_race(const NamedRaceRoute& route) {
    const auto name=race_route_name(route.name);
    if(name.empty() || route.map.empty() || route.map.size()>512 || !race_route_ready(route.settings,route.settings.race_finish))
        throw std::invalid_argument("Name the completed route before saving.");
    Message message;message.kind=Message::Kind::setup;message.game=1;message.leader=1;message.settings=route.settings;
    const auto settings=encode(message);
    std::vector<std::uint8_t> bytes{'R','S','R','O','U','T','E',1};
    const auto number=[&](std::size_t n){for(unsigned i=0;i<4;++i)bytes.push_back(static_cast<std::uint8_t>((n>>(8*i))&255));};
    number(name.size());number(route.map.size());number(settings.size());
    bytes.insert(bytes.end(),name.begin(),name.end());bytes.insert(bytes.end(),route.map.begin(),route.map.end());
    bytes.insert(bytes.end(),settings.begin(),settings.end());return bytes;
}
inline std::optional<NamedRaceRoute> decode_named_race(std::span<const std::uint8_t> bytes) {
    constexpr std::array<std::uint8_t,8> magic{'R','S','R','O','U','T','E',1};
    if(bytes.size()<20 || bytes.size()>65536 || !std::equal(magic.begin(),magic.end(),bytes.begin()))return {};
    const auto number=[&](unsigned offset){std::uint32_t value{};for(unsigned i=0;i<4;++i)value|=std::uint32_t(bytes[offset+i])<<(8*i);return value;};
    const auto names=number(8),maps=number(12),settings=number(16);
    if(!names || names>64 || !maps || maps>512 || settings>64000 || std::uint64_t(20)+names+maps+settings!=bytes.size())return {};
    NamedRaceRoute route;
    route.name.assign(reinterpret_cast<const char*>(bytes.data()+20),names);
    route.map.assign(reinterpret_cast<const char*>(bytes.data()+20+names),maps);
    if(race_route_name(route.name)!=route.name || route.map.find('\0')!=std::string::npos)return {};
    const auto message=decode(bytes.subspan(20+names+maps,settings));
    if(!message || message->kind!=Message::Kind::setup || message->settings.mode!=Mode::race ||
       !race_route_ready(message->settings,message->settings.race_finish))return {};
    route.settings=message->settings;return route;
}
class RaceRouteLibrary {
    std::filesystem::path directory_,legacy_directory_;
    std::vector<NamedRaceRoute> routes_;
    std::map<std::uint64_t,std::filesystem::path> paths_;
    std::uint64_t next_=1;
    void migrate_legacy_directory();
public:
    explicit RaceRouteLibrary(std::filesystem::path directory,std::filesystem::path legacy={}):directory_(std::move(directory)),legacy_directory_(std::move(legacy)) {}
    const std::vector<NamedRaceRoute>& routes() const noexcept {return routes_;}
    void read();
    void erase(std::uint64_t id);
    std::filesystem::path path(std::uint64_t id) const;
    std::uint64_t import_json(const std::filesystem::path& source,const std::string& current_map);
    const NamedRaceRoute* find(std::uint64_t id) const noexcept {
        const auto it=std::find_if(routes_.begin(),routes_.end(),[&](const auto& route){return route.id==id;});
        return it==routes_.end()?nullptr:&*it;
    }
    std::uint64_t save(NamedRaceRoute route);
    void rename(std::uint64_t id,std::string name) {
        const auto* original=find(id);if(!original)throw std::runtime_error("This saved route is no longer available.");
        auto route=*original;route.name=std::move(name);save(std::move(route));
    }
};
}
