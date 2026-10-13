// The game modes' rules and wire format without the game: referees are driven by hand with
// the events players would send.
#include "Extension/Modes/mode_rules.h"
#include "Extension/Modes/race_route.h"
#include "Extension/Modes/mode_transport.h"
#include "Engine/Game/Multiplayer/session_tools.h"
#include "Engine/Game/Multiplayer/session_physics.h"
#include "Extension/Modes/placement_camera.h"
#include <algorithm>
#include <iostream>
#include <limits>
#include <stdexcept>
#include <string>

using namespace dingosdk::modes;

namespace {
void check(bool condition, const std::string &what) {
    if (!condition) throw std::runtime_error("FAILED: " + what);
}
constexpr std::uint64_t a = 76561198000000001ULL, b = 76561198000000002ULL, c = 76561198000000003ULL;
const std::vector<Vec3> square{{0, 0, 0}, {40, 0, 0}, {40, 0, 40}, {0, 0, 40}};

void placement_camera_flow() {
    PlacementCamera camera;
    check(camera.update(true,false,1000)==true,"placement requests a native camera");
    check(!camera.update(true,false,1200),"waiting for acknowledgement does not flood requests");
    check(camera.update(true,false,1400)==true,"a rejected flag-transition request is retried");
    check(!camera.update(true,true,1500),"acknowledged camera does not enqueue another enable");
    check(camera.update(true,false,1800)==true,"lost placement camera is reacquired");
    check(camera.update(false,false,1801)==false,"cancel drains an enable still in the queue");
    check(camera.update(false,true,2201)==false,"a late enable is restored after cancel");
    check(!camera.update(false,false,3401),"restoration stops after the queue drains");
    check(camera.update(true,false,3402)==true,"reopening acquires a fresh camera");
    check(!camera.update(true,true,3403),"reopened editor accepts the live camera");
    check(camera.update(false,true,3404)==false,"second exit restores the camera too");
    check(camera.update(false,true,6000)==false,"a busy restore is retried until actually released");
    check(!camera.update(false,false,6001),"released camera clears ownership");
    PlacementCamera borrowed;
    check(!borrowed.update(true,true,1000),"preexisting free camera is borrowed");
    check(!borrowed.update(false,true,1100),"closing preserves a preexisting camera");
    check(borrowed.update(true,false,1200)==true,"borrowed camera loss is reacquired when needed");
    check(borrowed.update(false,true,1201)==false,"only the reacquired camera is restored");
}

void area_flow() {
    check(inside(square, {20, 0, 20}), "the middle of a square is inside it");
    check(!inside(square, {50, 0, 20}), "a point past its edge is outside");
    check(inside(std::vector<Vec3>{}, {1e6f, 0, 1e6f}), "without an area everywhere counts");
    const auto centre = area_centre(square);
    check(centre[0] == 20 && centre[2] == 20, "the area's centre");
    Settings graffiti;
    graffiti.mode = Mode::graffiti;
    check(missing(graffiti).empty(), "Graffiti needs nothing set up: what gets skated is tagged");
    check(missing(Settings{Mode::race}) != "", "a race without checkpoints is not ready");
    Settings two_corners;
    two_corners.corners = {{0, 0, 0}, {1, 0, 1}};
    check(!missing(two_corners).empty(), "two corners are not an area");
    // A circle, like skate.'s own jam drops: a centre and how far it reaches.
    Settings circle;
    circle.mode = Mode::graffiti;
    circle.corners = {{100, 0, 100}};
    circle.area_radius = 20;
    check(has_area(circle) && missing(circle).empty(), "a centre and a radius are an area");
    check(inside(circle, {115, 0, 100}) && !inside(circle, {121, 0, 100}), "inside the circle and past it");
    Message setup;
    setup.kind = Message::Kind::setup;
    setup.leader = a;
    setup.game = 3;
    setup.settings = circle;
    check(decode(encode(setup)) == setup, "a circle round trips");
    circle.corners.push_back({0, 0, 0});
    setup.settings = circle;
    bool refused = false;
    try { (void)encode(setup); } catch (const std::invalid_argument &) { refused = true; }
    check(refused, "a circle has exactly one centre");
}

void scoring_flow() {
    check(trick_score({0.1f}) == 0, "a hop scores nothing");
    const auto ollie = trick_score({0.5f, 0.5f, 3});
    const auto kickflip = trick_score({0.5f, 0.5f, 3, 0, 0, 1000});
    const auto three_sixty = trick_score({0.8f, 1.0f, 4, 360});
    check(ollie > 0 && kickflip > ollie && three_sixty > kickflip, "flips and spins beat an ollie");
    check(line_score(1000, 1) == 1000 && line_score(1000, 3) == 2000, "a line multiplies its tricks");
    check(line_score(1000, 50) == 6000, "the multiplier is capped");
    check(bail_score({15, 4, 2}) > bail_score({5, 0, 1}), "a faster, higher bail is more meat");
}

void wire_flow() {
    Message setup;
    setup.kind = Message::Kind::setup;
    setup.leader = a;
    setup.game = 7;
    setup.settings.mode = Mode::domination;
    setup.settings.corners = square;
    setup.settings.points = {{10, 1, 10}, {30, 1, 30}};
    const auto bytes = encode(setup);
    check(is_mode_message(bytes) && bytes[0] != 25, "game mode messages are told apart by their first byte");
    check(decode(bytes) == setup, "setup round trip");
    Message state;
    state.kind = Message::Kind::state;
    state.leader = a;
    state.game = 7;
    state.phase = Phase::playing;
    state.remaining_ms = 1234;
    state.standings = {{a, 10, 2, false, true}, {b, 5, 0, true, false}};
    state.zones = {{1, b, 900}};
    state.calls = {"GO!", "Sam stole a tag from Alex"};
    state.call_serial = 12;
    check(decode(encode(state)) == state, "state round trip");
    Message join;
    join.kind = Message::Kind::join;
    join.leader = a;
    join.game = 7;
    check(decode(encode(join)) == join, "join round trip");
    Message event;
    event.kind = Message::Kind::event;
    event.leader = a;
    event.game = 7;
    event.value = 4000;
    event.extra = 3;
    event.at = {1, 2, 3};
    event.sequence = 9;
    check(decode(encode(event)) == event, "event round trip");
    auto future = encode(event);
    future[1] = wire_version + 1;
    check(!decode(future) && message_version(future) == wire_version + 1, "another version is recognised, not read");
    auto cut = encode(state);
    cut.pop_back();
    check(!decode(cut), "a cut message is rejected");
    check(!decode(std::vector<std::uint8_t>{1, 2, 3}), "a throwdown message is not a game mode message");
}

void jam_flow() {
    Settings s;
    s.mode = Mode::jam;
    s.duration_s = 60;
    s.corners = square;
    Referee r(s, a, 1);
    r.add_player(a);
    r.add_player(b);
    r.start(0);
    r.event(a, Event::line, 500, {10, 0, 10}, 1, 1000);
    check(r.state(1000).standings[0].score == 0, "nothing counts during the countdown");
    check(r.tick(countdown_ms), "the countdown ends");
    r.event(a, Event::line, 500, {10, 0, 10}, 2, 6000);
    r.event(a, Event::line, 500, {10, 0, 10}, 2, 6100); // a repeat
    r.event(b, Event::line, 900, {10, 0, 10}, 1, 7000);
    r.event(b, Event::line, 5000, {99, 0, 99}, 2, 7500); // outside the area
    auto st = r.state(8000);
    check(st.standings[0].player == b && st.standings[0].score == 900, "b leads with one line");
    check(st.standings[1].score == 500, "a's repeated event counted once");
    r.tick(countdown_ms + 60000);
    check(r.phase() == Phase::results, "the clock runs out");
    check(r.finished(countdown_ms + 60000 + results_ms), "results show, then the game is over");
}

void one_up_flow() {
    Settings s;
    s.mode = Mode::one_up;
    s.strikes = 2;
    Referee r(s, a, 2);
    r.add_player(a);
    r.add_player(b);
    r.start(0);
    r.tick(countdown_ms);
    auto st = r.state(countdown_ms);
    check(st.turn == a, "the leader goes first");
    r.event(b, Event::line, 9999, {}, 1, countdown_ms + 10); // not b's turn
    check(r.state(countdown_ms + 10).turn == a, "only the player up counts");
    r.event(a, Event::line, 1000, {}, 1, countdown_ms + 100);
    st = r.state(countdown_ms + 100);
    check(st.turn == b && st.target == 1000, "a set 1000, b is up");
    r.event(b, Event::line, 800, {}, 2, countdown_ms + 200);
    st = r.state(countdown_ms + 200);
    check(st.turn == a, "b fell short: a strike, a is up");
    r.event(a, Event::bail, 100, {}, 2, countdown_ms + 300);
    st = r.state(countdown_ms + 300);
    check(r.phase() == Phase::playing && st.turn == b, "a bailed: one strike each, b is up");
    r.tick(countdown_ms + 300 + s.turn_s * 1000ull);
    check(r.phase() == Phase::results, "b ran out of time: two strikes and out, the last one standing wins");
    check(r.state(countdown_ms + 300 + s.turn_s * 1000ull).standings[0].player == a, "a wins");
}

void race_flow() {
    Settings s;
    s.mode = Mode::race;
    s.points = {{0, 0, 0}, {50, 0, 0}, {100, 0, 0}};
    Referee r(s, a, 3);
    r.add_player(a);
    r.add_player(b);
    r.start(0);
    r.tick(countdown_ms);
    r.event(a, Event::checkpoint, 2, {100, 0, 0}, 1, countdown_ms + 100);
    check(r.state(countdown_ms + 100).standings[0].score == 1, "race starts at first visible gate and refuses skipped gates");
    r.event(a, Event::checkpoint, 0, {0, 0, 0}, 2, countdown_ms + 200);
    r.event(a, Event::checkpoint, 1, {50, 0, 0}, 3, countdown_ms + 300);
    r.event(a, Event::checkpoint, 2, {100, 0, 0}, 4, countdown_ms + 400);
    r.event(b, Event::checkpoint, 0, {0, 0, 0}, 1, countdown_ms + 500);
    r.event(b, Event::checkpoint, 1, {500, 0, 0}, 2, countdown_ms + 600); // nowhere near it
    auto st = r.state(countdown_ms + 600);
    check(st.standings[0].player == a && st.standings[0].aux == 400, "a finished in 0.4 s");
    check(st.standings[1].score == 1, "b's far-off checkpoint was refused");
    r.remove_player(b, countdown_ms + 700);
    r.tick(countdown_ms + 700);
    check(r.phase() == Phase::results, "everyone still racing finished");
}

void race_builder_flow() {
    const auto pose=race_spawn_transform({10,2,30},90);
    check(std::abs(pose[8]-1)<.0001f && std::abs(pose[10])<.0001f && std::abs(pose[2]+1)<.0001f,
        "east-facing flag produces an east-facing native spawn, not identity rotation");
    check(pose[12]==10 && std::abs(pose[13]-2.3f)<.0001f && pose[14]==30,"lineup retains native placement position");
    Settings s;s.mode=Mode::race;bool finish{};
    const Vec3 start{0,0,0},gate{20,0,0},end{60,0,0},replacement{80,0,0};
    check(!edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"cannot place a checkpoint before the flag");
    check(accept_race_flag(s,finish,start,90,6)==RaceFlagTransition::edit_route,"first native flag opens the route editor");
    check(!edit_race_route(s,finish,RaceObject::finish,end,90,6),"finish needs at least one normal gate");
    check(edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"normal checkpoint placed");
    check(!race_route_ready(s,finish),"unfinished route cannot start");
    check(edit_race_route(s,finish,RaceObject::finish,end,90,6) && race_route_ready(s,finish),"one finish makes a valid route");
    check(edit_race_route(s,finish,RaceObject::finish,replacement,45,4) && s.points.size()==3 && s.points.back()==replacement,"placing another finish moves the sole finish");
    check(edit_race_route(s,finish,RaceObject::checkpoint,{40,0,0},90,6) && s.points[2]==Vec3{40,0,0} && s.points.back()==replacement,"later normal gate inserts before the finish");
    check(edit_race_route(s,finish,RaceObject::start,{1,0,0},0,6) && s.points.size()==4 && s.spawn==Vec3{1,0,0},"moving start preserves the gates and finish");
    const auto saved=s;
    check(accept_race_flag(s,finish,{2,1,3},45,4)==RaceFlagTransition::edit_route,
        "saved-route native Confirm reopens the builder before host Start");
    check(s.points.size()==saved.points.size() && s.points[0]==Vec3{2,1,3} && s.spawn==Vec3{2,1,3} &&
        s.yaws[0]==45 && s.widths[0]==4 && s.race_finish && finish &&
        std::equal(s.points.begin()+1,s.points.end(),saved.points.begin()+1) &&
        std::equal(s.yaws.begin()+1,s.yaws.end(),saved.yaws.begin()+1) &&
        std::equal(s.widths.begin()+1,s.widths.end(),saved.widths.begin()+1),
        "saved native flag updates only start pose, retaining every checkpoint and finish");
    const auto before_invalid=s;
    check(accept_race_flag(s,finish,{0,0,0},std::numeric_limits<float>::infinity(),6)==RaceFlagTransition::rejected && s==before_invalid,
        "invalid native flag cannot corrupt the saved course");
    Message msg;msg.kind=Message::Kind::setup;msg.leader=a;msg.game=12;msg.settings=s;
    check(decode(encode(msg))==msg,"finish identity and gate poses reach other racers");
    check(undo_race_gate(s,finish) && !race_route_ready(s,finish) && !s.race_finish,"undo removes finish identity");
    while(s.points.size()<1025)check(edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"more than 1024 required gates accepted");
    check(edit_race_route(s,finish,RaceObject::finish,end,90,6),"finish fits an uncapped route");
    check(edit_race_route(s,finish,RaceObject::guiding,{100,0,0},90,6) && s.points.size()==1027 && guiding_gate(s,1025),"guiding gate inserts before the sole finish");
    check(edit_race_route(s,finish,RaceObject::finish,replacement,90,6) && s.points.size()==1027 && s.points.back()==replacement,"moving finish never adds another finish on a long route");
    check(mandatory_gates(s,s.points.size())==1024,"guide and finish excluded from mandatory count");
    check(cycle_race_object(RaceObject::start,false)==RaceObject::checkpoint && cycle_race_object(RaceObject::checkpoint,false)==RaceObject::finish &&
        cycle_race_object(RaceObject::finish,false)==RaceObject::guiding && cycle_race_object(RaceObject::guiding,false)==RaceObject::checkpoint &&
        cycle_race_object(RaceObject::checkpoint,true)==RaceObject::guiding && cycle_race_object(RaceObject::guiding,true)==RaceObject::finish,
        "editor cycles default mandatory, finish, white guide in both directions");
    Message long_route;long_route.kind=Message::Kind::setup;long_route.leader=a;long_route.game=99;long_route.settings=s;
    auto packets=transport_messages(long_route);check(packets.size()>1,"long route is fragmented");
    for(const auto& packet:packets)check(packet.size()<=4096,"each route fragment fits multiplayer payload");
    ModeTransfers incoming;std::optional<Message> decoded;
    for(auto i=packets.rbegin();i!=packets.rend();++i) {
        check(!incoming.receive(b,*i,1),"foreign sender cannot supply another host's route");
        auto result=incoming.receive(a,*i,1);if(result)decoded=result;
    }
    check(decoded==long_route,"1024 mandatory gates, guide kind, poses and finish reconstruct out of order");
    incoming.clear();check(!incoming.receive(a,packets.front(),1) && !incoming.receive(a,packets.front(),2),"duplicate fragment cannot complete a partial route");
    auto corrupt=packets;corrupt.back().back()^=1;decoded.reset();
    for(const auto& packet:corrupt)if(auto result=incoming.receive(a,packet,3))decoded=result;
    check(!decoded,"corrupt assembled route rejected");
    auto invalid=packets.front();for(unsigned i=19;i<23;++i)invalid[i]=255;
    check(!incoming.receive(a,invalid,4),"advertised huge allocation rejected before buffering");
    incoming.clear();incoming.receive(a,packets.front(),1);decoded.reset();
    for(std::size_t i=1;i<packets.size();++i)if(auto result=incoming.receive(a,packets[i],10002))decoded=result;
    check(!decoded,"expired incomplete route discarded");
    incoming.clear();for(const auto& packet:packets)if(auto result=incoming.receive(a,packet,10003))decoded=result;
    check(decoded==long_route,"next periodic route publication recovers from missing fragments");
    auto bad=long_route;bad.settings.gate_kinds.back()=RaceGateKind::guiding;
    bool refused{};try{encode(bad);}catch(const std::invalid_argument&){refused=true;}
    check(refused,"finish cannot be optional");

}
void race_clock_flow() {
    Settings s;s.mode=Mode::race;s.duration_s=120;s.race_finish=true;
    s.points={{0,0,0},{20,0,0},{40,0,0},{60,0,0}};
    Referee r(s,a,3);r.add_player(a);r.add_player(b);r.start(0);r.tick(countdown_ms);
    const auto of=[&](std::uint64_t id,std::uint64_t t) {const auto m=r.state(t);return *std::find_if(m.standings.begin(),m.standings.end(),[&](const auto& p){return p.player==id;});};
    check(of(a,countdown_ms).remaining_ms==120000,"race uses its selected shared deadline");
    r.event(a,Event::checkpoint,1,{20,0,0},1,countdown_ms+1000);
    check(of(a,countdown_ms+1000).remaining_ms==119000 && of(b,countdown_ms+1000).remaining_ms==119000,"gate awards no time and racers share deadline");
    r.tick(countdown_ms+89999);check(!race_clock_visible(of(a,countdown_ms+89999).remaining_ms),"timer hidden until ninety seconds");
    r.tick(countdown_ms+90000);check(race_clock_visible(of(a,countdown_ms+90000).remaining_ms),"timer appears with thirty seconds remaining");
    r.event(a,Event::checkpoint,2,{40,0,0},2,countdown_ms+119998);
    r.event(a,Event::checkpoint,3,{60,0,0},3,countdown_ms+119999);
    r.tick(countdown_ms+120000);
    check(r.phase()==Phase::results && of(b,countdown_ms+120000).out && !of(a,countdown_ms+120000).out &&
        r.state(countdown_ms+120000).standings.front().player==a,"unfinished racer DNF; finish just before deadline wins");
    check(!race_clock_visible(0),"no clock left after deadline");
    Referee late(s,a,4);late.add_player(a);late.start(0);late.tick(countdown_ms);
    late.event(a,Event::checkpoint,1,{20,0,0},1,countdown_ms+1);late.event(a,Event::checkpoint,2,{40,0,0},2,countdown_ms+2);
    late.event(a,Event::checkpoint,3,{60,0,0},3,countdown_ms+120000);late.tick(countdown_ms+120000);
    check(late.phase()==Phase::results && late.state(countdown_ms+120000).standings.front().out,"finish at exact deadline is DNF");
    Referee early(s,a,5);early.add_player(a);early.start(0);early.tick(countdown_ms);
    for(int i=1;i<4;++i)early.event(a,Event::checkpoint,i,s.points[i],i,countdown_ms+i);
    early.tick(countdown_ms+4);check(early.phase()==Phase::results,"all finished ends race without waiting two minutes");
}
void race_custom_deadlines_and_snap() {
    Settings route;route.mode=Mode::race;route.race_finish=true;
    route.points={{0,0,0},{0,0,20},{0,0,40}};route.yaws={0,0,0};route.widths={6,6,6};
    for(const auto duration:{30u,90u,180u,300u,3600u}) {
        route.duration_s=duration;
        Referee race(route,a,duration);race.add_player(a);race.start(0);race.tick(countdown_ms);
        const auto deadline=duration*1000ull;
        check(race.state(countdown_ms).remaining_ms==deadline,"selected deadline applies from GO");
        if(duration>30)race.tick(countdown_ms+deadline-30001);
        if(duration>30)check(!race_clock_visible(race.state(countdown_ms+deadline-30001).remaining_ms),"custom deadline stays hidden before final thirty seconds");
        race.tick(countdown_ms+deadline-30000);
        check(race_clock_visible(race.state(countdown_ms+deadline-30000).remaining_ms),"custom deadline exposes final thirty seconds");
        race.tick(countdown_ms+deadline-1);check(race.phase()==Phase::playing,"not DNF before selected deadline");
        race.tick(countdown_ms+deadline);check(race.phase()==Phase::results && race.state(countdown_ms+deadline).standings.front().out,"DNF exactly at selected deadline");
        Message setup;setup.kind=Message::Kind::setup;setup.game=12;setup.leader=a;setup.settings=route;
        const auto restored=decode(encode(setup));check(restored && restored->settings.duration_s==duration,"selected deadline survives multiplayer setup transfer");
    }
    route.duration_s=120;
    check(race_gate_missed(route,1,{50,0,10},{50,0,30}),"wide detour beyond twenty metres warns");
    check(race_gate_missed(route,1,{0,25,10},{0,25,30}),"flying over mandatory gate warns");
    check(!race_gate_missed(route,1,{0,0,10},{0,0,30}),"valid crossing does not warn");
    check(!race_gate_missed(route,1,{50,0,10},{50,0,15}),"approaching a gate does not warn");
    route.gate_kinds={RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::mandatory};
    check(!race_gate_missed(route,1,{50,0,10},{50,0,30}),"optional branch is never a missed required gate");
    route.points={{0,0,0},{10,3,20}};route.yaws={0,90};route.race_finish=false;
    const Vec3 raw{25.3f,7.2f,20.9f};
    const auto snap=snap_race_gate(route,raw,94,true);
    check(std::abs(snap.at[0]-25.f)<.001f && std::abs(snap.at[2]-20.f)<.001f && snap.at[1]==raw[1] && snap.yaw==90,"snap aligns previous gate axis and yaw while keeping ground height");
    const auto free=snap_race_gate(route,raw,94,false);check(free.at==raw && free.yaw==94,"snap off preserves exact placement for forks");
    const auto fork=snap_race_gate(route,{25.3f,7,28.1f},139,true);
    check(fork.at[2]==28.f && fork.yaw==135,"fork does not attract a distant centre line and rotates in fifteen-degree steps");
    route.race_finish=true;
    const auto before_finish=snap_race_gate(route,{.7f,8,12.2f},359,true);
    check(before_finish.at[0]==0 && before_finish.at[2]==12 && before_finish.yaw==0,"snap references the last placed gate rather than reserved finish; yaw wraps");
}
void race_optional_flow() {
    Settings s;s.mode=Mode::race;s.race_finish=true;
    s.points={{0,0,0},{5,0,0},{10,0,-5},{10,0,5},{20,0,0},{30,0,-5},{30,0,5},{40,0,0},{50,0,0}};
    s.gate_kinds={RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::guiding,RaceGateKind::guiding,
        RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::guiding,RaceGateKind::mandatory,RaceGateKind::mandatory};
    check(mandatory_gates(s,s.points.size())==2,"required count ignores both fork branches and finish");
    Referee r(s,a,13);r.add_player(a);r.add_player(b);r.add_player(c);r.start(0);r.tick(countdown_ms);
    const auto of=[&](std::uint64_t id,std::uint64_t time){auto m=r.state(time);return *std::find_if(m.standings.begin(),m.standings.end(),[&](const auto& p){return p.player==id;});};
    check(of(a,countdown_ms).score==4,"race begins at first required gate beyond white guides");
    r.event(a,Event::checkpoint,8,s.points[8],1,countdown_ms+1);r.event(a,Event::checkpoint,7,s.points[7],2,countdown_ms+2);
    check(of(a,countdown_ms+2).score==4,"finish and later required gate blocked while mandatory gate missing");
    for(int i=1;i<4;++i)r.event(a,Event::checkpoint,i,s.points[i],i+2,countdown_ms+10+i);
    check(of(a,countdown_ms+15).score==4,"either or both guide branches never alter required progress");
    r.event(a,Event::checkpoint,4,{20,30,0},6,countdown_ms+16);
    check(of(a,countdown_ms+16).score==4,"crossing above mandatory arch refused");
    r.event(a,Event::checkpoint,4,s.points[4],7,countdown_ms+20);
    check(of(a,countdown_ms+20).score==7 && mandatory_gates(s,7)==1,"required crossing advances past all optional guides");
    r.event(b,Event::position,0,{10,0,0},1,countdown_ms+21);r.event(c,Event::position,0,{19,0,0},1,countdown_ms+21);
    const auto ranked=r.state(countdown_ms+22);check(ranked.standings[0].player==a && ranked.standings[1].player==c,"required progress then fresh distance determines racing places");
    r.event(a,Event::checkpoint,7,s.points[7],8,countdown_ms+30);r.event(a,Event::checkpoint,8,s.points[8],9,countdown_ms+40);
    check(of(a,countdown_ms+40).score==9 && of(a,countdown_ms+40).aux==40,"required route finishes without crossing any guides");
}
void race_stock_policy_flow() {
    using namespace dingosdk;
    set_session_tools_allowed(true,true,true);set_session_tuning_enforced(false);
    set_competition_stock_physics(true);
    check(!session_noclip_allowed() && !session_no_bail_allowed() && !session_boosts_allowed() && session_tuning_enforced(),"race overrides host/admin tools and forces stock physics offline too");
    set_competition_stock_physics(false);
    check(session_noclip_allowed() && session_no_bail_allowed() && session_boosts_allowed() && !session_tuning_enforced(),"leave restores retained free-skate permissions");
    set_session_tools_allowed(false,true,false);set_session_tuning_enforced(true);set_competition_stock_physics(true);set_competition_stock_physics(false);
    check(!session_noclip_allowed() && session_no_bail_allowed() && !session_boosts_allowed() && session_tuning_enforced(),"leaving race respects existing session policy");
    set_session_tools_allowed(true,true,true);set_session_tuning_enforced(false);
}

// Each Deathrace gate has its own width: it goes over the wire, bad ones are refused, and the
// referee takes a crossing as far from the gate's middle as the gate is wide.
void gate_width_flow() {
    Settings s;
    s.mode = Mode::race;
    s.points = {{0, 0, 0}, {50, 0, 0}, {100, 0, 0}};
    s.yaws = {90, 90, 90};
    s.widths = {6, 25, 6};
    Message setup;
    setup.kind = Message::Kind::setup;
    setup.leader = a;
    setup.game = 9;
    setup.settings = s;
    check(decode(encode(setup)) == setup, "gate widths round trip");
    check(gate_half_width(s, 1) == 25 && gate_half_width(Settings{}, 0) == Settings{}.radius, "a gate's own width, else the radius");
    auto bad = setup;
    bad.settings.widths = {6, 6};
    bool refused = false;
    try { (void)encode(bad); } catch (const std::invalid_argument &) { refused = true; }
    check(refused, "one width per gate");
    bad.settings.widths = {6, 0.1f, 6};
    refused = false;
    try { (void)encode(bad); } catch (const std::invalid_argument &) { refused = true; }
    check(refused, "a gate is at least a few metres wide");
    Referee r(s, a, 9);
    r.add_player(a);
    r.add_player(b);
    r.start(0);
    r.tick(countdown_ms);
    r.event(a, Event::checkpoint, 0, {0, 0, 2}, 1, countdown_ms + 100);
    r.event(a, Event::checkpoint, 1, {50, 0, 20}, 2, countdown_ms + 200); // 20 m off the middle of a 50 m gate
    r.event(b, Event::checkpoint, 1, {50, 0, 40}, 1, countdown_ms + 300); // outside the wide gate's accepted edge
    const auto st = r.state(countdown_ms + 300);
    const auto score = [&](std::uint64_t id) {
        for (const auto &p : st.standings) if (p.player == id) return p.score;
        return -1;
    };
    check(score(a) == 2, "a wide gate takes a crossing near its edge");
    check(score(b) == 1, "a gate does not take a crossing far outside it");
    const auto fast=race_gate_crossing(s,1,{30,0,20},{70,0,20});
    check(fast.crossed && fast.inside && std::abs(fast.at[0]-50)<.001f,"fast movement crossing is interpolated on the gate plane");
    check(race_gate_crossing(s,1,{70,0,20},{30,0,20}).inside,"reversed gate orientation still counts a crossing");
    check(!race_gate_crossing(s,1,{30,0,30},{70,0,30}).inside,"movement outside the posts does not count");
    check(!race_gate_crossing(s,1,{30,50,0},{70,50,0}).inside,"movement above a scaled gate does not count");
    check(!race_gate_crossing(s,1,{30,0,0},{40,0,0}).crossed,"approaching the gate does not count as crossing");
}

// Skate Tag: positions decide who tags whom; no tag-backs for a few seconds; least time it wins.
void skate_tag_flow() {
    Settings s;
    s.mode = Mode::tag;
    s.radius = default_tag_reach;
    s.duration_s = 60;
    Referee r(s, a, 4); // game 4 with three players: players[4 % 3] = b starts it
    r.add_player(a);
    r.add_player(b);
    r.add_player(c);
    r.start(0);
    r.tick(countdown_ms);
    std::uint64_t t = countdown_ms;
    check(r.state(t).turn == b, "the game's id picks who starts it");
    std::uint32_t seq_a = 0, seq_b = 0, seq_c = 0;
    r.event(a, Event::position, 0, {0, 0, 0}, ++seq_a, t);
    r.event(c, Event::position, 0, {50, 0, 0}, ++seq_c, t);
    r.event(b, Event::position, 0, {10, 0, 0}, ++seq_b, t);
    check(r.state(t).turn == b, "out of reach: nobody tagged");
    t += 2000;
    r.tick(t);
    r.event(a, Event::position, 0, {0, 0, 0}, ++seq_a, t);
    r.event(b, Event::position, 0, {2, 0, 0}, ++seq_b, t);
    check(r.state(t).turn == a, "within reach: b tagged a");
    t += 1000;
    r.tick(t);
    r.event(b, Event::position, 0, {1, 0, 0}, ++seq_b, t);
    r.event(a, Event::position, 0, {1.5f, 0, 0}, ++seq_a, t);
    check(r.state(t).turn == a, "no tag-backs straight away");
    t += no_tag_back_ms;
    r.tick(t);
    r.event(b, Event::position, 0, {1, 0, 0}, ++seq_b, t);
    r.event(a, Event::position, 0, {1.5f, 0, 0}, ++seq_a, t);
    check(r.state(t).turn == b, "a tag-back once the wait is over");
    t += 2000;
    r.event(c, Event::position, 0, {1.2f, 0, 0}, ++seq_c, t); // c's position is fresh, b's is not
    check(r.state(t).turn == b, "an old position tags nobody");
    r.tick(countdown_ms + 60000);
    const auto st = r.state(countdown_ms + 60000);
    check(r.phase() == Phase::results && st.standings.front().player == c, "c was never it and wins");
    check(st.standings.back().score > st.standings.front().score, "time spent it is counted");
    Message move;
    move.kind = Message::Kind::event;
    move.leader = a;
    move.game = 4;
    move.event = Event::position;
    move.at = {12.5f, 3, -7};
    move.sequence = 3;
    check(decode(encode(move)) == move, "a position round trips");
}

void domination_flow() {
    Settings s;
    s.mode = Mode::domination;
    s.points = {{0, 0, 0}, {100, 0, 0}};
    Referee r(s, a, 4);
    r.add_player(a);
    r.add_player(b);
    r.start(0);
    r.tick(countdown_ms);
    r.event(a, Event::line, 500, {1, 0, 1}, 1, countdown_ms);
    r.tick(countdown_ms + 3000);
    check(r.state(countdown_ms + 3000).standings[0].score == 3, "a spot held for 3 s is 3 points");
    r.event(b, Event::line, 400, {1, 0, 1}, 1, countdown_ms + 3000);
    check(r.state(countdown_ms + 3000).zones[0].owner == a, "a smaller line does not take it");
    r.event(b, Event::line, 600, {1, 0, 1}, 2, countdown_ms + 3000);
    check(r.state(countdown_ms + 3000).zones[0].owner == b, "a bigger line does");
}

void tag_flow() {
    // A curb ground twice from different ends is one curb; a ledge beside it is another.
    const Tag curb{TagKind::grind, {{0, 0.2f, 0}, {2, 0.2f, 0}, {4, 0.2f, 0}}};
    const Tag same_curb{TagKind::grind, {{3, 0.25f, 0.3f}, {5, 0.2f, 0.2f}, {6, 0.2f, 0}}};
    const Tag ledge{TagKind::grind, {{0, 0.5f, 5}, {4, 0.5f, 5}}};
    check(same_tag(curb, same_curb) && !same_tag(curb, ledge), "grinds on one curb are the same tag");
    const Tag stairs{TagKind::gap, {{10, 2, 10}, {13, 0, 10}}}, stairs_again{TagKind::gap, {{10.5f, 2, 9}, {14, 0, 11}}};
    check(same_tag(stairs, stairs_again) && !same_tag(stairs, curb), "a gap jumped again is the same gap");
    std::vector<Vec3> long_path;
    for (int i = 0; i <= 30; ++i) long_path.push_back({static_cast<float>(i), 0, 0});
    const auto thin = thin_path(long_path);
    check(thin.size() == max_tag_points && thin.front()[0] == 0 && thin.back()[0] == 30, "a long grind keeps its ends");
    Message event;
    event.kind = Message::Kind::event;
    event.leader = a;
    event.game = 4;
    event.value = 900;
    event.sequence = 1;
    event.tags = {curb, stairs};
    check(decode(encode(event)) == event, "a line's tags round trip");
    Message shapes;
    shapes.kind = Message::Kind::tags;
    shapes.leader = a;
    shapes.game = 4;
    shapes.first = 3;
    shapes.tags = {ledge};
    check(decode(encode(shapes)) == shapes, "tag shapes round trip");
}

void graffiti_flow() {
    Settings s;
    s.mode = Mode::graffiti;
    s.corners = square;
    Referee r(s, a, 5);
    r.add_player(a);
    r.add_player(b);
    r.add_player(c);
    r.start(0);
    r.tick(countdown_ms);
    const Tag curb{TagKind::grind, {{5, 0.2f, 5}, {10, 0.2f, 5}}};
    const Tag rail{TagKind::grind, {{30, 0.8f, 30}, {35, 0.8f, 30}}};
    const Tag outside{TagKind::grind, {{90, 0.2f, 90}, {95, 0.2f, 90}}};
    r.event(a, Event::line, 500, {10, 0, 5}, 1, countdown_ms, {curb});
    r.event(a, Event::line, 500, {35, 0, 30}, 2, countdown_ms, {rail, outside});
    check(r.tags().size() == 2, "two tags; the one outside the area is not");
    r.event(b, Event::line, 400, {10, 0, 5}, 1, countdown_ms, {{TagKind::grind, {{6, 0.2f, 5}, {9, 0.2f, 5}}}});
    check(r.state(countdown_ms).zones[0].owner == a, "a smaller line does not steal the curb");
    r.event(b, Event::line, 800, {10, 0, 5}, 2, countdown_ms, {{TagKind::grind, {{6, 0.2f, 5}, {9, 0.2f, 5}}}});
    auto st = r.state(countdown_ms);
    check(r.tags().size() == 2 && st.zones.size() == 2, "the same curb, still two tags");
    check(st.standings[0].score == 1 && st.standings[1].score == 1, "b stole one of a's");
    const auto calls = r.take_calls();
    check(!calls.empty() && calls.back().find("stole") != std::string::npos, "the steal is called out");
    const auto shown = r.state(countdown_ms);
    check(shown.calls.size() <= max_calls && shown.calls.back() == calls.back(), "the state carries the latest calls");
    check(decode(encode(shown)) == shown, "a real state round trips");
    check(r.state(countdown_ms).standings.size() == 3, "everyone is on the board");
}

void infection_flow() {
    Settings s;
    s.mode = Mode::infection;
    s.radius = default_tag_reach;
    s.duration_s = 120;
    Referee r(s, a, 9); // game 9 with three players: players[9 % 3] = a is patient zero
    r.add_player(a);
    r.add_player(b);
    r.add_player(c);
    r.start(0);
    r.tick(countdown_ms);
    std::uint64_t t = countdown_ms;
    std::uint32_t sa = 0, sb = 0, sc = 0;
    const auto of = [&](const Message &m, std::uint64_t id) {
        return *std::find_if(m.standings.begin(), m.standings.end(), [&](const Standing &x) { return x.player == id; });
    };
    auto st = r.state(t);
    check(of(st, a).up && of(st, a).aux == 1 && !of(st, b).up && !of(st, c).up, "a is patient zero");
    r.event(a, Event::position, 0, {0, 0, 0}, ++sa, t);
    r.event(b, Event::position, 0, {20, 0, 0}, ++sb, t);
    r.event(c, Event::position, 0, {60, 0, 0}, ++sc, t);
    t += 10000;
    r.tick(t);
    st = r.state(t);
    check(of(st, b).score == 100 && of(st, c).score == 100 && of(st, a).score == 0, "survivors score their time clean");
    r.event(b, Event::position, 0, {20, 0, 0}, ++sb, t);
    r.event(a, Event::position, 0, {18, 0, 0}, ++sa, t);
    st = r.state(t);
    check(of(st, b).up && !of(st, c).up && r.phase() == Phase::playing, "a infected b; c survives");
    t += 5000;
    r.tick(t);
    r.event(c, Event::position, 0, {60, 0, 0}, ++sc, t);
    r.event(b, Event::position, 0, {58, 0, 0}, ++sb, t); // the newly infected hunt too
    check(r.phase() == Phase::results, "b infected c, the last survivor: game over");
    st = r.state(t);
    check(st.standings.front().player == c && of(st, c).score == 150 && of(st, b).score == 100, "c survived longest and wins");
    check(decode(encode(st)) == st, "an Infection state round trips");
    Message setup;
    setup.kind = Message::Kind::setup;
    setup.leader = a;
    setup.game = 9;
    setup.settings = s;
    check(decode(encode(setup)) == setup && parse_mode("zombies") == Mode::infection, "the Infection setup round trips");
}

void hide_flow() {
    Settings s;
    s.mode = Mode::hide;
    s.radius = default_tag_reach;
    s.duration_s = 120;
    s.turn_s = 20; // hiding time
    Referee r(s, a, 9); // players[9 % 3] = a seeks
    r.add_player(a);
    r.add_player(b);
    r.add_player(c);
    r.start(0);
    r.tick(countdown_ms);
    std::uint64_t t = countdown_ms;
    std::uint32_t sa = 0, sb = 0, sc = 0;
    const auto of = [&](const Message &m, std::uint64_t id) {
        return *std::find_if(m.standings.begin(), m.standings.end(), [&](const Standing &x) { return x.player == id; });
    };
    auto st = r.state(t);
    check(of(st, a).up && !of(st, b).up && st.target == 20000, "a seeks; 20 s to hide");
    // Right next to b while still counting: nobody is found, and hiding scores nothing.
    t += 10000;
    r.tick(t);
    r.event(b, Event::position, 0, {1, 0, 0}, ++sb, t);
    r.event(a, Event::position, 0, {0, 0, 0}, ++sa, t);
    st = r.state(t);
    check(!of(st, b).up && of(st, b).score == 0 && st.target == 10000, "nobody is found while the seeker counts");
    t += 10000;
    check(r.tick(t), "the seekers come out at once");
    t += 5000;
    r.tick(t);
    st = r.state(t);
    check(st.target == 0 && of(st, b).score == 50 && of(st, c).score == 50, "hidden time counts from the release");
    r.event(b, Event::position, 0, {40, 0, 0}, ++sb, t);
    r.event(c, Event::position, 0, {80, 0, 0}, ++sc, t);
    r.event(a, Event::position, 0, {39, 0, 0}, ++sa, t);
    st = r.state(t);
    check(of(st, b).up && of(st, a).aux == 1 && !of(st, c).up, "a found b, who seeks now");
    t += 3000;
    r.tick(t);
    r.event(c, Event::position, 0, {80, 0, 0}, ++sc, t);
    r.event(b, Event::position, 0, {79, 0, 0}, ++sb, t);
    check(r.phase() == Phase::results, "b found c, the last one hiding");
    st = r.state(t);
    check(st.standings.front().player == c && of(st, c).score == 80 && of(st, b).aux == 1, "c hid longest and wins");
    check(decode(encode(st)) == st && parse_mode("hideandseek") == Mode::hide, "a Hide & Seek state round trips");
}

void skate_flow() {
    check(trick_kinds_of("Kickflip") == trick_flips && trick_kinds_of("BS 50-50 Grind") == trick_grinds, "flips and grinds are told apart");
    check(trick_kinds_of("Heelflip + Seatbelt") == (trick_flips | trick_grabs) && trick_kinds_of("Manual") == trick_manuals,
          "a combo is every kind its parts are");
    check(trick_kinds_of("Rocket Air") == trick_grabs && trick_kinds_of("FS Powerslide") == trick_grinds, "grabs and slides");
    check(skate_letters(3, 5) == "S.K.A" && skate_letters(0, 5).empty(), "letters spell S.K.A.T.E.");
    Settings s;
    s.mode = Mode::skate;
    s.strikes = 2;
    s.trick_kinds = trick_flips | trick_grinds; // no grabs, no manuals
    check(trick_allowed(s, "Kickflip") && !trick_allowed(s, "Heelflip + Seatbelt") && !trick_allowed(s, "Manual"), "the leader's kinds");
    Referee r(s, a, 8);
    r.add_player(a);
    r.add_player(b);
    r.add_player(c);
    r.start(0);
    r.tick(countdown_ms);
    std::uint64_t t = countdown_ms;
    std::uint32_t sa = 0, sb = 0, sc = 0;
    const Vec3 here{};
    auto st = r.state(t);
    check(st.turn == a && st.setter == a && st.trick.empty(), "a sets first");
    r.event(b, Event::trick, 1, here, ++sb, t, {}, "Kickflip");
    check(r.state(t).trick.empty(), "only the player up counts");
    r.event(a, Event::trick, 1, here, ++sa, t, {}, "Heelflip + Seatbelt");
    st = r.state(t);
    check(st.setter == b && st.turn == b && st.trick.empty(), "a set a grab in a game without grabs: b sets");
    r.event(b, Event::trick, 1, here, ++sb, t, {}, "Kickflip");
    st = r.state(t);
    check(st.trick == "Kickflip" && st.turn == c, "b set the kickflip; c copies");
    r.event(c, Event::trick, 1, here, ++sc, t, {}, "kickflip");
    check(r.state(t).turn == a, "c landed it (any case); a copies");
    r.event(a, Event::trick, 1, here, ++sa, t, {}, "Heelflip");
    st = r.state(t);
    check(st.turn == b && st.trick.empty(), "round over: b sets again");
    check(std::find_if(st.standings.begin(), st.standings.end(), [](const Standing &x) { return x.player == a; })->aux == 1,
          "a did the wrong trick: S");
    r.event(b, Event::trick, 1, here, ++sb, t, {}, "Kickflip");
    st = r.state(t);
    check(st.setter == c && st.turn == c, "a trick set already cannot be set again: c sets");
    r.event(c, Event::trick, 0, here, ++sc, t, {}, "");
    check(r.state(t).setter == a, "a missed set passes it on, no letter");
    r.event(a, Event::trick, 1, here, ++sa, t, {}, "BS 50-50 Grind");
    r.event(b, Event::trick, 1, here, ++sb, t, {}, "BS 50-50 Grind");
    t += s.turn_s * 1000ull; // c never tries
    r.tick(t);
    st = r.state(t);
    check(std::find_if(st.standings.begin(), st.standings.end(), [](const Standing &x) { return x.player == c; })->aux == 1,
          "running out of time is a letter");
    check(st.turn == a && st.setter == a, "back to the setter");
    r.event(a, Event::trick, 1, here, ++sa, t, {}, "Varial Kickflip");
    r.event(b, Event::trick, 0, here, ++sb, t, {}, "");
    r.event(c, Event::trick, 0, here, ++sc, t, {}, "");
    check(r.phase() == Phase::playing && r.state(t).turn == a, "c spelled S.K. (two letters) and is out; b has an S");
    r.event(a, Event::trick, 1, here, ++sa, t, {}, "Hardflip");
    r.event(b, Event::trick, 1, here, ++sb, t, {}, "Kickflip");
    check(r.phase() == Phase::results, "b missed the hardflip: S.K., out, a wins");
    st = r.state(t);
    check(st.standings.front().player == a, "a is first");
    check(decode(encode(st)) == st, "a S.K.A.T.E. state round trips");
    Message attempt;
    attempt.kind = Message::Kind::event;
    attempt.leader = a;
    attempt.game = 8;
    attempt.event = Event::trick;
    attempt.value = 1;
    attempt.trick = "Heelflip + Seatbelt";
    check(decode(encode(attempt)) == attempt, "an attempt round trips");
    Message setup;
    setup.kind = Message::Kind::setup;
    setup.leader = a;
    setup.game = 8;
    setup.settings = s;
    check(decode(encode(setup)) == setup, "the trick kinds go with the setup");
    Message hello;
    hello.kind = Message::Kind::hello;
    hello.leader = b;
    hello.game = 1;
    check(decode(encode(hello)) == hello, "a hello round trips");
    auto other = encode(hello);
    other[1] = static_cast<std::uint8_t>(wire_version - 1);
    check(!decode(other) && message_version(other) == wire_version - 1, "another version's hello is told apart, not read");
}
} // namespace

void race_saved_course_and_recovery() {
    using namespace dingosdk::modes;
    Settings route;route.mode=Mode::race;bool finish=false;
    check(edit_race_route(route,finish,RaceObject::start,{0,0,0},0,6),"memory start");
    check(edit_race_route(route,finish,RaceObject::checkpoint,{0,0,20},0,12),"memory mandatory");
    check(edit_race_route(route,finish,RaceObject::guiding,{10,0,10},90,6),"memory fork guide");
    check(edit_race_route(route,finish,RaceObject::finish,{0,0,40},0,6),"memory finish");
    RaceCourseMemory memory;memory.save("map-a",route);
    auto restored=memory.restore("map-a");check(restored && restored->points==route.points && restored->widths==route.widths && restored->gate_kinds==route.gate_kinds && race_route_ready(*restored,true),"finish/cancel retains complete course");
    auto reopened=*restored;bool reopened_finish=reopened.race_finish;
    check(accept_race_flag(reopened,reopened_finish,{2,0,0},90,6)==RaceFlagTransition::edit_route && race_route_ready(reopened,reopened_finish),
        "reopening a remembered complete course always allows editing and unchanged save");
    clear_race_gates(reopened,reopened_finish);
    check(reopened.points.size()==1 && !race_route_ready(reopened,reopened_finish),"clear in reopened builder retains start and removes old finish");
    check(edit_race_route(reopened,reopened_finish,RaceObject::checkpoint,{20,0,0},90,6) &&
        edit_race_route(reopened,reopened_finish,RaceObject::finish,{40,0,0},90,6) && race_route_ready(reopened,reopened_finish),
        "remembered course can be replaced with new gates and one finish");
    memory.save("map-a",reopened);check(memory.restore("map-a")->points==reopened.points,"edited replacement updates course memory");
    memory.save("map-a",*restored);
    check(!memory.restore("map-b"),"courses cannot leak to another map");
    route.points.clear();check(memory.restore("map-a")->points.size()==4,"match cleanup cannot mutate saved course");
    const auto pose=race_recovery_pose(*restored,3);check(pose.at==Vec3{0,0,21.5f},"recovery skips optional guides and continues from last mandatory");
    check(race_gate_crossing(*restored,1,{0,19,19},{0,19,21}).inside,"enlarged gate accepts enlarged vertical trigger");
    restored->widths[1]=6;check(!race_gate_crossing(*restored,1,{0,19,19},{0,19,21}).inside,"default height excludes high bypass");
    auto snap=snap_race_gate(Settings{},Vec3{0,0,0},26,true,45);check(snap.yaw==45,"selected angle is used by preview and placement");
    Settings alignment;alignment.mode=Mode::race;alignment.points={{0,0,0}};alignment.yaws={0};alignment.widths={6};
    const auto weak=snap_race_gate(alignment,{4,0,20},0,true,5);
    const auto strong=snap_race_gate(alignment,{4,0,20},0,true,90);
    check(weak.at[0]==4 && strong.at[0]==0,"larger snap angles increase positional alignment reach");
    check(snap_race_gate(alignment,{10,0,20},90,true,90).at[0]==10,"strong snapping still permits distinct fork branches");
    for(const auto angle:{5.f,15.f,30.f,45.f,90.f}) {
        const auto next=rotate_race_gate(2,1,true,angle);
        check(std::abs(next-angle)<.001f,"one rotate press advances the selected full snap step");
        check(std::abs(rotate_race_gate(next,-1,true,angle))<.001f,"one reverse press restores previous step");
    }
    check(std::abs(size_race_gate(6,1)-6.3f)<.001f && std::abs(size_race_gate(6,-1)-5.7f)<.001f,"size advances five percentage points per press");
    RaceEditorPresses presses;check(presses.update(8)==8 && presses.update(8)==0,"holding size does not repeat");
    presses.update(0);check(presses.update(8)==8,"size rearms after release");
    alignment.points.push_back({0,0,20});alignment.yaws.push_back(0);alignment.widths.push_back(24);
    bool pending=false;
    check(!race_miss_recovery_ready(alignment,1,{23,0,18},{23,0,21},pending),"large mandatory gate counts within opening without recovery");
    check(!race_miss_recovery_ready(alignment,1,{30,0,18},{30,0,20.5f},pending) && pending,"outside pass gives time to clear the plane");
    check(race_miss_recovery_ready(alignment,1,{30,0,20.5f},{30,0,24},pending),"actual missed mandatory recovers beyond grace distance");
    pending=false;check(!race_miss_recovery_ready(alignment,1,{30,0,22},{30,0,18},pending) && !pending,"backward approach cannot trigger mandatory recovery");
    pending=true;check(!race_miss_recovery_ready(alignment,1,{30,0,20.5f},{30,0,19},pending) && !pending,"returning before grace cancels false recovery");
    alignment.points.push_back({0,0,30});alignment.yaws.push_back(0);alignment.widths.push_back(40);
    pending=false;check(!race_miss_recovery_ready(alignment,1,{0,0,15},{0,0,18},pending),"nearby oversized later gate never penalizes entry into required gate");
    RaceClearHold hold;
    check(!hold.update(true,true,100) && !hold.update(true,true,1599),"tap cannot clear course");
    check(hold.update(true,true,1600) && !hold.update(true,true,1700),"full hold clears once");
    hold.update(false,true,1800);check(!hold.update(true,true,1900),"release resets hold");
    hold.update(true,false,3300);check(!hold.update(true,true,3400),"focus loss resets hold");
    memory.clear("map-a");check(!memory.restore("map-a"),"explicit clear removes saved course");
    clear_race_gates(*restored,finish);check(restored->points.size()==1 && !finish && !restored->race_finish && restored->widths.size()==1,"clear removes all gates and finish while retaining start flag");
}
int main() {
    try {
        area_flow();
        placement_camera_flow();
        scoring_flow();
        wire_flow();
        jam_flow();
        one_up_flow();
        race_flow();
        race_builder_flow();
        race_saved_course_and_recovery();
        race_clock_flow();
        race_custom_deadlines_and_snap();
        race_optional_flow();
        race_stock_policy_flow();
        gate_width_flow();
        skate_tag_flow();
        domination_flow();
        tag_flow();
        graffiti_flow();
        skate_flow();
        infection_flow();
        hide_flow();
        std::cout << "Game modes: area, scoring, wire, Spot Jam, 1-Up, Deathrace, Domination, tag, Graffiti and S.K.A.T.E. flows passed.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
