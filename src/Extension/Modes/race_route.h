#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <cmath>

namespace dingosdk::modes {
enum class RaceObject : unsigned { start, checkpoint, finish };
inline constexpr RaceObject cycle_race_object(RaceObject object, bool previous) noexcept {
    // The native flag picker already establishes the start. The route editor
    // offers only checkpoints and its single finish.
    (void)previous;
    return object==RaceObject::checkpoint?RaceObject::finish:RaceObject::checkpoint;
}
inline std::array<float,16> race_spawn_transform(const Vec3& at, float heading) noexcept {
    const float yaw=heading*3.14159265f/180.f, co=std::cos(yaw), si=std::sin(yaw);
    return {co,0,-si,0,0,1,0,0,si,0,co,0,at[0],at[1]+.3f,at[2],1};
}
struct RaceGateCrossing { bool crossed{}, inside{}; Vec3 at{}; };
inline RaceGateCrossing race_gate_crossing(const Settings& settings,std::size_t index,const Vec3& before,const Vec3& after) noexcept {
    if(index>=settings.points.size())return {};
    const auto& gate=settings.points[index];
    const auto from=index?settings.points[index-1]:settings.points[index];
    const auto to=index?settings.points[index]:index+1<settings.points.size()?settings.points[index+1]:settings.points[index];
    const float heading=index<settings.yaws.size()?settings.yaws[index]:std::atan2(to[0]-from[0],to[2]-from[2])*180.f/3.14159265f;
    const float yaw=heading*3.14159265f/180.f;
    const float fx=std::sin(yaw),fz=std::cos(yaw);
    const auto along=[&](const Vec3& p){return (p[0]-gate[0])*fx+(p[2]-gate[2])*fz;};
    const float a=along(before),b=along(after);
    if((a<0)==(b<0) || a==b)return {};
    const float fraction=a/(a-b);
    Vec3 at{};for(unsigned axis=0;axis<3;++axis)at[axis]=before[axis]+(after[axis]-before[axis])*fraction;
    const float across=std::abs((at[0]-gate[0])*fz-(at[2]-gate[2])*fx);
    return {true,across<=gate_half_width(settings,index) && std::abs(at[1]-gate[1])<=10.f,at};
}
// Builder mutations keep pose arrays aligned and reserve the last slot for
// exactly one finish. The wire continues to send the route in crossing order.
inline bool edit_race_route(Settings& settings, bool& finish, RaceObject object, const Vec3& at, float yaw, float width) {
    if(settings.mode!=Mode::race || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size() ||
       !std::isfinite(yaw) || !std::isfinite(width) || width<min_gate_half_width || width>max_gate_half_width ||
       !std::all_of(at.begin(),at.end(),[](float v){return std::isfinite(v) && std::abs(v)<100000;}))return false;
    if(object==RaceObject::start) {
        if(settings.points.empty()) {settings.points.push_back(at);settings.yaws.push_back(yaw);settings.widths.push_back(width);}
        else {settings.points[0]=at;settings.yaws[0]=yaw;settings.widths[0]=width;}
        settings.spawn=at;settings.has_spawn=true;return true;
    }
    if(settings.points.empty())return false;
    auto index=settings.points.size()-(finish?1:0);
    if(object==RaceObject::finish) {
        if(index<2 || (!finish && settings.points.size()>=max_points))return false;
        if(finish) {settings.points.back()=at;settings.yaws.back()=yaw;settings.widths.back()=width;settings.race_finish=true;return true;}
        finish=true;settings.race_finish=true;
    } else if(index>=max_points-1)return false;
    settings.points.insert(settings.points.begin()+index,at);
    settings.yaws.insert(settings.yaws.begin()+index,yaw);
    settings.widths.insert(settings.widths.begin()+index,width);
    return true;
}
inline bool undo_race_gate(Settings& settings, bool& finish) {
    if(settings.points.size()<=1 || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size())return false;
    settings.points.pop_back();settings.yaws.pop_back();settings.widths.pop_back();finish=false;settings.race_finish=false;return true;
}
inline bool race_route_ready(const Settings& settings, bool finish) noexcept {
    return finish && settings.race_finish && settings.has_spawn && settings.points.size()>=3 && settings.points.size()<=max_points &&
        settings.points.size()==settings.yaws.size() && settings.points.size()==settings.widths.size();
}
enum class RaceFlagTransition { rejected, edit_route, wait_for_start };
// A saved course keeps every gate and its single finish when its native start
// flag is placed again. A first flag opens the builder; neither starts a race.
inline RaceFlagTransition accept_race_flag(Settings& settings, bool& finish, const Vec3& at, float yaw, float width) {
    const bool saved=race_route_ready(settings,finish);
    if(!edit_race_route(settings,finish,RaceObject::start,at,yaw,width))return RaceFlagTransition::rejected;
    return saved?RaceFlagTransition::wait_for_start:RaceFlagTransition::edit_route;
}
}
