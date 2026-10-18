#include "one_up_placement.h"
#include "one_up_runtime.h"
#include "Extension/Modes/game_modes.h"
#include "native_throwdowns.h"
#include "throwdown_lab.h"
#include "Extension/UI/NativeMenu/native_menu_data.h"
#include "Engine/Core/Log/logging.h"
#include <Windows.h>
#include <algorithm>
#include <cmath>
#include <mutex>
#include <optional>
#include <vector>

namespace dingosdk::multiplayer::one_up {
namespace {
enum class Step { idle, open, setup, flag, placing, closing, waiting, cleanup };
struct SetupRequest { unsigned seconds{}, players{}; std::uint64_t world{}, local{}; };
struct ModeSetupRequest { std::uint64_t world{}, local{}; std::uint32_t game{}; unsigned players{}; };
struct Placement {
    std::mutex mutex;
    Step step{};
    std::uint64_t world{}, local{}, until{}, changed{}, sent{}, reopen_until{}, reopen_at{};
    unsigned seconds{}, players{};
    std::uint32_t mmid{};
    std::vector<std::uint32_t> custom_mmids;
    std::uint32_t race_game{};
    bool created{}, setup_seen{}, flag_seen{}, exited{}, cancelling{}, destroy_queued{}, destroyed{};
    bool handed_off{}, match_seen{};
    bool show_setup{}, native_entry{}, generic_mode{};
    std::optional<std::array<float,3>> spawn;
    Facing facing=default_facing;
    std::optional<SetupRequest> pending_setup;
    std::optional<ModeSetupRequest> pending_race_flag;
    std::string status;
};
Placement& placement() { static Placement p; return p; }
void reset(Placement& p) {
    p.step=Step::idle; p.spawn.reset(); p.facing=default_facing;p.mmid=0; p.sent=0;
    p.created=p.setup_seen=p.flag_seen=p.exited=p.cancelling=p.destroy_queued=p.destroyed=false;
    p.handed_off=p.match_seen=false;
    p.show_setup=p.native_entry=p.generic_mode=false;
    p.race_game=0;
}
void open_native_setup(Placement& p,const SetupRequest& request,std::uint64_t now) {
    reset(p); p.pending_setup.reset();p.pending_race_flag.reset();
    p.step=Step::open; p.show_setup=p.native_entry=true;
    p.world=request.world; p.local=request.local;
    p.seconds=request.seconds; p.players=request.players;
    p.changed=now; p.until=now+10000;
    p.reopen_until=p.reopen_at=0; p.status.clear();
}
void open_race_flag(Placement& p,const ModeSetupRequest& request,std::uint64_t now) {
    reset(p);p.pending_setup.reset();p.pending_race_flag.reset();
    p.step=Step::open;p.world=request.world;p.local=request.local;p.race_game=request.game;
    p.seconds=30;p.players=request.players;p.changed=now;p.until=now+15000;
    p.reopen_until=p.reopen_at=0;p.status="Place the Deathrace starting flag.";
}
bool begin_mode_setup(std::uint64_t world,std::uint32_t game,unsigned players,bool show_native_setup,bool generic_mode) {
    const auto v=view();
    const auto mode=modes::native_view();
    if(!world || v.world!=world || !v.local || v.state.match || !game || players<1 || players>6)return false;
    auto& p=placement();std::lock_guard lock(p.mutex);
    // Replacing the start from the route editor needs a new native picker,
    // after the old registration is destroyed. Never reuse its MMID or drop
    // a native flag which is already starting a different match.
    if(!generic_mode && !show_native_setup && p.step==Step::waiting && !p.generic_mode &&
       p.race_game==game && p.world==world && p.handed_off && p.mmid && !p.destroy_queued &&
       mode.active && mode.leading && mode.world==world && mode.game==game && mode.placing &&
       mode.state && mode.state->phase==modes::Phase::setup) {
        p.pending_race_flag=ModeSetupRequest{world,v.local,game,players};
        p.step=Step::cleanup;p.cancelling=true;p.sent=0;p.until=GetTickCount64()+15000;
        return true;
    }
    // A fresh grid callback can replace an abandoned native setup even when
    // the state machine omitted OnEnd. Never replace an actual placed flag.
    if(p.step!=Step::idle && !(p.step==Step::setup && p.native_entry && (show_native_setup || !p.show_setup) && !p.flag_seen && !p.mmid))return false;
    if(p.created)release_one_up_native_placeholder();
    reset(p);p.pending_race_flag.reset();p.step=Step::open;p.world=world;p.local=v.local;p.race_game=game;
    p.show_setup=p.native_entry=show_native_setup;p.generic_mode=generic_mode;
    p.seconds=30;p.players=players;p.changed=GetTickCount64();p.until=p.changed+15000;
    p.status=generic_mode?"Opening Throwdown setup...":"Place the Deathrace starting flag.";
    prepare_throwdown_injection();return true;
}
// QueueStateNavigation's live graph publishes these two fields, with metadata
// [-1] for a command without arguments. Do not change the Back keyword stack.
bool navigate(const menu_data::Context& c, std::string_view name, std::uint32_t consumed=0) {
    using namespace menu_data;
    const auto roots=c.roots({0xcc4776b5});
    require(roots.size()==1,"Native navigation model is unavailable.");
    const auto current=c.field(roots.front().model,0xf3ba9cf8);
    const auto metadata=c.field(roots.front().model,0xf437b255);
    const auto pending=read<std::uint32_t>(c.address(current));
    if(pending && pending!=consumed) return false;
    c.set(current,game::native_name_hash(name));
    c.array(metadata,std::vector<std::int32_t>{-1});
    logging::log(logging::Level::info,logging::Channel::ui,"1-Up flag: navigation {}.",name);
    return true;
}
}
bool begin_flag_placement(unsigned seconds,unsigned players,bool show_native_setup) {
    const auto v=view();
    if(!v.can_create || v.state.match || seconds<10 || seconds>120 || players<1 || players>6) return false;
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step!=Step::idle) return false;
    reset(p); p.step=Step::open; p.world=v.world; p.local=v.local;
    p.show_setup=p.native_entry=show_native_setup;
    p.reopen_until=p.reopen_at=0;
    p.seconds=seconds; p.players=players; p.changed=GetTickCount64(); p.until=p.changed+15000;
    p.status="Opening flag placement...";
    prepare_throwdown_injection(); // Prewarm cleanup before the player places the flag.
    return true;
}
bool arm_native_setup(unsigned seconds,unsigned players) {
    const auto v=view();
    const bool ended=v.state.phase==Phase::finished || v.state.phase==Phase::cancelled;
    if(modes::active() || seconds<10 || seconds>120 || players<1 || players>6 || (v.state.match && !ended))return false;
    auto& p=placement();
    {
        std::lock_guard lock(p.mutex);
        const SetupRequest request{seconds,players,v.world,v.local};
        if(p.step==Step::cleanup || p.step==Step::waiting) {
            // The old native queue still owns a flag. Finish its teardown
            // before opening the next setup, without losing this card click.
            if(p.step==Step::waiting && p.handed_off && !p.match_seen && !ended)return false;
            p.pending_setup=request;
            if(p.step!=Step::cleanup) {
                p.step=Step::cleanup;p.cancelling=true;p.sent=0;p.until=GetTickCount64()+15000;
            }
        } else {
            if(p.step!=Step::idle && p.step!=Step::open &&
               !(p.step==Step::setup && !p.flag_seen && !p.mmid))return false;
            if(p.created)release_one_up_native_placeholder();
            open_native_setup(p,request,GetTickCount64());
        }
    }
    if(v.state.match)queue("leave");
    prepare_throwdown_injection();
    return true;
}
bool begin_race_flag_placement(std::uint64_t world,std::uint32_t game,unsigned players,bool show_native_setup) {
    return begin_mode_setup(world,game,players,show_native_setup,false);
}
bool begin_native_mode_setup(std::uint64_t world,std::uint32_t game,unsigned players) {
    return begin_mode_setup(world,game,players,true,true);
}
bool race_flag_registration_owned() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.race_game && !p.generic_mode && p.step!=Step::idle && (p.step!=Step::waiting || p.mmid!=0);
}
bool mode_flag_registration_owned(std::uint32_t game) noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.generic_mode && (!game || p.race_game==game) && p.step!=Step::idle &&
        (p.step!=Step::waiting || p.mmid!=0);
}
std::uint32_t native_flag_mode_game() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.step!=Step::idle?p.race_game:0;
}
bool native_flag_mode_waiting() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.race_game && p.step==Step::waiting && p.handed_off && p.spawn && p.mmid;
}
std::uint32_t native_setup_race_game() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return !p.generic_mode && p.show_setup && (p.step==Step::open || p.step==Step::setup)?p.race_game:0;
}
std::uint32_t native_setup_mode_game() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.show_setup && (p.step==Step::open || p.step==Step::setup)?p.race_game:0;
}
bool close_race_setup_for_start(std::uint32_t game) noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    if(!game || p.race_game!=game || !p.show_setup || p.flag_seen || p.mmid ||
       (p.step!=Step::open && p.step!=Step::setup))return false;
    p.show_setup=false;p.cancelling=true;p.step=Step::cleanup;p.sent=0;
    p.until=GetTickCount64()+15000;return true;
}
bool native_setup_active() noexcept { auto& p=placement(); std::lock_guard lock(p.mutex); return p.show_setup && (p.step==Step::open || p.step==Step::setup); }
void retire_unconfirmed_native_setup() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    // This is called by a verified fresh picker callback, after returning to
    // Throwdowns. A cancelled setup with no registered flag cannot keep an
    // unacknowledged Exit alive and close or block the next page.
    if((p.step!=Step::open && p.step!=Step::setup && p.step!=Step::cleanup) ||
       !p.native_entry || p.flag_seen || p.mmid || p.handed_off)return;
    if(p.created)release_one_up_native_placeholder();
    reset(p);p.pending_setup.reset();p.pending_race_flag.reset();p.status.clear();p.reopen_until=p.reopen_at=0;
}
void set_native_setup_options(unsigned seconds,unsigned players) noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step==Step::setup && seconds>=10 && seconds<=120 && players>=1 && players<=6) { p.seconds=seconds; p.players=players; }
}
bool flag_placement_active() noexcept { auto& p=placement(); std::lock_guard lock(p.mutex); return p.step!=Step::idle && p.step!=Step::waiting; }
bool flag_registration_owned() noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    // The running match outlives its temporary native flag registration.
    // Once that registration is destroyed, stock challenges own their events.
    return p.step!=Step::idle && (p.step!=Step::waiting || p.mmid!=0);
}
bool owns_flag_registration(std::uint32_t mmid) noexcept { auto& p=placement(); std::lock_guard lock(p.mutex); return mmid && p.step!=Step::idle && p.mmid==mmid; }
static bool adopt_native_flag(std::uint64_t world,std::uint32_t game,std::uint32_t mmid,
    const std::array<float,3>& at,float yaw,unsigned seconds,unsigned players,bool race,bool oneup) noexcept {
    try {
        const auto local=view();
        if(!world || world!=local.world || !local.local || local.state.match || local_throwdown_event_active() || (!game && !oneup) || !mmid ||
           !std::isfinite(yaw) || !std::all_of(at.begin(),at.end(),[](float n){return std::isfinite(n) && std::abs(n)<100000.f;}))return false;
        auto& p=placement();std::lock_guard lock(p.mutex);
        if(p.step!=Step::idle)return p.step==Step::waiting && p.world==world && p.race_game==game && p.mmid==mmid;
        reset(p);p.step=Step::waiting;p.world=world;p.local=local.local;p.race_game=game;p.mmid=mmid;
        p.generic_mode=!race && !oneup;p.spawn=at;p.seconds=seconds;p.players=players;
        const float radians=yaw*3.14159265f/180.f,co=std::cos(radians),si=std::sin(radians);
        p.facing={co,0,-si,0,1,0,si,0,co};
        p.handed_off=p.exited=true;p.match_seen=!oneup;
        p.changed=GetTickCount64();p.until=p.changed+15000;p.status.clear();
        if(std::find(p.custom_mmids.begin(),p.custom_mmids.end(),mmid)==p.custom_mmids.end())p.custom_mmids.push_back(mmid);
        release_one_up_native_placeholder(); // Only this donor queue's activity-presence lease; no actual event exists.
        return true;
    } catch(...) {return false;}
}
bool adopt_native_mode_flag(std::uint64_t world,std::uint32_t game,std::uint32_t mmid,
    const std::array<float,3>& at,float yaw,unsigned seconds,unsigned players,bool race) noexcept {
    return adopt_native_flag(world,game,mmid,at,yaw,seconds,players,race,false);
}
bool adopt_native_one_up_flag(std::uint64_t world,std::uint32_t mmid,
    const std::array<float,3>& at,float yaw,unsigned seconds,unsigned players) noexcept {
    return adopt_native_flag(world,0,mmid,at,yaw,seconds,players,false,true);
}
bool flag_registration_waiting() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.step==Step::waiting && p.handed_off && p.spawn && p.mmid;
}
bool flag_registration_retiring() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.step==Step::cleanup;
}
FlagSetupOptions native_flag_options() noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return p.step!=Step::idle?FlagSetupOptions{p.seconds,p.players}:FlagSetupOptions{};
}
void flag_registration_left() noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step==Step::idle)return;
    p.pending_race_flag.reset(); // Quit/Cancel must not reopen a queued replacement picker.
    // sent belongs to the previous navigation step. A newly cancelled flag
    // still needs its Exit, but repeated leave notifications must not queue
    // that same Exit again.
    if(p.step!=Step::cleanup)p.sent=0;
    p.cancelling=true; p.step=Step::cleanup; p.until=GetTickCount64()+15000;
}
std::string flag_placement_status() { auto& p=placement(); std::lock_guard lock(p.mutex); return p.status; }
bool flag_lobby_requested() noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    const auto now=GetTickCount64(); return p.reopen_at && now>=p.reopen_at && now<p.reopen_until;
}
void flag_lobby_shown() noexcept { auto& p=placement(); std::lock_guard lock(p.mutex); p.reopen_until=p.reopen_at=0; }
void observe_flag_graph(std::uint32_t graph) noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step==Step::idle) return;
    if(graph==0x0f131312 && p.step==Step::setup) { // Spot Battle setup OnBegin
        p.setup_seen=true; p.until=GetTickCount64()+300000;
    }
    if(graph==1867583346U && p.step==Step::setup && p.setup_seen) { // Same setup OnEnd
        // Back and Confirm both leave this native state. Stop adapting its UI
        // immediately; a successful Confirm enters the flag update below.
        // Ignore a previous entry's delayed OnEnd before this one's OnBegin.
        p.show_setup=false; p.changed=GetTickCount64(); p.until=p.changed+1500;
    }
    if(graph==1867583346U && p.step==Step::cleanup && !p.flag_seen && !p.mmid)
        p.exited=true; // Saved-route Start closed the native setup, without creating a flag.
    // Spot Battle's own placement update runs after its validator/spawn
    // initialization. Its Confirm goes directly here; no S.K.A.T.E. area
    // state, skip predicate, or synthetic navigation is involved.
    if(graph==0x9700be92 && p.created && p.setup_seen &&
       (p.step==Step::setup || (p.step==Step::flag && !p.flag_seen))) {
        p.show_setup=false;
        p.step=Step::flag; p.flag_seen=true; p.until=GetTickCount64()+300000;
        logging::write(logging::Level::info,logging::Channel::ui,"1-Up flag: entered native Spot Battle flag placement.");
    }
}
void observe_flag_queue(std::uint32_t mmid) noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    if(!mmid || p.step==Step::idle)return;
    if(std::find(p.custom_mmids.begin(),p.custom_mmids.end(),mmid)==p.custom_mmids.end()) {
        if(p.custom_mmids.size()==64)p.custom_mmids.erase(p.custom_mmids.begin());
        p.custom_mmids.push_back(mmid);
    }
}
bool known_custom_flag_queue(std::uint32_t mmid) noexcept {
    auto& p=placement();std::lock_guard lock(p.mutex);
    return mmid && std::find(p.custom_mmids.begin(),p.custom_mmids.end(),mmid)!=p.custom_mmids.end();
}
void observe_flag_created() noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step==Step::setup) p.created=true;
}
void observe_flag_position(std::uint32_t mmid,const std::array<float,3>& spawn,const std::array<float,16>* transform) noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if((p.step!=Step::placing && p.step!=Step::flag) || !p.created || !p.flag_seen || !mmid ||
       !std::all_of(spawn.begin(),spawn.end(),[](float n){return std::isfinite(n) && std::abs(n)<100000;})) return;
    if(transform) {
        const auto facing=transform_facing(*transform);
        if(!valid_facing(facing))return;
        p.facing=facing;
    }
    p.spawn=spawn; p.mmid=mmid; p.step=Step::closing; p.until=GetTickCount64()+15000;
    p.status="Saving the starting flag...";
    logging::log(logging::Level::info,logging::Channel::ui,"1-Up flag: accepted spawn ({:.2f}, {:.2f}, {:.2f}), facing {:.2f} degrees, native queue {:#x}.",spawn[0],spawn[1],spawn[2],
        std::atan2(p.facing[6],p.facing[8])*180.f/3.14159265f,mmid);
}
void observe_flag_exit(bool cancelling) noexcept {
    std::uint64_t world{};std::uint32_t game{};
    auto& p=placement();
    {
        std::lock_guard lock(p.mutex);
        if(p.step==Step::idle) return;
        p.exited=true; p.cancelling=cancelling || !p.spawn;
        p.step=p.cancelling?Step::cleanup:Step::waiting; p.changed=GetTickCount64(); p.until=p.changed+15000;
        if(p.cancelling) {
            p.status="Flag placement cancelled.";
            world=p.world;game=p.race_game;
        }
    }
    // Native Back bypasses the course editor's cancel command. Queue the
    // exact match Quit outside the placement lock, so the mode thread can
    // retire gates without reversing its normal registration lock order.
    if(game)modes::request_native_quit(world,game);
}
void observe_flag_destroyed(std::uint32_t mmid) noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    if(p.step==Step::waiting && p.handed_off && p.destroy_queued && p.mmid==mmid) {
        // The registration is a lobby flag, not the running 1-Up match. Once
        // its transform is saved, use the native teardown to remove the pole,
        // ground decal and waiting notification without ending 1-Up.
        p.mmid=0;p.destroyed=true;p.changed=GetTickCount64();return;
    }
    if(p.step==Step::cleanup && p.mmid==mmid) { p.destroyed=true; p.changed=GetTickCount64(); }
}
void abandon_flag_placement() noexcept {
    auto& p=placement(); std::lock_guard lock(p.mutex);
    // The level's own teardown removes its queues. Never destroy an old MMID
    // in a new world, where the native coordinator can reuse it.
    if(p.step!=Step::idle) {
        if(p.created) release_one_up_native_placeholder();
        reset(p); p.status="Flag placement ended because the map changed.";
    }
    p.pending_setup.reset();p.pending_race_flag.reset();p.reopen_until=p.reopen_at=0;p.custom_mmids.clear();
}
void tick_flag_placement(const menu_data::Context& c) {
    const auto v=view(); const auto now=GetTickCount64();
    const auto mode=modes::native_view();
    auto& p=placement();
    bool reopen{};
    {
        std::lock_guard lock(p.mutex);
        if(p.step==Step::idle) {
            reopen=p.reopen_until>now && !p.reopen_at && v.state.match && v.world==p.world && v.local==p.local;
            if(!reopen) return;
        }
    }
    if(reopen) {
        // Toolbox_RadialListItem_Throwdowns uses this same navigation id.
        if(navigate(c,"Throwdowner")) { std::lock_guard lock(p.mutex); p.reopen_at=now+500; }
        return;
    }
    std::string command; Step next{}; std::uint32_t destroy{};
    std::optional<SpawnRequest> handoff;
    std::uint32_t race_game{};
    bool generic_mode{};
    bool release{};
    {
        std::lock_guard lock(p.mutex);
        if(p.step==Step::idle) return;
        // The same native setup survives connecting to a session. Offline
        // player/level IDs can change to transport IDs before the flag is
        // placed; keep ownership and bind the eventual spawn to that session.
        // Real level teardown calls abandon_flag_placement before unloading.
        if(p.native_entry && !p.handed_off && !p.spawn && p.step!=Step::cleanup) {
            if(v.world && v.local) { p.world=v.world; p.local=v.local; }
            else if(now<p.until && p.step!=Step::open)return; // Setup can open before capture initializes.
        } else if(v.world!=p.world || v.local!=p.local || !v.world) {
            if(p.created) release_one_up_native_placeholder();
            logging::log(logging::Level::warning,logging::Channel::ui,
                "1-Up flag: session changed during step {}, world {} -> {}, player {} -> {}.",
                static_cast<unsigned>(p.step),p.world,v.world,p.local,v.local);
            reset(p);p.pending_setup.reset();p.pending_race_flag.reset(); p.reopen_until=p.reopen_at=0; p.status="Flag placement ended because the session changed."; return;
        }
        if(p.step==Step::flag && p.flag_seen) { p.step=Step::placing; p.until=now+300000; p.status="Place your starting flag. Score anywhere."; }
        if(p.step==Step::waiting) {
            // Keep the native registration alive: it owns the world flag,
            // waiting notification, and Start/Leave input prompts.
            if(!p.handed_off && p.spawn) {
                handoff=SpawnRequest{*p.spawn,p.world,p.local,p.seconds,p.players,p.facing};
                race_game=p.race_game;
                generic_mode=p.generic_mode;
                p.handed_off=true; p.status.clear(); release=true;
            }
            const bool mode_matches=mode.active && mode.game==p.race_game && mode.world==p.world;
            if(p.race_game ? mode_matches : v.state.match!=0)p.match_seen=true;
            const bool started=p.race_game ? mode_matches && mode.state &&
                (mode.state->phase==modes::Phase::countdown || mode.state->phase==modes::Phase::playing) :
                v.state.phase==Phase::countdown || v.state.phase==Phase::playing;
            if(p.handed_off && p.match_seen && p.mmid && !p.destroy_queued && started)destroy=p.mmid;
            const bool ended=p.race_game ? !mode_matches || (mode.state && mode.state->phase==modes::Phase::results) :
                !v.state.match || v.state.phase==Phase::cancelled;
            if((p.match_seen && ended) || (p.handed_off && !p.match_seen && now>=p.until)) {
                p.step=Step::cleanup; p.cancelling=true; p.sent=0; p.until=now+15000;
            }
        }
        if(p.step==Step::setup && p.setup_seen && !p.show_setup && p.native_entry && now>=p.until) {
            release=p.created; reset(p); p.status.clear();
            // Back already performed the native navigation. Do not send an
            // exit that could close the next stock challenge's setup.
            if(release)release_one_up_native_placeholder();
            return;
        }
        if((p.step==Step::open || p.step==Step::setup) && !p.created && !p.setup_seen && now>=p.until) {
            reset(p); p.status="The setup did not open. Select 1-Up to try again.";
            return; // No native queue was created; do not close another menu.
        }
        if(now>=p.until && p.step!=Step::cleanup && p.step!=Step::waiting) {
            p.cancelling=true; p.spawn.reset(); p.status="Flag placement timed out. Open 1-Up to try again.";
            p.step=Step::cleanup; p.until=now+15000; p.sent=0;
        }
        if(p.step==Step::open) { command="SpotBattle"; next=Step::setup; }
        // Native Confirm owns this transition for menu entries. OnEnd also
        // fires on Back, so it must never become an automatic placement.
        else if(p.step==Step::setup && p.created && p.setup_seen && !p.show_setup && !p.native_entry) { command="ThrowdownPlacement"; next=Step::flag; }
        else if(p.step==Step::cleanup) {
            if(!p.exited && !p.sent) { command="ThrowdownerExit"; next=Step::cleanup; }
            if(p.mmid && !p.destroy_queued) destroy=p.mmid;
            if((p.exited && !p.mmid) || (p.destroyed && now>=p.changed+500)) {
                release=p.created;
                const auto requested=p.pending_setup;
                const auto race_requested=p.pending_race_flag;
                reset(p);p.pending_setup.reset();p.pending_race_flag.reset();p.status.clear();
                if(requested && requested->world==v.world && requested->local==v.local)
                    open_native_setup(p,*requested,now);
                else if(race_requested && race_requested->world==v.world && race_requested->local==v.local &&
                        mode.active && mode.leading && mode.world==v.world && mode.game==race_requested->game &&
                        mode.state && mode.state->phase==modes::Phase::setup)
                    open_race_flag(p,*race_requested,now);
            } else if(now>=p.until) {
                // Do not create 1-Up if the native queue could not be removed.
                p.status="Could not close flag placement. Back out of the native Throwdown before retrying.";
            }
        }
    }
    if(release) release_one_up_native_placeholder();
    if(handoff) {
        if(generic_mode)modes::accept_native_start_flag(handoff->world,race_game,handoff->position,
            std::atan2(handoff->facing[6],handoff->facing[8])*180.f/3.14159265f);
        else if(race_game)modes::accept_race_start(handoff->world,race_game,handoff->position,
            std::atan2(handoff->facing[6],handoff->facing[8])*180.f/3.14159265f);
        else create_at_spawn(*handoff);
    }
    if(destroy && prepare_throwdown_injection() && queue_throwdown_destroy(destroy)) {
        std::lock_guard lock(p.mutex); if(p.mmid==destroy) p.destroy_queued=true;
    }
    if(!command.empty()) {
        // Publication can run expression callbacks. Arm the expected step
        // before publishing, without holding this mutex across native code.
        Step previous;
        { std::lock_guard lock(p.mutex); previous=p.step; p.step=next; }
        bool sent{};
        try { sent=navigate(c,command); }
        catch(...) { std::lock_guard lock(p.mutex); p.step=previous; throw; }
        std::lock_guard lock(p.mutex);
        if(!sent) p.step=previous;
        else { p.sent=now; if(next!=Step::cleanup) p.until=now+15000; }
    }
}
}
