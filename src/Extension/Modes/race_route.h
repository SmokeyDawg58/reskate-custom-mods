#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <cmath>
#include <map>
#include <deque>
#include <string>
#include <optional>

namespace dingosdk::modes {
enum class RaceObject : unsigned { start, checkpoint, finish, guiding, arrow };
inline constexpr RaceObject cycle_race_object(RaceObject object, bool previous) noexcept {
    const auto n=static_cast<unsigned>(object);
    return static_cast<RaceObject>(previous?(n<=1?4:n-1):(n>=4?1:n+1));
}
inline std::array<float,16> race_spawn_transform(const Vec3& at, float heading) noexcept {
    const float yaw=heading*3.14159265f/180.f, co=std::cos(yaw), si=std::sin(yaw);
    return {co,0,-si,0,0,1,0,0,si,0,co,0,at[0],at[1]+.3f,at[2],1};
}
inline std::array<float,16> race_arrow_transform(const Vec3& at,float heading,float roll,float half_width,float pitch=0.f) noexcept {
    const float yaw=heading*3.14159265f/180.f,r=roll*3.14159265f/180.f;
    const float co=std::cos(yaw),si=std::sin(yaw),cr=std::cos(r),sr=std::sin(r),scale=4.f*half_width/6.f;
    const float p=pitch*3.14159265f/180.f,cp=std::cos(p),sp=std::sin(p);
    // The native chevron is an XY quad. Aim supplies yaw and tilt; the
    // D-pad adds roll in that aimed plane. Only the preview follows the camera.
    const Vec3 right{co,0,-si},up{si*sp,cp,co*sp},forward{si*cp,-sp,co*cp};
    std::array<float,16> pose{};
    for(unsigned a=0;a<3;++a){pose[a]=(right[a]*cr+up[a]*sr)*scale;pose[4+a]=(-right[a]*sr+up[a]*cr)*scale;pose[8+a]=forward[a]*scale;pose[12+a]=at[a];}
    pose[15]=1;return pose;
}
inline std::array<float,16> race_arrow_layer_transform(const Vec3& at,float heading,float roll,float width,float pitch,unsigned part) noexcept {
    auto pose=race_arrow_transform(at,heading,roll,width,pitch);
    // These separately spawned native FX layers used exactly the same plane.
    // Keep the translucent blue glow just above the cutout core, so depth
    // precision at distance cannot let that core suppress its own glow.
    if(part==1) {
        const float length=std::hypot(pose[8],pose[9],pose[10]);
        if(length>.001f)for(unsigned axis=0;axis<3;++axis)pose[12+axis]+=.015f*pose[8+axis]/length;
    }
    return pose;
}
// The placed starting flag owns the lineup direction, including on a saved
// course. Only legacy routes without an authored facing infer it from a gate.
inline float race_start_heading(const Settings& route) noexcept {
    if(!route.yaws.empty() && std::isfinite(route.yaws.front()))return route.yaws.front();
    if(!route.points.empty())for(std::size_t i=1;i<route.points.size();++i) {
        if(guiding_gate(route,i))continue;
        const float x=route.points[i][0]-route.points[0][0],z=route.points[i][2]-route.points[0][2];
        if(std::hypot(x,z)>.1f)return std::atan2(x,z)*180.f/3.14159265f;
    }
    return 0.f;
}
inline void align_race_start(Settings& route) {
    if(route.mode!=Mode::race || route.points.size()<2)return;
    const auto heading=race_start_heading(route);
    if(route.yaws.empty())route.yaws.resize(1);
    route.yaws[0]=heading;
}
// A centered row perpendicular to the flag, with everyone behind its plane.
inline Vec3 race_start_position(const Settings& route,float index,float count) noexcept {
    if(route.points.empty())return route.spawn;
    const auto& start=route.points.front();
    const float yaw=race_start_heading(route)*3.14159265f/180.f,dx=std::sin(yaw),dz=std::cos(yaw);
    const float across=(index-(count-1.f)*.5f)*2.5f;
    return {start[0]-dz*across-dx*1.5f,start[1],start[2]+dx*across-dz*1.5f};
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
enum class RaceRouteMenuAction {none,load,save};
class RaceRouteMenuPress {
    RaceClearHold hold_;bool previous{},fired{};
public:
    float progress{};
    RaceRouteMenuAction update(bool down,bool focused,std::uint64_t now) noexcept {
        if(!focused) {hold_={};previous=fired=false;progress=0;return RaceRouteMenuAction::none;}
        const bool complete=hold_.update(down,true,now);progress=hold_.progress;
        const bool tap=previous && !down && !fired;
        previous=down;if(complete)fired=true;else if(!down)fired=false;
        return complete?RaceRouteMenuAction::load:tap?RaceRouteMenuAction::save:RaceRouteMenuAction::none;
    }
};
inline void clear_race_gates(Settings& route,bool& finish) {
    // Retain the native start flag while clearing all checkpoints and finish.
    const auto keep=std::min<std::size_t>(1,route.points.size());
    route.points.resize(keep);route.yaws.resize(keep);route.widths.resize(keep);route.gate_kinds.resize(keep);route.arrow_rolls.resize(keep);route.gate_pitches.resize(keep);
    route.race_finish=finish=false;
}
inline float race_gate_height(const Settings& settings,std::size_t index) noexcept {return 10.f*gate_half_width(settings,index)/6.f;}
inline float race_gate_pitch(const Settings& settings,std::size_t index) noexcept {
    return index<settings.gate_pitches.size()?settings.gate_pitches[index]:0.f;
}
struct RaceGateAxes {Vec3 right,up,forward;};
inline RaceGateAxes race_gate_axes(float yaw,float pitch,float roll=0.f) noexcept {
    const auto matrix=race_arrow_transform({},yaw,roll,1.5f,pitch);
    return {{matrix[0],matrix[1],matrix[2]},{matrix[4],matrix[5],matrix[6]},{matrix[8],matrix[9],matrix[10]}};
}
inline float race_gate_dot(const Vec3& p,const Vec3& origin,const Vec3& axis) noexcept {
    return (p[0]-origin[0])*axis[0]+(p[1]-origin[1])*axis[1]+(p[2]-origin[2])*axis[2];
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
    const auto pitch=race_gate_pitch(settings,index);const auto axes=race_gate_axes(heading,pitch);
    const auto along=[&](const Vec3& p){return pitch==0.f?(p[0]-gate[0])*fx+(p[2]-gate[2])*fz:race_gate_dot(p,gate,axes.forward);};
    const float a=along(before),b=along(after);
    if((a<0)==(b<0) || a==b)return {};
    const float fraction=a/(a-b);
    Vec3 at{};for(unsigned axis=0;axis<3;++axis)at[axis]=before[axis]+(after[axis]-before[axis])*fraction;
    const float across=std::abs((at[0]-gate[0])*fz-(at[2]-gate[2])*fx);
    const float up=pitch==0.f?at[1]-gate[1]:race_gate_dot(at,gate,axes.up);
    return {true,across<=gate_half_width(settings,index) && std::abs(up)<=race_gate_height(settings,index),at};
}
inline bool race_gate_missed(const Settings& settings,std::size_t index,const Vec3& before,const Vec3& after) noexcept {
    if(index==0 || index>=settings.points.size() || guiding_gate(settings,index))return false;
    const auto crossing=race_gate_crossing(settings,index,before,after);
    // Going around (or above) a required gate still crosses its plane. Do not
    // restrict this warning to a narrow band outside the posts.
    return crossing.crossed && !crossing.inside;
}
// Signed distance to a gate plane, independent of recovery policy.
inline float race_gate_along(const Settings& route,std::size_t index,const Vec3& at) noexcept {
    if(index>=route.points.size())return 0;
    const float heading=index<route.yaws.size()?route.yaws[index]:index?std::atan2(route.points[index][0]-route.points[index-1][0],route.points[index][2]-route.points[index-1][2])*180.f/3.14159265f:0;
    const float yaw=heading*3.14159265f/180.f;
    if(const auto pitch=race_gate_pitch(route,index);pitch!=0.f)return race_gate_dot(at,route.points[index],race_gate_axes(heading,pitch).forward);
    return (at[0]-route.points[index][0])*std::sin(yaw)+(at[2]-route.points[index][2])*std::cos(yaw);
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
    std::array<std::uint64_t,4> next{},started{};
    void suppress(unsigned held=15) noexcept {blocked=held;previous=0;next={};started={};}
    unsigned update(unsigned held,std::uint64_t now,bool rotating=false) noexcept {
        blocked&=held;held&=~blocked;
        unsigned pressed{};
        for(unsigned i=0;i<4;++i) {
            const auto bit=1u<<i;
            if(!(held&bit)){next[i]=started[i]=0;continue;}
            if(!(previous&bit)){pressed|=bit;started[i]=now;next[i]=now+(rotating?250:350);}
            else if(now>=next[i]) {
                pressed|=bit;
                const auto interval=rotating?(now-started[i]>=900?50u:80u):150u;
                next[i]=now+interval; // Never replay missed repeats after a frame stall.
            }
        }
        previous=held;return pressed;
    }
};
struct RaceGatePose { Vec3 at{}; float yaw{}; };
inline std::size_t previous_required_checkpoint(const Settings& route,std::size_t before) noexcept {
    if(route.points.empty())return 0;
    auto index=std::min(before,route.points.size()-1);
    while(index>0){--index;if(!guiding_gate(route,index))break;}
    return index;
}
// Manual fallback persists through another bail at the unsafe checkpoint. A
// newly cleared Required gate resets it; guides and arrows never become targets.
class RaceCheckpointFallback {
    std::optional<std::size_t> selected_,visible_;
public:
    std::size_t step_back(const Settings& route,std::size_t next) noexcept {
        const auto last=selected_.value_or(previous_required_checkpoint(route,next));
        selected_=previous_required_checkpoint(route,last);visible_=selected_;return *selected_;
    }
    std::size_t target(const Settings& route,std::size_t next) const noexcept {
        return selected_.value_or(previous_required_checkpoint(route,next));
    }
    std::size_t visible_from(std::size_t next) const noexcept {return visible_.value_or(next);}
    void observe_crossings(const Settings& route,std::size_t next,const Vec3& before,const Vec3& after) noexcept {
        if(!selected_ || route.points.empty())return;
        const auto end=std::min(next,route.points.size()-1);
        // Authoritative progress stays cleared during rewind. Re-crossing an
        // older Required gate must still move the local recovery point ahead.
        for(auto i=*selected_;i<end;++i)if(!guiding_gate(route,i) &&
            race_gate_along(route,i,after)>race_gate_along(route,i,before) && race_gate_crossing(route,i,before,after).inside){selected_=i;visible_=i+1;}
    }
    void advanced() noexcept {selected_.reset();visible_.reset();}
};
struct RaceCheckpointBackPress {
    bool down=true;
    bool update(bool held,bool allowed) noexcept {
        const bool fire=allowed && held && !down;down=held || !allowed;return fire;
    }
};
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
inline RaceGatePose race_checkpoint_pose(const Settings& route,std::size_t checkpoint) noexcept {
    if(route.points.empty())return {};
    const auto last=std::min(checkpoint,route.points.size()-1);
    RaceGatePose pose{route.points[last],last<route.yaws.size()?route.yaws[last]:0.f};
    const auto yaw=pose.yaw*3.14159265f/180.f;
    // Leave room to regain control before skating through the gate again.
    // Progress remains cleared; the teleport is never a gate crossing.
    pose.at[0]-=std::sin(yaw)*3.f;pose.at[2]-=std::cos(yaw)*3.f;
    return pose;
}
inline RaceGatePose race_recovery_pose(const Settings& route,std::size_t missed) noexcept {
    return race_checkpoint_pose(route,previous_required_checkpoint(route,missed));
}
// Builder mutations keep pose arrays aligned and reserve the last slot for
// exactly one finish. The wire continues to send the route in crossing order.
inline bool edit_race_route(Settings& settings, bool& finish, RaceObject object, const Vec3& at, float yaw, float width, float roll=0.f,float pitch=0.f) {
    if(settings.mode!=Mode::race || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size() ||
       !std::isfinite(yaw) || !std::isfinite(roll) || std::abs(roll)>360.f || !std::isfinite(pitch) || std::abs(pitch)>90.f || !std::isfinite(width) || width<min_gate_half_width || width>max_gate_half_width ||
       !std::all_of(at.begin(),at.end(),[](float v){return std::isfinite(v) && std::abs(v)<100000;}))return false;
    if(!settings.gate_kinds.empty() && settings.gate_kinds.size()!=settings.points.size())return false;
    if(!settings.arrow_rolls.empty() && settings.arrow_rolls.size()!=settings.points.size())return false;
    if(!settings.gate_pitches.empty() && settings.gate_pitches.size()!=settings.points.size())return false;
    if(object!=RaceObject::start && object!=RaceObject::checkpoint && object!=RaceObject::finish && object!=RaceObject::guiding && object!=RaceObject::arrow)return false;
    if(settings.gate_kinds.empty())settings.gate_kinds.resize(settings.points.size(),RaceGateKind::mandatory);
    if(settings.arrow_rolls.empty())settings.arrow_rolls.resize(settings.points.size());
    if(settings.gate_pitches.empty())settings.gate_pitches.resize(settings.points.size());
    if(object==RaceObject::start) {
        if(settings.points.empty()) {settings.points.push_back(at);settings.yaws.push_back(yaw);settings.widths.push_back(width);settings.gate_kinds.push_back(RaceGateKind::mandatory);settings.arrow_rolls.push_back(0.f);settings.gate_pitches.push_back(0.f);}
        else {settings.points[0]=at;settings.yaws[0]=yaw;settings.widths[0]=width;}
        settings.spawn=at;settings.has_spawn=true;return true;
    }
    if(settings.points.empty())return false;
    auto index=settings.points.size()-(finish?1:0);
    if(object==RaceObject::finish) {
        if(index<2 || !mandatory_gates(settings,index))return false;
        if(finish) {settings.points.back()=at;settings.yaws.back()=yaw;settings.widths.back()=width;settings.gate_pitches.back()=pitch;settings.race_finish=true;return true;}
        finish=true;settings.race_finish=true;
    }
    settings.points.insert(settings.points.begin()+index,at);
    settings.yaws.insert(settings.yaws.begin()+index,yaw);
    settings.widths.insert(settings.widths.begin()+index,width);
    settings.gate_kinds.insert(settings.gate_kinds.begin()+index,object==RaceObject::arrow?RaceGateKind::arrow:object==RaceObject::guiding?RaceGateKind::guiding:RaceGateKind::mandatory);
    settings.arrow_rolls.insert(settings.arrow_rolls.begin()+index,object==RaceObject::arrow?roll:0.f);
    settings.gate_pitches.insert(settings.gate_pitches.begin()+index,pitch);
    return true;
}
inline bool undo_race_gate(Settings& settings, bool& finish) {
    if(settings.points.size()<=1 || settings.points.size()!=settings.yaws.size() || settings.points.size()!=settings.widths.size())return false;
    settings.points.pop_back();settings.yaws.pop_back();settings.widths.pop_back();
    if(!settings.gate_kinds.empty())settings.gate_kinds.pop_back();
    if(!settings.arrow_rolls.empty())settings.arrow_rolls.pop_back();
    if(!settings.gate_pitches.empty())settings.gate_pitches.pop_back();
    finish=false;settings.race_finish=false;return true;
}
inline bool race_route_ready(const Settings& settings, bool finish) noexcept {
    return finish && settings.race_finish && settings.has_spawn && settings.points.size()>=3 &&
        settings.points.size()==settings.yaws.size() && settings.points.size()==settings.widths.size() &&
        (settings.gate_kinds.empty() || settings.gate_kinds.size()==settings.points.size()) &&
        (settings.arrow_rolls.empty() || settings.arrow_rolls.size()==settings.points.size()) &&
        (settings.gate_pitches.empty() || settings.gate_pitches.size()==settings.points.size()) &&
        !guiding_gate(settings,0) && !guiding_gate(settings,settings.points.size()-1) && mandatory_gates(settings,settings.points.size()-1)>0;
}
// A finish mesh is not publication. Only an explicit Save commits the shape;
// any later edit withdraws it until saved again. Timer/player edits keep the shape.
inline bool same_race_shape(const Settings& a,const Settings& b) noexcept {
    return a.mode==b.mode && a.points==b.points && a.yaws==b.yaws && a.widths==b.widths &&
        a.gate_kinds==b.gate_kinds && a.arrow_rolls==b.arrow_rolls && a.gate_pitches==b.gate_pitches &&
        a.race_finish==b.race_finish && a.has_spawn==b.has_spawn && a.spawn==b.spawn;
}
class RaceRoutePublication {
    std::optional<Settings> saved_;
public:
    void begin_edit() noexcept {saved_.reset();}
    bool save(const Settings& route) {if(!race_route_ready(route,route.race_finish))return false;saved_=route;return true;}
    bool ready(const Settings& route) const noexcept {return saved_ && same_race_shape(*saved_,route) && race_route_ready(route,route.race_finish);}
    // An unchanged loaded/saved course is already on disk. Publish its
    // current play copy without making the caller name or write it again.
    bool reuse_saved(const RaceRoutePublication& source,const Settings& route) {
        return source.ready(route) && save(route);
    }
    // Native flag placement is a separate editor phase. A stale Join must not
    // enroll a guest while that picker is open, even when old geometry is valid.
    bool accepting(const Settings& route,bool editing,bool flag_pending) const noexcept {
        return !editing && !flag_pending && ready(route);
    }
};
inline bool race_gate_visible(const Settings& route,std::size_t index,std::size_t from,bool playing) noexcept {
    return !playing || arrow_marker(route,index) || index>=from;
}
// Missing a gate's posts never teleports. A later Required gate proves a
// skipped checkpoint; the Finish is ignored until every Required gate is clear.
inline std::optional<std::size_t> race_later_gate_crossed(const Settings& route,std::size_t expected,
        const Vec3& before,const Vec3& after) noexcept {
    if(expected==0 || expected>=route.points.size())return {};
    const auto end=route.points.size()-(route.race_finish?1u:0u);
    for(auto i=expected+1;i<end;++i)
        if(!guiding_gate(route,i) && race_gate_along(route,i,after)>race_gate_along(route,i,before) &&
            race_gate_crossing(route,i,before,after).inside)return i;
    return {};
}
inline std::string_view race_route_save_error(const Settings& route,bool finish) noexcept {
    if(!finish || !route.race_finish)return "You need a Finish line before saving this route.";
    if(!route.has_spawn || route.points.empty())return "Place the start flag before saving this route.";
    if(!mandatory_gates(route,route.points.size()-1))return "Place at least one Required gate before saving this route.";
    if(!race_route_ready(route,finish))return "The route is incomplete. Check the start, Required gates and Finish line.";
    return {};
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
