#include "challenge_registry.h"
#include "Extension/Modes/mode_rules.h"
#include <algorithm>
#include <array>
#include <cctype>

namespace dingosdk::throwdowns {
Palette palette_for(std::string_view id) noexcept {
    std::uint32_t hash=2166136261U;
    for(const auto c:id) hash=(hash^static_cast<unsigned char>(c))*16777619U;
    // Authored Throwdown hues: fixed assignment across sessions and reloads.
    constexpr std::array<std::uint32_t,6> hues{0xffff8000U,0xffff5df2U,0xff00cfffU,0xff60d64bU,0xffed935bU,0xffdfae78U};
    return {hues[hash%hues.size()],0xfff3f0e5U,0xfff2cc52U,0xff171717U,0xffffffffU};
}
Registration Registry::add(Definition d) {
    const auto valid_id=[](std::string_view id) {
        return !id.empty() && id.size()<=64 && std::all_of(id.begin(),id.end(),[](unsigned char c) {
            return (c>='a'&&c<='z') || (c>='0'&&c<='9') || c=='.' || c=='-' || c=='_';
        });
    };
    if(!valid_id(d.id) || d.name.empty() || d.name.size()>64 || d.description.size()>1024 || d.donor>2)
        return {false,false,"Invalid challenge identity, description or artwork family."};
    if(d.adapter==Adapter::game_mode && (!d.mode || !modes::parse_mode(d.key) ||
        static_cast<unsigned>(*modes::parse_mode(d.key))!=d.mode))
        return {false,false,"Game-mode adapter does not resolve to an implemented mode."};
    if(d.palette.primary==0)d.palette=palette_for(d.id);
    if(const auto* existing=find(d.id)) {
        if(*existing!=d)
            return {false,false,"Challenge identifier already belongs to a different definition."};
        return {true,false,{}}; // Repeated initialization is idempotent.
    }
    if(d.adapter==Adapter::game_mode && mode(d.mode))return {false,false,"Game mode already registered."};
    definitions_.push_back(std::move(d));++revision_;
    return {true,true,{}};
}
const Definition* Registry::find(std::string_view id) const noexcept {
    const auto it=std::find_if(definitions_.begin(),definitions_.end(),[&](const auto& d){return d.id==id;});
    return it==definitions_.end()?nullptr:&*it;
}
const Definition* Registry::mode(unsigned value) const noexcept {
    const auto it=std::find_if(definitions_.begin(),definitions_.end(),[&](const auto& d){return d.adapter==Adapter::game_mode && d.mode==value;});
    return it==definitions_.end()?nullptr:&*it;
}
Registry& registry() {
    static Registry value=[] {
        Registry result;
        result.add({"native.skate","R.E.S.K.A.T.E.","Set a trick. Copy it or take a letter. Spell R.E.S.K.A.T.E. and you are out; last skater wins.",
            "skate","UI/ReSkate/Throwdowns/img_skate_Reskate_1024",Adapter::game_mode,static_cast<unsigned>(modes::Mode::skate),0,
            {0xffd9df00U,0xfff3f0e5U,0xfff2cc52U,0xff171717U,0xffffffffU},false,true});
        result.add({"native.spot","Spot Battle","Earn the highest score in a single round.","SpotBattle",{},Adapter::native_spot,0,1,{}});
        result.add({"native.jam","Skate Jam","Accumulate points across all tricks.","ThrowdownerActive",{},Adapter::native_jam,0,2,{}});
        result.add({"native.oneup","1-UP","Beat the previous turn total. Three penalties and you're out. Last skater wins!",
            "oneup","UI/ReSkate/OneUp/img_OneUp_Blue_1024",Adapter::native_one_up,0,1,
            {0xffef8b22U,0xfff3f0e5U,0xfff2cc52U,0xff171717U,0xffffffffU}});
        for(const auto mode:modes::all_modes) {
            if(mode==modes::Mode::one_up || mode==modes::Mode::skate)continue;
            const auto key=std::string(modes::mode_key(mode));
            const unsigned donor=modes::tag_like(mode)?0U:mode==modes::Mode::jam?2U:1U;
            // Matches the single generated 4x2 placeholder atlas, RGBA bytes.
            const unsigned color=mode==modes::Mode::jam?0xffab4fffU:mode==modes::Mode::meat?0xff218affU:
                mode==modes::Mode::race?0xff3434ffU:mode==modes::Mode::domination?0xffff558cU:
                mode==modes::Mode::graffiti?0xff9fd929U:mode==modes::Mode::tag?0xff31d3ffU:
                mode==modes::Mode::infection?0xff24d675U:0xffffbd39U;
            result.add({"mode."+key,std::string(modes::mode_name(mode)),std::string(modes::mode_summary(mode)),key,
                "UI/ReSkate/Throwdowns/img_"+key+"_Placeholder",
                Adapter::game_mode,static_cast<unsigned>(mode),donor,{color,0xfff3f0e5U,0xfff2cc52U,0xff171717U,0xffffffffU},
                mode==modes::Mode::race || mode==modes::Mode::domination,mode==modes::Mode::hide});
        }
        return result;
    }();
    return value;
}
}
