#include "one_up_menu.h"
#include "Extension/Throwdowns/challenge_registry.h"
#include "Extension/Modes/game_modes.h"
#include "Extension/Modes/race_navigation.h"
#include "Extension/Modes/mode_lobby.h"
#include "native_menu_data.h"
#include "native_menu_lifetime.h"
#include "native_menu_ownership.h"
#include "native_setup_roles.h"
#include "native_hud_selection.h"
#include "native_hud_values.h"
#include "Extension/UI/Overlay/overlay.h"
#include "native_menu_dump.h"
#include "native_menu_internal.h"
#include "Extension/Throwdowns/one_up_runtime.h"
#include "Extension/Throwdowns/native_throwdowns.h"
#include "Extension/Throwdowns/native_flag_bubble.h"
#include "Extension/Throwdowns/one_up_placement.h"
#include "Extension/Throwdowns/throwdown_lab.h"
#include "Extension/Multiplayer/Session/session.h"
#include "Extension/Multiplayer/Hud/native_party.h"
#include "Extension/Customization/local_player_card_runtime.h"
#include "Engine/Core/Log/logging.h"
#include "Engine/Game/Build/20260929/engine.h"
#include "Engine/Game/Build/20260929/native_menu.h"
#include "Engine/Game/Build/20260929/native_party.h"
#include <Windows.h>
#include <algorithm>
#include <array>
#include <atomic>
#include <cstring>
#include <cmath>
#include <cctype>
#include <functional>
#include <map>
#include <mutex>
#include <optional>
#include <set>
#include <utility>

namespace dingosdk::multiplayer {
namespace {
using namespace menu_data;
namespace build = game::build::v20260929;
constexpr Schema tile_schema{0xdeb20e0f, 1200}, category_schema{0x628aa99c, 320},
    anchor_schema{0x0e3be640, 192}, label_schema{0xbfe23948, 104}, action_schema{0x85b7eac0, 72},
    panel_schema{0xd29bbf63,576}, header_schema{0x4efe5e19,312}, display_schema{0x1a647e6e,1040},
    panel_style_schema{0x1b6c904d,272}, setting_schema{0x94803e85,176}, selector_schema{0xf45cb4c0,1344},
    selector_item_schema{0x6d9d5106,312}, notice_schema{0x7290e514,256},
    texture_schema{0x370c9975,144}, content_tile_schema{0xcbe47f59,1184},
    page_action_schema{0x896f8a1a,784};
constexpr std::uint32_t content = 0x716496c8, items = 0x61742cb4, widget = 0x214d4984,
    rows_field = 0x67223da7, text_field = 0x4d8e01b9;
// Reflection NameHash from PlayerProfileViewModel. Graph property hashes from
// native_name_hash are a different namespace and cannot address model fields.
constexpr std::uint32_t profile_name_tag_field = 0x042ddc73;
using Action = MenuAction;
struct Descriptor { alignas(8) std::array<std::byte, 0x70> bytes{}; Address info{}; Action action; };
struct Row { Value model, anchor, tile, selector; std::vector<Handle> choices; std::string text; Address callback{}; };
struct Original { Value tile, category, description; std::array<float,2> tile_size{}, description_size{}; float icon_width{}, icon_height{}; };
struct HudPlayer { Value row, background, name, stamps, strike_anchor, strike_item; std::array<Value,3> letters; bool out{}; };
struct ResultPlayer { Value row, rank, outcome; };
struct MatchPanelPlayer { Value row,rank,outcome; bool portrait_attempted{}; };
struct MatchPanel {
    ActivityMenuTransitionGate transition;
    float camera_fov_before{};
    std::string match_key;
    Value page, panel, header, header_rows, header_titles, header_anchor, badge, notice, settings, columns, outer_anchor;
    Value quit, board, prompt, prompt_list, rewind_prompt, prompt_anchor, prompt_item;
    std::array<float,3> board_widths{};
    std::vector<std::uint64_t> player_order;
    Value mounted_slot, saved_slot;
    std::array<Value,3> setting_rows, setting_values, setting_titles, setting_icons, setting_lists, setting_anchors;
    std::map<std::uint64_t,MatchPanelPlayer> players;
    std::uint64_t requested_until{};
    std::uint64_t request_after{};
    std::uint64_t retry_after{};
    bool quitting{};
    bool menu_was_open{};
    bool prompt_visible{},rewind_visible{};
};
struct ModeCard { Value tile, category, description, logo; };
struct BrandingPresenter { Value slot, saved, model; };
struct BubbleBranding { Value display, saved; Address applied{}; };
struct MapBranding {
    Address map{};
    std::map<Handle,std::vector<BrandingPresenter>> markers;
    std::map<Handle,BubbleBranding> bubbles;
    Value panel;
    std::vector<BrandingPresenter> card;
    std::uint32_t card_mmid{};
};
struct ModeMarker { Value notice, anchor, item; };
struct RaceHudPlayer { Value list, rank, name, timer, background; unsigned place=~0U; bool crown=false; };
struct NativeSetupRows {
    Value list;
    std::vector<Ref> records;
    std::vector<Handle> handles;
    // Keep a typed native owner for the original inline rows. Raw saved bytes
    // cannot retain their nested arrays after the live list changes count.
    Value retained_rows;
    bool stack{};
    bool owned_tiles{};
    Value timer_row;
    std::map<std::string,Value> extra;
};
struct NativeSetupActions { std::size_t callback_offset{};std::vector<OwnedNativeAction> rows; };
// One bounded header owns background, icon and title. It never positions a
// fixed-size logo independently over the centered title's text.
struct ChallengeHeader {
    Value frame,row,content,title,badge,frame_anchor,frame_item,anchor,item;
};
struct ModeHud {
    ChallengeHeader header;
    HudPublicationGate publication;
    HudCountdownLifecycle countdown_lifecycle;
    HudCountdownVisibility transients;
    Value stack, title, status, score, target, current, clock, content, rows, anchor, item, clock_anchor, clock_item;
    Value countdown, countdown_anchor, countdown_item, intro, intro_badge, intro_anchor, intro_item;
    Value blind_anchor, blind_item, results, results_title, results_notice, results_rows, results_anchor, results_item;
    std::map<std::uint64_t,ResultPlayer> result_players;
    Value race_rows,race_board,race_anchor,race_item;
    Value race_title,race_columns;std::array<Value,3> race_captions;
    Value race_elapsed,race_elapsed_anchor,race_elapsed_item;
    Value race_progress,race_progress_anchor,race_progress_item;
    Value race_deadline,race_deadline_anchor,race_deadline_item,race_deadline_caption,race_deadline_caption_anchor,race_deadline_caption_item;
    std::array<float,3> race_widths{100.f,224.f,320.f};
    Value results_header,results_columns,results_badge,results_panel_anchor,results_panel_item;
    Value results_logo_anchor,results_logo_item,results_notice_anchor,results_notice_item,results_columns_anchor,results_columns_item;
    std::array<Value,3> results_captions;
    Value race_position,race_position_anchor,race_position_item,race_warning,race_warning_anchor,race_warning_item;
    bool race_warning_visible{};
    bool clock_visible{};
    std::array<Value,16> placement_prompts;
    Value placement_list,placement_anchor,placement_item,placement_size,placement_snap;
    Value editor_panel_anchor,editor_panel_item,exit_panel_anchor,exit_panel_item,exit_text,exit_list,exit_anchor,exit_item;
    bool exit_visible{},clear_ring_visible{};
    Value clear_ring_anchor,clear_ring_item;
    Value race_header_anchor,race_header_item;
    Value gate_compass,gate_compass_list,gate_compass_anchor,gate_compass_item;
    bool gate_compass_visible{};
    Value score_anchor,score_item;
    unsigned theme_mode=~0U;
    std::array<Value,11> editor_row_anchors,editor_row_items;
    std::array<Value,2> exit_actions,exit_captions;
    std::map<std::uint64_t,RaceHudPlayer> race_players;
    Value badge, heading, badge_anchor, badge_item, crown, crown_space;
    std::array<Value,5> placement_pairs;
    std::array<Value,11> placement_lines,placement_captions;
    std::vector<Handle> race_player_order;
    std::array<ModeMarker,32> markers;
    std::vector<Value> world_items;
    std::uint32_t results_game{};
    unsigned results_theme=~0U;
    std::map<std::uint64_t,Value> labels;
    std::uint32_t game{};
    std::uint64_t go_until{};
    unsigned row_count=~0U;
    bool race_layout{},placing_layout{};
};
struct HudSurfaceSlot { Value anchor,item;Widget display{}; };
struct HudSurface {
    Value stack,transition;
    std::map<std::int32_t,HudSurfaceSlot> slots;
    std::set<std::int32_t> visible;
    HudSurfaceContentGate content_gate;
    HudActivationHandoff activation;
    bool revealed{},suspended{};
    std::size_t displayed_count{};
};
struct State {
    ChallengeHeader hud_heading;
    HudPublicationGate hud_publication;
    HudStackActiveBudget hud_active_budget;
    std::map<Handle,HudSurface> hud_surfaces;
    std::map<std::string,ModeCard> mode_cards;
    std::string selected_mode, mode_status;
    ModeHud mode_hud,oneup_result_hud;
    HudResultsWindow oneup_result_window;
    MapBranding map_branding;
    MatchPanel match_panel;
    std::atomic<bool> match_panel_open{};
    std::mutex panel_input_mutex;
    MatchPanelStartInput panel_start_input;
    std::string panel_start_request;
    bool in_menu{};
    std::atomic<bool> gameplay_input_allowed{};
    bool panel_key_down{};
    unsigned mode_seconds=300, mode_capacity=6, mode_hide_seconds=30;

    // Borrowed native marker fields, restored when this match releases them.
    std::map<Handle,std::pair<Value,bool>> marker_originals;
    Address manager{}, base{}, anchor_asset{}, primary_input{};
    Value page, cards, card, category, description, details_list, original_body, original_back;
    Value picker_grid, picker_anchor, oneup_logo;
    std::array<Value,3> picker_stock, picker_stock_logos;
    Value source_label;
    Value panel_anchor, panel, header, badge, panel_style, panel_notice, footer_anchor, footer_list;
    Value footer_back, footer_confirm, duration_selector, confirm_action, original_actions;
    Value official_badge, official_race_badge;
    Value waiting_label, waiting_countdown, waiting_badge;
    Value hud_root, hud_stack, hud_notice, hud_anchor, hud_item, hud_timer, hud_timer_anchor, hud_timer_item;
    Value hud_content, hud_title, hud_title_anchor, hud_title_item, hud_badge, hud_badge_anchor, hud_badge_item;
    Value hud_target, hud_current, hud_clock, hud_clock_anchor, hud_clock_item, hud_board, hud_rows;
    Value hud_intro, hud_intro_badge, hud_intro_anchor, hud_intro_item;
    Value hud_results, hud_results_title, hud_results_notice, hud_results_rows, hud_results_anchor, hud_results_item;
    std::map<std::uint64_t,ResultPlayer> result_players;
    std::map<std::uint64_t,std::uint32_t> eliminated_at;
    std::uint64_t results_match{};
    std::uint32_t results_turn{};
    std::map<std::uint64_t,HudPlayer> hud_players;
    std::vector<std::uint64_t> hud_player_ids;
    bool hud_initialized{}, hud_countdown_visible{}, hud_intro_visible{}, hud_results_visible{};
    std::atomic<bool> hud_ready{};
    std::uint64_t hud_ended_at{}, hud_go_until{};
    one_up::Token hud_countdown_token;
    std::uint64_t waiting_diagnostic{};
    std::set<Handle> waiting_widgets;
    std::set<Handle> waiting_retired_notifications;
    std::uint64_t waiting_stock_revision{};
    std::set<Handle> waiting_backups;
    std::set<Handle> waiting_hidden_slots;
    std::map<Handle,Value> waiting_originals;
    std::map<Handle,std::pair<Value,Value>> waiting_presentations;
    std::map<Handle,Value> waiting_counts;
    std::map<Handle,std::pair<Value,Value>> official_originals;
    std::set<Handle> official_panels;
    std::map<Handle,Value> official_headers;
    std::map<Handle,Value> official_contents;
    std::map<Handle,Handle> official_slot_panels;
    std::map<Handle,Value> official_sliders;
    std::map<Handle,NativeSetupRows> official_setup_rows;
    std::map<Handle,NativeSetupActions> official_page_actions;
    std::map<Handle,Value> solo_sliders;
    std::atomic<bool> reset_setup{};
    std::uint32_t official_race{};
    std::uint32_t mode_start_game{};
    std::uint64_t mode_start_until{};
    std::string mode_setup_command,mode_setup_argument;
    std::uint64_t entry_guard_until{};
    std::array<Value,1> body_items{};
    std::vector<Handle> duration_choices;
    std::map<std::string,Address,std::less<>> assets;
    OwnedMenuModels<Value> owned;
    std::vector<Original> originals;
    std::map<std::string, Row> rows;
    std::set<std::pair<Handle,Handle>> generations;
    std::vector<std::string> displayed;
    std::array<Descriptor, 128> descriptors;
    unsigned used{}, seconds=20, max_players=4;
    std::atomic<unsigned> entry_seconds{20}, entry_players{4};
    std::mutex mutex;
    std::vector<Action> pending;
    std::atomic<std::uint64_t> owner{};
    std::atomic<bool> setup_open{};
    std::uint64_t next_id{}, next_tick{}, pass{};
    std::string original_title;
    std::string setup_status;
    std::uint64_t place_until{};
    unsigned place_seconds{}, place_players{};
    bool details{};
};
State& state() { static auto* s = new State; return *s; }
void session_marker_controls(const Context& c,bool release=false);
void mode_cards(const Context& c);
void logo_picker(const Context& c);
void mode_render(const Context& c);
void mode_hud(const Context& c);
void mode_hud_transients(const Context& c,const modes::NativeView& view,std::uint64_t now);
void mode_action(const Context& c,const Action& a);
void mode_art(const Context& c,std::span<const Value> fields,const std::string& logo);
void mode_hud_publish(const Context& c,bool show,bool countdown=false,bool intro=false,bool blind=false,bool results=false,bool race=false);
void mode_results(const Context&,const modes::NativeView&,const overlay::ModesHud&,const throwdowns::Definition&,ModeHud*);
void match_panel_notice_width(const Context&,Value,float);
void challenge_header(const Context&,ChallengeHeader&,std::string_view,const std::string&,const std::string&,Value,std::int32_t,std::int32_t);
void restore_match_panel(const Context& c);
void restore_map_branding(const Context& c);
void throwdown_map_identity(const Context& c);
bool request_menu_toggle(const Context& c);
bool discard_mode_setup() {
    const auto v=modes::native_view();
    if(!v.active)return true;
    if(!v.leading || (v.state && v.state->phase!=modes::Phase::setup && v.state->phase!=modes::Phase::results))return false;
    return !modes::command("stop",{}).starts_with("error:");
}

Value reference(const Context& c, Ref ref) { return {ref.handle, c.type_of(ref.handle)}; }
Value primary(const Context& c, Value tile) {
    return c.path(tile, {widget, 0x0fb0d794, 0xa704272a, 0xc52416ef, 0xa704272a, 0xa2b71c93});
}
Value back(const Context& c, Value page_value) {
    return c.path(page_value, {0x10810f0b, 0x0fb0d794, 0xa704272a, 0xc52416ef, 0xa704272a, 0xa2b71c93});
}
Value body(const Context& c, Value p) { return c.field(c.element(c.path(p, {content, items}), 0), content); }
std::optional<Value> mounted_body(const Context& c, Value p) {
    if(!p.handle || c.type_of(p.handle)!=p.type || !game::native_data().models.value(c.manager,p.handle,0,1))return {};
    const auto children=c.path(p,{content,items});
    const auto pointer=read<Address>(c.address(children));
    // Native page navigation empties the content array while the next view is
    // being installed. This is an ordinary transition, not a failed menu build.
    if(!pointer || !(read<std::uint32_t>(pointer-4)&0x7fffffff))return {};
    return c.field(c.element(children,0),content);
}
Value make(const Context& c, Schema schema, Address authored_type=0) {
    auto& s = state(); const auto id=0x3155500000000000ULL + ++s.next_id;
    Value value;
    if(authored_type) {
        require((kind(authored_type)==2 || kind(authored_type)==4) && size(authored_type)==schema.size && read<std::uint32_t>(read<Address>(authored_type))==schema.hash,
                "1-Up authored model schema differs.");
        value={game::native_data().models.create(c.manager,authored_type,id,game::native_name_hash("ReSkate.NativeMultiplayerMenu"),false,2),authored_type};
        c.address(value);
        c.initialize_text(value);
    } else value=c.create(schema,id);
    s.owned.track(value); return value;
}
template<std::size_t I> void activate() noexcept {
    auto& s = state();
    try {
        std::unique_lock lock(s.mutex);
        // The native waiting card can outlive its picker generation. These
        // actions are checked against its live private presenter at dispatch.
        if (I < s.used && action_generation_current(s.descriptors[I].action,s.owner.load()) && s.pending.size() < 8 &&
            std::none_of(s.pending.begin(),s.pending.end(),[&](const auto& a){return a.command==s.descriptors[I].action.command && a.argument==s.descriptors[I].action.argument;})) {
            logging::log(logging::Level::debug,logging::Channel::ui,"Throwdowns callback: {} {}.",
                s.descriptors[I].action.command,s.descriptors[I].action.argument);
            const auto now=GetTickCount64();
            if((s.descriptors[I].action.command=="open" || s.descriptors[I].action.command=="mode-open") && now<s.entry_guard_until)return;
            if(s.descriptors[I].action.command=="open") {
                // Reserve the original setup before any navigation is sent.
                // The flag bridge dispatches the verified native navigation
                // after old registrations have finished their cleanup.
                lock.unlock();
                one_up::retire_unconfirmed_native_setup();
                if(!discard_mode_setup())return;
                if(!one_up::arm_native_setup(20,s.entry_players.load()))return;
                s.selected_mode.clear();
                s.reset_setup.store(true);
                s.entry_guard_until=now+350;
                lock.lock();
            }
            const auto& command=s.descriptors[I].action.command;
            if(command=="mode-open" && s.details)return;
            if(command.starts_with("mode-") && command!="mode-open" && GetTickCount64()<s.entry_guard_until)return;
            if(command=="mode-open")s.entry_guard_until=now+350;
            s.pending.push_back(s.descriptors[I].action);
        }
    } catch (...) {}
}
template<std::size_t... I> auto functions(std::index_sequence<I...>) { return std::array{&activate<I>...}; }
Address callback(const Context& c, std::string command, std::string argument = {}) {
    auto& s = state(); std::lock_guard lock(s.mutex);
    const auto slot=action_slot(s.used,s.descriptors.size(),[&](std::size_t i)->const Action& {return s.descriptors[i].action;},command,argument,s.pass);
    require(slot.has_value(), "1-Up native action capacity reached.");
    static const auto f = functions(std::make_index_sequence<128>{});
    auto& d = s.descriptors[*slot];
    if(*slot==s.used) {
        const auto prototype = c.base + build::native_menu::action_prototype;
        require(read<Address>(prototype + 0x40) == c.base + build::native_menu::action_invoker && read<std::uint8_t>(prototype + 0x68) == 0, "1-Up action ABI differs.");
        require(memory::peek_bytes(prototype, d.bytes.data(), d.bytes.size()), "1-Up action prototype unavailable.");
        const auto invoker = reinterpret_cast<Address>(f[*slot]);
        for (const auto offset : {0x30,0x40,0x48}) std::memcpy(d.bytes.data()+offset, &invoker, 8);
        d.info = reinterpret_cast<Address>(d.bytes.data()); ++s.used;
    }
    // A waiting card may be mounted after its picker has retired (owner=0).
    // Its manager plus the exact presentation check own these delegates.
    const auto generation=waiting_card_action(command)?c.manager:s.owner.load();
    d.action = {generation,std::move(command),std::move(argument),s.pass};
    return reinterpret_cast<Address>(&d.info);
}
Address asset(const Context& c, const char* name) {
    auto& s=state();
    if(const auto cached=s.assets.find(name);cached!=s.assets.end())return cached->second;
    std::vector<Address> anchors;
    std::set<unsigned> domains;
    const auto add_anchor=[&](Address anchor) {
        if(anchor && domains.insert(read<std::uint16_t>(anchor+0x16)).second)anchors.push_back(anchor);
    };
    // CategoryTile_Widget is a shared foundation asset. Its parent domain
    // cannot find the Throwdown resources in the child domain. The retained
    // badge is owned by that same child bundle, even after its menu retires.
    const auto add_icon=[&](Value model,std::uint32_t field) {
        if(!model.handle || c.type_of(model.handle)!=model.type)return;
        const auto icon=read<Address>(c.address(c.field(model,field)));
        if(!icon)return;
        const auto type=read<Address>(icon+8);
        if(type==c.base+build::native_menu::texture_asset_type || type==c.base+build::native_menu::image_asset_type)
            add_anchor(icon);
    };
    add_icon(s.waiting_badge,0x6e469d2a);
    add_icon(s.category,0x189084da);
    add_anchor(s.anchor_asset);
    // Gameplay and menu presenters can own additional native widget domains.
    for(const auto& root:c.roots({presenter.hash})) {
        const auto blueprint=read<Widget>(root.value).blueprint;
        if(!blueprint || read<Address>(blueprint+8)!=c.base+build::native_menu::widget_blueprint_type)continue;
        add_anchor(blueprint);
        if(domains.size()>=32)break;
    }
    const auto find_loaded=[&](const char* path) {
        for(const auto anchor:anchors)if(const auto found=named_asset(c,anchor,path))return found;
        // Shared widgets may belong to a sibling/resident UI domain after a
        // custom-map switch. Only the engine's loaded-domain lookup is used;
        // no stale asset pointer or unloaded archive entry is retained.
        const auto find=game::native_data().find_asset;
        if(find)for(unsigned candidate=0;candidate<0xbbf;++candidate) {
            if(!read<Address>(c.base+build::engine::domain_owners+candidate*8ULL))continue;
            if(const auto found=find(static_cast<std::uint16_t>(candidate),path))return found;
        }
        return Address{};
    };
    if(const auto found=find_loaded(name)) {s.assets.emplace(name,found);return found;}
    // Exported dynamic-model records are not indexed as ordinary assets.
    // Resolve their actual resource list, then select the exact exported name.
    constexpr std::array containers{
        std::pair{"Activities_Shared_ContentResource/","UI/Features/Activities/Resources/Activities_Shared_ContentResource"},
        std::pair{"Activities_CelebrationMenus_ContentResource/","UI/Features/Activities/Resources/Activities_CelebrationMenus_ContentResource"},
        std::pair{"UI_Layer_Data/","UI/Foundations/Layers/UI_Layer_Data"},
        std::pair{"Activities_DetailsMenu_ContentResource/","UI/Features/Activities/Resources/Activities_DetailsMenu_ContentResource"},
        std::pair{"Throwdown_DetailsMenu_ContentResource/","UI/Features/Activities/Resources/Throwdown_DetailsMenu_ContentResource"},
        std::pair{"Activities_DetailsMenu_ContentResource_Challenge/","UI/Features/Activities/Resources/Activities_DetailsMenu_ContentResource_Challenge"},
        std::pair{"StackPanelStylesList/","UI/Foundations/Components/StackVariations/StackPanel/StackPanelStylesList"},
        std::pair{"TD_HostParameters_ContentResource/","UI/Features/Throwdowns/ContentResources/TD_HostParameters_ContentResource"},
        std::pair{"TD_Page_ContentResources/","UI/Features/Throwdowns/TD_Page_ContentResources"},
        std::pair{"TD_Hud_ContentResources/","UI/Features/Throwdowns/TD_Hud_ContentResources"},
        std::pair{"TD_Scoreboard_ContentResources/","UI/Features/Throwdowns/TD_Scoreboard_ContentResources"},
        std::pair{"Activities_HudElementData_ContentResource/","UI/Features/Activities/Resources/Activities_HudElementData_ContentResource"},
        std::pair{"ActivityBannerStylesList/","UI/Features/Activities/Models/ActivityBannerStylesList"},
        std::pair{"SettingWrapperStylesList/","UI/Foundations/Components/Settings/Widgets/SettingWrapperStylesList"},
        std::pair{"ColorPaletteStylesList/","UI/Foundations/Styles/Lists/ColorPaletteStylesList"},
        std::pair{"TextStylesList/","UI/Foundations/Styles/Lists/TextStylesList"},
        std::pair{"InteractivityColorStylesList/","UI/Foundations/Styles/Lists/InteractivityColorStylesList"},
        std::pair{"NoticeStylesList/","UI/Foundations/Components/Notices/NoticeStylesList"},
        std::pair{"ButtonStyleList/","UI/Foundations/Styles/Lists/ButtonStyleList"},
        std::pair{"","UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources"}};
    for(const auto& [prefix,path]:containers) if(std::string_view(name).starts_with(prefix)) {
        const Address list=find_loaded(path);
        if(!list || read<Address>(list+8)!=c.base+build::native_menu::asset_list_type)continue;
        const auto records=read<Address>(list+0x20);
        const auto count=records?read<std::uint32_t>(records-4)&0x7fffffff:0;
        require(count<=2048,"1-Up native resource list exceeds its limit.");
        for(unsigned i=0;i<count;++i) {
            const auto record=read<Address>(records+i*8ULL);
            if(!record || read<Address>(record+8)!=c.base+build::native_menu::asset_record_type)continue;
            const auto exported_name=string(read<Address>(record+0x28),256);
            s.assets.emplace(exported_name,record);
        }
        if(const auto found=s.assets.find(name);found!=s.assets.end())return found->second;
    }
    // Shared foundation widgets can live outside the Throwdowns asset domain.
    // Reuse the native Multiplayer page's discovered, retained blueprints.
    for (unsigned slot=0; slot<menu_view::page_count; ++slot) {
        const auto& page_state = native_menu_detail::page_state(slot);
        if (page_state.base != c.base || page_state.manager != c.manager) continue;
        if (const auto found=page_state.assets.find(name); found!=page_state.assets.end() &&
            (read<Address>(found->second+8)==c.base+build::native_menu::widget_blueprint_type ||
             read<Address>(found->second+8)==c.base+build::native_menu::asset_record_type)) return found->second;
    }
    // Throwdowns can open before the separate ReSkate pause page has loaded.
    // Use the same bounded, validated foundation discovery directly instead
    // of depending on that unrelated page having populated its cache first.
    for(const auto& root:c.roots({core.hash})) {
        try {
            const auto found=blueprints(c,root.model);
            s.assets.insert(found.begin(),found.end());
            if(const auto loaded=s.assets.find(name);loaded!=s.assets.end())return loaded->second;
        } catch(const std::exception&) { }
    }
    throw std::runtime_error(std::string("1-Up foundation widget unavailable: ")+name);
}
Value category(const Context& c, Value tile) {
    const auto w = read<Widget>(c.address(c.path(tile, {widget, widget})));
    const auto model = reference(c,w.data); require(model.type && size(model.type)==category_schema.size && read<std::uint32_t>(read<Address>(model.type))==category_schema.hash, "1-Up category schema differs."); return model;
}
Address native_record_type(const Context& c,Address record) {
    // A DataRef may contain a native record or an already materialized handle.
    // Never treat an arbitrary asset's name/fields as record TypeInfo: the game
    // model factory assumes a valid TypeInfo pointer and cannot reject it safely.
    require(record && read<Address>(record+8)==c.base+build::native_menu::asset_record_type,
            "Private Throwdown UI reference is not a native data-model record.");
    const auto type=read<Address>(record+0x18);
    require(type && read<Address>(record+0x20),"Private Throwdown UI record has no type or data.");
    return type;
}
Value record_model(const Context& c, const char* name, Schema schema) {
    const auto record=asset(c,name);
    const auto type=native_record_type(c,record),data=read<Address>(record+0x20);
    // A loaded authored record already carries validated native TypeInfo.
    // Its type need not have a live model yet. The native factory registers it.
    auto result=make(c,schema,type); c.copy(result,data); return result;
}
Value record_model(const Context& c,const char* name) {
    const auto record=asset(c,name),type=native_record_type(c,record);
    require(kind(type)==2,"1-Up HUD template schema differs.");
    return record_model(c,name,{read<std::uint32_t>(read<Address>(type)),static_cast<std::uint16_t>(size(type))});
}
Value clone_widget_model(const Context& c,Value target) {
    auto w=read<Widget>(c.address(target));
    const auto type=w.data.handle?c.type_of(w.data.handle):(w.data.record?native_record_type(c,w.data.record):0);
    require(type && kind(type)==2,"Native HUD widget model unavailable.");
    const auto data=w.data.handle?c.address({w.data.handle,type}):read<Address>(w.data.record+0x20);
    const auto result=make(c,{read<std::uint32_t>(read<Address>(type)),static_cast<std::uint16_t>(size(type))},type);
    c.copy(result,data); w.data={0,result.handle}; c.set(target,w); return result;
}
// A typed native copy retains DataRefs; it does not clone the models they
// reference. Copy every mutable UI edge before mounting a private menu. Asset
// styles, expression contexts and backend bindings remain borrowed read-only.
struct PrivateUiGraph {
    const Context& c;
    UiCloneCache<std::pair<Handle,Address>,Value> copies;
    std::set<Handle> private_roots, walked;
    unsigned budget=2400;
    Value clone(Ref source,unsigned depth=0) {
        const auto type=source.handle?c.type_of(source.handle):(source.record?native_record_type(c,source.record):0);
        require(type && (kind(type)==2 || kind(type)==4),"Private Throwdown UI model is unavailable.");
        if(source.handle && (private_roots.contains(source.handle) || walked.contains(source.handle)))return {source.handle,type};
        const auto key=std::pair{source.handle,source.handle?type:source.record};
        return copies.clone(key,[&] {
            require(depth<96 && budget,"Private Throwdown UI graph exceeds its limit.");--budget;
            const auto data=source.handle?c.address({source.handle,type}):read<Address>(source.record+0x20);
            const auto result=make(c,{read<std::uint32_t>(read<Address>(type)),static_cast<std::uint16_t>(size(type))},type);
            c.copy(result,data);private_roots.insert(result.handle);return result;
        },[&](Value result){walk(result,depth+1);});
    }
    void walk(Value value,unsigned depth) {
        if(!walked.insert(value.handle).second)return;
        require(depth<96,"Private Throwdown UI graph depth differs.");
        const auto hash=read<std::uint32_t>(read<Address>(value.type));
        if(hash==presenter.hash) {
            auto w=read<Widget>(c.address(value));
            if(w.data.handle || w.data.record) {w.data={0,clone(w.data,depth).handle};c.set(value,w);}
            return;
        }
        if(hash==0x62088281) {
            const auto ref=read<Ref>(c.address(value));
            if(ref.handle || ref.record) {
                const auto type=ref.handle?c.type_of(ref.handle):native_record_type(c,ref.record);
                // Scalar backend expressions are borrowed read-only. Mutable
                // widgets/lists are private, including array-valued models.
                if(type && (kind(type)==2 || kind(type)==4))c.set(value,Ref{0,clone(ref,depth).handle});
            }
            return;
        }
        if(kind(value.type)==4) {
            unsigned count{},stride{};auto bytes=c.array(value,128,count,stride);
            for(unsigned i=0;i<count;++i) {
                const auto child=c.element(value,i);
                if(kind(child.type)==2 || kind(child.type)==4) {
                    // Publishing an inline array element can retire its
                    // siblings' storage. Adapt detached roots, then replace
                    // the counted array once after every element is ready.
                    const auto copy=clone({0,child.handle},depth+1);
                    require(memory::peek_bytes(c.address(copy),bytes.data()+i*stride,stride),"Private UI array copy is unavailable.");
                }
            }
            c.array(value,bytes,count);
        } else if(kind(value.type)==2) {
            const auto meta=read<Address>(value.type),fields=read<Address>(meta+0x60);
            const auto count=read<std::uint16_t>(meta+0x2a);
            require(count<=128,"Private Throwdown field count differs.");
            for(unsigned i=0;i<count;++i) {
                const auto key=read<std::uint32_t>(fields+i*24);
                if(key==0x8cc042ef || key==0xa1e84eb1 || key==0x25189231 || key==0xf94d8cc6 ||
                   key==0x8a3e2159 || key==0x54c9abee || key==0xd78d240b || key==0x85b8c3e6 || key==655725074U)continue;
                const auto child=c.field(value,key);
                if(key==0xf2c90867 && kind(child.type)==4) {
                    unsigned n{},stride{};const auto bytes=c.array(child,128,n,stride);
                    require(stride==sizeof(Handle),"Private UI handle array differs.");
                    std::vector<Handle> handles(n);if(n)std::memcpy(handles.data(),bytes.data(),bytes.size());
                    for(auto& handle:handles)if(handle)handle=clone({0,handle},depth+1).handle;
                    c.array(child,handles);
                } else if(kind(child.type)==2 || kind(child.type)==4)walk(child,depth+1);
            }
        }
    }
};
Value clone_widget_tree(const Context& c,Value slot) {
    auto w=read<Widget>(c.address(slot));
    PrivateUiGraph graph{c};const auto result=graph.clone(w.data);
    w.data={0,result.handle};c.set(slot,w);return result;
}
std::vector<Value> ui_models(const Context& c,Value root,std::uint32_t wanted) {
    std::vector<Value> result;std::set<Handle> seen;unsigned budget=2400;
    std::function<void(Value,unsigned)> visit=[&](Value value,unsigned depth) {
        if(!value.handle || !value.type || !budget || depth>96 || !seen.insert(value.handle).second)return;
        --budget;const auto hash=read<std::uint32_t>(read<Address>(value.type));
        if(hash==wanted) {result.push_back(value);return;}
        if(hash==0x62088281) {
            const auto ref=read<Ref>(c.address(value));if(ref.handle)visit(reference(c,ref),depth+1);return;
        }
        if(kind(value.type)==4) {
            unsigned count{},stride{};c.array(value,128,count,stride);
            for(unsigned i=0;i<count;++i)visit(c.element(value,i),depth+1);
        } else if(kind(value.type)==2) {
            const auto meta=read<Address>(value.type),fields=read<Address>(meta+0x60);
            const auto count=read<std::uint16_t>(meta+0x2a);require(count<=128,"Throwdown UI field count differs.");
            for(unsigned i=0;i<count;++i) {
                const auto key=read<std::uint32_t>(fields+i*24);
                if(key==0x8cc042ef || key==0xa1e84eb1 || key==0x25189231 || key==0xf94d8cc6 ||
                   key==0x8a3e2159 || key==0x54c9abee || key==0xd78d240b || key==0x85b8c3e6 || key==655725074U)continue;
                const auto child=c.field(value,key);
                if(key==0xf2c90867 && kind(child.type)==4) {
                    unsigned n{},stride{};const auto bytes=c.array(child,128,n,stride);require(stride==sizeof(Handle),"UI handle list differs.");
                    for(unsigned j=0;j<n;++j) {
                        Handle handle{};std::memcpy(&handle,bytes.data()+j*stride,stride);
                        if(handle)visit({handle,c.type_of(handle)},depth+1);
                    }
                } else if(kind(child.type)==2 || kind(child.type)==4)visit(child,depth+1);
            }
        }
    };
    visit(root,0);return result;
}
void hud_list(const Context& c,Value& list,const std::vector<Widget>& widgets,bool horizontal,float item_size,float spacing,float last_height=0.f,float width=850.f,float first_width=0.f,bool fit_first=false) {
    if(!list.handle)list=make(c,linear_list);
    // Stock StackComponent always binds a one-item focus window; its blueprint
    // does not connect MaxFocusedItemCount to the list element. A native linear
    // list presents every noninteractive anchored HUD item, without that owner.
    c.set(c.field(list,0x55ca89f4),asset(c,"UI/Foundations/Components/Lists/Shared/AnchoredContentPresenterListItem_Widget"));
    c.set(c.field(list,0x4cb61cac),horizontal?0:1);
    c.set(c.field(list,0x64b9a9e8),0); // Preserve the presenters' authored measures.
    c.set(c.field(list,0x19ff199a),item_size);
    c.set(c.field(list,0xebca7354),spacing);
    c.set(c.field(list,0x4554761c),false); // No clipping of the last, variable-height leaderboard.
    c.set(c.field(list,0xafe8434b),false); // No virtualization of penalty rows.
    c.set(c.field(list,0x8bbf7d67),true);
    c.array(c.field(list,rows_field),std::vector<Ref>{});
    std::vector<Handle> handles;
    for(unsigned i=0;i<widgets.size();++i) {
        const auto anchor=make(c,anchor_schema);
        c.set(c.field(anchor,widget),widgets[i]);
        c.set(c.field(anchor,0x144aee01),false);
        // The engine measures the name itself, so penalty stamps follow the
        // actual lettering rather than an empty fixed-width name column.
        c.set(c.field(anchor,0x6efc1a61),horizontal && i==0 && fit_first);
        const float height=(!horizontal && i+1==widgets.size() && last_height>0)?last_height:item_size;
        c.set(c.field(anchor,0x1cff7243),std::array<float,2>{horizontal?(i==0 && first_width>0?first_width:item_size):width,height});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const auto layout=c.field(anchor,axis);
            c.set(c.field(layout,0x99bbed5a),0.f);c.set(c.field(layout,0x49943bed),0.f);
            c.set(c.field(layout,0xfec5e0b1),0.f);c.set(c.field(layout,0xd01f4a21),1.f);
        }
        handles.push_back(anchor.handle);
    }
    c.array(c.field(list,0xf2c90867),handles);
}
void native_timer_seconds(const Context& c,Value timer,std::uint32_t field,std::uint64_t milliseconds) {
    const auto value=c.field(timer,field);
    require(kind(value.type)==17,"Native countdown time is not Uint64.");
    const auto seconds=native_hud_seconds(milliseconds);
    // A new value starts the native number transition. Publish once per
    // displayed second rather than restarting its animation on every tick.
    if(read<std::uint64_t>(c.address(value))!=seconds)c.set(value,seconds);
}
bool hud_list_has_items(const Context& c,Value list,unsigned expected) {
    if(!list.handle)return false;
    unsigned count{},stride{};
    c.array(c.field(list,0xf2c90867),expected+1,count,stride);
    return count==expected;
}
Ref record_ref(const Context& c,const char* name) {
    const auto record=asset(c,name);native_record_type(c,record);return {record,0};
}
void clear_value(const Context& c,Value value) {
    if(kind(value.type)==7) {if(!read<Address>(c.address(value)) || !c.text(value).empty())c.text(value,"");return;}
    if(kind(value.type)==4) {
        // Native array publication reads the count preceding the pointer even
        // for an empty array. A null pointer is not an empty counted array.
        c.array(value,std::span<const std::byte>{},0); return;
    }
    if(kind(value.type)==2) {
        // Bindings can contain counted arrays and owned delegates. Reset from
        // a native-constructed value instead of fabricating zeroed storage.
        const auto defaults=make(c,{read<std::uint32_t>(read<Address>(value.type)),static_cast<std::uint16_t>(size(value.type))},value.type);
        c.copy(value,c.address(defaults)); return;
    }
    const std::vector<std::byte> empty(size(value.type));
    std::vector<std::byte> current(empty.size());
    require(memory::peek_bytes(c.address(value),current.data(),current.size()),"Native reset value is unavailable.");
    if(current!=empty)c.publish(value,empty.data());
}
constexpr std::int32_t hud_score_id=0x31555071, hud_countdown_id=0x31555072, hud_intro_id=0x31555073;
constexpr std::int32_t hud_results_id=0x31555074;
constexpr std::int32_t hud_clock_id=0x31555075;
constexpr std::int32_t hud_panel_id=0x31555076;
constexpr std::int32_t hud_title_id=0x31555077, hud_badge_id=0x31555078;
constexpr std::int32_t hud_strike_id=0x31555800;
void clear_hud_state(bool preserve_match_input=false) {
    auto& s=state();s.hud_ready.store(false);s.hud_ended_at=0;s.hud_go_until=0;s.hud_countdown_token={};
    s.hud_root={};s.hud_stack={};s.hud_notice={};s.hud_anchor={};s.hud_item={};
    s.hud_timer={};s.hud_timer_anchor={};s.hud_timer_item={};
    s.hud_intro={};s.hud_intro_badge={};s.hud_intro_anchor={};s.hud_intro_item={};
    s.hud_content={};s.hud_title={};s.hud_target={};s.hud_current={};s.hud_clock={};s.hud_clock_anchor={};s.hud_clock_item={};
    s.hud_title_anchor={};s.hud_title_item={};s.hud_badge={};s.hud_badge_anchor={};s.hud_badge_item={};s.hud_heading={};
    s.hud_board={};s.hud_rows={};s.hud_players.clear();s.hud_player_ids.clear();
    s.hud_results={};s.hud_results_title={};s.hud_results_notice={};s.hud_results_rows={};
    s.hud_results_anchor={};s.hud_results_item={};s.result_players.clear();s.eliminated_at.clear();s.results_match=0;s.results_turn=0;
    s.hud_initialized=false;s.hud_countdown_visible=false;s.hud_intro_visible=false;s.hud_results_visible=false;
    s.match_panel={};s.match_panel_open.store(false);s.oneup_result_hud={};s.oneup_result_window={};
    {std::lock_guard input_lock(s.panel_input_mutex);reset_match_panel_input(s.panel_start_input,s.panel_start_request,preserve_match_input);}
    // Waiting notifications outlive score/countdown HUD rebuilds. Keep their
    // hidden ownership until queue teardown or a genuine native creation/join.
}
template<class PublishRows> void publish_gameplay_hud_rows(const Context& c,Value stack,std::span<const std::byte> rows,unsigned stride,unsigned id_offset,PublishRows&& publish_rows) {
    unsigned owned{};
    for(std::size_t at=0;at<rows.size();at+=stride) {
        std::int32_t id{};std::memcpy(&id,rows.data()+at+id_offset,sizeof(id));
        owned+=custom_hud_item(id);
    }
    const auto limit=c.field(stack,0x6a93bc8a); // MaxFocusedItemCount / MaxActiveItemCount
    const auto live=read<int>(c.address(limit));
    state().hud_active_budget.publish(stack.handle,live,owned,[&](int next) {
        c.set_if_changed(limit,next);
        logging::log(logging::Level::debug,logging::Channel::ui,
            "Custom HUD active budget: stack={:#x}, owned={}, limit={} -> {}.",stack.handle,owned,live,next);
    },std::forward<PublishRows>(publish_rows));
}
void restore_hud_active_budgets(const Context& c) {
    auto& budget=state().hud_active_budget;
    for(const auto handle:budget.stacks()) {
        const auto type=c.type_of(handle);
        if(!type || type!=c.type({0xd71ee0a1,80}))continue;
        const auto limit=c.field(Value{handle,type},0x6a93bc8a);
        budget.update(handle,read<int>(c.address(limit)),0,[&](int next){c.set_if_changed(limit,next);});
    }
    budget.manager_replaced();
}
Value opaque_hud_transition(const Context& c,Value stack,HudSurface& surface) {
    if(surface.transition.handle)return surface.transition;
    const auto checkpoint=state().owned.checkpoint();
    try {
        // The captured default fades 0->1 and scales 1.5->1. These are menu
        // entrance effects, inappropriate for persistent gameplay overlays.
        // Clone every mutable curve edge; never edit shared native styles.
        PrivateUiGraph graph{c};
        const auto style=graph.clone(read<Ref>(c.address(c.field(stack,0x54c9abee))));
        require(read<std::uint32_t>(read<Address>(style.type))==0x69df7d5a && size(style.type)==64,
                "Native HUD transition style differs.");
        const auto curve=[&](std::uint32_t key,std::uint32_t hash) {
            const auto value=reference(c,read<Ref>(c.address(c.field(style,key))));
            require(value.handle && read<std::uint32_t>(read<Address>(value.type))==hash,
                    "Native HUD transition curve differs.");
            return c.field(value,0x600f43a4); // Keyframes, with original input times retained.
        };
        const auto alpha=curve(0x36120f6c,0x6beddefb);
        unsigned count{},stride{};auto points=c.array(alpha,128,count,stride);
        require(count>=2 && stride==8,"Native HUD opacity keyframes differ.");
        const auto alpha_type=c.element(alpha,0).type;
        points=constant_hud_curve<float>(points,stride,c.member(alpha_type,0x2358a817).offset,1.f);
        c.array(alpha,points,count);
        const auto transform=curve(0x5f61032a,0xa5a8bd1f);
        points=c.array(transform,128,count,stride);
        require(count>=2 && stride==80,"Native HUD transform keyframes differ.");
        const auto transform_type=c.element(transform,0).type;
        const std::array<float,16> identity{1,0,0,0, 0,1,0,0, 0,0,1,0, 0,0,0,0};
        points=constant_hud_curve(points,stride,c.member(transform_type,0x2358a817).offset,identity);
        c.array(transform,points,count);
        surface.transition=style;
        logging::log(logging::Level::info,logging::Channel::ui,
            "Custom HUD private transition: stack={:#x}, opacity=1, transform=identity; stock styles unchanged.",stack.handle);
        return style;
    } catch(...) {
        state().owned.rollback(checkpoint,[&](Value value){c.destroy(value);});
        throw;
    }
}
HudSurfaceSlot& hud_surface_slot(const Context& c,Value stack,std::int32_t id) {
    auto& surface=state().hud_surfaces[stack.handle];surface.stack=stack;
    auto& slot=surface.slots[id];
    if(slot.anchor.handle && slot.item.handle)return slot;
    const auto transition=opaque_hud_transition(c,stack,surface);
    slot.anchor=make(c,anchor_schema);slot.item=make(c,stack_item);
    c.set(c.field(slot.anchor,widget),Widget{});
    c.set(c.field(slot.item,0x1e95752c),id);
    c.set(c.field(slot.item,0xd78d240b),Ref{0,transition.handle});
    c.set(c.field(slot.item,content),Widget{asset(c,"UI/Foundations/Templates/Layouts/Anchored/AnchoredContentPresenter_Widget"),{0,slot.anchor.handle}});
    c.set(c.field(slot.item,0x369babfe),true);
    for(const auto track:{0x8c0583eeU,0xc5c7cc11U}) {
        c.set(c.path(slot.item,{track,0x20247753}),1.f);c.set(c.path(slot.item,{track,0x2426103f}),1.f);
        c.set(c.path(slot.item,{track,0x1e3ce013}),true);
    }
    return slot;
}
void hide_hud_surface_content(const Context& c,HudSurface& surface) {
    surface.content_gate.reset();surface.revealed=false;
    for(auto& [id,slot]:surface.slots)if(slot.anchor.handle && c.type_of(slot.anchor.handle)==slot.anchor.type) {
        const auto value=c.field(slot.anchor,widget);
        const auto display=read<Widget>(c.address(value));
        if(display.blueprint)slot.display=display;
        c.set_if_changed(value,Widget{});
    }
}
void suspend_hud_surface_content(const Context& c) {
    for(auto& [handle,surface]:state().hud_surfaces) {
        surface.suspended=true;hide_hud_surface_content(c,surface);
    }
}
void publish_hud_surface(const Context& c,Value stack,std::span<const Value> visible,bool mode) {
    auto& s=state();auto found=s.hud_surfaces.find(stack.handle);
    if(found==s.hud_surfaces.end() && visible.empty())return;
    auto& surface=s.hud_surfaces[stack.handle];surface.stack=stack;
    std::erase_if(surface.visible,[&](int id){return (id>=0x4d4f4400 && id<0x4d4f4440)==mode;});
    for(const auto item:visible)if(item.handle)surface.visible.insert(read<int>(c.address(c.field(item,0x1e95752c))));
    const auto target=c.field(stack,items);unsigned count{},stride{};
    const auto old=c.array(target,custom_hud_stack_item_limit,count,stride);
    require(stride==stack_item.size,"Native stable HUD stack ABI differs.");
    const auto id_offset=c.member(c.type(stack_item),0x1e95752c).offset;
    const auto presenter_offset=c.member(c.type(stack_item),content).offset;
    std::vector<std::byte> next;
    for(unsigned i=0;i<count;++i) {
        std::int32_t id{};std::memcpy(&id,old.data()+i*stride+id_offset,4);
        if(!custom_hud_item(id))next.insert(next.end(),old.begin()+i*stride,old.begin()+(i+1)*stride);
    }
    if(!surface.visible.empty())for(const auto id:custom_hud_slot_order) {
        auto& slot=hud_surface_slot(c,stack,id);
        // Dynamic artwork (rank color, badge, results panel) is still authored
        // through the same private anchor. Remember it before blanking a slot.
        const auto display=read<Widget>(c.address(c.field(slot.anchor,widget)));
        if(display.blueprint)slot.display=display;
        const auto at=next.size();next.resize(at+stride);
        require(memory::peek_bytes(c.address(slot.item),next.data()+at,stride),"Native stable HUD source unavailable.");
        next[at+c.member(c.type(stack_item),0x369babfe).offset]=std::byte{1};
    }
    const auto stable=reconcile_hud_items<Widget>(old,next,stride,id_offset,presenter_offset);
    if(stable)hide_hud_surface_content(c,surface);
    publish_gameplay_hud_rows(c,stack,next,stride,id_offset,[&] {
        if(stable) {
            c.array(target,*stable,static_cast<unsigned>(stable->size()/stride));
            surface.activation.remounted(GetTickCount64());
            logging::log(logging::Level::info,logging::Channel::ui,
                "Stable HUD presenters mounted: stack={:#x}, native_items={}, custom_active_items={}; content waits for native activation.",
                stack.handle,next.size()/stride-(surface.visible.empty()?0:custom_hud_slot_order.size()),
                surface.visible.empty()?0:custom_hud_slot_order.size());
        }
    });
    if(surface.visible.empty()) {hide_hud_surface_content(c,surface);return;}
    if(surface.suspended) {
        // A real native menu retires/rebinds item contexts even though its
        // retained presenter identities stay the same. Bootstrap that return
        // once; ordinary content changes never set suspended.
        surface.suspended=false;surface.activation.remounted(GetTickCount64());
    }
    // Context construction follows the initial list notification. Once it
    // exists, one same-identity bootstrap refresh supplies the full active
    // state before any content is revealed. New HUD elements never do this.
    const bool handoff=surface.activation.due(GetTickCount64());
    if(handoff) {
        c.array(target,next,static_cast<unsigned>(next.size()/stride));
        surface.activation.published();surface.content_gate.reset();
        logging::log(logging::Level::info,logging::Channel::ui,
            "Stable HUD activation handoff: stack={:#x}; presenter count/IDs unchanged, content still blank.",stack.handle);
    }
    // Native content creation can temporarily reset IsActive/interpolation for
    // every initialized sibling. The private curves stay opaque/identity, so
    // copying live inline items or blanking their content would cause the
    // very flicker this path must avoid. Only real mount/menu gates hide it.
    const bool settled=!stable && !handoff && !surface.activation.pending();
    const bool reveal=surface.content_gate.allow(settled,GetTickCount64());
    if(reveal && (!surface.revealed || surface.displayed_count!=surface.visible.size()))
        logging::log(logging::Level::debug,logging::Channel::ui,
            "Stable HUD content visible: stack={:#x}, visible={}, mounted={}; Items unchanged.",
            stack.handle,surface.visible.size(),custom_hud_slot_order.size());
    surface.revealed=reveal;surface.displayed_count=surface.visible.size();
    for(auto& [id,slot]:surface.slots)
        c.set_if_changed(c.field(slot.anchor,widget),reveal && surface.visible.contains(id)?slot.display:Widget{});
}
void detach_hud_surfaces(const Context& c) {
    for(auto& [handle,surface]:state().hud_surfaces) {
        hide_hud_surface_content(c,surface);
        if(c.type_of(handle)!=surface.stack.type)continue;
        const auto target=c.field(surface.stack,items);unsigned count{},stride{};
        const auto old=c.array(target,custom_hud_stack_item_limit,count,stride);
        const auto offset=c.member(c.type(stack_item),0x1e95752c).offset;
        std::vector<std::byte> next;
        for(unsigned i=0;i<count;++i) {
            int id{};std::memcpy(&id,old.data()+i*stride+offset,4);
            if(!custom_hud_item(id))next.insert(next.end(),old.begin()+i*stride,old.begin()+(i+1)*stride);
        }
        publish_gameplay_hud_rows(c,surface.stack,next,stride,offset,[&] {
            if(next.size()!=old.size())c.array(target,next,static_cast<unsigned>(next.size()/stride));
        });
    }
    state().hud_surfaces.clear();
}
void publish_hud_items(const Context& c,bool score,bool countdown,bool intro=false,bool results=false) {
    auto& s=state();if(!s.hud_stack.handle)return;
    if(c.type_of(s.hud_stack.handle)!=s.hud_stack.type) {s.hud_ready.store(false);return;}
    const bool ready=hud_publication_allowed(read<bool>(c.address(c.field(s.hud_stack,0x369babfe))),s.in_menu,
        (score || countdown || intro || results) && s.match_panel_open.load());
    const bool settled=s.hud_publication.allow(s.hud_stack.handle,one_up::view().state.match,ready,GetTickCount64());
    if(!ready || ((score || countdown || intro || results || s.match_panel.prompt_visible) && !settled)) {
        if(const auto found=s.hud_surfaces.find(s.hud_stack.handle);found!=s.hud_surfaces.end()) {
            if(!ready)found->second.suspended=true;
            hide_hud_surface_content(c,found->second);
        }
        return;
    }
    std::vector<Value> visible;
    for(const auto item:{score?s.hud_item:Value{},score?s.hud_clock_item:Value{},score?s.hud_title_item:Value{},score?s.hud_badge_item:Value{},countdown?s.hud_timer_item:Value{},intro?s.hud_intro_item:Value{},results?s.hud_results_item:Value{},
        s.match_panel.prompt_visible && !s.in_menu && !s.match_panel.quitting?s.match_panel.prompt_item:Value{}})if(item.handle)visible.push_back(item);
    if(score)for(const auto id:s.hud_player_ids)if(const auto& player=s.hud_players.at(id);player.out && player.strike_item.handle)visible.push_back(player.strike_item);
    publish_hud_surface(c,s.hud_stack,visible,false);
}
void plain_label(const Context& c,Value value,const std::string& text) {
    const auto label_text=c.field(value,text_field);
    if(!read<Address>(c.address(label_text)) || c.text(label_text,2048)!=text) c.text(label_text,text);
    const auto visible=c.field(value,0x042924a4);
    if(!read<bool>(c.address(visible)))c.set(visible,true);
}
bool setup_body_content_matches(const Context& c,Value live,Value saved) {
    unsigned live_count{},live_stride{},saved_count{},saved_stride{};
    const auto live_items=c.array(c.field(live,items),128,live_count,live_stride);
    const auto saved_items=c.array(c.field(saved,items),128,saved_count,saved_stride);
    require(live_stride==stack_item.size && saved_stride==stack_item.size,"Native setup body item schema differs.");
    const auto offset=c.member(c.type(stack_item),content).offset;
    return matching_presenters<Widget>(live_items,saved_items,stack_item.size,offset);
}
void mount_setup_body(const Context& c,Value live,Value saved) {
    // Array publication makes a separate allocation. Compare presenter identity,
    // never its address or the native transition/focus fields surrounding Items.
    const auto live_array=c.field(live,items),saved_array=c.field(saved,items);
    unsigned live_count{},live_stride{},saved_count{},saved_stride{};
    const auto live_items=c.array(live_array,128,live_count,live_stride);
    const auto saved_items=c.array(saved_array,128,saved_count,saved_stride);
    require(live_stride==stack_item.size && saved_stride==stack_item.size,"Native setup body item schema differs.");
    const auto offset=c.member(c.type(stack_item),content).offset;
    // The body still has the same authored notice/list slots. Change just their
    // presenters; do not restart the StackItems' native visibility animation.
    if(const auto next=remount_presenters<Widget>(live_items,saved_items,stack_item.size,offset))
        c.array(live_array,*next,saved_count);
}
void restore_official(const Context& c) {
    auto& s=state();
    std::set<Handle> owned_panels;
    for(const auto& [panel,header]:s.official_headers) {
        if(c.type_of(panel)!=c.type(panel_schema) || !game::native_data().models.value(c.manager,panel,0,1))continue;
        if(read<Widget>(c.address(c.field({panel,c.type(panel_schema)},0xc0f1b449))).data.handle==header.handle)owned_panels.insert(panel);
    }
    for(const auto& [id,pair]:s.official_originals) {
        // A retired panel can still have a typed handle after its embedded
        // storage disappears. That must not block every later menu update.
        if(c.type_of(id)!=pair.first.type || !game::native_data().models.value(c.manager,id,0,1))continue;
        if(const auto owned=s.official_page_actions.find(id);owned!=s.official_page_actions.end()) {
            unsigned count{},stride{},saved_count{},saved_stride{};
            const auto live=c.array(pair.first,8,count,stride),saved=c.array(pair.second,8,saved_count,saved_stride);
            require(stride==page_action_schema.size && saved_stride==stride,"Native setup action schema differs during cleanup.");
            if(const auto restored=restore_owned_actions(live,saved,stride,owned->second.callback_offset,owned->second.rows))
                c.array(pair.first,*restored,count);
            continue;
        }
        // Body is an inline native StackComponent, whereas header/footer are
        // Widget presenters. Restore only content that is still ours.
        const auto panel=s.official_slot_panels.find(id);
        if(panel==s.official_slot_panels.end())continue;
        if(read<std::uint32_t>(read<Address>(pair.first.type))==0xd71ee0a1) {
            // A new native menu can have replaced the header first. Restore the
            // body only while it still presents our controls, and leave its
            // active/selection/transition state under the native owner's control.
            const auto private_body=s.official_contents.find(id);
            if(private_body!=s.official_contents.end() && setup_body_content_matches(c,pair.first,private_body->second))
                mount_setup_body(c,pair.first,pair.second);
        } else if(owned_panels.contains(panel->second))c.copy(pair.first,c.address(pair.second));
    }
    s.official_originals.clear(); s.official_panels.clear(); s.official_headers.clear(); s.official_contents.clear(); s.official_slot_panels.clear(); s.official_sliders.clear();s.official_setup_rows.clear();s.official_page_actions.clear();
}
void remember_official(const Context& c,Value value) {
    auto& originals=state().official_originals;
    if(originals.contains(value.handle))return;
    const auto copy=make(c,{read<std::uint32_t>(read<Address>(value.type)),static_cast<std::uint16_t>(size(value.type))},value.type);
    c.copy(copy,c.address(value)); originals.emplace(value.handle,std::pair{value,copy});
}
void label_styles(const Context& c,Value label,const char* idle,const char* focused) {
    c.set_if_changed(c.field(label,0xf94d8cc6),record_ref(c,idle));
    c.set_if_changed(c.field(label,0xa1e84eb1),record_ref(c,focused));
    c.set_if_changed(c.field(label,0xfc23a999),record_ref(c,idle));
    c.set_if_changed(c.field(label,0x25189231),record_ref(c,focused));
}
Value compact_anchor(const Context& c,Value model,const char* blueprint,float height,bool focusable) {
    auto result=make(c,anchor_schema);
    c.set(c.field(result,widget),Widget{asset(c,blueprint),{0,model.handle}});
    c.set(c.field(result,0x1cff7243),std::array<float,2>{1130.f,height});
    c.set(c.field(result,0x6efc1a61),false); c.set(c.field(result,0xbe5683d9),false);
    c.set(c.field(result,0x144aee01),focusable); return result;
}
void compact_panel(const Context& c) {
    auto& s=state(); if(s.panel.handle)return;
    s.panel=record_model(c,"Activities_DetailsMenu_ContentResource/Activities_Details_StackPanel",panel_schema);
    s.panel_anchor=record_model(c,"Activities_DetailsMenu_ContentResource/Activities_Details_AnchoredContentPresenter",anchor_schema);
    // This is the original compact Throwdown panel. All mutable content is
    // owned by 1-Up; the three native Throwdown models remain untouched.
    c.set(c.field(s.panel_anchor,widget),Widget{asset(c,"UI/Foundations/Components/StackVariations/StackPanel/StackPanel_Widget"),{0,s.panel.handle}});
    clear_value(c,c.field(s.panel_anchor,0x1aa5017f));
    // The authored Throwdown creation style supplies the same panel grain,
    // margins and footer dimensions as S.K.A.T.E.'s setup. Center it in the
    // page instead of inheriting the activity browser's left-column anchor.
    // RimeAxisLayoutData is stored as offsets/start/pivot/end/weight, not
    // in the order used by its authored schema. Publish named fields so the
    // panel cannot accidentally anchor at zero and extend off the left edge.
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const auto layout=c.field(s.panel_anchor,axis);
        const bool vertical=axis==0x185a5736U;
        c.set(c.field(layout,0x99bbed5a),vertical?0.f:.5f); // AnchorStart
        c.set(c.field(layout,0x49943bed),vertical?1.f:.5f); // AnchorEnd
        c.set(c.field(layout,0x40bfbff4),vertical?100.f:0.f); // OffsetStart
        c.set(c.field(layout,0x1d233266),vertical?-100.f:0.f); // OffsetEnd
        c.set(c.field(layout,0xfec5e0b1),.5f); // SizingPivot
        c.set(c.field(layout,0xd01f4a21),vertical?0.f:1.f); // SizingWeight
    }
    c.set(c.field(s.panel_anchor,0x6efc1a61),true);
    c.set(c.field(s.panel_anchor,0xbe5683d9),false);
    s.panel_style=record_model(c,"StackPanelStylesList/StackPanelStyle.ThrowdownCreation",panel_style_schema);
    c.set(c.field(s.panel_style,0xc13ba637),false);
    c.set(c.field(s.panel,0x8cc042ef),Ref{0,s.panel_style.handle});
    s.header=record_model(c,"Activities_DetailsMenu_ContentResource/Activities_Details_StackPanel_HeaderContent",header_schema);
    plain_label(c,c.field(s.header,0x44688629),"Throwdown");
    label_styles(c,c.field(s.header,0x44688629),"TextStylesList/B52-SemiB_Bright","TextStylesList/B52-SemiB_Bright");
    plain_label(c,c.field(s.header,0x00f3b15e),"1-UP");
    label_styles(c,c.field(s.header,0x00f3b15e),"TextStylesList/D180-Caps_White","TextStylesList/H72-XBold_White");
    c.set(c.path(s.header,{0x00f3b15e,0xfc53d427}),false);
    c.set(c.field(s.header,0x2840ecf6),Widget{});
    // FramedIcon paints a solid plate; its palette is RGB, not RGBA. The native
    // Texture widget preserves the badge's encoded alpha without a frame.
    s.badge=make(c,texture_schema);
    c.copy(c.field(s.badge,0x6e469d2a),c.address(c.field(s.originals[2].category,0x189084da)));
    c.set(c.field(s.badge,0xf524c836),288.f);
    c.set(c.field(s.badge,0x6fbd254e),288.f);
    c.set(c.field(s.badge,0x81c94d8a),false);
    c.set(c.field(s.badge,0xd1997d0c),false);
    c.set(c.field(s.badge,0x190e039c),false);
    c.set(c.field(s.header,content),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,s.badge.handle}});
    c.set(c.field(s.panel,0xc0f1b449),Widget{asset(c,"UI/Foundations/Components/ContentPresenterVariations/ContentWithTitle_Widget"),{0,s.header.handle}});
    s.panel_notice=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    plain_label(c,c.field(s.panel_notice,0x94703efc),"Beat the previous turn total. Three penalties and you're out.\nLast skater wins!");
    c.set(c.field(s.panel_notice,0x13be0d51),std::array<float,2>{1120.f,200.f});
    label_styles(c,c.field(s.panel_notice,0x94703efc),"TextStylesList/B52-SemiB_Bright","TextStylesList/B52-SemiB_Bright");
    c.set(c.field(s.panel,0x4fd7853d),Widget{});
    // Rules belong in the body, under the title. HeaderSecondaryContent is
    // an overlay intended for the mode notice, not the rules description.
    auto& rules=s.rows["rules"];
    rules.model=s.panel_notice;
    rules.anchor=compact_anchor(c,s.panel_notice,"UI/Foundations/Components/Notices/BasicNotice_Widget",200.f,false);
    auto first=make(c,stack_item);
    s.body_items={first};
    for(unsigned index=0;index<s.body_items.size();++index) {
        const auto item=s.body_items[index];
        c.set(c.field(item,0x1e95752c),static_cast<int>(0x31555010U+index));
        // StackComponentItem resolves its layout through Transition. Native
        // pushes supply this reference; directly published items must too.
        // Progress/IsActive alone do not give the widget a visible layout.
        c.copy(c.field(item,0xd78d240b),c.address(c.path(s.panel,{0x43967ece,0x54c9abee})));
    }
    c.set(c.field(first,content),Widget{asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,s.details_list.handle}});
    c.set(c.field(first,0x369babfe),true);
    // Fresh StackItems begin at interpolation progress zero, so their content
    // stays collapsed until the stack controller receives a show transition.
    // The panel body is permanently visible; initialize both tracks as shown.
    for(const auto item:s.body_items)for(const auto track:{0x8c0583eeU,0xc5c7cc11U}) {
        c.set(c.path(item,{track,0x20247753}),1.f);
        c.set(c.path(item,{track,0x2426103f}),1.f);
        c.set(c.path(item,{track,0x1e3ce013}),true);
    }
    std::vector<std::byte> item_bytes(stack_item.size);
    require(memory::peek_bytes(c.address(first),item_bytes.data(),stack_item.size),"1-Up panel items unavailable.");
    c.array(c.path(s.panel,{0x43967ece,items}),item_bytes,1);
    c.set(c.path(s.panel,{0x43967ece,0x1fbbb5da}),true);
    c.set(c.path(s.panel,{0x43967ece,0x369babfe}),true);
    c.set(c.path(s.panel,{0x43967ece,0xc912aa1d}),false);
    c.set(c.path(s.panel,{0x43967ece,0x2a98baa3}),false);
    c.set(c.path(s.panel,{0x43967ece,0x18f8355b}),-1);
    s.footer_anchor=record_model(c,"Activities_DetailsMenu_ContentResource/Activities_Details_StackPanel_Footer_AnchoredContentPresenter",anchor_schema);
    s.footer_list=record_model(c,"Activities_DetailsMenu_ContentResource/Activities_Details_StackPanel_FooterContent",linear_list);
    s.footer_back=record_model(c,"TD_Page_ContentResources/TD_Back_Button",button);
    s.footer_confirm=record_model(c,"TD_Page_ContentResources/TD_Create_Button",button);
    for(const auto b:{s.footer_back,s.footer_confirm}) {
        c.copy(c.field(b,0x58c9d354),c.address(s.source_label));
        label_styles(c,c.field(b,0x58c9d354),"TextStylesList/B52-SemiB-Caps_Dark","TextStylesList/B52-SemiB_Black");
        c.set(c.path(b,{0x58c9d354,0x3d8639d0}),2);
        c.set(c.field(b,0x659db23e),false);
        // These predicates are tied to the native mode's backend queue.
        // 1-Up uses its own coordinator and supplies its own availability.
        clear_value(c,c.field(b,0x67212c85));
    }
    c.set(c.path(s.footer_back,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/ButtonStyle.CTA.Default"));
    c.set(c.path(s.footer_confirm,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/ButtonStyle.CTA.Hero"));
    c.array(c.field(s.footer_list,rows_field),std::vector<Ref>{{0,s.footer_back.handle},{0,s.footer_confirm.handle}});
    c.set(c.field(s.footer_anchor,widget),Widget{asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,s.footer_list.handle}});
    c.set(c.field(s.panel,0xbfc64535),Widget{asset(c,"UI/Foundations/Templates/Layouts/Anchored/AnchoredContentPresenter_Widget"),{0,s.footer_anchor.handle}});
    const auto action_type=read<Address>(read<Address>(c.field(s.page,0xd2118ca8).type)+0x30);
    s.confirm_action=make(c,page_action_schema,action_type);
    c.copy(c.field(s.confirm_action,0xa704272a),c.address(c.path(s.footer_confirm,{0x0fb0d794,0xa704272a})));
    s.primary_input=0;
    const auto native_actions=c.field(s.original_actions,0xd2118ca8);
    unsigned action_count{},action_stride{};
    c.array(native_actions,8,action_count,action_stride);
    require(action_stride==page_action_schema.size,"1-Up native page action schema differs.");
    for(unsigned i=0;i<action_count && !s.primary_input;++i) {
        const auto original=c.path(c.element(native_actions,i),{0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
        const auto candidate=read<Address>(c.address(c.field(original,0x3d00de27)));
        if(candidate && string(read<Address>(candidate+0x18))=="Configurations/Input/UI/UI_PrimaryAction") s.primary_input=candidate;
    }
    if(!s.primary_input) s.primary_input=asset(c,"Configurations/Input/UI/UI_PrimaryAction");
    const auto confirm=c.path(s.confirm_action,{0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
    c.set(c.field(confirm,0x3d00de27),s.primary_input);
    c.set(c.field(confirm,0x0169c2db),s.primary_input);
    logging::write(logging::Level::info,logging::Channel::ui,"1-Up: authored Throwdown creation layout, settings and Confirm controls constructed.");
}
Value description(const Context& c, Value cat) { return reference(c, read<Widget>(c.address(c.field(cat,widget))).data); }
std::optional<Value> card_list(const Context& c, Value p) {
    try {
        const auto slot=mounted_body(c,p);if(!slot)return {};
        auto w = read<Widget>(c.address(*slot));
        // The private grid replaces the presenter, not the stock list. Retain
        // the original list as the generation's identity while our grid mounts.
        const auto& s=state();
        if(p.handle==s.page.handle && s.picker_anchor.handle && w.data.handle==s.picker_anchor.handle &&
           c.type_of(s.cards.handle)==s.cards.type) return s.cards;
        for (unsigned depth=0; depth<4; ++depth) {
            auto value = reference(c,w.data); if (!value.type) return {};
            const auto hash = read<std::uint32_t>(read<Address>(value.type));
            if (hash == linear_list.hash && size(value.type)==linear_list.size) {
                unsigned count{}, stride{}; auto bytes=c.array(c.field(value,rows_field),4,count,stride);
                if (count!=3 || stride!=sizeof(Ref)) return {};
                // The live Skate Jam card navigates through ThrowdownerActive;
                // JamSession is its gameplay mode, not its menu action name.
                const std::array<std::string_view,3> expected{"ThrowdownSkate","SpotBattle","ThrowdownerActive"};
                for (unsigned i=0;i<3;++i) {
                    Ref ref{}; std::memcpy(&ref,bytes.data()+i*stride,stride); auto t=reference(c,ref);
                    if (!t.type || size(t.type)!=tile_schema.size || read<std::uint32_t>(read<Address>(t.type))!=tile_schema.hash ||
                        c.text(c.field(primary(c,t),0xb1e14cd6)) != expected[i]) return {};
                }
                return value;
            }
            if (hash!=anchor_schema.hash || size(value.type)!=anchor_schema.size) return {};
            w=read<Widget>(c.address(c.field(value,widget)));
        }
    } catch (...) {}
    return {};
}
void switch_page(const Context& c, bool details) {
    auto& s=state();
    if (details) {
        compact_panel(c);
        c.set(body(c,s.page),Widget{asset(c,"UI/Foundations/Templates/Layouts/Anchored/AnchoredContentPresenter_Widget"),{0,s.panel_anchor.handle}});
        c.set(c.field(back(c,s.page),0x9c76b86c),callback(c,"back"));
        c.text(c.field(back(c,s.page),0xb1e14cd6),"");
        c.text(c.path(s.page,{0x3781b603,text_field}),"");
        request_ui_dump("0xd29bbf63",2);
    } else {
        if(s.picker_anchor.handle)
            c.set(body(c,s.page),Widget{asset(c,"UI/Foundations/Templates/Layouts/Anchored/AnchoredContentPresenter_Widget"),{0,s.picker_anchor.handle}});
        else c.copy(body(c,s.page),c.address(s.original_body));
        c.copy(back(c,s.page),c.address(s.original_back));
        c.copy(c.field(s.page,0xd2118ca8),c.address(c.field(s.original_actions,0xd2118ca8)));
        c.text(c.path(s.page,{0x3781b603,text_field}),s.original_title);
    }
    s.details=details;
    s.setup_open.store(details,std::memory_order_release);
}
void release(const Context& c,bool preserve_match_input=false) {
    auto& s=state(); s.owner.store(0);
    if(preserve_match_input) {
        // A picker generation is not a world/UI-manager lifetime. Retiring
        // it during Start previously destroyed the active HUD, panel and
        // delegates while native navigation still retained those references.
        if(const auto slot=mounted_body(c,s.page)) {
            const auto mounted=read<Widget>(c.address(*slot)).data.handle;
            if(mounted==s.picker_anchor.handle || mounted==s.panel_anchor.handle) {
                c.copy(*slot,c.address(s.original_body));
                c.copy(back(c,s.page),c.address(s.original_back));
                c.copy(c.field(s.page,0xd2118ca8),c.address(c.field(s.original_actions,0xd2118ca8)));
                c.text(c.path(s.page,{0x3781b603,text_field}),s.original_title);
            }
        }
        if(s.cards.handle && c.type_of(s.cards.handle)==s.cards.type) {
            unsigned count{},stride{};const auto bytes=c.array(c.field(s.cards,rows_field),64,count,stride);
            require(stride==sizeof(Ref),"1-Up card list changed during picker retirement.");
            std::vector<Ref> retained;
            for(unsigned i=0;i<count;++i) {
                Ref ref{};std::memcpy(&ref,bytes.data()+i*stride,stride);
                if(ref.handle!=s.card.handle && std::none_of(s.mode_cards.begin(),s.mode_cards.end(),
                    [&](const auto& entry){return entry.second.tile.handle==ref.handle;}))retained.push_back(ref);
            }
            c.array(c.field(s.cards,rows_field),retained);
        }
        for(const auto& o:s.originals) {
            if(c.type_of(o.tile.handle)==o.tile.type)c.set(c.field(o.tile,0x868043e1),o.tile_size);
            if(c.type_of(o.category.handle)==o.category.type) {
                c.set(c.field(o.category,0x495ffd43),o.icon_width);c.set(c.field(o.category,0x24fb5ca8),o.icon_height);
            }
            if(c.type_of(o.description.handle)==o.description.type)c.set(c.field(o.description,0x1cff7243),o.description_size);
        }
        // Retained roots are released by the existing real world/manager
        // cleanup. Native cached pages can still reference this generation.
        s.page={};s.cards={};s.card={};s.panel={};s.panel_anchor={};s.badge={};
        s.duration_selector={};s.body_items={};s.confirm_action={};s.original_actions={};
        s.picker_grid={};s.picker_anchor={};s.oneup_logo={};s.picker_stock={};s.picker_stock_logos={};
        s.mode_cards.clear();s.selected_mode.clear();s.details_list={};s.rows.clear();s.originals.clear();
        s.displayed.clear();s.duration_choices.clear();s.details=false;s.setup_open.store(false);
        logging::write(logging::Level::info,logging::Channel::ui,
            "Throwdowns: picker generation retired; active gameplay HUD, native panel and delegates retained.");
        return;
    }
    restore_map_branding(c);
    restore_match_panel(c);
    session_marker_controls(c,true);
    s.solo_sliders.clear();
    s.waiting_retired_notifications.insert(s.waiting_widgets.begin(),s.waiting_widgets.end());
    for(const auto& [handle,presentation]:s.waiting_presentations)
        if(c.type_of(handle)==presentation.first.type) {
            const auto at=game::native_data().models.value(c.manager,handle,0,1);
            if(!at)continue;
            const auto current=read<Widget>(at).data.handle;
            const bool owned=(s.waiting_hidden_slots.contains(handle) && !current) || std::any_of(s.waiting_originals.begin(),s.waiting_originals.end(),[&](const auto& entry) {
                return current==entry.second.handle || current==c.field(entry.second,0xc13bcfcd).handle;
            });
            // The donor is a temporary backend queue. Restoring its original
            // Spot Battle card during exit exposes a challenge we never joined.
            if(owned)c.set(presentation.first,Widget{});
        }
    s.waiting_presentations.clear();
    s.waiting_hidden_slots.clear();
    { std::lock_guard lock(s.mutex);
      std::erase_if(s.pending,[](const Action& a){return a.command!="oneup" && a.command!="mode-setup-area" && a.command!="mode-setup-join" && a.command!="panel-quit" && a.command!="throwdown-panel" && a.command!="mode-panel-quit" && a.command!="mode-throwdown-panel" && a.command!="mode-wait-start" && a.command!="mode-wait-leave";}); }
    s.owned.release([&] {
        detach_hud_surfaces(c);restore_hud_active_budgets(c);s.hud_ready.store(false);
        restore_official(c);
    s.waiting_originals.clear(); s.waiting_widgets.clear(); s.waiting_backups.clear(); s.waiting_presentations.clear(); s.waiting_counts.clear(); s.waiting_label={}; s.waiting_countdown={}; s.waiting_badge={}; s.waiting_diagnostic=0;
        if (const auto slot=mounted_body(c,s.page)) {
            const auto mounted=read<Widget>(c.address(*slot)).data.handle;
            if(mounted==s.picker_anchor.handle || mounted==s.panel_anchor.handle) {
                c.copy(*slot,c.address(s.original_body));
                c.copy(back(c,s.page),c.address(s.original_back));
                c.copy(c.field(s.page,0xd2118ca8),c.address(c.field(s.original_actions,0xd2118ca8)));
                c.text(c.path(s.page,{0x3781b603,text_field}),s.original_title);
            }
        }
        if (s.cards.handle && c.type_of(s.cards.handle)==s.cards.type) {
            unsigned count{},stride{}; auto bytes=c.array(c.field(s.cards,rows_field),64,count,stride);
            std::vector<Ref> retained;
            require(stride==sizeof(Ref),"1-Up card list changed during cleanup.");
            for (unsigned i=0;i<count;++i) { Ref ref{}; std::memcpy(&ref,bytes.data()+i*stride,stride); if(ref.handle!=s.card.handle && std::none_of(s.mode_cards.begin(),s.mode_cards.end(),[&](const auto& entry){return entry.second.tile.handle==ref.handle;})) retained.push_back(ref); }
            c.array(c.field(s.cards,rows_field),retained);
        }
        for (const auto& o:s.originals) {
            if (c.type_of(o.tile.handle)==o.tile.type) c.set(c.field(o.tile,0x868043e1),o.tile_size);
            if (c.type_of(o.category.handle)==o.category.type) { c.set(c.field(o.category,0x495ffd43),o.icon_width); c.set(c.field(o.category,0x24fb5ca8),o.icon_height); }
            if (c.type_of(o.description.handle)==o.description.type) c.set(c.field(o.description,0x1cff7243),o.description_size);
        }
    },[&](Value value){c.destroy(value);});
    s.page={}; s.cards={}; s.card={}; s.panel={}; s.panel_anchor={}; s.badge={}; s.official_badge={};s.official_race_badge={};s.official_race=0; s.duration_selector={}; s.body_items={}; s.confirm_action={}; s.original_actions={};
    s.picker_grid={};s.picker_anchor={};s.oneup_logo={};s.picker_stock={};s.picker_stock_logos={};
    s.mode_cards.clear();s.selected_mode.clear();s.mode_hud={};s.map_branding={};s.details_list={};s.rows.clear(); s.originals.clear(); s.assets.clear(); s.displayed.clear(); s.duration_choices.clear(); s.details=false;
    clear_hud_state(preserve_match_input);
    s.setup_open.store(false,std::memory_order_release);
}
void initialize(const Context& c, Value p, Value list) {
    auto& s=state(); s.page=p; s.cards=list; s.owner.store(p.handle);
    unsigned count{},stride{}; auto bytes=c.array(c.field(list,rows_field),3,count,stride);
    std::vector<Ref> refs(count); std::memcpy(refs.data(),bytes.data(),bytes.size());
    for (const auto& ref:refs) {
        auto tile=reference(c,ref), cat=category(c,tile), desc=description(c,cat);
        s.originals.push_back({tile,cat,desc,read<std::array<float,2>>(c.address(c.field(tile,0x868043e1))),read<std::array<float,2>>(c.address(c.field(desc,0x1cff7243))),
            read<float>(c.address(c.field(cat,0x495ffd43))),read<float>(c.address(c.field(cat,0x24fb5ca8)))});
    }
    const auto donor=s.originals[1]; const auto card_widget=read<Widget>(c.address(c.path(donor.tile,{widget,widget})));
    s.anchor_asset=card_widget.blueprint;
    const auto donor_label=reference(c,read<Widget>(c.address(c.field(donor.description,widget))).data);
    require(donor_label.type && size(donor_label.type)==label_schema.size,"1-Up description label unavailable.");
    // Gameplay and the details panel outlive the picker that supplied this
    // label. Own its typed copy instead of borrowing a retired native page.
    s.source_label=make(c,label_schema,donor_label.type);c.copy(s.source_label,c.address(donor_label));
    s.card=make(c,tile_schema,donor.tile.type); c.copy(s.card,c.address(donor.tile));
    s.category=make(c,category_schema,donor.category.type); c.copy(s.category,c.address(donor.category));
    for (const auto hash : {0x189084daU, 0xc2917efcU})
        c.copy(c.field(s.category, hash), c.address(c.field(s.originals[2].category, hash)));
    s.description=make(c,anchor_schema,donor.description.type); c.copy(s.description,c.address(donor.description));
    auto label=make(c,label_schema,s.source_label.type); c.copy(label,c.address(s.source_label));
    c.text(c.field(label,text_field),"Take turns beating the last total. Three penalties and you're out."); c.set(c.field(label,0x042924a4),true);
    auto desc_widget=read<Widget>(c.address(c.field(s.description,widget))); desc_widget.data={0,label.handle}; c.set(c.field(s.description,widget),desc_widget);
    auto cat_widget=read<Widget>(c.address(c.field(s.category,widget))); cat_widget.data={0,s.description.handle}; c.set(c.field(s.category,widget),cat_widget);
    c.text(c.path(s.category,{0x77b8ed0b,text_field}),"1-UP"); c.set(c.path(s.category,{0x77b8ed0b,0x042924a4}),true);
    auto cw=card_widget; cw.data={0,s.category.handle}; c.set(c.path(s.card,{widget,widget}),cw);
    c.text(c.field(primary(c,s.card),0xb1e14cd6),""); c.set(c.field(primary(c,s.card),0x9c76b86c),callback(c,"open"));
    s.original_body=make(c,presenter,body(c,p).type); c.copy(s.original_body,c.address(body(c,p)));
    s.original_back=make(c,action_schema,back(c,p).type); c.copy(s.original_back,c.address(back(c,p)));
    s.original_title=c.text(c.path(p,{0x3781b603,text_field}));
    s.original_actions=make(c,page,p.type);
    c.copy(c.field(s.original_actions,0xd2118ca8),c.address(c.field(p,0xd2118ca8)));
    // The card only needs its three live siblings. Setup content is adapted
    // after the native screen opens; unavailable optional widgets must never
    // prevent the fourth card from appearing.
    mode_cards(c);
    logo_picker(c);
    s.generations.emplace(p.handle,list.handle);
    logging::write(logging::Level::info,logging::Channel::ui,"Throwdowns: native logo grid registered below the page heading.");
}
void row(const Context& c,std::vector<std::string>& shown,const std::string& id,const std::string& text,std::string command={},std::string argument={}) {
    auto& s=state(); auto [entry,fresh]=s.rows.try_emplace(id); auto& r=entry->second;
    const bool button_row=!command.empty();
    if(fresh) {
        r.model=make(c,button_row ? button : label_schema); r.anchor=make(c,anchor_schema);
        auto l=button_row ? c.field(r.model,0x58c9d354) : r.model; c.copy(l,c.address(s.source_label)); c.set(c.field(l,0x042924a4),true);
        c.set(c.field(r.anchor,widget),Widget{asset(c,button_row?"UI/Foundations/Components/Buttons/LabelButton/LabelButton_Widget":"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,r.model.handle}});
        c.set(c.field(r.anchor,0x1cff7243),std::array<float,2>{1130.f,button_row?144.f:100.f});
        c.set(c.field(r.anchor,0x6efc1a61),false); c.set(c.field(r.anchor,0xbe5683d9),false); c.set(c.field(r.anchor,0x144aee01),button_row);
    }
    auto l=button_row?c.field(r.model,0x58c9d354):r.model;
    if(c.text(c.field(l,text_field),2048)!=text) c.text(c.field(l,text_field),text);
    r.text=text;
    // Constructors can restore button defaults after mounting. Reconcile the
    // native styles so focused buttons do not become white text on white fill.
    c.set(c.field(l,0x042924a4),true);
    c.set(c.field(l,0xfc53d427),false);
    for(const auto hash:{0xf94d8cc6U,0xa1e84eb1U,0xfc23a999U,0x25189231U}) {
        const auto source=button_row?hash:0xf94d8cc6U;
        const auto wanted=read<Ref>(c.address(c.field(s.source_label,source)));
        const auto current=read<Ref>(c.address(c.field(l,hash)));
        if(current.record!=wanted.record || current.handle!=wanted.handle) c.set(c.field(l,hash),wanted);
    }
    if(button_row) {
        c.set(c.field(r.model,0x659db23e),false);
        c.set(c.field(l,0x3d8639d0),0);
        const auto& native_page=native_menu_detail::page_state(0);
        if(native_page.manager==c.manager && native_page.menu_style.handle && c.type_of(native_page.menu_style.handle)==native_page.menu_style.type)
            c.set(c.path(r.model,{0x0fb0d794,0x8cc042ef}),Ref{0,native_page.menu_style.handle});
    } else {
        c.set(c.field(l,0xcd71a279),false);
        c.set(c.field(l,0x3fa0b887),false);
    }
    if(button_row) {
        const auto at=c.path(r.model,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
        const auto action=callback(c,std::move(command),std::move(argument));
        if(r.callback!=action) { c.set(c.field(at,0x9c76b86c),action); c.text(c.field(at,0xb1e14cd6),""); r.callback=action; }
    }
    shown.push_back(id);
}
void setting_row(const Context& c,std::vector<std::string>& shown,const std::string& id,const std::string& title,const std::string& value,bool selectable=false,bool embedded=false) {
    auto& s=state(); auto [entry,fresh]=s.rows.try_emplace(id); auto& r=entry->second;
    const bool players=id=="players-setting";
    if(fresh) {
        r.model=make(c,setting_schema);
        auto label=c.field(r.model,0x58c9d354); c.copy(label,c.address(s.source_label));
        label_styles(c,label,"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_Black");
        c.set(c.field(r.model,0xc9bf367d),record_ref(c,"SettingWrapperStylesList/Horizontal.WithDivider"));
        c.set(c.field(r.model,0xf3d74cdd),0);
        Value control;
        if(selectable) {
            control=record_model(c,"TD_HostParameters_ContentResource/TD_Timer_BasicSelector",selector_schema);
            // The original selector's two-way bindings belong to Skate Jam.
            // Own the choices and observe only this selector's selected index.
            for(const auto field:{0x4256fd65U,0x85b8c3e6U,0xe88c0f54U}) clear_value(c,c.field(control,field));
            c.set(c.field(control,0xeb92bf1c),Ref{});
            for(const auto field:{0x41b10c27U,0xd637e1f4U}) c.set(c.field(control,field),Address{});
            auto list=c.field(control,0x760fc12b); c.set(c.field(list,0x340cfb95),Address{});
            r.choices.clear();
            for(unsigned n=players?1U:10U;n<=(players?6U:120U);n+=players?1U:10U) {
                const auto option=std::to_string(n)+(players?"":" sec");
                auto choice=make(c,selector_item_schema), text=make(c,label_schema);
                c.copy(text,c.address(s.source_label));
                label_styles(c,text,"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_Black");
                plain_label(c,text,option);
                c.text(c.field(choice,0x58c9d354),option);
                c.set(c.field(choice,0x34c8bcc2),Ref{0,text.handle}); r.choices.push_back(choice.handle);
            }
            c.array(c.field(list,rows_field),std::vector<Ref>{});
            c.array(c.field(list,0xf2c90867),r.choices);
            c.set(c.field(list,0x55511280),static_cast<int>(players?s.max_players-1:s.seconds/10-1));
            c.set(c.field(list,0x783c4b49),r.choices[players?s.max_players-1:s.seconds/10-1]);
            c.set(c.field(control,0x369babfe),true);
            c.set(c.field(r.model,content),Widget{asset(c,"UI/Foundations/Components/Settings/Widgets/BasicSelector_Widget"),{0,control.handle}});
            r.selector=control; r.callback=callback(c,players?"players":"duration");
        } else {
            control=make(c,label_schema); c.copy(control,c.address(s.source_label));
            label_styles(c,control,"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_Black");
            plain_label(c,control,value); c.set(c.field(control,0x3d8639d0),2);
            c.set(c.field(r.model,content),Widget{asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,control.handle}});
        }
        if(selectable && !embedded) {
            // The original Throwdown list wraps settings in ContentPresenterTile;
            // SettingWrapper alone changes label colors but has no blue fill.
            r.tile=make(c,content_tile_schema);
            c.set(c.field(r.tile,widget),Widget{asset(c,"UI/Foundations/Components/Settings/Widgets/SettingWrapper_Widget"),{0,r.model.handle}});
            c.set(c.field(r.tile,0xa04998ca),false);
            c.set(c.field(r.tile,0xbc750e6a),true);
            c.set(c.path(r.tile,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/TileButton.RoughPartial.Default"));
            c.set(c.path(r.tile,{0x0fb0d794,0xa704272a,0xc52416ef,0x0abf7c31}),Ref{0,r.model.handle});
            r.anchor=compact_anchor(c,r.tile,"UI/Foundations/Components/Buttons/ContentPresenterTile/ContentPresenterTile",172.f,true);
        } else if(!embedded) r.anchor=compact_anchor(c,r.model,"UI/Foundations/Components/Settings/Widgets/SettingWrapper_Widget",172.f,false);
    }
    plain_label(c,c.field(r.model,0x58c9d354),title);
    if(r.tile.handle) {
        const auto style=c.path(r.tile,{0x0fb0d794,0x8cc042ef});
        const auto desired=record_ref(c,"ButtonStyleList/TileButton.RoughPartial.Default");
        const auto current=read<Ref>(c.address(style));
        if(current.record!=desired.record || current.handle!=desired.handle)c.set(style,desired);
    }
    if(selectable) {
        const auto choices=c.field(r.selector,0x760fc12b);
        const auto current=read<Address>(c.address(c.field(choices,0xf2c90867)));
        const auto count=current?read<std::uint32_t>(current-4)&0x7fffffff:0;
        if(count!=r.choices.size()) {
            c.array(c.field(choices,0xf2c90867),r.choices);
            const auto index=players?s.max_players-1:s.seconds/10-1;
            c.set(c.field(choices,0x55511280),static_cast<int>(index));
            c.set(c.field(choices,0x783c4b49),r.choices[index]);
        }
        const auto index=read<int>(c.address(c.field(choices,0x55511280)));
        if(index>=0 && static_cast<std::size_t>(index)<r.choices.size()) {
            if(players)s.max_players=static_cast<unsigned>(index)+1;
            else s.seconds=static_cast<unsigned>(index+1)*10;
        }
    } else {
        const auto control=reference(c,read<Widget>(c.address(c.field(r.model,content))).data);
        plain_label(c,control,value);
    }
    shown.push_back(id);
}
void penalty_row(const Context& c,std::vector<std::string>& shown) {
    auto& s=state(); auto [entry,fresh]=s.rows.try_emplace("penalty-stamps"); auto& r=entry->second;
    if(fresh) {
        r.model=make(c,setting_schema);
        const auto label=c.field(r.model,0x58c9d354); c.copy(label,c.address(s.source_label));
        label_styles(c,label,"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_White");
        plain_label(c,label,"Penalties");
        c.set(c.field(r.model,0xc9bf367d),record_ref(c,"SettingWrapperStylesList/Horizontal.WithDivider"));
        c.set(c.field(r.model,0xf3d74cdd),0);
        r.selector=make(c,linear_list);
        c.array(c.field(r.selector,rows_field),std::vector<Ref>{});
        c.array(c.field(r.selector,0xf2c90867),std::vector<Handle>{});
        c.set(c.field(r.selector,0x55ca89f4),asset(c,"UI/Foundations/Components/Lists/Shared/AnchoredContentPresenterListItem_Widget"));
        c.set(c.field(r.selector,0x64b9a9e8),0);
        c.set(c.field(r.selector,0x4cb61cac),0); // Horizontal stamps.
        c.set(c.field(r.selector,0x19ff199a),108.f);
        c.set(c.field(r.selector,0xebca7354),20.f);
        c.set(c.field(r.selector,0x4554761c),true);
        c.set(c.field(r.selector,0x8bbf7d67),false);
        for(const auto* letter:{"1","U","P"}) {
            auto stamp=make(c,button);
            const auto text=c.field(stamp,0x58c9d354); c.copy(text,c.address(s.source_label));
            plain_label(c,text,letter);
            label_styles(c,text,"TextStylesList/H64-XBold_Black","TextStylesList/H64-XBold_Black");
            c.set(c.field(text,0x3d8639d0),2);
            c.set(c.path(stamp,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/ButtonStyle.CTA.Default"));
            const auto anchor=compact_anchor(c,stamp,"UI/Foundations/Components/Buttons/LabelButton/LabelButton_Widget",108.f,false);
            c.set(c.field(anchor,0x1cff7243),std::array<float,2>{108.f,108.f});
            // A new anchor has stretch axes. Fixed measures must be paired
            // with a non-stretch axis or the three letters cover each other.
            for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
                const auto layout=c.field(anchor,axis);
                c.set(c.field(layout,0x99bbed5a),.5f); c.set(c.field(layout,0x49943bed),.5f);
                c.set(c.field(layout,0xfec5e0b1),.5f); c.set(c.field(layout,0xd01f4a21),1.f);
            }
            r.choices.push_back(anchor.handle);
        }
        c.set(c.field(r.model,content),Widget{asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,r.selector.handle}});
        r.anchor=compact_anchor(c,r.model,"UI/Foundations/Components/Settings/Widgets/SettingWrapper_Widget",144.f,false);
    }
    const auto mounted=read<Address>(c.address(c.field(r.selector,0xf2c90867)));
    if(!mounted || (read<std::uint32_t>(mounted-4)&0x7fffffff)!=r.choices.size())
        c.array(c.field(r.selector,0xf2c90867),r.choices);
    shown.push_back("penalty-stamps");
}
// Adapt the ACTUAL native setup mounted by SpotBattle. Its stack, anchors,
// transitions, footer and Confirm navigation stay under the game's ownership.
void remove_one_up_rounds(const Context& c,Value root) {
    std::set<Handle> seen;
    std::function<bool(Value,unsigned)> visit=[&](Value value,unsigned depth) -> bool {
        require(depth<96,"1-Up settings graph depth differs.");
        if(!value.handle)return false;
        const auto hash=read<std::uint32_t>(read<Address>(value.type));
        if(hash==setting_schema.hash) {
            const auto id=c.text(c.path(value,{0x58c9d354,text_field}));
            return native_setup_role(id)==NativeSetupRole::rounds;
        }
        if(!seen.insert(value.handle).second)return false;
        struct Unvisit { std::set<Handle>& seen;Handle handle;~Unvisit(){seen.erase(handle);} } unvisit{seen,value.handle};
        if(hash==0x62088281) {
            const auto ref=read<Ref>(c.address(value));
            return ref.handle && visit(reference(c,ref),depth+1);
        }
        if(kind(value.type)==4) {
            unsigned n{},stride{};const auto old=c.array(value,128,n,stride);
            std::vector<std::byte> kept;
            for(unsigned i=0;i<n;++i) {
                const auto child=c.element(value,i);
                if(kind(child.type)==2 || kind(child.type)==4) {
                    const auto copy=make(c,{read<std::uint32_t>(read<Address>(child.type)),static_cast<std::uint16_t>(size(child.type))},child.type);
                    c.copy(copy,c.address(child));
                    if(visit(copy,depth+1))continue;
                    const auto offset=kept.size();kept.resize(offset+stride);
                    require(memory::peek_bytes(c.address(copy),kept.data()+offset,stride),"Private settings row is unavailable.");
                } else kept.insert(kept.end(),old.begin()+i*stride,old.begin()+(i+1)*stride);
            }
            if(kept!=old)c.array(value,kept,static_cast<unsigned>(kept.size()/stride));
            return false; // Removed an individual row, not its enclosing list.
        }
        bool remove=false;
        if(kind(value.type)==2) {
            const auto meta=read<Address>(value.type),fields=read<Address>(meta+0x60);
            const auto count=read<std::uint16_t>(meta+0x2a);require(count<=128,"1-Up settings field count differs.");
            for(unsigned i=0;i<count;++i) {
                const auto key=read<std::uint32_t>(fields+i*24);
                if(key==0x8cc042ef || key==0xa1e84eb1 || key==0x25189231 || key==0xf94d8cc6 ||
                   key==0x8a3e2159 || key==0x54c9abee || key==0xd78d240b || key==0x85b8c3e6 || key==655725074U)continue;
                const auto child=c.field(value,key);
                if(key==0xf2c90867 && kind(child.type)==4) {
                    unsigned n{},stride{};const auto old=c.array(child,128,n,stride);
                    require(stride==sizeof(Handle),"1-Up setting handle list differs.");
                    std::vector<Handle> kept;
                    for(unsigned j=0;j<n;++j) {
                        Handle handle{};std::memcpy(&handle,old.data()+j*stride,stride);
                        if(!handle || !visit({handle,c.type_of(handle)},depth+1))kept.push_back(handle);
                    }
                    if(kept.size()!=n)c.array(child,kept);
                } else if(kind(child.type)==2 || kind(child.type)==4)remove=visit(child,depth+1)||remove;
            }
        }
        return remove;
    };
    visit(root,0);
}
NativeSetupRows native_setup_rows(const Context& c,Value body_value) {
    // HostParameters materializes its settings into the native LinearList's
    // OwnedItems ContentPresenterTile array. ListItems/ListItemHandles are
    // legitimate alternate representations but are empty in the captured page.
    for(const auto list:ui_models(c,body_value,linear_list.hash)) {
        NativeSetupRows result;result.list=list;
        const auto owned=c.field(list,0x345b75b1);unsigned owned_count{},owned_stride{};
        c.array(owned,128,owned_count,owned_stride);
        require(owned_stride==content_tile_schema.size,"Native setup OwnedItems tile schema differs.");
        bool owned_players{};unsigned timer_index{};
        for(unsigned i=0;i<owned_count;++i) {
            const auto row=c.element(owned,i);
            for(const auto setting:ui_models(c,row,setting_schema.hash)) {
                const auto role=native_setup_role(c.text(c.path(setting,{0x58c9d354,text_field})));
                owned_players=owned_players || role==NativeSetupRole::players;
                if(role==NativeSetupRole::timer) {result.timer_row=row;timer_index=i;}
            }
        }
        if(owned_players && result.timer_row.handle) {
            result.retained_rows=make(c,{read<std::uint32_t>(read<Address>(owned.type)),
                static_cast<std::uint16_t>(size(owned.type))},owned.type);
            c.copy(result.retained_rows,c.address(owned));
            result.timer_row=c.element(result.retained_rows,timer_index);
            result.owned_tiles=true;return result;
        }
        result.timer_row={};
        unsigned records_count{},records_stride{},handles_count{},handles_stride{};
        const auto records=c.array(c.field(list,rows_field),128,records_count,records_stride);
        const auto handles=c.array(c.field(list,0xf2c90867),128,handles_count,handles_stride);
        require(records_stride==sizeof(Ref) && handles_stride==sizeof(Handle),"Native setup list item types differ.");
        if(records_count && handles_count)continue;
        result.records.resize(records_count);result.handles.resize(handles_count);
        if(records_count)std::memcpy(result.records.data(),records.data(),records.size());
        if(handles_count)std::memcpy(result.handles.data(),handles.data(),handles.size());
        bool players{};
        const auto inspect=[&](Value row) {
            for(const auto setting:ui_models(c,row,setting_schema.hash)) {
                const auto id=c.text(c.path(setting,{0x58c9d354,text_field}));
                const auto role=native_setup_role(id);
                if(role==NativeSetupRole::players)players=true;
                if(role==NativeSetupRole::timer)result.timer_row=row;
            }
        };
        for(const auto ref:result.records)if(ref.handle)inspect(reference(c,ref));
        for(const auto handle:result.handles)if(handle)inspect({handle,c.type_of(handle)});
        if(players && result.timer_row.handle)return result;
    }
    // StackPanel.Body is the exported StackViewModel (0xd71ee0a1). Some
    // native HostParameters generations mount each setting as a StackItem
    // instead of using a nested LinearList. Inspect its real Items contract.
    if(read<std::uint32_t>(read<Address>(body_value.type))==0xd71ee0a1) {
        NativeSetupRows result;result.list=body_value;result.stack=true;
        const auto items_value=c.field(body_value,items);unsigned count{},stride{};
        c.array(items_value,128,count,stride);
        require(stride==stack_item.size,"Native HostParameters StackItem schema differs.");
        bool players{};unsigned timer_index{};
        for(unsigned i=0;i<count;++i) {
            const auto row=c.element(items_value,i);
            const auto settings=ui_models(c,row,setting_schema.hash);
            if(settings.size()!=1)continue;
            const auto id=c.text(c.path(settings.front(),{0x58c9d354,text_field}));
            const auto role=native_setup_role(id);
            if(role==NativeSetupRole::players)players=true;
            if(role==NativeSetupRole::timer) {result.timer_row=row;timer_index=i;}
        }
        if(players && result.timer_row.handle) {
            result.retained_rows=make(c,{read<std::uint32_t>(read<Address>(items_value.type)),
                static_cast<std::uint16_t>(size(items_value.type))},items_value.type);
            c.copy(result.retained_rows,c.address(items_value));
            result.timer_row=c.element(result.retained_rows,timer_index);
            return result;
        }
    }
    throw std::runtime_error("Native HostParameters list with Players and Timer is not mounted yet.");
}
Value native_setup_extra(const Context& c,NativeSetupRows& rows,const std::string& key,const std::string& caption) {
    if(const auto found=rows.extra.find(key);found!=rows.extra.end())return found->second;
    PrivateUiGraph graph{c};const auto row=graph.clone({0,rows.timer_row.handle});
    const auto settings=ui_models(c,row,setting_schema.hash);
    require(settings.size()==1,"Native setup template must contain one SettingWrapper.");
    plain_label(c,c.field(settings.front(),0x58c9d354),caption);
    if(rows.stack)c.set(c.field(row,0x1e95752c),static_cast<std::int32_t>(2000+rows.extra.size()));
    rows.extra.emplace(key,row);return row;
}
void native_setup_action_row(const Context& c,NativeSetupRows& rows,const std::string& key,
        const std::string& caption,const std::string& value,const std::string& command,const std::string& argument) {
    const bool fresh=!rows.extra.contains(key);
    const auto checkpoint=state().owned.checkpoint();
    try {
    const auto row=native_setup_extra(c,rows,key,caption);
    const auto setting=ui_models(c,row,setting_schema.hash).front();
    plain_label(c,c.field(setting,0x58c9d354),caption);
    auto slot=c.field(setting,content);
    Value control;
    if(fresh) {
        control=make(c,button);
        // Reuse the game's setting label's immutable font styles, with a fresh
        // LabelButton input model. No shared host parameter/backend binding.
        c.copy(c.field(control,0x58c9d354),c.address(c.field(setting,0x58c9d354)));
        const auto donor=record_model(c,"Throwdown_DetailsMenu_ContentResource/ThrowdownDetails_HUDButton",button);
        for(const auto field:{0x8cc042efU,0xf9515817U,0xf343d160U})
            c.copy(c.field(control,field),c.address(c.field(donor,field)));
        clear_value(c,c.field(control,0x67212c85));
        c.set(c.path(control,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/TileButton.RoughPartial.Default"));
        c.set(slot,Widget{asset(c,"UI/Foundations/Components/Buttons/LabelButton/LabelButton_Widget"),{0,control.handle}});
    } else control=reference(c,read<Widget>(c.address(slot)).data);
    plain_label(c,c.field(control,0x58c9d354),value);
    const auto set_action=[&](Value model) {
        const auto input_root=c.path(model,{0x0fb0d794,0xa704272a,0xc52416ef});
        const auto interactable=c.field(input_root,0xa704272a);
        c.set(c.field(interactable,0x4b3146e9),true);c.set(c.field(interactable,0x38f7355f),false);
        clear_value(c,c.path(input_root,{0x66a997b9,0x66a997b9,0x4c09193f}));
        const auto action=c.field(interactable,0xa2b71c93);
        c.set(c.field(action,0x9c76b86c),callback(c,command,argument));c.text(c.field(action,0xb1e14cd6),"");
        const auto primary_input=asset(c,"Configurations/Input/UI/UI_PrimaryAction");
        c.set(c.field(action,0x3d00de27),primary_input);c.set(c.field(action,0x0169c2db),primary_input);
    };
    set_action(control);
    // Native HostParameters items use ContentPresenterTile for controller
    // selection. Keep its authored template and replace its private action.
    for(const auto tile:ui_models(c,row,content_tile_schema.hash)) {
        set_action(tile);
        c.set(c.path(tile,{0x0fb0d794,0xa704272a,0xc52416ef,0x0abf7c31}),Ref{0,control.handle});
    }
    } catch(...) {
        if(fresh) {
            rows.extra.erase(key);
            state().owned.rollback(checkpoint,[&](Value model){c.destroy(model);});
        }
        throw;
    }
}
void native_setup_capabilities(const Context& c,Value panel,Value body_value,const throwdowns::Definition& definition,
        const modes::NativeView& mode) {
    auto& s=state();auto [found,fresh]=s.official_setup_rows.try_emplace(panel.handle);
    if(fresh) {
        const auto checkpoint=s.owned.checkpoint();
        try {found->second=native_setup_rows(c,body_value);}
        catch(...) {
            s.official_setup_rows.erase(found);
            s.owned.rollback(checkpoint,[&](Value model){c.destroy(model);});
            throw;
        }
    }
    auto& rows=found->second;std::vector<Value> extra;
    if(mode.settings.mode==modes::Mode::hide)
        extra.push_back(native_setup_extra(c,rows,"hide-timer","Hide Timer"));
    if(!definition.needs_points) {
        native_setup_action_row(c,rows,"area","Area",mode.settings.corners.empty()?"Set optional area":"Change area",
            "mode-setup-area",std::to_string(mode.game));
        extra.push_back(rows.extra.at("area"));
    } else if(mode.settings.mode==modes::Mode::domination) {
        native_setup_action_row(c,rows,"capture-spots","Capture Spots",
            mode.settings.points.empty()?"Place spots":"Add spots","mode-setup-area",std::to_string(mode.game)+":points");
        extra.push_back(rows.extra.at("capture-spots"));
    }
    auto offer_mode=definition.name;
    std::transform(offer_mode.begin(),offer_mode.end(),offer_mode.begin(),[](unsigned char ch){return static_cast<char>(std::toupper(ch));});
    for(const auto& offer:modes::hud().offers)if(!offer.own && offer.open && offer.mode==offer_mode) {
        const auto key="join-"+std::to_string(offer.id);
        native_setup_action_row(c,rows,key,"Join",offer.host,"mode-setup-join",std::to_string(mode.game)+":"+std::to_string(offer.id));
        extra.push_back(rows.extra.at(key));
        if(extra.size()>=8)break;
    }
    // Preserve the native list's layout, scroll/focus state and item template;
    // publish its existing row representation, never an alternate details page.
    if(rows.owned_tiles) {
        unsigned saved_count{},saved_stride{};
        auto bytes=c.array(rows.retained_rows,128,saved_count,saved_stride);
        require(saved_stride==content_tile_schema.size,"Retained native setup tile schema differs.");
        for(const auto row:extra) {
            const auto at=bytes.size();bytes.resize(at+content_tile_schema.size);
            require(memory::peek_bytes(c.address(row),bytes.data()+at,content_tile_schema.size),"Private native setup tile is unavailable.");
        }
        const auto owned=c.field(rows.list,0x345b75b1);unsigned count{},stride{};
        const auto live=c.array(owned,128,count,stride);
        require(stride==content_tile_schema.size,"Private native setup tile schema differs.");
        const auto offset=c.member(c.type(content_tile_schema),widget).offset;
        // The native list updates focus/interaction fields in these inline
        // tiles. Remount only their Widget identities with the same item count.
        if(const auto next=remount_presenters<Widget>(live,bytes,stride,offset))
            c.array(owned,*next,static_cast<unsigned>(bytes.size()/stride));
    } else if(rows.stack) {
        unsigned saved_count{},saved_stride{};
        auto bytes=c.array(rows.retained_rows,128,saved_count,saved_stride);
        require(saved_stride==stack_item.size,"Retained native setup StackItem schema differs.");
        for(const auto row:extra) {
            const auto at=bytes.size();bytes.resize(at+stack_item.size);
            require(memory::peek_bytes(c.address(row),bytes.data()+at,stack_item.size),"Private native setup StackItem is unavailable.");
        }
        unsigned count{},stride{};const auto live=c.array(c.field(rows.list,items),128,count,stride);
        require(stride==stack_item.size,"Private native setup StackItem schema differs.");
        const auto offset=c.member(c.type(stack_item),content).offset;
        if(const auto next=remount_presenters<Widget>(live,bytes,stack_item.size,offset))
            c.array(c.field(rows.list,items),*next,static_cast<unsigned>(bytes.size()/stack_item.size));
    } else if(!rows.records.empty()) {
        auto records=rows.records;for(const auto row:extra)records.push_back({0,row.handle});
        unsigned count{},stride{};const auto live=c.array(c.field(rows.list,rows_field),128,count,stride);
        if(live!=std::vector<std::byte>(std::as_bytes(std::span(records)).begin(),std::as_bytes(std::span(records)).end()))c.array(c.field(rows.list,rows_field),records);
    } else {
        auto handles=rows.handles;for(const auto row:extra)handles.push_back(row.handle);
        unsigned count{},stride{};const auto live=c.array(c.field(rows.list,0xf2c90867),128,count,stride);
        if(live!=std::vector<std::byte>(std::as_bytes(std::span(handles)).begin(),std::as_bytes(std::span(handles)).end()))c.array(c.field(rows.list,0xf2c90867),handles);
    }
}
void official_setup(const Context& c) {
    auto& s=state();
    if(!one_up::native_setup_active()) {
        restore_official(c);
        s.setup_open.store(false); return;
    }
    if(s.reset_setup.exchange(false))s.seconds=20;
    const auto race_game=one_up::native_setup_mode_game();
    const auto race=modes::native_view();
    if(race_game && (!race.active || race.game!=race_game))return;
    const auto* definition=race_game?throwdowns::registry().mode(static_cast<unsigned>(race.settings.mode)):nullptr;
    if(race_game && !definition)return;
    const bool deathrace=definition && race.settings.mode==modes::Mode::race;
    const bool hide_timer=definition && race.settings.mode==modes::Mode::hide;
    if(s.official_race!=race_game) {
        restore_official(c);s.official_race=race_game;
        if(race_game) {s.mode_capacity=race.capacity;s.mode_seconds=race.settings.duration_s;s.mode_hide_seconds=race.settings.turn_s;}
    }
    s.setup_open.store(true);
    for(const auto& root:c.roots({panel_schema.hash})) {
      try {
        if(root.model.handle==s.panel.handle)continue;
        const auto header_slot=c.field(root.model,0xc0f1b449);
        auto header=reference(c,read<Widget>(c.address(header_slot)).data);
        if(!header.handle || read<std::uint32_t>(read<Address>(header.type))!=header_schema.hash)continue;
        auto title=c.field(header,0x00f3b15e);
        const auto name=c.text(c.field(title,text_field));
        if(!s.official_panels.contains(root.model.handle) && name!="Spot Battle" && name!="SPOT BATTLE" && name!="ID_ACTIVITY_SPOTBATTLE_TITLE")continue;
        const bool fresh=!s.official_panels.contains(root.model.handle);
        if(fresh) {
            // Header/footer mount before the native body. Do not retain that
            // temporary empty body and then overwrite its populated controls.
            bool has_players{},has_timer{};
            for(const auto setting:ui_models(c,c.field(root.model,0x43967ece),setting_schema.hash)) {
                const auto id=c.text(c.path(setting,{0x58c9d354,text_field}));
                const auto role=native_setup_role(id);
                has_players=has_players || role==NativeSetupRole::players;
                has_timer=has_timer || role==NativeSetupRole::timer;
            }
            if(!has_players || !has_timer)continue;
            // Finish the private graph before changing any live presenter. A
            // missing native dependency must leave the stock panel intact and
            // must not mark a half-built panel as a successful 1-Up adaptation.
            PrivateUiGraph graph{c};
            std::vector<std::pair<Value,Value>> copies;
            for(const auto key:{0xc0f1b449U,0x43967eceU,0xbfc64535U}) {
                const auto slot=c.field(root.model,key);
                const auto copy=graph.clone(key==0x43967eceU?Ref{0,slot.handle}:read<Widget>(c.address(slot)).data);
                if(key==0x43967eceU)remove_one_up_rounds(c,copy);
                copies.emplace_back(slot,copy);
                if(key==0xc0f1b449U)header=copy;
            }
            for(const auto& [slot,copy]:copies)remember_official(c,slot);
            try {
                for(const auto& [slot,copy]:copies) {
                    if(slot.type==copy.type)mount_setup_body(c,slot,copy);
                    else {
                        auto current=read<Widget>(c.address(slot));current.data={0,copy.handle};c.set(slot,current);
                    }
                    s.official_contents.emplace(slot.handle,copy);
                    s.official_slot_panels.emplace(slot.handle,root.model.handle);
                }
                s.official_headers.emplace(root.model.handle,header);
                s.official_panels.insert(root.model.handle);
            } catch(...) {
                for(const auto& [slot,copy]:copies) {
                    try {
                        const auto saved=s.official_originals.at(slot.handle).second;
                        if(slot.type==copy.type)mount_setup_body(c,slot,saved);
                        else c.copy(slot,c.address(saved));
                    }catch(...) {}
                    s.official_contents.erase(slot.handle);s.official_slot_panels.erase(slot.handle);
                    s.official_originals.erase(slot.handle);
                }
                s.official_headers.erase(root.model.handle);s.official_panels.erase(root.model.handle);
                throw;
            }
            title=c.field(header,0x00f3b15e);
        } else if(const auto saved=s.official_headers.find(root.model.handle);saved!=s.official_headers.end() && header.handle!=saved->second.handle) {
            auto mounted=read<Widget>(c.address(header_slot));mounted.data={0,saved->second.handle};
            c.set(header_slot,mounted);header=saved->second;title=c.field(header,0x00f3b15e);
        }
        if(fresh) logging::log(logging::Level::info,logging::Channel::ui,
            "1-Up: using original native Throwdown setup {:#x}.",root.model.handle);
        // Native page reentry can republish Spot Battle's presenters. Keep our
        // slider models while preserving the live stack's animation and focus.
        const auto body_slot=c.field(root.model,0x43967ece);
        const auto private_body=s.official_contents.at(body_slot.handle);
        mount_setup_body(c,body_slot,private_body);
        for(const auto key:{0xbfc64535U}) {
            const auto slot=c.field(root.model,key);const auto copy=s.official_contents.at(slot.handle);
            auto current=read<Widget>(c.address(slot));
            if(current.data.handle!=copy.handle) {current.data={0,copy.handle};c.set(slot,current);}
        }
        plain_label(c,title,definition?definition->name:"1-UP");
        // The blue art is a RimeTexture, while ActivityDisplay's framed icon
        // expects a RimeImage. Use the native Texture widget for this slot;
        // optional art must never block the title or native controls.
        try {
            const auto blueprint=asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget");
            auto& badge=race_game?s.official_race_badge:s.official_badge;
            if(!badge.handle) {
                badge=make(c,texture_schema);
                c.set(c.field(badge,0xf524c836),288.f);c.set(c.field(badge,0x6fbd254e),288.f);
                c.set(c.field(badge,0x81c94d8a),false);c.set(c.field(badge,0xd1997d0c),false);
                c.set(c.field(badge,0x190e039c),false);
            }
            mode_art(c,std::array{c.field(badge,0x6e469d2a)},definition?definition->logo:"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
            const auto icon=c.field(header,content);
            const auto current=read<Widget>(c.address(icon));
            if(current.data.handle!=badge.handle)c.set(icon,Widget{blueprint,{0,badge.handle}});
        } catch(const std::exception& e) {
            if(fresh)logging::log(logging::Level::warning,logging::Channel::ui,"1-Up optional setup badge: {}",e.what());
        }
        if(definition && !deathrace) {
            native_setup_capabilities(c,root.model,private_body,*definition,race);
            mount_setup_body(c,body_slot,private_body);
        }
        const auto settings=ui_models(c,c.field(root.model,0x43967ece),setting_schema.hash);
        const auto notices=ui_models(c,c.field(root.model,0x43967ece),notice_schema.hash);
        const auto buttons=ui_models(c,c.field(root.model,0xbfc64535),button.hash);
        for(const auto setting:settings) {
            const auto label=c.field(setting,0x58c9d354);
            const auto id=c.text(c.field(label,text_field));
            const auto role=native_setup_role(id);
            const bool players=role==NativeSetupRole::players;
            const bool hiding=hide_timer && role==NativeSetupRole::hide_timer;
            const bool turn=role==NativeSetupRole::timer || hiding;
            if(!players && !turn)continue;
            const auto presentation=c.field(setting,content);
            auto slider=reference(c,read<Widget>(c.address(presentation)).data);
            if(!slider.handle || read<std::uint32_t>(read<Address>(slider.type))!=0xcdc749b2)continue;
            const bool first=!s.official_sliders.contains(presentation.handle);
            if(first) {
                // The native widget still handles input, focus and rendering.
                // Its shared Spot Battle setting must not receive 1-Up values.
                clear_value(c,c.field(slider,2243478502U));
                clear_value(c,c.field(slider,655725074U));
                s.official_sliders.emplace(presentation.handle,slider);
            } else {
                const auto saved=s.official_sliders.at(presentation.handle);
                if(slider.handle!=saved.handle) {
                    auto mounted=read<Widget>(c.address(presentation));mounted.data={0,saved.handle};c.set(presentation,mounted);
                }
                slider=saved;
            }
            const auto range=c.field(slider,0x170bebf7), value=c.field(slider,0x5a547e87);
            const auto step=c.field(slider,0xb47e6cec);
            const float minimum=players?1.f:deathrace?30.f:race_game && !hiding?30.f:10.f,
                        maximum=players?6.f:deathrace?3600.f:race_game && !hiding?3600.f:120.f, increment=players?1.f:deathrace?30.f:hiding?5.f:10.f;
            if(read<std::array<float,2>>(c.address(range))!=std::array<float,2>{minimum,maximum})
                c.set(range,std::array<float,2>{minimum,maximum});
            if(read<float>(c.address(step))!=increment)c.set(step,increment);
            c.set(c.field(slider,264787447U),true); // TrimWholeValues
            c.set(c.field(slider,3516482810U),true); // RoundToNearestWhole
            if(first)c.set(value,native_slider_position(players?(race_game?s.mode_capacity:s.max_players):(race_game?(hiding?s.mode_hide_seconds:s.mode_seconds):s.seconds),unsigned(minimum),unsigned(maximum)));
            const auto normalized=read<float>(c.address(value));
            const unsigned selected=native_slider_choice(normalized,unsigned(minimum),unsigned(maximum),unsigned(increment));
            const auto snapped=native_slider_position(selected,unsigned(minimum),unsigned(maximum));
            if(!std::isfinite(normalized) || std::abs(normalized-snapped)>.00001f)c.set(value,snapped);
            if(race_game) {
                auto& chosen=players?s.mode_capacity:hiding?s.mode_hide_seconds:s.mode_seconds;
                if(chosen!=selected) {chosen=selected;modes::command(players?"players":hiding?"turn":"time",{std::to_string(selected)});}
                if(!players)plain_label(c,label,deathrace?"Finish deadline":hiding?"Hide Timer":hide_timer?"Duration":"Timer");
            } else if(players)s.max_players=selected;else s.seconds=selected;
        }
        one_up::set_native_setup_options(race_game?30:s.seconds,race_game?s.mode_capacity:s.max_players);
        for(const auto notice:notices) {
            const auto label=c.field(notice,0x94703efc);
            plain_label(c,label,definition?definition->description:"Add up every landed trick before time runs out.\nBeat the previous total. Three penalties and you're out!");
        }
        // Keep the native Confirm input and its location initializer. The
        // placement observer hands off to the flag as soon as it completes.
        for(const auto b:buttons) {
            const auto action=c.path(b,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
            const auto nav=c.text(c.field(action,0xb1e14cd6));
            if(nav=="ThrowdownPlacement" || nav=="Throwdown_SKATE_Start")continue;
            if(nav!="AdvancedParameters")continue;
            c.text(c.field(action,0xb1e14cd6),"ThrowdownerExit");
            const auto label=c.field(b,0x58c9d354);plain_label(c,label,"BACK");
        }
        // All modes, including a saved Deathrace route, retain the authored
        // Confirm -> native start flag -> waiting-for-players flow. Starting
        // requires the matching waiting registration's hold-Start action.
      } catch(const std::exception& e) {
          static std::uint64_t report{};const auto now=GetTickCount64();
          if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"Throwdown setup candidate {:#x}: {}",root.model.handle,e.what());}
      }
    }
}
void stock_solo_controls(const Context& c) {
    auto& s=state();
    for(const auto& root:c.roots({panel_schema.hash})) {
        if(s.official_panels.contains(root.model.handle) || root.model.handle==s.panel.handle)continue;
        const auto header=reference(c,read<Widget>(c.address(c.field(root.model,0xc0f1b449))).data);
        if(!header.handle || read<std::uint32_t>(read<Address>(header.type))!=header_schema.hash)continue;
        const auto title=c.text(c.path(header,{0x00f3b15e,text_field}));
        const bool stock=title=="S.K.A.T.E." || title=="ID_ACTIVITY_SKATE_TITLE" ||
            title=="Spot Battle" || title=="SPOT BATTLE" || title=="ID_ACTIVITY_SPOTBATTLE_TITLE" ||
            title=="Skate Jam" || title=="SKATE JAM" || title=="ID_ACTIVITY_JAMSESSION_TITLE";
        if(!stock)continue;
        for(const auto setting:ui_models(c,c.field(root.model,0x43967ece),setting_schema.hash)) {
            if(c.text(c.path(setting,{0x58c9d354,text_field}))!="ID_SETTINGS_PLAYERS")continue;
            const auto slot=c.field(setting,content);
            auto slider=reference(c,read<Widget>(c.address(slot)).data);
            if(!slider.handle || read<std::uint32_t>(read<Address>(slider.type))!=0xcdc749b2)continue;
            if(const auto existing=s.solo_sliders.find(slot.handle);existing!=s.solo_sliders.end() && existing->second.handle==slider.handle)continue;
            const auto old_range=read<std::array<float,2>>(c.address(c.field(slider,0x170bebf7)));
            if(!std::isfinite(old_range[0]) || !std::isfinite(old_range[1]) || old_range[0]<1 || old_range[1]<2 || old_range[1]>32)continue;
            const auto maximum=static_cast<unsigned>(old_range[1]);
            const auto selected=native_slider_choice(read<float>(c.address(c.field(slider,0x5a547e87))),static_cast<unsigned>(old_range[0]),maximum,1);
            // This change is intentionally shared by all three stock modes.
            // Keep the original model identity so its authored backend binding
            // reads the same slider the native widget changes.
            c.set(c.field(slider,0x170bebf7),std::array<float,2>{1.f,old_range[1]});
            c.set(c.field(slider,0xb47e6cec),1.f);
            c.set(c.field(slider,264787447U),true);c.set(c.field(slider,3516482810U),true);
            c.set(c.field(slider,0x5a547e87),native_slider_position(selected,1,maximum));
            s.solo_sliders[slot.handle]=slider;
            logging::log(logging::Level::info,logging::Channel::ui,"Throwdowns: native {} Players slider supports 1-{}.",title,maximum);
        }
    }
}
void retire_solo_pending_queue(const Context& c) {
    if(!native_solo_throwdown_menu_cleanup_pending())return;
    unsigned found{};
    // Live capture of the locked wheel after confirmed Spot Battle solo Quit:
    // dependencies were entitlement=1, IsInQueueState=false,
    // IsPendingQueue=true, ThrowdownerDisabledByRule_RefCount=0.
    // Publish the actual dependency, rather than bypassing IsThrowdownerDisabled
    // or erasing unrelated activity/entitlement restrictions.
    for(const auto& root:c.roots({0x6ee228a8})) {
        if(c.text(c.path(root.model,{0x58c9d354,text_field}))!="ID_THROWDOWNS")continue;
        const auto dependencies=c.path(root.model,{0x67212c85,0x4c09193f,0x8a56e2ac});
        unsigned count{},stride{};c.array(dependencies,8,count,stride);
        if(count!=4 || stride!=sizeof(Ref))continue;
        const auto ref=read<Ref>(c.address(c.element(dependencies,2)));
        if(!ref.record || string(read<Address>(ref.record+0x28),128)!="TD_Page_ContentResources/IsPendingQueue")continue;
        const auto pending=reference(c,ref);
        if(!pending.handle || kind(pending.type)!=10 || size(pending.type)!=1)continue;
        const bool was_pending=read<bool>(c.address(pending));
        if(was_pending)c.set(pending,false);
        ++found;
        logging::log(logging::Level::info,logging::Channel::ui,
            "Throwdowns: confirmed solo exit published IsPendingQueue {} -> false on the native wheel (model {:#x}).",
            was_pending,pending.handle);
    }
    if(found)native_solo_throwdown_menu_cleaned();
}
void footer_action(const Context& c,Value button_value,const std::string& text,std::string command,std::string argument={}) {
    plain_label(c,c.field(button_value,0x58c9d354),text);
    auto action=c.path(button_value,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
    c.set(c.field(action,0x9c76b86c),callback(c,std::move(command),std::move(argument)));
    c.text(c.field(action,0xb1e14cd6),"");
    if(button_value.handle==state().footer_confirm.handle) {
        // PageActions is an array of native ButtonAction values. Like the
        // original Throwdown Confirm, this action is available while a
        // selector has focus; it does not require navigating into the footer.
        auto page_action=c.path(state().confirm_action,{0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
        c.copy(page_action,c.address(action));
        const auto input_asset=state().primary_input;
        c.set(c.field(page_action,0x3d00de27),input_asset);
        c.set(c.field(page_action,0x0169c2db),input_asset);
        const auto actions=c.field(state().page,0xd2118ca8);
        unsigned count{},stride{};
        c.array(actions,8,count,stride);
        require(stride==page_action_schema.size,"1-Up page action schema differs.");
        bool publish=count!=1;
        if(!publish) {
            const auto mounted=c.path(c.element(actions,0),{0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
            publish=read<Address>(c.address(c.field(mounted,0x9c76b86c)))!=read<Address>(c.address(c.field(page_action,0x9c76b86c)));
        }
        if(publish) {
            std::vector<std::byte> bytes(page_action_schema.size);
            require(memory::peek_bytes(c.address(state().confirm_action),bytes.data(),bytes.size()),"1-Up Confirm action unavailable.");
            c.array(actions,bytes,1);
        }
    }
}
void session_marker_controls(const Context& c,bool release) {
    auto& originals=state().marker_originals;
    if(release || !(one_up::restricts_session_markers() || modes::restricts_session_markers())) {
        for(const auto& [handle,saved]:originals)
            if(c.type_of(handle)==saved.first.type) c.set(saved.first,saved.second);
        originals.clear();
        return;
    }
    // SessionMarkerDataModel is the game's keyboard/menu binding. The
    // skater-intent predicates separately guard controller marker actions.
    for(const auto& root:c.roots({4200505362U}))
        for(const auto [hash,disabled]:std::array{
                std::pair{2627953652U,true},std::pair{753092917U,false},std::pair{1976179319U,false}}) {
            const auto field=c.field(root.model,hash);
            require(size(field.type)==sizeof(bool),"Native session marker field ABI differs.");
            const auto current=read<bool>(c.address(field));
            originals.try_emplace(field.handle,std::pair{field,current});
            if(current!=disabled)c.set(field,disabled);
        }
}
void waiting_hud(const Context& c) {
    auto& s=state(); const auto v=one_up::view();
    const auto stock_revision=native_throwdown_queue_revision();
    if(s.waiting_stock_revision!=stock_revision) {
        // Native queues can reuse the same notification model and HUD slot.
        // A real creation/join, rather than a timeout, releases suppression.
        for(const auto& [handle,presentation]:s.waiting_presentations) {
            const auto at=c.type_of(handle)==presentation.first.type?
                game::native_data().models.value(c.manager,handle,0,1):0;
            if(!at)continue;
            const auto current=read<Widget>(at).data.handle;
            const auto original=read<Widget>(c.address(presentation.second));
            const auto copy=s.waiting_originals.find(original.data.handle);
            if(copy!=s.waiting_originals.end() && owns_waiting_presentation(current,copy->second.handle,s.waiting_hidden_slots.contains(handle)))
                c.copy(presentation.first,c.address(presentation.second));
        }
        s.waiting_retired_notifications.clear();s.waiting_widgets.clear();s.waiting_originals.clear();
        s.waiting_presentations.clear();s.waiting_counts.clear();s.waiting_hidden_slots.clear();
        s.waiting_stock_revision=stock_revision;
    }
    const auto mode=modes::native_view();
    const auto flag_game=one_up::native_flag_mode_game();
    const bool generic=flag_game && mode.active && mode.game==flag_game;
    const auto* definition=generic?throwdowns::registry().mode(static_cast<unsigned>(mode.settings.mode)):nullptr;
    const bool registration_owned=one_up::flag_registration_owned();
    const bool retiring=one_up::flag_registration_retiring() ||
        (!generic && !registration_owned && (!v.state.match || v.state.phase==one_up::Phase::cancelled));
    const bool awaiting_lobby=!generic && registration_owned && !v.state.match;
    const bool lobby=!retiring && (generic?mode.waiting:awaiting_lobby || v.state.phase==one_up::Phase::lobby);
    // Keep retired custom slots hidden until the game removes/replaces them.
    // A backend teardown can lag the custom match by several UI ticks.
    // The game's queue creates and places this SecondaryNotification itself.
    // Keep its layout, transitions and input prompts; only supply 1-Up data.
    // The live HUD is in the gameplay asset domain, which need not be a parent
    // of the card's domain. Resolve its registered type directly.
    constexpr std::uint32_t hash=273922540U; // NotificationViewModel
    const auto count=generic?(mode.state?modes::lobby_ready_count(*mode.state):(mode.leading?1U:0U)):awaiting_lobby?1:
        std::count_if(v.state.players.begin(),v.state.players.end(),[](const auto& p){return p.connected && p.ready;});
    const auto joined=generic?(mode.state?std::max<std::size_t>(1,mode.state->standings.size()):1U):0U;
    const auto selected=one_up::native_flag_options();
    const auto capacity=generic?mode.capacity:awaiting_lobby?selected.players:v.state.config.max_players;
    const std::string name=definition?definition->name:"1-UP";
    std::string message;
    if(generic && !mode.waiting_status.empty())message=mode.waiting_status;
    else if(lobby && generic) message=std::to_string(count)+"/"+std::to_string(joined)+" ready"+
        (mode.leading?"  |  Host starts":"  |  Waiting for host");
    else if(lobby) message="Waiting for players  "+std::to_string(count)+" / "+std::to_string(capacity)+" ready";
    else if(generic) message=mode.waiting_status;
    else if(v.state.phase==one_up::Phase::countdown) message="Starting in "+std::to_string((v.remaining+999)/1000);
    else if(v.state.phase==one_up::Phase::finished) message=v.name(v.state.winner)+" wins!";
    else message=v.name(v.state.active)+"  |  Target "+std::to_string(static_cast<unsigned>(v.state.target));
    // Authored Notification defaults and other challenge queues are registered
    // roots too. Only adapt a notification actually mounted in the HUD stack.
    std::map<Handle,std::vector<Value>> mounted_notifications;
    for(const auto& root:c.roots({0xa842b4d0})) {
        const auto notifications=c.path(root.model,{0xd9f424d1,items});
        unsigned n{},stride{};c.array(notifications,128,n,stride);
        for(unsigned i=0;i<n;++i) {
            const auto presentation=c.field(c.element(notifications,i),content);
            const auto mounted=mounted_notification(presentation.handle,[&](Handle handle) {
                const Value slot{handle,c.type_of(handle)};
                const auto saved=s.waiting_presentations.find(handle);
                auto current=read<Widget>(c.address(slot));
                if(saved!=s.waiting_presentations.end()) {
                    const auto original=read<Widget>(c.address(saved->second.second));
                    const auto copy=s.waiting_originals.find(original.data.handle);
                    if(copy!=s.waiting_originals.end() && owns_waiting_presentation(
                            current.data.handle,copy->second.handle,s.waiting_hidden_slots.contains(handle)))
                        current=original;
                    else {
                        // Native remount: do not suppress a newly selected stock
                        // Spot Battle merely because it reused this HUD slot.
                        // Several mounted presenters can share this source.
                        // Retire this slot only; the other slots still use the
                        // private notification and participant model.
                        s.waiting_hidden_slots.erase(handle);
                        s.waiting_presentations.erase(saved);
                    }
                }
                current.data.handle=waiting_notification_source(current.data.handle,s.waiting_originals);
                const auto type=current.data.handle?c.type_of(current.data.handle):0;
                if(!type)return NotificationMountNode{};
                const auto hash=read<std::uint32_t>(read<Address>(type));
                const auto inner=hash==0xffb83e48?c.field({current.data.handle,type},content).handle:0;
                return NotificationMountNode{current.data.handle,hash,inner};
            });
            if(mounted) {
                auto& slots=mounted_notifications[mounted->model];
                if(std::none_of(slots.begin(),slots.end(),[&](const auto& slot){return slot.handle==mounted->presentation;}))
                    slots.push_back({mounted->presentation,c.type_of(mounted->presentation)});
            }
        }
    }
    const auto candidates=c.roots({hash});
    if(s.waiting_widgets.empty() && GetTickCount64()>=s.waiting_diagnostic) {
        s.waiting_diagnostic=GetTickCount64()+10000;
        logging::log(logging::Level::info,logging::Channel::ui,"1-Up: looking for native queue HUD type {:#x}, {} root models, {} mounted notifications.",hash,candidates.size(),mounted_notifications.size());
    }
    // The mounted graph is authoritative even when navigation changes the
    // source's root registration. Never depend on a second registry scan to
    // decide whether an already mounted private card needs fresh controls.
    for(const auto& [source_handle,presentations]:mounted_notifications) {
        const Value source{source_handle,c.type_of(source_handle)};
        if(!source.type || read<std::uint32_t>(read<Address>(source.type))!=hash)continue;
        // Save the native presenter before clone_widget_tree publishes its
        // replacement. The map can remount this same source in another slot.
        const auto save_presentations=[&] {
            for(const auto& presentation:presentations)if(!s.waiting_presentations.contains(presentation.handle)) {
                const auto original=make(c,presenter,presentation.type);
                auto native=read<Widget>(c.address(presentation));
                native.data={0,source.handle};
                c.set(original,native);
                s.waiting_presentations.emplace(presentation.handle,std::pair{presentation,original});
            }
        };
        if(!s.waiting_widgets.contains(source.handle)) {
            if(!registration_owned && !(retiring && s.waiting_retired_notifications.contains(source.handle)))continue;
            const auto title=c.text(c.path(source,{0x1c1d36ea,0x44688629,text_field}));
            if(title!="Spot Battle" && title!="SPOT BATTLE" && title!="ID_ACTIVITY_SPOTBATTLE_TITLE")continue;
            if(c.text(c.path(source,{0xd31e2918,0x58c9d354,text_field}))!="ID_START" ||
               c.text(c.path(source,{0xc13bcfcd,0x58c9d354,text_field}))!="ID_LEAVE")continue;
            save_presentations();
            const auto copy=clone_widget_tree(c,presentations.front());
            s.waiting_originals.emplace(source.handle,copy);
            s.waiting_widgets.insert(source.handle);
            logging::log(logging::Level::info,logging::Channel::ui,"1-Up: private native queue HUD bound, source {:#x}, model {:#x}.",source.handle,copy.handle);
        }
        save_presentations();
        if(retiring) {s.waiting_retired_notifications.insert(source.handle);continue;}
        const auto model=s.waiting_originals.at(source.handle);
        const auto start=c.field(model,0xd31e2918),leave=c.field(model,0xc13bcfcd);
        clear_value(c,c.field(start,0x67212c85));
        const auto heading=c.field(model,0x1c1d36ea);
        plain_label(c,c.field(heading,0x44688629),name);
        plain_label(c,c.field(heading,0x00f3b15e),message);
        // Start/Leave must remain usable even if optional icon loading is late.
        // Keep the delegate identity stable when this native card is reused
        // for Deathrace -> 1-UP. Its live flag selects the rules at dispatch;
        // a retired/hidden card or a real Spot Battle fails the owner check.
        footer_action(c,leave,"LEAVE","flag-wait-leave",std::to_string(source.handle));
        footer_action(c,start,lobby?(generic && !mode.leading?"READY":"START"):name,"flag-wait-start",std::to_string(source.handle));
        if(!s.waiting_badge.handle) {
            s.waiting_badge=make(c,texture_schema);
            c.set(c.field(s.waiting_badge,0xf524c836),96.f);
            c.set(c.field(s.waiting_badge,0x6fbd254e),96.f);
        }
        mode_art(c,std::array{c.field(s.waiting_badge,0x6e469d2a)},definition?definition->logo:"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
        c.set(c.field(heading,content),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,s.waiting_badge.handle}});
        // The stock queue's decorator displays its backend limit (10). Keep
        // the native green participant icon but bind a private count model to
        // the actual 1-Up roster and selected player limit.
        auto& participants=s.waiting_counts[source.handle];
        const auto decorator=c.field(model,0xe01931be);
        if(!participants.handle)participants=clone_widget_model(c,decorator);
        auto participant_widget=read<Widget>(c.address(decorator));
        if(participant_widget.data.handle!=participants.handle) {
            participant_widget.data={0,participants.handle};
            c.set(decorator,participant_widget);
        }
        const auto connected=generic || awaiting_lobby?count:std::count_if(v.state.players.begin(),v.state.players.end(),[](const auto& p){return p.connected;});
        plain_label(c,c.field(participants,0x58c9d354),generic?std::to_string(count)+"/"+std::to_string(joined)+" ready":
            std::to_string(connected)+"/"+std::to_string(capacity));
        // CountdownViewModel explicitly supports an outside time source when
        // DoCountdown is false. Keep the authored timer artwork/animation and
        // feed the synchronized match clock, never the native S.K.A.T.E. clock.
        if(!s.waiting_countdown.handle) {
            const auto original=read<Ref>(c.address(c.field(model,0x0750d51d)));
            const auto timer_type=original.handle?c.type_of(original.handle):(original.record?native_record_type(c,original.record):0);
            require(timer_type!=0,"Native waiting timer is not ready.");
            const auto data=original.handle?c.address({original.handle,timer_type}):read<Address>(original.record+0x20);
            s.waiting_countdown=make(c,{1783008579U,static_cast<std::uint16_t>(size(timer_type))},timer_type);
            c.copy(s.waiting_countdown,data);
        }
        const bool ticking=!generic && v.state.phase==one_up::Phase::countdown;
        const auto timer=s.waiting_countdown;
        c.set(c.field(timer,1883000211U),false); // DoCountdown: externally driven
        c.set(c.field(timer,376517051U),ticking); // ForceActive
        c.set(c.field(timer,1636171836U),ticking); // Running
        c.set(c.field(timer,3239084782U),true); // UpdateText
        native_timer_seconds(c,timer,1288039561U,v.state.config.countdown_ms);
        native_timer_seconds(c,timer,247769280U,ticking?v.remaining:0);
        c.text(c.field(timer,378491941U),ticking?std::to_string((v.remaining+999)/1000):"");
        c.set(c.field(model,0x0750d51d),Ref{0,timer.handle});
    }
    // The registration keeps the world flag alive. Its large waiting card is
    // only needed in the lobby. The native Throwdown Panel owns Quit during
    // play, so hide this presentation without popping its backend queue.
    const bool compact=!lobby;
    for(const auto& [notification,presentations]:mounted_notifications) {
        if(!s.waiting_widgets.contains(notification))continue;
        const auto model=s.waiting_originals.at(notification);
        for(const auto& presentation:presentations) {
            const auto saved=s.waiting_presentations.find(presentation.handle);
            if(saved==s.waiting_presentations.end())continue;
            const auto current=read<Widget>(c.address(presentation));
            auto replacement=read<Widget>(c.address(saved->second.second));
            replacement.data={0,model.handle};
            if(compact) {replacement={};s.waiting_hidden_slots.insert(presentation.handle);}
            else s.waiting_hidden_slots.erase(presentation.handle);
            if(current.blueprint!=replacement.blueprint || current.data.handle!=replacement.data.handle)c.set(presentation,replacement);
        }
    }
}
void mount_hud(const Context& c,Value model,Address blueprint,Value& anchor,Value& item,int id,bool center) {
    auto& s=state();
    const auto roots=c.roots({0xe4c44873}); // HudViewModel, including authored defaults
    std::vector<HudRootCandidate> candidates;
    for(const auto& root:roots) {
        const auto stack=c.field(root.model,0x44736c13); // FullAspectRatioStack
        const bool active=read<bool>(c.address(c.field(stack,0x369babfe)));
        unsigned populated{};
        // Never let 1-Up's own appended items make an unmounted template look
        // live. These are independent, game-owned gameplay HUD stacks.
        for(const auto field:{0xa7b6b40fU,0xee58682eU,0x0f3c11c2U}) {
            unsigned count{},stride{};
            c.array(c.path(root.model,{field,items}),64,count,stride);
            populated+=count!=0;
        }
        candidates.push_back({root.model.handle,active,populated});
    }
    const auto selected=mounted_hud_root(candidates,s.hud_root.handle);
    require(selected!=0,"Native gameplay HUD is not mounted.");
    if(selected!=s.hud_root.handle || !s.hud_stack.handle) {
        publish_hud_items(c,false,false); // Detach from a retired root before rebinding.
        const auto root=std::find_if(roots.begin(),roots.end(),[&](const auto& r){return r.model.handle==selected;});
        s.hud_root=root->model;
        s.hud_stack=c.field(s.hud_root,0x44736c13);
        logging::log(logging::Level::info,logging::Channel::ui,
            "1-Up: attached to live gameplay HUD {:#x}; authored defaults excluded.",selected);
    }
        auto& slot=hud_surface_slot(c,s.hud_stack,id);
        if(anchor.handle==slot.anchor.handle && item.handle==slot.item.handle)return;
        const auto next_anchor=slot.anchor;
        slot.display=Widget{blueprint,{0,model.handle}};
        c.set(c.field(next_anchor,widget),Widget{});
        c.set(c.field(next_anchor,0x1cff7243),std::array<float,2>{center?700.f:1100.f,center?480.f:900.f});
        c.set(c.field(next_anchor,0x144aee01),false);
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool vertical=axis==0x185a5736U;
            const auto layout=c.field(next_anchor,axis);
            const float at=center?(vertical?.38f:.5f):(vertical?native_hud_screen_anchor(.008f):native_hud_screen_anchor(.006f));
            const float offset=!center && !vertical?16.f:0.f;
            c.set(c.field(layout,0x99bbed5a),at);c.set(c.field(layout,0x49943bed),at);
            c.set(c.field(layout,0x40bfbff4),offset);c.set(c.field(layout,0x1d233266),offset);
            c.set(c.field(layout,0xfec5e0b1),center?.5f:0.f);c.set(c.field(layout,0xd01f4a21),1.f);
        }
        const auto next_item=slot.item;
        anchor=next_anchor;item=next_item;
}
void native_countdown_hud(const Context& c) {
    auto& s=state();const auto v=one_up::view();const auto now=GetTickCount64();
    const bool countdown=v.state.match && v.state.phase==one_up::Phase::countdown && !v.positioning;
    const one_up::Token token{v.state.match,v.state.turn,v.state.active};
    const bool fresh_countdown=countdown && s.hud_countdown_token!=token;
    // The first half announces the mode using the native Throwdown banner;
    // the final three seconds use TurnStartCountdownHUDRule's actual widget.
    s.hud_intro_visible=countdown && v.remaining>3000;
    if(countdown && !s.hud_intro_visible)s.hud_go_until=now+900;
    const bool show_go=v.state.match && v.state.phase==one_up::Phase::playing &&
        s.hud_countdown_token==token && now<s.hud_go_until;
    s.hud_countdown_visible=(countdown && !s.hud_intro_visible) || show_go;
    if(s.hud_intro_visible) {
        if(!s.hud_intro.handle) {
            s.hud_intro=record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner");
            c.set_if_changed(c.field(s.hud_intro,0xf79313be),record_ref(c,"ActivityBannerStylesList/ActivityBannerCenterFullWidthStyle.Throwdown.Default"));
            const auto heading=c.field(s.hud_intro,0xcbc57608);
            const auto authored=record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner_ContentWithTitle");
            c.copy(heading,c.address(authored));
            plain_label(c,c.field(heading,0x44688629),"1-UP");
            label_styles(c,c.field(heading,0x44688629),"TextStylesList/D180-Caps_White","TextStylesList/D180-Caps_White");
            plain_label(c,c.field(heading,0x00f3b15e),"Beat the target. Last skater standing wins!");
            s.hud_intro_badge=make(c,texture_schema);
            c.set_if_changed(c.field(s.hud_intro_badge,0xf524c836),256.f);
            c.set_if_changed(c.field(s.hud_intro_badge,0x6fbd254e),256.f);
            c.set_if_changed(c.field(heading,content),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,s.hud_intro_badge.handle}});
        }
        publish_texture(c,std::array{c.field(s.hud_intro_badge,0x6e469d2a)},"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
        mount_hud(c,s.hud_intro,asset(c,"UI/Features/Activities/Blueprints/ActivityBanner_CenterFullWidth_Widget"),s.hud_intro_anchor,s.hud_intro_item,hud_intro_id,true);
        c.set_if_changed(c.field(s.hud_intro_anchor,0x1cff7243),std::array<float,2>{1920.f,600.f});
    }
    if(s.hud_countdown_visible) {
        if(!s.hud_timer.handle)s.hud_timer=record_model(c,"Activities_HudElementData_ContentResource/Activities_CountdownItemHUD");
        // ActivityCountdownViewModel drives the stock grunge/scribble
        // transitions. CountdownViewModel belongs to the small queue timer
        // and is not the model consumed by the game's turn-start animation.
        plain_label(c,c.field(s.hud_timer,0x58c9d354),show_go?"GO!":std::to_string(native_hud_seconds(v.remaining)));
        c.set_if_changed(c.field(s.hud_timer,1698918111U),show_go?0.f:std::clamp(static_cast<float>(v.remaining)/3000.f,0.f,1.f));
        mount_hud(c,s.hud_timer,asset(c,"UI/Features/Activities/Widgets/System/ActivityCountdown_Widget"),s.hud_timer_anchor,s.hud_timer_item,hud_countdown_id,true);
        c.set_if_changed(c.field(s.hud_timer_anchor,0x1cff7243),std::array<float,2>{512.f,512.f});
    }
    if(fresh_countdown) {
        s.hud_countdown_token=token;
        logging::write(logging::Level::info,logging::Channel::ui,"1-Up: native Throwdown intro and ActivityCountdown 3-2-1/GO sequence mounted.");
    }
}
void native_score_hud(const Context& c) {
    auto& s=state();const auto v=one_up::view();const auto now=GetTickCount64();
    const bool ended=v.state.phase==one_up::Phase::finished || v.state.phase==one_up::Phase::cancelled;
    if(!ended)s.hud_ended_at=0;else if(!s.hud_ended_at)s.hud_ended_at=now;
    const bool show=v.state.match && v.state.phase!=one_up::Phase::lobby &&
        (!ended || now-s.hud_ended_at<4000);
    if(!show) {s.hud_ready.store(false);return;}
    const auto label_widget=asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget");
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const auto notice_widget=asset(c,"UI/ReSkate/Throwdowns/OneUpScore_Widget");
    float board_width=496.f;
    for(const auto& player:v.state.players)board_width=std::max(board_width,float(native_hud_name_width(v.name(player.id)))+268.f);
    constexpr float row_height=76.f,row_pitch=84.f;
    constexpr float score_height=224.f;
    const float height=score_height+row_pitch*static_cast<float>(v.state.players.size());
    const auto place=[&](Value anchor,float width,float h,float x,float y,float pivot_x,float pivot_y,float scale=1.f) {
        c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{width,h});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool vertical=axis==0x185a5736U;const auto layout=c.field(anchor,axis);
            c.set_if_changed(c.field(layout,0x99bbed5a),vertical?y:x);c.set_if_changed(c.field(layout,0x49943bed),vertical?y:x);
            c.set_if_changed(c.field(layout,0x40bfbff4),0.f);c.set_if_changed(c.field(layout,0x1d233266),0.f);
            c.set_if_changed(c.field(layout,0xfec5e0b1),vertical?pivot_y:pivot_x);
            c.set_if_changed(c.field(layout,0xd01f4a21),scale);
        }
    };
    const bool rebuild=!s.hud_initialized || !hud_list_has_items(c,s.hud_content,3) || s.hud_player_ids.size()!=v.state.players.size();
    if(rebuild) {
        if(!s.hud_title.handle) {
            s.hud_title=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
            label_styles(c,s.hud_title,"TextStylesList/D94-Caps-Bold_White","TextStylesList/D94-Caps-Bold_White");
            s.hud_badge=make(c,texture_schema);
            c.set_if_changed(c.field(s.hud_badge,0xf524c836),144.f);c.set_if_changed(c.field(s.hud_badge,0x6fbd254e),144.f);
            c.set_if_changed(c.field(s.hud_badge,0xd1997d0c),false);
            c.set_if_changed(c.field(s.hud_badge,0xcd4a53c0),record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite"));
            s.hud_board=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
            label_styles(c,s.hud_board,"TextStylesList/SP38-SemiB_White","TextStylesList/SP38-SemiB_White");
        }
        // Each counter owns a measured native torn strip. The donor's crown /
        // small-purple-counter combination has independent offsets and cannot
        // share a leaderboard row's bounds.
        for(auto* counter:{&s.hud_target,&s.hud_current})if(!counter->handle) {
            *counter=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
            c.set_if_changed(c.field(*counter,0x13be0d51),std::array<float,2>{480.f,72.f});
            const auto label=c.field(*counter,0x94703efc);
            label_styles(c,label,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
            c.set_if_changed(c.field(label,0x3d8639d0),0);c.set_if_changed(c.field(label,0x87ad624c),2);
            c.set_if_changed(c.field(label,0xc13ba637),false);
        }
        if(!s.hud_notice.handle)s.hud_notice=make(c,linear_list);
        hud_list(c,s.hud_notice,{{notice_widget,{0,s.hud_target.handle}},{notice_widget,{0,s.hud_current.handle}}},false,72.f,8.f,0.f,480.f);
        if(!s.hud_clock.handle)s.hud_clock=record_model(c,"Activities_HudElementData_ContentResource/A_ActivityCountdown");
        if(!s.hud_rows.handle)s.hud_rows=make(c,linear_list);
        hud_list(c,s.hud_content,{{label_widget,{0,s.hud_board.handle}},{list_widget,{0,s.hud_notice.handle}},
            {list_widget,{0,s.hud_rows.handle}}},false,48.f,12.f,row_pitch*static_cast<float>(v.state.players.size()),board_width);
        const auto counter_anchor=read<Handle>(c.address(c.element(c.field(s.hud_content,0xf2c90867),1)));
        c.set_if_changed(c.field({counter_anchor,c.type_of(counter_anchor)},0x1cff7243),std::array<float,2>{board_width,152.f});
        s.hud_initialized=true;
    }
    const auto content_cells=c.field(s.hud_content,0xf2c90867);
    for(unsigned i=0;i<3;++i){const auto handle=read<Handle>(c.address(c.element(content_cells,i)));
        c.set_if_changed(c.field({handle,c.type_of(handle)},0x1cff7243),std::array<float,2>{board_width,i==0?48.f:i==1?152.f:row_pitch*static_cast<float>(v.state.players.size())});}
    mount_hud(c,s.hud_content,list_widget,s.hud_anchor,s.hud_item,hud_score_id,false);
    place(s.hud_anchor,board_width,height,native_hud_screen_anchor(.994f),native_hud_screen_anchor(native_hud_side_center),1.f,.5f,1.f);
    s.hud_heading.title=s.hud_title;s.hud_heading.badge=s.hud_badge;
    challenge_header(c,s.hud_heading,"oneup","1-UP","UI/ReSkate/OneUp/img_OneUp_Blue_1024",{},hud_title_id,hud_badge_id);
    s.hud_title_anchor=s.hud_heading.frame_anchor;s.hud_title_item=s.hud_heading.frame_item;
    s.hud_badge_anchor=s.hud_heading.anchor;s.hud_badge_item=s.hud_heading.item;
    mount_hud(c,s.hud_clock,asset(c,"UI/Foundations/Components/Countdown/Countdown_Widget"),s.hud_clock_anchor,s.hud_clock_item,hud_clock_id,false);
    place(s.hud_clock_anchor,150.f,64.f,.5f,native_hud_screen_anchor(.988f),.5f,1.f,1.f);
    const auto points=[](double n){return std::to_string(static_cast<std::uint32_t>(std::max(0.0,n)));};
    plain_label(c,s.hud_title,"1-UP");
    plain_label(c,s.hud_board,ended?(v.state.winner?v.name(v.state.winner)+" WINS!":"MATCH COMPLETE"):v.name(v.state.active)+"'S TURN");
    plain_label(c,c.field(s.hud_target,0x94703efc),"TARGET   "+points(v.state.target));
    plain_label(c,c.field(s.hud_current,0x94703efc),"SCORE   "+points(v.state.best));
    c.set_if_changed(c.field(s.hud_clock,0x703c4d93),false);
    c.set_if_changed(c.field(s.hud_clock,0x167131bb),!ended);c.set_if_changed(c.field(s.hud_clock,0x6186003c),!ended);
    c.set_if_changed(c.field(s.hud_clock,0xc11082ee),true);
    native_timer_seconds(c,s.hud_clock,0x4cc5ec89,v.state.config.turn_ms);
    native_timer_seconds(c,s.hud_clock,0x0ec4a8c0,v.state.phase==one_up::Phase::playing?v.remaining:0);
    std::vector<std::uint64_t> ids;
    std::vector<Widget> rows;
    for(const auto& player:v.state.players) {
        auto& hud=s.hud_players.try_emplace(player.id).first->second;
        if(!hud.row.handle) {
            hud.row=make(c,linear_list);
            hud.background=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
            c.set_if_changed(c.field(hud.background,0x13be0d51),std::array<float,2>{620.f,row_height});
            hud.name=c.field(hud.background,0x94703efc);
            c.set_if_changed(c.field(hud.name,0x87ad624c),2);c.set_if_changed(c.field(hud.name,0x3d8639d0),2);
            c.set_if_changed(c.field(hud.name,0xc13ba637),false);c.set_if_changed(c.field(hud.name,0xfc53d427),false);
            unsigned index{};
            for(const auto* letter:{"1","U","P"}) {
                auto& stamp=hud.letters[index++];
                stamp=record_model(c,"Activities_HudElementData_ContentResource/Activity_HUD_CurrentScore_Notice");
                c.set_if_changed(c.field(stamp,0x13be0d51),std::array<float,2>{row_height,row_height});
                const auto label=c.field(stamp,0x94703efc);plain_label(c,label,letter);
                label_styles(c,label,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
                c.set_if_changed(c.field(label,0x87ad624c),2);c.set_if_changed(c.field(label,0x3d8639d0),2);
                c.set_if_changed(c.field(label,0xc13ba637),false);
            }

        }
        const float name_width=float(native_hud_name_width(v.name(player.id)));
        for(const auto stamp:hud.letters)match_panel_notice_width(c,stamp,row_height-32.f);
        c.set_if_changed(c.field(hud.background,0x13be0d51),std::array<float,2>{name_width,row_height});
        if(!hud_list_has_items(c,hud.row,4)) {
            std::vector<Widget> cells{{asset(c,"UI/ReSkate/Throwdowns/OneUpName_Widget"),{0,hud.background.handle}}};
            for(const auto& stamp:hud.letters)cells.push_back({asset(c,"UI/Foundations/Components/Notices/SmallNotice_Widget"),{0,stamp.handle}});
            hud_list(c,hud.row,cells,true,row_height,8.f,0.f,board_width,name_width);
            for(unsigned col=0;col<4;++col) {
                const auto ref=read<Handle>(c.address(c.element(c.field(hud.row,0xf2c90867),col)));
                const Value anchor{ref,c.type_of(ref)};
                c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{col==0?name_width:row_height,row_height});
                for(const auto key:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(anchor,{0x185a5736,key}),0.f);
            }
        }
        plain_label(c,hud.name,v.name(player.id));hud.out=!player.eligible();
        label_styles(c,hud.name,hud.out?"TextStylesList/H52-XBold_Grey500":"TextStylesList/H52-XBold_White",
            hud.out?"TextStylesList/H52-XBold_Grey500":"TextStylesList/H52-XBold_White");
        const auto name_art=std::string("UI/ReSkate/Throwdowns/")+(hud.out?"OneUpEliminated_":"OneUpName_")+std::to_string(static_cast<unsigned>(name_width))+"_Widget";
        const auto name_blueprint=asset(c,name_art.c_str());
        const auto name_anchor=read<Handle>(c.address(c.element(c.field(hud.row,0xf2c90867),0)));
        c.set_if_changed(c.field({name_anchor,c.type_of(name_anchor)},0x1cff7243),std::array<float,2>{name_width,row_height});
        const auto name_slot=c.field({name_anchor,c.type_of(name_anchor)},widget);
        if(read<Widget>(c.address(name_slot)).blueprint!=name_blueprint)
            c.set_if_changed(name_slot,Widget{name_blueprint,{0,hud.background.handle}});
        for(unsigned letter=0;letter<3;++letter) {
            const auto style=record_ref(c,earned_one_up_letter(player.penalties,letter)?
                "NoticeStylesList/SmallNotice_ControlSelection_Focus":"NoticeStylesList/SmallNotice_ControlSelection_Default");
            c.set_if_changed(c.field(hud.letters[letter],0x8cc042ef),style);c.set_if_changed(c.field(hud.letters[letter],0xa1e84eb1),style);
        }
        ids.push_back(player.id);rows.push_back({list_widget,{0,hud.row.handle}});
    }
    if(ids!=s.hud_player_ids || !hud_list_has_items(c,s.hud_rows,static_cast<unsigned>(rows.size()))) {
        hud_list(c,s.hud_rows,rows,false,row_height,8.f,0.f,board_width);s.hud_player_ids=std::move(ids);
        for(unsigned row=0;row<rows.size();++row) {
            const auto ref=read<Handle>(c.address(c.element(c.field(s.hud_rows,0xf2c90867),row)));
            const Value anchor{ref,c.type_of(ref)};
            c.set_if_changed(c.field(anchor,0x6efc1a61),false);c.set_if_changed(c.field(anchor,0xbe5683d9),false);
        }
    }
    if(!s.hud_ready.exchange(true))logging::write(logging::Level::info,logging::Channel::ui,"1-Up: logo, brush heading, target/score strips, centered standings and clock mounted.");
}
void native_results_hud(const Context& c) {
    auto& s=state();const auto v=one_up::view();const auto now=GetTickCount64();
    if(s.results_match!=v.state.match || v.state.phase==one_up::Phase::lobby){s.results_match=v.state.match;s.eliminated_at.clear();}
    for(const auto& player:v.state.players)if(!player.eligible())s.eliminated_at.try_emplace(player.id,v.state.turn);
    const bool practice=v.solo_test && v.state.phase==one_up::Phase::cancelled && std::any_of(v.state.players.begin(),v.state.players.end(),[](const auto& p){return p.penalties==3;});
    const bool finished=v.state.match && (v.state.phase==one_up::Phase::finished || practice);
    if(!s.oneup_result_window.visible(v.state.match,finished,now) || modes::native_view().active){s.hud_results_visible=false;return;}
    s.hud_results_visible=true;
    const auto* definition=throwdowns::registry().find("native.oneup");if(!definition)return;
    modes::NativeView view;view.active=true;view.local=v.local;view.game=static_cast<std::uint32_t>(v.state.match);view.settings.mode=modes::Mode::one_up;
    view.state.emplace();view.state->phase=modes::Phase::results;
    overlay::ModesHud feed;feed.active=true;feed.results=true;feed.winner=v.state.winner?v.name(v.state.winner):std::string{};
    auto players=v.state.players;const auto winner=v.state.winner;
    std::stable_sort(players.begin(),players.end(),[&](const auto& a,const auto& b){
        if(a.id==winner || b.id==winner)return a.id==winner && b.id!=winner;
        const auto x=s.eliminated_at.find(a.id),y=s.eliminated_at.find(b.id);
        return (x==s.eliminated_at.end()?UINT32_MAX:x->second)>(y==s.eliminated_at.end()?UINT32_MAX:y->second);
    });
    for(const auto& p:players){modes::Standing row;row.player=p.id;row.out=false;view.state->standings.push_back(row);
        overlay::ModesHudRow label;label.name=v.name(p.id);label.value=p.id==winner?"WINNER":!p.connected?"LEFT":p.penalties>=3?"1 U P":"ACTIVE";feed.rows.push_back(std::move(label));}
    auto& h=s.oneup_result_hud;h.stack=s.hud_stack;h.game=view.game;
    mode_results(c,view,feed,*definition,&h);
    // Reuse the working measured results frame, never the legacy backend table.
    plain_label(c,c.field(h.results_captions[2],0x94703efc),"1-UP");
    if(practice)plain_label(c,c.field(h.results_notice,0x94703efc),"PRACTICE COMPLETE");
    const bool ready=hud_publication_allowed(read<bool>(c.address(c.field(s.hud_stack,0x369babfe))),s.in_menu,s.match_panel_open.load());
    if(!h.publication.allow(s.hud_stack.handle,v.state.match,ready,now)){s.hud_results_visible=true;return;}
    const std::array result_items{h.results_panel_item,h.results_item};
    publish_hud_surface(c,s.hud_stack,result_items,true);s.hud_results_visible=true;
}

#include "one_up_match_panel.inl"
#if !defined(DINGOSDK_LEAN_PLAYTEST)
void trace_official_panel_models(const Context& c,std::uint64_t now) noexcept {
    try {
        const auto mode=modes::native_view();
        const auto& lease=match_panel_lease(c);
        logging::log(logging::Level::info,logging::Channel::ui,
            "PanelTrace UI snapshot: tick={}, manager={:#x}, in_menu={}, custom_panel={}, requested={}, oneup={}, mode_game={}, mode_active={}, native_activity={}, phase={}, countdown_lock={}, closing={}, panel_page={:#x}, hud_root={:#x}, hud_stack={:#x}, race_game={}.",
            now,c.manager,state().in_menu,state().match_panel_open.load(),state().match_panel.requested_until,
            one_up::view().state.match,mode.game,mode.active,local_throwdown_active(),
            mode.state?static_cast<int>(mode.state->phase):-1,
            one_up::countdown_locks_input() || modes::countdown_locks_input(),lease.closing.pending(),
            state().match_panel.page.handle,state().hud_root.handle,state().hud_stack.handle,state().mode_hud.game);
        for(const auto& root:c.roots({0x1cead64a,0xcc4776b5,0xaf4c67d7})) {
            const auto hash=read<std::uint32_t>(read<Address>(root.model.type));
            std::string values;
            if(hash==0x1cead64a)for(const auto value:read<std::array<std::int32_t,5>>(c.address(root.model)))values+=std::format(" {}",value);
            else if(hash==0xaf4c67d7)values=std::to_string(read<std::int32_t>(c.address(c.field(root.model,0x88391637))));
            else {
                values=std::format(" action={:#x}",read<std::uint32_t>(c.address(c.field(root.model,0xf3ba9cf8))));
                unsigned count{},stride{};const auto bytes=c.array(c.field(root.model,0xf437b255),32,count,stride);
                if(stride==4)for(unsigned i=0;i<count;++i) {std::int32_t value{};std::memcpy(&value,bytes.data()+i*4,4);values+=std::format(" {}",value);}
            }
            logging::log(logging::Level::info,logging::Channel::ui,"PanelTrace navigation: tick={}, schema={:#x}, root={:#x};{}.",now,hash,root.model.handle,values);
        }
        unsigned captured{};
        for(const auto& root:c.roots({0xd71ee0a1})) {
            if(captured>=12)break;
            if(!read<bool>(c.address(c.field(root.model,0x369babfe))))continue;
            const auto item=active_match_panel_layer_item(c,root.model);if(!item.handle)continue;
            const auto display=read<Widget>(c.address(c.field(item,content)));
            if(!display.data.handle || !display.blueprint)continue;
            const auto type=c.type_of(display.data.handle);if(type!=c.type(page))continue;
            const Value page_model{display.data.handle,type};const auto action=back(c,page_model);
            ++captured;
            logging::log(logging::Level::info,logging::Channel::ui,
                "PanelTrace native Page: tick={}, stack={:#x}, page={:#x}, item={}, blueprint={:#x}, Back={}, delegate={:#x}.",
                now,root.model.handle,page_model.handle,read<int>(c.address(c.field(item,0x1e95752c))),display.blueprint,
                c.text(c.field(action,0xb1e14cd6)),read<Address>(c.address(c.field(action,0x9c76b86c))));
        }
        const auto view_manager=read<Address>(c.base+0x75cbc48);
        if(view_manager && read<Address>(view_manager)==c.base+0x63ece68) {
            const auto first=read<Address>(view_manager+8),last=read<Address>(view_manager+16);
            if(last>=first && last-first<=32*8)for(auto at=first;at<last;at+=8) {
                const auto view=read<Address>(at);
                if(!view || read<Address>(view)!=c.base+0x63ed0a0 || read<unsigned>(view+0x148) || read<unsigned>(view+0x14c))continue;
                const auto camera=read<Address>(view+0x2b0);if(!camera)continue;
                const auto pose=read<std::array<float,16>>(camera+0x50);
                logging::log(logging::Level::info,logging::Channel::ui,
                    "PanelTrace camera: tick={}, view={:#x}, camera={:#x}, vtable={:#x}, origin=({},{},{}), forward=({},{},{}), FOV={}.",
                    now,view,camera,read<Address>(camera)-c.base,pose[12],pose[13],pose[14],pose[8],pose[9],pose[10],read<float>(camera+0xac));
            }
        }
    } catch(const std::exception& e) {logging::log(logging::Level::info,logging::Channel::ui,"PanelTrace snapshot unavailable: {}.",e.what());}
}
#endif
void gameplay_hud(const Context& c,std::uint64_t now) {
    auto& s=state();
    // Retire expired countdown/intro before any optional HUD builder can fail.
    try {mode_hud_transients(c,modes::native_view(),now);}
    catch(const std::exception&) { /* Retired native parents are retried safely. */ }
    if(s.in_menu || s.match_panel_open.load()) {
        suspend_hud_surface_content(c);
        s.hud_publication.allow(s.hud_stack.handle,one_up::view().state.match,false,now);
        s.mode_hud.publication.allow(s.mode_hud.stack.handle,s.mode_hud.game,false,now);
        try {native_match_panel(c,now);}
        catch(const std::exception& e) {
            if(s.match_panel.requested_until && now>=s.match_panel.requested_until) {
                try {restore_match_panel(c);}catch(...) {}
                s.match_panel.requested_until=0;s.match_panel.request_after=0;
            }
            static std::uint64_t report{};
            if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"Native challenge menu transition: {}",e.what());}
        }
        return;
    }
    try {throwdown_map_identity(c);}
    catch(const std::exception& e) {
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"Custom Throwdown map identity: {}",e.what());}
    }
    try {waiting_hud(c);}
    catch(const std::exception& e) {
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native waiting HUD: {}",e.what());}
    }
    try {native_countdown_hud(c);}
    catch(const std::exception& e) {
        s.hud_countdown_visible=false;s.hud_intro_visible=false;
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native countdown HUD: {}",e.what());}
    }
    try {native_score_hud(c);}
    catch(const std::exception& e) {
        // A transient missing native presenter must not erase the last valid
        // gameplay frame. The next tick retries mounting; normal lobby/end
        // handling and world teardown still remove every owned HUD item.
        const auto match=one_up::view();
        if(!match.state.match || match.state.phase==one_up::Phase::lobby) {
            s.hud_ready.store(false);
        }
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native gameplay HUD: {}",e.what());}
    }
    try {native_results_hud(c);}
    catch(const std::exception&){ /* Keep the owned result frame during a transient asset retry. */ }
    try {if(!s.hud_results_visible)mode_hud(c);}
    catch(const std::exception& e) {
        // A transition can temporarily hide the native parent or delay an
        // optional asset. Keep the last complete frame while the challenge
        // still owns it; tearing it down here produces a double flash.
        if(!modes::native_view().active)try {mode_hud_publish(c,false);}catch(...) {}
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"Throwdown mode HUD: {}",e.what());}
    }
    try {native_match_panel(c,now);}
    catch(const std::exception& e) {
        try {restore_match_panel(c);}catch(...) {}
        s.match_panel.retry_after=now+5000;
        static std::uint64_t report{};
        if(now>=report) {report=now+10000;logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native Throwdown Panel: {}",e.what());}
    }
    // The panel prompt can be created after score/countdown published their
    // items. Reconcile now so it is visible on the first gameplay frame.
    publish_hud_items(c,!s.hud_results_visible && s.hud_ready.load(),
        !s.hud_results_visible && s.hud_countdown_visible,!s.hud_results_visible && s.hud_intro_visible,s.hud_results_visible);
}
void render(const Context& c) {
    auto& s=state(); const auto v=one_up::view(); std::vector<std::string> shown{"rules"};
    const auto note=[&](const char* id,std::string text){row(c,shown,id,text);};
    const auto action=[&](const char* id,std::string text,std::string command,std::string argument={}){row(c,shown,id,text,std::move(command),std::move(argument));};
    footer_action(c,s.footer_back,"BACK","back");
    if(!v.state.match) {
        setting_row(c,shown,"session-type","Session Type","1-Up");
        setting_row(c,shown,"players-setting","Players","",true);
        setting_row(c,shown,"turn-setting","Turn","",true);
        penalty_row(c,shown);
        footer_action(c,s.footer_confirm,s.place_until?"CONNECTING...":"READY UP","place");
        for(const auto& offer:v.offers) action(("offer-"+std::to_string(offer.leader)).c_str(),std::string(offer.started?"Watch ":"Join ")+offer.name+"'s 1-Up","oneup","join "+std::to_string(offer.leader)+" "+std::to_string(offer.match));
    } else {
        if(v.state.phase==one_up::Phase::lobby) {
            const auto ready_count=std::count_if(v.state.players.begin(),v.state.players.end(),[](const auto& p){return p.ready && p.connected;});
            setting_row(c,shown,"session-type","Session Type","1-Up");
            setting_row(c,shown,"lobby-players","Players",std::to_string(v.state.players.size())+" / "+std::to_string(v.state.config.max_players));
            setting_row(c,shown,"lobby-duration","Duration",std::to_string(v.state.config.turn_ms/1000)+" sec");
            penalty_row(c,shown);
            const auto me=std::find_if(v.state.players.begin(),v.state.players.end(),[&](const auto& p){return p.id==v.local;});
            const bool ready=me!=v.state.players.end() && me->ready;
            if(v.state.leader==v.local && ready) footer_action(c,s.footer_confirm,"START 1-UP","oneup","start");
            else if(v.participant) footer_action(c,s.footer_confirm,ready?"READY":"READY UP","oneup","ready");
            else footer_action(c,s.footer_confirm,"JOIN","oneup","enroll");
            note("lobby-help",ready_count<2?"Waiting for skaters to ready up.":"Everyone is ready!");
        }
        else if(v.state.phase==one_up::Phase::finished) {
            note("winner",v.name(v.state.winner)+" WINS!");
            if(v.state.leader==v.local) footer_action(c,s.footer_confirm,"REMATCH","oneup","rematch");
            else footer_action(c,s.footer_confirm,"EXIT 1-UP","oneup","leave");
        } else {
            setting_row(c,shown,"score-setting","Target",std::to_string(static_cast<std::uint32_t>(v.state.target)));
            setting_row(c,shown,"best-setting","Total",std::to_string(static_cast<std::uint32_t>(v.state.best)));
            if(v.state.active) setting_row(c,shown,"active-setting","Up Next",v.name(v.state.active));
            note("notice",v.state.notice);
            footer_action(c,s.footer_confirm,"EXIT 1-UP","oneup","leave");
        }
        footer_action(c,s.footer_back,"EXIT","oneup","leave");
    }
    if(!v.status.empty() && v.status!="You left 1-Up." && !v.status.starts_with("Spot placed.") && !v.status.starts_with("Flag placed.")) note("status",v.status);
    if(!v.state.match && !one_up::flag_placement_status().empty()) note("placement-status",one_up::flag_placement_status());
    if(!s.setup_status.empty() && !v.state.match) note("setup-status",s.setup_status);
    // A primary-action event is consumed by the mounted button handler. Keep
    // the source model current too so a page reload gets the same Confirm.
    const auto mounted=read<Address>(c.address(c.field(s.footer_list,0xf2c90867)));
    const auto mounted_count=mounted?read<std::uint32_t>(mounted-4)&0x7fffffff:0;
    if(mounted_count!=2) {
        c.array(c.field(s.footer_list,rows_field),std::vector<Ref>{});
        c.array(c.field(s.footer_list,0xf2c90867),std::vector<Handle>{s.footer_back.handle,s.footer_confirm.handle});
    }
    const auto current=read<Address>(c.address(c.field(s.details_list,0xf2c90867)));
    const auto count=current?read<std::uint32_t>(current-4)&0x7fffffff:0;
    if(shown!=s.displayed || count!=shown.size()) {
        const auto old_focus=read<int>(c.address(c.field(s.details_list,0x55511280)));
        const auto focused=old_focus>=0 && static_cast<std::size_t>(old_focus)<s.displayed.size()?s.displayed[old_focus]:std::string{};
        std::vector<Handle> handles; int first=-1, preserved=-1;
        for(const auto& id:shown) {
            const auto& r=s.rows.at(id);
            if(r.callback) { if(first<0)first=static_cast<int>(handles.size()); if(id==focused)preserved=static_cast<int>(handles.size()); }
            handles.push_back(r.anchor.handle);
        }
        // These are independent collections: publishing both displays every
        // row twice. Reconcile handles after native mounting clears the list.
        c.array(c.field(s.details_list,0xf2c90867),handles);
        c.set(c.field(s.details_list,0x55511280),preserved>=0?preserved:first);
        s.displayed=std::move(shown);
    }
}
#include "throwdown_modes_ui.inl"
#include "throwdown_map_identity.inl"
} // namespace
bool native_one_up_setup_open() noexcept { return state().setup_open.load(std::memory_order_acquire); }
bool native_one_up_hud_ready() noexcept { return state().hud_ready.load(std::memory_order_acquire); }
bool native_one_up_match_panel_open() noexcept { return state().match_panel_open.load(std::memory_order_acquire); }
bool native_throwdown_gameplay_input_allowed() noexcept {
    auto& s=state();return s.gameplay_input_allowed.load(std::memory_order_acquire) && !s.match_panel_open.load(std::memory_order_acquire);
}
bool native_one_up_match_active() noexcept { try {return match_panel_view().available;}catch(...) {return false;} }
void prepare_native_one_up_match_panel() noexcept {
    // Native ModelWriteLock is recursive for the same thread (1933460);
    // completion can therefore also run inside an SDK model publication.
    static thread_local bool preparing{};
    if(preparing)return;
    auto& s=state();const auto now=GetTickCount64();
    if(!s.manager || !s.base || !s.match_panel.page.handle || !pending_match_panel_request(Context{s.base,s.manager},now))return;
    preparing=true;
    struct Reset {bool& value;~Reset(){value=false;}} reset{preparing};
    try {
        const auto view=match_panel_view();
        if(!view.available || view.open_command+":"+view.argument!=s.match_panel.match_key)return;
        const auto ui=read<Address>(s.base+build::engine::ui_manager);
        if(!ui || read<Address>(ui+0x140)!=s.manager)return;
        const Context c{s.base,s.manager};game::ModelWriteLock lock{s.manager};
        prepare_match_panel_body(c,now);
    } catch(const std::exception& e) {
        logging::log(logging::Level::warning,logging::Channel::ui,
            "Throwdowns: native Details presentation preparation deferred: {}",e.what());
    }
}
bool release_native_one_up_menu(std::uintptr_t base) noexcept {
    overlay::capture_challenge_menu_input(false);
    sync_custom_activity_ui_presence(false);
    auto& s=state(); if(s.owned.empty()) { s.owner.store(0); s.setup_open.store(false); return true; }
    try { const auto ui=read<Address>(base+build::engine::ui_manager); const auto manager=ui?read<Address>(ui+0x140):0;
        require(manager!=0,"1-Up UI manager unavailable during cleanup.");
        if(manager!=s.manager) { s.owned.manager_replaced(); s.hud_active_budget.manager_replaced(); s.hud_surfaces.clear(); s.picker_grid={};s.picker_anchor={};s.oneup_logo={};s.picker_stock={};s.picker_stock_logos={}; s.mode_cards.clear();s.selected_mode.clear();s.mode_hud={};s.map_branding={};s.details_list={};s.marker_originals.clear(); clear_hud_state(); s.official_originals.clear(); s.official_panels.clear(); s.official_headers.clear(); s.official_contents.clear(); s.official_slot_panels.clear(); s.official_sliders.clear();s.official_setup_rows.clear();s.official_page_actions.clear(); s.solo_sliders.clear(); s.waiting_originals.clear(); s.waiting_widgets.clear(); s.waiting_backups.clear(); s.waiting_presentations.clear(); s.waiting_counts.clear(); s.waiting_retired_notifications.clear(); s.waiting_hidden_slots.clear(); s.waiting_stock_revision=0; s.waiting_label={}; s.waiting_countdown={}; s.waiting_badge={}; s.waiting_diagnostic=0; s.owner.store(0); s.setup_open.store(false); s.page={}; s.cards={}; s.panel={}; s.badge={}; s.official_badge={};s.official_race_badge={};s.official_race=0; s.duration_selector={}; s.originals.clear(); s.rows.clear(); s.assets.clear(); s.displayed.clear(); s.generations.clear(); s.details=false; return true; }
        const Context c(base,manager); game::ModelWriteLock lock(manager); release(c); s.generations.clear(); return true;
    } catch(const std::exception& e) { logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native menu cleanup: {}",e.what()); return false; }
}
void tick_native_one_up_menu(std::uintptr_t base,bool loading,bool in_menu) noexcept {
    state().gameplay_input_allowed.store(!loading && !in_menu,std::memory_order_release);
    auto& s=state();if(s.in_menu!=in_menu)s.next_tick=0;s.in_menu=in_menu;
    const auto now=GetTickCount64();
    try {
    // Poll at the caller's client tick rate, not the 200ms UI refresh rate.
    // This snapshot is normalized XInput for every supported controller and
    // contains only keyboard state from the foreground game window.
    const auto panel_view=match_panel_view();
    // Waiting/placement retains the registration's native controls. Only
    // competing gameplay or an explicit panel transition borrows UI presence.
    sync_custom_activity_ui_presence(!loading && panel_view.available &&
        (one_up::restricts_session_markers() || modes::restricts_session_markers() ||
         s.match_panel.requested_until || s.match_panel_open.load()));
    ControllerInput panel_input;
    DingoSDKOverlayReadControllerInput(&panel_input,true);
    // ControllerInput.keys contains bindable gameplay keys, which excludes
    // Escape. PromptKeys explicitly samples Escape from the foreground game.
    const bool escape=(DingoSDKOverlayReadPromptKeys() & 8u)!=0;
    s.panel_key_down=(panel_input.buttons & 0x0010u)!=0 || escape;
#if !defined(DINGOSDK_LEAN_PLAYTEST)
    tick_official_panel_trace(s.panel_key_down,now);
    static std::uint64_t next_panel_trace{};
    if((official_panel_trace_active() || panel_view.available || s.match_panel_open.load()) &&
       !loading && now>=next_panel_trace) {
        next_panel_trace=now+(official_panel_trace_active()?100:1000);
        const auto ui=read<Address>(base+build::engine::ui_manager);
        const auto manager=ui?read<Address>(ui+0x140):0;
        if(manager) {const Context c(base,manager);game::ModelWriteLock lock(manager);trace_official_panel_models(c,now);}
    }
#endif
    DWORD foreground_process{};
    GetWindowThreadProcessId(GetForegroundWindow(),&foreground_process);
    const bool input_allowed=!loading && panel_view.available && foreground_process==GetCurrentProcessId() && !overlay::interface_open();
    overlay::capture_challenge_menu_input(!loading && panel_view.available,
        !loading && panel_view.available && s.match_panel_open.load(std::memory_order_acquire),s.panel_key_down);
    {
        std::lock_guard input_lock(s.panel_input_mutex);
        if(s.panel_start_input.sample(panel_input.buttons,escape,input_allowed,in_menu,s.match_panel_open.load())) {
            s.panel_start_request=panel_view.open_command+":"+panel_view.argument;
            s.next_tick=0;
        }
        if(!input_allowed)s.panel_start_request.clear();
    }
    if(loading)return;
    if(now<s.next_tick && !one_up::flag_placement_active()) {
        // Retry presentation at frame rate if native model publication lands
        // after its script callback. This never rebuilds/restarts HUD stacks.
        if(s.match_panel.requested_until)prepare_native_one_up_match_panel();
        // Only clocks and compass position advance between full HUD ticks.
        // Re-publishing a stack here races its visibility/layout transitions.
        const auto view=modes::native_view();
        if(view.active && view.settings.mode==modes::Mode::race &&
           view.game==s.mode_hud.game && s.mode_hud.race_rows.handle) {
            const auto ui=read<Address>(base+build::engine::ui_manager);
            const auto manager=ui?read<Address>(ui+0x140):0;
            if(manager && manager==s.manager) {
                try {
                    const Context c(base,manager);game::ModelWriteLock lock(manager);
                    if(in_menu && s.match_panel_open.load())match_panel_race_times(c,view,now);
                    if(!in_menu && s.mode_hud.stack.handle && c.type_of(s.mode_hud.stack.handle)==s.mode_hud.stack.type &&
                       hud_publication_allowed(read<bool>(c.address(c.field(s.mode_hud.stack,0x369babfe))),in_menu,
                           s.match_panel_open.load())) {
                        mode_race_clocks(c,view,now);
                        mode_gate_compass_motion(c,view,modes::hud());
                    }
                } catch(...) { /* A retiring clock must not retire the whole HUD. */ }
            }
        }
        return;
    }
    s.next_tick=now+200;
        const auto ui=read<Address>(base+build::engine::ui_manager); const auto manager=ui?read<Address>(ui+0x140):0; if(!manager)return;
        const Context c(base,manager); game::ModelWriteLock lock(manager);
        if(s.manager && s.manager!=manager) { s.owned.manager_replaced(); s.hud_active_budget.manager_replaced(); s.hud_surfaces.clear(); s.picker_grid={};s.picker_anchor={};s.oneup_logo={};s.picker_stock={};s.picker_stock_logos={}; s.mode_cards.clear();s.selected_mode.clear();s.mode_hud={};s.map_branding={};s.details_list={};s.marker_originals.clear(); clear_hud_state(); s.official_originals.clear(); s.official_panels.clear(); s.official_headers.clear(); s.official_contents.clear(); s.official_slot_panels.clear(); s.official_sliders.clear();s.official_setup_rows.clear();s.official_page_actions.clear(); s.solo_sliders.clear(); s.waiting_originals.clear(); s.waiting_widgets.clear(); s.waiting_backups.clear(); s.waiting_presentations.clear(); s.waiting_counts.clear(); s.waiting_retired_notifications.clear(); s.waiting_hidden_slots.clear(); s.waiting_stock_revision=0; s.waiting_label={}; s.waiting_countdown={}; s.waiting_badge={}; s.waiting_diagnostic=0; s.owner.store(0); s.setup_open.store(false); s.page={}; s.cards={}; s.panel={}; s.badge={}; s.official_badge={};s.official_race_badge={};s.official_race=0; s.duration_selector={}; s.originals.clear(); s.rows.clear(); s.assets.clear(); s.displayed.clear(); s.generations.clear(); s.details=false; }
        s.manager=manager; s.base=base;
        // The native menu lifetime releases models BEFORE a level unload and
        // rebuilds each new page below. Multiplayer world/session rebinding is
        // independent of level loading and must not erase a mounted picker/HUD.
        retire_solo_pending_queue(c);
        one_up::tick_flag_placement(c);
        // A native page can retire before its Quit/Start delegate is pumped.
        // Match controls belong to the match, not the picker generation.
        std::vector<Action> match_controls;
        {std::lock_guard pending_lock(s.mutex);
            for(auto it=s.pending.begin();it!=s.pending.end();) {
                if(it->command=="oneup" || it->command=="panel-quit" || it->command=="throwdown-panel" ||
                   it->command=="mode-panel-quit" || it->command=="mode-throwdown-panel" ||
                   it->command=="mode-wait-start" || it->command=="mode-wait-leave" ||
                   waiting_card_action(it->command) ||
                   it->command=="mode-setup-area" || it->command=="mode-setup-join") {
                    match_controls.push_back(*it);it=s.pending.erase(it);
                } else ++it;
            }
        }
        // Native publication can dispatch a button callback. Never hold the
        // callback queue's mutex while publishing navigation or presenters.
        for(const auto& action:match_controls) {
                if(waiting_card_action(action.command)) {
                    const auto mode=modes::native_view();
                    const auto flag_game=one_up::native_flag_mode_game();
                    const auto v=one_up::view();
                    const bool generic=flag_game && mode.active && mode.game==flag_game;
                    const bool owned=one_up::flag_registration_waiting();
                    const bool retiring=one_up::flag_registration_retiring();
                    const bool lobby=generic?mode.waiting:!flag_game && (!v.state.match || v.state.phase==one_up::Phase::lobby);
                    bool mounted{};
                    for(const auto& [source,copy]:s.waiting_originals) {
                        if(action.argument!=std::to_string(source))continue;
                        for(const auto& [handle,presentation]:s.waiting_presentations) {
                            if(c.type_of(handle)!=presentation.first.type)continue;
                            const auto at=game::native_data().models.value(c.manager,handle,0,1);
                            if(at && owns_waiting_action(source,source,read<Widget>(at).data.handle,copy.handle,owned,retiring,lobby)) {
                                mounted=true;break;
                            }
                        }
                        break;
                    }
                    if(!mounted) {
                        logging::log(logging::Level::debug,logging::Channel::ui,
                            "Throwdown waiting control rejected: card {}, confirmed {}, retiring {}, lobby {}, game {}, match {}.",
                            action.argument,owned,retiring,lobby,flag_game,v.state.match);
                        continue;
                    }
                    const bool start=action.command=="flag-wait-start";
                    logging::log(logging::Level::info,logging::Channel::ui,
                        "Throwdown waiting control: {} owns card {}, game {}, match {}.",start?"Start":"Leave",action.argument,flag_game,v.state.match);
                    if(generic) {
                        if(start) {
                            if(mode.leading)modes::request_native_start(mode.world,mode.game);
                            else modes::request_native_ready(mode.world,mode.game);
                        }
                        else modes::request_native_quit(mode.world,mode.game);
                    } else one_up::queue(start?"start":"leave");
                }
                else if(action.command=="oneup")one_up::queue(action.argument);
                else if(action.command=="mode-wait-start" || action.command=="mode-wait-leave") {
                    const auto mode=modes::native_view();
                    if(mode.active && mode.waiting && action.argument==std::to_string(mode.game)) {
                        if(action.command=="mode-wait-start") {
                            if(mode.leading)modes::request_native_start(mode.world,mode.game);
                            else modes::request_native_ready(mode.world,mode.game);
                        }
                        else modes::request_native_quit(mode.world,mode.game);
                    }
                }
                else if(action.command=="panel-quit") {
                    const auto v=one_up::view();
                    if(action.argument==std::to_string(v.state.match) && v.state.match) {
                        s.match_panel.quitting=true;one_up::queue("leave");
                        close_match_panel(c,now);
                    }
                }
                else if(action.command=="throwdown-panel") {
                    const auto v=one_up::view();
                    if(action.argument==std::to_string(v.state.match) && v.state.match && v.participant && v.state.phase!=one_up::Phase::lobby && v.state.phase!=one_up::Phase::cancelled) {
                        s.match_panel.requested_until=now+4000;
                        s.match_panel.request_after=now;
                    }
                }
                else if(action.command=="mode-setup-area" || action.command=="mode-setup-join") {
                    const auto v=modes::native_view();
                    const auto split=action.argument.find(':');
                    const auto requested_game=action.argument.substr(0,split);
                    if(v.active && v.leading && requested_game==std::to_string(v.game) &&
                       (!v.state || v.state->phase==modes::Phase::setup) && one_up::close_race_setup_for_start(v.game)) {
                        s.mode_start_game=v.game;s.mode_start_until=now+15000;
                        s.mode_setup_command=action.command=="mode-setup-area"?"place":"join";
                        s.mode_setup_argument=action.command=="mode-setup-area"?(split!=std::string::npos?action.argument.substr(split+1):"circle"):
                            action.command=="mode-setup-join" && split!=std::string::npos?action.argument.substr(split+1):std::string{};
                    }
                }
                else if(action.command=="mode-panel-quit" || action.command=="mode-throwdown-panel") {
                    const auto v=modes::native_view();
                    const bool participant=v.state && std::any_of(v.state->standings.begin(),v.state->standings.end(),
                        [&](const auto& row){return row.player==v.local;});
                    if(v.active && v.state && v.state->phase!=modes::Phase::setup && participant &&
                       action.argument==std::to_string(v.game)) {
                        if(action.command=="mode-panel-quit") {
                            s.match_panel.quitting=true;modes::command("quit",{});
                            close_match_panel(c,now);
                        } else {
                            s.match_panel.requested_until=now+4000;s.match_panel.request_after=now;

                        }
                    }
                }
        }
        if(s.mode_start_game) {
            const auto v=modes::native_view();
            if(now>=s.mode_start_until || !v.active || v.game!=s.mode_start_game ||
               (v.state && v.state->phase!=modes::Phase::setup))s.mode_start_game=0;
            else if(!one_up::race_flag_registration_owned() && !one_up::mode_flag_registration_owned()) {
                if(s.mode_setup_command=="join") {
                    // The native page reserved our unstarted draft. Discard
                    // that draft before joining the offer selected on the page.
                    const auto offers=modes::hud().offers;
                    const bool available=std::any_of(offers.begin(),offers.end(),[&](const auto& offer) {
                        return !offer.own && offer.open && std::to_string(offer.id)==s.mode_setup_argument;
                    });
                    if(!available)s.mode_status="error: that challenge is no longer accepting players.";
                    else {
                        s.mode_status=modes::command("stop",{});
                        if(!s.mode_status.starts_with("error:"))s.mode_status=modes::command("join",{s.mode_setup_argument});
                    }
                } else if(s.mode_setup_command=="place")s.mode_status=modes::command("place",{s.mode_setup_argument});
                logging::log(logging::Level::info,logging::Channel::ui,"Throwdown native setup action: {}",s.mode_status);
                s.mode_start_game=0;s.mode_setup_command.clear();s.mode_setup_argument.clear();
            }
        }
        ++s.pass;
        session_marker_controls(c);
        if(s.page.handle && c.type_of(s.page.handle)!=s.page.type) release(c,true);
        // The game creates new page/list generations when Throwdowns reopens.
        // Find fresh instances rather than leaving the card on a retired page.
        for(const auto& p:c.roots({page.hash})) if(const auto list=card_list(c,p.model)) {
            if(s.page.handle && (p.model.handle!=s.page.handle || list->handle!=s.cards.handle) && !s.generations.contains({p.model.handle,list->handle})) release(c,true);
            if(!s.page.handle && !s.generations.contains({p.model.handle,list->handle})) { initialize(c,p.model,*list); break; }
        }
        if(!s.page.handle) {
            // The native page is retired when skating resumes. Gameplay HUDs
            // must keep updating without a Throwdowns menu generation.
            if(!s.owner.load())s.owner.store(manager);
            // Native navigation retires the card page before setup mounts.
            // Continue adapting the independent native panel in that interval.
            official_setup(c);
            stock_solo_controls(c);
            std::vector<Action> pending;
            {std::lock_guard pending_lock(s.mutex);pending=std::exchange(s.pending,{});}
            for(const auto& a:pending)if(a.generation==s.owner.load() && a.command=="oneup")one_up::queue(a.argument);
            gameplay_hud(c,now);
            return;
        }
        if(one_up::flag_lobby_requested() && one_up::view().state.match) {
            one_up::flag_lobby_shown(); // Keep the native waiting-for-skaters HUD.
        }
        // Keep the retained models and delegates through empty native page
        // transitions. Rebuilding/releasing them here disables custom tiles
        // and imposes the exception retry delay on every subsequent reopen.
        if(!mounted_body(c,s.page)) {
            official_setup(c);stock_solo_controls(c);gameplay_hud(c,now);return;
        }
        mode_art(c,std::array{c.field(s.category,0x189084da),c.field(s.category,0xc2917efc)},"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
        if(s.badge.handle && s.selected_mode.empty())mode_art(c,std::array{c.field(s.badge,0x6e469d2a)},"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
        std::vector<Action> pending; {std::lock_guard pending_lock(s.mutex); pending=std::exchange(s.pending,{});}
        // Keep the card's callback reserved even while its setup is displayed.
        callback(c,"open");
        mode_cards(c);
        logo_picker(c);
        for(const auto& a:pending) if(a.generation==s.owner.load()) {
            if(a.command.starts_with("mode-")) {mode_action(c,a);continue;}
            logging::log(logging::Level::debug,logging::Channel::ui,"1-Up native menu action: {}.",a.command);
            if(a.command=="open") {
                // Navigation already ran through the native button. Hosting
                // must not enqueue a second navigation after setup opens.
                if(one_up::native_setup_active() && s.max_players!=1 && !model().active)queue_command("host","",{});
            }
            else if(a.command=="back") switch_page(c,false);
            else if(a.command=="duration") s.seconds=s.seconds>=120?10:s.seconds+10;
            else if(a.command=="players") s.max_players=s.max_players>=6?1:s.max_players+1;
            else if(a.command=="place" && !s.place_until) {
                const auto v=one_up::view();
                if(v.can_create) {
                    if(one_up::begin_flag_placement(s.seconds,s.max_players)) { s.setup_status.clear(); switch_page(c,false); }
                }
                else {
                    s.place_seconds=s.seconds; s.place_players=s.max_players; s.place_until=now+30000;
                    s.setup_status="Connecting multiplayer, then opening flag placement...";
                    if(!model().active && !queue_command("host","",{})) {
                        s.place_until=0; s.setup_status="Could not start multiplayer. Try Confirm again.";
                    }
                }
            }
            else if(a.command=="oneup") one_up::queue(a.argument);
        }
        if(s.place_until) {
            const auto v=one_up::view();
            if(v.can_create) {
                s.place_until=0; s.setup_status.clear();
                one_up::begin_flag_placement(s.place_seconds,s.place_players,true);
            } else if(now>=s.place_until) {
                s.place_until=0;
                s.setup_status="Could not place 1-Up. Load the map, finish your current Throwdown, and try Confirm again. "+model().status;
            }
        }
        try { official_setup(c); }
        catch(const std::exception& e) {
            static std::uint64_t setup_retry_log{};
            if(now>=setup_retry_log) {
                setup_retry_log=now+10000;
                logging::log(logging::Level::warning,logging::Channel::ui,"1-Up official setup: {}",e.what());
            }
        }
        if(s.details && !s.selected_mode.empty()) mode_render(c);
        s.entry_seconds.store(s.seconds);s.entry_players.store(s.max_players);
        stock_solo_controls(c);
        gameplay_hud(c,now);
    } catch(const std::exception& e) {
        logging::log(logging::Level::warning,logging::Channel::ui,"1-Up native menu: {}",e.what());
        s.next_tick=now+5000; release_native_one_up_menu(base);
    }
}
} // namespace dingosdk::multiplayer

