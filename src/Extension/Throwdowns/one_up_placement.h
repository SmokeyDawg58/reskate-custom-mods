#pragma once
#include <array>
#include <cstdint>
#include <string>

namespace dingosdk::multiplayer::menu_data { class Context; }
namespace dingosdk::multiplayer::one_up {
bool native_race_flag_cursor_active() noexcept;
// Only owns a native Throwdown created by this request. Ordinary Spot Battle
// placement, navigation and network events never enter this adapter.
bool begin_flag_placement(unsigned seconds, unsigned players, bool show_native_setup=false);
// Private race setup uses the same verified Spot Battle placement adapter,
// but hands its pose to the mode pack rather than creating a 1-Up match.
bool begin_race_flag_placement(std::uint64_t world, std::uint32_t game, unsigned players, bool show_native_setup=false);
// Generic modes use the native settings -> starting flag -> waiting HUD flow.
// Their accepted pose belongs to the mode coordinator, never a 1-Up match.
bool begin_native_mode_setup(std::uint64_t world, std::uint32_t game, unsigned players);
bool race_flag_registration_owned() noexcept;
bool mode_flag_registration_owned(std::uint32_t game=0) noexcept;
std::uint32_t native_flag_mode_game() noexcept;
bool native_flag_mode_waiting() noexcept;
// Move this editor's existing native registration to its saved pose. No new
// donor queue, setup screen or second placement interaction is created.
bool restore_race_flag(std::uint64_t world,std::uint32_t game,const std::array<float,3>& at,float yaw) noexcept;
bool race_flag_move_pending(std::uint32_t mmid,const std::array<float,3>& at,float yaw) noexcept;
bool restored_race_flag_spawned(std::uint64_t token,std::uint32_t mmid) noexcept;
std::uint32_t native_setup_race_game() noexcept;
std::uint32_t native_setup_mode_game() noexcept;
// Close an unconfirmed mode setup before an explicit extra settings action. Never
// cancel another mode's setup or a flag placement that is already underway.
bool close_race_setup_for_start(std::uint32_t game) noexcept;
// Reserve the native setup navigation for this card. The tick publishes it
// only after previous flag cleanup; a rejected click cannot open Spot Battle.
bool arm_native_setup(unsigned seconds, unsigned players);
bool native_setup_active() noexcept;
// A verified grid selection supersedes a queued, open or cancelled unconfirmed
// setup. No flag, MMID or handed-off match is discarded, even if native OnEnd
// was omitted. Call only when the player selects a fresh Throwdown grid item.
void retire_unconfirmed_native_setup() noexcept;
void set_native_setup_options(unsigned seconds,unsigned players) noexcept;
bool flag_placement_active() noexcept;
// Includes the placed registration while its native waiting HUD owns the flag.
bool flag_registration_owned() noexcept;
// Only a confirmed, handed-off flag owns lobby Start/Leave. A new setup
// cannot inherit input from a previous registration's lingering card.
bool flag_registration_waiting() noexcept;
bool flag_registration_retiring() noexcept;
bool owns_flag_registration(std::uint32_t mmid) noexcept;
// A guest joined a mirrored native registration for an advertised custom mode.
// It keeps the stock Join/waiting/Leave flow, without starting its donor event.
bool adopt_native_mode_flag(std::uint64_t world,std::uint32_t game,std::uint32_t mmid,
    const std::array<float,3>& at,float yaw,unsigned seconds,unsigned players,bool race) noexcept;
bool adopt_native_one_up_flag(std::uint64_t world,std::uint32_t mmid,
    const std::array<float,3>& at,float yaw,unsigned seconds,unsigned players) noexcept;
struct FlagSetupOptions { unsigned seconds{}, players{}; };
// The accepted flag keeps its selected limits before the custom lobby has
// received its first state. UI must not borrow the Spot Battle donor defaults.
FlagSetupOptions native_flag_options() noexcept;
void flag_registration_left() noexcept;
void tick_flag_placement(const menu_data::Context& context);
void observe_flag_graph(std::uint32_t graph) noexcept;
void observe_flag_created() noexcept;
// Retain custom queue identities through their asynchronous native teardown.
// Late joins of those queues must not revive a stock Spot Battle notification.
void observe_flag_queue(std::uint32_t mmid) noexcept;
bool known_custom_flag_queue(std::uint32_t mmid) noexcept;
void observe_flag_position(std::uint32_t mmid, const std::array<float, 3>& spawn,
    const std::array<float,16>* transform=nullptr) noexcept;
void observe_flag_exit(bool cancelling) noexcept;
void observe_flag_destroyed(std::uint32_t mmid) noexcept;
void abandon_flag_placement() noexcept;
std::string flag_placement_status();
bool flag_lobby_requested() noexcept;
void flag_lobby_shown() noexcept;
}
