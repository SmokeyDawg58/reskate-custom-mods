#include "../../Extension/UI/NativeMenu/deathrace_native_presentation.h"
#include <iostream>
#include <iomanip>
namespace dr=dingosdk::deathrace_ui;
void quoted(const std::string& s) {
    std::cout<<'"';for(const unsigned char c:s){
        if(c=='"' || c=='\\')std::cout<<'\\'<<c;
        else if(c=='\n')std::cout<<"\\n";
        else if(c<32)std::cout<<' ';
        else std::cout<<c;
    }std::cout<<'"';
}
int main(){
    std::cout<<"[";bool first=true;
    for(const float aspect:{9.f/16.f,4.f/3.f,16.f/10.f,16.f/9.f,21.f/9.f,32.f/9.f,48.f/9.f})
    for(unsigned screen=0;screen<=static_cast<unsigned>(dr::Screen::results);++screen)
    for(bool long_names:{false,true}){
        dr::Input in;in.aspect=aspect;in.screen=static_cast<dr::Screen>(screen);
        in.player=long_names?std::string(64,'W'):"Smokey";in.host="Syphrus";in.winner=in.player;in.host_local=true;
        in.route=long_names?std::string(64,'W'):"Blue Light Run";in.map="Skate3Improved";in.name_entry=in.route;
        in.required=8;in.cleared=5;in.guides=3;in.arrows=6;in.finish=true;in.players=6;in.position=2;
        in.elapsed="01:23.906";in.remaining="01:36.094";in.deadline=180;in.yaw=95;in.pitch=-20;in.roll=10;
        in.arrow=true;in.object="Arrow";in.roll_axis=true;in.error="Finish gate needed";in.notification="Required gate 5 passed";
        in.recovering=true;in.height=198.7f;in.distance=1540;in.clear_hold=.6f;in.save_hold=.4f;
        for(unsigned i=0;i<6;++i){in.rows.push_back({i==1?in.player:i==0?"Syphrus":long_names?std::string(64,'W'):"Skater "+std::to_string(i),i==5?"DNF\n00:58.930":"01:23.906",i==5?"":"5 / 8 gates",i==1,i==0,i==5,i+1});
            in.routes.push_back({i+1,in.route+" "+std::to_string(i+1),in.map,8,19});}
        const auto scene=dr::compose(in);if(!first)std::cout<<',';first=false;
        std::cout<<"{\"screen\":"<<screen<<",\"long_names\":"<<(long_names?"true":"false")<<",\"aspect\":"<<aspect<<",\"width\":"<<scene.width<<",\"height\":"<<scene.height<<",\"cells\":[";
        bool first_cell=true;for(const auto& c:scene.cells){if(!first_cell)std::cout<<',';first_cell=false;
            std::cout<<"{\"box\":["<<c.box.x<<','<<c.box.y<<','<<c.box.w<<','<<c.box.h<<"],\"paint\":"<<static_cast<unsigned>(c.paint)<<",\"points\":"<<c.points<<",\"align\":"<<c.align<<",\"marker\":"<<(c.marker?"true":"false")<<",\"alpha\":"<<c.alpha<<",\"fraction\":"<<c.fraction<<",\"text\":";quoted(c.text);
            std::cout<<",\"image\":";quoted(c.image);std::cout<<",\"input\":";quoted(c.input);std::cout<<",\"skin\":";quoted(dr::asset_name(c));std::cout<<'}';
        }std::cout<<"]}";
    }std::cout<<"]\n";
}
