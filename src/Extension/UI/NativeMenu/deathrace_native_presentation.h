#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <format>
#include <string>
#include <vector>
#include "deathrace_native_font_metrics.h"
#include "deathrace_route_navigation.h"
#include "deathrace_redline.h"
#include "Extension/Modes/race_skate2_presentation.h"
// Presentation only. The referee, route library and existing placement/input
// state machines remain the owners of all game actions and measurements.
namespace dingosdk::deathrace_ui {
enum class Screen { hidden, flag, builder, rotate, exit_builder, save, load, rename, erase,
    lobby, intro, countdown, go, racing, finished, dnf, results };
enum class Paint { text, panel, soft, red, yellow, brush, paper, field, ink, muted, accent, line, selected, success, danger, gate_tag, guide_tag, finish_tag, podium, countdown, gate_frame, race_clock, race_label, race_notice, gate_target, gold, player_name };
inline bool race_surface(Screen s) {return s==Screen::countdown || s==Screen::go || s==Screen::racing || s==Screen::finished || s==Screen::dnf;}
inline Screen transition_family(Screen s) {return race_surface(s)?Screen::racing:s;}
struct Rect { float x{},y{},w{},h{}; };
struct Cell {
    Rect box; Paint paint=Paint::text; std::string text,image,input;
    unsigned points=24,align{}; bool marker{},wrap{}; float fraction=1,alpha=1; bool interactive{};unsigned rank{},total{};
};
inline std::string bounded_text(const std::string& value,float width,float height,unsigned points,bool wrap,bool marker=false) {
    // Explicit line breaks plus an ellipsis bound even one unbroken gamertag.
    // Conservative native-font advances leave room for glyph overhang/shadow.
    const unsigned lines=wrap?std::max(1u,static_cast<unsigned>(height/(points*1.15f))):1u;
    const float limit=std::max(1.f,width-8.f);
    std::string result;float advance{};unsigned line=1;
    const auto step_for=[&](unsigned char ch){return points*(ch<128?(marker?native_marker_advance[ch]:native_gilroy_advance[ch])*1.02f:1.03f);};
    std::size_t break_result=std::string::npos,break_source{};
    const auto ellipsis=[&]{
        while(!result.empty() && advance+step_for('.')*3>limit && result.back()!='\n'){
            auto at=result.size()-1;while(at && (static_cast<unsigned char>(result[at])&0xc0)==0x80)--at;
            advance=std::max(0.f,advance-step_for(static_cast<unsigned char>(result[at])));result.resize(at);
        }
        result+="...";
    };
    for(std::size_t i=0;i<value.size();){
        const unsigned char ch=value[i];unsigned bytes=ch<128?1:(ch&0xe0)==0xc0?2:(ch&0xf0)==0xe0?3:4;
        bytes=static_cast<unsigned>(std::min<std::size_t>(bytes,value.size()-i));
        if(ch=='\n'){
            if(line>=lines){ellipsis();break;}result+='\n';advance=0;++line;++i;break_result=std::string::npos;continue;
        }
        const float step=step_for(ch);
        if(advance+step>limit){
            if(line>=lines){
                ellipsis();break;
            }
            if(break_result!=std::string::npos){result.resize(break_result);i=break_source;result+='\n';advance=0;++line;break_result=std::string::npos;continue;}
            result+='\n';advance=0;++line;break_result=std::string::npos;
        }
        if(ch==' '){break_result=result.size();break_source=i+1;}
        result.append(value,i,bytes);advance+=step;i+=bytes;
    }
    return result;
}
struct Row { std::string name,time,detail; bool self{},finished{},dnf{}; unsigned place{}; };
// One uniform fit maps every cell and pointer hit to the same physical frame.
// Extremely narrow or wide windows keep a centered layout instead of pulling
// text away from its backing panel.
struct Frame {
    float actual_width{},width{},scale{};
    explicit Frame(float aspect) {
        actual_width=1080.f*(std::isfinite(aspect) && aspect>0?aspect:16.f/9.f);
        width=std::clamp(actual_width,1440.f,3840.f);scale=std::min(1.f,actual_width/width);
    }
    std::array<float,2> normalized(float x,float y) const {return {.5f+(x-width*.5f)*scale/actual_width,.5f+(y-540.f)*scale/1080.f};}
    std::array<float,2> logical(float x,float y) const {return {width*.5f+(x-.5f)*actual_width/scale,540.f+(y-.5f)*1080.f/scale};}
};
struct Route { std::uint64_t id{};std::string name,map;unsigned required{},objects{}; };
struct Input {
    Screen screen=Screen::hidden;
    float aspect=16.f/9.f;
    std::string player,host,route,map,object="Required gate",error,notification,elapsed="00:00.000",remaining="02:00.000",winner;
    std::string name_entry,folder;
    unsigned required{},cleared{},guides{},arrows{},players=1,capacity=6,position{},deadline=120,countdown=5;
    unsigned selected_route{},route_action{},first_route{};
    bool action_focus{},restoring_flag{},host_local{},finish{},saved{},reusable{},snap=true,roll_axis{},arrow{},recovering{},compass=true;
    float yaw{},pitch{},roll{},width=125,snap_degrees=90,height{},distance{},clear_hold{},save_hold{},object_alpha=1;
    RedlineOptions options;GateProjection navigation;bool have_speed{},next_finish{},have_gate{};float speed_mps{},gate_distance{},gate_pulse{};
    unsigned next_gate{};std::uint64_t sampled_ms{};
    std::vector<Row> rows;std::vector<Route> routes;skate2_ribbons::View world_ribbons;
};
inline constexpr std::size_t max_cells=96;
inline std::string asset_name(const Cell& c) {
    const auto base=std::format("UI/ReSkate/DeathraceNative/Cell_{}_{}_{}_{}_{}_{}_{}",
        static_cast<unsigned>(c.paint),static_cast<unsigned>(std::lround(c.box.w*2)),
        static_cast<unsigned>(std::lround(c.box.h*2)),c.points*2,c.align,c.marker?1:0,c.wrap?1:0);
    return base+(c.alpha<1?std::format("_A{}",static_cast<unsigned>(std::lround(std::clamp(c.alpha,0.f,1.f)*15))):"")+"_Widget";
}
struct Scene {
    float width=1920,height=1080;
    std::vector<Cell> cells;
    void box(Rect r,Paint p,std::string text={},unsigned points=24,unsigned align=0,bool marker=false,bool wrap=false) {
        if(marker)for(auto& c:text)if(c>='a' && c<='z')c=static_cast<char>(c-'a'+'A');
        cells.push_back({r,p,text,{}, {},points,align,marker,wrap});
    }
    void text(Rect r,std::string value,unsigned points=24,unsigned align=0,bool marker=false,bool wrap=false) {
        box(r,Paint::text,std::move(value),points,align,marker,wrap);
    }
    void image(Rect r,std::string name) { Cell c;c.box=r;c.image=std::move(name);cells.push_back(std::move(c)); }
    void glyph(Rect r,std::string name) {Cell c;c.box=r;c.input=std::move(name);cells.push_back(std::move(c));}
    void position(Rect r,unsigned place,unsigned points=48,unsigned total=0) {box(r,Paint::podium,std::to_string(place),points,0,true);cells.back().rank=place;cells.back().total=total;}
    void progress(Rect r,float amount,Paint color) {box(r,Paint::soft);box(r,color);cells.back().fraction=std::clamp(amount,0.f,1.f);}
};
inline constexpr const char* logo="UI/ReSkate/Throwdowns/Badge_Race";
inline std::string image_asset_name(const Cell& c) {
    const unsigned id=c.image==logo?0:c.image=="native-rank-1"?1:c.image=="native-rank-2"?2:c.image=="native-rank-3"?3:
        c.image.find("TimerOutline")!=std::string::npos?4:5;
    return std::format("UI/ReSkate/DeathraceNative/Image_{}_{}_{}_Widget",id,
        static_cast<unsigned>(std::lround(c.box.w*2)),static_cast<unsigned>(std::lround(c.box.h*2)));
}
inline constexpr const char* input_asset="UI/ReSkate/DeathraceNative/BoundedInputGlyph_Widget";
inline constexpr float input_native_size=56.f;
inline std::string clock(std::uint64_t ms) {return std::format("{:02}:{:02}.{:03}",ms/60000,(ms/1000)%60,ms%1000);}
inline std::string safe_text(std::string text) {
    // The private native labels disable markup. Preserve literal player names.
    for(auto& c:text)if(static_cast<unsigned char>(c)<32 && c!='\n')c=' ';
    return text;
}
inline std::string_view place_suffix(unsigned n) {const auto tail=n%100;return tail>=11 && tail<=13?"th":n%10==1?"st":n%10==2?"nd":n%10==3?"rd":"th";}
inline std::string ordinal_total(unsigned n) {return "/"+std::to_string(n)+std::string(place_suffix(n));}
inline std::string degrees(float n) {if(std::abs(n)<.05f)n=0;return std::format("{:.0f} deg",n);}
struct RouteLayout {
    Rect box;float rows_y{},row_h=62,footer_y{};bool list{};
    explicit RouteLayout(float width,bool list_value):box{(width-(list_value?980.f:800.f))*.5f,list_value?178.f:190.f,list_value?980.f:800.f,list_value?614.f:466.f},list(list_value) {
        rows_y=box.y+146;footer_y=box.y+451;
    }
    Rect row(unsigned index) const {return {box.x+28,rows_y+70*index,box.w-56,row_h};}
    Rect field() const {return {box.x+28,box.y+254,box.w-56,58};}
    Rect action(unsigned index) const {
        if(!list)return {box.x+(index?418.f:28.f),box.y+394,354,46};
        return {box.x+28+310*(index%3),footer_y+53*(index/3),298,45};
    }
};
inline Rect compass_reserved(float width){return {(width-800)*.5f,0,800,166};}
inline Rect dpad_reserved(float width){return {0,800,std::min(460.f,width*.24f),280};}
inline bool overlaps(Rect a,Rect b){return a.x<b.x+b.w && a.x+a.w>b.x && a.y<b.y+b.h && a.y+a.h>b.y;}
inline Scene compose_legacy(const Input& in) {
    Scene s;s.width=Frame(in.aspect).width;
    // Join/Leave and Hold Start belong to the game's authored notification.
    if(in.screen==Screen::hidden || in.screen==Screen::lobby)return s;
    const auto label=[&](Rect r,std::string t,double size=22,unsigned align=0,bool marker=false,bool wrap=false,Paint paint=Paint::text){s.box(r,paint,safe_text(std::move(t)),static_cast<unsigned>(std::lround(size)),align,marker,wrap);};
    const auto rule=[&](Rect r){s.box(r,Paint::line);};
    const auto header=[&](Rect r,std::string title,std::string sub={}){
        s.box({r.x,r.y,r.w,78},Paint::brush);s.image({r.x+18,r.y+9,58,58},logo);
        if(title=="DEATHRACE BUILDER"){
            label({r.x+94,r.y+8,r.w-116,38},"DEATHRACE",29,0,true);
            label({r.x+96,r.y+48,r.w-118,20},"BUILDER",15);
        }else label({r.x+94,r.y+8,r.w-116,54},title,32,0,true);
        if(!sub.empty())label({r.x+24,r.y+92,r.w-48,28},sub,20,0,false,false,Paint::muted);
    };
    const auto action=[&](Rect r,std::string t,bool selected=false){s.box(r,selected?Paint::selected:Paint::soft,std::move(t),19,2);s.cells.back().interactive=true;};
    const auto center=[&](float w,float h){return Rect{(s.width-w)*.5f,(1080-h)*.5f,w,h};};
    const bool editing=in.screen==Screen::builder || in.screen==Screen::rotate;
    if(editing){
        const float x=40,y=176,w=340;const bool rotate=in.screen==Screen::rotate;
        s.box({x,y,w,rotate?432.f:518.f},Paint::panel);
        header({x,y,w,78},rotate?"ROTATE OBJECT":"DEATHRACE BUILDER");
        if(rotate){
            const std::array names{"YAW","TILT","ROLL"};const std::array values{in.yaw,in.pitch,in.roll};
            for(unsigned i=0;i<3;++i){const float xx=x+16+104*i;s.box({xx,y+88,100,70},Paint::soft);label({xx,y+92,100,22},names[i],16,2,false,false,Paint::muted);label({xx,y+118,100,32},degrees(values[i]),23,2);}
            const std::array<std::pair<const char*,const char*>,5> rows{{{"UI_DpadLeft","Yaw left / right"},{"UI_DpadUp",in.roll_axis?"Arrow up / down":"Tilt up / down"},{"UI_L3Click","Reset original orientation"},{"UI_R3Click",in.arrow?"Switch tilt / arrow axis":"Original tilt and yaw"},{"UI_SecondaryAction","Finish rotating"}}};
            for(unsigned i=0;i<rows.size();++i){const float yy=y+178+40*i;s.glyph({x+20,yy,30,30},rows[i].first);label({x+66,yy-3,w-86,34},rows[i].second,18);}
            rule({x+18,y+390,w-36,1});label({x+18,y+400,w-36,24},"5-degree steps / hold to turn faster",15,0,false,false,Paint::muted);
        }else{
            struct Control{const char* a;const char* b;std::string text;};
            const std::array<Control,11> rows{{{"UI_TabSecondaryNavLeft","UI_TabSecondaryNavRight",in.object},{"UI_TabPrimaryNavLeft","UI_TabPrimaryNavRight","Camera down / up"},{"UI_PrimaryAction","","Place object"},{"UI_SecondaryAction","","Rotate / 5 degrees"},{"UI_DpadLeft","UI_DpadRight","Undo / Redo"},{"UI_DpadUp","UI_DpadDown",std::format("Size {:.0f}%",in.width)},{"UI_Activities_AccessPause","",in.reusable?"Tap: Play / Hold: Load":"Tap: Save / Hold: Load"},{"UI_Back","","Close editor"},{"UI_TertiaryAction","",in.snap?"Snap ON":"Snap OFF"},{"UI_R3Click","",std::format("Placement snap: {:.0f} deg",in.snap_degrees)},{"UI_SelectButton","","Hold: Clear route"}}};
            for(unsigned i=0;i<rows.size();++i){const float yy=y+88+36*i;s.glyph({x+18,yy,28,28},rows[i].a);if(*rows[i].b)s.glyph({x+52,yy,28,28},rows[i].b);label({x+92,yy-3,w-108,34},rows[i].text,18);}
            s.progress({x+92,y+88+36*6+30,w-110,3},in.save_hold,Paint::red);
            s.progress({x+92,y+88+36*10+30,w-110,3},in.clear_hold,Paint::red);
            rule({x+18,y+488,w-36,1});label({x+18,y+494,w-36,20},"PRIVATE DRAFT",14,0,false,false,Paint::muted);
        }
        const float ix=s.width-314,iy=184,iw=274;s.box({ix,iy,iw,334},Paint::panel);
        s.box({ix,iy,iw,48},Paint::brush,"ROUTE INFO",22,2,true);rule({ix+20,iy+56,iw-40,1});
        const std::array<std::pair<std::string,std::string>,7> rows{{{"Required gates",std::to_string(in.required)},{"Guide gates",std::to_string(in.guides)},{"Arrows",std::to_string(in.arrows)},{"Finish gate",in.finish?"Placed":"Needed"},{"Height",std::format("{:.1f} m",in.height)},{"Size",std::format("{:.0f}%",in.width)},{"Route length",std::format("{:.0f} m",in.distance)}}};
        for(unsigned i=0;i<rows.size();++i){label({ix+20,iy+68+30*i,154,26},rows[i].first,17,0,false,false,Paint::muted);label({ix+174,iy+68+30*i,80,26},rows[i].second,18,1);}
        s.box({ix+18,iy+292,iw-36,28},in.saved?Paint::success:Paint::soft,in.saved?"SAVED ROUTE":"LOCAL DRAFT / PRIVATE",15,2);
        if(in.object_alpha>0){s.box({(s.width-330)*.5f,190,330,52},Paint::soft,in.object,23,2);s.cells.back().alpha=in.object_alpha;}
        if(!in.error.empty())s.box({(s.width-640)*.5f,864,640,64},Paint::danger,in.error,21,2,false,true);
        return s;
    }
    if(in.screen==Screen::save || in.screen==Screen::load || in.screen==Screen::rename || in.screen==Screen::erase){
        const bool list=in.screen==Screen::load;const RouteLayout layout(s.width,list);const auto r=layout.box;
        s.box(r,Paint::panel);header(r,list?"ROUTE LIBRARY":in.screen==Screen::erase?"DELETE ROUTE?":in.screen==Screen::rename?"RENAME ROUTE":"SAVE ROUTE",list?std::string{}:in.map);
        if(list){
            label({r.x+24,r.y+92,274,28},in.map,20,0,false,false,Paint::muted);
            label({r.x+300,r.y+94,260,24},"D-pad / Left stick to navigate",15,0,false,false,Paint::muted);
            label({r.x+r.w-240,r.y+94,216,24},std::format("{} / {} ROUTES",in.routes.empty()?0:in.selected_route+1,in.routes.size()),15,1,false,false,Paint::muted);
            if(in.routes.empty())label({r.x+30,r.y+204,r.w-60,106},"No saved routes on this map.\nBuild a course, then tap Start to save it.",24,2,false,true);
            for(unsigned i=in.first_route;i<std::min<unsigned>(static_cast<unsigned>(in.routes.size()),in.first_route+5);++i){const auto& route=in.routes[i];const auto row=layout.row(i-in.first_route);
                s.box(row,i==in.selected_route?Paint::selected:Paint::soft);s.cells.back().interactive=true;
                label({row.x+18,row.y+3,row.w-36,26},route.name,21);label({row.x+18,row.y+30,row.w-36,20},std::format("{} Required + Finish   /   {} objects",route.required,route.objects),15,0,false,false,Paint::muted);
            }
            const std::array actions{"Load route","Rename","Show folder","Delete","Load .json file","Back to builder"};
            for(unsigned i=0;i<actions.size();++i)action(layout.action(i),actions[i],in.action_focus && i==in.route_action);
            const std::array<std::pair<const char*,const char*>,6> controls{{{"UI_PrimaryAction","Load / select"},{"UI_SecondaryAction","Rename"},{"UI_TertiaryAction","Delete"},{"UI_TabPrimaryNavLeft","Import JSON"},{"UI_TabPrimaryNavRight","Show folder"},{"UI_Back","Back"}}};
            for(unsigned i=0;i<controls.size();++i){const float xx=r.x+24+282*(i%3),yy=r.y+534+36*(i/3);s.glyph({xx,yy,28,28},controls[i].first);label({xx+36,yy,234,28},controls[i].second,17,0,false,false,Paint::muted);}
            if(!in.error.empty())label({r.x+24,layout.footer_y-24,r.w-48,20},in.error,16,0,false,false,Paint::accent);
        }else{
            if(in.screen==Screen::erase){label({r.x+28,r.y+152,r.w-56,78},"Delete \""+in.route+"\"?",28,2,false,true);label({r.x+28,r.y+252,r.w-56,40},"The file moves to .deleted so it can be recovered.",19,2,false,false,Paint::muted);}
            else{label({r.x+28,r.y+146,r.w-56,28},std::format("{} Required gates + Finish",in.required),20,0,false,false,Paint::muted);
                s.box(layout.field(),Paint::field,in.name_entry.empty()?"Name your route":in.name_entry,27,0,false,true);
                label({r.x+28,r.y+286,r.w-56,28},"Type with your keyboard / shareable JSON",18,0,false,false,Paint::muted);
                label({r.x+r.w-128,r.y+146,100,28},std::format("{} / 64",in.name_entry.size()),15,1,false,false,Paint::muted);
            }
            if(!in.error.empty())label({r.x+28,r.y+328,r.w-56,44},in.error,19,0,false,true,Paint::accent);
            action(layout.action(0),in.screen==Screen::erase?"Cancel":in.screen==Screen::rename?"Save name":"Save route",in.route_action==0);
            action(layout.action(1),in.screen==Screen::erase?"Delete route":"Cancel",in.route_action==1);
        }return s;
    }
    if(in.screen==Screen::exit_builder){const auto r=center(740,300);s.box(r,Paint::panel);header(r,"LEAVE BUILDER?");label({r.x+28,r.y+106,684,66},"Your unfinished draft stays private.\nSave it before sharing with other players.",22,2,false,true);
        s.glyph({r.x+30,r.y+224,32,32},"UI_PrimaryAction");label({r.x+78,r.y+218,250,42},"Leave editor",21);s.glyph({r.x+390,r.y+224,32,32},"UI_Back");label({r.x+438,r.y+218,270,42},"Keep editing",21);return s;}
    if(in.screen==Screen::flag){s.box({40,190,392,138},Paint::panel);header({40,190,392,78},in.restoring_flag?"LOADING ROUTE":"STARTING FLAG");label({62,282,348,30},in.restoring_flag?"Restoring your saved starting flag...":"Choose your point and facing direction.",18,0,false,false,Paint::muted);return s;}
    if(in.screen==Screen::intro){const auto r=center(760,342);s.box(r,Paint::panel);header(r,"DEATHRACE",in.route);
        label({r.x+32,r.y+152,r.w-64,82},"Pass the yellow Required gates in order.\nCross the red Finish to stop your clock.",25,2,false,true);
        s.box({r.x+30,r.y+270,r.w-60,46},Paint::soft,"D-pad Up / return to your previous gate",19,2);return s;}
    if(in.screen==Screen::countdown || in.screen==Screen::go){const auto r=center(300,260);const bool go=in.screen==Screen::go;
        s.box({r.x+55,r.y+8,190,28},Paint::brush,go?"RACE START":"GET READY",17,2,true);
        s.box({r.x-20,r.y+42,340,154},Paint::countdown,go?"GO!":std::to_string(in.countdown),124,2,true);
        for(unsigned i=0;i<3;++i)s.box({r.x+94+40*i,r.y+210,32,4},go || in.countdown<=3-i?Paint::red:Paint::soft);
        label({r.x-100,r.y+232,500,28},in.route,19,2,false,false,Paint::muted);return s;}
    const bool results=in.screen==Screen::results,done=in.screen==Screen::finished || in.screen==Screen::dnf;
    const auto board=[&](Rect r,bool final){
        const float pos=final?54.f:42.f,time=final?132.f:116.f,name=r.w-pos-time-44;
        label({r.x+12,r.y,pos,28},"POS",14,0,false,false,Paint::muted);label({r.x+pos+18,r.y,name,28},"PLAYER",14,0,false,false,Paint::muted);label({r.x+r.w-time-12,r.y,time,28},"TIME",14,1,false,false,Paint::muted);
        for(unsigned i=0;i<std::min<std::size_t>(6,in.rows.size());++i){const auto& row=in.rows[i];const float yy=r.y+(final?36.f:30.f)+(final?58.f:48.f)*i;const float rh=final?52.f:44.f;s.box({r.x,yy,r.w,rh},row.self?Paint::selected:Paint::soft);
            if(!row.dnf)s.position({r.x+8,yy+3,pos+2,rh-6},row.place,final?30:26);
            else label({r.x+10,yy+4,pos-2,rh-8},final?"DNF":"-",final?18:16,2);
            label({r.x+pos+18,yy+4,name,rh-8},row.name,final?23:18,0,false,true);
            label({r.x+r.w-time-12,yy+4,time,rh-8},row.time+(final?"":"\n"+row.detail),final?21:15,1,false,!final);
        }
    };
    if(results){const unsigned n=static_cast<unsigned>(std::min<std::size_t>(6,in.rows.size()));const Rect r{(s.width-960)*.5f,174,960,260.f+58.f*static_cast<float>(n)};s.box(r,Paint::panel);header(r,"DEATHRACE RESULTS",in.route+" / "+in.map);
        s.box({r.x+24,r.y+134,r.w-48,66},Paint::brush,in.winner.empty()?"NO FINISHERS":in.winner+" WINS!",32,2,true);board({r.x+24,r.y+214,r.w-48,40},true);return s;}
    // Keep the entire top center clear for the game's compass, including its
    // location banner. The lower-left belongs to its D-pad/phone HUD.
    if(!in.options.classic){
        auto options=in.options;options.normalize();const float k=options.scale,pad=options.safe;const auto points=[&](float size){return static_cast<unsigned>(std::lround(size*k));};
        const float cw=358*k,ch=130*k,cx=(s.width-cw)*.5f,cy=184+pad;
        const Rect clock_box{cx,cy,cw,ch};s.box(clock_box,Paint::gate_frame);
        label({cx+18*k,cy+10*k,cw-36*k,22*k},done?(in.screen==Screen::dnf?"DID NOT FINISH":"FINAL TIME"):"TIME LEFT",18*k,0,false,false,Paint::muted);
        s.box({cx+18*k,cy+34*k,cw-36*k,52*k},Paint::race_clock,done?in.elapsed:in.remaining,points(48));
        const auto count=in.required?std::format("CLEARED {:02} / {:02}",in.cleared,in.required):std::string("FINISH ROUTE");
        label({cx+18*k,cy+92*k,cw-36*k,24*k},count,20*k);
        s.progress({cx+18*k,cy+122*k,cw-36*k,3*k},in.required?float(in.cleared)/in.required:0.f,Paint::red);
        if(in.have_gate && in.compass && options.guidance && !done){
            const auto& n=in.navigation;if(n.valid){
                float nx=n.x,ny=n.y;bool edge=n.edge;Rect label_box{nx-88*k,ny+18*k,176*k,25*k};
                if(overlaps(label_box,clock_box)){ny=cy+ch+46*k;edge=true;}
                if(nx>s.width-400*k && ny>640){ny=640;edge=true;}
                if(options.standings && nx<400 && ny<580){ny=580;edge=true;}
                if((!in.notification.empty() || in.recovering) && ny>730){ny=730;edge=true;}
                const auto sequence=in.next_finish?std::string("FINISH"):std::format("{:02}",in.next_gate);
                s.box({nx-31*k,ny-19*k,62*k,34*k},Paint::gate_target,sequence,points(in.next_finish?17.f:24.f),2);
                std::string caption=n.behind?"BEHIND":edge?(n.dx<-.2f?"LEFT":n.dx>.2f?"RIGHT":n.dy<0?"UP":"DOWN"):"NEXT";
                caption+=std::format(" / {:.0f} M",in.gate_distance);
                s.box({nx-88*k,ny+18*k,176*k,25*k},Paint::race_label,caption,points(18),2);
            }
        }
        const float rx=s.width-216*k-pad,ry=846-34*(k-1);
        if(in.position)s.position({rx,ry,174*k,120*k},in.position,points(116));else label({rx,ry,174*k,100*k},"DNF",54*k,0,true);
        s.box({rx,ry+120*k,174*k,24*k},Paint::race_label,std::format("OF {} RACERS",in.players),points(18),2);
        if(options.speed && in.have_speed && !done){const auto speed=speed_readout(in.speed_mps,options.mph);
            const float sx=s.width-216*k-pad,sy=716;s.box({sx,sy,174*k,76*k},Paint::gate_frame);
            label({sx+12*k,sy+9*k,94*k,38*k},std::format("{:.0f}",speed),32*k);label({sx+90*k,sy+15*k,72*k,24*k},options.mph?"MPH":"KM/H",18*k);
            s.progress({sx+12*k,sy+62*k,150*k,2*k},std::clamp(in.speed_mps/25.f,0.f,1.f),Paint::red);
        }
        if(options.standings){const float bx=40+pad,bw=312;s.box({bx,184+pad,bw,66+48.f*static_cast<float>(std::min<std::size_t>(6,in.rows.size()))},Paint::gate_frame);label({bx+12,194+pad,bw-24,28},"LIVE STANDINGS",20,0,true);board({bx+12,228+pad,bw-24,40},false);}
        // One event location: warnings take precedence over finish/recovery.
        std::string event=in.notification;
        if(event.empty() && in.recovering)event="RECOVER / BAIL RESET AFTER 1 SECOND";
        if(event.empty() && done)event=in.screen==Screen::dnf?"DID NOT FINISH":"FINISHED";
        if(!event.empty())s.box({(s.width-620)*.5f,866,620,62},Paint::race_notice,event,23,2,false,true);
        return s;
    }
    const float x=40,y=184,w=286;
    const float brand=std::min(w,(s.width-800)*.5f-58);s.box({x,28,brand,76},Paint::panel);s.box({x,28,brand,48},Paint::brush);s.image({x+10,32,38,38},logo);label({x+58,30,brand-72,36},"DEATHRACE",24,0,true);label({x+12,79,brand-24,20},in.route,15,0,false,false,Paint::muted);
    // Clock and checkpoint progress are compact edge telemetry. Placement is
    // a large free-standing ordinal at the lower right, clear of the D-pad.
    s.box({x,y,w,110},Paint::panel);s.box({x,y,w,24},Paint::brush,done?"FINISH TIME":"TIME LEFT",15,2,true);
    label({x+14,y+28,w-28,48},done?in.elapsed:in.remaining,42);
    label({x+14,y+82,90,18},"TIME LIMIT",12,0,false,false,Paint::muted);label({x+106,y+80,w-120,22},clock(std::uint64_t(in.deadline)*1000),15,1);
    s.box({x,y+122,w,76},Paint::panel);s.box({x,y+122,174,24},Paint::brush,"REQUIRED GATES",14,2,true);
    label({x+182,y+123,w-196,25},std::format("{} / {}",in.cleared,in.required),22,1);
    s.progress({x+14,y+158,w-28,5},in.required?float(in.cleared)/in.required:0.f,Paint::yellow);
    label({x+14,y+174,w-28,20},in.cleared>=in.required?"FINISH UNLOCKED":"FOLLOW THE YELLOW MARKERS",13,0,false,false,in.cleared>=in.required?Paint::success:Paint::muted);
    const float rank_x=s.width-236,rank_y=818;
    if(in.position)s.position({rank_x,rank_y,190,142},in.position,138);else label({rank_x,rank_y,190,142},"DNF",66,0,true);
    s.box({rank_x,rank_y+146,190,25},Paint::brush,done?(in.screen==Screen::dnf?"DID NOT FINISH":"FINISHED"):std::format("OF {} RACERS",in.players),14,2,true);
    const float bx=s.width-398,bw=358;s.box({bx,184,bw,72+48.f*static_cast<float>(std::min<std::size_t>(6,in.rows.size()))},Paint::panel);s.box({bx,184,bw,32},Paint::brush,done?"RACERS FINISHING":"LIVE STANDINGS",19,2,true);board({bx+12,220,bw-24,40},false);
    const std::string event=!in.notification.empty()?in.notification:done?(in.screen==Screen::dnf?"DID NOT FINISH":"FINISHED!"):"";
    if(!event.empty())s.box({(s.width-640)*.5f,866,640,68},Paint::brush,event,25,2,false,true);
    if(in.recovering)s.box({(s.width-640)*.5f,802,640,48},Paint::danger,"Recover now / bail reset after 1 second",20,2);
    return s;
}

#include "deathrace_skate2_layout.inl"

inline bool inside(const Rect& r,float width,float height) noexcept {
    return r.x>=0 && r.y>=0 && r.w>=0 && r.h>=0 && r.x+r.w<=width+.1f && r.y+r.h<=height+.1f;
}
} // namespace dingosdk::deathrace_ui
