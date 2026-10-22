#pragma once
#include "deathrace_skate2_art.h"
#include "Extension/Modes/race_skate2_presentation.h"
#include <imgui.h>
#include <algorithm>
#include <cmath>
#include <span>
#include <unordered_map>

namespace dingosdk::deathrace_ui {
// The game thread publishes values only. No entity, physics, asset-domain or
// game UI pointer is ever read by the presentation thread.
class Skate2Ribbons {
    using V=std::array<float,3>;
    struct Copy {std::vector<V> a,b;};
    struct Cached {std::array<Copy,3> layers;std::uint64_t updated{};bool arrow{};};
    std::unordered_map<unsigned,Cached> cache_;std::uint32_t seed_=0x12345678+42;std::uint64_t world_{};std::uint32_t game_{};
    static V add(V a,V b){for(unsigned i=0;i<3;++i)a[i]+=b[i];return a;}
    static V mul(V a,float k){for(auto& v:a)v*=k;return a;}
    float random(){seed_=seed_*0x10dcd+0x29;return float(seed_&0x7fffff)/8388608.f;}
    static V point(std::span<const V> cp,float t){const auto i=std::min<std::size_t>(static_cast<std::size_t>(t),cp.size()-4);const float u=t-float(i);const std::array<float,4> weights{(1-u)*(1-u)*(1-u),4-6*u*u+3*u*u*u,1+3*u+3*u*u-3*u*u*u,u*u*u};V out{};for(unsigned k=0;k<4;++k)out=add(out,mul(cp[i+k],weights[k]/6));return out;}
    struct Vertex {float x,y,z,u,v;};
    static Vertex mix(Vertex a,Vertex b,float t){return {a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t,a.z+(b.z-a.z)*t,a.u+(b.u-a.u)*t,a.v+(b.v-a.v)*t};}
    static float fade(float z){return std::min(std::clamp((z-2.f)/5.f,0.f,1.f),1-std::clamp((z-90.f)/10.f,0.f,1.f));}
    static void triangle(ImDrawList& draw,Vertex a,Vertex b,Vertex c,ImVec2 origin,ImVec2 size,float focal,bool additive){
        // Clip before dividing. Crossing a ribbon never produces infinite
        // screen coordinates or the giant triangles caused by a near-plane hit.
        std::array<Vertex,5> polygon{};unsigned count{};const std::array input{a,b,c};
        for(unsigned i=0;i<3;++i){const auto prev=input[(i+2)%3],next=input[i];const bool pin=prev.z>=.1f,nin=next.z>=.1f;if(pin!=nin)polygon[count++]=mix(prev,next,(.1f-prev.z)/(next.z-prev.z));if(nin)polygon[count++]=next;}
        const auto emit=[&](Vertex p,Vertex q,Vertex r){const std::array v{p,q,r};std::array<ImVec2,3> screen{};for(unsigned i=0;i<3;++i){screen[i]={origin.x+size.x*.5f+v[i].x*focal/v[i].z,origin.y+size.y*.5f-v[i].y*focal/v[i].z};if(!std::isfinite(screen[i].x) || !std::isfinite(screen[i].y))return;}
            if(std::all_of(screen.begin(),screen.end(),[&](auto p){return p.x<origin.x;}) || std::all_of(screen.begin(),screen.end(),[&](auto p){return p.x>origin.x+size.x;}) || std::all_of(screen.begin(),screen.end(),[&](auto p){return p.y<origin.y;}) || std::all_of(screen.begin(),screen.end(),[&](auto p){return p.y>origin.y+size.y;}))return;
            draw.PrimReserve(3,3);const auto base=draw._VtxCurrentIdx;for(unsigned i=0;i<3;++i)draw.PrimWriteIdx(static_cast<ImDrawIdx>(base+i));for(unsigned i=0;i<3;++i){const auto f=static_cast<unsigned>(255*fade(v[i].z));draw.PrimWriteVtx(screen[i],{v[i].u,v[i].v},additive?IM_COL32(f,f,f,255):IM_COL32(255,255,255,f));}
        };
        for(unsigned i=1;i+1<count;++i)emit(polygon[0],polygon[i],polygon[i+1]);
    }
public:
    void reset(){cache_.clear();seed_=0x12345678+42;world_=0;game_=0;}
    bool draw(ImDrawList& draw,ImTextureID atlas,const skate2_ribbons::View& view,std::uint64_t world,std::uint32_t game,std::uint64_t now,ImVec2 origin,ImVec2 size,ImDrawCallback additive_callback){
        if(!atlas || !view.active || !std::isfinite(view.fov) || view.fov<=1 || view.fov>=175 || size.x<=0 || size.y<=0)return false;
        for(float x:view.camera)if(!std::isfinite(x))return false;
        for(unsigned row:{0u,4u,8u}){const auto& m=view.camera;const float norm=m[row]*m[row]+m[row+1]*m[row+1]+m[row+2]*m[row+2];if(norm<.9f || norm>1.1f)return false;}
        if(world_!=world || game_!=game){reset();world_=world;game_=game;}
        struct Item {const skate2_ribbons::Ribbon* ribbon;float distance;};std::vector<Item> selected;
        for(const auto& r:view.ribbons){bool valid=std::isfinite(r.yaw) && std::isfinite(r.pitch) && std::isfinite(r.roll) && std::isfinite(r.width) && r.width>0 && r.width<1000;for(float x:r.at)valid&=std::isfinite(x);if(!valid)continue;const float distance=std::hypot(r.at[0]-view.camera[12],r.at[1]-view.camera[13],r.at[2]-view.camera[14]);if(distance<112)selected.push_back({&r,distance});}
        std::stable_sort(selected.begin(),selected.end(),[](auto a,auto b){if(a.ribbon->preview!=b.ribbon->preview)return a.ribbon->preview;return a.distance<b.distance;});if(selected.size()>32)selected.resize(32);
        std::erase_if(cache_,[&](const auto& e){return std::none_of(selected.begin(),selected.end(),[&](const auto& x){return x.ribbon->id==e.first;});});
        const V camera_right{view.camera[0],view.camera[1],view.camera[2]},camera_up{view.camera[4],view.camera[5],view.camera[6]};
        for(const auto& item:selected){const auto& r=*item.ribbon;auto& cache=cache_[r.id];if(cache.updated && cache.arrow==r.arrow && now>=cache.updated && now-cache.updated<50)continue;cache.updated=now;cache.arrow=r.arrow;
            const std::span<const V> cp=r.arrow?std::span<const V>(skate2_chevron_right):std::span<const V>(skate2_gate_controls);const float blend=std::clamp((item.distance-20.f)/((r.arrow?90.f:91.f)-20.f),0.f,1.f),amplitude=(r.arrow?.1f:.2f)*(1-blend)+.4f*blend,width=.15f*(1-blend)+(r.arrow?1.f:.4f)*blend;const auto across=mul(add(camera_right,camera_up),width);
            for(auto& copy:cache.layers){copy.a.resize(cp.size());copy.b.resize(cp.size());for(unsigned i=0;i<cp.size();++i){const float w=random(),z=random(),y=random(),x=random();static_cast<void>(w);const V jitter{(x-.5f)*amplitude,(y-.5f)*amplitude,(z-.5f)*amplitude};copy.a[i]=add(cp[i],jitter);copy.b[i]=add(copy.a[i],mul(across,-1));}}
        }
        const float focal=size.y/(2*std::tan(view.fov*3.14159265359f/360.f));draw.PushClipRect(origin,{origin.x+size.x,origin.y+size.y},true);draw.PushTextureID(atlas);
        for(unsigned pass=0;pass<2;++pass){if(pass==1)draw.AddCallback(additive_callback,nullptr);
            for(const auto& item:selected){const auto& r=*item.ribbon;const auto* art=skate2_sprite(pass==0?"gate-back":r.arrow?"chevron-orange":r.finish?"gate-red":r.guide?"gate-gray":"gate-yellow");if(!art)continue;const float yaw=r.yaw*3.14159265359f/180,pitch=r.pitch*3.14159265359f/180,roll=r.roll*3.14159265359f/180,co=std::cos(yaw),si=std::sin(yaw),cp=std::cos(pitch),sp=std::sin(pitch),cr=std::cos(roll),sr=std::sin(roll);const V right{co,0,-si},up{si*sp,cp,co*sp},forward{si*cp,-sp,co*cp};const V rr=add(mul(right,cr),mul(up,sr)),uu=add(mul(right,-sr),mul(up,cr));const float scale=r.arrow?r.width/3.f:r.width/skate2_gate_half_width;
                const auto transform=[&](V p,unsigned edge){if(r.arrow){const float x=p[0];p[0]=-p[1];p[1]=x;}else p[1]+=skate2_gate_half_height;auto world_p=add(r.at,add(add(mul(rr,p[0]*scale),mul(uu,p[1]*scale)),mul(forward,p[2]*scale)));const V delta{world_p[0]-view.camera[12],world_p[1]-view.camera[13],world_p[2]-view.camera[14]};const auto dot=[&](unsigned row){return delta[0]*view.camera[row]+delta[1]*view.camera[row+1]+delta[2]*view.camera[row+2];};return Vertex{dot(0),dot(4),-dot(8),(art->uv[0]+art->uv[2])*.5f,edge?art->uv[3]:art->uv[1]};};
                for(const auto& copy:cache_[r.id].layers){const unsigned samples=static_cast<unsigned>((copy.a.size()-3)*10);Vertex a{},b{};for(unsigned i=0;i<samples;++i){const float t=float(i)*.1f;const auto c=transform(point(copy.a,t),0),d=transform(point(copy.b,t),1);if(i){triangle(draw,a,b,c,origin,size,focal,pass==1);triangle(draw,c,b,d,origin,size,focal,pass==1);}a=c;b=d;}}
            }
        }
        draw.AddCallback(ImDrawCallback_ResetRenderState,nullptr);draw.PopTextureID();draw.PopClipRect();return true;
    }
};
}
