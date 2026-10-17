// Drawn only for the editor's active rotation tool. Native HUD presenters are
// untouched; this is a world-projected editing guide and captures no input.
bool race_rotation_visible() {
    const auto view=modes::native_view();
    return view.active && view.placing && view.settings.mode==modes::Mode::race && view.rotating &&
        view.cursor_valid && !view.exit_confirm && !interface_open() && !multiplayer::native_one_up_match_panel_open();
}
void draw_race_rotation() {
    static modes::RaceRotationVisual visual;
    static std::uint64_t world{};static std::uint32_t game{};static unsigned object{};
    if(!race_rotation_visible()){visual.reset();return;}
    const auto view=modes::native_view();const auto frame=modes::hud();
    if(world!=view.world || game!=view.game || object!=view.route_object){visual.reset();world=view.world;game=view.game;object=view.route_object;}
    const auto pose=visual.update({view.cursor,view.cursor_yaw,view.cursor_pitch,view.cursor_width},GetTickCount64());
    if(!(frame.vertical_fov>1.f && frame.vertical_fov<175.f))return;
    const auto display=ImGui::GetIO().DisplaySize;if(display.x<=0 || display.y<=0)return;
    const auto axes=modes::race_gate_axes(pose.yaw,pose.pitch);
    auto center=pose.at;
    if(view.route_object!=4)for(unsigned axis=0;axis<3;++axis)center[axis]+=axes.up[axis]*1.70747f*pose.width/6.f;
    const float focal=display.y/(2.f*std::tan(frame.vertical_fov*3.14159265f/360.f));
    const auto project=[&](const modes::Vec3& point)->std::optional<ImVec2> {
        modes::Vec3 delta{};for(unsigned axis=0;axis<3;++axis)delta[axis]=point[axis]-frame.camera[12+axis];
        const auto dot=[&](unsigned start){return delta[0]*frame.camera[start]+delta[1]*frame.camera[start+1]+delta[2]*frame.camera[start+2];};
        const float depth=-dot(8);if(!(depth>.1f))return {};
        const ImVec2 pixel{display.x*.5f+dot(0)*focal/depth,display.y*.5f-dot(4)*focal/depth};
        if(!std::isfinite(pixel.x) || !std::isfinite(pixel.y) || pixel.x<-display.x || pixel.x>display.x*2.f || pixel.y<-display.y || pixel.y>display.y*2.f)return {};return pixel;
    };
    const auto origin=project(center);if(!origin)return;
    float depth{};for(unsigned axis=0;axis<3;++axis)depth-=(center[axis]-frame.camera[12+axis])*frame.camera[8+axis];
    const float scale=std::clamp(display.y/1080.f,.75f,3.f),pixels=90.f*scale;
    const float radius=pixels*depth/focal;
    auto* draw=ImGui::GetForegroundDrawList();
    const std::array<ImU32,3> colors{IM_COL32(244,94,103,235),IM_COL32(111,224,131,235),IM_COL32(85,159,255,220)};
    const std::array<modes::Vec3,3> first{axes.up,modes::Vec3{1,0,0},axes.right};
    const std::array<modes::Vec3,3> second{axes.forward,modes::Vec3{0,0,1},axes.up};
    draw->AddCircle(*origin,pixels*1.18f,IM_COL32(245,245,245,220),96,1.5f*scale);
    for(unsigned ring=0;ring<3;++ring) {
        std::optional<ImVec2> previous;
        for(unsigned segment=0;segment<=96;++segment) {
            const float angle=float(segment)*2.f*3.14159265f/96.f;auto point=center;
            for(unsigned axis=0;axis<3;++axis)point[axis]+=radius*(first[ring][axis]*std::cos(angle)+second[ring][axis]*std::sin(angle));
            const auto pixel=project(point);
            if(pixel && previous)draw->AddLine(*previous,*pixel,colors[ring],2.f*scale);
            previous=pixel;
        }
        auto tip=center;
        const auto& axis=ring==0?axes.right:ring==1?modes::Vec3{0,1,0}:axes.forward;
        for(unsigned component=0;component<3;++component)tip[component]+=axis[component]*radius;
        if(const auto pixel=project(tip)) {
            const char* text=ring==0?"X":ring==1?"Y":"Z";
            draw->AddText({pixel->x+5.f*scale,pixel->y-6.f*scale},colors[ring],text);
        }
    }
    draw->AddCircleFilled(*origin,3.f*scale,IM_COL32(255,255,255,235));
}
