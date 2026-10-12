#pragma once
#include <cstdint>
#include <optional>

namespace dingosdk::modes {
// A native camera request can be rejected while flag placement is closing.
// Keep asking until the live camera acknowledges it, and drain a late enable
// on exit. A camera already active before the editor remains the user's own.
class PlacementCamera {
public:
    std::optional<bool> update(bool requested, bool active, std::uint64_t now) noexcept {
        if (requested != requested_) {
            requested_ = requested;
            sent_.reset();
            if (!requested && owned_) release_until_ = now + 1600;
        }
        if (requested && !owned_ && !active) owned_ = true;
        if (!owned_) return {};
        if (requested && active) return {};
        if (!requested && !active && now >= release_until_) {
            owned_ = false;
            sent_.reset();
            return {};
        }
        if (sent_ && now >= *sent_ && now - *sent_ < 400) return {};
        sent_ = now;
        return requested;
    }
private:
    bool requested_{}, owned_{};
    std::uint64_t release_until_{};
    std::optional<std::uint64_t> sent_;
};
} // namespace dingosdk::modes
