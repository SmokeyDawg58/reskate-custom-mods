// Route dialogs only. The game owns Join, Leave and Hold Start in the lobby.
bool draw_native_race_routes(const modes::NativeView& view) {
    namespace dr=deathrace_ui;const auto now=GetTickCount64();
    if(!dr::current(view.world,view.game,now) || dr::presented_serial.load()!=view.route_dialog_serial)return false;
    static unsigned previous{};static std::uint64_t repeat_at{};static bool released{},foreground_before{};
    auto current=dr::cursor();
    const bool fresh=current.world!=view.world || current.game!=view.game || current.serial!=view.route_dialog_serial;
    if(fresh){current={view.world,view.route_dialog_serial,view.game,0,0,0,view.route_name};released=false;repeat_at=0;}
    OverlayInputAccess input_access;const auto pad=read_gamepad();
    const bool foreground=game_window_foreground(state().window.load());
    unsigned held=pad.buttons;
    if(ImGui::IsKeyDown(ImGuiKey_UpArrow))held|=1;if(ImGui::IsKeyDown(ImGuiKey_DownArrow))held|=2;
    if(ImGui::IsKeyDown(ImGuiKey_LeftArrow))held|=4;if(ImGui::IsKeyDown(ImGuiKey_RightArrow))held|=8;
    if(ImGui::IsKeyDown(ImGuiKey_Enter))held|=0x1000;if(ImGui::IsKeyDown(ImGuiKey_Escape))held|=0x2000;
    if(pad.left_y>.65f)held|=1;if(pad.left_y<-.65f)held|=2;if(pad.left_x<-.65f)held|=4;if(pad.left_x>.65f)held|=8;
    const auto store=[&]{std::lock_guard lock(dr::route_cursor_mutex);dr::route_cursor=current;};
    if(!foreground || (foreground && !foreground_before)){released=false;previous=held;foreground_before=foreground;store();return true;}
    foreground_before=foreground;
    if(!released){released=held==0;previous=held;store();return true;}
    unsigned pressed=held&~previous;previous=held;
    if(held&15u){if((pressed&15u)!=0)repeat_at=now+330;else if(now>=repeat_at){pressed|=held&15u;repeat_at=now+110;}}
    else repeat_at=0;
    const bool list=view.route_dialog==2;
    std::vector<std::uint64_t> route_ids;for(const auto& r:view.saved_routes)if(r.map==view.route_map)route_ids.push_back(r.id);
    const unsigned count=static_cast<unsigned>(route_ids.size());
    if(list){
        dr::RouteNavigation nav{current.route,current.action,current.action_focus,current.column};nav.normalize(count);
        if(pressed&1u)nav.vertical(-1,count);if(pressed&2u)nav.vertical(1,count);
        if(pressed&4u)nav.horizontal(-1,count);if(pressed&8u)nav.horizontal(1,count);
        current.route=nav.route;current.action=nav.action;current.action_focus=nav.actions;current.column=nav.column;current.first=nav.first_route();
    }else if(pressed&15u)current.action=1-current.action;
    const auto send=[&](const char* action,std::string argument={}){modes::request_race_route_action(view.world,view.game,action,std::move(argument));released=false;};
    bool activate=(pressed&0x1000u)!=0;
    const auto* viewport=ImGui::GetMainViewport();const auto mouse=ImGui::GetMousePos();
    const float aspect=viewport->Size.y>0?viewport->Size.x/viewport->Size.y:16.f/9.f;
    const dr::Frame frame(aspect);const float logical_width=frame.width;
    const auto pointer=frame.logical((mouse.x-viewport->Pos.x)/std::max(1.f,viewport->Size.x),(mouse.y-viewport->Pos.y)/std::max(1.f,viewport->Size.y));
    const float mx=pointer[0],my=pointer[1];
    const dr::RouteLayout layout(logical_width,list);
    const auto hit=[&](dr::Rect r){return mx>=r.x && mx<=r.x+r.w && my>=r.y && my<=r.y+r.h;};
    if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
        if(list)for(unsigned i=current.first;i<std::min(count,current.first+4);++i)if(hit(layout.row(i-current.first))){current.route=i;current.action=0;current.action_focus=false;activate=true;}
        for(unsigned i=0;i<(list?6u:2u);++i)if(hit(layout.action(i))){current.action=i;current.action_focus=true;activate=true;}
    }
    if(view.route_dialog==1 || view.route_dialog==3){
        if(ImGui::IsKeyPressed(ImGuiKey_Backspace) && !current.name.empty()){
            auto at=current.name.size()-1;while(at && (static_cast<unsigned char>(current.name[at])&0xc0)==0x80)--at;current.name.resize(at);
        }
        if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_A))current.name.clear();
        const auto append=[&](unsigned code){if(code<32 || code==127)return;std::string utf;
            if(code<128)utf+=static_cast<char>(code);
            else if(code<2048){utf+=static_cast<char>(0xc0|(code>>6));utf+=static_cast<char>(0x80|(code&63));}
            else if(code<65536){utf+=static_cast<char>(0xe0|(code>>12));utf+=static_cast<char>(0x80|((code>>6)&63));utf+=static_cast<char>(0x80|(code&63));}
            else {utf+=static_cast<char>(0xf0|(code>>18));utf+=static_cast<char>(0x80|((code>>12)&63));utf+=static_cast<char>(0x80|((code>>6)&63));utf+=static_cast<char>(0x80|(code&63));}
            if(current.name.size()+utf.size()<=64)current.name+=utf;
        };
        for(const auto code:ImGui::GetIO().InputQueueCharacters)append(code);
        if(ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_V))if(const auto* text=ImGui::GetClipboardText()){
            std::string pasted=text;if(pasted.size()>64){auto end=std::size_t{64};while(end && (static_cast<unsigned char>(pasted[end])&0xc0)==0x80)--end;pasted.resize(end);}current.name=pasted;
        }
    }
    // Shortcuts only exist in the library, never while typing a route name or
    // confirming deletion. They use the same world/game/dialog action guard.
    if(list){
        const auto shortcut=[&](unsigned action){current.action=action;current.action_focus=true;activate=true;};
        if((pressed&0x8000u) || ImGui::IsKeyPressed(ImGuiKey_Delete))shortcut(3);
        else if((pressed&0x4000u) || ImGui::IsKeyPressed(ImGuiKey_F2))shortcut(1);
        else if(pressed&0x0200u)shortcut(2);
        else if(pressed&0x0100u)shortcut(4);
        else if(activate && !current.action_focus)current.action=0;
    }
    if(pressed&0x2000u)send("cancel");
    else if(activate){
        if(list){
            if(current.action==4){store();auto file=choose_route_json();if(!file.empty())send("import",std::move(file));}
            else if(current.action==5)send("cancel");
            else if(current.route<count){const auto id=std::to_string(route_ids[current.route]);send(std::array{"load","rename","reveal","delete"}[current.action],id);}
        } else if(view.route_dialog==4)send(current.action==0?"cancel":"delete-confirm");
        else if(current.action==1)send("cancel");
        else if(!current.name.empty())send(view.route_dialog==3?"rename-save":"save",current.name);
    }
    store();return true;
}
