#pragma once
#include <atomic>
#include <cstdint>
#include <mutex>
#include <string>
#include <optional>
#include "deathrace_native_presentation.h"

namespace dingosdk::deathrace_ui {
// The renderer acknowledges the exact race/dialog identity. An old native
// presenter cannot suppress a new route menu while a map/domain is rebuilding.
inline std::atomic<std::uint64_t> presented_world{},presented_at{},presented_serial{};
inline std::atomic<std::uint32_t> presented_game{};
inline std::atomic<bool> native_enabled{true};
struct RouteCursor {
    std::uint64_t world{},serial{};std::uint32_t game{};
    unsigned route{},action{},first{};
    std::string name;
    bool action_focus{};unsigned column{};
};
inline std::mutex route_cursor_mutex;
inline RouteCursor route_cursor;
struct Presentation {Input input;std::vector<Cell> numbers;std::uint64_t world{},serial{},sampled{};std::uint32_t game{};};
inline std::mutex presentation_mutex;
inline Presentation presentation;
inline void present(Input input,std::vector<Cell> numbers,std::uint64_t world,std::uint32_t game,std::uint64_t serial,std::uint64_t now) {
    std::lock_guard lock(presentation_mutex);presentation={std::move(input),std::move(numbers),world,serial,now,game};
}
inline std::optional<Presentation> presentation_at(std::uint64_t now) {
    std::lock_guard lock(presentation_mutex);
    if(!native_enabled.load() || !presentation.sampled || now<presentation.sampled || now-presentation.sampled>=500)return {};
    return presentation;
}
inline bool current(std::uint64_t world,std::uint32_t game,std::uint64_t now) noexcept {
    const auto at=presented_at.load(std::memory_order_acquire);
    return native_enabled.load() && at && now>=at && now-at<500 &&
        presented_world.load()==world && presented_game.load()==game;
}
inline RouteCursor cursor() {std::lock_guard lock(route_cursor_mutex);return route_cursor;}
inline void acknowledge(std::uint64_t world,std::uint32_t game,std::uint64_t serial,std::uint64_t now) {
    presented_at.store(0,std::memory_order_release);
    presented_world.store(world);presented_game.store(game);presented_serial.store(serial);
    presented_at.store(now,std::memory_order_release);
}
inline void retire() {
    skate2_ribbons::retire();
    presented_at.store(0,std::memory_order_release);
    std::lock_guard lock(presentation_mutex);presentation.sampled=0;
}
}
