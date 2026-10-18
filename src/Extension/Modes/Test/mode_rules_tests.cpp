// The game modes' rules and wire format without the game: referees are driven by hand with
// the events players would send.
#include "Extension/Modes/mode_rules.h"
#include "Extension/Modes/race_route.h"
#include "Extension/Modes/race_editor.h"
#include "Extension/Modes/race_arrow_placement.h"
#include "Extension/Modes/race_momentum.h"
#include "Extension/Modes/race_navigation.h"
#include "Extension/Modes/race_sound.h"
#include "Extension/Modes/mode_announcements.h"
#include "Extension/Modes/mode_transport.h"
#include "Extension/Modes/mode_participation.h"
#include "Extension/Modes/mode_lobby.h"
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

void lobby_enrollment_flow() {
    Message state;state.kind=Message::Kind::state;state.leader=a;state.game=11;
    state.standings={{.player=a},{.player=b},{.player=c,.out=true}};
    const auto echoed=decode(encode(state));
    check(echoed && lobby_enrolled_count(*echoed)==2,"enrolled players count without any Ready inputs; departed players do not");
    state.standings.erase(state.standings.begin()+1);
    check(lobby_enrolled_count(state)==1,"leaving the lobby immediately removes the guest");
    state.standings.push_back({.player=b});
    check(lobby_enrolled_count(state)==2,"re-enrollment needs no additional confirmation");
    state.phase=Phase::playing;
    check(lobby_enrolled_count(state)==0,"gameplay standings are not a lobby count");
}

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
    for(const auto heading:{-170.f,-90.f,0.f,45.f,90.f,179.f}) {
        const float rad=heading*3.14159265f/180.f;
        Settings course;course.mode=Mode::race;
        course.points={{10,2,30},{10,8,30},{10+50*std::sin(rad),2,30+50*std::cos(rad)}};
        course.yaws={heading+180.f,0.f,heading};
        RaceCourseMemory memory;memory.save("first-map",course);
        Settings other=course;other.yaws[0]=0;memory.save("second-map",other);
        auto loaded=*memory.restore("first-map");
        loaded.yaws[0]=90; // Native flag re-placement on the saved map.
        const auto aligned=race_spawn_transform(loaded.points[0],race_start_heading(loaded));
        check(std::abs(aligned[8]-std::sin(rad))<.0001f && std::abs(aligned[10]-std::cos(rad))<.0001f,
            "Saved-route lineup faces the course despite flag re-placement and duplicate start points");
        check(loaded.yaws[0]==90 && loaded.points==course.points,
            "Lineup does not rotate or discard saved gates");
    }
    Settings s;s.mode=Mode::race;bool finish{};
    Settings restored;restored.mode=Mode::race;restored.points={{0,0,0},{0,0,50},{0,0,100}};
    restored.yaws={90,0,15};align_race_start(restored);
    check(race_gate_crossing(restored,0,{0,0,-1.5f},{0,0,1.5f}).inside,
        "A restored race's corrected spawn crosses its corrected start plane");
    check(restored.yaws==std::vector<float>{0,0,15},"Start alignment preserves other gate rotations");
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
        cycle_race_object(RaceObject::finish,false)==RaceObject::guiding && cycle_race_object(RaceObject::guiding,false)==RaceObject::arrow &&
        cycle_race_object(RaceObject::arrow,false)==RaceObject::checkpoint && cycle_race_object(RaceObject::checkpoint,true)==RaceObject::arrow &&
        cycle_race_object(RaceObject::arrow,true)==RaceObject::guiding && cycle_race_object(RaceObject::guiding,true)==RaceObject::finish,
        "editor cycles Required, Finish, Guide and floating Arrow in both directions");
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
    check(of(a,countdown_ms).aux==0 && of(b,countdown_ms).aux==0,"elapsed race timers start at zero on GO");
    check(of(a,countdown_ms+500).aux==500,"elapsed timer advances before the first gate");
    r.event(a,Event::checkpoint,1,{20,0,0},1,countdown_ms+1000);
    check(of(a,countdown_ms+1000).remaining_ms==119000 && of(b,countdown_ms+1000).remaining_ms==119000,"gate awards no time and racers share deadline");
    r.tick(countdown_ms+89999);check(!race_clock_visible(of(a,countdown_ms+89999).remaining_ms),"timer hidden until ninety seconds");
    r.tick(countdown_ms+90000);check(race_clock_visible(of(a,countdown_ms+90000).remaining_ms),"timer appears with thirty seconds remaining");
    r.event(a,Event::checkpoint,2,{40,0,0},2,countdown_ms+119998);
    r.event(a,Event::checkpoint,3,{60,0,0},3,countdown_ms+119999);
    r.tick(countdown_ms+120000);
    check(r.phase()==Phase::results && of(b,countdown_ms+120000).out && !of(a,countdown_ms+120000).out &&
        r.state(countdown_ms+120000).standings.front().player==a,"unfinished racer DNF; finish just before deadline wins");
    check(of(a,countdown_ms+120000).aux==119999 && of(a,countdown_ms+125000).aux==119999,"finisher elapsed time freezes through results");
    check(of(b,countdown_ms+125000).aux==120000,"DNF elapsed time freezes at deadline");
    const auto timed_state=r.state(countdown_ms+120000);
    check(decode(encode(timed_state))==timed_state,"per-player elapsed times survive state transfer");
    check(!race_clock_visible(0),"no clock left after deadline");
    Referee late(s,a,4);late.add_player(a);late.start(0);late.tick(countdown_ms);
    late.event(a,Event::checkpoint,1,{20,0,0},1,countdown_ms+1);late.event(a,Event::checkpoint,2,{40,0,0},2,countdown_ms+2);
    late.event(a,Event::checkpoint,3,{60,0,0},3,countdown_ms+120000);late.tick(countdown_ms+120000);
    check(late.phase()==Phase::results && late.state(countdown_ms+120000).standings.front().out,"finish at exact deadline is DNF");
    Referee early(s,a,5);early.add_player(a);early.start(0);early.tick(countdown_ms);
    for(int i=1;i<4;++i)early.event(a,Event::checkpoint,i,s.points[i],i,countdown_ms+i);
    early.tick(countdown_ms+4);check(early.phase()==Phase::results,"all finished ends race without waiting two minutes");
    check(early.state(countdown_ms+9000).standings.front().aux==3,"results do not restart the race timer");
    Referee leaving(s,a,6);leaving.add_player(a);leaving.add_player(b);leaving.start(0);leaving.tick(countdown_ms);
    leaving.remove_player(b,countdown_ms+1234);
    const auto left=leaving.state(countdown_ms+9000);
    const auto departed=std::find_if(left.standings.begin(),left.standings.end(),[&](const auto& p){return p.player==b;});
    check(departed!=left.standings.end() && departed->out && departed->aux==1234,"departed racer timer freezes when leaving");
}
void race_arrow_surface_placement() {
    // Analytic collision fixtures exercise complete placements on walls,
    // floors, slopes and corners, independent of native physics availability.
    const auto plane=[](Vec3 point,Vec3 normal) {
        return [=](const Vec3& origin,const Vec3& direction,float length) {
            const float denominator=arrow_dot(direction,normal);
            if(std::abs(denominator)<.00001f)return RaceArrowRay{true,false,{}};
            const float distance=arrow_dot(arrow_add(point,origin,-1.f),normal)/denominator;
            return distance>=0 && distance<=length?RaceArrowRay{true,true,arrow_add(origin,direction,distance)}:RaceArrowRay{true,false,{}};
        };
    };
    const auto wall=plane({0,0,8},{0,0,-1});const Vec3 eye{0,3,0};
    const auto aim=aim_race_arrow(eye,{0,0,1},{1,0,0},{0,1,0},wall);
    check(aim.valid && aim.surface.contact && std::abs(aim.at[2]-8)<.001f && aim.surface.normal[2]<-.99f,
        "chevrons stop on the visible wall instead of passing through it at twelve metres");
    check(aim.align_surface,"a direct wall hit mounts automatically, including nearer than the free-air preview distance");
    const auto close_surface=plane({0,0,11.8f},{0,0,-1});
    check(aim_race_arrow(eye,{0,0,1},{1,0,0},{0,1,0},close_surface).align_surface,
        "bringing the preview close to a wall still permits surface alignment");
    const auto angles=race_arrow_surface_angles(aim.surface.normal);
    const auto mounted=clear_race_arrow_surface(aim.at,angles[0],angles[1],0,7.5f,aim.surface);
    check(std::abs(mounted[2]-7.96f)<.001f && race_arrow_clear(eye,mounted,angles[0],angles[1],0,7.5f,wall),
        "wall-mounted full-size chevron stays flush with a small visible clearance");
    const auto turned=clear_race_arrow_surface(aim.at,90,0,0,7.5f,aim.surface);
    check(turned[2]<5.47f && race_arrow_clear(eye,turned,90,0,0,7.5f,wall),
        "rotating an arrow against a wall clears its whole footprint, not only its center");
    check(!race_arrow_clear(eye,{0,3,8.1f},180,0,0,7.5f,wall),"saving a chevron hidden inside a wall is refused");
    const auto ground=plane({0,0,0},{0,1,0});const Vec3 down=arrow_unit({0,-1,1}),camera_up=arrow_unit({0,1,1});
    const auto floor=aim_race_arrow(eye,down,{1,0,0},camera_up,ground);
    check(floor.valid && floor.align_surface && floor.surface.contact && floor.surface.normal[1]>.99f,"ground placement recovers the real floor plane");
    const auto floor_angles=race_arrow_surface_angles(floor.surface.normal);
    const auto flat=clear_race_arrow_surface(floor.at,floor_angles[0],floor_angles[1],0,7.5f,floor.surface);
    check(std::abs(floor_angles[1]+90)<.001f && std::abs(flat[1]-.04f)<.001f &&
        race_arrow_clear(eye,flat,floor_angles[0],floor_angles[1],0,7.5f,ground),"arrows can be mounted flat on the ground");
    const auto snapped_below=clear_race_arrow_surface({0,-2,3},0,-90,0,7.5f,floor.surface);
    check(std::abs(snapped_below[1]-.04f)<.001f,"height snapping cannot sink a surface-mounted arrow below the ground");
    const auto slope=plane({0,0,4},arrow_unit({0,1,-1}));
    const auto ramp=aim_race_arrow(eye,{0,0,1},{1,0,0},{0,1,0},slope);
    const auto ramp_angles=race_arrow_surface_angles(ramp.surface.normal);
    check(ramp.valid && std::abs(ramp_angles[1]+45)<.001f,"sloped meshes supply the mounted arrow's actual tilt");
    const auto corner=[&](const Vec3& origin,const Vec3& direction,float length) {
        const auto main=wall(origin,direction,length);
        const auto side=plane({1,0,0},{-1,0,0})(origin,direction,length);
        if(!main.hit)return side;if(!side.hit)return main;
        return arrow_dot(arrow_add(main.at,origin,-1.f),direction)<arrow_dot(arrow_add(side.at,origin,-1.f),direction)?main:side;
    };
    check(!race_arrow_clear(eye,mounted,180,0,0,7.5f,corner),"an adjacent wall crossing an arrow edge blocks placement");
    const auto empty=[](const Vec3&,const Vec3&,float){return RaceArrowRay{true,false,{}};};
    const auto floating=aim_race_arrow(eye,{0,0,1},{1,0,0},{0,1,0},empty);
    check(floating.valid && !floating.surface.contact && floating.at==Vec3{0,3,12},"clear air still supports floating arrows");
    const auto foreground=[](const Vec3& origin,const Vec3& direction,float length) {
        // A small foreground object obscures only the camera-to-upper-corner
        // rays from the old validator. It does not intersect the actual quad.
        if(origin[2]<.01f && direction[0]>.1f && direction[1]>.1f)
            return RaceArrowRay{true,true,arrow_add(origin,direction,std::min(1.f,length))};
        return RaceArrowRay{true,false,{}};
    };
    check(race_arrow_clear(eye,floating.at,180,0,0,7.5f,foreground),
        "partial foreground occlusion cannot reject a clear airborne Chevron");
    check(race_arrow_preview_visible(true,false,false) && race_arrow_preview_visible(false,true,true) &&
        !race_arrow_preview_visible(false,false,false),"a blocked placement keeps its preview visible while a missing camera does not invent one");
    const auto unavailable=[](const Vec3&,const Vec3&,float){return RaceArrowRay{};};
    check(!aim_race_arrow(eye,{0,0,1},{1,0,0},{0,1,0},unavailable).valid &&
        !race_arrow_clear(eye,mounted,180,0,0,7.5f,unavailable),"unavailable collision data cannot authorize a through-mesh placement");
    const auto one_sided_ground=[&](const Vec3& origin,const Vec3& direction,float length) {
        if(origin[1]<0 || direction[1]>=0)return RaceArrowRay{true,false,{}};
        return ground(origin,direction,length);
    };
    for(const float width:{1.5f,7.5f,40.f})for(const float pitch:{-90.f,-60.f,-15.f,0.f,35.f,90.f})for(const float roll:{0.f,30.f,90.f}) {
        const auto safe=clear_race_arrow_ground({0,1,3},25,pitch,roll,width,one_sided_ground);
        check(safe.available && safe.at[1]>=1,"footprint clearance only raises a penetrating arrow, never pulls floating placement downward");
        for(unsigned part=0;part<2;++part) {
            const auto mesh=race_arrow_layer_transform(safe.at,25,roll,width,pitch,part);
            for(const float x:{-.5f,.5f})for(const float y:{-.5f,.5f})
                check(mesh[13]+x*mesh[1]+y*mesh[5]>=.02f,"both native Chevron layers stay above one-sided road meshes at all supported sizes and tilts");
        }
    }
    const auto air_clear=clear_race_arrow_ground({0,20,3},0,0,0,7.5f,one_sided_ground);
    check(air_clear.at==Vec3{0,20,3},"a road far below an airborne arrow cannot attract it onto the floor");
    const auto flat_clear=clear_race_arrow_ground(flat,0,-90,0,7.5f,one_sided_ground);
    check(flat_clear.at==flat,"already flush floor placement does not creep upwards every frame");
    const auto slope_clear=clear_race_arrow_ground({0,1,4},0,0,0,7.5f,slope);
    const auto slope_mesh=race_arrow_transform(slope_clear.at,0,0,7.5f,0);
    for(const float x:{-.5f,0.f,.5f})for(const float y:{-.5f,0.f,.5f}) {
        const Vec3 footprint_point{slope_mesh[12]+x*slope_mesh[0]+y*slope_mesh[4],slope_mesh[13]+x*slope_mesh[1]+y*slope_mesh[5],slope_mesh[14]+x*slope_mesh[2]+y*slope_mesh[6]};
        check(arrow_dot(arrow_add(footprint_point,Vec3{0,0,4},-1.f),arrow_unit({0,1,-1}))>=.02f,"upright arrows clear sloped roads across their full footprint");
    }
    check(!clear_race_arrow_ground({0,1,3},0,0,0,7.5f,unavailable).available,
          "missing footprint collision data cannot authorize placement");
}
void race_editor_rotation_history() {
    for(const float pitch:{0.f,-90.f,45.f}) {
        const auto core=race_arrow_layer_transform({1,2,3},35,20,7.5f,pitch,0);
        const auto glow=race_arrow_layer_transform({1,2,3},35,20,7.5f,pitch,1);
        check(std::abs(std::hypot(glow[12]-core[12],glow[13]-core[13],glow[14]-core[14])-.015f)<.00001f,
            "Chevron glow stays a fixed small distance above its core in wall, ground and air poses");
        check(std::equal(core.begin(),core.begin()+12,glow.begin()),"glow depth separation preserves the saved orientation and scale");
    }
    RaceArrowLineSnap line;bool line_locked{};
    Settings row;row.mode=Mode::race;row.points={{0,0,0},{0,10,0},{.7f,10.4f,5}};
    row.gate_kinds={RaceGateKind::mandatory,RaceGateKind::arrow,RaceGateKind::arrow};
    auto snap=line.update(row,{{2,11,10},8,7,0},true,90,line_locked);
    check(line_locked && snap.at==Vec3{.7f,10.4f,10} && snap.yaw==0 && snap.pitch==0,
        "upright Chevron rows use authored axes rather than carrying the last two markers' diagonal error");
    for(unsigned n=0;n<12;++n) {
        const float z=15.f+5.f*n;
        snap=line.update(row,{{2,11,z},12,-8,0},true,90,line_locked);
        check(line_locked && snap.at==Vec3{.7f,10.4f,z} && snap.yaw==0 && snap.pitch==0,"each placement remains on one exact upright line with an identical pose");
        row.points.push_back(snap.at);row.gate_kinds.push_back(RaceGateKind::arrow);
    }
    snap=line.update(row,{{8,20,80},0,0,0},true,90,line_locked);
    check(!line_locked && snap.at==Vec3{8,20,80},"moving clearly away permits starting another row");
    snap=line.update(row,{{1,11,9},12,8,3},false,90,line_locked);
    check(!line_locked && snap.at==Vec3{1,11,9} && snap.yaw==12 && snap.pitch==8 && snap.roll==3,"Snap OFF releases both position and orientation");
    row.points={{0,0,0},{5,15,5}};row.gate_kinds.resize(2);line.reset();
    snap=line.update(row,{{6,22,6},0,0,0},true,90,line_locked);
    check(line_locked && snap.at==Vec3{5,22,5},"vertical columns hold both horizontal axes exactly");
    row.yaws={0,45};line.reset();
    snap=line.update(row,{{12.5f,15,-1.5f},45,0,0},true,15,line_locked);
    check(line_locked && std::abs(snap.at[0]+snap.at[2]-10.f)<.0001f,"a deliberately rotated marker provides a straight local axis without taking an accidental placement diagonal");
    row.gate_pitches={0,-90};line.reset();
    snap=line.update(row,{{5,23,5},45,0,0},true,15,line_locked);
    check(!line_locked && snap.at==Vec3{5,23,5},"a ground-mounted row does not capture upright airborne placement");
    const auto snapped_pose=snap_race_arrow_angles({{},11,7,0},true,90);
    check(snapped_pose.yaw==0 && snapped_pose.pitch==0,"default90 snapping keeps an upright preview square despite small aiming changes");
    check(default_race_editor_width/6.f==1.25f && default_race_editor_snap_angle==4,
          "new race editor defaults are 125 percent and the 90-degree option");
    check(race_camera_yaw(30,0,false)==30 && race_camera_yaw(60,15,false)==75 && race_camera_yaw(30,0,true)==-150,
          "camera following returns for gates and front-facing chevrons with retained manual yaw offsets");
    check(race_camera_pitch(20,0,true)==0 && race_camera_pitch(50,0,true)==0 &&
          race_camera_pitch(50,15,true)==15 && race_camera_pitch(50,15,false)==15,
          "airborne Chevrons stay upright at oblique camera angles while X and gates retain manual tilt");
    check(race_camera_pitch(-90,0,true,true)==-90 && race_camera_pitch(0,0,true,true)==0,
          "looking onto the floor or wall still uses the actual mounting plane");
    Settings air_row;air_row.mode=Mode::race;air_row.points={{0,0,0},{5,12,5}};
    air_row.gate_kinds={RaceGateKind::mandatory,RaceGateKind::arrow};air_row.yaws={0,0};
    line.reset();
    snap=line.update(air_row,{{6,14,12},0,0,0},true,90,line_locked,true);
    check(line_locked && snap.at==Vec3{5,12,12},"first upright marker captures a horizontal row at exactly the authored height");
    for(unsigned n=0;n<12;++n) {
        const float z=20.f+n*5.f;
        snap=line.update(air_row,{{6.2f,14.2f,z},n%2?90.f:-90.f,0,0},true,90,line_locked,true);
        check(line_locked && snap.at==Vec3{5,12,z} && snap.yaw==0 && snap.pitch==0,
              "turning the camera and small aim drift cannot bend an upright row or accumulate diagonal error");
        const auto repeated=line.update(air_row,{{6.2f,14.2f,z},n%2?90.f:-90.f,0,0},true,90,line_locked,true);
        check(repeated.at==snap.at,"two editor input passes on the same raw aim cannot accumulate snapping drift");
        air_row.points.push_back(snap.at);air_row.gate_kinds.push_back(RaceGateKind::arrow);air_row.yaws.push_back(0);
    }
    const auto last=air_row.points.back();
    const auto near_point=[](const Vec3& x,const Vec3& y){return std::hypot(x[0]-y[0],x[1]-y[1],x[2]-y[2])<.0001f;};
    snap=line.update(air_row,{{6.2f,24.2f,last[2]},0,0,0},true,90,line_locked,true);
    check(line_locked && near_point(snap.at,Vec3{last[0],last[1]+10,last[2]}),"clear upward aim switches a horizontal row to an exact vertical axis without X or toggling Snap");
    snap=line.update(air_row,{{6.2f,4.2f,last[2]},0,0,0},true,90,line_locked,true);
    check(line_locked && near_point(snap.at,Vec3{last[0],last[1]-10,last[2]}),"vertical alignment also works downward");
    snap=line.update(air_row,{{16.2f,14.2f,last[2]},0,0,0},true,90,line_locked,true);
    check(line_locked && near_point(snap.at,Vec3{last[0]+10,last[1],last[2]}),"clear sideways aim switches onto the second horizontal axis with identical height");
    snap=line.update(air_row,{{6.2f,14.2f,last[2]+10},0,0,0},true,90,line_locked,true);
    check(line_locked && near_point(snap.at,Vec3{last[0],last[1],last[2]+10}),"forward aim switches back onto the third axis rather than remaining stuck sideways");
    snap=line.update(air_row,{{18,0,80},90,-90,0},true,0,line_locked,false);
    check(snap.pitch==-90 && snap.at==Vec3{18,0,80},"aiming down onto a real floor releases the airborne row immediately");
    line.reset();line.begin_rotation(air_row.points.size());
    snap=line.update(air_row,{{25,30,80},90,0,0},true,90,line_locked,true);
    check(!line_locked && snap.at==Vec3{25,30,80} && snap.yaw==90,"X can deliberately start a different row without snapping back to an old arrow");
    air_row.points.push_back(snap.at);air_row.gate_kinds.push_back(RaceGateKind::arrow);air_row.yaws.push_back(90);
    snap=line.update(air_row,{{35,32,81},-90,0,0},true,90,line_locked,true);
    check(line_locked && std::abs(snap.at[0]-35)<.0001f && snap.at[1]==30 && std::abs(snap.at[2]-80)<.0001f && snap.yaw==90,
          "placing the manually rotated marker starts its own exact horizontal row");
    snap=line.update(air_row,{{42,42,42},-35,0,0},false,90,line_locked,true);
    check(!line_locked && snap.at==Vec3{42,42,42} && snap.yaw==-35,"Snap off always restores free placement away from the captured row");
    Settings markers;markers.mode=Mode::race;markers.points={{0,0,0},{0,10,10},{20,12,10},{5,30,10}};
    markers.gate_kinds={RaceGateKind::mandatory,RaceGateKind::arrow,RaceGateKind::arrow,RaceGateKind::mandatory};
    bool snapped{};
    check(snap_race_arrow_height(markers,{2,11,11},true,snapped)[1]==10 && snapped,"new arrow aligns exactly to the nearby arrow height");
    check(snap_race_arrow_height(markers,{19,11,11},true,snapped)[1]==12 && snapped,"closest arrow supplies the height rather than an unrelated gate");
    check(snap_race_arrow_height(markers,{2,11,11},false,snapped)[1]==11 && !snapped,"Snap OFF permits free arrow height");
    markers.widths={7.5f,40.f,40.f,7.5f};
    check(snap_race_arrow_height(markers,{2,14,11},true,snapped)[1]==14 && !snapped,
        "even very large nearby arrows cannot pull an airborne preview several metres down");
    check(snap_race_arrow_height(markers,{200,11,11},true,snapped)[1]==11 && !snapped &&
          snap_race_arrow_height(markers,{2,70,11},true,snapped)[1]==70 && !snapped,"remote or different-level arrows do not pull the preview away");
    RaceRotationVisual visual;
    visual.update({{},179,0,7.5f},1000);
    const auto smooth=visual.update({{10,0,0},-179,90,7.5f},1016);
    check(smooth.at[0]>0 && smooth.at[0]<10 && smooth.yaw>179 && smooth.pitch>0 && smooth.pitch<90,
          "rendered rings ease between updates and take the short path across the yaw seam");
    const auto settled=visual.update({{10,0,0},-179,90,7.5f},1200);
    check(std::abs(settled.yaw+179)<.1f && settled.pitch>88,"ring motion settles promptly at the exact snapped target");
    visual.reset();check(visual.update({{20,0,0},0,0,7.5f},1210).at[0]==20,"another object or tool entry has no leftover smoothing drift");
    const auto normal=race_editor_directions(false,1|8);
    check(normal.undo && !normal.redo && normal.size==1 && normal.yaw==0 && normal.tilt==0,"normal D-pad undo/size cannot rotate");
    const auto rotate=race_editor_directions(true,2|8);
    check(rotate.yaw==1 && rotate.tilt==1 && rotate.size==0 && !rotate.undo && !rotate.redo,"rotate mode uses all D-pad directions without history/size actions");
    check(race_editor_directions(false,2).redo && !race_editor_directions(false,3).undo && !race_editor_directions(true,15).tilt,"opposed directions cancel and Right redoes outside rotate");
    RaceEditorPresses presses;presses.suppress(2);
    check(presses.update(2,100)==0 && presses.update(0,110)==0 && presses.update(2,120)==2,"switching X mode requires releasing a held direction");
    check(tilt_race_gate(0,1,true,15)==15 && tilt_race_gate(90,1,true,15)==90 && tilt_race_gate(-90,-1,true,15)==-90,"tilt uses explicit rotation steps and bounded limits");
    Settings route;route.mode=Mode::race;bool finish=false;RaceRouteHistory history;
    check(edit_race_route(route,finish,RaceObject::start,{0,0,0},0,6),"history start");
    auto before=route;check(edit_race_route(route,finish,RaceObject::checkpoint,{0,0,10},0,6,0,45),"tilted Required placement");history.changed(before,route);
    const auto checkpoint=route;
    before=route;check(edit_race_route(route,finish,RaceObject::finish,{0,0,20},0,6,0,-30),"tilted Finish placement");history.changed(before,route);
    const auto completed=route;
    check(history.undo(route,finish) && route==checkpoint && !finish,"Undo removes the actual last placement and Finish flag");
    check(history.redo(route,finish) && route==completed && finish && route.gate_pitches.back()==-30,"Redo restores Finish and its full pitched pose");
    before=route;check(edit_race_route(route,finish,RaceObject::guiding,{1,0,15},10,8,0,20),"insert Guide before existing Finish");history.changed(before,route);
    check(history.undo(route,finish) && route==completed && finish,"Undo last inserted gate preserves the prior Finish");
    before=route;check(edit_race_route(route,finish,RaceObject::checkpoint,{2,0,16},30,7),"branch edit");history.changed(before,route);
    check(!history.redo(route,finish),"new placement invalidates stale redo history");
    const Vec3 prior{0,2,8},after{0,2,14};
    const auto crossing=race_gate_crossing(checkpoint,1,prior,after);
    check(crossing.inside && std::abs(crossing.at[2]-12.f)<.001f,"pitched checkpoint detection follows the same local plane as its mesh");
    check(!race_gate_crossing(checkpoint,1,{10,2,8},{10,2,14}).inside,"tilted gate still rejects a pass outside its posts");
    auto flat=checkpoint;flat.gate_pitches.clear();
    check(race_gate_crossing(flat,1,prior,after).at[2]==10.f && race_gate_along(flat,1,after)==4.f,"ordinary flat checkpoint math remains unchanged");
    Message message;message.kind=Message::Kind::setup;message.leader=a;message.game=234;message.settings=completed;
    const auto decoded=decode(encode(message));check(decoded && decoded->settings==completed,"all gate tilts round-trip through the multiplayer format");
    history.reset();check(!history.undo(route,finish) && !history.redo(route,finish),"another editor context cannot undo a different route");
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
    Settings course;course.mode=Mode::race;course.points={{0,0,0},{10,0,0},{20,0,0}};course.race_finish=true;
    Message race;race.phase=Phase::playing;race.standings={{a,1},{b,1},{c,1}};
    check(local_competition_active(course,race,a),"unfinished local racer retains the stock-tools restriction");
    race.standings[0].score=3;
    check(!local_competition_active(course,race,a) && local_competition_active(course,race,b),"finishing releases only that player's tool/binding gate while others race");
    race.standings[1].out=true;
    check(!local_competition_active(course,race,b) && local_competition_active(course,race,c),"DNF releases its local gate without releasing another racer");
    check(!local_competition_active(course,race,999),"spectators do not lose their bindings");
    check(local_quit_kind(false)==Message::Kind::leave && local_quit_kind(true)==Message::Kind::end,"guest Quit sends only Leave; only the leader can send End");
    Referee continuing(course,a,99);continuing.add_player(a);continuing.add_player(b);continuing.add_player(c);
    continuing.start(0);continuing.tick(countdown_ms);continuing.remove_player(b,countdown_ms+100);
    check(continuing.phase()==Phase::playing,"a guest leaving does not end the other racers' challenge");
    continuing.event(c,Event::checkpoint,1,course.points[1],1,countdown_ms+200);
    continuing.event(c,Event::checkpoint,2,course.points[2],2,countdown_ms+300);
    continuing.remove_player(c,countdown_ms+400);
    const auto after_leave=continuing.state(countdown_ms+500);
    const auto finisher=std::find_if(after_leave.standings.begin(),after_leave.standings.end(),[](const auto& row){return row.player==c;});
    check(continuing.phase()==Phase::playing && finisher!=after_leave.standings.end() && !finisher->out && finisher->aux==300,
        "a finished guest can leave while the host races without losing their finish time or ending the race");
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

void reskate_seven_letters_flow() {
    check(mode_name(Mode::skate)=="R.E.S.K.A.T.E." && parse_mode("R.E.S.K.A.T.E.")==Mode::skate &&
        parse_mode("skate")==Mode::skate,"new branding and existing command aliases resolve to the same mode");
    check(skate_letters(7,7)=="R.E.S.K.A.T.E" && skate_letters(99,7)=="R.E.S.K.A.T.E",
        "all seven penalty letters have the correct order and are bounded");
    Settings s;s.mode=Mode::skate;s.strikes=7;s.turn_s=10;
    Message setup;setup.kind=Message::Kind::setup;setup.leader=a;setup.game=71;setup.settings=s;
    check(decode(encode(setup))==setup,"seven-letter settings roundtrip to guests");
    Referee r(s,a,71);r.add_player(a);r.add_player(b);r.start(1000);r.tick(6000);
    std::uint64_t t=6000;
    std::uint32_t serial=0;
    // Every setter lands a new allowed trick; the copier times out. Five
    // misses must leave them playing, while the seventh eliminates them.
    const std::array tricks{"Kickflip","Heelflip","Hardflip","Varial Kickflip","Laser Flip","FS Kickflip","BS Heelflip"};
    for(unsigned i=0;i<tricks.size();++i) {
        check(r.state(t).turn==a,"setter retains the set after a failed copy");
        r.event(a,Event::trick,1,{0,0,0},++serial,t,{},tricks[i]);
        check(r.state(t).turn==b,"other player must copy the trick");
        t+=s.turn_s*1000ULL;r.tick(t);
        const auto st=r.state(t);
        const auto loser=std::find_if(st.standings.begin(),st.standings.end(),[](const auto& p){return p.player==b;});
        check(loser!=st.standings.end() && loser->aux==static_cast<int>(i+1),"one missed copy earns exactly one letter");
        check(r.phase()==(i==6?Phase::results:Phase::playing),"elimination occurs on the seventh letter, never the fifth");
    }
    check(r.state(t).standings.front().player==a,"last remaining skater wins the seven-letter match");
}

void skate_flow() {
    check(trick_kinds_of("Kickflip") == trick_flips && trick_kinds_of("BS 50-50 Grind") == trick_grinds, "flips and grinds are told apart");
    check(trick_kinds_of("Heelflip + Seatbelt") == (trick_flips | trick_grabs) && trick_kinds_of("Manual") == trick_manuals,
          "a combo is every kind its parts are");
    check(trick_kinds_of("Rocket Air") == trick_grabs && trick_kinds_of("FS Powerslide") == trick_grinds, "grabs and slides");
    check(skate_letters(3, 7) == "R.E.S" && skate_letters(0, 5).empty(), "letters spell R.E.S.K.A.T.E.");
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

void race_momentum_recovery_flow() {
    Settings navigation;navigation.mode=Mode::race;navigation.race_finish=true;
    navigation.points={{0,0,0},{0,0,10},{0,0,15},{0,0,20},{0,0,30}};
    navigation.gate_kinds={RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::arrow,RaceGateKind::mandatory,RaceGateKind::mandatory};
    Message state;state.phase=Phase::playing;Standing rider;rider.player=a;rider.score=1;state.standings={rider};
    check(race_compass_gate(navigation,state,a)==3,"compass skips Guide gates and decorative Chevrons");
    state.standings[0].score=4;check(race_compass_gate(navigation,state,a)==4,"compass targets the finish after the last Required gate");
    state.standings[0].score=5;check(!race_compass_gate(navigation,state,a),"finished riders have no compass gate");
    state.standings[0].score=3;state.standings[0].out=true;check(!race_compass_gate(navigation,state,a),"DNF riders have no compass gate");
    state.standings[0].out=false;state.phase=Phase::results;check(!race_compass_gate(navigation,state,a),"results and expired games remove navigation");
    std::array<float,16> camera{1,0,0,0,0,1,0,0,0,0,1,0,0,0,0,1};
    check(race_compass_offset({0,0,-10},camera)==0,"forward gate sits at compass center");
    check(race_compass_icon_size==96 && race_compass_max_offset+race_compass_icon_size*.5f==592,
          "larger compass gates keep their full image inside the native compass ends");
    check(race_compass_offset({10,0,0},camera)==race_compass_max_offset && race_compass_offset({-10,0,0},camera)==-race_compass_max_offset,"side gates stay within both compass ends");
    check(race_compass_offset({10,0,10},camera)==race_compass_max_offset,"a gate behind remains visible at the nearer compass end");
    for(unsigned angle=0;angle<360;angle+=5) {
        const float radians=angle*3.14159265f/180.f;
        const Vec3 near{std::sin(radians),0,-std::cos(radians)},far{near[0]*10000.f,0,near[2]*10000.f};
        const auto near_bearing=race_compass_offset(near,camera),far_bearing=race_compass_offset(far,camera);
        check(near_bearing && far_bearing && std::abs(*near_bearing-*far_bearing)<.001f && std::abs(*near_bearing)<=race_compass_max_offset,
              "all-around compass bearing is bounded and independent of gate distance");
    }
    camera[0]=0;camera[2]=-1;camera[8]=1;camera[10]=0;
    check(race_compass_offset({-10,0,0},camera)==0,"compass bearing follows camera rotation independently of skating direction");
    RaceRecoveryGrace grace;
    RaceHippyJump hippy;
    check(!hippy.observe(true,100,100,0,false,500),"riding alone is not a hippy jump");
    grace.observe(false,true,510);
    check(!hippy.observe(true,504,100,0,false,510) && hippy.observe(true,504,100,3.f,false,550),
        "riding-to-offboard becomes a hippy jump only when upward motion confirms the takeoff");
    grace.cancel_runout();check(!grace.pending(),"confirmed hippy takeoff cancels a pending false runout");
    check(hippy.observe(true,504,100,-2.f,false,900),"descending part of a hippy jump remains intentional");
    check(!hippy.observe(true,504,100,-2.f,true,910),"a real collision during a hippy jump still recovers");
    grace.observe(true,false,920);grace.cancel_runout();check(grace.pending(),"hippy filtering never cancels a full bail");
    grace.reset();
    RaceHippyJump board_loss;board_loss.observe(true,100,100,0,false,100);
    check(!board_loss.observe(true,504,100,-1.f,false,110) && !board_loss.observe(true,504,100,0,false,400),
        "accidental board loss and deliberate grounded dismount are not classified as jumps");
    check(!lobby_departure_blocks_rejoin(Phase::setup) && lobby_departure_blocks_rejoin(Phase::playing),
        "leaving before a race starts permits another explicit join without replaying an in-progress race");
    Settings draft;draft.mode=Mode::race;draft.race_finish=true;draft.points={{0,0,0},{0,0,10},{0,0,20}};
    check(!advertised_lobby_settings(draft,true).race_finish && advertised_lobby_settings(draft,false).race_finish && draft.race_finish,
        "unsaved routes withdraw their invitations, and saving publishes the finish without mutating the draft");
    check(grace.observe(true,false,1000) && grace.waiting(2999) && !grace.ready(2999) && grace.ready(3000),
        "a full bail always receives two complete seconds before recovery");
    grace.observe(true,false,1800);check(grace.ready(3000),"repeated bail samples do not extend recovery indefinitely");
    grace.reset();check(grace.observe(false,true,4000) && !grace.ready(4999) && grace.ready(5000),
        "accidental rolling board loss receives one complete second");
    grace.observe(true,false,4500);check(!grace.ready(6499) && grace.ready(6500),"runout becoming a full bail gets the full two-second bail grace");
    Settings finish_route;finish_route.mode=Mode::race;finish_route.race_finish=true;finish_route.points={{0,0,0},{0,0,10},{0,0,20}};
    grace.reset();grace.observe(true,false,7000);
    const auto finish_cross=race_gate_crossing(finish_route,2,{0,1,19},{0,1,21});
    check(grace.waiting(8999) && finish_cross.inside,"ragdoll movement can sweep through the finish during bail grace");
    if(finish_cross.inside && grace.waiting(8999))grace.reset();
    check(!grace.pending() && !grace.ready(9000),"a valid finish crossing cancels delayed recovery before it can teleport the winner");
    RagdollBailEdge fall;
    check(!fall.observe(true,false),"ordinary offboard walking/dismount is not a fall");
    check(fall.observe(true,true),"native ragdoll within offboard physics triggers a bail");
    check(!fall.observe(true,true) && !fall.observe(false,false) && !fall.observe(true,true),
        "sustained ragdoll and transient missing telemetry do not duplicate recovery");
    check(!fall.observe(true,false) && fall.observe(true,true),
        "a second fall after getting up rearms immediately, independent of the Meat scoring timer");
    RaceRunoutWatch runout;
    check(!runout.observe(false,0) && !runout.observe(true,7),
        "old free-skate runouts establish a baseline without recovering a new race");
    check(runout.observe(true,8),"a brief native failed-landing runout is retained after its animation pulse ends");
    check(!runout.observe(true,8) && !runout.observe(false,8),
        "walking, voluntary dismounts and repeated samples without a native runout cannot trigger recovery");
    check(runout.observe(false,9) && !runout.observe(true,9),
        "an already-counted runout survives a temporary missing owner and is consumed once");
    check(runout.observe(true,10),"a second failed landing triggers another return without waiting for Meat scoring");
    RaceMomentum momentum;
    momentum.observe(21.f,1000);momentum.observe(20.5f,1100);momentum.observe(0.f,1120);
    check(momentum.before_bail(1130)==21.f,"bail restores speed before collision rather than collision zero or fixed 6 m/s");
    check(momentum.current(1130)==0.f,"missed-gate recovery uses current measured motion, including a stopped rider");
    momentum.observe(18.f,1400);
    check(momentum.before_bail(1410)==18.f,"old high approach speed expires");
    momentum.observe(std::numeric_limits<float>::quiet_NaN(),1420);momentum.observe(1000.f,1430);
    check(momentum.current(1440)==18.f,"invalid telemetry cannot inject a recovery boost");
    check(momentum.before_bail(1800)==18.f && momentum.current(1800)==0.f,
        "late ragdoll confirmation retains approach motion without changing missed-gate freshness");
    check(momentum.before_bail(2200)==0.f && momentum.current(2200)==0.f,"stale samples never boost a later bail");
    RaceRecoverySpeedGuard speed_guard;
    check(speed_guard.limit(21.f,1000)==21.f,"first recovery uses measured approach speed");speed_guard.completed(1500);
    for(unsigned i=0;i<20;++i){const auto t=1600+100u*i;check(speed_guard.limit(110.f,t)<=21.f,"repeated recovery cannot amplify speed");speed_guard.completed(t);}
    check(speed_guard.limit(14.f,3700)==14.f && speed_guard.limit(90.f,3800)==14.f,"a slower rider lowers the repeat-recovery ceiling");
    check(speed_guard.limit(25.f,7000)==25.f,"clean riding after the recovery chain can retain its real new speed");
    speed_guard.reset();check(speed_guard.limit(0.f,7100)==0.f && speed_guard.limit(80.f,7200)==0.f,"stationary repeat returns cannot create speed");
    momentum.suspend_until(8000);momentum.observe(119.f,7900);check(momentum.current(7900)==0,"post-teleport measurements cannot enter momentum history");
    auto velocity=race_recovery_motion(22.f,90);
    check(std::abs(velocity[0]-22.f)<.001f && std::abs(velocity[2])<.001f && velocity[1]==0,
        "recovered motion keeps measured horizontal speed in the checkpoint's direction");
    check(valid_race_recovery_velocity(velocity) && !valid_race_recovery_velocity({121,0,0}) &&
        !valid_race_recovery_velocity({0,1,0}) && !valid_race_recovery_velocity({0,0,std::numeric_limits<float>::infinity()}),
        "native recovery accepts real racing speeds with finite horizontal-only bounds");
    RaceRecoveryDelivery delivery;delivery.request(velocity,2000);
    check(delivery.peek(2001)==velocity && delivery.peek(2500)==velocity,"a busy/off-board receiver retains recovery until accepted");
    delivery.accepted({1,0,0});check(delivery.peek(2600)==velocity,"an unrelated acknowledgement cannot consume pending recovery");
    delivery.accepted(velocity);check(!delivery.peek(2601),"success applies recovery once");
    delivery.request(velocity,3000);check(!delivery.peek(5000),"expired native restore cannot fire late");
    delivery.request(velocity,6000);delivery.reset();check(!delivery.peek(6001),"match/world cleanup cancels recovery");
}
void race_sound_and_announcements_flow() {
    RaceCountdownAudio audio;
    check(!audio.update(Phase::countdown,5000) && !audio.update(Phase::countdown,4000),"countdown does not emit unwanted five/four cues");
    check(audio.update(Phase::countdown,3000)==RaceCountdownCue::count && !audio.update(Phase::countdown,2800),"three plays once across repeated state samples");
    check(audio.update(Phase::countdown,2000)==RaceCountdownCue::count && audio.update(Phase::countdown,1000)==RaceCountdownCue::count,"two and one play once");
    check(audio.update(Phase::playing,120000)==RaceCountdownCue::go && !audio.update(Phase::playing,119500),"GO plays once at the race transition");
    audio.update(Phase::setup,0);
    check(audio.update(Phase::countdown,2500)==RaceCountdownCue::count && audio.update(Phase::countdown,900)==RaceCountdownCue::count &&
        !audio.update(Phase::countdown,2000),"network frame skips never replay missed countdown cues or run them backwards");
    RaceCountdownAudio late;check(!late.update(Phase::playing,100000),"joining/spectating mid-race does not replay GO");
    Settings route;route.mode=Mode::race;route.duration_s=120;route.points={{0,0,0},{0,0,10},{0,0,20},{0,0,30}};
    route.gate_kinds={RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::mandatory,RaceGateKind::mandatory};route.race_finish=true;
    check(race_gate_cue(route,1,2)==RaceGateCue::none,"Guide gate progress is always silent");
    check(race_gate_cue(route,2,3)==RaceGateCue::required && race_gate_cue(route,3,4)==RaceGateCue::finish,"Required and finish gates select separate user-chosen clips");
    check(race_gate_cue(route,3,3)==RaceGateCue::none && race_gate_cue(route,3,2)==RaceGateCue::none,"duplicate/backward state cannot replay gate sounds");
    // Drive the actual referee through a solo finish: its next tick is already
    // Results, so an observer restricted to Playing would lose the sound.
    Referee solo(route,a,19);solo.add_player(a);solo.start(0);solo.tick(countdown_ms);
    RaceProgressAudio progress;
    auto snapshot=solo.state(countdown_ms);
    check(progress.update(route,snapshot.phase,snapshot.standings[0].score,false)==RaceGateCue::none,"race start is silent");
    solo.event(a,Event::checkpoint,2,route.points[2],1,countdown_ms+100);
    snapshot=solo.state(countdown_ms+100);
    check(progress.update(route,snapshot.phase,snapshot.standings[0].score,false)==RaceGateCue::required,"acknowledged Required gate plays its selected clip");
    solo.event(a,Event::checkpoint,3,route.points[3],2,countdown_ms+200);solo.tick(countdown_ms+201);
    snapshot=solo.state(countdown_ms+201);
    check(snapshot.phase==Phase::results && progress.update(route,snapshot.phase,snapshot.standings[0].score,false)==RaceGateCue::finish,
        "solo finish celebration survives the Playing-to-Results transition");
    check(progress.update(route,snapshot.phase,snapshot.standings[0].score,false)==RaceGateCue::none,"repeated results never replay the finish");
    RaceProgressAudio dnf;
    check(dnf.update(route,Phase::results,2,true)==RaceGateCue::none,"DNF never plays a finish celebration");
    MatchAnnouncements notices;Message state;state.phase=Phase::setup;
    check(!notices.observe("Host",route,state,1000),"setup and invitation produce no server spam");
    state.phase=Phase::countdown;state.remaining_ms=5000;
    check(notices.observe("Host",route,state,1000)=="Host has started Deathrace (02:00.000).","start notification contains only the host, challenge and time");
    check(!notices.observe("Host",route,state,1500),"retransmitted countdown cannot duplicate the start notification");
    state.phase=Phase::playing;check(!notices.observe("Host",route,state,6000),"countdown-to-play does not announce start twice");
    state.phase=Phase::results;state.standings={{.player=a,.score=4,.aux=14468}};
    check(notices.observe("Host",route,state,21000)=="Host has ended Deathrace (00:14.468).","end notification uses the precise finishing time");
    check(!notices.observe("Host",route,state,21500),"results and subsequent End cannot duplicate the end notification");
    MatchAnnouncements cancelled;check(!cancelled.observe("Host",route,state,22000),"cancelling an unstarted setup produces no misleading ended notice");
    Settings reskate;reskate.mode=Mode::skate;reskate.turn_s=45;reskate.strikes=7;
    MatchAnnouncements turn_game;state.phase=Phase::playing;
    check(turn_game.observe("Host",reskate,state,1000)=="Host has started R.E.S.K.A.T.E. (00:45.000).",
        "turn-based games announce their real turn timer instead of an unused match limit");
    state.phase=Phase::results;
    check(turn_game.observe("Host",reskate,state,391123)=="Host has ended R.E.S.K.A.T.E. (06:30.123).",
        "a long seven-letter match reports its full elapsed time without a false timed-mode cap");
}

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
    RaceEditorPresses presses;
    check(presses.update(8,100)==8 && presses.update(8,449)==0,"tap responds immediately with a deliberate hold delay");
    check(presses.update(8,450)==8 && presses.update(8,599)==0 && presses.update(8,600)==8,"held D-pad repeats at controlled 150 ms intervals");
    check(presses.update(8,5000)==8 && presses.update(8,5001)==0,"frame stall never produces a catch-up burst");
    presses.update(0,5002);check(presses.update(8,5003)==8,"release rearms immediate size tap");
    presses.suppress();check(presses.update(8,6000)==0 && presses.update(8,9000)==0,"focus loss blocks held D-pad until release");
    presses.update(0,9001);check(presses.update(8,9002)==8,"focus reset rearms only after release");
    presses.update(0,9003);check(presses.update(3,9004)==3,"opposing direction steps cancel symmetrically");
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
void race_floating_arrow_flow() {
    Settings route;route.mode=Mode::race;bool finish=false;
    check(edit_race_route(route,finish,RaceObject::start,{0,0,0},0,6),"arrow course start");
    check(edit_race_route(route,finish,RaceObject::arrow,{50,30,0},180,6,-90),"arrow places in air before any Required gate");
    check(!edit_race_route(route,finish,RaceObject::finish,{0,0,50},0,6),"decorative arrows alone cannot make a race");
    check(edit_race_route(route,finish,RaceObject::checkpoint,{0,0,20},0,6),"Required gate follows arrow");
    check(edit_race_route(route,finish,RaceObject::finish,{0,0,50},0,6),"arrow course finish");
    check(race_start_heading(route)==0 && next_race_gate(route,1)==2 && mandatory_gates(route,route.points.size())==1,
          "floating arrow never redirects spawn or counts toward progress");
    check(!race_gate_missed(route,1,{100,30,-5},{100,30,5}),"missing an arrow cannot penalize a racer");
    check(race_recovery_pose(route,2).at==Vec3{0,0,1.5f},"recovery skips a decorative arrow");
    check(edit_race_route(route,finish,RaceObject::arrow,{0,35,25},-45,9,30,-60) && route.gate_pitches[3]==-60,"aimed pitch is stored before the existing finish");
    Message pitched;pitched.kind=Message::Kind::setup;pitched.game=15;pitched.leader=1;pitched.settings=route;
    check(decode(encode(pitched))==pitched,"camera-aimed yaw, pitch, roll and size reach guests unchanged");
    auto previous=encode(pitched);previous[1]=14;check(!decode(previous),"previous peers cannot misread the new pitch field");
    auto bad_pitch=pitched;bad_pitch.settings.gate_pitches[3]=91;bool pitch_rejected=false;
    try{(void)encode(bad_pitch);}catch(const std::invalid_argument&){pitch_rejected=true;}check(pitch_rejected,"out-of-range pitch is rejected");
    check(undo_race_gate(route,finish),"remove finish to exercise incomplete-route warning");
    check(race_route_save_error(route,finish)=="You need a Finish line before saving this route.","missing finish returns the explicit user-facing save reason");
    check(edit_race_route(route,finish,RaceObject::finish,{0,0,50},0,6) && race_route_save_error(route,finish).empty(),"placing Finish clears the save error");
    const auto poses=route.arrow_rolls;
    check(edit_race_route(route,finish,RaceObject::arrow,{0,40,30},90,12,45) && route.points.back()==Vec3{0,0,50} &&
          route.arrow_rolls[4]==45 && route.arrow_rolls.back()==0 && arrow_marker(route,4) && race_route_ready(route,finish),
          "arrow insertion keeps the sole finish and all pose arrays aligned");
    Message m;m.kind=Message::Kind::setup;m.game=17;m.leader=1;m.settings=route;
    check(decode(encode(m))==m,"floating arrow height, direction, rotation and size reach guests");
    auto old=encode(m);old[1]=13;check(!decode(old),"old peers cannot interpret new arrow records");
    auto invalid=m;invalid.settings.arrow_rolls.pop_back();
    bool rejected=false;try{(void)encode(invalid);}catch(const std::invalid_argument&){rejected=true;}
    check(rejected,"misaligned remote arrow rotations rejected");
    RaceCourseMemory memory;memory.save("air",route);check(memory.restore("air")==route,"map course retains floating arrows");
    const auto matrix=race_arrow_transform({3,40,5},0,90,6);
    check(std::abs(matrix[0])<.0001f && std::abs(matrix[1]-4.f)<.0001f && matrix[13]==40,
          "arrow rotation changes its plane orientation without grounding it");
    const auto aimed=race_arrow_transform({3,40,5},0,0,6,90);
    check(std::abs(aimed[6]-4.f)<.0001f && std::abs(aimed[9]+4.f)<.0001f && aimed[13]==40,"camera tilt rotates the marker into a horizontal plane without moving it");
    const auto combined=race_arrow_transform({3,40,5},45,35,6,-60);
    for(unsigned axis=0;axis<3;++axis){float norm{};for(unsigned component=0;component<3;++component)norm+=combined[axis*4+component]*combined[axis*4+component];check(std::abs(norm-16.f)<.001f,"aim and roll preserve marker size on every axis");}
    check(undo_race_gate(route,finish) && route.arrow_rolls.size()==route.points.size() && route.gate_pitches.size()==route.points.size(),"undo preserves arrow pose alignment");
    clear_race_gates(route,finish);check(route.arrow_rolls.size()==1 && route.gate_pitches.size()==1 && route.points.size()==1,"clear removes floating arrows too");
}
void race_checkpoint_back_flow() {
    Settings route;route.mode=Mode::race;route.race_finish=true;
    route.points={{0,0,0},{0,0,10},{1,20,15},{0,0,20},{0,0,25},{0,0,30},{0,0,40}};
    route.yaws.resize(7);route.widths.resize(7,6);
    route.gate_kinds={RaceGateKind::mandatory,RaceGateKind::mandatory,RaceGateKind::arrow,
        RaceGateKind::mandatory,RaceGateKind::guiding,RaceGateKind::mandatory,RaceGateKind::mandatory};
    RaceCheckpointFallback back;
    check(back.target(route,6)==5,"normal recovery retains the last cleared Required gate");
    check(back.step_back(route,6)==3,"first Up goes one checkpoint before the unsafe last gate");
    check(back.target(route,6)==3 && back.target(route,6)==3,"another bail retains the selected safe fallback");
    back.observe_crossings(route,6,{0,0,31},{0,0,29});check(back.target(route,6)==3,"crossing a cleared gate backward cannot advance recovery");
    back.observe_crossings(route,6,{1,20,14},{1,20,16});check(back.target(route,6)==3,"decorative arrows do not advance rewind recovery");
    back.observe_crossings(route,6,{0,0,29},{0,0,31});check(back.target(route,6)==5,"re-crossing an already-cleared Required gate advances local recovery");
    check(race_gate_missed(route,6,{30,0,39},{30,0,41}) && back.target(route,6)==5,"a later miss returns to the newly re-crossed gate, not the stale manual target");
    check(back.step_back(route,6)==3,"manual rewind still steps back from the updated gate");
    check(back.step_back(route,6)==1 && back.step_back(route,6)==0 && back.step_back(route,6)==0,
          "each Up steps farther back through Required gates to start, skipping guides/arrows");
    back.advanced();check(back.target(route,6)==5,"clearing a new Required gate restores normal recovery");
    check(back.step_back(route,1)==0 && back.target(route,1)==0,"before the first gate Up returns to start");
    RaceCheckpointBackPress press;
    check(!press.update(true,true),"held menu press cannot rewind on race entry");
    check(!press.update(false,true) && press.update(true,true) && !press.update(true,true),"one physical Up press steps once, with no hold-repeat");
    press.update(true,false);check(!press.update(true,true),"menu/focus loss cannot manufacture a new Up press");
    press.update(false,false);check(!press.update(true,false) && !press.update(true,true),"Up used in a menu cannot rewind on returning");
    press.update(false,true);check(press.update(true,true),"release rearms the next deliberate Up press");
    press.update(false,false);check(!press.update(true,true),"unknown controller state while unfocused cannot trigger on returning with Up held");
    press.update(false,true);check(press.update(true,true),"a fresh release and press after focus loss works");
}
int main() {
    try {
        area_flow();
        lobby_enrollment_flow();
        placement_camera_flow();
        scoring_flow();
        wire_flow();
        jam_flow();
        one_up_flow();
        race_flow();
        race_builder_flow();
        race_floating_arrow_flow();
        race_checkpoint_back_flow();
        race_saved_course_and_recovery();
        race_momentum_recovery_flow();
        race_sound_and_announcements_flow();
        race_clock_flow();
        race_editor_rotation_history();
        race_arrow_surface_placement();
        race_custom_deadlines_and_snap();
        race_optional_flow();
        race_stock_policy_flow();
        gate_width_flow();
        skate_tag_flow();
        domination_flow();
        tag_flow();
        graffiti_flow();
        skate_flow();
        reskate_seven_letters_flow();
        infection_flow();
        hide_flow();
        std::cout << "Game modes: area, scoring, wire, Spot Jam, 1-Up, Deathrace, Domination, tag, Graffiti and S.K.A.T.E. flows passed.\n";
        return 0;
    } catch (const std::exception &e) {
        std::cerr << e.what() << '\n';
        return 1;
    }
}
