#pragma once
#include <utility>
#include "Extension/UI/Overlay/overlay.h"
#include "mode_view.h"
#include "mode_rules.h"
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

// ReSkate's game modes in the running game (rules: mode_rules.h). The game thread drives
// everything: the session tick hands over who is here and what arrived, and the console's
// `mode` command sets a game up. The local player's lines and bails come from what the
// Trainer measures each tick; the HUD reads a snapshot from any thread.
namespace dingosdk::modes {
struct SessionPlayer {
    std::uint64_t id{}; // Steam ID
    std::string name;
};
struct SessionInput {
    std::uint64_t local{};    // this player's Steam ID; 0 without a session (then playing alone)
    std::string local_name;
    std::vector<SessionPlayer> peers; // the other players in this world (never a dedicated server)
    bool in_world{};          // the world is loaded and playing
    std::uint64_t world{};    // changes whenever the map or world changes
    bool barred{};            // the local player's mods change scoring: they cannot play
};
// Session tick (game thread): runs the local game and returns the messages to send to
// every other player (dropped without a session).
std::vector<std::vector<std::uint8_t>> tick(const SessionInput &input);
// One game mode message from another player (game thread). False when the bytes are not one.
bool receive(std::uint64_t sender, std::span<const std::uint8_t> message);
// Lines for the local chat since the last call (game thread).
std::vector<std::string> take_notices();
// Infection: whether this other player is infected now, so their skater wears the reaper here
// (game thread, from the session's outfit pass).
bool infected_look(std::uint64_t player) noexcept;
// Hide & Seek: whether this other player is hiding now, so their nametag, dot and chat bubbles
// stay off (game thread).
bool hidden_player(std::uint64_t player) noexcept;
// `mode <verb> [arguments]` from the console dispatcher (game thread).
// setup opens the native Throwdown creation page for any custom mode draft;
// quit leaves immediately, while stop keeps the leader's results presentation.
std::string command(std::string_view verb, const std::vector<std::string> &arguments);
// The HUD's snapshot, with the latest camera (any thread).
overlay::ModesHud hud();
// While ReSkate's own native menus and HUD write their labels (on this thread), the trick reader
// ignores every UI write: those are not skate.'s trick list.
inline thread_local bool writing_own_ui = false;
struct OwnUiWrites {
    bool previous = std::exchange(writing_own_ui, true);
    ~OwnUiWrites() { writing_own_ui = previous; }
};
// `mode flicktest on|off`: R.E.S.K.A.T.E.'s picture slot shows the 1UP badge instead (diagnostics).
bool flick_test() noexcept;
// What the ReSkate menu's Game Modes page shows (any thread).
overlay::ModesMenu menu_view();
struct SavedRaceRouteSummary {
    std::uint64_t id{};std::string name,map;unsigned required{},objects{};
};
struct NativeView {
    bool active{}, leading{}, placing{};
    bool pending_flag{}, flag_placed{}, waiting{};
    std::uint64_t local{}, leader{}, world{};
    std::uint64_t state_sampled_ms{};
    std::uint32_t game{};
    unsigned capacity=6;
    bool route_finish{},cursor_valid{};
    unsigned route_object=1;
    bool route_snap=true,exit_confirm{},recovering{},rotating{},arrow_height_snapped{};
    float snap_degrees=15.f,clear_progress{},save_progress{};
    unsigned route_dialog{};std::uint64_t route_dialog_serial{};
    std::string route_dialog_error,route_map,route_name;std::vector<SavedRaceRouteSummary> saved_routes;
    unsigned missed_gate{};
    Vec3 cursor{};
    float cursor_yaw{},cursor_width=6.f,cursor_roll{},cursor_pitch{};
    bool editor_roll_axis{},route_saved{};unsigned gates_visible_from{};std::uint64_t object_changed_ms{};
    bool placing_circle{}; float draft_radius{}; // an area being placed: its size so far
    bool placing_corners{}, zone_custom{};          // R.E.S.K.A.T.E.: its cones being placed; a custom zone
    Settings settings;
    std::optional<Message> state;
    std::string waiting_status,editor_warning;
    std::vector<std::pair<std::uint64_t,std::string>> names;
};
NativeView native_view();
bool request_race_route_action(std::uint64_t world,std::uint32_t game,std::string action,std::string argument);
struct NativeFlagOffer {
    std::uint64_t leader{};
    std::uint32_t game{};
    std::string key;
    Vec3 at{};
    float yaw{};
    unsigned players=1,duration_s=120,required_gates{};
    std::uint64_t native_match{}; // Full 1-UP identity; mode IDs remain 32 bit.
    bool operator==(const NativeFlagOffer&) const=default;
    bool same_registration(const NativeFlagOffer& other) const {
        return leader==other.leader && game==other.game && native_match==other.native_match && key==other.key && at==other.at && yaw==other.yaw;
    }
};
// Game-thread bridge: the existing mode invitation owns the race; a native
// registration supplies its physical flag, POI and stock Join interaction.
std::vector<NativeFlagOffer> native_flag_offers(std::uint64_t world);
bool join_native_flag_offer(const NativeFlagOffer& offer,std::uint32_t mmid);
void accept_race_start(std::uint64_t world, std::uint32_t game, const Vec3& at, float yaw);
// The native flag callback only registers a matching draft's spawn. Starting
// stays an explicit waiting-HUD hold action, dispatched on the client tick.
void accept_native_start_flag(std::uint64_t world, std::uint32_t game, const Vec3& at, float yaw);
bool request_native_start(std::uint64_t world, std::uint32_t game) noexcept;
bool request_native_quit(std::uint64_t world, std::uint32_t game) noexcept;
bool active() noexcept;
std::optional<Vec3> peek_race_recovery_velocity() noexcept;
void accept_race_recovery_velocity(const Vec3&) noexcept;
bool internal_teleport() noexcept;
bool countdown_locks_input() noexcept;
bool race_recovery_pending() noexcept;
bool race_countdown_holds() noexcept;
bool placement_camera_requested() noexcept;
bool restricts_session_markers() noexcept;
} // namespace dingosdk::modes
