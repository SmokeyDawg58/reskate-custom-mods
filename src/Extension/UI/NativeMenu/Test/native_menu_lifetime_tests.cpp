#include "Extension/UI/NativeMenu/native_menu_lifetime.h"
#include "Extension/UI/NativeMenu/native_array_publication.h"
#include "Extension/UI/NativeMenu/native_menu_ownership.h"
#include "Extension/UI/NativeMenu/native_setup_roles.h"
#include "Extension/UI/NativeMenu/native_hud_selection.h"
#include "Extension/UI/NativeMenu/native_hud_values.h"
#include "Extension/Throwdowns/native_flag_preview.h"
#include "Extension/Throwdowns/native_flag_bubble.h"
#include <cstdio>
#include <array>
#include <map>
#include <limits>
#include <stdexcept>
#include <string>
#include <vector>
#include <functional>

using dingosdk::multiplayer::menu_data::OwnedMenuModels;
using dingosdk::multiplayer::menu_data::MenuLifetime;
using dingosdk::multiplayer::menu_data::MenuAction;
using dingosdk::multiplayer::menu_data::action_slot;
struct Model { unsigned handle, type; };
void check(bool value, const char* message) { if (!value) throw std::runtime_error(message); }

void map_transition() {
    OwnedMenuModels<Model> owned;
    std::map<unsigned, unsigned> live{{1, 100}, {2, 200}, {3, 300}};
    owned.track({2, 200}); // First pause-menu instance.
    owned.track({3, 300}); // Partially built replacement instance.
    owned.track({3, 300}); // Engine create can return an existing owned root.
    bool attached = true, assets_loaded = true;
    std::vector<unsigned> destroyed;
    const auto destroy = [&](Model value) {
        check(!attached, "Detach tabs before destroying their data");
        check(assets_loaded, "Release list templates while their assets are still loaded");
        check(live.at(value.handle) == value.type, "Destroy only matching owned roots");
        live.erase(value.handle);
        destroyed.push_back(value.handle);
    };
    owned.release([&] { attached = false; }, destroy);
    assets_loaded = false; // The common loader may now submit either menu's request.
    check(owned.empty() && live.size() == 1 && live.contains(1), "Native base-game roots remain owned by the game");
    check(destroyed == std::vector<unsigned>{3, 2}, "Partial builds and older menu models are released once");
    owned.release([&] { throw std::runtime_error("Repeated teardown must be inert"); }, destroy);
    assets_loaded = true;
    live.emplace(4, 400);
    owned.track({4, 400}); // Reopening after loading creates a fresh menu.
    owned.release([] {}, destroy);
    check(destroyed.back() == 4 && owned.empty(), "The next world gets an independent menu lifetime");
}
void interrupted_cleanup() {
    OwnedMenuModels<Model> owned;
    owned.track({10, 100}); owned.track({20, 200});
    unsigned calls{};
    bool failed{};
    try {
        owned.release([] { throw std::runtime_error("Detach failed"); }, [&](Model) { ++calls; });
    } catch (const std::runtime_error&) { failed = true; }
    check(failed && calls == 0 && !owned.empty(), "Failed detach must preserve models and prevent unload");
    std::vector<unsigned> destroyed;
    try {
        owned.release([] {}, [&](Model value) {
            if (value.handle == 10) throw std::runtime_error("Destroy failed");
            destroyed.push_back(value.handle);
        });
    } catch (const std::runtime_error&) { }
    owned.release([] {}, [&](Model value) { destroyed.push_back(value.handle); });
    check(destroyed == std::vector<unsigned>{20, 10}, "Retry must finish cleanup without double destruction");
    owned.track({30, 300});
    owned.manager_replaced();
    owned.release([] {}, [&](Model) { throw std::runtime_error("Old manager handle reused"); });
    check(owned.empty(), "Replacing the native manager must discard its expired handles");
}
void partial_build_rollback() {
    OwnedMenuModels<Model> owned;
    std::map<unsigned,unsigned> live{{1,100}};
    owned.track({1,100}); // A mounted menu that precedes the optional panel.
    const auto checkpoint=owned.checkpoint();
    std::vector<unsigned> destroyed;
    const auto destroy=[&](Model value) {
        check(value.handle!=1,"A failed optional panel must preserve the mounted menu");
        check(live.at(value.handle)==value.type,"Rollback destroys only roots owned by its build");
        live.erase(value.handle);destroyed.push_back(value.handle);
    };
    // A persistent missing dependency must not retain a growing chain of
    // half-built panel roots over successive menu ticks.
    for(unsigned attempt=0;attempt<100;++attempt) {
        const unsigned first=2+attempt*2,second=first+1;
        live.emplace(first,200);owned.track({first,200});
        live.emplace(second,300);owned.track({second,300});
        owned.track({second,300});owned.track({1,100}); // Retained/deduplicated roots are not new build allocations.
        try {throw std::runtime_error("Optional native panel dependency unavailable");}
        catch(const std::runtime_error&) {owned.rollback(checkpoint,destroy);}
        check(owned.checkpoint()==checkpoint && live.size()==1 && live.contains(1),
            "Every failed build returns to its checkpoint without destroying earlier roots");
        check(destroyed[attempt*2]==second && destroyed[attempt*2+1]==first,
            "Rollback releases a failed build in reverse creation order");
    }
    owned.rollback(checkpoint,[](Model){throw std::runtime_error("An empty rollback must be inert");});
    bool rejected{};
    try {owned.rollback(checkpoint+1,destroy);}
    catch(const std::out_of_range&) {rejected=true;}
    check(rejected && owned.checkpoint()==checkpoint && destroyed.size()==200 && live.contains(1),
        "Reject an invalid checkpoint before destroying any owned model");
    owned.release([] {},[&](Model value){live.erase(value.handle);});
    check(owned.empty() && live.empty(),"Normal teardown still releases the retained menu after rollback");
}
void interrupted_build_rollback() {
    OwnedMenuModels<Model> owned;
    owned.track({10,100});
    const auto checkpoint=owned.checkpoint();
    for(const auto value:{Model{20,200},Model{30,300},Model{40,400}})owned.track(value);
    std::vector<unsigned> destroyed;
    bool failed{};
    try {
        owned.rollback(checkpoint,[&](Model value) {
            if(value.handle==30)throw std::runtime_error("Native model destroy unavailable");
            destroyed.push_back(value.handle);
        });
    } catch(const std::runtime_error&) {failed=true;}
    check(failed && destroyed==std::vector<unsigned>{40} && owned.checkpoint()==checkpoint+2,
        "Failed rollback preserves the failed root and remaining build for a retry");
    owned.rollback(checkpoint,[&](Model value){destroyed.push_back(value.handle);});
    check(destroyed==std::vector<unsigned>{40,30,20} && owned.checkpoint()==checkpoint,
        "Retry finishes a partial rollback without destroying completed roots twice");
    owned.manager_replaced();
    bool rejected{};
    try {owned.rollback(checkpoint,[&](Model value){destroyed.push_back(value.handle);});}
    catch(const std::out_of_range&) {rejected=true;}
    check(rejected && owned.empty() && destroyed.size()==3,
        "A checkpoint invalidated by manager replacement cannot destroy expired handles");
    owned.rollback(0,[](Model){throw std::runtime_error("An empty manager has no roots to roll back");});
}
void activity_details_ownership() {
    using dingosdk::multiplayer::menu_data::ActivityDetailsOwnership;
    ActivityDetailsOwnership lease;
    constexpr std::uint64_t private_page=100,stock_page=200,menu_layer=300;
    constexpr int menu_item=7;
    unsigned closes{};
    const auto close=[&](std::uint64_t page,std::uint64_t layer,int item) {
        if(lease.owns(page,layer,item))++closes;
    };
    lease.request(); // The shared enum is Details, but our page has not mounted.
    close(stock_page,menu_layer,menu_item);
    check(closes==0,"A pending Start request must not close a stock native Details page");
    lease.mount(private_page,menu_layer,menu_item);
    close(stock_page,menu_layer,menu_item);
    close(private_page,menu_layer+1,menu_item);
    close(private_page,menu_layer,menu_item+1);
    check(closes==0,"A shared Details enum or reused item id does not establish ownership");
    close(private_page,menu_layer,menu_item);
    check(closes==1,"Quit may close the exact private native activity layer item");
    // Native arrays can relocate inline fields without changing the owner.
    check(lease.owns(private_page,menu_layer,menu_item),"Array relocation keeps the mounted page identity");
    lease.clear_mount();
    close(private_page,menu_layer,menu_item);
    check(lease.requested && closes==1,"Restoring a pending transition drops only its mounted page ownership");
    lease.mount(private_page,menu_layer,menu_item);
    lease.retire(); // Native Back or challenge end restores the mount.
    for(unsigned tick=0;tick<20;++tick) {
        close(stock_page,menu_layer,menu_item);close(private_page,menu_layer,menu_item);
    }
    check(!lease.requested && !lease.page && !lease.layer && closes==1,
        "Back/end retirement cannot close a later stock page or reused private page on future ticks");
    lease.request();lease.mount(private_page,menu_layer,menu_item);
    lease={}; // Native manager replacement invalidates every old identity.
    close(private_page,menu_layer,menu_item);
    check(closes==1,"Manager replacement discards every stale activity Details owner");
    lease.request();lease.mount(private_page+1,menu_layer+1,menu_item);
    close(private_page,menu_layer,menu_item);close(private_page+1,menu_layer+1,menu_item);
    check(closes==2,"A later custom match acquires an independent native activity Details lease");
}
void native_activity_stack_selection() {
    using dingosdk::multiplayer::menu_data::ActivityDetailsOwnership;
    using dingosdk::multiplayer::menu_data::active_activity_stack_item;
    // Saved official panel-001/006: a settled visible Page_Widget has a
    // cleared transition TargetIndex, while StackIndex identifies item zero.
    struct Item {std::uint64_t page;int id;bool active;};
    struct Stack {int target,current,focus;bool active;std::vector<Item> items;};
    constexpr std::uint64_t layer=300,private_page=100,stock_page=200;
    Stack stack{-1,0,0,true,{{stock_page,7,true}}};
    const auto selected=[&]() -> const Item* {
        const auto index=active_activity_stack_item(stack.current,static_cast<unsigned>(stack.items.size()),stack.active);
        if(!index || !stack.items[*index].active)return nullptr;
        return &stack.items[*index];
    };
    check(stack.target==-1 && selected() && selected()->page==stock_page,
        "A settled native Details page remains selectable when its transition target is -1");
    ActivityDetailsOwnership lease;lease.request();
    check(!lease.owns(selected()->page,layer,selected()->id),
        "Finding an active stock Details page does not grant close ownership to a pending request");
    stack.items[0].page=private_page;lease.mount(private_page,layer,stack.items[0].id);
    check(lease.owns(selected()->page,layer,selected()->id),
        "The mounted private current item can be closed after TargetIndex has cleared");
    // A transition can target another retained page before it becomes current.
    // Neither a target nor page-row focus is a safe substitute for StackIndex.
    stack.items.push_back({stock_page,8,true});stack.target=1;stack.focus=1;
    check(selected()->page==private_page && lease.owns(selected()->page,layer,selected()->id),
        "A transition target or focus change cannot redirect publication or close to a foreign page");
    stack.current=1;
    check(selected()->page==stock_page && !lease.owns(selected()->page,layer,selected()->id),
        "A newly current stock page keeps its own native activity owner");
    for(const auto invalid:{-1,2,std::numeric_limits<int>::max()}) {
        stack.current=invalid;
        check(!selected(),"A missing or out-of-range current index has no fallback item");
    }
    stack.current=0;stack.active=false;
    check(!selected(),"An inactive native menu layer cannot be mounted or closed");
    stack.active=true;stack.items[0].active=false;
    check(!selected(),"An inactive retiring stack item cannot be mounted or closed");
    stack.items.clear();
    check(!selected(),"An empty native menu layer has no current page");
    lease.retire();stack.items={{private_page,7,true}};
    check(!lease.owns(selected()->page,layer,selected()->id),
        "Retiring Back/end ownership prevents future closes even if native item identity is reused");
}
void window_close() {
    MenuLifetime lifetime;
    lifetime.before_transition(13, [] {});
    std::array<OwnedMenuModels<Model>, 2> pages;
    pages[0].track({10, 100}); pages[1].track({20, 200});
    std::array<bool, 2> attached{true, true};
    bool assets_loaded = true;
    unsigned destroyed{}, cleanups{};
    const auto cleanup = [&] {
        ++cleanups;
        check(lifetime.blocked(), "Stop updates before cleanup can invoke UI callbacks");
        for (unsigned slot = 0; slot < pages.size(); ++slot) {
            pages[slot].release([&] { attached[slot] = false; }, [&](Model) {
                check(!attached[slot] && assets_loaded,
                    "Close-window cleanup must release widget references before asset unload");
                ++destroyed;
            });
        }
    };
    for (unsigned state : {13U, 13U, 21U}) lifetime.before_transition(state, cleanup);
    check(cleanups == 0 && !lifetime.blocked(), "Active-state notifications must not destroy menu models");
    lifetime.before_transition(24, cleanup); // Before native state-24 handler.
    assets_loaded = false; // The native handler unloads the widget asset arena.
    for (unsigned state : {24U, 25U, 25U, 13U, 21U, 14U}) {
        lifetime.before_transition(state, cleanup);
        if (!lifetime.blocked()) pages[0].track({30, 300}); // A remaining client tick.
    }
    check(cleanups == 1 && destroyed == 2 && pages[0].empty() && pages[1].empty(),
        "Both menu tabs must stay released through all remaining shutdown ticks");
    for (auto& page : pages)
        page.release([] {}, [&](Model) { throw std::runtime_error("Late widget release reproduces the shutdown crash"); });

    MenuLifetime failed;
    failed.before_transition(24, [] {}); // Cleanup can report failure without clearing the latch.
    check(failed.blocked(), "Even an incomplete cleanup must prevent new models during shutdown");
}
void native_transition_without_load_request() {
    // The multiplayer crash used the game's transition path. No ReSkate load
    // was queued, so the scheduler's explicit cleanup never ran. Reopening the
    // pause menu and later ticks must not retain/recreate the old asset refs.
    for (unsigned leaving : {14U, 22U, 3U}) {
        MenuLifetime lifetime;
        std::array<OwnedMenuModels<Model>, 2> pages;
        check(lifetime.blocked(), "Do not build menus before the first active world");
        lifetime.before_transition(13, [] { throw std::runtime_error("Unexpected initial cleanup"); });
        pages[0].track({10, 100}); pages[1].track({20, 200});
        bool assets_loaded = true;
        unsigned cleanups{}, destroyed{}, published{}, actions{};
        const auto cleanup = [&] {
            ++cleanups;
            check(lifetime.blocked(), "Block reentrant callbacks before native transition cleanup");
            for (auto& page : pages) page.release([] {}, [&](Model) {
                check(assets_loaded, "Destroy menu asset references before the engine frees assets");
                ++destroyed;
                if (!lifetime.blocked()) ++actions;
            });
        };
        lifetime.before_transition(leaving, cleanup);
        assets_loaded = false;
        for (unsigned state : {leaving, 3U, 4U, 8U, 9U, 12U}) {
            lifetime.before_transition(state, cleanup);
            if (!lifetime.blocked()) { ++published; ++actions; }
        }
        check(cleanups == 1 && destroyed == 2 && published == 0 && actions == 0,
            "Native unload must clean both pages once and suppress updates/actions throughout loading");
        for (auto& page : pages) page.release([] {}, [&](Model) {
            throw std::runtime_error("Late model destruction reproduces the multiplayer crash");
        });
        assets_loaded = true;
        lifetime.before_transition(leaving == 22 ? 21 : 13, cleanup);
        check(!lifetime.blocked(), "The next active level/sublevel must allow reopening the menu");
        pages[0].track({30, 300}); pages[1].track({40, 400});
        lifetime.before_transition(14, cleanup);
        check(cleanups == 2 && destroyed == 4, "Clean subsequent worlds, not just the first transition");
        lifetime.before_transition(24, cleanup);
        check(lifetime.blocked() && destroyed == 4, "Shutdown during loading must not release roots twice");
    }
}
void action_slots() {
    // A page's buttons, rendered the way the menu does it: every button asks for
    // its command's slot on every pass, and a map load starts a new generation.
    constexpr std::size_t capacity = 8;
    std::array<MenuAction, capacity> actions;
    std::size_t used{};
    std::uint64_t pass{}, generation{};
    const auto ask = [&](std::string command, std::string argument = {}) {
        const auto slot = action_slot(used, capacity, [&](std::size_t i) -> const MenuAction& { return actions[i]; },
            command, argument, pass);
        if (!slot) return capacity;
        actions[*slot] = {generation, std::move(command), std::move(argument), pass};
        if (*slot == used) ++used;
        return *slot;
    };
    const auto render = [&](std::initializer_list<const char*> buttons) {
        ++pass;
        std::vector<std::size_t> slots;
        for (const auto* button : buttons) slots.push_back(ask("load", button));
        return slots;
    };
    const auto first = render({"a", "b", "c", "d", "e", "f"});
    check(used == 6 && first == std::vector<std::size_t>{0, 1, 2, 3, 4, 5}, "Each command takes the next unused slot");
    // Fifteen map loads filled the table when every generation took new slots.
    for (unsigned load = 0; load < 100; ++load) {
        ++generation;
        check(render({"a", "b", "c", "d", "e", "f"}) == first && used == 6,
            "The same buttons after a map load keep their slots");
        check(actions[0].generation == generation, "A kept slot belongs to the menu that is up now");
    }
    // A list whose rows change (lobbies, players): the slots of rows that went away are reused.
    check(render({"a", "b", "c", "d", "e", "f", "g", "h"}).back() == 7 && used == capacity, "The table can fill");
    check(render({"a", "b", "c", "d", "e", "f", "g", "h"}).back() == 7, "A full table still serves the commands it holds");
    check(ask("load", "i") == capacity, "A slot a button asked for in this pass or the last is not given away");
    render({"a", "b", "c", "d", "e", "f"});
    check(ask("load", "i") == capacity, "One pass without its button is not enough: the pass may not be over");
    const auto later = render({"a", "b", "c", "d", "e", "f", "i"});
    check(later.back() == 6 && actions[6].argument == "i", "A slot no button has asked for is handed to a new command");
    check(render({"a", "b", "c", "d", "e", "f", "i", "g"}).back() == 7 && actions[7].argument == "g",
        "A command that lost its slot gets another when its button returns");
}
void live_hud_binding() {
    using dingosdk::multiplayer::menu_data::HudRootCandidate;
    using dingosdk::multiplayer::menu_data::mounted_hud_root;
    // These identities/order reproduce the captured running Spot Battle HUD:
    // first the registered authored defaults, then the populated live root.
    std::array roots{HudRootCandidate{0x1000103210000ULL,false,0},
                     HudRootCandidate{0x16ced0000ULL,true,3}};
    check(mounted_hud_root(roots)==0x16ced0000ULL,
          "Bind the rendered Spot Battle HUD, not its registered template");
    roots[0].active=true; // Appended 1-Up items must not qualify a template.
    check(mounted_hud_root(roots,roots[0].handle)==roots[1].handle,
          "Ignore an earlier mistaken binding with no game-owned HUD contents");
    std::swap(roots[0],roots[1]);
    check(mounted_hud_root(roots)==0x16ced0000ULL,
          "Registry order cannot decide which HUD is rendered");
    roots[0].active=false;
    check(mounted_hud_root(roots)==0,"Wait when only inactive/empty HUD roots remain");
    roots[1]={0x180000000ULL,true,2};
    check(mounted_hud_root(roots,0x16ced0000ULL)==0x180000000ULL,
          "Rebind when the previous live HUD retires");
    roots[1].gameplay_stacks=0;
    check(mounted_hud_root(roots)==0,"An empty root cannot establish a new HUD binding");
    check(mounted_hud_root(roots,roots[1].handle)==roots[1].handle,
          "Keep the verified active HUD through temporarily empty native stacks");
    roots[0]={0x190000000ULL,true,1};
    check(mounted_hud_root(roots,roots[1].handle)==roots[0].handle,
          "A newly mounted populated root supersedes an empty previous root");
    roots[0].active=false;roots[1].active=false;
    check(mounted_hud_root(roots,roots[1].handle)==0,"Never retain a retired inactive HUD");
}
void native_hud_clock_and_penalties() {
    using dingosdk::multiplayer::menu_data::native_hud_seconds;
    using dingosdk::multiplayer::menu_data::earned_one_up_letter;
    check(native_hud_seconds(3000) == 3 && native_hud_seconds(2980) == 3,
          "The native animated countdown receives whole seconds, rounded up");
    check(native_hud_seconds(1000) == 1 && native_hud_seconds(999) == 1 &&
          native_hud_seconds(1) == 1 && native_hud_seconds(0) == 0,
          "GO begins only after the synchronized clock reaches zero");
    check(native_hud_seconds(28000) == 28,
          "Match the captured native Spot Battle timer's 28-second value");
    constexpr auto maximum = std::numeric_limits<std::uint64_t>::max();
    check(native_hud_seconds(maximum) == maximum / 1000 + 1,
          "Rounding large clock values cannot overflow");
    for (unsigned penalties = 0; penalties <= 3; ++penalties) {
        unsigned earned{};
        for (unsigned letter = 0; letter < 3; ++letter)
            earned += earned_one_up_letter(penalties, letter);
        check(earned == penalties, "Show all three stamps and highlight each earned 1-U-P penalty");
        check(!earned_one_up_letter(penalties, 3), "There is no fourth penalty stamp");
    }
}
void native_one_up_slider_values() {
    using namespace dingosdk::multiplayer::menu_data;
    check(native_slider_position(120,120,120)==0.f && native_slider_choice(1.f,120,120,10)==120,
          "A fixed Deathrace deadline has a finite readonly native slider");
    check(native_setup_role("Finish deadline")==NativeSetupRole::timer,
          "Renamed Deathrace deadline remains identifiable after a UI refresh");
    check(native_slider_choice((1.16f-1.f)/5.f,1,6,1)==1,
          "The captured fractional Players value snaps to a whole player");
    check(native_slider_choice((106.8f-10.f)/110.f,10,120,10)==110,
          "The captured fractional timer snaps to a ten-second choice");
    for(unsigned players=1;players<=6;++players)
        check(native_slider_choice(native_slider_position(players,1,6),1,6,1)==players,
              "All supported player counts round trip through native normalized storage");
    for(unsigned seconds=10;seconds<=120;seconds+=10)
        check(native_slider_choice(native_slider_position(seconds,10,120),10,120,10)==seconds,
              "Native turn times round trip without fractional display values");
    check(native_slider_choice(native_slider_position(20,10,120),10,120,10)==20,
          "A fresh 1-Up setup defaults to twenty seconds");
    check(native_slider_choice(std::numeric_limits<float>::quiet_NaN(),1,6,1)==1 &&
          native_slider_choice(4.f,1,6,1)==6 && native_slider_choice(-4.f,1,6,1)==1,
          "Invalid native input cannot create an unsupported player count");
}
void wrapped_waiting_notification() {
    using namespace dingosdk::multiplayer::menu_data;
    // Reproduce the captured live secondary stack: the outer Foundations
    // wrapper points to the actual Spot Battle waiting notification.
    std::map<std::uint64_t,NotificationMountNode> slots{
        {10,{0x174b30000ULL,0xffb83e48,11}},
        {11,{0x174a90000ULL,0x1053b9ec,0}},
        {20,{0x15d8b0000ULL,0x1053b9ec,0}}, // Unmounted RIP-score template.
        {30,{999,0x0e3be640,11}}, // Unrelated HUD content is not a queue wrapper.
        {40,{888,0xffb83e48,41}}, {41,{887,0xffb83e48,40}}};
    const auto read=[&](std::uint64_t handle){return slots.at(handle);};
    check(mounted_notification(10,read)==NotificationMount{11,0x174a90000ULL},
          "Resolve the rendered waiting model and its inner presenter through the native wrapper");
    slots[11]={123,0x608f1aab,0}; // During play, inner slot shows the native Leave button.
    const auto original=[&](std::uint64_t handle) {
        return handle==11?NotificationMountNode{0x174a90000ULL,0x1053b9ec,0}:read(handle);
    };
    check(mounted_notification(10,original)==NotificationMount{11,0x174a90000ULL},
          "A compact Leave presentation retains the same queue identity for restoration");
    check(!mounted_notification(30,read) && !mounted_notification(40,read) &&
          !mounted_notification(0,read),"Ignore unrelated wrappers, missing mounts and cycles");
    check(owns_waiting_presentation(123,123,false) && owns_waiting_presentation(0,123,true),
          "Cancellation retains ownership of the hidden custom queue card");
    check(!owns_waiting_presentation(456,123,true) && !owns_waiting_presentation(0,123,false),
          "A later native Spot Battle remount cannot inherit custom suppression");
    struct Copy {std::uint64_t handle;};
    const std::map<std::uint64_t,Copy> copies{{0x174a90000ULL,{0x18c660000ULL}}};
    slots[50]={777,0xffb83e48,51};
    slots[51]={0x18c660000ULL,0x1053b9ec,0};
    const auto remounted=[&](std::uint64_t handle) {
        auto node=read(handle);node.model=waiting_notification_source(node.model,copies);return node;
    };
    check(mounted_notification(50,remounted)==NotificationMount{51,0x174a90000ULL},
          "A private 1-Up card remounted in a new slot resolves its native source for Deathrace rebinding");
    check(waiting_notification_source(456,copies)==456,
          "A new official notification keeps its own identity");
}
void reused_waiting_card_controls() {
    using namespace dingosdk::multiplayer::menu_data;
    constexpr std::uint64_t source=0x1ac570000ULL,copy=0x1ac8a0000ULL;
    const MenuAction start{11,"flag-wait-start",std::to_string(source),443};
    const MenuAction leave{11,"flag-wait-leave",std::to_string(source),443};
    // Captured failure: the same private waiting card moves to two new
    // presenters after Deathrace -> Leave -> 1-UP, retaining old delegates.
    // Both inputs must survive a new picker generation and remain attached
    // to the current custom flag, rather than a captured Deathrace game id.
    for(const auto& action:{start,leave}) {
        check(action_generation_current(action,22),"Reused waiting delegates survive the previous picker generation");
        check(action_generation_current(action,0),"A confirmed waiting card does not require an open picker");
        check(owns_waiting_action(source,source,copy,copy,true,false,true),
              "The reused private lobby card still owns Start and Leave");
        check(!owns_waiting_action(source,source,0,copy,true,false,true),
              "A hidden retired card cannot control the next flag");
        check(!owns_waiting_action(source,source,999,copy,true,false,true),
              "A stock remount cannot dispatch a custom lobby delegate");
        check(!owns_waiting_action(source+1,source,copy,copy,true,false,true),
              "A different notification cannot control this custom flag");
        check(!owns_waiting_action(source,source,copy,copy,false,false,true) &&
              !owns_waiting_action(source,source,copy,copy,true,true,true) &&
              !owns_waiting_action(source,source,copy,copy,true,false,false),
              "Unowned, cancelling and running registrations reject lobby controls");
    }
    check(!action_generation_current({11,"mode-wait-leave","1384877487",443},22) &&
          !action_generation_current({11,"open","",443},22),
          "Old Deathrace and picker delegates remain guarded by their own generation");
    check(!action_generation_current({0,"flag-wait-leave",std::to_string(source),443},22),
          "An uninitialized waiting delegate cannot activate");
}
void private_throwdown_graphs() {
    using dingosdk::multiplayer::menu_data::UiCloneCache;
    struct Model {std::string name;std::vector<unsigned> children;};
    // Header/body share a native label; the callback owner points back to the
    // panel. Another stock panel also uses that same label and must stay stock.
    std::map<unsigned,Model> nodes{{1,{"Spot Battle",{2,3}}},{2,{"Spot rules",{4}}},
        {3,{"Footer",{4}}},{4,{"Native label",{1}}},{5,{"Skate Jam",{4}}}};
    UiCloneCache<unsigned,unsigned> one_up;unsigned next=100,created{};
    std::function<unsigned(unsigned)> copy=[&](unsigned source) {
        return one_up.clone(source,[&] {
            const auto id=++next;nodes[id]=nodes.at(source);++created;return id;
        },[&](unsigned owned) {
            const auto original=nodes.at(owned).children;
            std::vector<unsigned> children;for(const auto child:original)children.push_back(copy(child));
            nodes.at(owned).children=std::move(children);
        });
    };
    const auto private_panel=copy(1),private_label=copy(4);
    nodes.at(private_panel).name="1-UP";nodes.at(private_label).name="Three penalties";
    check(created==4 && private_panel!=1 && private_label!=4,"Clone every referenced model once, including a cycle");
    check(nodes.at(1).name=="Spot Battle" && nodes.at(4).name=="Native label" && nodes.at(5).children[0]==4,
        "Changing 1-Up leaves the original Spot Battle and Skate Jam data untouched");
    check(nodes.at(copy(2)).children[0]==private_label && nodes.at(copy(3)).children[0]==private_label &&
          nodes.at(private_label).children[0]==private_panel,"Private presenters share only private mutable identities");
    UiCloneCache<unsigned,unsigned> next_open;
    const auto reopened=next_open.clone(4,[&]{const auto id=++next;nodes[id]=nodes.at(4);return id;},[](unsigned){});
    check(reopened!=private_label && nodes.at(reopened).name=="Native label","A later menu starts from clean stock data");
    using namespace dingosdk::multiplayer::menu_data;
    const auto original_four=native_slider_choice(.25f,2,10,1);
    check(original_four==4 && native_slider_choice(native_slider_position(original_four,1,10),1,10,1)==4,
        "Enabling solo keeps the stock slider's selected player count");
    check(native_slider_choice(0.f,1,10,1)==1,"The stock Players slider can select exactly one skater");
}
void native_setup_body_reentry() {
    using namespace dingosdk::multiplayer::menu_data;
    struct Presenter { std::uint64_t blueprint;struct {std::uint64_t record,handle;} data; };
    // The captured body has two 1424-byte StackItems. Its native visibility,
    // focus and transition tracks surround the 24-byte Content presenter.
    constexpr std::size_t stride=1424,offset=24;
    const std::array presenters{Presenter{100,{0,1001}},Presenter{200,{0,1002}}};
    std::vector<std::byte> saved(stride*presenters.size(),std::byte{0x11});
    for(std::size_t i=0;i<presenters.size();++i)
        std::memcpy(saved.data()+i*stride+offset,&presenters[i],sizeof(Presenter));
    auto live=saved;
    check(live.data()!=saved.data(),"Native publication must use a separate array allocation");
    check(matching_presenters<Presenter>(live,saved,stride,offset) &&
          !remount_presenters<Presenter>(live,saved,stride,offset),
          "A typed native copy does not require another publication merely because its address changed");
    for(unsigned frame=0;frame<100;++frame) {
        live[0]=std::byte{static_cast<unsigned char>(frame)};
        live[stride+100]=std::byte{static_cast<unsigned char>(255-frame)};
        check(!remount_presenters<Presenter>(live,saved,stride,offset),
              "Native visibility and focus progression cannot restart the setup body each tick");
    }
    const auto animated=live;
    auto stock=live;
    auto native_presenter=presenters[0];native_presenter.data.handle=5001;
    std::memcpy(stock.data()+offset,&native_presenter,sizeof(Presenter));
    check(!matching_presenters<Presenter>(stock,saved,stride,offset),
          "A genuine native reentry is identified by its restored stock presenter");
    const auto reopened=remount_presenters<Presenter>(stock,saved,stride,offset);
    check(reopened && *reopened==animated && matching_presenters<Presenter>(*reopened,saved,stride,offset),
          "Reentry replaces only Content and preserves every live animation/focus byte");
    const auto published=*reopened;auto independent_copy=published;
    check(independent_copy.data()!=saved.data() && !remount_presenters<Presenter>(independent_copy,saved,stride,offset),
          "The independently allocated remount becomes stable instead of rebuilding on the next tick");
    for(unsigned field=0;field<3;++field) {
        auto changed=live;auto presenter=presenters[1];
        if(field==0)++presenter.blueprint;else if(field==1)++presenter.data.record;else ++presenter.data.handle;
        std::memcpy(changed.data()+stride+offset,&presenter,sizeof(Presenter));
        check(!matching_presenters<Presenter>(changed,saved,stride,offset),
              "Blueprint, authored record and live handle each participate in presenter identity");
    }
    std::vector<std::byte> empty;
    check(!remount_presenters<Presenter>(empty,empty,stride,offset),"Empty equivalent bodies need no publication");
    const auto expanded=remount_presenters<Presenter>(empty,saved,stride,offset);
    check(expanded && *expanded==saved,"A changed authored slot count mounts the complete retained item array");
    auto invalid=live;invalid.pop_back();bool rejected{};
    try {(void)remount_presenters<Presenter>(invalid,saved,stride,offset);}
    catch(const std::invalid_argument&) {rejected=true;}
    check(rejected,"Reject truncated native arrays before reading any presenter");
    rejected=false;
    try {(void)matching_presenters<Presenter>(live,saved,stride,stride-1);}
    catch(const std::invalid_argument&) {rejected=true;}
    check(rejected,"A presenter cannot read past its native stack item");
}
void native_setup_capability_rows() {
    using namespace dingosdk::multiplayer::menu_data;
    struct Presenter {std::uint64_t blueprint;struct {std::uint64_t record,handle;} data;};
    constexpr std::size_t stride=1424,offset=24;
    // Native Players/Duration plus private Hide Timer, Area and Join rows use
    // the same StackItem container as the actual HostParameters body.
    std::vector<std::byte> original(stride*2,std::byte{0x11});
    auto desired=original;desired.resize(stride*5,std::byte{0x22});
    for(std::size_t i=0;i<5;++i) {
        const Presenter presenter{100+i,{0,1000+i}};
        std::memcpy(desired.data()+i*stride+offset,&presenter,sizeof(presenter));
        if(i<2)std::memcpy(original.data()+i*stride+offset,&presenter,sizeof(presenter));
    }
    const auto mounted=remount_presenters<Presenter>(original,desired,stride,offset);
    check(mounted && *mounted==desired,
          "Adding native Hide Timer, Area and Join rows mounts their complete retained StackItems once");
    auto live=*mounted;
    for(unsigned frame=0;frame<100;++frame) {
        for(std::size_t row=0;row<5;++row) {
            live[row*stride]=std::byte{static_cast<unsigned char>(frame+row)};
            live[row*stride+100]=std::byte{static_cast<unsigned char>(255-frame-row)};
        }
        check(!remount_presenters<Presenter>(live,desired,stride,offset),
              "Appended native setup rows retain live visibility, focus and transition progression");
    }
    const auto animated=live;
    Presenter stock_join{100+4,{0,9000}};
    std::memcpy(live.data()+stride*4+offset,&stock_join,sizeof(stock_join));
    const auto repaired=remount_presenters<Presenter>(live,desired,stride,offset);
    check(repaired && *repaired==animated,
          "Native replacement of an extra row remounts only its presenter and preserves animated bytes");
    check(repaired && !remount_presenters<Presenter>(*repaired,desired,stride,offset),
          "A repaired capability row remains stable on the next menu tick");
}
void native_setup_action_ownership() {
    using namespace dingosdk::multiplayer::menu_data;
    constexpr std::size_t stride=784,callback_offset=200;
    std::vector<std::byte> original(stride*2,std::byte{0x11}),live=original;
    const std::uintptr_t native_confirm=100,our_confirm=200,native_back=300;
    std::memcpy(original.data()+callback_offset,&native_confirm,sizeof(native_confirm));
    std::memcpy(original.data()+stride+callback_offset,&native_back,sizeof(native_back));
    live=original;std::memcpy(live.data()+callback_offset,&our_confirm,sizeof(our_confirm));
    live[stride+10]=std::byte{0x55}; // Native Back changed after our publication.
    const std::array owned{OwnedNativeAction{0,our_confirm}};
    const auto restored=restore_owned_actions(live,original,stride,callback_offset,owned);
    check(restored && std::equal(restored->begin(),restored->begin()+stride,original.begin()),
          "Our Confirm is restored even after the native header has already been replaced");
    check(restored && (*restored)[stride+10]==std::byte{0x55},
          "Restoring Confirm preserves the independently replaced native Back action");
    auto replaced=live;const std::uintptr_t another_native_action=400;
    std::memcpy(replaced.data()+callback_offset,&another_native_action,sizeof(another_native_action));
    check(!restore_owned_actions(replaced,original,stride,callback_offset,owned),
          "A new native callback releases our ownership instead of restoring stale setup actions");
    auto extended=live;extended.resize(stride*3,std::byte{0x77});
    const auto resized=restore_owned_actions(extended,original,stride,callback_offset,owned);
    check(resized && resized->size()==extended.size() && resized->back()==std::byte{0x77},
          "Native reentry can add actions without cleanup truncating its array");
    check(!restore_owned_actions({},original,stride,callback_offset,owned),
          "An emptied native page cannot have old setup actions republished into it");
    auto truncated=live;truncated.pop_back();bool rejected{};
    try {(void)restore_owned_actions(truncated,original,stride,callback_offset,owned);}
    catch(const std::invalid_argument&) {rejected=true;}
    check(rejected,"Malformed native action rows are rejected before callback access");
}
void native_setup_owned_tiles() {
    using namespace dingosdk::multiplayer::menu_data;
    struct Presenter {std::uint64_t blueprint;struct {std::uint64_t record,handle;} data;};
    // Captured real HostParameters: OwnedItems is ContentPresenterTile[5],
    // stride 1184, Widget at +288; ListItems and ListItemHandles are empty.
    constexpr std::size_t stride=1184,offset=288;
    const std::array labels{"ID_SETTINGS_PLAYERS","Allow Join","Invite","ID_LABEL_ROUNDS","ID_LABEL_TURNTIMER"};
    std::vector<std::byte> desired;
    for(std::size_t i=0;i<labels.size();++i) {
        if(native_setup_role(labels[i])==NativeSetupRole::rounds)continue;
        const auto at=desired.size();desired.resize(at+stride,std::byte{0x22});
        const Presenter presenter{100+i,{0,1000+i}};
        std::memcpy(desired.data()+at+offset,&presenter,sizeof(presenter));
    }
    check(desired.size()==stride*4,
          "Owned HostParameters tiles retain Players, Allow Join, Invite and Timer while removing Rounds");
    check(native_setup_role("Players")==NativeSetupRole::players &&
          native_setup_role("Duration")==NativeSetupRole::timer &&
          native_setup_role("Timer")==NativeSetupRole::timer,
          "Localized native captions remain discoverable after their first adaptation tick");
    check(native_setup_role("Hide Timer")==NativeSetupRole::hide_timer &&
          native_setup_role("Rounds")==NativeSetupRole::rounds,
          "The independent Hide Timer cannot replace the overall timer template");
    const auto at=desired.size();desired.resize(at+stride,std::byte{0x33});
    const Presenter area{200,{0,2000}};
    std::memcpy(desired.data()+at+offset,&area,sizeof(area));
    std::vector<std::byte> original(desired.begin(),desired.begin()+at);
    const auto expanded=remount_presenters<Presenter>(original,desired,stride,offset);
    check(expanded && *expanded==desired,"The native OwnedItems container appends a complete private Area tile");
    auto live=*expanded;
    for(unsigned frame=0;frame<100;++frame) {
        live[0]=std::byte{static_cast<unsigned char>(frame)};
        live[stride*4+100]=std::byte{static_cast<unsigned char>(255-frame)};
        check(!remount_presenters<Presenter>(live,desired,stride,offset),
              "Owned tile focus and interaction changes cannot rebuild the native setup list");
    }
    const auto animated=live;
    auto replacement=area;replacement.data.handle=9999;
    std::memcpy(live.data()+stride*4+offset,&replacement,sizeof(replacement));
    const auto repaired=remount_presenters<Presenter>(live,desired,stride,offset);
    check(repaired && *repaired==animated,"Reentry restores only an owned tile Widget and keeps native input bytes");
}
// Model the observed native order: resize destroys trailing rows first, then
// typed assignment copies each row and releases the old destination fields.
// A plain row snapshot borrows a nested array; that array independently owns
// text, artwork references and a delegate, just like StackItem.TabData.
struct NativeArrayFixture {
    struct Row { unsigned identity{};std::uint64_t payload{}; };
    struct Payload {std::string text;std::vector<unsigned> artwork;unsigned callback{};};
    std::map<std::uint64_t,Payload> payloads;
    std::map<unsigned,unsigned> artwork_refs,callback_refs;
    std::map<std::uint64_t,std::vector<Row>> transient;
    std::vector<Row> target;
    std::uint64_t next_payload{},next_root{};
    unsigned roots_created{},roots_released{},peak_roots{};
    Row retain(Row source) {
        if(!source.payload)return source;
        const auto found=payloads.find(source.payload);
        check(found!=payloads.end(),"Typed copy read a nested array after its source row was destroyed");
        const auto payload=found->second;
        const auto owned=++next_payload;payloads.emplace(owned,payload);
        for(const auto texture:payload.artwork)++artwork_refs[texture];
        ++callback_refs[payload.callback];
        return {source.identity,owned};
    }
    void release(Row row) noexcept {
        const auto found=payloads.find(row.payload);if(found==payloads.end())return;
        for(const auto texture:found->second.artwork) {
            const auto count=artwork_refs.find(texture);
            if(--count->second==0)artwork_refs.erase(count);
        }
        const auto callback=callback_refs.find(found->second.callback);
        if(--callback->second==0)callback_refs.erase(callback);
        payloads.erase(found);
    }
    void publish(std::vector<Row>& destination,const std::vector<Row>& source) {
        while(destination.size()>source.size()) {release(destination.back());destination.pop_back();}
        destination.resize(source.size());
        for(std::size_t i=0;i<source.size();++i) {
            const auto replacement=retain(source[i]);
            release(destination[i]);destination[i]=replacement;
        }
    }
    std::uint64_t acquire() {
        const auto root=++next_root;transient.emplace(root,std::vector<Row>{});++roots_created;
        peak_roots=std::max(peak_roots,static_cast<unsigned>(transient.size()));return root;
    }
    void destroy(std::uint64_t root) noexcept {
        const auto found=transient.find(root);if(found==transient.end())return;
        for(const auto row:found->second)release(row);
        transient.erase(found);++roots_released;
    }
    void reset() {
        for(const auto row:target)release(row);target.clear();
        for(unsigned identity=1;identity<=4;++identity) {
            const auto payload=++next_payload;
            payloads.emplace(payload,Payload{"Native row "+std::to_string(identity),{100+identity,200+identity},300+identity});
            ++artwork_refs[100+identity];++artwork_refs[200+identity];++callback_refs[300+identity];
            target.push_back({identity,payload});
        }
    }
    void retained_publish(const std::vector<Row>& snapshot) {
        using dingosdk::multiplayer::menu_data::publish_retained_array;
        publish_retained_array([&]{return acquire();},
            [&](std::uint64_t root){publish(transient.at(root),snapshot);},
            [&](std::uint64_t root){publish(target,transient.at(root));},
            [&](std::uint64_t root) noexcept {destroy(root);});
    }
};
void native_array_source_retention() {
    using dingosdk::multiplayer::menu_data::publish_retained_array;
    NativeArrayFixture direct;direct.reset();
    const std::vector<NativeArrayFixture::Row> shrink{direct.target[1],direct.target[2],direct.target[3]};
    bool stale{};
    try {direct.publish(direct.target,shrink);}catch(const std::runtime_error&){stale=true;}
    check(stale,"The captured resize-before-copy order reproduces a dangling later foreign HUD row");

    NativeArrayFixture held;held.reset();
    const std::vector<NativeArrayFixture::Row> preserved{held.target[1],held.target[2],held.target[3]};
    held.retained_publish(preserved);
    check(held.target.size()==3 && held.target[0].identity==2 && held.target[1].identity==3 && held.target[2].identity==4,
          "Retaining the complete snapshot preserves foreign row order through native shrink");
    for(const auto row:held.target) {
        const auto& payload=held.payloads.at(row.payload);
        check(payload.text=="Native row "+std::to_string(row.identity) &&
              payload.artwork==std::vector<unsigned>{100+row.identity,200+row.identity} && payload.callback==300+row.identity,
              "Native retained copies preserve text, both artwork assets and the input delegate");
    }
    check(held.payloads.size()==3 && held.artwork_refs.size()==6 && held.callback_refs.size()==3 &&
          held.transient.empty() && held.roots_created==1 && held.roots_released==1,
          "Only destination owners survive publication; the transient root releases every staged nested value");

    held.reset();const auto old=held.target;
    const std::vector<NativeArrayFixture::Row> reordered{old[3],old[0],old[1],old[2]};
    held.retained_publish(reordered);
    check(held.target[0].identity==4 && held.target[1].identity==1 && held.target[2].identity==2 && held.target[3].identity==3,
          "The same retained path also protects sources overwritten earlier during a same-size reorder");
    held.retained_publish({});
    check(held.target.empty() && held.payloads.empty() && held.artwork_refs.empty() && held.callback_refs.empty(),
          "Clearing an array releases destination and transient ownership without retaining empty roots");

    NativeArrayFixture cached;cached.reset();
    const auto saved_root=cached.acquire();
    cached.publish(cached.transient.at(saved_root),cached.target);
    // Setup reentry can empty the rendered OwnedItems array on a later tick.
    // Its retained original list must own the baseline instead of caching raw
    // rows that point back to the now-destroyed live list's nested values.
    const auto baseline=cached.transient.at(saved_root);
    const auto expired=cached.target;
    cached.retained_publish({});
    bool expired_rejected{};
    try {cached.retained_publish(expired);}catch(const std::runtime_error&){expired_rejected=true;}
    check(expired_rejected && cached.transient.size()==1 && cached.payloads.size()==4,
          "Transient staging rejects an already-expired raw baseline and releases its failed temporary");
    cached.retained_publish(baseline);
    cached.destroy(saved_root);
    check(cached.target.size()==4 && cached.payloads.size()==4 && cached.transient.empty(),
          "A retained original native list supports count-change reentry after the live rows have been destroyed");
    cached.retained_publish({});
    check(cached.payloads.empty() && cached.artwork_refs.empty() && cached.callback_refs.empty() &&
          cached.roots_created==cached.roots_released && cached.peak_roots==2,
          "The original list owner and synchronous publication owner have bounded, independent lifetimes");

    for(unsigned attempt=0;attempt<100;++attempt) {
        held.reset();const auto before=held.target;const auto artwork=held.artwork_refs,callbacks=held.callback_refs;
        bool stage_failed{};
        try {
            publish_retained_array([&]{return held.acquire();},
                [&](std::uint64_t root){held.publish(held.transient.at(root),{before.front()});throw std::runtime_error("Partial typed copy failed");},
                [&](std::uint64_t){throw std::runtime_error("Commit must not run after a staging failure");},
                [&](std::uint64_t root) noexcept {held.destroy(root);});
        }catch(const std::runtime_error&){stage_failed=true;}
        check(stage_failed && held.transient.empty() && held.payloads.size()==4 &&
              held.artwork_refs==artwork && held.callback_refs==callbacks,
              "A partial staging failure releases its native root and retains the untouched destination");
        bool commit_failed{};
        try {
            publish_retained_array([&]{return held.acquire();},
                [&](std::uint64_t root){held.publish(held.transient.at(root),before);},
                [&](std::uint64_t){throw std::runtime_error("Native destination validation failed");},
                [&](std::uint64_t root) noexcept {held.destroy(root);});
        }catch(const std::runtime_error&){commit_failed=true;}
        check(commit_failed && held.transient.empty() && held.payloads.size()==4 &&
              held.artwork_refs==artwork && held.callback_refs==callbacks && held.roots_created==held.roots_released,
              "A failed destination publication releases all retained sources exactly once");
        held.retained_publish({before[1],before[2],before[3]});held.retained_publish({});
        check(held.payloads.empty() && held.transient.empty() && held.roots_created==held.roots_released && held.peak_roots==1,
              "Repeated native HUD teardown creates no growing collection of retained models");
    }
}
void native_store_and_hub_snapshot_lifetimes() {
    using dingosdk::multiplayer::menu_data::publish_retained_array;
    NativeArrayFixture store;store.reset();
    const auto original_store=store.target[2];
    const auto owner=store.acquire();
    store.publish(store.transient.at(owner),{original_store});
    store.retained_publish({store.target[0],store.target[1],store.target[3]});
    check(!store.payloads.contains(original_store.payload) && store.transient.size()==1,
          "Removing the Store destroys the old inline payload while its one typed row owner survives");
    const auto restored_store=store.transient.at(owner).front();
    auto restored=store.target;restored.insert(restored.begin()+2,restored_store);
    bool retry{};
    try {
        publish_retained_array([&]{return store.acquire();},
            [&](std::uint64_t root){store.publish(store.transient.at(root),restored);},
            [&](std::uint64_t){throw std::runtime_error("Native cleanup publication failed");},
            [&](std::uint64_t root) noexcept {store.destroy(root);});
    }catch(const std::runtime_error&){retry=true;}
    check(retry && store.transient.size()==1 && store.payloads.contains(restored_store.payload),
          "A failed cleanup keeps the retained Store available for its next detach attempt");
    store.retained_publish(restored);store.destroy(owner);
    check(store.target.size()==4 && store.target[2].identity==3 && store.transient.empty() &&
          store.payloads.at(store.target[2].payload).callback==303,
          "Cleanup restores the Store at its original position before releasing its typed row owner");
    store.retained_publish({});
    check(store.payloads.empty() && store.artwork_refs.empty() && store.callback_refs.empty() &&
          store.roots_created==store.roots_released && store.peak_roots==2,
          "The retained Store and publication temporary have bounded independent lifetimes");

    NativeArrayFixture hub;hub.reset();const auto before_hide=hub.target;
    // Native hide_in_place clears inline navigation, delegate, artwork and
    // style references. A row captured before that operation is invalid even
    // before the native list is resized; transient staging cannot revive it.
    const auto hidden_payload=++hub.next_payload;
    hub.payloads.emplace(hidden_payload,NativeArrayFixture::Payload{"",{},0});
    ++hub.callback_refs[0];hub.release(hub.target[1]);hub.target[1]={2,hidden_payload};
    bool stale{};
    try {hub.retained_publish({before_hide[0],before_hide[1],before_hide[2]});}
    catch(const std::runtime_error&){stale=true;}
    check(stale && hub.target[1].payload==hidden_payload && hub.transient.empty(),
          "Staging rejects a Hub snapshot captured before hide_in_place released its nested values");
    const std::vector<NativeArrayFixture::Row> after_hide{hub.target[0],hub.target[1],hub.target[2]};
    hub.retained_publish(after_hide);
    const auto& hidden=hub.payloads.at(hub.target[1].payload);
    check(hub.target.size()==3 && hidden.text.empty() && hidden.artwork.empty() && hidden.callback==0,
          "Recapturing Hub rows after the inline hide keeps the hidden fields empty while trimming the tail");
    hub.retained_publish({});
    check(hub.payloads.empty() && hub.artwork_refs.empty() && hub.callback_refs.empty() &&
          hub.roots_created==hub.roots_released && hub.peak_roots==1,
          "The Hub recapture and failed stale staging leave no nested values or temporary roots behind");
}
void private_flag_preview_invocation() {
    using namespace dingosdk::multiplayer;
    constexpr std::array<std::uint32_t,10> layout{80,720,1205,561,6,0,4,655391,4456459,16844800};
    check(flag_preview_graph(0xdba685be,layout) && !flag_preview_graph(0x091ea871,layout),
          "Only the captured flag creation entries accept a preview substitution");
    auto altered=layout;++altered[1];
    check(!flag_preview_graph(0xdba685be,altered),"A changed graph layout keeps the native path");
    check(flag_preview_asset_name("ECS/Activities/Throwdowns/SpotBattleThrowdownerInteractableValidator_EcsPrefab") &&
          !flag_preview_asset_name("gameplay/activity/prefabs/throwdowns/spotbattlethrowdownregistrationpoint_ecsprefab"),
          "Match the visual preview, preserving the distinct registered flag and official gameplay assets");
    std::array<std::byte,720> original{};std::uintptr_t native=101;
    std::memcpy(original.data()+88,&native,sizeof(native));
    auto slot=reinterpret_cast<std::uintptr_t>(original.data());const auto native_page=slot;
    try {
        FlagPreviewPage outer;outer.prepare(original,88,202);outer.publish(slot);
        const auto outer_page=slot;std::uintptr_t selected{};std::memcpy(&selected,reinterpret_cast<void*>(slot+88),8);
        check(selected==202 && slot!=native_page,"One invocation uses the selected custom preview");
        {
            FlagPreviewPage inner;inner.prepare(std::span(reinterpret_cast<const std::byte*>(slot),720),88,303);inner.publish(slot);
        }
        check(slot==outer_page,"A nested graph restores its caller's private constant page");
        throw 1; // Simulate an interrupted native runner; assertion failures remain uncaught here.
    } catch(int) {}
    std::uintptr_t retained{};std::memcpy(&retained,original.data()+88,8);
    check(slot==native_page && retained==101,"Even an interrupted custom graph restores the shared official page unchanged");
}
void exact_flag_bubble_owner() {
    // A native POI can outlive its removed entity, and a nearby stock flag can
    // share the donor artwork. Only the exact ECS owner may supply the model.
    struct Memory {
        std::map<std::uintptr_t,std::byte> bytes;
        void put(std::uintptr_t at,const void* value,std::size_t n) {
            const auto* source=static_cast<const std::byte*>(value);
            for(std::size_t i=0;i<n;++i)bytes[at+i]=source[i];
        }
        bool read(std::uintptr_t at,void* value,std::size_t n) const {
            auto* destination=static_cast<std::byte*>(value);
            for(std::size_t i=0;i<n;++i) {
                const auto it=bytes.find(at+i);if(it==bytes.end())return false;
                destination[i]=it->second;
            }
            return true;
        }
    } memory;
    const auto put=[&]<class T>(std::uintptr_t at,T value){memory.put(at,&value,sizeof(value));};
    const auto q=[&](std::uintptr_t at,std::uintptr_t value){put(at,value);};
    const auto d=[&](std::uintptr_t at,std::uint32_t value){put(at,value);};
    const auto w=[&](std::uintptr_t at,std::uint16_t value){put(at,value);};
    constexpr std::uintptr_t base=0x100000,context=0x3a30,block=0x7000,record=block+32;
    constexpr std::uint64_t entity=(std::uint64_t{0x100}<<32)|0x101;
    q(base+0x74cd760,0x1000);q(0x1070,0x2000);q(0x2000,0x3000);d(context+8,0x100);
    q(context+0x18,0x5000);d(0x502c,6);d(0x5024,32);d(0x5028,63);d(0x5034,64);
    q(0x5000,0x6000);q(0x5008,0x6008);q(0x6000,block);q(block-64,2);d(record,0x101);
    q(record+16,0x8000);q(0x8010,0x8100);d(0x8018,1);q(0x8100,0x9000);
    q(0x9008,0xa000);q(0xa000,0xb000);d(0xb000,0x68015bc7);w(0xb006,704);
    put(0xb008,std::array<std::uint8_t,16>{0xf6,0xa1,0x98,0x17,0x4e,0x41,0xb5,0x43,0xaf,0xde,0,0x28,0x5b,0xd7,0x92,0x7d});
    q(0xb038,0xc000);w(0xb02a,2);d(0xc000,0x9f0a28d2);w(0xc008,576);
    d(0xc018,0x7ac3f897);w(0xc020,584);
    q(context+0x9b0,0xd000);d(context+0x9b8,1);q(0xd000,0xd100);
    q(0xd100,0x9000);q(0xd108,0xe000);q(0xe000,base+0x65d0e48);q(0xe350,0x9000);
    q(0xe010,0xf000);q(0xf000,0xe010);w(0xf010,1);d(0xf018,0x101);
    q(0xf118,0x11000);q(0x11000,0x12000);q(0x12000+576,0x12340000);
    q(0x12000+584,0x56780000);d(0x12000+656,42);
    const auto find=[&](std::uint64_t state=0x10000000101ULL,std::uint64_t data=0x12340000,std::uint32_t mmid=42) {
        return dingosdk::multiplayer::flag_bubble_display(
            [&](auto at,void* out,auto n){return memory.read(at,out,n);},base,state,data,mmid).display;
    };
    check(find()==0x56780000,"The bubble comes from its exact registered MapObjectComponent");
    check(!find(entity+1),"An expired entity generation cannot brand a replacement flag");
    check(!find(entity,0x99990000) && !find(entity,0x12340000,43),
          "Foreign POI context or MMID must not redirect the bubble");
    d(0xf018,0x201);check(!find(),"A missing component key keeps stock indicators unchanged");d(0xf018,0x101);
    q(0x11000,0);check(!find(),"A retired component holder is harmless");q(0x11000,0x12000);
    w(0xc020,588);check(!find(),"A changed reflected layout fails closed");w(0xc020,584);
    w(0xf010,65);check(!find(),"Malformed component leaves are bounded");w(0xf010,1);
    q(0xd100,0x9900);q(0xd110,0xd100);check(!find(),"Cyclic component buckets cannot loop indefinitely");
}
int main() {
    try {
        activity_details_ownership();
        private_flag_preview_invocation();
        exact_flag_bubble_owner();
        reused_waiting_card_controls();
        native_activity_stack_selection();
        map_transition(); interrupted_cleanup(); partial_build_rollback(); interrupted_build_rollback(); window_close(); native_transition_without_load_request(); action_slots(); live_hud_binding(); native_hud_clock_and_penalties(); native_one_up_slider_values(); wrapped_waiting_notification(); private_throwdown_graphs(); native_setup_body_reentry(); native_setup_capability_rows(); native_setup_action_ownership(); native_setup_owned_tiles(); native_array_source_retention(); native_store_and_hub_snapshot_lifetimes();
        std::puts("Native menu lifetime: transitions, cleanup, rollback, action slots, HUD binding, setup presenter reentry, and native array source retention passed.");
        return 0;
    } catch (const std::exception& e) {
        std::fprintf(stderr, "%s\n", e.what()); return 1;
    }
}
