#pragma once
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <string_view>
#include <format>
#include <string>
#include <array>

namespace dingosdk::multiplayer::menu_data {
// The supported desktop gameplay screen lays out inside a 90% safe frame.
// Our anchors belong to that frame, so a small positive anchor alone still
// leaves the outer 5% margin visible. Convert only our screen-edge anchors;
// stock screens, centered notices and the stable HUD presenters stay intact.
inline constexpr float native_hud_screen_anchor(float screen) noexcept {
    return (screen-.05f)/.90f;
}
// The native 1-UP artwork predates the custom mode's shorter wire key.
inline constexpr std::string_view native_hud_theme_key(std::string_view key) noexcept {
    return key=="1up"?"oneup":key;
}
inline constexpr float native_hud_side_center=.60f;
// Rime's start/end offsets are opposing insets. Equal values resize a
// rectangle; opposite values translate it without changing its bounds.
inline constexpr std::array<float,2> native_hud_translation(float offset) noexcept {
    return {offset,-offset};
}
inline constexpr float native_hud_shortcut_width=672.f;
inline constexpr float native_hud_shortcut_edge=.998f;
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
struct HudHeaderGeometry {
    unsigned frame_width,content_width,title_width,icon=64,content_height=80,padding=48;
    float badge_y_shift{},title_y_shift{};
    static constexpr unsigned height=112,text_height=80,gap=24;
};
inline std::string native_hud_title_text(std::string_view title) {
    std::string result(title);for(auto& character:result)if(character>='a' && character<='z')character=char(character-'a'+'A');return result;
}
inline HudHeaderGeometry native_hud_header_geometry(std::string_view title) {
    // The captured D78 brush lettering spans 333 units, not the generic
    // 488-unit reserve. Center its visible icon/title group inside the existing
    // 704-unit frame. Separate paint-baseline corrections align their centers.
    if(native_hud_title_text(title)=="DEATHRACE")return {704,464,344,96,96,120,-6.f,-2.f};
    // Brush title advances are distinct from H52 player-name cells. Reserve a
    // small font allowance, then fit the authored frame to this title's row.
    // Short names must not inherit a scoreboard-wide minimum background.
    unsigned advance=12;
    for(const unsigned char ch:native_hud_title_text(title)) {
        if((ch&0xc0)==0x80)continue;
        if(ch==' ' || ch=='.')advance+=16;
        else if(ch=='-' || ch=='t' || ch=='f' || ch=='r')advance+=24;
        else if(ch=='I' || ch=='i' || ch=='l' || ch=='j')advance+=18;
        else if(ch=='W' || ch=='M')advance+=58;
        else if(ch=='w' || ch=='m' || ch>=0x80)advance+=48;
        else if(ch>='a' && ch<='z')advance+=34;
        else if(ch=='E' || ch=='F' || ch=='T' || ch=='P' || ch=='S')advance+=40;
        else advance+=44;
    }
    const auto scaled=(advance*39u+31u)/32u; // Native D78 brush title, with a conservative width allowance.
    const auto text=std::clamp((scaled+7u)/8u*8u,144u,1600u);
    const unsigned icon=64u;
    const auto content=text+icon+HudHeaderGeometry::gap;
    const auto frame=(std::max(320u,content+96u)+31u)/32u*32u;
    return {frame,content,text,icon,80u};
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
