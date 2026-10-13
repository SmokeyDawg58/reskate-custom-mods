#pragma once
#include <cstdint>
#include <optional>
#include <span>

namespace dingosdk::multiplayer::menu_data {
struct HudRootCandidate {
    std::uint64_t handle{};
    bool active{};
    unsigned gameplay_stacks{};
};
struct NotificationMountNode {
    std::uint64_t model{};
    std::uint32_t type{};
    std::uint64_t content{};
};
struct NotificationMount {
    std::uint64_t presentation{}, model{};
    bool operator==(const NotificationMount&) const = default;
};
// Only a private model or a deliberately hidden slot belongs to our adapter.
// A new native notification in the same slot must keep its own presentation.
inline bool owns_waiting_presentation(std::uint64_t current,std::uint64_t custom,bool hidden) {
    return custom && (current==custom || (!current && hidden));
}
// The backend reuses a waiting card and its input delegates across challenges.
// Resolve actions through its visible private presentation and current flag.
inline bool owns_waiting_action(std::uint64_t requested_source,std::uint64_t source,
        std::uint64_t current,std::uint64_t custom,bool registration_owned,
        bool retiring,bool lobby) noexcept {
    return requested_source && requested_source==source && custom && current==custom &&
        registration_owned && !retiring && lobby;
}
// Native navigation can move our private notification into a new presenter.
// Recover its source independently of the previous presenter's identity so
// the next challenge can update its title and callbacks.
template<class Copies> std::uint64_t waiting_notification_source(
        std::uint64_t current,const Copies& copies) {
    for(const auto& [source,copy]:copies)if(current==copy.handle)return source;
    return current;
}
// The secondary HUD stack holds FoundationsNotificationViewModel wrappers,
// not NotificationViewModel directly. Resolve only mounted, known wrappers;
// scanning all registered notifications also includes unrendered defaults.
template<class Read> std::optional<NotificationMount> mounted_notification(
        std::uint64_t presentation, Read&& read) {
    for(unsigned depth=0; presentation && depth<8; ++depth) {
        const auto node=read(presentation);
        if(!node.model)return std::nullopt;
        if(node.type==0x1053b9ec)return NotificationMount{presentation,node.model};
        if(node.type!=0xffb83e48 || node.content==presentation)return std::nullopt;
        presentation=node.content;
    }
    return std::nullopt;
}
// Authored defaults are registered as roots too. They have no game-owned
// compass/scoring/notification items and are not connected to a HUD widget.
inline std::uint64_t mounted_hud_root(std::span<const HudRootCandidate> roots,
                                     std::uint64_t preferred = 0) {
    std::uint64_t selected{};
    std::uint64_t retained{};
    unsigned most{};
    for (const auto& root : roots) {
        if (!root.handle || !root.active) continue;
        // Native score/compass stacks briefly empty during activity changes.
        // Retain only a previously verified live identity, never a new default.
        if (!root.gameplay_stacks) {
            if(root.handle==preferred)retained=root.handle;
            continue;
        }
        if (root.handle == preferred) return root.handle;
        if (root.gameplay_stacks > most) {
            selected = root.handle;
            most = root.gameplay_stacks;
        }
    }
    return selected?selected:retained;
}
} // namespace dingosdk::multiplayer::menu_data
