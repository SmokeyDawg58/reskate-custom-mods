#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>

namespace dingosdk::deathrace_ui {
struct RedlineOptions {
    float scale=1,safe=16,opacity=.75f,effects=.65f;
    bool reduced_motion{},speed=true,mph{},standings=true,guidance=true,classic{};
    void normalize() {
        const auto bounded=[](float v,float fallback,float low,float high){return std::isfinite(v)?std::clamp(v,low,high):fallback;};
        scale=bounded(scale,1,.85f,1.10f);safe=bounded(safe,16,0,48);opacity=bounded(opacity,.75f,.5f,1);effects=bounded(effects,.65f,0,1);
    }
};
struct GateProjection {bool valid{},edge{},behind{};float x{},y{},dx{},dy{},distance{};};
// A navigation target, not a visibility/occlusion claim. The camera is the
// same captured world matrix already used by the builder's numbered gates.
inline GateProjection project_gate(const std::array<float,3>& target,const std::array<float,16>& camera,float fov,float width,float safe=16) {
    GateProjection out;if(!std::isfinite(fov) || fov<=1 || fov>=175 || width<100)return out;
    for(float v:target)if(!std::isfinite(v))return out;for(float v:camera)if(!std::isfinite(v))return out;
    for(unsigned row:{0u,4u,8u}){const float norm=camera[row]*camera[row]+camera[row+1]*camera[row+1]+camera[row+2]*camera[row+2];if(norm<.9f || norm>1.1f)return out;}
    std::array<float,3> delta{};for(unsigned i=0;i<3;++i)delta[i]=target[i]-camera[12+i];
    const auto dot=[&](unsigned row){return delta[0]*camera[row]+delta[1]*camera[row+1]+delta[2]*camera[row+2];};
    const float depth=-dot(8),right=dot(0),up=dot(4),focal=540.f/std::tan(fov*3.14159265f/360.f);
    const float length=std::hypot(right,up);out.valid=true;out.behind=depth<=.1f;
    if(out.behind){out.dx=length>.01f?right/length:0;out.dy=length>.01f?-up/length:1;}
    else {out.dx=right*focal/std::max(.1f,depth);out.dy=-up*focal/std::max(.1f,depth);}
    // Labels remain above the gate, and screen-edge targets stay outside the
    // native compass/D-pad and this HUD's timing assembly.
    const float limit_x=width*.5f-100-safe,limit_y=306-safe;
    const float factor=out.behind?std::min(limit_x/std::max(.001f,std::abs(out.dx)),limit_y/std::max(.001f,std::abs(out.dy))):std::min({1.f,limit_x/std::max(.001f,std::abs(out.dx)),limit_y/std::max(.001f,std::abs(out.dy))});
    out.edge=out.behind || factor<.999f;out.x=width*.5f+out.dx*factor;out.y=560+out.dy*factor;
    // A target directly above the clock uses a small side label; no element
    // is allowed to cover the native compass even on a narrow aspect ratio.
    if(out.y<350 && std::abs(out.x-width*.5f)<300){out.y=350;out.edge=true;}
    if(out.x<560 && out.y>730){out.y=730;out.edge=true;}
    return out;
}
class GatePulse {
    std::uint64_t world_{},at_{};std::uint32_t game_{};unsigned cleared_{},required_{};bool active_{},seen_{};
public:
    void reset(){*this={};}
    float update(std::uint64_t world,std::uint32_t game,bool active,unsigned cleared,unsigned required,std::uint64_t now,bool reduced=false) {
        const bool fresh=!seen_ || world_!=world || game_!=game || required_!=required || !active_ || !active || cleared<cleared_;
        if(fresh)at_=0;else if(cleared>cleared_)at_=now;
        world_=world;game_=game;cleared_=cleared;required_=required;active_=active;seen_=true;
        if(reduced || !at_ || now<at_ || now-at_>=360)return 0;
        const float t=float(now-at_)/360.f;return (1-t)*(1-t);
    }
};
inline float speed_readout(float metres_per_second,bool mph) {return metres_per_second*(mph?2.23693629f:3.6f);}
}
