#include "overlay_internal.h"
#include "cursor.h"
#include "input_capture.h"
#include "Extension/Modes/game_modes.h"
#include "Extension/UI/NativeMenu/deathrace_native_bridge.h"
#include "Extension/UI/NativeMenu/deathrace_native_presentation.h"
#include <algorithm>
#include <array>
#include <string>
#include <commdlg.h>
#include "Engine/Core/Platform/path_text.h"
#include "Engine/Game/World/world_names.h"
#include "Engine/Game/World/custom_level_manifest.h"
#include "Engine/Vfs/mod_catalog.h"
#pragma comment(lib,"comdlg32.lib")

namespace dingosdk::overlay::detail {
namespace {
std::string choose_route_json() {
    std::array<wchar_t,32768> filename{};
    OPENFILENAMEW picker{};picker.lStructSize=sizeof(picker);picker.hwndOwner=state().window.load();
    picker.lpstrFilter=L"Deathrace routes (*.json)\0*.json\0";picker.lpstrFile=filename.data();picker.nMaxFile=static_cast<DWORD>(filename.size());
    picker.lpstrTitle=L"Load a Deathrace route JSON file";
    picker.Flags=OFN_FILEMUSTEXIST|OFN_PATHMUSTEXIST|OFN_NOCHANGEDIR|OFN_DONTADDTORECENT;
    OverlayInputAccess access;SystemCursorLease cursor;cursor.show();
    return GetOpenFileNameW(&picker)?path_utf8(std::filesystem::path(filename.data())):std::string{};
}
std::string route_map_title(const std::string& map) {
    static const auto levels=[] {
        std::vector<CustomLevelManifest> manifests;
        for(const auto& path:mods::level_manifest_paths(mods::catalog()))manifests.push_back(read_custom_level_manifest(path));
        return combine_custom_level_manifests(manifests).levels;
    }();
    return world_destination_name(map,levels,true);
}
}
#include "race_routes_native_input.inl"
void draw_race_routes() {
    const auto view=modes::native_view();
    if(!view.active || !view.leading || !view.placing || !view.route_dialog)return;
    if(draw_native_race_routes(view))return;
    static bool was_foreground{},restore_focus{};
    static std::uint64_t serial{},world{};static std::uint32_t game{};
    static std::array<char,65> name{};
    const bool fresh=serial!=view.route_dialog_serial || world!=view.world || game!=view.game;
    if(fresh){serial=view.route_dialog_serial;world=view.world;game=view.game;name={};
        std::copy_n(view.route_name.c_str(),std::min(view.route_name.size(),name.size()-1),name.data());}
    const bool foreground=game_window_foreground(state().window.load());
    const bool focus=fresh || restore_focus || (foreground && !was_foreground);
    was_foreground=foreground;restore_focus=false;
    if(focus && foreground)ImGui::SetNextWindowFocus();
    const auto* viewport=ImGui::GetMainViewport();
    const float scale=std::clamp(std::min(viewport->WorkSize.x/1600.f,viewport->WorkSize.y/900.f),.65f,1.4f);
    const bool deleting=view.route_dialog==4,renaming=view.route_dialog==3,saving=view.route_dialog==1 || renaming;
    const float width=std::min(800.f*scale,viewport->WorkSize.x-32.f);
    const float height=std::min((saving || deleting?430.f:640.f)*scale,viewport->WorkSize.y-32.f);
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x+viewport->WorkSize.x*.5f,viewport->WorkPos.y+viewport->WorkSize.y*.5f),ImGuiCond_Always,ImVec2(.5f,.5f));
    ImGui::SetNextWindowSize(ImVec2(width,height),ImGuiCond_Always);
    ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding,ImVec2(28.f*scale,24.f*scale));
    ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing,ImVec2(12.f*scale,14.f*scale));
    ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding,8.f*scale);
    ImGui::PushStyleColor(ImGuiCol_WindowBg,ImVec4(.055f,.06f,.07f,.97f));
    ImGui::PushStyleColor(ImGuiCol_Border,ImVec4(.72f,.19f,.2f,1));
    ImGui::PushStyleColor(ImGuiCol_Button,ImVec4(.42f,.1f,.12f,1));
    ImGui::PushStyleColor(ImGuiCol_ButtonHovered,ImVec4(.63f,.16f,.18f,1));
    ImGui::PushStyleColor(ImGuiCol_Header,ImVec4(.44f,.12f,.14f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered,ImVec4(.36f,.14f,.16f,1));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive,ImVec4(.53f,.15f,.17f,1));
    ImGui::PushFont(state().menu.body?state().menu.body:ImGui::GetFont());
    if(ImGui::Begin("Deathrace routes##route-library",nullptr,ImGuiWindowFlags_NoTitleBar|ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoCollapse|ImGuiWindowFlags_NoSavedSettings)) {
        ImGui::SetWindowFontScale(1.1f*scale);
        const auto send=[&](const char* action,std::string argument={}){modes::request_race_route_action(view.world,view.game,action,std::move(argument));};
        ImGui::PushFont(state().menu.heading?state().menu.heading:ImGui::GetFont());
        ImGui::TextUnformatted(deleting?"DELETE ROUTE?":renaming?"RENAME ROUTE":saving?"SAVE DEATHRACE ROUTE":"LOAD DEATHRACE ROUTE");ImGui::PopFont();ImGui::Separator();
        ImGui::TextWrapped("Map: %s",route_map_title(view.route_map).c_str());
        if(deleting) {
            ImGui::TextWrapped("Are you sure you want to delete \"%s\"?",view.route_name.c_str());
            ImGui::TextWrapped("This removes it from your saved routes.");
            if(!view.route_dialog_error.empty())ImGui::TextWrapped("%s",view.route_dialog_error.c_str());
            ImGui::Separator();
            if(focus)ImGui::SetKeyboardFocusHere();
            if(ImGui::Button("Cancel",ImVec2(180.f*scale,44.f*scale)))send("cancel");
            if(focus){ImGui::SetItemDefaultFocus();ImGui::SetNavCursorVisible(true);}
            ImGui::SameLine();if(ImGui::Button("Delete route",ImVec2(180.f*scale,44.f*scale)))send("delete-confirm");
        } else if(saving) {
            const auto required=modes::mandatory_gates(view.settings,view.settings.points.size());
            if(!renaming)ImGui::Text("%zu Required gates + Finish",required);
            ImGui::TextUnformatted("Route name");ImGui::SetNextItemWidth(-1);
            if(focus)ImGui::SetKeyboardFocusHere();
            const bool entered=ImGui::InputTextWithHint("##route-name","Name your route",name.data(),name.size(),ImGuiInputTextFlags_EnterReturnsTrue);
            if(!view.route_dialog_error.empty())ImGui::TextWrapped("%s",view.route_dialog_error.c_str());
            ImGui::Separator();
            ImGui::BeginDisabled(name[0]==0);
            if(ImGui::Button(renaming?"Save name":"Save route",ImVec2(180.f*scale,44.f*scale)) || entered)send(renaming?"rename-save":"save",name.data());
            ImGui::EndDisabled();ImGui::SameLine();if(ImGui::Button("Cancel",ImVec2(130.f*scale,44.f*scale)))send("cancel");
        } else {
            ImGui::TextWrapped("Choose a saved route. Only routes saved on this map are shown.");
            const float footer=110.f*scale;
            if(ImGui::BeginChild("##saved-routes",ImVec2(0,std::max(80.f,ImGui::GetContentRegionAvail().y-footer)),true)) {
                if(view.saved_routes.empty())ImGui::TextWrapped("No saved routes yet. Build your course, then tap Start to name and save it.");
                bool first=true;
                for(const auto& route:view.saved_routes) {
                    if(route.map!=view.route_map)continue;
                    ImGui::PushID(std::to_string(route.id).c_str());
                    const bool matching=route.map==view.route_map;ImGui::BeginDisabled(!matching);
                    const auto at=ImGui::GetCursorScreenPos();const float row_width=ImGui::GetContentRegionAvail().x-164.f*scale;
                    if(focus && first)ImGui::SetKeyboardFocusHere();
                    if(ImGui::Selectable("##route",false,ImGuiSelectableFlags_None,ImVec2(row_width,88.f*scale)))send("load",std::to_string(route.id));
                    if(focus && first){ImGui::SetItemDefaultFocus();ImGui::SetNavCursorVisible(true);}first=false;
                    auto* draw=ImGui::GetWindowDrawList();draw->PushClipRect(at,ImVec2(at.x+row_width,at.y+88.f*scale),true);
                    const auto color=ImGui::GetColorU32(matching?ImGuiCol_Text:ImGuiCol_TextDisabled);
                    draw->AddText(ImVec2(at.x+8.f*scale,at.y+5.f*scale),color,route.name.c_str());
                    const auto info=std::to_string(route.required)+" Required + Finish  |  "+std::to_string(route.objects)+" objects";
                    draw->AddText(ImVec2(at.x+8.f*scale,at.y+44.f*scale),color,info.c_str());draw->PopClipRect();
                    ImGui::EndDisabled();
                    ImGui::SameLine();ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x,at.y+24.f*scale));const auto pencil=ImGui::GetCursorScreenPos();
                    if(ImGui::Button("##rename",ImVec2(40.f*scale,40.f*scale)))send("rename",std::to_string(route.id));
                    auto* icon=ImGui::GetWindowDrawList();const auto ink=ImGui::GetColorU32(ImGuiCol_Text);
                    const auto a=ImVec2(pencil.x+11.f*scale,pencil.y+29.f*scale),b=ImVec2(pencil.x+29.f*scale,pencil.y+11.f*scale);
                    icon->AddLine(a,b,ink,5.f*scale);icon->AddLine(ImVec2(a.x-3.f*scale,a.y+3.f*scale),a,ink,2.f*scale);
                    if(ImGui::IsItemHovered() || ImGui::IsItemFocused())ImGui::SetTooltip("Rename route");
                    ImGui::SameLine();ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x,at.y+24.f*scale));const auto folder=ImGui::GetCursorScreenPos();
                    if(ImGui::Button("##folder",ImVec2(40.f*scale,40.f*scale)))send("reveal",std::to_string(route.id));
                    icon->AddRect(ImVec2(folder.x+8.f*scale,folder.y+13.f*scale),ImVec2(folder.x+32.f*scale,folder.y+29.f*scale),ink,2.f*scale,0,2.f*scale);
                    icon->AddLine(ImVec2(folder.x+8.f*scale,folder.y+13.f*scale),ImVec2(folder.x+8.f*scale,folder.y+9.f*scale),ink,2.f*scale);
                    icon->AddLine(ImVec2(folder.x+8.f*scale,folder.y+9.f*scale),ImVec2(folder.x+19.f*scale,folder.y+9.f*scale),ink,2.f*scale);
                    icon->AddLine(ImVec2(folder.x+19.f*scale,folder.y+9.f*scale),ImVec2(folder.x+23.f*scale,folder.y+13.f*scale),ink,2.f*scale);
                    if(ImGui::IsItemHovered() || ImGui::IsItemFocused())ImGui::SetTooltip("Show JSON file in folder");
                    ImGui::SameLine();ImGui::SetCursorScreenPos(ImVec2(ImGui::GetCursorScreenPos().x,at.y+24.f*scale));const auto bin=ImGui::GetCursorScreenPos();
                    if(ImGui::Button("##delete",ImVec2(40.f*scale,40.f*scale)))send("delete",std::to_string(route.id));
                    icon->AddRect(ImVec2(bin.x+12.f*scale,bin.y+15.f*scale),ImVec2(bin.x+29.f*scale,bin.y+31.f*scale),ink,2.f*scale,0,2.f*scale);
                    icon->AddLine(ImVec2(bin.x+9.f*scale,bin.y+12.f*scale),ImVec2(bin.x+32.f*scale,bin.y+12.f*scale),ink,2.f*scale);
                    icon->AddLine(ImVec2(bin.x+16.f*scale,bin.y+8.f*scale),ImVec2(bin.x+25.f*scale,bin.y+8.f*scale),ink,2.f*scale);
                    if(ImGui::IsItemHovered() || ImGui::IsItemFocused())ImGui::SetTooltip("Delete route");
                    ImGui::PopID();
                }
            }
            ImGui::EndChild();
            if(!view.route_dialog_error.empty())ImGui::TextWrapped("%s",view.route_dialog_error.c_str());
            if(ImGui::Button("Load .json file",ImVec2(200.f*scale,44.f*scale))){auto file=choose_route_json();restore_focus=true;if(!file.empty())send("import",std::move(file));}
            ImGui::SameLine();if(ImGui::Button("Back to builder",ImVec2(220.f*scale,44.f*scale)))send("cancel");
        }
        if(ImGui::IsKeyPressed(ImGuiKey_Escape,false) || ImGui::IsKeyPressed(ImGuiKey_GamepadFaceRight,false))send("cancel");
    }
    ImGui::End();ImGui::PopFont();ImGui::PopStyleColor(7);ImGui::PopStyleVar(3);
}
}

namespace dingosdk::overlay {
std::array<float,2> deathrace_viewport() noexcept {
    RECT bounds{};const auto window=detail::state().window.load();
    if(window && GetClientRect(window,&bounds) && bounds.right>0 && bounds.bottom>0)
        return {static_cast<float>(bounds.right),static_cast<float>(bounds.bottom)};
    return {1920,1080};
}
std::string race_route_map_label(const std::string& map){return detail::route_map_title(map);}
}
