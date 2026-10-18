#pragma once
#include <array>
#include <cstdint>
#include <string>
#include <vector>
namespace dingosdk::overlay {
bool interface_open() noexcept;
void hide_game_buttons(std::uint16_t buttons) noexcept;
void hold_game_buttons(std::uint16_t held) noexcept;
void pause_game_input(bool paused) noexcept;
bool key_down(int virtual_key) noexcept;
int take_mouse_wheel() noexcept;
struct ModesHudRow {
    std::string name, value;
    std::uint32_t color{0xffffffffU}; // IM_COL32
    bool self{}, up{}, out{};
};
// A Graffiti tag: the path a board slid along (a grind) or a gap's takeoff and landing, in the
// colour of whoever holds it.
struct ModesHudTag {
    std::vector<std::array<float, 3>> path;
    bool gap{};
    std::uint32_t color{};
};
// Another player's game the local player is not in, shown like a throwdown drop: a banner over
// its spot (the start gate, the area's centre or where it was set up) and a row in the menu.
struct ModesHudOffer {
    std::string mode, host, detail;  // "HALL OF MEAT", "Huntredbanzzz", "3 players - join now"
    std::array<float, 3> at{};
    bool has_at{};
    bool open{};   // still taking players (setting up or counting down)
    bool target{}; // the one the join button acts on now
    bool own{};    // the local player's own game: its flag only, where the others will join
    std::uint64_t id{}; // `mode join <id>`
};
// Skate Tag: a player where they last said they were. The one who is it wears a neon crown; arrows at
// the screen's edge point to whoever matters off screen (it, or everyone else for the one who is).
struct ModesHudPlayer {
    std::string name;
    std::array<float, 3> at{};
    std::uint32_t color{}; // IM_COL32
    bool it{}, self{};
};
// S.K.A.T.E.: one part of the trick to copy, with its flick-it stick path when it has one (x to the
// left, y pulled back, the gate the unit circle, a regular stance), else what kind of trick it is.
struct ModesHudTrickPart {
    std::string name;                         // "Kickflip", "BS 50-50 Grind"
    std::vector<std::array<float, 2>> path;   // empty: no flick (a grab, grind or manual)
    std::string kind;                         // "GRAB", "GRIND", "MANUAL" when there is no path
};
struct ModesHud {
    bool active{};        // in a game (the panel, the area and the rest); offers draw either way
    std::string trick_title;                  // S.K.A.T.E.: "COPY THIS", over the trick's diagram
    std::string trick_steps;                  // R.E.S.K.A.T.E.: how to do the trick to copy (Skatepedia markup)
    std::vector<std::string> done_tricks;     // R.E.S.K.A.T.E.: set already this game, newest last
    std::string callout;                      // R.E.S.K.A.T.E.: "TRICK SET!", "SMOKEY  R.E.S", "SMOKEY IS OUT"
    std::uint64_t callout_serial{};           // changes with each new callout
    std::vector<ModesHudTrickPart> trick_parts;
    bool goofy{};                             // mirror the flicks for a goofy stance
    std::vector<ModesHudPlayer> players; // Skate Tag and Infection, while they are played
    bool infection{};                     // the players' `it` are the infected (the reapers), not one tagger
    // Hide & Seek: `players` holds only the seekers (hiders are never shown); a blind seeker's screen
    // is black with `blind` (the seconds left) on it; `heat` 0 (cold) .. 1 (right there) is how close
    // the nearest hider is for a seeker, or the nearest seeker for a hider (< 0: no meter).
    bool hide{};
    std::string blind;
    float heat = -1;
    std::string heat_label;               // "FREEZING" .. "ON FIRE"
    bool heat_seeking{};
    std::vector<ModesHudOffer> offers;
    std::string invite;   // "Huntredbanzzz is starting Hall of Meat", shown for a while after it appears
    float invite_fade{};  // 0..1
    bool can_join{};      // a join button press would join `target` now
    std::string title;    // "Graffiti"
    std::string clock;    // "2:31", the countdown's "3", or empty
    std::string status;   // what to do now
    std::string line;     // the local player's line in progress
    std::string banner;   // the latest callout (big, for a moment)
    std::uint64_t banner_serial{};
    std::string warning;  // out of the area
    // The results at the end: the winner, their score and colour, and how long until it closes.
    bool results{};
    std::string winner, winner_value;
    std::uint32_t winner_color{0xffffffffU};
    std::uint32_t closing_ms{};
    std::vector<ModesHudRow> rows;
    std::vector<std::array<float, 3>> corners; // the area, in order around it (a circle's centre when area_radius > 0)
    float area_radius{};
    bool placing{};                            // the area or points are being placed: drawn as a preview
    bool aiming{}, aim_ok{};                   // placing from the free camera: a reticle; aim_ok when it finds ground
    std::array<float, 3> cursor{};             // placing: where the next thing goes
    std::string hint;                          // what to do while placing ("Skate to the centre.")
    // The controls while placing, as button prompts: a D-pad direction ('U', 'D', 'L', 'R', or 0
    // for none), the keyboard key and what it does.
    struct Prompt {
        char dpad{};
        std::string key, label;
    };
    std::vector<Prompt> prompts;
    std::array<float, 3> me{};                 // the local skater, for the walls fading in near them
    bool have_me{};
    std::vector<std::array<float, 3>> points;  // checkpoints or spots
    std::vector<std::uint32_t> point_colors;   // one per point (0: plain)
    int next_point{-1};                        // Deathrace: the local player's next checkpoint
    bool route{};                              // the points are a Deathrace route: start, checkpoints, finish
    std::vector<float> point_yaws;             // each gate's facing, degrees (the trainer's heading); empty: along the route
    std::vector<float> point_widths;           // each gate's half width in metres; empty: `radius`
    float radius{};
    std::vector<ModesHudTag> tags;
    std::array<float, 16> camera{}; // world matrix: right, up, back, position rows
    float vertical_fov{};
};
using ModesHudFeed = ModesHud (*)();
// The Bone Cam (Extension/Modes/bone_cam.h): during a big bail the screen goes X-ray and every
// bone of the local skater is drawn between its real joints, the ones that took a hard hit in red,
// with a Skate 2 style injury list. Inactive = nothing to draw.
struct BoneCamBone {
    std::uint8_t sprite{};         // modes::BoneSprite
    std::array<float, 3> a{}, b{}; // world: the bone's first joint, and a point along it
    std::uint8_t hurt{};           // 0 sound, 1 cracked, 2 broken
};
struct BoneCamInjury {
    std::string bone, what; // "LEFT FEMUR", "FRACTURED"
    bool severe{};
};
// What a slam leaves behind: marks on the ground where the body slid (a skid for the torso, thin
// scrapes for hands, knees and head), and the hit itself.
struct BoneCamMark {
    std::vector<std::array<float, 3>> path; // world, along the ground
    float alpha{};                          // 0..1, fading with age
    bool skid{};
};
// The skater's pose for the 3D X-ray (bone_cam_3d.cpp), in world space. Sides are [0] right,
// [1] left. `valid` once a pose has been read this bail.
struct BoneCamPose {
    using Point = std::array<float, 3>;
    bool valid{};
    std::vector<Point> spine; // the joints from the hips up to the head, in order
    Point head{}, head_up{}; // the head joint and its up axis (unit)
    std::array<Point, 2> clavicle{}, shoulder{}, elbow{}, wrist{}, fingers{}, hip{}, knee{}, ankle{}, toe{};
    // Injuries by region (modes::BoneSprite order): 0 sound, 1 cracked, 2 broken.
    std::vector<std::uint8_t> hurt;
};
struct BoneCam {
    bool active{};         // the X-ray itself
    BoneCamPose pose;
    bool effects{};        // marks or a hit to draw (with or without the X-ray)
    float fade{};          // 0..1: fades in when the bail starts, out at the end
    std::vector<BoneCamMark> marks;
    float hit{};                    // 0..1: the flash of an impact, fading fast
    float daze{};                   // 0..1: a concussion's after-effects (kept faint), fading
    std::array<float, 3> hit_at{};  // world: where it landed
    bool preview{};        // the menu's preview: the injuries are examples
    std::uint64_t serial{}; // one per bail
    std::vector<BoneCamBone> bones;
    std::vector<BoneCamInjury> injuries;
    std::array<float, 3> left{}; // world: from the skater's right shoulder toward the left one (which way the art faces)
    std::array<float, 16> camera{}; // world matrix: right, up, back, position rows
    float vertical_fov{};
};
using BoneCamFeed = BoneCam (*)();
// One row of the lobby leaderboard (Game Modes page), best first.
struct ModesLeaderRow {
    std::string name;
    int points{}, wins{}, podiums{}, games{};
    int streak{}, best_streak{}; // wins in a row: now, and the most this session
    std::string best_mode;       // the mode they win most ("Skate Tag"), or empty without a win
    bool you{};
};
// A finished game on the lobby leaderboard, newest first.
struct ModesLeaderGame {
    std::string mode, winner, second;
    int players{};
    std::uint32_t ago_s{};
};
// ModesHud's menu needs (the Game Modes page of the ReSkate menu): what the local player can do.
struct ModesMenu {
    bool in_game{}, leading{};
    int phase{};                  // modes::Phase, 0 when no game
    std::string mode;             // "graffiti"
    std::size_t corners{}, points{}, players{};
    std::uint32_t duration{}, turn{};
    int strikes{};
    float radius{};
    std::string missing;          // what `mode start` still needs
    std::string bone_cam;         // "meat", "on" or "off"
    bool bone_cam_ringing = true; // a concussion's ringing is on (not muted)
    // ReSkate's own Hall of Meat (Extension/HallOfMeat) runs on this game build: it replaces the Bone
    // Cam, and `meat_every_bail` is its switch (on: every bail in free skate too).
    bool official_meat{}, meat_every_bail{};
    std::uint8_t trick_kinds{0x0f}; // S.K.A.T.E.: the kinds of trick that may be set (1 flips, 2 grabs, 4 grinds, 8 manuals)
    bool spectate = true;           // S.K.A.T.E.: the camera follows whoever is up
    bool goofy{};                   // S.K.A.T.E.: flick diagrams for a goofy stance
    // The other players in the session: who has game modes (this version), who has another one
    // and has to update, and how many have none (they cannot see or join a game).
    bool in_session{};
    std::vector<std::string> lobby_modded, lobby_outdated;
    int lobby_without{};
    // The lobby leaderboard: every game of two or more finished in this session.
    std::vector<ModesLeaderRow> leaders;
    std::vector<ModesLeaderGame> recent;
    int lobby_games{};
    std::string lobby_top_mode; // the most played mode
    float area_radius{};          // > 0: the area is a circle
    std::string placing;          // "circle", "corners", "points" while placing on the skater; else empty
    std::vector<ModesHudOffer> offers; // other players' games, to join from the menu
};
using ModesMenuFeed = ModesMenu (*)();
}
