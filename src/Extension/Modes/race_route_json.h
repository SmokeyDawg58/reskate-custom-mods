#pragma once
#include "race_route_library.h"
#include "Engine/Core/Json/json.h"

namespace dingosdk::modes {
inline std::string encode_named_race_json(const NamedRaceRoute& route) {
    if(encode_named_race(route).size()>65536)throw std::runtime_error("This route is too large to save.");
    const auto& s=route.settings;
    Json kinds=Json::array();
    for(const auto k:s.gate_kinds)kinds.push_back(k==RaceGateKind::arrow?"arrow":k==RaceGateKind::guiding?"guide":"required");
    Json data=Json::object();data["format"]="ReSkate Deathrace route";data["version"]=1;
    data["id"]=route.id;data["name"]=race_route_name(route.name);data["map"]=route.map;
    data["starting_flag"]={{"position",s.points.front()},{"yaw_degrees",race_start_heading(s)}};
    data["settings"]={{"duration_seconds",s.duration_s},{"turn_seconds",s.turn_s},{"strikes",s.strikes},
        {"radius",s.radius},{"area_radius",s.area_radius},{"corners",s.corners},{"points",s.points},
        {"yaw_degrees",s.yaws},{"half_widths",s.widths},{"gate_types",kinds},
        {"arrow_roll_degrees",s.arrow_rolls},{"pitch_degrees",s.gate_pitches},
        {"spawn",s.spawn},{"has_spawn",s.has_spawn},{"finish_gate",s.race_finish},{"trick_kinds",s.trick_kinds}};
    return data.dump(2)+"\n";
}
inline std::optional<NamedRaceRoute> decode_named_race_json(std::string_view text) {
    try {
        const auto j=Json::parse(text,JsonLimits{.bytes=65536,.depth=12,.events=32768});
        if(j.at("format").get<std::string>()!="ReSkate Deathrace route" || !j.at("version").is_number_integer() || j.at("version").get<int>()!=1)return {};
        NamedRaceRoute r;r.name=j.at("name").get<std::string>();r.map=j.at("map").get<std::string>();
        if(j.contains("id")) {
            if(!j.at("id").is_number_integer() || j.at("id").get<double>()<0 || j.at("id").get<double>()>=double(UINT64_MAX))return {};
            r.id=j.at("id").get<std::uint64_t>();
        }
        if(race_route_name(r.name)!=r.name || r.map.find('\0')!=std::string::npos)return {};
        const auto& v=j.at("settings");auto& s=r.settings;s.mode=Mode::race;
        const auto integer=[&](const char* key,unsigned maximum) {
            const auto& n=v.at(key);if(!n.is_number_integer() || n.get<double>()<0 || n.get<double>()>maximum)throw std::runtime_error("Invalid route setting");
            return n.get<unsigned>();
        };
        const auto number=[](const Json& n) {const auto value=n.get<double>();if(!std::isfinite(value) || std::abs(value)>10000000)throw std::runtime_error("Invalid route coordinate");return static_cast<float>(value);};
        const auto point=[&](const Json& value) {if(!value.is_array() || value.size()!=3)throw std::runtime_error("Invalid route position");return Vec3{number(value[0]),number(value[1]),number(value[2])};};
        const auto floats=[&](const char* key) {const auto& a=v.at(key);if(!a.is_array() || a.size()>2048)throw std::runtime_error("Invalid route array");std::vector<float> out;for(const auto& n:a)out.push_back(number(n));return out;};
        const auto points=[&](const char* key) {const auto& a=v.at(key);if(!a.is_array() || a.size()>2048)throw std::runtime_error("Invalid route array");std::vector<Vec3> out;for(const auto& n:a)out.push_back(point(n));return out;};
        s.duration_s=integer("duration_seconds",3600);s.turn_s=integer("turn_seconds",120);s.strikes=static_cast<std::uint8_t>(integer("strikes",5));
        s.radius=number(v.at("radius"));s.area_radius=number(v.at("area_radius"));s.corners=points("corners");s.points=points("points");
        s.yaws=floats("yaw_degrees");s.widths=floats("half_widths");s.arrow_rolls=floats("arrow_roll_degrees");s.gate_pitches=floats("pitch_degrees");
        const auto& kinds=v.at("gate_types");if(!kinds.is_array() || kinds.size()>2048)return {};
        for(const auto& kind:kinds){const auto k=kind.get<std::string>();if(k!="required" && k!="guide" && k!="arrow")return {};s.gate_kinds.push_back(k=="arrow"?RaceGateKind::arrow:k=="guide"?RaceGateKind::guiding:RaceGateKind::mandatory);}
        s.spawn=point(v.at("spawn"));s.has_spawn=v.at("has_spawn").get<bool>();s.race_finish=v.at("finish_gate").get<bool>();s.trick_kinds=static_cast<std::uint8_t>(integer("trick_kinds",15));
        // Older JSON/binary routes already kept the first point and yaw. The
        // descriptive field makes that pose easy to find in newly shared files.
        if(j.contains("starting_flag")){
            const auto& flag=j.at("starting_flag");const auto at=point(flag.at("position"));const auto yaw=number(flag.at("yaw_degrees"));
            if(s.points.empty() || at!=s.points.front() || yaw!=race_start_heading(s))return {};
        }
        (void)encode_named_race(r);return r;
    }catch(const std::exception&){return {};}
}
}
