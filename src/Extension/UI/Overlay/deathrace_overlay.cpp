#include "overlay_internal.h"
#include "deathrace_draw.h"
#include "deathrace_fonts.h"
#include "Extension/UI/NativeMenu/deathrace_native_bridge.h"
#include "Extension/Modes/game_modes.h"
#include "input_capture.h"
#include "deathrace_options.h"
#include "Extension/Profile/local_profile_runtime.h"
#include "Engine/Core/Profiling/profiler.h"

namespace dingosdk::overlay::detail {
namespace {
ImTextureID atlas{};bool pixels_waiting=true;
std::string resource(const wchar_t* name){
    HMODULE module{};GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS|GET_MODULE_HANDLE_EX_FLAG_UNCHANGED_REFCOUNT,reinterpret_cast<LPCWSTR>(&resource),&module);
    const auto r=FindResourceW(module,name,MAKEINTRESOURCEW(10));const auto h=r?LoadResource(module,r):nullptr;
    const auto* p=h?static_cast<const char*>(LockResource(h)):nullptr;return p?std::string(p,SizeofResource(module,r)):std::string{};
}
}
void load_deathrace_fonts(){
    auto& s=state();wchar_t directory[MAX_PATH]{};GetWindowsDirectoryW(directory,MAX_PATH);
    const auto path=std::filesystem::path(directory)/L"Fonts"/L"seguisym.ttf";
    const auto fonts=deathrace_ui::load_native_fonts(resource,std::filesystem::exists(path)?path.string():std::string{});
    s.deathrace_body=fonts.body;s.deathrace_secondary=fonts.secondary;s.deathrace_marker=fonts.marker;
    s.deathrace_names=fonts.names;s.deathrace_display=fonts.display;s.deathrace_clocks=fonts.clocks;
}

bool take_deathrace_pixels(CardPixels& pixels){
    if(!pixels_waiting)return false;const auto bytes=resource(L"DEATHRACE_ATLAS");
    if(bytes.empty() || !decode_picture(bytes,pixels,2048))return false;pixels_waiting=false;return true;
}
void deathrace_image_uploaded(ImTextureID texture){atlas=texture;}
void deathrace_image_failed(){pixels_waiting=true;}
void deathrace_image_lost(){skate2_ribbons::retire();atlas={};pixels_waiting=true;state().deathrace_names=state().deathrace_body=state().deathrace_marker=state().deathrace_secondary=state().deathrace_display=nullptr;state().deathrace_clocks={};}
bool deathrace_pending(){const auto p=deathrace_ui::presentation_at(GetTickCount64());return p && p->input.screen!=deathrace_ui::Screen::lobby && p->input.screen!=deathrace_ui::Screen::hidden;}
void draw_deathrace(){
    DINGO_PROFILE_ZONE("Overlay/Deathrace Redline");
    namespace dr=deathrace_ui;const auto now=GetTickCount64();auto sample=dr::presentation_at(now);if(!sample)return;
    // Native data is sampled under the game's model lock. Only immutable
    // presentation data crosses into the renderer, never native UI pointers.
    auto in=sample->input;const auto* viewport=ImGui::GetMainViewport();in.aspect=viewport->Size.x/std::max(1.f,viewport->Size.y);
    if(in.screen==dr::Screen::load || in.screen==dr::Screen::save || in.screen==dr::Screen::rename || in.screen==dr::Screen::erase){
        const auto cursor=dr::cursor();if(cursor.world==sample->world && cursor.game==sample->game && cursor.serial==sample->serial){
            in.name_entry=cursor.name;in.selected_route=cursor.route;in.route_action=cursor.action;in.first_route=cursor.first;in.action_focus=cursor.action_focus;
        }
    }
    if(state().visible.load() || state().console_visible.load() || state().hub_typing.load())return;
    // Lobby input and presentation are entirely native, including Hold Start.
    if(in.screen==dr::Screen::lobby || in.screen==dr::Screen::hidden)return;
    static std::uint64_t world{},entered{},last{},serial{},notice_at{};static std::uint32_t game{};
    static unsigned digit{};static std::uint64_t beat_at{};
    static auto screen=dr::Screen::hidden;static std::string notice;static float clear{},save{};
    const bool fresh=world!=sample->world || game!=sample->game || dr::transition_family(screen)!=dr::transition_family(in.screen) || serial!=sample->serial || now-last>500;
    if(fresh){world=sample->world;game=sample->game;screen=in.screen;serial=sample->serial;entered=now;clear=in.clear_hold;save=in.save_hold;notice.clear();}
    if(fresh || digit!=in.countdown){digit=in.countdown;beat_at=now;}
    const float dt=std::min(.05f,static_cast<float>(now-last)*.001f);last=now;
    const auto smooth=[&](float& display,float actual){display=actual<=0?0.f:display+(actual-display)*std::min(1.f,dt*24.f);return display;};
    in.clear_hold=smooth(clear,in.clear_hold);in.save_hold=smooth(save,in.save_hold);
    if(notice!=in.notification){notice=in.notification;notice_at=now;}
    in.options=deathrace_options();
    static dr::GatePulse gate_pulse;
    const bool running=in.screen==dr::Screen::racing || in.screen==dr::Screen::go;
    in.gate_pulse=gate_pulse.update(sample->world,sample->game,running,in.cleared,in.required,now,in.options.reduced_motion);
    auto scene=dr::compose(in);for(const auto& n:sample->numbers)scene.cells.push_back(n);
    auto* draw=ImGui::GetBackgroundDrawList();
    const bool modal=in.screen==dr::Screen::save || in.screen==dr::Screen::load || in.screen==dr::Screen::rename || in.screen==dr::Screen::erase || in.screen==dr::Screen::exit_builder || in.screen==dr::Screen::intro || in.screen==dr::Screen::results;
    const float t=std::clamp(static_cast<float>(now-entered)/180.f,0.f,1.f),fade=in.options.reduced_motion?1.f:1.f-(1.f-t)*(1.f-t)*(1.f-t);
    if(modal)draw->AddRectFilled(viewport->Pos,ImVec2(viewport->Pos.x+viewport->Size.x,viewport->Pos.y+viewport->Size.y),IM_COL32(0,0,0,static_cast<unsigned>(125.f*fade)));
    for(auto& c:scene.cells)if(c.box.y==866)c.alpha*=std::min(1.f,static_cast<float>(now-notice_at)/140.f);
    dr::DrawContext context{state().deathrace_body,state().deathrace_marker,atlas,viewport->Pos,viewport->Size};
    context.name_font=state().deathrace_names;context.clock_fonts=state().deathrace_clocks;context.display_font=state().deathrace_display;context.secondary=state().deathrace_secondary;context.seconds=static_cast<float>(now%60000)*.001f;context.entrance=in.options.reduced_motion || dr::race_surface(in.screen)?1.f:fade;context.pointer=ImGui::GetMousePos();context.text_entry=in.screen==dr::Screen::save || in.screen==dr::Screen::rename;context.blank_name=in.name_entry.empty();
    context.options=in.options;context.gate_pulse=in.gate_pulse;context.race_active=running;
    context.beat=in.options.reduced_motion?1.f:std::clamp(static_cast<float>(now-beat_at)/240.f,0.f,1.f);
    dr::draw_scene(*draw,context,scene);
    if(!scene.cells.empty() && atlas && state().deathrace_body && state().deathrace_marker)dr::acknowledge(sample->world,sample->game,sample->serial,now);
}
}

namespace dingosdk::overlay {
namespace {std::mutex options_mutex;deathrace_ui::RedlineOptions options_cache;bool options_loaded{};std::uint64_t options_retry{};}
deathrace_ui::RedlineOptions deathrace_options() {
    std::lock_guard lock(options_mutex);const auto now=GetTickCount64();
    if(!options_loaded && now>=options_retry){options_retry=now+2000;if(profile_runtime::local_profile_store_ready()){
        const auto real=[&](const char* key,float& field){if(const auto v=profile_runtime::local_value(key);v && v->is_number())field=static_cast<float>(v->get<double>());};
        const auto flag=[&](const char* key,bool& field){if(const auto v=profile_runtime::local_preference(key))field=*v;};
        real("DeathraceHUDScale",options_cache.scale);real("DeathraceHUDSafeArea",options_cache.safe);real("DeathraceHUDOpacity",options_cache.opacity);real("DeathraceHUDEffects",options_cache.effects);
        flag("DeathraceHUDReducedMotion",options_cache.reduced_motion);flag("DeathraceHUDSpeed",options_cache.speed);flag("DeathraceHUDMPH",options_cache.mph);flag("DeathraceHUDStandings",options_cache.standings);flag("DeathraceHUDGuidance",options_cache.guidance);flag("DeathraceHUDClassic",options_cache.classic);
        if(!profile_runtime::local_preference("DeathraceHUDSkate2").value_or(false)){options_cache.opacity=.75f;options_cache.standings=true;options_cache.classic=false;}
        options_cache.normalize();options_loaded=true;
    }}return options_cache;
}
void set_deathrace_options(deathrace_ui::RedlineOptions value) {
    value.normalize();{std::lock_guard lock(options_mutex);options_cache=value;options_loaded=true;}
    profile_runtime::set_local_values({{"DeathraceHUDSkate2",true},{"DeathraceHUDScale",double(value.scale)},{"DeathraceHUDSafeArea",double(value.safe)},{"DeathraceHUDOpacity",double(value.opacity)},{"DeathraceHUDEffects",double(value.effects)},
        {"DeathraceHUDReducedMotion",value.reduced_motion},{"DeathraceHUDSpeed",value.speed},{"DeathraceHUDMPH",value.mph},{"DeathraceHUDStandings",value.standings},{"DeathraceHUDGuidance",value.guidance},{"DeathraceHUDClassic",value.classic}});
}
}
