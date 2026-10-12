#pragma once
#include <cstdint>
#include <span>
#include <string>
#include <string_view>
#include <vector>

namespace dingosdk::throwdowns {
enum class Adapter : std::uint8_t { native_skate, native_spot, native_jam, native_one_up, game_mode };
struct Palette {
    std::uint32_t primary{}, secondary{}, accent{}, background=0xff171717U, text=0xffffffffU;
    bool operator==(const Palette&) const = default;
};
struct Definition {
    std::string id, name, description, key, logo;
    Adapter adapter{};
    unsigned mode{}, donor=1;
    Palette palette;
    bool needs_points{}, supports_turns{}, supports_players=true;
    bool operator==(const Definition&) const = default;
};
struct Registration {
    bool accepted{}, inserted{};
    std::string error;
};
class Registry {
    std::vector<Definition> definitions_;
    std::uint64_t revision_{};
public:
    Registration add(Definition definition);
    const Definition* find(std::string_view id) const noexcept;
    const Definition* mode(unsigned value) const noexcept;
    std::span<const Definition> definitions() const noexcept { return definitions_; }
    std::uint64_t revision() const noexcept { return revision_; }
};
Palette palette_for(std::string_view stable_id) noexcept;
Registry& registry(); // Built-ins registered once; no process-global DLL ABI changes.
}
