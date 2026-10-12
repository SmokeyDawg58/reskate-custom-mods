// The game modes' rules and wire format without the game: referees are driven by hand with
// the events players would send.
#include "Extension/Modes/mode_rules.h"
#include "Extension/Modes/race_route.h"
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
    check(accept_race_flag(s,finish,{2,1,3},45,4)==RaceFlagTransition::wait_for_start,
        "saved-route native Confirm places a flag and waits for host Start");
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
    while(s.points.size()<max_points-1)check(edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"multiple normal gates accepted");
    check(!edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"builder reserves a slot for finish");
    check(edit_race_route(s,finish,RaceObject::finish,end,90,6) && s.points.size()==max_points,"finish fits the last slot");
    check(!edit_race_route(s,finish,RaceObject::checkpoint,gate,90,6),"full route remains bounded");
    check(cycle_race_object(RaceObject::start,false)==RaceObject::checkpoint && cycle_race_object(RaceObject::start,true)==RaceObject::checkpoint &&
        cycle_race_object(RaceObject::finish,false)==RaceObject::checkpoint && cycle_race_object(RaceObject::checkpoint,true)==RaceObject::finish,
        "editor cycles checkpoint and finish, never the already placed start flag");
}
void race_clock_flow() {
    Settings s; s.mode=Mode::race; s.duration_s=30;
    s.points={{0,0,0},{20,0,0},{40,0,0},{60,0,0}};
    Referee r(s,a,3);r.add_player(a);r.add_player(b);r.start(0);r.tick(countdown_ms);
    const auto of=[&](std::uint64_t id,std::uint64_t t) {
        const auto m=r.state(t);
        return *std::find_if(m.standings.begin(),m.standings.end(),[&](const auto& p){return p.player==id;});
    };
    check(of(a,countdown_ms).score==1 && of(b,countdown_ms).score==1,"all racers begin at checkpoint one without crossing the flag");
    r.event(a,Event::checkpoint,0,{0,0,0},1,countdown_ms+100);
    check(of(a,countdown_ms+100).remaining_ms==29900,"start flag awards no bonus");
    r.event(a,Event::checkpoint,1,{20,0,0},2,countdown_ms+1000);
    check(of(a,countdown_ms+1000).remaining_ms==31000,"checkpoint adds exactly two seconds");
    check(of(b,countdown_ms+1000).remaining_ms==29000,"another racer's clock is independent");
    r.event(a,Event::checkpoint,1,{20,0,0},3,countdown_ms+1000);
    check(of(a,countdown_ms+1000).remaining_ms==31000,"crossing the same checkpoint again awards nothing");
    r.tick(countdown_ms+30000);
    check(of(b,countdown_ms+30000).out && !of(a,countdown_ms+30000).out,"only expired racers time out");
    r.event(b,Event::checkpoint,0,{0,0,0},1,countdown_ms+30001);
    check(of(b,countdown_ms+30001).score==1,"expired racers cannot revive their clock");
    r.event(a,Event::checkpoint,2,{40,0,0},4,countdown_ms+31000);
    check(of(a,countdown_ms+31000).remaining_ms==3000,"second checkpoint adds its own bonus");
    check(decode(encode(r.state(countdown_ms+31000)))==r.state(countdown_ms+31000),"per-player race clocks round trip");
    r.event(a,Event::checkpoint,3,{60,0,0},5,countdown_ms+32000);
    r.tick(countdown_ms+32000);
    check(r.phase()==Phase::results && r.state(countdown_ms+32000).standings.front().player==a,"finish ranks ahead of DNF");
    Referee late(s,a,4);late.add_player(a);late.start(0);late.tick(countdown_ms);
    late.event(a,Event::checkpoint,0,{0,0,0},1,countdown_ms+30000);
    late.tick(countdown_ms+30000);
    check(late.phase()==Phase::results && late.state(countdown_ms+30000).standings.front().out,"exact deadline cannot accept a late crossing");
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
    check(!race_gate_crossing(s,1,{30,20,0},{70,20,0}).inside,"movement above the gate does not count");
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
        race_clock_flow();
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
