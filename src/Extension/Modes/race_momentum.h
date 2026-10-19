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
// Hippy jumps and walking share offboard states in the 500 range. Riding/air -> offboard
// transition followed by upward motion identifies the jump; state alone
// cannot distinguish it from an accidental loss of the board.
class RaceHippyJump {
    std::uint64_t candidate_until{},jump_until{},landed_until{},board_until{},intent_until{};
    bool input_down{};
    std::uint32_t last{};
public:
    void input(bool down,bool board,std::uint64_t now) noexcept {
        if(down && !input_down && board)intent_until=now+600;
        input_down=down;
    }
    bool taking_off(std::uint64_t now) const noexcept {return now<intent_until || active(now);}
    bool observe(bool valid,std::uint32_t state,std::uint32_t previous,float vertical,
                 bool bail,std::uint64_t now,bool airborne=false) noexcept {
        if(bail) {
            candidate_until=jump_until=landed_until=board_until=intent_until=0;last=state;return false;
        }
        // A missing sample must not turn an intentional jump into board loss.
        // Keep its existing deadline; do not grant or extend immunity here.
        if(!valid || !std::isfinite(vertical))return active(now);
        const bool board=state>=100 && state<300,offboard=state>=500 && state<700;
        if(board)board_until=now+350;
        if(state!=last) {
            candidate_until=offboard && ((previous>=100 && previous<300) || now<board_until)?now+350:0;
            if(!offboard) {
                if(jump_until && now<jump_until && state<300)landed_until=now+750;
                jump_until=0;
            }
            last=state;
        }
        if(offboard && ((now<intent_until) || (candidate_until && now<=candidate_until && (vertical>.2f || (airborne && vertical>=-1.5f))))) {
            jump_until=now+4000;candidate_until=intent_until=0;
        }
        if(!offboard || now>=jump_until)jump_until=0;
        return jump_until!=0 || (state<300 && now<landed_until);
    }
    bool active(std::uint64_t now) const noexcept {return now<jump_until || now<landed_until;}
};
// The offboard animation flag can pulse at a hippy takeoff. A native 300
// state/counter is immediate; this secondary flag must persist to prove a bail.
class RaceOffboardRagdoll {
    std::optional<std::uint64_t> since;bool reported{};
public:
    void reset() noexcept {since.reset();reported=false;}
    bool active() const noexcept {return reported;}
    bool observe(bool known,bool ragdoll,std::uint64_t now) noexcept {
        if(!known)return false;
        if(!ragdoll){since.reset();reported=false;return false;}
        if(!since)since=now;
        if(reported || now<*since || now-*since<250)return false;
        reported=true;return true;
    }
};
class RaceRecoveryGrace {
    std::uint64_t until{};bool full_bail{};
public:
    void reset() noexcept {until=0;full_bail=false;}
    void cancel_runout() noexcept {if(!full_bail)reset();}
    bool pending() const noexcept {return until!=0;}
    bool waiting(std::uint64_t now) const noexcept {return until && now<=until;}
    bool ready(std::uint64_t now) const noexcept {return until && now>=until;}
    bool observe(bool bail,bool runout,std::uint64_t now) noexcept {
        if(!bail && !runout)return false;
        if(!until){full_bail=bail;until=now+1000u;return true;}
        // The same fall may acquire a stronger native signal. Preserve its
        // original one-second deadline instead of restarting the wait.
        if(bail)full_bail=true;
        return false;
    }
};
// A native return to controlled board/on-foot physics ends an ordinary bail.
// Offboard alone is never an automatic-recovery request.
inline bool race_recovered_control(bool valid,std::uint32_t physics,bool ragdoll_known,bool ragdoll) noexcept {
    if(!valid || (ragdoll_known && ragdoll))return false;
    return (physics>=100 && physics<300) || (physics>=500 && physics<700 && ragdoll_known);
}
// Walking, jumping and getting back on the board can clear a gate. A
// confirmed ragdoll retains the finish-only one-second bail grace.
inline bool race_offboard_progress(bool skater,std::uint32_t state,bool hippy=false) noexcept {
    return hippy || (skater && state>=500 && state<700);
}
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
    std::optional<float> offboard_speed;
public:
    void suspend_until(std::uint64_t until) noexcept {sample_after=until;}
    void observe_onboard(bool onboard,float speed,std::uint64_t now) {
        if(now<sample_after)return;
        if(!onboard) {
            if(!offboard_speed)offboard_speed=before_bail(now);
            return;
        }
        if(!std::isfinite(speed) || speed<0 || speed>max_race_recovery_speed)return;
        if(offboard_speed){offboard_speed.reset();samples.clear();}
        observe(speed,now);
    }
    void observe(float speed,std::uint64_t now) {
        if(now<sample_after)return;
        if(!samples.empty() && now<samples.back().first)samples.clear();
        while(!samples.empty() && now-samples.front().first>750)samples.pop_front();
        if(std::isfinite(speed) && speed>=0 && speed<=max_race_recovery_speed)samples.emplace_back(now,speed);
    }
    float before_bail(std::uint64_t now) const noexcept {
        if(offboard_speed)return *offboard_speed;
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
        if(offboard_speed)return *offboard_speed;
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
    std::optional<Vec3> peek(std::uint64_t now,bool returning=false) noexcept {
        if(now>=until)reset();
        if(pending)return pending;
        return returning?std::optional(Vec3{}):std::nullopt;
    }
    void wait_for_board(std::uint64_t now) noexcept {
        if(pending && now<until)until=now+2000;
    }
    void accepted(const Vec3& value) noexcept {if(pending && *pending==value)reset();}
};
}
