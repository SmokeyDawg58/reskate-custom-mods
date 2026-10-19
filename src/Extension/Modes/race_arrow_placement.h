#pragma once
#include "race_route.h"

namespace dingosdk::modes {
struct RaceArrowRay {bool available{},hit{};Vec3 at{};};
struct RaceArrowSurface {bool contact{};Vec3 at{},normal{};};
struct RaceArrowAim {bool valid{},align_surface{};Vec3 at{};RaceArrowSurface surface;};
class RaceArrowFloorAlignment {
    bool flat{};
public:
    void reset() noexcept {flat=false;}
    bool update(bool floor_contact,float forward_y,bool manual) noexcept {
        // Engage only on a direct floor hit while looking down at least 45
        // degrees; release at 30 degrees. No distant/downward ground search.
        flat=!manual && floor_contact && std::isfinite(forward_y) &&
            forward_y<=(flat?-.5f:-.70710678f);
        return flat;
    }
};
inline Vec3 arrow_add(const Vec3& a,const Vec3& b,float scale=1.f) noexcept {
    return {a[0]+b[0]*scale,a[1]+b[1]*scale,a[2]+b[2]*scale};
}
inline float arrow_dot(const Vec3& a,const Vec3& b) noexcept {return a[0]*b[0]+a[1]*b[1]+a[2]*b[2];}
inline Vec3 arrow_unit(const Vec3& value) noexcept {
    const float length=std::sqrt(arrow_dot(value,value));
    return length>.0001f?Vec3{value[0]/length,value[1]/length,value[2]/length}:Vec3{};
}
inline Vec3 arrow_cross(const Vec3& a,const Vec3& b) noexcept {
    return {a[1]*b[2]-a[2]*b[1],a[2]*b[0]-a[0]*b[2],a[0]*b[1]-a[1]*b[0]};
}
// Query the same verified collision API as gate placement. A clear ray keeps
// free-air placement; a hit keeps the marker on the visible side of a surface.
// A direct hit mounts to that visible surface. A clear ray stays in the air;
// there is no downward ground search or attraction to another arrow's surface.
// Nearby parallel samples recover its plane without assuming a native normal ABI.
template<class Cast> RaceArrowAim aim_race_arrow(const Vec3& origin,const Vec3& forward,
        const Vec3& right,const Vec3& up,Cast&& cast) {
    RaceArrowAim result;result.at=arrow_add(origin,forward,12.f);
    const auto hit=cast(origin,forward,12.f);
    if(!hit.available)return result;
    result.valid=true;if(!hit.hit)return result;
    result.align_surface=true;
    result.at=hit.at;
    std::array<Vec3,4> nearby{};std::array<bool,4> valid{};
    for(unsigned i=0;i<4;++i) {
        const auto sample=cast(arrow_add(origin,i<2?right:up,i%2?.08f:-.08f),forward,14.f);
        nearby[i]=arrow_add(sample.at,hit.at,-1.f);
        valid[i]=sample.available && sample.hit && arrow_dot(nearby[i],nearby[i])<4.f;
    }
    for(unsigned i=0;i<2;++i)for(unsigned j=2;j<4;++j)if(valid[i] && valid[j]) {
        auto normal=arrow_unit(arrow_cross(nearby[i],nearby[j]));
        if(arrow_dot(normal,normal)<.9f || std::abs(arrow_dot(normal,forward))<.02f)continue;
        if(arrow_dot(normal,forward)>0)for(auto& component:normal)component=-component;
        result.surface={true,hit.at,normal};return result;
    }
    // At a thin edge there may be no reliable mounting plane. Refuse this
    // position rather than pretending the ray missed and placing behind it.
    result.valid=false;return result;
}
inline std::array<float,2> race_arrow_surface_angles(const Vec3& normal) noexcept {
    return {std::atan2(normal[0],normal[2])*180.f/3.14159265f,
        -std::asin(std::clamp(normal[1],-1.f,1.f))*180.f/3.14159265f};
}
inline Vec3 clear_race_arrow_surface(Vec3 at,float yaw,float pitch,float roll,float width,
        const RaceArrowSurface& surface) noexcept {
    if(!surface.contact)return at;
    const auto pose=race_arrow_transform({},yaw,roll,width,pitch);
    const Vec3 right{pose[0],pose[1],pose[2]},up{pose[4],pose[5],pose[6]};
    // Both native chevron meshes are XY quads with authored bounds +/-0.5.
    // Account for the full rotated footprint, plus 4 cm to avoid z-fighting.
    const float support=.5f*(std::abs(arrow_dot(right,surface.normal))+std::abs(arrow_dot(up,surface.normal)))+.04f;
    const float distance=arrow_dot(arrow_add(at,surface.at,-1.f),surface.normal);
    return arrow_add(at,surface.normal,std::max(0.f,support-distance));
}
struct RaceArrowGroundClearance {Vec3 at{};bool available=true;};
// A center aim can miss the road while the bottom of an upright/tilted quad
// intersects it. Probe the footprint from above, where one-sided road meshes
// can be hit. Only lift an intersecting footprint; never pull free air down.
template<class Cast>
#ifdef _MSC_VER
[[msvc::noinline]]
#endif
RaceArrowGroundClearance clear_race_arrow_ground(Vec3 at,float yaw,float pitch,
        float roll,float width,Cast&& cast) {
    const auto pose=race_arrow_transform(at,yaw,roll,width,pitch);
    const float top=at[1]+.5f*(std::abs(pose[1])+std::abs(pose[5]))+.25f;
    float lift{};bool available=true;
    for(int x=-1;x<=1;++x)for(int y=-1;y<=1;++y) {
        Vec3 point{};for(unsigned axis=0;axis<3;++axis)
            point[axis]=at[axis]+float(x)*.5f*pose[axis]+float(y)*.5f*pose[4+axis];
        const auto hit=cast(Vec3{point[0],top,point[2]},Vec3{0,-1,0},top-point[1]+.04f);
        available=available && hit.available;
        if(hit.available && hit.hit)lift=std::max(lift,hit.at[1]+.04f-point[1]);
    }
    at[1]+=lift;return {at,available};
}
template<class Cast> bool race_arrow_clear(const Vec3& origin,const Vec3& at,float yaw,float pitch,
        float roll,float width,Cast&& cast) {
    const auto pose=race_arrow_transform(at,yaw,roll,width,pitch);
    const auto clear_segment=[&](const Vec3& from,const Vec3& to) {
        const auto delta=arrow_add(to,from,-1.f);const float length=std::sqrt(arrow_dot(delta,delta));
        if(!(length>.1f && length<500.f))return false;
        const auto direction=arrow_unit(delta);const auto hit=cast(from,direction,length);
        if(!hit.available)return false;
        return !hit.hit || arrow_dot(arrow_add(hit.at,from,-1.f),direction)>=length-.015f;
    };
    // The center cannot be placed behind the aimed obstruction. Test actual
    // segments on the quad, rather than camera visibility rays to its corners:
    // a foreground curb/tree must not invalidate an otherwise clear arrow.
    if(!clear_segment(origin,at))return false;
    const auto point=[&](float x,float y) {
        Vec3 result{};for(unsigned axis=0;axis<3;++axis)result[axis]=at[axis]+x*pose[axis]+y*pose[4+axis];
        return result;
    };
    for(int line=-1;line<=1;++line) {
        const float offset=.5f*float(line);
        if(!clear_segment(point(-.5f,offset),point(.5f,offset)) ||
           !clear_segment(point(offset,-.5f),point(offset,.5f)))return false;
    }
    if(!clear_segment(point(-.5f,-.5f),point(.5f,.5f)) ||
       !clear_segment(point(-.5f,.5f),point(.5f,-.5f)))return false;
    return true;
}
inline bool race_arrow_preview_visible(bool ready,bool rotating,bool angles_ready) noexcept {
    return ready || (rotating && angles_ready);
}
} // namespace dingosdk::modes
