#pragma once
#include "mode_rules.h"
#include <algorithm>
#include <cmath>
#include <deque>
#include <optional>

namespace dingosdk::modes {
// Unknown samples during a fall must not rearm its edge. A real return to
// riding/walking rearms it so a second fast fall gets its own recovery.
struct RagdollBailEdge {
    bool falling{};
    bool observe(bool known,bool ragdoll) noexcept {
        if(!known)return false;
        const bool began=ragdoll && !falling;falling=ragdoll;return began;
    }
};
// The physics publisher counts native runout edges, even if their short pulse
// ends between mode ticks. Establish a baseline before consuming later events.
class RaceRunoutWatch {
    std::optional<std::uint64_t> seen;
public:
    bool observe(bool valid,std::uint64_t count) noexcept {
        if(!seen) {if(valid)seen=count;return false;}
        const bool changed=*seen!=count;seen=count;return changed;
    }
};
class RaceRecoveryGrace {
    std::uint64_t until{};bool full_bail{};
public:
    void reset() noexcept {until=0;full_bail=false;}
    bool pending() const noexcept {return until!=0;}
    bool waiting(std::uint64_t now) const noexcept {return until && now<=until;}
    bool ready(std::uint64_t now) const noexcept {return until && now>=until;}
    bool observe(bool bail,bool runout,std::uint64_t now) noexcept {
        if(!bail && !runout)return false;
        if(!until){full_bail=bail;until=now+(bail?2000u:1000u);return true;}
        // A runout can become a full ragdoll. Give that new full bail its two
        // seconds, but duplicate state samples must never extend the grace.
        if(bail && !full_bail){full_bail=true;until=std::max(until,now+2000u);}
        return false;
    }
};
// Only restore measured horizontal motion. This is a safety ceiling for bad
// telemetry, not a recovery boost or the speed assigned to a stopped rider.
inline constexpr float max_race_recovery_speed=120.f;
inline bool valid_race_recovery_velocity(const Vec3& velocity) noexcept {
    return std::all_of(velocity.begin(),velocity.end(),[](float n){return std::isfinite(n);}) &&
        velocity[1]==0.f && std::hypot(velocity[0],velocity[2])<=max_race_recovery_speed+.001f;
}
class RaceMomentum {
    std::deque<std::pair<std::uint64_t,float>> samples;
    std::uint64_t sample_after{};
public:
    void suspend_until(std::uint64_t until) noexcept {sample_after=until;}
    void observe(float speed,std::uint64_t now) {
        if(now<sample_after)return;
        if(!samples.empty() && now<samples.back().first)samples.clear();
        while(!samples.empty() && now-samples.front().first>750)samples.pop_front();
        if(std::isfinite(speed) && speed>=0 && speed<=max_race_recovery_speed)samples.emplace_back(now,speed);
    }
    float before_bail(std::uint64_t now) const noexcept {
        float speed{},recent{};
        // A collision can report zero before the bail state changes. Retain
        // the approach velocity for this short detection window only.
        for(const auto& [at,value]:samples)if(now>=at && now-at<=750) {
            if(value>0)speed=value;
            if(now-at<=250)recent=std::max(recent,value);
        }
        return recent>0?recent:speed;
    }
    float current(std::uint64_t now) const noexcept {
        return samples.empty() || now<samples.back().first || now-samples.back().first>250?0.f:samples.back().second;
    }
};
// Repeated returns must never raise the first trusted approach-speed budget.
// A clean riding interval or genuine new checkpoint starts another budget.
class RaceRecoverySpeedGuard {
    std::optional<float> ceiling;
    std::uint64_t until{};
public:
    float limit(float measured,std::uint64_t now) noexcept {
        if(!std::isfinite(measured) || measured<0)measured=0;
        measured=std::min(measured,max_race_recovery_speed);
        if(ceiling && now<until)measured=std::min(measured,*ceiling);
        ceiling=measured;until=now+3000;return measured;
    }
    void completed(std::uint64_t now) noexcept {if(ceiling)until=now+3000;}
    void reset() noexcept {ceiling.reset();until=0;}
};
inline Vec3 race_recovery_motion(float speed,float yaw) noexcept {
    if(!std::isfinite(speed) || speed<0 || speed>max_race_recovery_speed || !std::isfinite(yaw))return {};
    const auto angle=yaw*3.14159265f/180.f;
    return {std::sin(angle)*speed,0.f,std::cos(angle)*speed};
}
// A busy/off-board native queue must not consume the only copy of a restore.
class RaceRecoveryDelivery {
    std::optional<Vec3> pending;
    std::uint64_t until{};
public:
    void reset() noexcept {pending.reset();until=0;}
    void request(Vec3 value,std::uint64_t now) noexcept {
        reset();if(valid_race_recovery_velocity(value)){pending=value;until=now+2000;}
    }
    std::optional<Vec3> peek(std::uint64_t now) noexcept {
        if(now>=until)reset();return pending;
    }
    void accepted(const Vec3& value) noexcept {if(pending && *pending==value)reset();}
};
}
