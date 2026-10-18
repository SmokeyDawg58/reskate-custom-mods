#include <Windows.h>
static ULONGLONG test_now=1000;
static ULONGLONG placement_test_clock() { return test_now; }
#define GetTickCount64 placement_test_clock
#include "Extension/Throwdowns/one_up_placement.cpp"
#undef GetTickCount64
static bool test_mode_active{};
static unsigned race_handoffs{};
static std::uint64_t race_world_seen{};
static std::uint32_t race_game_seen{};
static dingosdk::modes::Vec3 race_pose_seen{};
static float race_yaw_seen{};
static unsigned mode_handoffs{};
static std::uint64_t mode_world_seen{};
static std::uint32_t mode_game_seen{};
static dingosdk::modes::Vec3 mode_pose_seen{};
static float mode_yaw_seen{};
static dingosdk::modes::NativeView current_mode_view;
static std::vector<std::pair<std::uint64_t,std::uint32_t>> cancelled_modes;
static bool quit_outside_placement_lock=true;
namespace dingosdk::modes {
bool active() noexcept { return test_mode_active; }
NativeView native_view() { return current_mode_view; }
bool request_native_quit(std::uint64_t world,std::uint32_t game) noexcept {
    auto& mutex=multiplayer::one_up::placement().mutex;
    if(mutex.try_lock())mutex.unlock();else quit_outside_placement_lock=false;
    if(!current_mode_view.active || current_mode_view.world!=world || current_mode_view.game!=game)return false;
    cancelled_modes.emplace_back(world,game);return true;
}
void accept_race_start(std::uint64_t world,std::uint32_t game,const Vec3& at,float yaw) {
    ++race_handoffs;race_world_seen=world;race_game_seen=game;race_pose_seen=at;race_yaw_seen=yaw;
}
void accept_native_start_flag(std::uint64_t world,std::uint32_t game,const Vec3& at,float yaw) {
    ++mode_handoffs;mode_world_seen=world;mode_game_seen=game;mode_pose_seen=at;mode_yaw_seen=yaw;
}
}
#include <iostream>
#include <limits>

namespace {
using namespace dingosdk::multiplayer;
one_up::View current_view;
std::vector<one_up::SpawnRequest> handed_off;
std::vector<std::uint32_t> destroyed, navigations;
std::vector<std::string> commands;
unsigned released{};
std::uint32_t nav{};
bool available=true;
}
namespace dingosdk::multiplayer {
bool local_throwdown_event_active() noexcept {return false;}
void release_one_up_native_placeholder() noexcept { ++released; }
bool prepare_throwdown_injection() noexcept { return true; }
bool queue_throwdown_destroy(std::uint32_t id) noexcept { destroyed.push_back(id); return true; }
namespace one_up {
View view() { return current_view; }
void create_at_spawn(const SpawnRequest& request) { handed_off.push_back(request); }
void queue(std::string_view command) {
    commands.emplace_back(command);
    if(command=="leave")current_view.state={};
}
}
namespace menu_data {
unsigned size(Address) { return 4; }
void require(bool yes,const char* message) { if(!yes) throw std::runtime_error(message); }
std::vector<Root> Context::roots(std::initializer_list<std::uint32_t>) const { return available?std::vector<Root>{{{1,1},0}}:std::vector<Root>{}; }
Value Context::field(Value,std::uint32_t hash) const { return {hash,1}; }
Address Context::address(Value) const { return reinterpret_cast<Address>(&nav); }
void Context::publish(Value,const void* bytes) const { std::memcpy(&nav,bytes,4); navigations.push_back(nav); }
void Context::array(Value,std::span<const std::byte> bytes,unsigned count) const {
    std::int32_t metadata{}; std::memcpy(&metadata,bytes.data(),4);
    require(count==1 && metadata==-1,"Navigation metadata differs from native command.");
}
}
}
int main() {
    using namespace dingosdk::multiplayer::one_up;
    using dingosdk::game::native_name_hash;
    const menu_data::Context context(0,0);
    unsigned count{};
    const auto check=[&](bool value) { ++count; if(!value) throw std::runtime_error("Flag bridge check "+std::to_string(count)); };
    const auto tick=[&] { ++test_now; tick_flag_placement(context); };
    try {
        // Captured native SpawnPoint: XYZ + 2.7408998 radians. The old
        // TeleportLocation-only reader lost this yaw and spawned at zero.
        const auto native_pose=native_spawn_transform({643.75787f,198.9449f,1082.6879f,2.7408998f});
        const auto native_facing=transform_facing(native_pose);
        check(valid_facing(native_facing));
        check(std::abs(std::atan2(native_facing[6],native_facing[8])-2.7408998f)<.00001f);
        check(native_pose[12]==643.75787f && native_pose[13]==198.9449f && native_pose[14]==1082.6879f);
        const auto quarter=transform_facing(native_spawn_transform({7,8,9,1.5707963268f}));
        check(std::abs(quarter[6]-1.f)<.00001f && std::abs(quarter[8])<.00001f);
        check(!valid_facing(transform_facing(native_spawn_transform({7,8,9,std::numeric_limits<float>::quiet_NaN()}))));
        check(!begin_flag_placement(30,4));
        current_view.can_create=true; current_view.world=11; current_view.local=42;
        check(!begin_flag_placement(30,7));
        check(!begin_flag_placement(30,0));
        check(begin_flag_placement(30,1));abandon_flag_placement();released=0;
        check(begin_flag_placement(40,6) && !begin_flag_placement(30,4));
        available=false;
        try { tick(); check(false); } catch(const std::runtime_error&) {}
        check(flag_placement_active() && navigations.empty());
        available=true; tick(); check(nav==native_name_hash("SpotBattle"));
        observe_flag_created(); observe_flag_graph(0x0f131312);
        tick(); check(navigations.size()==1); // Never overwrite an unconsumed navigation.
        nav=0; tick(); check(nav==native_name_hash("ThrowdownPlacement"));
        observe_flag_position(100,{7,8,9}); check(!placement().spawn); // Not at the flag yet.
        observe_flag_graph(0x67122a51); observe_flag_graph(0x08a3b8c5);
        check(!placement().flag_seen); // S.K.A.T.E. graphs cannot advance Spot Battle ownership.
        observe_flag_graph(0x9700be92);
        check(nav==native_name_hash("ThrowdownPlacement") && placement().flag_seen);
        nav=0; tick();
        observe_flag_position(100,{std::numeric_limits<float>::infinity(),8,9}); check(!placement().spawn);
        const Facing facing{0,0,-1,0,1,0,1,0,0};
        auto flag_transform=spawn_transform({7,8,9},facing);
        auto invalid_transform=flag_transform;invalid_transform[0]=3;
        observe_flag_position(100,{7,8,9},&invalid_transform);check(!placement().spawn);
        observe_flag_position(100,{7,8,9},&flag_transform); observe_flag_exit(false); tick();
        check(destroyed.empty() && handed_off.size()==1);
        check(!flag_placement_active() && flag_registration_owned() && owns_flag_registration(100) && !owns_flag_registration(999) && released==1);
        check(handed_off[0].position==std::array<float,3>{7,8,9} && handed_off[0].seconds==40 && handed_off[0].players==6 && handed_off[0].world==11);
        check(handed_off[0].facing==facing);
        tick(); check(handed_off.size()==1);
        current_view.state.match=77; nav=0; tick(); check(!nav && !flag_lobby_requested()); // Stay skating with native waiting HUD.
        test_now+=300000; tick(); check(flag_registration_owned() && destroyed.empty()); // Waiting has no auto-start deadline.
        current_view.state.match=0;
        tick(); check(destroyed==std::vector<std::uint32_t>{100});
        observe_flag_destroyed(999); test_now+=600; tick(); check(flag_registration_owned());
        observe_flag_destroyed(100); test_now+=501; tick(); check(!flag_registration_owned() && released==2);
        check(begin_flag_placement(30,4)); nav=0; tick(); observe_flag_created(); observe_flag_exit(true); tick();
        check(!flag_placement_active() && released==3 && handed_off.size()==1);
        observe_flag_position(200,{7,8,9}); observe_flag_exit(false); tick(); check(handed_off.size()==1); // Unowned native match.
        check(begin_flag_placement(30,4)); current_view.world=12; tick(); check(!flag_placement_active() && destroyed.size()==1);
        check(begin_flag_placement(30,4)); abandon_flag_placement(); check(!flag_placement_active());
        check(begin_flag_placement(30,4)); nav=0; tick(); observe_flag_created(); observe_flag_graph(0x0f131312);
        nav=0; tick(); check(nav==native_name_hash("ThrowdownPlacement"));
        nav=native_name_hash("Back"); observe_flag_graph(0x9700be92);
        check(nav==native_name_hash("Back")); // Never overwrite another queued user action.
        nav=0; tick(); check(nav==0 && placement().step==Step::placing);
        abandon_flag_placement(); check(!flag_placement_active());
        check(begin_flag_placement(30,4,true));nav=0;tick();
        observe_flag_created();observe_flag_graph(0x0f131312);nav=0;
        test_now+=20000;tick();
        check(native_setup_active() && nav==0); // Original setup remains open until Confirm.
        set_native_setup_options(50,6);set_native_setup_options(30,7);
        check(placement().seconds==50 && placement().players==6);
        nav=native_name_hash("ThrowdownPlacement");observe_flag_graph(0x9700be92);
        check(!native_setup_active() && nav==native_name_hash("ThrowdownPlacement"));
        abandon_flag_placement();
        nav=0;check(begin_flag_placement(20,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
        tick();check(nav==0 && placement().step==Step::setup);
        abandon_flag_placement();
        // Card navigation belongs to the bridge. Ownership is armed before
        // the native setup opens, including when no local capture exists yet.
        current_view.world=0;current_view.local=0;current_view.can_create=false;
        nav=0;const auto before_native=navigations.size();
        check(!arm_native_setup(30,7));
        check(arm_native_setup(30,6));tick();
        check(native_setup_active() && navigations.size()==before_native+1 && nav==native_name_hash("SpotBattle"));
        nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        current_view.world=22;current_view.local=42;tick();
        test_now+=20000;tick();
        check(native_setup_active() && placement().world==22 && placement().players==6 && nav==0);
        nav=native_name_hash("ThrowdownPlacement");observe_flag_graph(0x9700be92);
        check(nav==native_name_hash("ThrowdownPlacement"));
        abandon_flag_placement();nav=0;
        check(arm_native_setup(30,4));
        test_now+=10001;tick();
        check(!flag_registration_owned() && nav==0); // Failed open cannot close another menu.
        check(arm_native_setup(40,5));
        check(arm_native_setup(50,6)); // Reopen after Back is not ignored.
        tick();check(nav==native_name_hash("SpotBattle"));nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        // Native Spot Battle Confirm initializes flag placement directly.
        nav=native_name_hash("ThrowdownPlacement");
        observe_flag_graph(0x9700be92);
        check(!native_setup_active() && nav==native_name_hash("ThrowdownPlacement"));
        const auto direct_count=navigations.size();nav=0;
        tick();
        check(placement().step==Step::placing && navigations.size()==direct_count);
        observe_flag_position(321,{1,2,3});observe_flag_exit(false);tick();
        check(handed_off.back().position==std::array<float,3>{1,2,3});
        current_view.state.match=88;current_view.state.phase=Phase::lobby;tick();
        retire_unconfirmed_native_setup();check(flag_registration_owned() && placement().mmid==321);
        const auto before_rejected=navigations.size();
        check(!arm_native_setup(30,1) && navigations.size()==before_rejected && nav==0);
        // A finished game may still own the native queue. A fresh click must
        // close it first, retain the request, then open exactly one 1-Up setup.
        current_view.state.phase=Phase::cancelled;
        check(arm_native_setup(20,1) && commands.back()=="leave");
        tick();check(destroyed.back()==321 && nav==0 && !native_setup_active());
        observe_flag_destroyed(321);test_now+=501;tick();
        check(native_setup_active() && nav==0 && placement().pending_setup==std::nullopt);
        tick();check(nav==native_name_hash("SpotBattle") && placement().seconds==20 && placement().players==1);
        nav=0;tick();check(navigations.size()==before_rejected+1); // No duplicate native navigation.
        abandon_flag_placement();
        // A native card can begin before the local capture is published. Flag
        // navigation must not cancel ownership simply because Steam is off.
        current_view.world=0;current_view.local=0;current_view.can_create=false;
        check(arm_native_setup(30,1));tick();nav=0;observe_flag_created();observe_flag_graph(0x0f131312);
        observe_flag_graph(0x9700be92);tick();check(flag_registration_owned() && placement().flag_seen);
        current_view.world=23;current_view.local=7;tick();
        check(placement().world==23 && placement().local==7 && placement().step==Step::placing);
        observe_flag_position(999,{2,3,4});observe_flag_exit(false);tick();
        check(handed_off.back().players==1 && handed_off.back().world==23 && handed_off.back().local==7);
        abandon_flag_placement();
        // Connecting while the native setup opens replaces offline IDs with
        // transport IDs. This is the same panel/level, not a stale flag.
        current_view.world=2;current_view.local=2;nav=0;
        check(arm_native_setup(30,1));tick();nav=0;
        const auto connecting_count=navigations.size();
        observe_flag_created();observe_flag_graph(0x0f131312);
        current_view.world=3715608209802941200ULL;current_view.local=76561198290301552ULL;
        tick();check(native_setup_active() && placement().created && placement().setup_seen);
        check(placement().world==current_view.world && placement().local==current_view.local && navigations.size()==connecting_count);
        observe_flag_graph(0x9700be92);tick();
        observe_flag_position(1000,{3,4,5});observe_flag_exit(false);tick();
        check(handed_off.back().world==current_view.world && handed_off.back().local==current_view.local);
        current_view.world=24;tick();check(!flag_registration_owned()); // Committed flags never rebind.
        check(arm_native_setup(20,1));tick();nav=0;
        abandon_flag_placement();check(!native_setup_active() && !flag_registration_owned());
        // Starting removes only the native lobby flag. The independently
        // coordinated 1-Up match and its saved facing remain intact.
        current_view.state={};nav=0;
        check(arm_native_setup(20,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(0x9700be92);tick();
        observe_flag_position(1001,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
        current_view.state.match=101;current_view.state.phase=Phase::countdown;tick();
        check(destroyed.back()==1001 && placement().destroy_queued);
        observe_flag_destroyed(1001);tick();
        check(current_view.state.match==101 && placement().mmid==0 && placement().facing==facing && !flag_registration_owned());
        const auto destroys=destroyed.size();current_view.state.phase=Phase::playing;tick();check(destroyed.size()==destroys);
        current_view.state.phase=Phase::cancelled;tick();tick();check(!flag_registration_owned());
        current_view.state={};nav=0;
        check(arm_native_setup(20,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        check(native_setup_active());
        observe_flag_graph(1867583346U);
        check(!native_setup_active()); // Back stops presentation adaptation immediately.
        const auto back_navigation_count=navigations.size();
        tick();check(nav==0 && placement().step==Step::setup && navigations.size()==back_navigation_count);
        // OnEnd is Back as well as Confirm. A tick within the grace period
        // must not synthesize ThrowdownPlacement after the menu was closed.
        test_now+=1501;tick();
        check(!flag_registration_owned() && navigations.size()==back_navigation_count);
        observe_flag_created();observe_flag_graph(0x0f131312);
        check(!native_setup_active()); // A later stock Spot Battle cannot claim this cancelled entry.
        check(arm_native_setup(20,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
        observe_flag_graph(0x9700be92);test_now+=1501;tick();
        check(placement().flag_seen && flag_registration_owned()); // Confirm proceeds to flag placement.
        abandon_flag_placement();
        test_mode_active=true;check(!arm_native_setup(20,1) && !native_setup_active());
        test_mode_active=false;check(arm_native_setup(20,1));abandon_flag_placement();
        // A Deathrace flag uses the same native Spot Battle picker, but hands
        // its pose to the route builder and retains its native waiting queue.
        current_view.state={};current_view.can_create=false;test_mode_active=true;nav=0;
        current_mode_view={};current_mode_view.active=current_mode_view.leading=true;
        current_mode_view.world=current_view.world;current_mode_view.game=12;
        current_mode_view.settings.mode=dingosdk::modes::Mode::race;
        current_mode_view.state=dingosdk::modes::Message{};
        current_mode_view.state->phase=dingosdk::modes::Phase::setup;current_mode_view.placing=true;
        check(!begin_race_flag_placement(current_view.world+1,12,1));
        check(begin_race_flag_placement(current_view.world,12,1) && race_flag_registration_owned());tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);tick();nav=0;
        observe_flag_graph(0x9700be92);tick();
        const auto one_up_handoffs=handed_off.size();
        observe_flag_position(1002,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
        check(race_handoffs==1 && race_world_seen==current_view.world && race_game_seen==12 && race_pose_seen==std::array<float,3>{7,8,9} && std::abs(race_yaw_seen-90.f)<.01f);
        const auto race_destroy_before=destroyed.size();
        check(handed_off.size()==one_up_handoffs && owns_flag_registration(1002) && native_flag_mode_game()==12 && race_flag_registration_owned());
        test_now+=300000;tick();
        check(race_handoffs==1 && destroyed.size()==race_destroy_before && owns_flag_registration(1002));
        // Complete Save releases the editor, but retains the roster and Start.
        current_mode_view.placing=false;current_mode_view.waiting=true;current_mode_view.route_finish=true;tick();
        check(native_flag_mode_waiting() && destroyed.size()==race_destroy_before && current_mode_view.state->phase==dingosdk::modes::Phase::setup);
        current_mode_view.state->phase=dingosdk::modes::Phase::countdown;tick();
        check(destroyed.size()==race_destroy_before+1 && destroyed.back()==1002);
        observe_flag_destroyed(1002);tick();check(!race_flag_registration_owned() && !flag_registration_owned());
        tick();check(race_handoffs==1);
        current_mode_view.state->phase=dingosdk::modes::Phase::results;tick();check(native_flag_mode_game()==0);
        // The Deathrace card opens the actual setup and waits for Confirm;
        // it must not automatically pass through setup like the flag-only API.
        current_mode_view.game=13;current_mode_view.state->phase=dingosdk::modes::Phase::setup;current_mode_view.placing=true;
        nav=0;check(begin_race_flag_placement(current_view.world,13,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);test_now+=20000;tick();
        check(native_setup_active() && native_setup_race_game()==13 && nav==0);
        set_native_setup_options(30,3);check(placement().players==3);
        check(!close_race_setup_for_start(12) && native_setup_active());
        observe_flag_graph(1867583346U);observe_flag_graph(0x9700be92);tick();
        check(!native_setup_active() && native_setup_race_game()==0 && placement().step==Step::placing);
        check(!close_race_setup_for_start(13)); // A running flag placement cannot be cancelled by Start.
        observe_flag_position(1003,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
        check(race_handoffs==2 && race_game_seen==13 && handed_off.size()==one_up_handoffs);
        // Selecting Start flag again closes the old MMID first, then opens a
        // fresh native picker. A different game cannot claim the queued entry.
        check(!begin_race_flag_placement(current_view.world,14,1));
        check(begin_race_flag_placement(current_view.world,13,3) && placement().pending_race_flag.has_value());
        tick();check(destroyed.back()==1003 && !native_flag_mode_waiting());
        observe_flag_destroyed(9999);test_now+=501;tick();check(owns_flag_registration(1003));
        observe_flag_destroyed(1003);test_now+=501;tick();
        check(!owns_flag_registration(1003) && placement().step==Step::open && placement().race_game==13 && !placement().pending_race_flag);
        nav=0;tick();check(nav==native_name_hash("SpotBattle"));nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);tick();nav=0;
        observe_flag_graph(0x9700be92);tick();observe_flag_position(1013,{17,8,19},&flag_transform);observe_flag_exit(false);tick();
        check(race_handoffs==3 && race_game_seen==13 && race_pose_seen==std::array<float,3>{17,8,19} && owns_flag_registration(1013));
        // Closing the draft destroys the retained registration and releases
        // the picker for another entry while the mode keeps its route draft.
        flag_registration_left();tick();check(destroyed.back()==1013);
        observe_flag_destroyed(1013);test_now+=501;tick();check(!race_flag_registration_owned());
        // Explicit extra settings actions may close an unconfirmed setup;
        // this is not the saved-route Confirm or a match Start.
        nav=0;check(begin_race_flag_placement(current_view.world,14,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        check(close_race_setup_for_start(14) && !native_setup_active() && race_flag_registration_owned());
        tick();check(nav==native_name_hash("ThrowdownerExit") && race_flag_registration_owned());
        observe_flag_graph(1867583346U);nav=0;tick();
        check(!race_flag_registration_owned() && race_handoffs==3 && handed_off.size()==one_up_handoffs);
        // Back out of Deathrace, then enter 1-Up: the next stock setup must
        // never inherit the race game ID or accidentally hand off its flag.
        nav=0;check(begin_race_flag_placement(current_view.world,15,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
        test_now+=1501;tick();check(!race_flag_registration_owned() && !native_setup_active());
        nav=0;check(begin_race_flag_placement(current_view.world,16,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        check(!begin_race_flag_placement(current_view.world,17,1,false)); // Flag-only entry cannot replace an open panel.
        const auto omitted_end_release=released;
        check(begin_race_flag_placement(current_view.world,16,1,true)); // A fresh grid click recovers omitted OnEnd.
        check(native_setup_race_game()==16 && released==omitted_end_release+1);
        tick();nav=0;observe_flag_created();observe_flag_graph(0x0f131312);
        observe_flag_graph(1867583346U);const auto previous_release=released;
        check(begin_race_flag_placement(current_view.world,17,1,true)); // Immediate re-entry after Back works.
        check(native_setup_race_game()==17 && released==previous_release+1);
        abandon_flag_placement();
        // Repeated native flag Back must end exactly its owning course, with
        // no nested mode request under the placement mutex. Retired custom
        // queue IDs stay identifiable until the actual level is abandoned.
        abandon_flag_placement();cancelled_modes.clear();
        current_view.state={};
        current_mode_view={};current_mode_view.active=true;current_mode_view.world=current_view.world;
        for(const std::uint32_t game:{90u,91u}) {
            current_mode_view.game=game;
            check(begin_race_flag_placement(current_view.world,game,1,false));
            observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(0x9700be92);
            observe_flag_queue(2000+game);check(known_custom_flag_queue(2000+game));
            const auto before=cancelled_modes.size();
            observe_flag_exit(true);
            check(cancelled_modes.size()==before+1 && cancelled_modes.back()==std::pair{current_view.world,game});
            check(quit_outside_placement_lock && placement().cancelling);
            { std::lock_guard lock(placement().mutex);reset(placement()); }
            observe_flag_exit(true);check(cancelled_modes.size()==before+1);
            check(known_custom_flag_queue(2000+game));
        }
        check(!known_custom_flag_queue(0) && !known_custom_flag_queue(9999));
        abandon_flag_placement();check(!known_custom_flag_queue(2090) && !known_custom_flag_queue(2091));
        test_mode_active=false;
        check(arm_native_setup(20,1) && native_setup_race_game()==0);abandon_flag_placement();
        nav=0;check(begin_race_flag_placement(current_view.world,25,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        const auto stale_release=released;
        retire_unconfirmed_native_setup();
        check(!flag_registration_owned() && !native_setup_active() && released==stale_release+1 && nav==0);
        check(arm_native_setup(20,1));tick();nav=0;observe_flag_created();observe_flag_graph(0x0f131312);
        observe_flag_graph(0x9700be92);retire_unconfirmed_native_setup();
        check(flag_registration_owned() && placement().flag_seen);abandon_flag_placement();
        // A second verified grid selection may arrive before the next bridge
        // tick. Supersede the queued unconfirmed entry without first opening
        // its stock setup or requiring a native OnEnd that cannot fire yet.
        current_view.state={};nav=0;
        const auto queued_navigation_count=navigations.size();
        check(arm_native_setup(20,1));
        retire_unconfirmed_native_setup();
        check(!flag_registration_owned() && !native_setup_active() && navigations.size()==queued_navigation_count);
        check(begin_race_flag_placement(current_view.world,26,1,true));tick();
        check(nav==native_name_hash("SpotBattle") && navigations.size()==queued_navigation_count+1 && native_setup_race_game()==26);
        observe_flag_graph(1867583346U); // The previous panel ended after this fresh card was queued.
        check(native_setup_active() && native_setup_race_game()==26 && !placement().setup_seen);
        nav=0;observe_flag_created();observe_flag_graph(0x0f131312);
        // Saved-route Start can close a native setup whose OnEnd is missing.
        // A fresh grid callback after returning to Throwdowns must release
        // that old adapter, rather than keeping every later card blocked.
        check(close_race_setup_for_start(26));tick();
        check(nav==native_name_hash("ThrowdownerExit") && race_flag_registration_owned());
        nav=0;test_now+=15001;tick();
        check(race_flag_registration_owned() && !native_setup_active());
        const auto cleanup_release=released;
        const auto cleanup_navigation_count=navigations.size();
        retire_unconfirmed_native_setup();
        check(!race_flag_registration_owned() && !flag_registration_owned() && released==cleanup_release+1);
        nav=native_name_hash("Back");tick();
        check(nav==native_name_hash("Back") && navigations.size()==cleanup_navigation_count);
        nav=0;check(arm_native_setup(20,1));tick();
        check(nav==native_name_hash("SpotBattle") && navigations.size()==cleanup_navigation_count+1 && native_setup_race_game()==0);
        abandon_flag_placement();nav=0;
        // Cancelling while the native flag camera is active must send Exit.
        // The earlier setup navigation's sent timestamp cannot suppress it,
        // and a grid callback must not retire an actual flag camera session.
        check(begin_race_flag_placement(current_view.world,27,1,true));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(0x9700be92);tick();
        const auto flag_camera_release=released;
        const auto flag_camera_navigation_count=navigations.size();
        flag_registration_left();retire_unconfirmed_native_setup();
        check(race_flag_registration_owned() && placement().flag_seen && released==flag_camera_release);
        check(!begin_race_flag_placement(current_view.world,28,1,true));
        tick();check(nav==native_name_hash("ThrowdownerExit") && navigations.size()==flag_camera_navigation_count+1);
        nav=0;flag_registration_left();tick();
        check(nav==0 && navigations.size()==flag_camera_navigation_count+1); // Repeated Leave queues no duplicate Exit.
        observe_flag_exit(true);tick();check(!race_flag_registration_owned());
        // Once a real MMID exists, re-entry waits for its native destroy ack.
        // Retiring an unconfirmed panel cannot bypass that lifetime, even
        // when the old 1-Up match has finished and a new setup is requested.
        nav=0;check(arm_native_setup(20,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(0x9700be92);tick();
        observe_flag_position(1004,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
        current_view.state.match=102;current_view.state.phase=Phase::finished;
        check(arm_native_setup(20,1));
        const auto committed_release=released;
        retire_unconfirmed_native_setup();
        check(owns_flag_registration(1004) && placement().pending_setup.has_value() && released==committed_release);
        tick();check(destroyed.back()==1004 && placement().destroy_queued && !native_setup_active());
        observe_flag_destroyed(9999);test_now+=501;tick();
        check(owns_flag_registration(1004) && !native_setup_active());
        observe_flag_destroyed(1004);test_now+=501;tick();
        check(native_setup_active() && released==committed_release+1 && placement().pending_setup==std::nullopt);
        tick();check(nav==native_name_hash("SpotBattle"));
        abandon_flag_placement();nav=0;
        // Native flag-only requests are not grid setup entries. Keep their
        // cleanup owned until native Exit acknowledges the cancellation.
        current_view.can_create=true;
        check(begin_flag_placement(20,1));tick();nav=0;
        observe_flag_created();flag_registration_left();
        const auto flag_only_release=released;
        retire_unconfirmed_native_setup();check(flag_registration_owned() && released==flag_only_release);
        tick();check(nav==native_name_hash("ThrowdownerExit"));
        nav=0;observe_flag_exit(true);tick();check(!flag_registration_owned());
        // Repeated native Back/re-entry and switching race -> 1-Up must never
        // leave the bridge stuck in flag placement or reuse the other badge.
        for(unsigned pass=0;pass<5;++pass) {
            nav=0;check(begin_race_flag_placement(current_view.world,30+pass,1,true));tick();nav=0;
            observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
            tick();check(nav==0 && !native_setup_active() && placement().step==Step::setup);
            check(arm_native_setup(20,1) && native_setup_race_game()==0);tick();nav=0;
            observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
            tick();check(nav==0 && placement().step==Step::setup);
            test_now+=1501;tick();check(!flag_registration_owned());
        }
        // Every generic card follows the native settings -> flag -> waiting
        // flow. Confirm/flag completion never starts any match, and solo
        // waiting stays available until the matching coordinator starts.
        current_view.state={};test_mode_active=true;nav=0;
        check(!begin_native_mode_setup(current_view.world+1,60,1));
        check(!begin_native_mode_setup(current_view.world,0,1));
        check(!begin_native_mode_setup(current_view.world,60,0));
        for(const auto mode:{dingosdk::modes::Mode::jam,dingosdk::modes::Mode::meat,
            dingosdk::modes::Mode::domination,dingosdk::modes::Mode::graffiti,
            dingosdk::modes::Mode::tag,dingosdk::modes::Mode::infection,dingosdk::modes::Mode::hide}) {
            const auto game=60+static_cast<std::uint32_t>(mode);
            const auto before_handoff=handed_off.size();const auto before_race_handoff=race_handoffs;
            const auto before_destroy=destroyed.size();const auto before_release=released;const auto before_mode_handoff=mode_handoffs;
            current_mode_view={};current_mode_view.active=current_mode_view.leading=true;
            current_mode_view.world=current_view.world;current_mode_view.game=game;current_mode_view.capacity=1;
            current_mode_view.settings.mode=mode;current_mode_view.state=dingosdk::modes::Message{};
            current_mode_view.state->phase=dingosdk::modes::Phase::setup;
            nav=0;check(begin_native_mode_setup(current_view.world,game,1));
            check(native_setup_active() && native_setup_mode_game()==game && native_setup_race_game()==0);
            check(mode_flag_registration_owned(game) && !mode_flag_registration_owned(game+100) && !race_flag_registration_owned());
            tick();check(nav==native_name_hash("SpotBattle"));nav=0;
            observe_flag_created();observe_flag_graph(0x0f131312);test_now+=20000;tick();
            check(native_setup_active() && nav==0 && !placement().flag_seen);
            observe_flag_graph(1867583346U);observe_flag_graph(0x9700be92);tick();
            check(!native_setup_active() && placement().step==Step::placing && native_flag_mode_game()==game);
            check(!native_flag_mode_waiting() && mode_handoffs==before_mode_handoff);
            check(!close_race_setup_for_start(game)); // Confirm is actual flag placement, not direct Start.
            const auto mmid=1100+static_cast<std::uint32_t>(mode);
            observe_flag_position(mmid,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
            check(mode_handoffs==before_mode_handoff+1 && mode_world_seen==current_view.world && mode_game_seen==game);
            check(mode_pose_seen==std::array<float,3>{7,8,9} && std::abs(mode_yaw_seen-90.f)<.01f);
            check(native_flag_mode_waiting() && mode_flag_registration_owned(game) && owns_flag_registration(mmid));
            check(handed_off.size()==before_handoff && race_handoffs==before_race_handoff && destroyed.size()==before_destroy);
            check(released==before_release+1 && current_mode_view.state->phase==dingosdk::modes::Phase::setup);
            test_now+=300000;tick();
            check(native_flag_mode_waiting() && mode_handoffs==before_mode_handoff+1 && destroyed.size()==before_destroy);
            current_mode_view.state->phase=dingosdk::modes::Phase::countdown;tick();
            check(destroyed.size()==before_destroy+1 && destroyed.back()==mmid && placement().destroy_queued);
            observe_flag_destroyed(mmid);tick();
            check(!mode_flag_registration_owned() && !flag_registration_owned() && native_flag_mode_game()==game);
            current_mode_view.state->phase=dingosdk::modes::Phase::playing;tick();
            check(destroyed.size()==before_destroy+1 && mode_handoffs==before_mode_handoff+1);
            current_mode_view.state->phase=dingosdk::modes::Phase::results;tick();
            check(native_flag_mode_game()==0 && !flag_registration_owned() && test_mode_active);
        }
        // Native Quit owns and destroys only this generic lobby's flag. A
        // fresh card cannot bypass the destroy acknowledgment or reuse it.
        current_mode_view={};current_mode_view.active=true;current_mode_view.world=current_view.world;current_mode_view.game=80;
        nav=0;check(begin_native_mode_setup(current_view.world,80,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(0x9700be92);tick();
        const auto stray_handoff=handed_off.size();const auto stray_race_handoff=race_handoffs;
        observe_flag_position(1005,{7,8,9},&flag_transform);observe_flag_exit(false);tick();
        check(native_flag_mode_waiting() && owns_flag_registration(1005));
        current_mode_view={};flag_registration_left();retire_unconfirmed_native_setup();tick();
        check(destroyed.back()==1005 && owns_flag_registration(1005) && !native_flag_mode_waiting());
        check(!begin_native_mode_setup(current_view.world,81,1));
        observe_flag_destroyed(9999);test_now+=501;tick();check(owns_flag_registration(1005));
        observe_flag_destroyed(1005);test_now+=501;tick();
        check(!flag_registration_owned() && handed_off.size()==stray_handoff && race_handoffs==stray_race_handoff);
        // Extra settings actions may close an unconfirmed generic setup,
        // but cannot start flag placement or hand off a pose while closing.
        nav=0;check(begin_native_mode_setup(current_view.world,83,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        const auto extra_mode_handoffs=mode_handoffs;
        check(close_race_setup_for_start(83) && mode_flag_registration_owned(83) && !race_flag_registration_owned());
        tick();check(nav==native_name_hash("ThrowdownerExit"));
        nav=0;observe_flag_graph(1867583346U);tick();
        check(!mode_flag_registration_owned() && !flag_registration_owned() && mode_handoffs==extra_mode_handoffs);
        // Back without OnEnd is recoverable only on a fresh verified card;
        // quitting this draft also keeps all native flag callbacks unowned.
        check(begin_native_mode_setup(current_view.world,81,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);
        retire_unconfirmed_native_setup();check(!native_setup_active() && native_setup_mode_game()==0);
        check(begin_native_mode_setup(current_view.world,82,1));tick();nav=0;
        observe_flag_created();observe_flag_graph(0x0f131312);observe_flag_graph(1867583346U);
        test_now+=1501;tick();check(!flag_registration_owned());
        test_mode_active=false;
        check(arm_native_setup(20,1) && native_setup_mode_game()==0);abandon_flag_placement();
        current_view.world=901;current_view.local=77;current_view.state={};
        current_mode_view={};current_mode_view.active=true;current_mode_view.leading=false;
        current_mode_view.world=901;current_mode_view.game=401;current_mode_view.settings.mode=dingosdk::modes::Mode::race;
        current_mode_view.state=dingosdk::modes::Message{};current_mode_view.state->phase=dingosdk::modes::Phase::setup;
        test_mode_active=true;
        const auto handoffs_before_guest=race_handoffs;
        check(!adopt_native_mode_flag(900,401,5001,{10,20,30},90,120,6,true));
        check(adopt_native_mode_flag(901,401,5001,{10,20,30},90,120,6,true));
        check(owns_flag_registration(5001) && known_custom_flag_queue(5001) && native_flag_mode_waiting() && race_flag_registration_owned());
        check(!adopt_native_mode_flag(901,402,5002,{10,20,30},90,120,6,true));
        tick();check(race_handoffs==handoffs_before_guest && native_flag_mode_waiting());
        const auto guest_destroy_before=destroyed.size();
        current_mode_view.state->phase=dingosdk::modes::Phase::countdown;tick();
        check(destroyed.size()==guest_destroy_before+1 && destroyed.back()==5001 && test_mode_active);
        observe_flag_destroyed(5001);tick();check(!flag_registration_owned() && test_mode_active);
        current_mode_view.state->phase=dingosdk::modes::Phase::results;tick();check(native_flag_mode_game()==0);
        abandon_flag_placement();
        current_mode_view={};test_mode_active=false;current_view.state={};
        check(adopt_native_one_up_flag(901,6001,{10,20,30},90,30,6));
        tick();check(flag_registration_waiting() && owns_flag_registration(6001) && native_flag_mode_game()==0);
        current_view.state.match=0x1234567800000021ULL;current_view.state.phase=Phase::lobby;tick();
        check(flag_registration_waiting());
        flag_registration_left();tick();check(destroyed.back()==6001);
        observe_flag_destroyed(6001);test_now+=501;tick();
        current_view.state={};
        check(adopt_native_one_up_flag(901,6002,{10,20,30},90,30,6));
        check(flag_registration_waiting() && owns_flag_registration(6002));
        abandon_flag_placement();
        std::cout<<count<<" flag bridge checks passed\n";
        return 0;
    } catch(const std::exception& e) { std::cerr<<e.what()<<'\n'; return 1; }
}
