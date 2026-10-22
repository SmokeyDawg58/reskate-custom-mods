// Native renderer for the approved Deathrace design. This is deliberately a
// separate adapter: legacy_mode_hud remains intact as a runtime fallback.
deathrace_ui::Input native_deathrace_input(const modes::NativeView& v,const overlay::ModesHud& feed,std::uint64_t now) {
    namespace dr=deathrace_ui;dr::Input in;
    const auto viewport=overlay::deathrace_viewport();if(viewport[1]>0)in.aspect=viewport[0]/viewport[1];
    const auto name=[&](std::uint64_t id){for(const auto& pair:v.names)if(pair.first==id)return pair.second;return std::string("Skater");};
    in.player=name(v.local);in.host=name(v.leader);in.host_local=v.leading;
    in.route=v.route_name;in.map=overlay::race_route_map_label(v.route_map);in.saved=v.route_saved;in.reusable=v.route_reusable;
    in.object=std::string(modes::race_editor_object_name(v.route_object));in.arrow=v.route_object==4;
    in.required=static_cast<unsigned>(modes::mandatory_gates(v.settings,v.settings.points.size()));
    in.finish=v.settings.race_finish;in.capacity=v.capacity;in.deadline=v.settings.duration_s;
    in.width=v.cursor_width/6.f*100.f;in.height=v.cursor[1];in.snap=v.route_snap;in.snap_degrees=v.snap_degrees;
    in.yaw=modes::race_rotation_degrees(v.cursor_yaw);in.pitch=modes::race_rotation_degrees(v.cursor_pitch);
    in.roll=modes::race_rotation_degrees(v.cursor_roll);in.roll_axis=v.editor_roll_axis;
    in.clear_hold=v.clear_progress;in.save_hold=v.save_progress;in.error=v.editor_warning;
    in.object_alpha=modes::race_object_label_alpha(now,v.object_changed_ms);
    for(std::size_t i=1;i<v.settings.points.size();++i){
        if(modes::guiding_gate(v.settings,i))++in.guides;
        if(modes::arrow_marker(v.settings,i))++in.arrows;
        in.distance+=std::hypot(v.settings.points[i][0]-v.settings.points[i-1][0],v.settings.points[i][2]-v.settings.points[i-1][2]);
    }
    if(v.state) {
        in.players=static_cast<unsigned>(v.state->standings.size());
        for(unsigned i=0;i<v.state->standings.size();++i){const auto& row=v.state->standings[i];
            const auto passed=modes::mandatory_gates(v.settings,static_cast<std::size_t>(std::max(0,row.score)));
            const bool finished=static_cast<std::size_t>(std::max(0,row.score))>=v.settings.points.size() && !row.out;
            in.rows.push_back({name(row.player),row.out?"DNF\n"+native_hud_race_time(mode_racer_elapsed(v,row,now)):mode_racer_time(v,row,now),
                finished?"Finished":std::format("{} / {} gates",passed,in.required),row.player==v.local,finished,row.out,i+1});
            if(row.player==v.local){in.cleared=static_cast<unsigned>(passed);in.elapsed=mode_racer_time(v,row,now);in.position=row.out?0:i+1;}
        }
        in.remaining=native_hud_race_time(native_hud_race_remaining(v.state->remaining_ms,v.state_sampled_ms,now,v.state->phase==modes::Phase::playing));
        in.countdown=static_cast<unsigned>(native_hud_seconds(v.state->remaining_ms));
    }
    if(in.rows.empty())in.rows.push_back({in.player,"","",true,false,false,1});
    in.recovering=v.recovering;in.winner=feed.winner;in.compass=profile_runtime::local_compass_enabled();
    in.sampled_ms=now;
    const auto telemetry=trainer::telemetry();in.have_speed=telemetry.skater && feed.have_me && std::isfinite(telemetry.speed) && telemetry.speed>=0 && telemetry.speed<200;
    if(in.have_speed)in.speed_mps=telemetry.speed;
    if(v.state && v.state->phase==modes::Phase::playing)for(const auto& row:v.state->standings)if(row.player==v.local && !row.out){
        const auto next=modes::next_race_gate(v.settings,static_cast<std::size_t>(std::max(0,row.score)));
        if(next<v.settings.points.size() && next>0 && feed.have_me){
            in.have_gate=true;in.next_finish=v.settings.race_finish && next+1==v.settings.points.size();in.next_gate=in.cleared+1;
            const auto target=modes::race_editor_gate_number_position(v.settings,next);
            in.navigation=dr::project_gate(target,feed.camera,feed.vertical_fov,dr::Frame(in.aspect).width);
            const auto& gate=v.settings.points[next];in.gate_distance=std::hypot(gate[0]-feed.me[0],gate[1]-feed.me[1],gate[2]-feed.me[2]);
        }
    }
    in.restoring_flag=v.restoring_route;
    if(v.placing){
        in.screen=v.exit_confirm?dr::Screen::exit_builder:v.rotating?dr::Screen::rotate:dr::Screen::builder;
        if(v.route_dialog){in.screen=v.route_dialog==1?dr::Screen::save:v.route_dialog==2?dr::Screen::load:v.route_dialog==3?dr::Screen::rename:dr::Screen::erase;
            in.error=v.route_dialog_error;const auto cursor=dr::cursor();
            const bool same=cursor.world==v.world && cursor.game==v.game && cursor.serial==v.route_dialog_serial;
            in.name_entry=same?cursor.name:v.route_name;in.selected_route=same?cursor.route:0;in.route_action=same?cursor.action:0;in.first_route=same?cursor.first:0;in.action_focus=same && cursor.action_focus;
            for(const auto& route:v.saved_routes)if(route.map==v.route_map)in.routes.push_back({route.id,route.name,overlay::race_route_map_label(route.map),route.required,route.objects});
        }
    } else if(v.pending_flag)in.screen=dr::Screen::flag;
    else if(v.waiting)in.screen=dr::Screen::lobby;
    else if(v.state && v.state->phase==modes::Phase::countdown)in.screen=state().mode_hud.transients.intro?dr::Screen::intro:dr::Screen::countdown;
    else if(v.state && v.state->phase==modes::Phase::results)in.screen=dr::Screen::results;
    else if(v.state && v.state->phase==modes::Phase::playing){
        in.screen=state().mode_hud.transients.go?dr::Screen::go:dr::Screen::racing;
        for(const auto& row:in.rows)if(row.self){if(row.dnf)in.screen=dr::Screen::dnf;else if(row.finished)in.screen=dr::Screen::finished;}
    }
    auto& native=state().mode_hud.native_race;
    if(native.world!=v.world || native.game!=v.game){native.world=v.world;native.game=v.game;native.notification.clear();native.notification_serial=0;native.notification_until=0;}
    if(feed.banner_serial && feed.banner_serial!=native.notification_serial){native.notification_serial=feed.banner_serial;native.notification=feed.banner;native.notification_until=now+2000;}
    if(now<native.notification_until)in.notification=native.notification;
    if(v.missed_gate)in.notification=std::format("Missed Required gate {} - returning behind the gate",v.missed_gate);
    else if(!feed.warning.empty())in.notification=feed.warning;
    return in;
}
Value native_deathrace_glyph(const Context& c,ModeHud::NativeCell& cell,const std::string& action_name) {
    if(!cell.glyph.handle){
        auto& owned=state().owned;const auto checkpoint=owned.checkpoint();
        try {
        const auto donor=record_model(c,"Throwdown_DetailsMenu_ContentResource/ThrowdownDetails_HUDButton",button);
        const auto candidate=make(c,button);
        for(const auto field:{0x8cc042efU,0xf9515817U,0xf343d160U})c.copy(c.field(candidate,field),c.address(c.field(donor,field)));
        c.copy(c.path(candidate,{0x0fb0d794,0x8cc042ef}),c.address(c.path(donor,{0x0fb0d794,0x8cc042ef})));
        PrivateUiGraph graph{c};graph.walk(candidate,0);clear_value(c,c.field(candidate,0x67212c85));
        cell.glyph=candidate;
        }catch(...){owned.rollback(checkpoint,[&](Value value){c.destroy(value);});throw;}
    }
    plain_label(c,c.field(cell.glyph,0x58c9d354),"");
    const auto root=c.path(cell.glyph,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a});
    c.set_if_changed(c.field(root,0x38f7355f),true);c.set_if_changed(c.field(root,0x4b3146e9),false);
    const auto action=c.field(root,0xa2b71c93);clear_value(c,c.field(action,0x9c76b86c));c.text(c.field(action,0xb1e14cd6),"");
    const auto pia=asset(c,("Configurations/Input/UI/"+action_name).c_str());
    c.set_if_changed(c.field(action,0x3d00de27),pia);c.set_if_changed(c.field(action,0x0169c2db),pia);
    return cell.glyph;
}
void native_deathrace_numbers(deathrace_ui::Scene& scene,const modes::NativeView& v,const overlay::ModesHud& feed) {
    namespace dr=deathrace_ui;
    if(!v.placing || v.route_dialog || v.exit_confirm || !(feed.vertical_fov>1 && feed.vertical_fov<175))return;
    const auto viewport=overlay::deathrace_viewport();const dr::Frame frame(viewport[1]>0?viewport[0]/viewport[1]:16.f/9.f);
    const float focal=1080.f/(2.f*std::tan(feed.vertical_fov*3.14159265f/360.f));
    const auto overlays=scene.cells;
    for(std::size_t i=1;i<v.settings.points.size();++i){
        const auto text=modes::race_editor_gate_number(v.settings,i);if(text.empty())continue;
        const auto position=modes::race_editor_gate_number_position(v.settings,i);modes::Vec3 delta{};
        for(unsigned axis=0;axis<3;++axis)delta[axis]=position[axis]-feed.camera[12+axis];
        const auto dot=[&](unsigned row){return delta[0]*feed.camera[row]+delta[1]*feed.camera[row+1]+delta[2]*feed.camera[row+2];};
        const float depth=-dot(8);if(depth<.5f || depth>350.f)continue;
        const auto position_on_frame=frame.logical(.5f+dot(0)*focal/depth/frame.actual_width,.5f-dot(4)*focal/depth/1080.f);
        const float x=position_on_frame[0],y=position_on_frame[1];
        const bool finish=v.settings.race_finish && i+1==v.settings.points.size(),guide=modes::guiding_gate(v.settings,i);
        const dr::Rect box{x-(finish?40.f:34.f),y-17,finish?80.f:68.f,34};if(!dr::inside(box,scene.width,scene.height))continue;
        const bool covered=std::any_of(overlays.begin(),overlays.end(),[&](const auto& cell){const auto r=cell.box;return cell.paint!=dr::Paint::text && box.x<r.x+r.w && box.x+box.w>r.x && box.y<r.y+r.h && box.y+box.h>r.y;});
        if(covered)continue;
        if(dr::overlaps(box,dr::compass_reserved(scene.width)) || dr::overlaps(box,dr::dpad_reserved(scene.width)))continue;
        const auto caption=finish?std::string("FINISH"):guide?std::string("GUIDE"):std::format("{:02}",modes::mandatory_gates(v.settings,i)+1);
        scene.box(box,finish?dr::Paint::finish_tag:guide?dr::Paint::guide_tag:dr::Paint::gate_tag,caption,finish || guide?15:25,2,!finish && !guide);
    }
}
void publish_deathrace_presentation(const Context& c,const modes::NativeView& v,std::uint64_t now) {
    static_cast<void>(c);
    namespace dr=deathrace_ui;
    // Update phase transients before sampling, including the fast publication path.
    mode_hud_transients(c,v,now);
    auto in=native_deathrace_input(v,modes::hud(),now);
    auto numbers=dr::compose(in);const auto panel_cells=numbers.cells.size();
    native_deathrace_numbers(numbers,v,modes::hud());
    numbers.cells.erase(numbers.cells.begin(),numbers.cells.begin()+static_cast<std::ptrdiff_t>(panel_cells));
    dr::present(std::move(in),std::move(numbers.cells),v.world,v.game,v.route_dialog_serial,now);
}
void native_deathrace_fast_values(const Context& c,const modes::NativeView& v,std::uint64_t now) {
    if(state().mode_hud.native_race.visible)publish_deathrace_presentation(c,v,now);
}
bool native_deathrace_hud(const Context& c) {
    namespace dr=deathrace_ui;auto& s=state();auto& h=s.mode_hud;auto& native=h.native_race;
    const auto v=modes::native_view();const auto feed=modes::hud();const auto now=GetTickCount64();
    if(!dr::native_enabled.load() || !v.active || v.settings.mode!=modes::Mode::race){dr::retire();native.visible=false;return false;}
    if(!s.hud_stack.handle || c.type_of(s.hud_stack.handle)!=s.hud_stack.type){
        auto& initial=native.cells.front();
        if(!initial.model.handle)initial.model=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        mount_hud(c,initial.model,asset(c,"UI/Foundations/Components/Notices/BasicNotice_Widget"),initial.anchor,initial.item,0x4d4f4500,false);
        if(!s.hud_stack.handle || c.type_of(s.hud_stack.handle)!=s.hud_stack.type){dr::retire();return false;}
    }
    h.stack=s.hud_stack;h.game=v.game;
    const bool ready=hud_publication_allowed(read<bool>(c.address(c.field(h.stack,0x369babfe))),s.in_menu,s.match_panel_open.load());
    if(!native.publication.allow(h.stack.handle,v.game,ready,now)){
        publish_hud_surface(c,h.stack,{},true);
        // A normal native stack remount waits 100 ms. Keep only this race's
        // fresh immutable frame during that gap; a real menu/map retires it.
        const auto held=dr::presentation_at(now);
        if(s.in_menu || s.match_panel_open.load() || !held || held->world!=v.world || held->game!=v.game)dr::retire();
        native.visible=false;return true;
    }
    // The compass remains the game's native marker. ReSkate's renderer owns
    // the complete Deathrace surface, so legacy/native cell grids cannot mix.
    std::vector<Value> visible;
    try {mode_gate_compass(c,v,feed);for(const auto& marker:h.gate_compass)if(marker.visible)visible.push_back(marker.item);}catch(const std::exception&){}
    publish_hud_surface(c,h.stack,visible,true);
    publish_deathrace_presentation(c,v,now);native.visible=true;
    static std::uint64_t renderer_world{},renderer_started{};static std::uint32_t renderer_game{};static bool was_waiting=true;
    if(renderer_world!=v.world || renderer_game!=v.game || (was_waiting && !v.waiting)){renderer_world=v.world;renderer_game=v.game;renderer_started=now;}
    was_waiting=v.waiting;
    if(!v.waiting && now>=renderer_started && now-renderer_started>1500 && !dr::current(v.world,v.game,now)){native.visible=false;return false;}
    return true;
}
void mode_hud(const Context& c) {
    try {if(native_deathrace_hud(c))return;}
    catch(const std::exception& error){
        auto& native=state().mode_hud.native_race;const auto now=GetTickCount64();
        if(now>=native.retry_at)logging::log(logging::Level::warning,logging::Channel::ui,"Deathrace native presentation: {}; retained legacy fallback.",error.what());
        native.retry_at=now+1500;native.visible=false;deathrace_ui::retire();
    }
    legacy_mode_hud(c);
}
