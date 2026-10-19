#pragma once
#include <cmath>
#include <optional>
namespace dingosdk::multiplayer {
// The native flag picker uses this movement ceiling independently of its
// ground validator. Retain the latest native value and restore only our edit.
class NativeFlagCursorHeight {
    float previous{};bool owned{};
public:
    static constexpr float ceiling=99999.f;
    std::optional<float> apply(float current) noexcept {
        if(owned && current==ceiling)return {};
        owned=false;
        if(!std::isfinite(current) || current<0 || current>=ceiling)return {};
        previous=current;owned=true;return ceiling;
    }
    std::optional<float> restore(float current) noexcept {
        const bool ours=owned && current==ceiling;owned=false;
        return ours?std::optional{previous}:std::nullopt;
    }
};
}
