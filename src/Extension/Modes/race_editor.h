#pragma once
#include "race_route.h"
#include <vector>
#include <limits>

namespace dingosdk::modes {
struct RaceEditorDirections { int yaw{},tilt{},size{};bool undo{},redo{}; };
inline RaceEditorDirections race_editor_directions(bool rotating,unsigned step) noexcept {
    const int horizontal=int(bool(step&2))-int(bool(step&1));
    const int vertical=int(bool(step&8))-int(bool(step&4));
    return rotating?RaceEditorDirections{horizontal,vertical,0,false,false}:
        RaceEditorDirections{0,0,vertical,horizontal<0,horizontal>0};
}
class RaceRouteHistory {
    std::vector<Settings> past,future;
public:
    void reset(){past.clear();future.clear();}
    void changed(const Settings& before,const Settings& after) {
        if(before==after)return;
        if(past.size()==64)past.erase(past.begin());
        past.push_back(before);future.clear();
    }
    bool undo(Settings& route,bool& finish) {
        if(past.empty())return false;
        future.push_back(route);route=std::move(past.back());past.pop_back();finish=route.race_finish;return true;
    }
    bool redo(Settings& route,bool& finish) {
        if(future.empty())return false;
        past.push_back(route);route=std::move(future.back());future.pop_back();finish=route.race_finish;return true;
    }
};
inline float tilt_race_gate(float pitch,int direction,bool snap,float degrees) noexcept {
    return std::clamp(rotate_race_gate(pitch,direction,snap,degrees),-90.f,90.f);
}
inline constexpr float default_race_editor_width=7.5f;
inline constexpr unsigned default_race_editor_snap_angle=4;
inline float race_camera_yaw(float camera,float offset,bool arrow) noexcept {
    return std::remainder(camera+offset+(arrow?180.f:0.f),360.f);
}
inline float race_camera_pitch(float camera,float offset,bool arrow,bool surface=false) noexcept {
    // Looking across open air keeps a vertical sign. A real surface hit still
    // supplies the floor/wall tilt, and X retains an explicit manual offset.
    return std::clamp((arrow && surface?camera:0.f)+offset,-90.f,90.f);
}
inline Vec3 snap_race_arrow_height(const Settings& settings,Vec3 at,bool enabled,bool& snapped) noexcept {
    snapped=false;if(!enabled)return at;
    float nearest=std::numeric_limits<float>::max();const float original_height=at[1];
    for(std::size_t i=1;i<settings.points.size();++i)if(arrow_marker(settings,i)) {
        const auto& other=settings.points[i];const float dx=at[0]-other[0],dz=at[2]-other[2],distance=dx*dx+dz*dz;
        const float reach=std::max(24.f,gate_half_width(settings,i)*4.f),height_reach=1.f;
        if(distance<=reach*reach && distance<=nearest && std::abs(original_height-other[1])<=height_reach) {
            nearest=distance;at[1]=other[1];snapped=true;
        }
    }
    return at;
}
// Capture a row on the previous marker's authored axes, never the noisy
// segment between two placements. Lock its whole pose so camera movement
// cannot give consecutive upright arrows different headings or tilts.
struct RaceArrowSnapPose {Vec3 at{};float yaw{},pitch{},roll{};};
inline float race_arrow_snap_angle(float angle,float step) noexcept {
    return step>0.f?std::remainder(std::round(angle/step)*step,360.f):angle;
}
inline RaceArrowSnapPose snap_race_arrow_angles(RaceArrowSnapPose pose,bool enabled,float step) noexcept {
    if(enabled){pose.yaw=race_arrow_snap_angle(pose.yaw,step);pose.pitch=std::clamp(race_arrow_snap_angle(pose.pitch,step),-90.f,90.f);pose.roll=race_arrow_snap_angle(pose.roll,step);}
    return pose;
}
class RaceArrowLineSnap {
    bool locked{},upright_locked{};Vec3 anchor{},direction{};float yaw{},pitch{},roll{};
    Vec3 last_raw{},input_bias{};bool raw_valid{};
    std::optional<std::size_t> rotation_count;
    static float dot(const Vec3& a,const Vec3& b){return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
    static Vec3 sub(const Vec3& a,const Vec3& b){return {a[0]-b[0],a[1]-b[1],a[2]-b[2]};}
public:
    void reset() noexcept {locked=false;upright_locked=false;raw_valid=false;input_bias={};rotation_count.reset();}
    void begin_rotation(std::size_t count) noexcept {reset();rotation_count=count;}
    // Keep the projection routine out of the large input handler's optimizer
    // graph. The routine itself still receives normal Release optimization.
#ifdef _MSC_VER
    [[msvc::noinline]]
#endif
    RaceArrowSnapPose update(const Settings& route,RaceArrowSnapPose pose,bool enabled,float step,bool& snapped,bool upright=false) noexcept {
        snapped=false;if(!enabled){reset();return pose;}
        pose=snap_race_arrow_angles(pose,true,step);
        // An explicit X rotation starts a new row; do not pull its preview
        // back to the previous row before the first new marker is placed.
        if(rotation_count) {
            if(*rotation_count==route.points.size())return pose;
            rotation_count.reset();
        }
        if(upright && std::abs(pose.pitch)<.001f && std::abs(pose.roll)<.001f) {
            std::optional<std::size_t> recent;
            for(std::size_t i=route.points.size();i>1;) {
                --i;if(!arrow_marker(route,i))continue;
                const float old_roll=i<route.arrow_rolls.size()?route.arrow_rolls[i]:0.f;
                if(std::abs(race_gate_pitch(route,i))<.001f && std::abs(old_roll)<.001f){recent=i;break;}
            }
            if(!recent){reset();return pose;}
            const auto i=*recent;const auto next=route.points[i];const auto delta=sub(pose.at,next);
            const float old_yaw=i<route.yaws.size()?route.yaws[i]:0.f;
            if(dot(delta,delta)>64.f*64.f){reset();return pose;}
            if(upright_locked && yaw==old_yaw) {
                const auto shift=sub(next,anchor);const float along=dot(shift,direction);
                const Vec3 off{shift[0]-direction[0]*along,shift[1]-direction[1]*along,shift[2]-direction[2]*along};
                if(dot(off,off)<.0001f) {
                    if(dot(shift,shift)>.0001f && raw_valid)input_bias=sub(last_raw,next);
                    anchor=next;
                } else reset();
            } else reset();
            const auto intent=sub(delta,input_bias);
            const auto basis=race_arrow_transform({},old_yaw,0,1.5f,0);
            const std::array<Vec3,3> axes{{{basis[0],0,basis[2]},{0,1,0},{basis[8],0,basis[10]}}};
            Vec3 best=axes[2];float strongest=std::abs(dot(intent,best));
            for(const auto& axis:axes)if(const float amount=std::abs(dot(intent,axis));amount>strongest){best=axis;strongest=amount;}
            // Keep small hand/camera drift on the current rail, but a clear
            // movement toward any of the other two axes switches immediately.
            if(!upright_locked || strongest>std::abs(dot(intent,direction))*1.4f+1.f)direction=best;
            anchor=next;yaw=old_yaw;pitch=roll=0;
            const float along=dot(intent,direction);Vec3 projected{};
            for(unsigned axis=0;axis<3;++axis)projected[axis]=next[axis]+direction[axis]*along;
            locked=upright_locked=locked || strongest>=1.f;
            last_raw=pose.at;raw_valid=true;snapped=true;
            return {projected,yaw,0,0};
        }
        if(upright_locked){locked=false;upright_locked=false;}
        std::optional<std::size_t> recent;
        for(std::size_t i=route.points.size();i>1;) {
            --i;if(!arrow_marker(route,i))continue;
            const float old_yaw=i<route.yaws.size()?route.yaws[i]:0.f;
            const float old_roll=i<route.arrow_rolls.size()?route.arrow_rolls[i]:0.f;
            const auto prior=snap_race_arrow_angles({route.points[i],old_yaw,race_gate_pitch(route,i),old_roll},true,step);
            if(std::abs(prior.pitch-pose.pitch)>35.f || std::abs(std::remainder(prior.yaw-pose.yaw,360.f))>35.f ||
               std::abs(std::remainder(prior.roll-pose.roll,360.f))>35.f)continue;
            recent=i;break;
        }
        if(!recent){reset();return pose;}
        const auto i=*recent;const auto next_anchor=route.points[i];const auto delta=sub(pose.at,next_anchor);
        if(dot(delta,delta)>64.f*64.f){reset();return pose;}
        const auto prior=snap_race_arrow_angles({next_anchor,i<route.yaws.size()?route.yaws[i]:0.f,
            race_gate_pitch(route,i),i<route.arrow_rolls.size()?route.arrow_rolls[i]:0.f},true,step);
        const auto project=[&](const Vec3& axis) {
            const float along=dot(delta,axis);Vec3 point{};
            for(unsigned c=0;c<3;++c)point[c]=next_anchor[c]+axis[c]*along;
            return point;
        };
        const auto error=[&](const Vec3& point){const auto d=sub(pose.at,point);return dot(d,d);};
        const auto aligned=[&](Vec3 at){snapped=true;return RaceArrowSnapPose{at,yaw,pitch,roll};};
        if(locked && yaw==prior.yaw && pitch==prior.pitch && roll==prior.roll) {
            // Re-anchor to a newly placed marker only if it remains on the
            // same axis. Undo/Redo and a genuinely new row cannot retain it.
            const auto shift=sub(next_anchor,anchor);const float along=dot(shift,direction);
            const auto off=Vec3{shift[0]-direction[0]*along,shift[1]-direction[1]*along,shift[2]-direction[2]*along};
            if(dot(off,off)<.0001f && error(project(direction))<=36.f){anchor=next_anchor;return aligned(project(direction));}
        }
        reset();float best=16.f;Vec3 selected{};bool found{};
        const auto basis=race_arrow_transform({},prior.yaw,prior.roll,1.5f,prior.pitch);
        for(unsigned start:{0u,4u,8u}) {
            Vec3 axis{basis[start],basis[start+1],basis[start+2]};
            const auto point=project(axis);const float distance=error(point);
            if(distance<=best){best=distance;selected=axis;found=true;}
        }
        if(!found)return pose;
        anchor=next_anchor;direction=selected;yaw=prior.yaw;pitch=prior.pitch;roll=prior.roll;locked=true;
        return aligned(project(direction));
    }
};
struct RaceRotationPose {Vec3 at{};float yaw{},pitch{},width=7.5f;};
class RaceRotationVisual {
    RaceRotationPose current;std::uint64_t previous{};bool initialized{};
public:
    void reset() noexcept {initialized=false;previous=0;}
    RaceRotationPose update(RaceRotationPose target,std::uint64_t now) noexcept {
        if(!initialized || now<previous || now-previous>250) {current=target;initialized=true;previous=now;return current;}
        const float dt=float(now-previous)/1000.f;previous=now;
        const float blend=1.f-std::exp(-dt/.045f);
        for(unsigned axis=0;axis<3;++axis)current.at[axis]+=(target.at[axis]-current.at[axis])*blend;
        current.yaw=std::remainder(current.yaw+std::remainder(target.yaw-current.yaw,360.f)*blend,360.f);
        current.pitch+=(target.pitch-current.pitch)*blend;current.width+=(target.width-current.width)*blend;
        return current;
    }
};

}
