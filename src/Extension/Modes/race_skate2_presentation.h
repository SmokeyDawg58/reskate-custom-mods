#pragma once
#include <array>
#include <atomic>
#include <cstdint>
#include <vector>
namespace dingosdk::skate2_ribbons {
struct Ribbon {std::array<float,3> at{};float yaw{},pitch{},roll{},width=6;unsigned id{};bool arrow{},finish{},guide{},preview{};};
struct View {std::array<float,16> camera{};float fov{};std::vector<Ribbon> ribbons;bool active{};};
// Never suppress resident native meshes until the new renderer has uploaded
// its resources and drawn the matching world and challenge successfully.
inline std::atomic<std::uint64_t> ready_world{},ready_at{};
inline std::atomic<std::uint32_t> ready_game{};
inline void acknowledge(std::uint64_t world,std::uint32_t game,std::uint64_t now){ready_at.store(0,std::memory_order_release);ready_world.store(world);ready_game.store(game);ready_at.store(now,std::memory_order_release);}
inline bool current(std::uint64_t world,std::uint32_t game,std::uint64_t now){const auto at=ready_at.load(std::memory_order_acquire);return at && now>=at && now-at<500 && ready_world.load()==world && ready_game.load()==game;}
inline void retire(){ready_at.store(0,std::memory_order_release);}
}
