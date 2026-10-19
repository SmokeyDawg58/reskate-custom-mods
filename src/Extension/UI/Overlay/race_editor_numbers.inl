bool race_editor_numbers_visible() {
    const auto view=modes::native_view();
    return view.active && view.leading && view.placing && view.settings.mode==modes::Mode::race &&
        !view.exit_confirm && !interface_open() && !multiplayer::native_one_up_match_panel_open();
}
void draw_race_editor_numbers() {
    if(!race_editor_numbers_visible())return;
    const auto view=modes::native_view();const auto frame=modes::hud();
    if(!(frame.vertical_fov>1 && frame.vertical_fov<175))return;
    const auto display=ImGui::GetIO().DisplaySize;if(display.x<=0 || display.y<=0)return;
    const float focal=display.y/(2.f*std::tan(frame.vertical_fov*3.14159265f/360.f));
    const float scale=std::clamp(display.y/1080.f,.75f,3.f),font=24.f*scale;
    auto* draw=ImGui::GetForegroundDrawList();
    for(std::size_t i=1;i<view.settings.points.size();++i) {
        const auto text=modes::race_editor_gate_number(view.settings,i);if(text.empty())continue;
        const auto at=modes::race_editor_gate_number_position(view.settings,i);
        modes::Vec3 delta{};for(unsigned a=0;a<3;++a)delta[a]=at[a]-frame.camera[12+a];
        const auto dot=[&](unsigned row){return delta[0]*frame.camera[row]+delta[1]*frame.camera[row+1]+delta[2]*frame.camera[row+2];};
        const float depth=-dot(8);if(depth<.5f || depth>350.f)continue;
        const float x=display.x*.5f+dot(0)*focal/depth,y=display.y*.5f-dot(4)*focal/depth;
        if(!std::isfinite(x) || !std::isfinite(y) || x<0 || x>display.x || y<0 || y>display.y)continue;
        const auto size=ImGui::GetFont()->CalcTextSizeA(font,FLT_MAX,0,text.c_str());
        const ImVec2 left{x-size.x*.5f,y-size.y*.5f};const float padding=7.f*scale;
        draw->AddRectFilled({left.x-padding,left.y-padding},{left.x+size.x+padding,left.y+size.y+padding},IM_COL32(10,13,18,210),4.f*scale);
        const auto tint=view.settings.race_finish && i+1==view.settings.points.size()?IM_COL32(255,115,115,255):IM_COL32(255,255,255,255);
        draw->AddText(ImGui::GetFont(),font,left,tint,text.c_str());

    }
}
