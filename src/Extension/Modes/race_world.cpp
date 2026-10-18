#include "race_world.h"
#include "race_asset_contract.h"
#include "race_source_contract.h"
#include "race_group_reconciliation.h"
#include "race_route.h"
#include "game_modes.h"
#include "Engine/Core/Platform/memory.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Abi/native_data.h"
#include "Engine/Game/Build/addresses.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/ai_skaters.h"
#include "Engine/Game/Build/20260929/skater_entities.h"
#include "Extension/Multiplayer/Remote/native_skater.h"
#include <Windows.h>
#include <intrin.h>
#include <algorithm>
#include <array>
#include <cmath>
#include <cstring>
#include <format>
#include <mutex>
#include <optional>
#include <stdexcept>
#include <set>
#include <vector>

namespace dingosdk::modes::race_world {
namespace {
namespace e=addr::skater_entities;
namespace b=addr::ai_skaters;
using Matrix=std::array<float,16>;
template<class T> T get(std::uintptr_t p) {T v{};memory::peek(p,v);return v;}
std::uintptr_t ptr(std::uintptr_t p,std::uintptr_t offset=0) {return get<std::uintptr_t>(p+offset);}
void require(bool ok,const char* why) {if(!ok)throw std::runtime_error(why);}
template<std::size_t N> void prefix(std::uintptr_t base,std::uintptr_t rva,const std::array<unsigned char,N>& expected) {
    std::array<unsigned char,N> actual{};require(memory::peek_bytes(base+rva,actual.data(),N) && actual==expected,"Deathrace entity ABI differs.");
}
struct Gate {Vec3 at{};float yaw{},width{};bool finish{},guiding{},arrow{};float roll{},pitch{};bool operator==(const Gate&) const=default;};
struct Entity {std::uintptr_t entity{},owner{},context{};unsigned group{},part{};};
struct State {
    detail::CheckpointWorldLifetime lifetime;
    std::mutex mutex;std::vector<Entity> entities;std::vector<Gate> gates;
    std::array<std::uintptr_t,11> assets{};std::array<std::uint16_t,11> asset_domains{};
    std::uintptr_t context{};std::uint64_t world{},retry{},preview_at{};
    std::uint32_t game{},ready_game{};std::uint64_t ready_world{};
    bool verified{},route_ready{};std::string error,waiting;
    std::set<unsigned> dirty;
    // R.E.S.K.A.T.E.'s trick zone: skate.'s own traffic cone around the circle's edge.
    std::uintptr_t cone{};std::uint16_t cone_domain{};unsigned cone_name{};std::vector<Vec3> cone_points;
    // And a beacon over its middle, seen from afar: Deathrace's lightning bolt, high up and turning.
    std::uintptr_t bolt{};std::uint16_t bolt_domain{};std::uint64_t bolt_retry{},bolt_moved{};
};
State& state(){static State s;return s;}
// R.E.S.K.A.T.E.'s zone cones: a copy packed with the Throwdown assets first (resident everywhere),
// else skate.'s own traffic cone wherever its world content has it loaded, else (custom maps,
// which do not load it) the Throwdown pack's grey Deathrace guide bolt, which every map has.
constexpr std::array<const char*,3> cone_names{
    "World/ReSkate/Throwdowns/TrafficCone",
    "World/Props/Common/Construction/CMN_PRP_TrafficCone_01/CMN_PRP_TrafficCone_01",
    "World/ReSkate/Deathrace/SM_FX_GP_LightningBolt_A_Guide"};
void waiting(State& s,std::string issue) {
    if(s.waiting==issue)return;
    s.waiting=std::move(issue);
    logging::log(logging::Level::info,logging::Channel::runtime,"Native world objects (Deathrace gates, R.E.S.K.A.T.E. cones): {}",s.waiting);
}
constexpr std::array<const char*,11> checkpoint_names{
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate",
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AL_Gate",
    "World/ReSkate/Deathrace/SM_FX_GP_LightningBolt_A_Gate",
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish",
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AL_Finish",
    "World/ReSkate/Deathrace/SM_FX_GP_LightningBolt_A_Finish",
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide",
    "World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AL_Guide",
    "World/ReSkate/Deathrace/SM_FX_GP_LightningBolt_A_Guide",
    "World/ReSkate/Deathrace/FloatingArrow_AT",
    "World/ReSkate/Deathrace/FloatingArrow_A"};
// A full domain lookup is bounded and done only when entering a race or loading
// its bundle. Nine known-domain lookups validate the cached pointers thereafter.
// UI assets are in a child domain, so searching the skater's parents alone
// cannot find a private checkpoint loaded by the Throwdowns picker.
bool load_assets(std::uintptr_t base,State& s) {
    const auto find=game::native_data().find_asset;if(!find)return false;
    for(std::uint16_t domain=0;domain<0xbbf;++domain) {
        if(!ptr(base+addr::engine::domain_owners,std::uintptr_t(domain)*8))continue;
        for(unsigned i=0;i<checkpoint_names.size();++i)if(!s.assets[i]) {
            const auto asset=find(domain,checkpoint_names[i]);
            if(detail::checkpoint_blueprint(asset,base,[](auto at){return ptr(at);})) {
                s.assets[i]=asset;s.asset_domains[i]=domain;
            }
        }
        if(std::all_of(s.assets.begin(),s.assets.end(),[](auto a){return a!=0;}))return true;
    }
    return false;
}
bool assets_resident(std::uintptr_t base,const State& s) {
    const auto find=game::native_data().find_asset;if(!find)return false;
    for(unsigned i=0;i<s.assets.size();++i) {
        if(!s.assets[i])continue;
        if(!detail::checkpoint_resident(s.assets[i],find(s.asset_domains[i],checkpoint_names[i]),base,
            [](auto at){return ptr(at);}))return false;
    }
    return true;
}
std::uintptr_t parent_bus(std::uintptr_t base,std::uintptr_t context) {
    return detail::checkpoint_parent_bus(base,context,[](auto at){return ptr(at);},
        [](auto at){return get<std::uint32_t>(at);},[](auto at){return get<std::uint8_t>(at);},
        [](auto at)->std::string {
            std::array<char,256> text{};
            if(!at || !memory::peek_bytes(at,text.data(),text.size()) ||
                std::find(text.begin(),text.end(),'\0')==text.end())return {};
            return text.data();
        });
}
Entity create(std::uintptr_t base,std::uintptr_t parent,std::uintptr_t asset,const Matrix& matrix,unsigned group,unsigned part) {
    using CreateBus=std::uintptr_t(*)(std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uintptr_t,std::uint8_t,std::uintptr_t);
    const auto bus=reinterpret_cast<CreateBus>(base+b::create_bus)(ptr(ptr(parent,8),0x48),parent,0,0,0,0);
    require(bus!=0,"Deathrace entity bus unavailable.");
    reinterpret_cast<int(*)(std::uintptr_t)>(base+b::acquire_bus)(bus);
    struct BusScope {std::uintptr_t base,bus;~BusScope(){reinterpret_cast<void(*)(std::uintptr_t)>(base+e::release_reference)(bus);}} bus_scope{base,bus};
    alignas(16) std::array<unsigned char,0x190> descriptor{};
    reinterpret_cast<void*(*)(void*,std::uintptr_t,std::uintptr_t,const void*)>(base+e::descriptor_init)(descriptor.data(),0,bus,matrix.data());
    struct Scope {std::uintptr_t base;void* data;~Scope(){reinterpret_cast<void(*)(void*)>(base+e::descriptor_destroy)(static_cast<unsigned char*>(data)+0x10);}} scope{base,descriptor.data()};
    const std::uint32_t id=255;std::memcpy(descriptor.data()+0x38,&id,4);descriptor[0x151]=0;
    std::array<std::uintptr_t,3> result{};
    reinterpret_cast<void*(*)(void*,void*,std::uintptr_t,std::uintptr_t,std::uintptr_t)>(base+e::create_entity)(result.data(),descriptor.data(),asset,0,0);
    if(result[1])reinterpret_cast<void(*)(std::uintptr_t)>(base+e::release_reference)(result[1]);
    require(result[0]!=0,"Native checkpoint mesh did not create an entity.");
    const Entity entity{result[0],ptr(result[0],0x40),ptr(result[0],0x20),group,part};
    {auto& s=state();std::lock_guard lock(s.mutex);s.entities.push_back(entity);}
    reinterpret_cast<void(*)(std::uintptr_t,const void*,std::uintptr_t,std::uint8_t)>(base+e::initialize_placement)(entity.entity,matrix.data(),0,1);
    return entity;
}
Matrix mesh_pose(const Gate& gate,unsigned part) {
    if(gate.arrow)return race_arrow_layer_transform(gate.at,gate.yaw,gate.roll,gate.width,gate.pitch,part);
    // Exact child transforms from SPF_UI_Checkpoint_Base. One size control
    // scales the complete gate, including bolt placement, about ground level.
    constexpr std::array<float,4> sx{7.f,7.f,-2.12332f,2.13802f},sy{7.f,7.f,2.12332f,2.13802f};
    constexpr std::array<float,4> right_y{0.f,0.f,-.286054f,.0179417f},up_x{0.f,0.f,-.286054f,-.0179417f};
    constexpr std::array<float,4> x{0.f,0.f,-1.67701f,1.70283f},y{1.70747f,1.70747f,4.31834f,4.34544f};
    constexpr std::array<float,4> z{.00849477f,0.f,.1f,.1f};
    const float yaw=gate.yaw*3.14159265f/180.f,co=std::cos(yaw),si=std::sin(yaw),width=gate.width*2.f/7.f;
    const float scale=gate.width/6.f;
    const float a=sx[part]*width,bb=right_y[part]*scale,c=up_x[part]*width,d=sy[part]*scale,depth=(part<2?7.f:part==2?2.1425f:2.1381f)*scale;
    if(gate.pitch!=0.f) {
        const auto axes=race_gate_axes(gate.yaw,gate.pitch);Matrix result{};
        for(unsigned axis=0;axis<3;++axis) {
            result[axis]=axes.right[axis]*a+axes.up[axis]*bb;
            result[4+axis]=axes.right[axis]*c+axes.up[axis]*d;
            result[8+axis]=axes.forward[axis]*depth;
            result[12+axis]=gate.at[axis]+axes.right[axis]*x[part]*width+axes.up[axis]*y[part]*scale+axes.forward[axis]*z[part]*scale;
        }
        result[15]=1;return result;
    }
    return {co*a,bb,-si*a,0,co*c,d,-si*c,0,si*depth,0,co*depth,0,
        gate.at[0]+co*x[part]*width+si*z[part]*scale,gate.at[1]+y[part]*scale,gate.at[2]-si*x[part]*width+co*z[part]*scale,1};
}
void clear(std::uintptr_t base) {
    auto& s=state();std::vector<Entity> old;
    {std::lock_guard lock(s.mutex);old.swap(s.entities);s.gates.clear();s.dirty.clear();s.route_ready=false;}
    // No lock across native destruction: the shared hook calls us back.
    for(const auto& e0:old)if(ptr(e0.entity,0x20)==e0.context && ptr(e0.entity,0x40)==e0.owner)
        reinterpret_cast<void(*)(std::uintptr_t,std::uintptr_t)>(base+e::destroy_entity)(e0.entity,e0.owner);
}
void clear_group(std::uintptr_t base,unsigned group) {
    auto& s=state();std::vector<Entity> old;
    {std::lock_guard lock(s.mutex);for(const auto& entity:s.entities)if(entity.group==group)old.push_back(entity);
        std::erase_if(s.entities,[&](const Entity& entity){return entity.group==group;});}
    for(const auto& entity:old)if(ptr(entity.entity,0x20)==entity.context && ptr(entity.entity,0x40)==entity.owner)
        reinterpret_cast<void(*)(std::uintptr_t,std::uintptr_t)>(base+e::destroy_entity)(entity.entity,entity.owner);
}
// The zone's edge, every 4 m or so (8 to 48 cones), at the height of its centre: the placed area,
// or while one is being placed, the circle under the cursor (moving with it, as Deathrace's gate).
std::vector<Vec3> zone_ring(const NativeView& view) {
    std::vector<Vec3> points;
    // A custom zone: a cone at each corner placed, and along its edges every 4 m (closed once it has
    // three corners); while placing, one more under the cursor.
    if(view.settings.area_radius<=0.f && (view.zone_custom || view.placing_corners)) {
        // While placing: the placed cones' edges only (open), and one cone where the next would go.
        // Filling the edges to the aim point too trailed lines of cones after the camera.
        const auto& corners=view.settings.corners;
        if(view.placing_corners && view.cursor_valid)points.push_back(view.cursor);
        for(std::size_t i=0;i<corners.size() && points.size()<48;++i) {
            const auto& a=corners[i];points.push_back(a);
            const bool closing=i+1==corners.size();
            if(closing && (corners.size()<3 || view.placing_corners))break;
            const auto& b=corners[closing?0:i+1];
            const float dx=b[0]-a[0],dy=b[1]-a[1],dz=b[2]-a[2],length=std::sqrt(dx*dx+dz*dz);
            const auto between=static_cast<unsigned>(length/4.f);
            for(unsigned k=1;k<between && points.size()<48;++k) {
                const float f=static_cast<float>(k)/static_cast<float>(between);
                points.push_back({a[0]+dx*f,a[1]+dy*f,a[2]+dz*f});
            }
        }
        return points;
    }
    const bool preview=view.placing_circle && view.cursor_valid;
    if(!preview && (view.settings.area_radius<=0.f || view.settings.corners.empty()))return points;
    const auto centre=preview?view.cursor:view.settings.corners[0];
    const float radius=preview?view.draft_radius:view.settings.area_radius;
    const auto count=std::clamp(static_cast<unsigned>(2.f*3.14159265f*radius/4.f),8u,48u);
    for(unsigned i=0;i<count;++i) {
        const float angle=2.f*3.14159265f*static_cast<float>(i)/static_cast<float>(count);
        points.push_back({centre[0]+radius*std::cos(angle),centre[1],centre[2]+radius*std::sin(angle)});
    }
    return points;
}
constexpr unsigned beacon_group=900; // apart from the cones (0..47)
constexpr const char* bolt_name="World/ReSkate/Deathrace/SM_FX_GP_LightningBolt_A_Gate";
// The beacon's place: a finished zone's middle (none while its cones are being placed).
std::optional<Vec3> zone_beacon(const NativeView& view) {
    if(view.placing_corners || view.placing_circle)return {};
    if(view.settings.area_radius>0.f && !view.settings.corners.empty())return view.settings.corners[0];
    if(view.zone_custom && view.settings.corners.size()>=3)return area_centre(view.settings.corners);
    return {};
}
void tick_beacon(std::uintptr_t base,std::uintptr_t context,const NativeView& view,State& s,std::uint64_t now,
                 std::uintptr_t(*find)(std::uint16_t,const char*)) {
    if(s.bolt && !detail::checkpoint_resident(s.bolt,find(s.bolt_domain,bolt_name),base,[](auto at){return ptr(at);})) {
        clear_group(base,beacon_group);s.bolt=0;
    }
    const auto at=zone_beacon(view);
    std::optional<Entity> live;
    {std::lock_guard lock(s.mutex);for(const auto& entity:s.entities)if(entity.group==beacon_group)live=entity;}
    if(!at) {if(live)clear_group(base,beacon_group);return;}
    if(!s.bolt) {
        if(now<s.bolt_retry)return;s.bolt_retry=now+2000;
        for(std::uint16_t domain=0;domain<0xbbf && !s.bolt;++domain) {
            if(!ptr(base+addr::engine::domain_owners,std::uintptr_t(domain)*8))continue;
            const auto asset=find(domain,bolt_name);
            if(detail::checkpoint_blueprint(asset,base,[](auto at){return ptr(at);}))s.bolt=asset,s.bolt_domain=domain;
        }
        if(!s.bolt)return;
    }
    if(now<s.bolt_moved+16)return;
    s.bolt_moved=now;
    // Eight metres up, four times a gate bolt's size, a turn every six seconds.
    constexpr float size=4.f;
    const float yaw=static_cast<float>(now%6000)/6000.f*2.f*3.14159265f,co=std::cos(yaw),si=std::sin(yaw);
    const Matrix matrix{co*size,0,-si*size,0,0,size,0,0,si*size,0,co*size,0,(*at)[0],(*at)[1]+8.f,(*at)[2],1};
    if(live) {
        const auto collection=ptr(live->entity,0x70);
        if(ptr(live->entity,0x20)==live->context && ptr(live->entity,0x40)==live->owner && collection && ptr(collection)==live->entity) {
            reinterpret_cast<void(*)(std::uintptr_t,const void*,std::uint8_t,std::uint8_t)>(base+e::move_spatial_entity)(live->entity,matrix.data(),1,1);
            return;
        }
        clear_group(base,beacon_group);
    }
    const auto parent=parent_bus(base,context);if(!parent)return;
    (void)create(base,parent,s.bolt,matrix,beacon_group,0);
}
void tick_zone(std::uintptr_t base,std::uintptr_t context,const NativeView& view,State& s,std::uint64_t now) {
    const auto find=game::native_data().find_asset;if(!find)return;
    tick_beacon(base,context,view,s,now,find);
    if(s.cone && !detail::checkpoint_resident(s.cone,find(s.cone_domain,cone_names[s.cone_name]),base,[](auto at){return ptr(at);})) {
        clear(base);s.cone=0;s.cone_points.clear();
    }
    if(!s.cone) {
        if(now<s.retry)return;s.retry=now+2000;
        // In order of preference, each looked for in every loaded domain before the next.
        for(unsigned i=0;i<cone_names.size() && !s.cone;++i)
            for(std::uint16_t domain=0;domain<0xbbf && !s.cone;++domain) {
                if(!ptr(base+addr::engine::domain_owners,std::uintptr_t(domain)*8))continue;
                const auto asset=find(domain,cone_names[i]);
                if(detail::checkpoint_blueprint(asset,base,[](auto at){return ptr(at);}))s.cone=asset,s.cone_domain=domain,s.cone_name=i;
            }
        if(!s.cone) {waiting(s,"no traffic cone or guide bolt is loaded in this part of the world; the zone shows on the HUD only");return;}
        logging::log(logging::Level::info,logging::Channel::runtime,"R.E.S.K.A.T.E. zone: cones from {}.",cone_names[s.cone_name]);
    }
    const auto points=zone_ring(view);
    std::vector<Entity> live;
    {std::lock_guard lock(s.mutex);for(const auto& entity:s.entities)if(entity.group!=beacon_group)live.push_back(entity);}
    if(points==s.cone_points && live.size()==points.size())return;
    if(now<s.preview_at)return;
    s.preview_at=now+16;
    // The same number of cones: move them (recreating every frame made Deathrace's preview jump).
    if(live.size()==points.size() && s.cone_points.size()==points.size() && std::all_of(live.begin(),live.end(),[](const auto& entity) {
        const auto collection=ptr(entity.entity,0x70);
        return ptr(entity.entity,0x20)==entity.context && ptr(entity.entity,0x40)==entity.owner && collection && ptr(collection)==entity.entity;
    })) {
        for(const auto& entity:live) {
            if(entity.group>=points.size())continue;
            const auto& at=points[entity.group];
            const Matrix matrix{1,0,0,0,0,1,0,0,0,0,1,0,at[0],at[1],at[2],1};
            reinterpret_cast<void(*)(std::uintptr_t,const void*,std::uint8_t,std::uint8_t)>(base+e::move_spatial_entity)(entity.entity,matrix.data(),1,1);
        }
        s.cone_points=points;
        return;
    }
    const auto parent=parent_bus(base,context);if(!parent) {waiting(s,"waiting for the native local skater entity bus");return;}
    std::set<unsigned> groups;for(const auto& entity:live)groups.insert(entity.group);
    for(const auto group:groups)clear_group(base,group); // the cones only: the beacon stays
    for(unsigned i=0;i<points.size();++i) {
        const auto& at=points[i];
        (void)create(base,parent,s.cone,Matrix{1,0,0,0,0,1,0,0,0,0,1,0,at[0],at[1],at[2],1},i,0);
    }
    s.cone_points=points;
    if(!points.empty())waiting(s,std::format("{} zone cones placed",points.size()));
}
}
void entity_destroyed(std::uintptr_t entity) noexcept {
    auto& s=state();std::lock_guard lock(s.mutex);
    for(const auto& e0:s.entities)if(e0.entity==entity) {s.dirty.insert(e0.group);s.route_ready=false;}
    std::erase_if(s.entities,[&](const Entity& e0){return e0.entity==entity;});
}
std::string start_error(std::uint64_t world,std::uint32_t game) {
    auto& s=state();std::lock_guard lock(s.mutex);
    return s.route_ready && s.ready_world==world && s.ready_game==game?std::string{}:
        std::string("Native checkpoint objects are still loading. Wait a moment before starting; check the runtime log if they remain invisible.");
}
void tick(std::uintptr_t base,std::uintptr_t client,bool ready) noexcept {
    auto& s=state();
    try {
        const auto view=native_view();
        {std::lock_guard lock(s.mutex);
            if(s.lifetime.blocked() || (view.active && !s.lifetime.can_publish(view.game)))return;
        }
        const auto context=ptr(client,8);const auto now=GetTickCount64();
        const bool active=view.active && view.settings.mode==Mode::race && !(view.state && view.state->phase==Phase::results);
        // R.E.S.K.A.T.E.'s trick zone, when its leader placed one: cones on its edge.
        const bool zone=view.active && view.settings.mode==Mode::skate && !(view.state && view.state->phase==Phase::results) &&
            ((view.settings.area_radius>0.f && !view.settings.corners.empty()) || (view.placing_circle && view.cursor_valid) ||
             (view.zone_custom && (!view.settings.corners.empty() || (view.placing_corners && view.cursor_valid))));
        {std::lock_guard lock(s.mutex);s.route_ready=false;}
        if(s.context && s.context!=context) {std::lock_guard lock(s.mutex);s.entities.clear();s.gates.clear();s.dirty.clear();s.assets={};s.context=0;}
        if(!ready || !context) {if(active)waiting(s,"waiting for local client world readiness");return;}
        // Releasing a route is independent of the native creation gate. That
        // gate can be closed after Leave even while these entities still live.
        if(!active && !zone) {
            bool tracked{};{std::lock_guard lock(s.mutex);tracked=!s.entities.empty();}
            if(tracked || !s.gates.empty())clear(base);
            s.cone_points.clear();s.waiting.clear();return;
        }
        const auto tls_array=static_cast<std::uintptr_t>(__readgsqword(0x58));
        const auto index=get<std::uint32_t>(base+addr::engine::tls_index);if(index>4095)return;
        const auto tls=ptr(tls_array,std::uintptr_t(index)*8);
        if(!get<std::uint8_t>(base+addr::engine::entity_creation_ready) || !get<std::uint8_t>(tls+0xb19) || ptr(tls,0x550)!=context) {
            if(active)waiting(s,"waiting for verified native client creation scope");return;
        }
        if(s.world!=view.world || s.game!=view.game) {clear(base);s.assets={};s.world=view.world;s.game=view.game;s.error.clear();}
        s.context=context;
        if(!s.verified) {
            prefix(base,e::descriptor_init,e::descriptor_init_prefix);prefix(base,e::descriptor_destroy,e::descriptor_destroy_prefix);
            prefix(base,e::create_entity,e::create_entity_prefix);prefix(base,e::initialize_placement,e::initialize_placement_prefix);
            prefix(base,e::move_spatial_entity,e::move_spatial_entity_prefix);
            // ReSkate's shared entity lifetime hook already detours Destroy.
            // install_entity_hooks below validates or reuses that verified hook.
            prefix(base,b::create_bus,b::create_bus_prefix);prefix(base,b::acquire_bus,b::acquire_bus_prefix);
            std::string detail;require(multiplayer::install_entity_hooks(base,detail),"Native entity lifetime hooks unavailable.");s.verified=true;
        }
        if(zone) {tick_zone(base,context,view,s,now);return;}
        if(!assets_resident(base,s)) {
            // The native registry owns the asset lifetime, not the local client
            // context. Recreate only after all current roots resolve again.
            clear(base);s.assets={};s.asset_domains={};s.retry=0;
        }
        if(!std::all_of(s.assets.begin(),s.assets.end(),[](auto a){return a!=0;})) {
            if(now<s.retry)return;s.retry=now+1000;
            if(!load_assets(base,s)) {
                waiting(s,std::format("waiting for route blueprints ({}/{} loaded)",std::count_if(s.assets.begin(),s.assets.end(),[](auto a){return a!=0;}),s.assets.size()));
                return;
            }
        }
        std::vector<Gate> gates;
        // Start uses the real flag picker. World checkpoint meshes begin at
        // index 1, preserving the separately stored start position and yaw.
        for(unsigned i=1;i<view.settings.points.size();++i)gates.push_back({view.settings.points[i],
            i<view.settings.yaws.size()?view.settings.yaws[i]:0.f,gate_half_width(view.settings,i),
            i+1==view.settings.points.size() && view.route_finish,guiding_gate(view.settings,i),arrow_marker(view.settings,i),
            i<view.settings.arrow_rolls.size()?view.settings.arrow_rolls[i]:0.f,i<view.settings.gate_pitches.size()?view.settings.gate_pitches[i]:0.f});
        if(view.placing && view.cursor_valid && view.route_object!=0)gates.push_back({view.cursor,view.cursor_yaw,view.cursor_width,view.route_object==2,view.route_object==3,view.route_object==4,view.cursor_roll,view.cursor_pitch});
        if(gates.empty())waiting(s,view.placing?"waiting for a valid placement surface":"waiting for route gates");
        std::set<unsigned> dirty;
        std::vector<unsigned> live_parts(gates.size());
        std::vector<unsigned> expected_parts;unsigned expected_total{};
        for(const auto& gate:gates){const auto parts=gate.arrow?2u:4u;expected_parts.push_back(parts);expected_total+=parts;}
        {std::lock_guard lock(s.mutex);
            for(const auto& entity:s.entities)if(entity.group<live_parts.size())++live_parts[entity.group];
            dirty=detail::pending_checkpoint_groups(s.dirty,live_parts,expected_parts);}
        const auto publish_ready=[&] {
            std::lock_guard lock(s.mutex);s.ready_world=view.world;s.ready_game=view.game;
            s.route_ready=!view.placing && view.route_finish && gates.size()>=2 &&
                s.entities.size()==expected_total && s.dirty.empty() && live_parts==expected_parts;
        };
        if(gates==s.gates && dirty.empty()) {publish_ready();return;}
        if(now<s.preview_at)return;
        s.preview_at=now+16;
        const auto parent=parent_bus(base,context);if(!parent) {waiting(s,"waiting for the native local skater entity bus");return;}
        {std::lock_guard lock(s.mutex);
            std::fill(live_parts.begin(),live_parts.end(),0);
            for(const auto& entity:s.entities)if(entity.group<live_parts.size())++live_parts[entity.group];
            dirty=detail::take_checkpoint_groups(s.dirty,live_parts,expected_parts);}
        unsigned created_groups{};
        for(unsigned i=0;i<std::max(gates.size(),s.gates.size());++i) {
            if(i<gates.size() && i<s.gates.size() && gates[i]==s.gates[i] && !dirty.contains(i))continue;
            // Move a resident arch in place. Recreating all four meshes five
            // times a second caused the preview to jump and restart its FX.
            if(i<gates.size() && i<s.gates.size() && gates[i].finish==s.gates[i].finish && gates[i].guiding==s.gates[i].guiding && gates[i].arrow==s.gates[i].arrow && !dirty.contains(i)) {
                std::vector<Entity> group;
                {std::lock_guard lock(s.mutex);for(const auto& entity:s.entities)if(entity.group==i)group.push_back(entity);}
                if(group.size()==expected_parts[i] && std::all_of(group.begin(),group.end(),[](const auto& entity) {
                    const auto collection=ptr(entity.entity,0x70);
                    return ptr(entity.entity,0x20)==entity.context && ptr(entity.entity,0x40)==entity.owner && collection && ptr(collection)==entity.entity;
                })) {
                    for(const auto& entity:group) {
                        const auto matrix=mesh_pose(gates[i],entity.part);
                        reinterpret_cast<void(*)(std::uintptr_t,const void*,std::uint8_t,std::uint8_t)>(base+e::move_spatial_entity)(entity.entity,matrix.data(),1,1);
                    }
                    continue;
                }
            }
            if(i<gates.size() && created_groups>=16)continue; // Spread a long route across client frames.
            clear_group(base,i);
            if(i<gates.size())++created_groups;
            if(i<gates.size())for(unsigned part=0;part<expected_parts[i];++part) {
                const auto& gate=gates[i];const auto asset=gate.arrow?9+part:(gate.finish?3:gate.guiding?6:0)+(part<2?part:2);
                (void)create(base,parent,s.assets[asset],mesh_pose(gate,part),i,part);
            }
        }
        {std::lock_guard lock(s.mutex);
            std::fill(live_parts.begin(),live_parts.end(),0);
            for(const auto& entity:s.entities)if(entity.group<live_parts.size())++live_parts[entity.group];}
        s.gates=gates;publish_ready();
        if(!gates.empty())waiting(s,"native checkpoint entities created");
    } catch(const std::exception& e0) {
        if(s.error!=e0.what()) {s.error=e0.what();logging::log(logging::Level::error,logging::Channel::runtime,"Deathrace objects: {}",s.error);}
    } catch(...) {logging::write(logging::Level::error,logging::Channel::runtime,"Deathrace object creation failed.");}
}
void before_level_transition(std::uintptr_t base,unsigned next) noexcept {
    auto& s=state();bool release{};
    {std::lock_guard lock(s.mutex);release=s.lifetime.before_transition(next,s.game);}
    if(!release)return;
    // Retire in the live world before its component data disappears. Saved
    // courses live in game_modes and remain intact.
    try {clear(base);}
    catch(...) {logging::write(logging::Level::error,logging::Channel::runtime,"Deathrace pre-transition gate cleanup failed.");}
    {std::lock_guard lock(s.mutex);
        s.entities.clear();s.gates.clear();s.dirty.clear();s.assets={};s.asset_domains={};
        s.context=0;s.world=0;s.game=0;s.ready_world=0;s.ready_game=0;s.route_ready=false;s.retry=0;
    }
    logging::log(logging::Level::info,logging::Channel::runtime,"Deathrace gate ownership retired before native state {}.",next);
}
}
