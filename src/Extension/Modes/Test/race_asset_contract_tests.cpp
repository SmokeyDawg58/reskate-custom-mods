#include "Extension/Modes/race_asset_contract.h"
#include "Extension/Modes/race_source_contract.h"
#include "Extension/Modes/race_group_reconciliation.h"
#include <array>
#include <cstdint>
#include <iostream>
#include <limits>
#include <map>
#include <stdexcept>
#include <string>

namespace {
using namespace dingosdk::modes::race_world;
void check(bool result, const char* message) {
    if (!result) throw std::runtime_error(message);
}
void parent_bus_checks() {
    constexpr std::uintptr_t base=0x140000000,context=0x4600d9a0,manager=0x2f4fdd00,
        first=0x2fb63960,source=0x3a682500,source2=0x3a682600,bus=0x3a783340,
        owner=0x3ac1e970,blueprint=0x3a198b290,player=0x11eb87e880,local=0x33db89b80;
    // Read-only live capture of Skate PID 34264, October 11 2026. Both CAS
    // source +0x68 slots were empty, but their native creation parent remained
    // valid and the actual skater was owned by the unique local player.
    std::map<std::uintptr_t,std::uintptr_t> pointers{
        {context+193184,0x41000000},{0x41000000,base+dingosdk::addr::engine::local_player_manager_vtable},
        {0x41000000+0x4c8,0x42000000},{0x41000000+0x4d0,0x42000008},{0x42000000,player},
        {player,base+dingosdk::addr::engine::local_player_vtable},{player+0x78,context},{player+0xb8,local},
        {local,base+dingosdk::addr::engine::skater_entity_vtable},{local+0x20,context},{local+0xf8,player},
        {base+dingosdk::addr::engine::source_managers+8,manager},
        {manager,base+dingosdk::addr::engine::source_manager_vtable},{manager+0x18,context},
        {manager+0xa0,first},{manager+0xa8,first+0x30},{first+8,source},{first+0x20,source2},
        {bus+0x20,context},{bus+8,owner},{owner+0x48,owner},{blueprint+0x18,0x43000000}};
    for(const auto s:{source,source2}) {
        pointers[s]=base+dingosdk::addr::engine::client_skater_source_vtable;
        pointers[s+0x20]=context;pointers[s+0x30]=bus;pointers[s+0x38]=s+0x1000;
        pointers[s+0x1000+0xd0]=blueprint|4;
    }
    std::map<std::uintptr_t,std::uint32_t> words{
        {base+dingosdk::addr::engine::context_player_manager_offset,193184},
        {base+dingosdk::addr::engine::context_partition_offset,0},{context,0}};
    std::map<std::uintptr_t,std::uint8_t> bytes{{player+0x45,std::uint8_t{1}}};
    std::string blueprint_name="Gameplay/Characters/CharacterBlueprint_Physics_Skater_RSP_CAS";
    const auto find=[&](std::uintptr_t b=0x140000000,std::uintptr_t c=0x4600d9a0) {
        return detail::checkpoint_parent_bus(b,c,[&](auto at){return pointers[at];},
            [&](auto at){return words[at];},[&](auto at){return bytes[at];},
            [&](auto at)->std::string_view{return at==0x43000000?blueprint_name:std::string_view{};});
    };
    check(find()==bus,"empty native source entity slot suppressed checkpoint creation");
    const auto valid_pointers=pointers;
    pointers[player+0x78]=context+8;
    check(!find(),"another-world local player accepted as checkpoint anchor");pointers=valid_pointers;
    pointers[local+0xf8]=player+8;
    check(!find(),"unbound local skater accepted as checkpoint anchor");pointers=valid_pointers;
    bytes[player+0x44]=1;check(!find(),"inactive local player accepted as checkpoint anchor");bytes[player+0x44]=0;
    pointers[bus+0x20]=context+8;
    check(!find(),"another-world source bus accepted");pointers=valid_pointers;
    pointers[owner+0x48]=0;check(!find(),"source bus without a creation owner accepted");pointers=valid_pointers;
    blueprint_name="Gameplay/Characters/OtherBlueprint";
    check(!find(),"unrelated source supplied checkpoint parent bus");
    blueprint_name="Gameplay/Characters/CharacterBlueprint_Physics_Skater_RSP_CAS";
    pointers[manager+0xa8]=first+65*0x18;
    check(!find(),"oversized source collection accepted");pointers=valid_pointers;
    pointers[source+0x68]=local+8;pointers[source2+0x68]=local+8;
    check(!find(),"another player's source supplied fallback bus");pointers=valid_pointers;
    // Prefer an exact native source-to-local-entity match over the fallback.
    constexpr auto other_bus=0x44000000;
    pointers[source2+0x30]=other_bus;pointers[source2+0x68]=local;
    pointers[other_bus+0x20]=context;pointers[other_bus+8]=owner;
    check(find()==other_bus,"verified direct local source did not beat empty-slot fallback");pointers=valid_pointers;
    pointers[manager+0x18]=context+8;
    check(!find(),"another-world source manager accepted");pointers=valid_pointers;
    check(!find(0,context) && !find(base,0),"missing world accepted for native checkpoint creation");
}
void group_reconciliation_checks() {
    std::set<unsigned> dirty{0};
    std::array<unsigned,2> parts{3,4};
    check(detail::pending_checkpoint_groups(dirty,parts)==std::set<unsigned>{0},
          "incomplete checkpoint did not request repair");
    check(dirty==std::set<unsigned>{0},"preview inspection consumed pending Destroy marks");
    auto rebuild=detail::take_checkpoint_groups(dirty,parts);
    check(rebuild==std::set<unsigned>{0} && dirty.empty(),"rebuild did not consume old marks");
    // Reproduce Destroy during initialize_placement of a replacement part.
    parts[0]=4;dirty.insert(0);--parts[0];
    check(detail::pending_checkpoint_groups(dirty,parts)==std::set<unsigned>{0},
          "Destroy during checkpoint creation was erased by reconciliation");
    rebuild=detail::take_checkpoint_groups(dirty,parts);parts[0]=4;
    check(rebuild==std::set<unsigned>{0} && detail::pending_checkpoint_groups(dirty,parts).empty(),
          "completed replacement did not converge");
    // Missing callbacks, interrupted creation and unrelated group removal.
    parts={4,2};
    check(detail::take_checkpoint_groups(dirty,parts)==std::set<unsigned>{1},
          "unchanged route hid an incomplete native checkpoint");
    parts={4,4};dirty.insert(1);
    check(detail::take_checkpoint_groups(dirty,parts)==std::set<unsigned>{1},
          "complete entity count suppressed a pending native Destroy mark");
    parts={4,2};const std::array<unsigned,2> mixed{4,2};
    check(detail::pending_checkpoint_groups(dirty,parts,mixed).empty(),
          "complete two-mesh arrow was repeatedly rebuilt as a four-mesh arch");
    parts[1]=1;check(detail::take_checkpoint_groups(dirty,parts,mixed)==std::set<unsigned>{1},
          "lost arrow layer did not request repair");

}
}
int main() {
    try {
        constexpr std::uintptr_t base = 0x140000000;
        // Read-only live registry capture, Skate PID 44644, October 10 2026.
        // Native name resolution returned these six Gate/Finish ObjectBlueprints;
        // their first two header words are the vtable and exact ClassInfo pointer.
        constexpr std::array<std::uintptr_t, 6> assets{
            0x11fdf01320,0x11fdf01d40,0x11fdf021d0,
            0x11fdf01830,0x11fdef5140,0x11fdf029a0};
        constexpr std::array<std::uintptr_t, 6> type_chain{
            0x75043a8,0x7504318,0x7504238,0x748a020,0x76de3f0,0x76de278};
        std::map<std::uintptr_t, std::uintptr_t> memory;
        for (const auto asset : assets) {
            memory[asset] = base + 0x638dfa8;
            memory[asset + 8] = base + type_chain.front();
        }
        for (unsigned i = 0; i + 1 < type_chain.size(); ++i)
            memory[base + type_chain[i] + 0x20] = base + type_chain[i + 1];
        const auto read = [&](auto at) {
            const auto found = memory.find(at);
            return found == memory.end() ? std::uintptr_t{} : found->second;
        };
        for (const auto asset : assets)
            check(detail::checkpoint_blueprint(asset,base,read), "resident checkpoint rejected");

        // The old guard used this skater-only ClassInfo. It rejected every
        // actual checkpoint despite all six names being present in domain 7.
        constexpr auto skater = 0x31000000;
        memory[skater + 8] = base + 0x7586ce8;
        memory[base + 0x7586ce8 + 0x20] = base + 0x7504318;
        check(!detail::checkpoint_blueprint(skater,base,read), "skater blueprint accepted as checkpoint");
        constexpr auto texture = 0x3aae9ce0;
        memory[texture + 8] = base + 0x7767800;
        memory[base + 0x7767800 + 0x20] = base + 0x76de3f0;
        check(!detail::checkpoint_blueprint(texture,base,read), "native texture accepted as checkpoint");
        memory[texture + 8] = base + 0x7504318;
        check(!detail::checkpoint_blueprint(texture,base,read), "generic Blueprint accepted as checkpoint");
        memory[texture + 8] = 0x41000000;
        memory[0x41000020] = 0x41000000;
        check(!detail::checkpoint_blueprint(texture,base,read), "cyclic ClassInfo walk accepted");
        check(!detail::checkpoint_blueprint(0,base,read), "null asset accepted");
        check(!detail::checkpoint_blueprint(0x12340000,base,read), "unreadable asset accepted");
        check(!detail::checkpoint_blueprint(assets[0],0,read), "missing image accepted");
        check(!detail::checkpoint_blueprint(assets[0],std::numeric_limits<std::uintptr_t>::max(),read),
            "overflowing image accepted");
        check(detail::checkpoint_resident(assets[0],assets[0],base,read), "current registry checkpoint rejected");
        check(!detail::checkpoint_resident(assets[0],0,base,read), "unloaded checkpoint retained despite readable old bytes");
        check(!detail::checkpoint_resident(assets[0],assets[1],base,read), "replaced checkpoint retained despite unchanged ClassInfo");
        check(!detail::checkpoint_resident(skater,skater,base,read), "resident skater accepted as native checkpoint");
        parent_bus_checks();
        group_reconciliation_checks();
        detail::CheckpointWorldLifetime lifetime;
        check(!lifetime.can_publish(17),"Gates created before an active world");
        lifetime.before_transition(13,0);
        check(lifetime.can_publish(17),"Active world rejected a new route");
        check(lifetime.before_transition(14,17),"Map exit did not retire gates before teardown");
        check(!lifetime.can_publish(17) && !lifetime.can_publish(18),"Gates created during map loading");
        check(!lifetime.before_transition(22,17),"Nested transition requested late duplicate cleanup");
        lifetime.before_transition(13,0);
        check(!lifetime.can_publish(17) && lifetime.can_publish(18),"Reused world context accepted stale race ownership");
        check(lifetime.before_transition(24,18),"Shutdown did not retire gates");
        lifetime.before_transition(13,0);
        check(!lifetime.can_publish(19),"Shutdown ownership resumed on a stale active transition");
        std::cout << "Deathrace native asset/source/reconciliation and world lifetime contracts passed\n";
    } catch (const std::exception& error) {
        std::cerr << error.what() << '\n';
        return 1;
    }
}
