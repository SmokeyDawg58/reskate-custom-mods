#pragma once
#include <cstdint>
#include <string>
namespace dingosdk::modes::race_world {
void tick(std::uintptr_t base,std::uintptr_t client,bool ready) noexcept;
void entity_destroyed(std::uintptr_t entity) noexcept;
std::string start_error(std::uint64_t world,std::uint32_t game);
}
