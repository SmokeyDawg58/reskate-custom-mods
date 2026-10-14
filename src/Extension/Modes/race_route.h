#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <string>
#include <optional>

namespace dingosdk::modes {
enum class RaceObject : unsigned { start, checkpoint, finish, guiding };
inline constexpr RaceObject cycle_race_object(RaceObject object, bool previous) noexcept {
    if(object==RaceObject::start)return RaceObject::checkpoint;
    if(previous)return object==RaceObject::checkpoint?RaceObject::guiding:
        object==RaceObject::guiding?RaceObject::finish:RaceObject::checkpoint;
    return object==RaceObject::checkpoint?RaceObject::finish:
        object==RaceObject::finish?RaceObject::guiding:RaceObject::checkpoint;
}
inline std::array<float,16> race_spawn_transform(const Vec3& at, float heading) noexcept {
    const float yaw=heading*3.14159265f/180.f, co=std::cos(yaw), si=std::sin(yaw);
    return {co,0,-si,0,0,1,0,0,si,0,co,0,at[0],at[1]+.3f,at[2],1};
}
// A course belongs to a map, independently of any lobby, player or match.
class RaceCourseMemory {
    std::map<std::string,Settings> courses;
public:
    void save(const std::string& map,const Settings& route) {
        if(!map.empty() && route.mode==Mode::race && !route.points.empty())courses[map]=route;
    }
    std::optional<Settings> restore(const std::string& map) const {
        const auto it=courses.find(map);return it==courses.end()?std::nullopt:std::optional(it->second);
    }
    void clear(const std::string& map) {courses.erase(map);}
};
// View is deliberately separate from B/Cancel. A release or loss of focus
// resets progress, and a completed hold fires only once until released.
struct RaceClearHold {
    std::uint64_t since{};bool fired{};
    float progress{};
    bool update(bool down,bool focused,std::uint64_t now) noexcept {
        if(!down || !focused){since=0;fired=false;progress=0;return false;}
        if(fired)return false;
        if(!since)since=now;
        progress=std::min(1.f,float(now-since)/1500.f);
        if(progress<1.f)return false;
        fired=true;return true;
    }
};
inline void clear_race_gates(Settings& route,bool& finish) {
    // Retain the native start flag while clearing all checkpoints and finish.
    const auto keep=std::min<std::size_t>(1,route.points.size());
    route.points.resize(keep);route.yaws.resize(keep);route.widths.resize(keep);route.gate_kinds.resize(keep);
    route.race_finish=finish=false;
}
inline float race_gate_height(const Settings& settings,std::size_t index) noexcept {return 10.f*gate_half_width(settings,index)/6.f;}
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
    return {true,across<=gate_half_width(settings,index) && std::abs(at[1]-gate[1])<=race_gate_height(settings,index),at};
}
inline bool race_gate_missed(const Settings& settings,std::size_t index,const Vec3& before,const Vec3& after) noexcept {
    if(index==0 || index>=settings.points.size() || guiding_gate(settings,index))return false;
    const auto crossing=race_gate_crossing(settings,index,before,after);
    // Going around (or above) a required gate still crosses its plane. Do not
    // restrict this warning to a narrow band outside the posts.
    return crossing.crossed && !crossing.inside;
}
// Let a rider clear the gate plane before recovering. Approaching a later
// large arch is never evidence of missing the current checkpoint.
inline float race_gate_along(const Settings& route,std::size_t index,const Vec3& at) noexcept {
    if(index>=route.points.size())return 0;
    const float heading=index<route.yaws.size()?route.yaws[index]:index?std::atan2(route.points[index][0]-route.points[index-1][0],route.points[index][2]-route.points[index-1][2])*180.f/3.14159265f:0;
    const float yaw=heading*3.14159265f/180.f;
    return (at[0]-route.points[index][0])*std::sin(yaw)+(at[2]-route.points[index][2])*std::cos(yaw);
}
inline bool race_miss_recovery_ready(const Settings& route,std::size_t index,const Vec3& before,const Vec3& after,bool& pending) noexcept {
    const float a=race_gate_along(route,index,before),b=race_gate_along(route,index,after);
    const auto crossing=race_gate_crossing(route,index,before,after);
    if(b<=0 || crossing.inside)pending=false;
    else if(b>a && race_gate_missed(route,index,before,after))pending=true;
    const float grace=std::clamp(gate_half_width(route,index)*.15f,1.5f,4.f);
    return pending && b>=grace;
}
inline float race_snap_reach(float degrees) noexcept {return std::clamp(degrees*.1f,.5f,6.f);}
inline float rotate_race_gate(float yaw,int direction,bool snap,float degrees) noexcept {
    degrees=std::clamp(degrees,5.f,90.f);
    return std::remainder((snap?std::round(yaw/degrees)*degrees:yaw)+direction*degrees,360.f);
}
inline float size_race_gate(float width,int direction) noexcept {
    // Five percentage points relative to the standard six-metre half width.
    return std::clamp(width+direction*.3f,min_gate_half_width,max_gate_half_width);
}
struct RaceEditorPresses {
    unsigned previous{}, blocked{};
    std::array<std::uint64_t,4> next{};
    void suppress(unsigned held=15) noexcept {blocked=held;previous=0;next={};}
    unsigned update(unsigned held,std::uint64_t now) noexcept {
        blocked&=held;held&=~blocked;
        unsigned pressed{};
        for(unsigned i=0;i<4;++i) {
            const auto bit=1u<<i;
            if(!(held&bit)){next[i]=0;continue;}
            if(!(previous&bit)){pressed|=bit;next[i]=now+350;}
            else if(now>=next[i]){pressed|=bit;next[i]=now+150;}
        }
        previous=held;return pressed;
    }
};
struct RaceGatePose { Vec3 at{}; float yaw{}; };
inline RaceGatePose snap_race_gate(const Settings& settings,const Vec3& at,float yaw,bool enabled,float degrees=15.f) noexcept {
    RaceGatePose pose{at,yaw};
    if(!enabled || !std::isfinite(yaw) || !std::all_of(at.begin(),at.end(),[](float n){return std::isfinite(n);}))return pose;
    degrees=std::clamp(degrees,5.f,90.f);
    pose.yaw=std::remainder(std::round(yaw/degrees)*degrees,360.f);
    pose.at[0]=std::round(at[0]*2.f)*.5f;pose.at[2]=std::round(at[2]*2.f)*.5f;
    const auto count=settings.points.size()-(settings.race_finish && !settings.points.empty()?1:0);
    if(!count || count>settings.yaws.size())return pose;
    const auto& previous=settings.points[count-1];
    const float heading=settings.yaws[count-1],angle=heading*3.14159265f/180.f;
    const float fx=std::sin(angle),fz=std::cos(angle),dx=at[0]-previous[0],dz=at[2]-previous[2];
    const float along=dx*fx+dz*fz,across=dx*fz-dz*fx;
    // Preserve the ground hit's height. Only attract a nearby centre line;
    // routes can bend and fork without being pulled across the road.
    if(std::abs(across)<=race_snap_reach(degrees) && std::abs(along)>=3.f) {
        const float spacing=std::round(along);
        pose.at[0]=previous[0]+fx*spacing;pose.at[2]=previous[2]+fz*spacing;
    }
    if(std::abs(std::remainder(yaw-heading,360.f))<=degrees*.5f)pose.yaw=heading;
    return pose;
}
inline RaceGatePose race_recovery_pose(const Settings& route,std::size_t missed) noexcept {
    if(route.points.empty())return {};
    std::size_t last=std::min(missed,route.points.size()-1);
    while(last>0){--last;if(!guiding_gate(route,last))break;}
    RaceGatePose pose{route.points[last],last<route.yaws.size()?route.yaws[last]:0.f};
    const auto yaw=pose.yaw*3.14159265f/180.f;
    // Continue just beyond the cleared gate; do not count the recovery teleport
    // as crossing the next required gate.
    pose.at[0]+=std::sin(yaw)*1.5f;pose.at[2]+=std::cos(yaw)*1.5f;
    return pose;
}
// Builder mutations keep pose arrays aligned and reserve the last slot for
// exactly one finish. The wire continues to send the route in crossing order.
inline bool edit_race_route(Settings& settings, bool& finish, RaceObject object, const Vec3& at, float yaw, float width) {
    if(settings.mode!=Mode::race || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size() ||
       !std::isfinite(yaw) || !std::isfinite(width) || width<min_gate_half_width || width>max_gate_half_width ||
       !std::all_of(at.begin(),at.end(),[](float v){return std::isfinite(v) && std::abs(v)<100000;}))return false;
    if(!settings.gate_kinds.empty() && settings.gate_kinds.size()!=settings.points.size())return false;
    if(object!=RaceObject::start && object!=RaceObject::checkpoint && object!=RaceObject::finish && object!=RaceObject::guiding)return false;
    if(settings.gate_kinds.empty())settings.gate_kinds.resize(settings.points.size(),RaceGateKind::mandatory);
    if(object==RaceObject::start) {
        if(settings.points.empty()) {settings.points.push_back(at);settings.yaws.push_back(yaw);settings.widths.push_back(width);settings.gate_kinds.push_back(RaceGateKind::mandatory);}
        else {settings.points[0]=at;settings.yaws[0]=yaw;settings.widths[0]=width;}
        settings.spawn=at;settings.has_spawn=true;return true;
    }
    if(settings.points.empty())return false;
    auto index=settings.points.size()-(finish?1:0);
    if(object==RaceObject::finish) {
        if(index<2 || !mandatory_gates(settings,index))return false;
        if(finish) {settings.points.back()=at;settings.yaws.back()=yaw;settings.widths.back()=width;settings.race_finish=true;return true;}
        finish=true;settings.race_finish=true;
    }
    settings.points.insert(settings.points.begin()+index,at);
    settings.yaws.insert(settings.yaws.begin()+index,yaw);
    settings.widths.insert(settings.widths.begin()+index,width);
    settings.gate_kinds.insert(settings.gate_kinds.begin()+index,object==RaceObject::guiding?RaceGateKind::guiding:RaceGateKind::mandatory);
    return true;
}
inline bool undo_race_gate(Settings& settings, bool& finish) {
    if(settings.points.size()<=1 || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size())return false;
    settings.points.pop_back();settings.yaws.pop_back();settings.widths.pop_back();
    if(!settings.gate_kinds.empty())settings.gate_kinds.pop_back();
    finish=false;settings.race_finish=false;return true;
}
inline bool race_route_ready(const Settings& settings, bool finish) noexcept {
    return finish && settings.race_finish && settings.has_spawn && settings.points.size()>=3 &&
        settings.points.size()==settings.yaws.size() && settings.points.size()==settings.widths.size() &&
        (settings.gate_kinds.empty() || settings.gate_kinds.size()==settings.points.size()) &&
        !guiding_gate(settings,0) && !guiding_gate(settings,settings.points.size()-1) && mandatory_gates(settings,settings.points.size()-1)>0;
}
enum class RaceFlagTransition { rejected, edit_route };
// A saved course keeps every gate and its single finish when its native start
// flag is placed again. Always open the builder so its owner can edit or clear
// the restored course, or save it unchanged before choosing Start.
inline RaceFlagTransition accept_race_flag(Settings& settings, bool& finish, const Vec3& at, float yaw, float width) {
    if(!edit_race_route(settings,finish,RaceObject::start,at,yaw,width))return RaceFlagTransition::rejected;
    return RaceFlagTransition::edit_route;
}
}
