// Coordinates reproduce the approved Skate 2 adaptation. Every interactive
// rectangle comes from RouteLayout, shared with the existing guarded input.
inline Scene compose(const Input& in) {
    Scene s;s.width=Frame(in.aspect).width;
    if(in.screen==Screen::hidden || in.screen==Screen::lobby)return s;
    const auto label=[&](Rect r,std::string text,unsigned points=24,unsigned align=0,bool marker=false,Paint paint=Paint::text){s.box(r,paint,safe_text(std::move(text)),points,align,marker);};
    const auto title=[&](float x,float y,float w,std::string text,unsigned points=28){label({x,y,w,36},std::move(text),points,0,true);};
    const auto line=[&](Rect r){s.box(r,Paint::line);};
    const auto button=[&](Rect r,std::string text,bool selected,const char* input="UI_PrimaryAction"){
        s.box(r,selected?Paint::selected:Paint::soft);s.cells.back().interactive=true;
        s.glyph({r.x+12,r.y+(r.h-40)*.5f,40,40},input);
        label({r.x+62,r.y+(r.h-34)*.5f,r.w-76,34},std::move(text),23);
    };
    const auto header=[&](Rect r,std::string text,std::string sub){s.image({r.x+25,r.y+22,56,56},logo);label({r.x+104,r.y+20,r.w-132,56},std::move(text),40,0,true);if(!sub.empty())label({r.x+28,r.y+87,r.w-56,33},std::move(sub),25);line({r.x+28,r.y+132,r.w-56,1});};
    const auto controls=[&](float x,float y,float w,const char* a,const char* b,std::string text,unsigned points=23){s.glyph({x,y-3,40,40},a);if(*b)s.glyph({x+42,y-3,40,40},b);label({x+94,y,w-94,34},std::move(text),points);};
    const bool builder=in.screen==Screen::builder,rotate=in.screen==Screen::rotate;
    if(builder || rotate){
        const float x=52,y=184,w=420;s.box({x,y,w,604},Paint::panel);s.image({x+20,y+20,56,56},logo);
        label({x+90,y+18,w-110,52},rotate?"ROTATE OBJECT":"DEATHRACE",35,0,true);label({x+93,y+68,w-116,23},rotate?(in.arrow?(in.roll_axis?"ARROW / ROLL":"ARROW / TILT"):"GATE ORIENTATION"):"BUILDER",19);
        line({x+24,y+94,w-48,1});
        if(builder){
            struct Control{const char* a;const char* b;std::string text;};
            const std::array<Control,11> rows{{{"UI_TabSecondaryNavLeft","UI_TabSecondaryNavRight","Object: "+in.object},{"UI_TabPrimaryNavLeft","UI_TabPrimaryNavRight","Camera down / up"},{"UI_PrimaryAction","","Place object"},{"UI_SecondaryAction","","Rotate / 5 degrees"},{"UI_DpadLeft","UI_DpadRight","Undo / Redo"},{"UI_DpadUp","UI_DpadDown",std::format("Size {:.0f}%",in.width)},{"UI_Activities_AccessPause","",in.reusable?"Tap: Play / Hold: Load":"Tap: Save / Hold: Load"},{"UI_Back","","Close editor"},{"UI_TertiaryAction","",in.snap?"Snap ON":"Snap OFF"},{"UI_R3Click","",std::format("Placement snap: {:.0f} deg",in.snap_degrees)},{"UI_SelectButton","","Hold: Clear route"}}};
            for(unsigned i=0;i<rows.size();++i)controls(x+23,y+108+37*i,w-46,rows[i].a,rows[i].b,rows[i].text);
            if(in.save_hold>0)s.progress({x+111,y+108+37*6+32,275,3},in.save_hold,Paint::red);
            if(in.clear_hold>0)s.progress({x+111,y+108+37*10+32,275,3},in.clear_hold,Paint::red);
            line({x+24,y+528,w-48,1});label({x+25,y+535,w-50,26},"MOVEMENT",20);
            controls(x+23,y+567,w-46,"UI_L3Click","","Hold / faster flight",23);
        }else{
            const std::array names{"YAW","TILT","ROLL"};const std::array values{in.yaw,in.pitch,in.roll};
            for(unsigned i=0;i<3;++i){label({x+25+127*i,y+113,115,25},names[i],19,2);label({x+25+127*i,y+149,115,47},degrees(values[i]),34,2);}
            controls(x+23,y+225,w-46,"UI_DpadLeft","UI_DpadRight","Yaw left / right");
            controls(x+23,y+279,w-46,"UI_DpadUp","UI_DpadDown",in.roll_axis?"Roll up / down":"Tilt up / down");
            controls(x+23,y+338,w-46,"UI_L3Click","","Reset original orientation");
            controls(x+23,y+393,w-46,"UI_R3Click","",in.arrow?"Switch tilt / roll axis":"Original tilt and yaw");
            controls(x+23,y+448,w-46,"UI_SecondaryAction","","Finish rotating");
            line({x+24,y+516,w-48,1});label({x+25,y+534,w-50,28},"5-degree steps",23);controls(x+25,y+567,w-50,"UI_DpadHorizontal","","Hold / turn faster",19);
        }
        const float ix=s.width-340,iy=184,iw=288;s.box({ix,iy,iw,334},Paint::panel);title(ix+12,iy+12,iw-24,"ROUTE INFO",28);
        const std::array<std::pair<std::string,std::string>,7> rows{{{"Required gates",std::to_string(in.required)},{"Guide gates",std::to_string(in.guides)},{"Arrows",std::to_string(in.arrows)},{"Finish gate",in.finish?"Placed":"Needed"},{"Height",std::format("{:.1f} m",in.height)},{"Size",std::format("{:.0f}%",in.width)},{"Route length",std::format("{:.0f} m",in.distance)}}};
        for(unsigned i=0;i<rows.size();++i){label({ix+15,iy+67+32*i,167,29},rows[i].first,23);label({ix+183,iy+67+32*i,90,29},rows[i].second,23,1);}
        if(!in.error.empty())label({(s.width-620)*.5f,860,620,65},in.error,26,2,false,Paint::danger);
        return s;
    }
    if(in.screen==Screen::flag){const Rect r{52,184,420,194};s.box(r,Paint::panel);header(r,"STARTING FLAG","");label({r.x+25,r.y+104,r.w-50,74},"Choose a grounded starting point\nand facing direction.",24);return s;}
    if(in.screen==Screen::load || in.screen==Screen::save || in.screen==Screen::rename || in.screen==Screen::erase){
        const bool list=in.screen==Screen::load;const RouteLayout layout(s.width,list);const auto r=layout.box;s.box(r,Paint::panel);
        header(r,list?"ROUTE LIBRARY":in.screen==Screen::erase?"DELETE ROUTE?":in.screen==Screen::rename?"RENAME ROUTE":"SAVE ROUTE",in.map);
        if(list){
            label({r.x+r.w-235,r.y+91,207,28},std::format("{} / {} ROUTES",in.routes.empty()?0:in.selected_route+1,in.routes.size()),20,1,false,Paint::muted);
            if(in.routes.empty()){
                label({r.x+28,r.y+205,r.w-56,38},"No saved routes on this map.",26,2);
                controls(r.x+210,r.y+256,r.w-390,"UI_Activities_AccessPause","","Build a course, then tap to save",23);
            }
            const unsigned first=in.routes.empty()?0:std::min(in.first_route,static_cast<unsigned>(in.routes.size()-1));
            for(unsigned i=first;i<std::min<unsigned>(static_cast<unsigned>(in.routes.size()),first+4);++i){const auto& route=in.routes[i];const auto row=layout.row(i-first);s.box(row,i==in.selected_route?Paint::selected:Paint::soft);if(i==in.selected_route)label({row.x+14,row.y+17,24,30},">",25,2,false,Paint::gold);label({row.x+46,row.y+2,row.w-60,34},route.name,29);label({row.x+46,row.y+35,row.w-60,24},std::format("{} Required + Finish  /  {} objects",route.required,route.objects),20,0,false,Paint::muted);}
            const std::array names{"Load route","Rename","Show folder","Delete","Load .json","Back to builder"};
            const std::array inputs{"UI_PrimaryAction","UI_SecondaryAction","UI_TabPrimaryNavRight","UI_TertiaryAction","UI_TabPrimaryNavLeft","UI_Back"};
            for(unsigned i=0;i<names.size();++i)button(layout.action(i),names[i],in.action_focus && i==in.route_action,inputs[i]);
            if(in.error.empty()){
                controls(r.x+28,r.y+573,390,"UI_DpadHorizontal","UI_LeftStick","Navigate",20);
                controls(r.x+425,r.y+573,230,"UI_PrimaryAction","","Select",20);
                label({r.x+675,r.y+573,r.w-703,34},"Buttons are shortcuts",18,1,false,Paint::muted);
            }
        }else{
            if(in.screen==Screen::erase){label({r.x+28,r.y+160,r.w-56,65},in.name_entry,34,2);label({r.x+28,r.y+243,r.w-56,84},"Delete this saved route?\nThis also removes its shareable JSON file.",26,2);button(layout.action(0),"Cancel",in.route_action==0,"UI_Back");button(layout.action(1),"Delete route",in.route_action==1);}
            else{label({r.x+28,r.y+156,r.w-56,35},std::format("{} Required gates + Finish",in.required),25);label({r.x+28,r.y+212,r.w-56,29},"Route name / type with your keyboard",21);label(layout.field(),in.name_entry.empty()?"Name your route":in.name_entry,28,0,false,Paint::field);label({r.x+28,r.y+339,r.w-56,31},"Shareable JSON / flag position and objects saved",21,0,false,Paint::muted);button(layout.action(0),in.screen==Screen::rename?"Save name":"Save route",in.route_action==0);button(layout.action(1),"Cancel",in.route_action==1,"UI_Back");}
        }
        if(!in.error.empty())label({r.x+28,list?r.y+556:r.y+r.h+15,r.w-56,list?48.f:56.f},in.error,23,2,false,Paint::danger);
        return s;
    }
    if(in.screen==Screen::exit_builder){const Rect r{(s.width-760)*.5f,190,760,340};s.box(r,Paint::panel);header(r,"LEAVE BUILDER?",in.route);label({r.x+28,r.y+151,r.w-56,82},in.saved?"Your saved route is ready to play.":"Your unfinished draft stays private.\nSave the route before sharing it with other players.",26,2);button({r.x+28,r.y+268,337,49},"Leave editor",false);button({r.x+395,r.y+268,337,49},"Keep editing",false,"UI_Back");return s;}
    if(in.screen==Screen::intro){const Rect r{(s.width-780)*.5f,190,780,335};s.box(r,Paint::panel);header(r,"DEATHRACE",in.route);label({r.x+28,r.y+155,r.w-56,84},"Pass Required gates in order, then the red Finish.\nGuide gates are optional. Off-board is allowed.",26,2);controls(r.x+165,r.y+268,r.w-300,"UI_DpadUp","","Return behind previous gate",23);return s;}
    const bool results=in.screen==Screen::results,done=in.screen==Screen::finished || in.screen==Screen::dnf;
    const auto rows=[&](float x,float y,float w,bool final){
        const auto count=std::min<std::size_t>(6,in.rows.size());
        for(unsigned i=0;i<count;++i){const auto& row=in.rows[i];const float yy=y+50*i;s.box({x,yy,w,44},row.self?Paint::selected:Paint::soft);if(!row.dnf)s.position({x+12,yy+2,final?70.f:48.f,40},row.place,final?35:30);else label({x+10,yy+5,final?70.f:48.f,34},"DNF",21,2);
            const float name_x=x+(final?100.f:65.f),time_width=final?168.f:105.f;label({name_x,yy+2,w-(name_x-x)-time_width-24,40},row.name,final?28:24,0,false,Paint::player_name);label({x+w-time_width-12,yy+1,time_width,final?40.f:23.f},row.time,final?25:21,1);if(!final)label({x+w-time_width-12,yy+25,time_width,19},row.detail,16,1,false,Paint::muted);
        }
    };
    if(results){const Rect r{(s.width-900)*.5f,178,900,610};s.box(r,Paint::panel);header(r,"DEATHRACE RESULTS",in.route+" / "+in.map);label({r.x+28,r.y+151,r.w-56,52},in.winner.empty()?"NO FINISHERS":in.winner+" WINS!",40,2,true,Paint::gold);label({r.x+40,r.y+210,65,26},"POS",19);label({r.x+128,r.y+210,400,26},"PLAYER",19);label({r.x+r.w-200,r.y+210,160,26},"TIME",19,1);rows(r.x+28,r.y+247,r.w-56,true);return s;}
    const float inset=in.options.safe-16;const float k=std::min(1.10f,in.options.scale),x=46+inset,y=190,w=296;
    const float brand_w=std::min(296.f,(s.width-800)*.5f-(57+inset)-8);
    s.box({46+inset,44,brand_w,104},Paint::panel);s.image({58+inset,55,48,48},logo);label({118+inset,55,brand_w-84,48},"DEATHRACE",31,0,true);label({60+inset,108,brand_w-28,29},in.route,21);
    const auto begin=s.cells.size();s.box({46,y,w,520},Paint::panel);
    title(58,204,260,"YOUR POSITION",26);if(in.position)s.position({60,233,260,107},in.position,101,in.players);else label({60,235,150,95},"DNF",54,0,true);
    title(58,361,260,done?"FINAL TIME":"TIME LEFT",26);s.box({58,403,270,59},Paint::race_clock,done?in.elapsed:in.remaining,51);
    label({60,470,260,30},"LIMIT  "+clock(std::uint64_t(in.deadline)*1000),21);line({58,509,245,1});title(58,529,260,"REQUIRED GATES",26);
    label({58,579,263,52},std::format("{} / {}",in.cleared,in.required),39,0,false,Paint::gold);label({60,641,260,51},in.cleared>=in.required?"Red Finish unlocked":"Follow the yellow markers",20);
    for(auto i=begin;i<s.cells.size();++i){auto& r=s.cells[i].box;r.x=x+(r.x-46)*k;r.y=y+(r.y-y)*k;r.w*=k;r.h*=k;s.cells[i].points=static_cast<unsigned>(std::lround(s.cells[i].points*k));}
    if(in.options.standings && !in.rows.empty()){const float bx=s.width-404-inset,by=190,bw=350;s.box({bx,by,bw,76+50.f*std::min<std::size_t>(6,in.rows.size())},Paint::panel);title(bx+18,by+14,bw-36,"LIVE STANDINGS",27);rows(bx+10,by+70,bw-20,false);}
    // World-space type/number labels belong to authoring only. The native
    // compass and stable colored world gates provide race guidance.
    std::string event=in.notification;if(event=="GO!" || event.find("passed")!=std::string::npos || event.find("Passed")!=std::string::npos)event.clear();
    if(event.empty() && in.recovering)event="Recover now / bail reset after 1 second";
    if(event.empty() && done)event=in.screen==Screen::dnf?"DID NOT FINISH":"FINISHED!";
    if(!event.empty())label({(s.width-620)*.5f,750,620,67},event,26,2,false,Paint::danger);
    // The game's top-right native prompt already owns Previous gate.
    if(in.screen==Screen::countdown || in.screen==Screen::go){
        const bool go=in.screen==Screen::go;const float cw=go?180.f:112.f,ch=go?123.f:154.f;
        s.box({(s.width-cw)*.5f,1080.f*.42f,cw,ch},Paint::countdown,go?"GO!":std::to_string(std::clamp(in.countdown,1u,3u)),112,2,true);
    }
    return s;
}
