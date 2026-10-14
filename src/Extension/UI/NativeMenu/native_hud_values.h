#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <string_view>

namespace dingosdk::multiplayer::menu_data {
// Approximate brush-font advance, including padding; round to an authored
// native frame. UTF-8 continuation bytes do not inflate a gamertag's width.
inline unsigned native_hud_name_width(std::string_view name) {
    unsigned advance=32;
    for(const unsigned char ch:name) {
        if((ch&0xc0)==0x80)continue;
        advance+=(ch=='I' || ch=='i' || ch=='l' || ch==' ' || ch=='.')?12:
            (ch=='W' || ch=='M' || ch=='w' || ch=='m' || ch>=0x80)?36:26;
    }
    return std::clamp((advance+31u)/32u*32u,224u,640u);
}
// CountdownViewModel stores whole seconds as Uint64. The match clock stores
// milliseconds. Round up so 2.9 seconds displays 3, and GO occurs only at zero.
constexpr std::uint64_t native_hud_seconds(std::uint64_t milliseconds) {
    return milliseconds / 1000 + (milliseconds % 1000 != 0);
}
constexpr bool earned_one_up_letter(unsigned penalties, unsigned letter) {
    return letter < 3 && letter < penalties;
}
inline unsigned native_slider_choice(float normalized,unsigned minimum,unsigned maximum,unsigned step) {
    if(!std::isfinite(normalized))return minimum;
    const auto steps=(maximum-minimum)/step;
    const auto index=static_cast<unsigned>(std::round(std::clamp(normalized,0.f,1.f)*steps));
    return minimum+std::min(index,steps)*step;
}
inline float native_slider_position(unsigned choice,unsigned minimum,unsigned maximum) {
    if(maximum<=minimum)return 0.f;
    return float(std::clamp(choice,minimum,maximum)-minimum)/float(maximum-minimum);
}
} // namespace dingosdk::multiplayer::menu_data
