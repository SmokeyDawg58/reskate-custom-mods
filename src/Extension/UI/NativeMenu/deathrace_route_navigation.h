#pragma once
#include <algorithm>
namespace dingosdk::deathrace_ui {
struct RouteNavigation {
    unsigned route{},action{};bool actions{};unsigned column{};
    void normalize(unsigned count) {if(count)route=std::min(route,count-1);else {route=0;actions=true;}action=std::clamp(action,count?0u:4u,5u);column=std::min(column,2u);}
    // A vertical sweep can still reach every button. Horizontal input chooses
    // a column; Up/Down from it follows the two visible rows independently.
    void step(int direction,unsigned count) {
        normalize(count);const unsigned first=count?0u:4u;
        if(!actions){if(direction>0){if(route+1<count)++route;else {actions=true;action=first;}}else if(route)--route;else {actions=true;action=5;}}
        else if(direction>0){if(action<5)++action;else if(count){actions=false;route=0;}else action=first;}
        else if(action>first)--action;else if(count){actions=false;route=count-1;}else action=5;
    }
    void horizontal(int direction,unsigned count) {
        normalize(count);actions=true;
        if(!count){action=action==4?5:4;column=action%3;return;}
        action=action/3*3+(action%3+(direction>0?1:2))%3;column=action%3;
    }
    void vertical(int direction,unsigned count) {
        normalize(count);
        if(!count){action=action==4?5:4;column=action%3;return;}
        if(!actions){if(direction>0 && route+1==count){actions=true;action=column;}else if(direction<0 && route==0){actions=true;action=3+column;}else route=direction>0?route+1:route-1;}
        else if(direction>0 && action<3){column=action%3;action+=3;}
        else if(direction<0 && action>=3){column=action%3;action-=3;}
        else {column=action%3;actions=false;route=direction>0?0:count-1;}
    }
    unsigned first_route() const {return route/4*4;}
};
}
