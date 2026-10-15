#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <format>
#include <string>

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
    return std::clamp((advance+31u)/32u*32u,224u,1280u);
}
inline std::string native_hud_race_time(std::uint64_t milliseconds) {
    return std::format("{:02}:{:02}.{:03}",milliseconds/60000,(milliseconds/1000)%60,milliseconds%1000);
}
// A remote state may arrive less often than a rendered frame. Its clock keeps
// advancing from the receipt time; completed and DNF rows retain the exact
// referee time. Never extrapolate beyond the race's deadline.
inline std::uint64_t native_hud_race_elapsed(std::uint64_t observed,std::uint64_t sampled,
        std::uint64_t now,bool running,std::uint64_t deadline) {
    const auto extra=running && sampled && now>sampled?now-sampled:0;
    return observed>=deadline?deadline:observed+std::min(extra,deadline-observed);
}
inline std::uint64_t native_hud_race_remaining(std::uint64_t observed,std::uint64_t sampled,
        std::uint64_t now,bool running) noexcept {
    const auto extra=running && sampled && now>sampled?now-sampled:0;
    return extra>=observed?0:observed-extra;
}
struct RaceClockDisplay { std::uint64_t countdown,time_to_beat; };
inline RaceClockDisplay native_hud_race_clocks(std::uint64_t observed,std::uint64_t sampled,
        std::uint64_t now,bool running,std::uint64_t limit) noexcept {
    return {std::min(limit,native_hud_race_remaining(observed,sampled,now,running)),limit};
}
inline unsigned native_hud_value_width(std::string_view text) {
    unsigned advance=40;
    for(const unsigned char ch:text) {
        if((ch&0xc0)==0x80)continue;
        advance+=(ch==':' || ch=='.' || ch==' ')?12:(ch=='W' || ch=='M' || ch>=0x80)?36:26;
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
