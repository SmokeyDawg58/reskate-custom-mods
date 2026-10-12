#pragma once
#include <algorithm>
#include <atomic>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

namespace dingosdk::multiplayer::menu_data {
// Native transitions also unload assets when no ReSkate load request exists.
// Suspend publication and queued actions before cleanup can dispatch callbacks.
// Only a new active world may resume them; shutdown remains permanent.
class MenuLifetime {
    enum class Phase { loading, active, shutdown };
    std::atomic<Phase> phase_{Phase::loading};
public:
    bool blocked() const noexcept { return phase_.load() != Phase::active; }
    template<class Cleanup> void before_transition(unsigned next, Cleanup&& cleanup) {
        if (next == 24) {
            if (phase_.exchange(Phase::shutdown) != Phase::shutdown) cleanup();
        } else if (next == 14 || next == 22 || next == 3) {
            auto expected = Phase::active;
            if (phase_.compare_exchange_strong(expected, Phase::loading)) cleanup();
        } else if (next == 13 || next == 21) {
            auto expected = Phase::loading;
            phase_.compare_exchange_strong(expected, Phase::active);
        }
    }
};

// The selected activity enum is shared by every native challenge. A custom
// Details request may close only the exact page it mounted, never a later
// stock page that happens to select the same enum or reuse an item id.
struct ActivityDetailsOwnership {
    std::uint64_t page{}, layer{};
    int item{};
    bool requested{};
    void request() noexcept { requested = true; }
    void mount(std::uint64_t next_page, std::uint64_t next_layer, int next_item) noexcept {
        page = next_page; layer = next_layer; item = next_item; requested = true;
    }
    bool owns(std::uint64_t live_page, std::uint64_t live_layer, int live_item) const noexcept {
        return requested && page && layer && live_page == page && live_layer == layer && live_item == item;
    }
    void clear_mount() noexcept { page = layer = 0; item = 0; }
    void retire() noexcept { clear_mount(); requested = false; }
};

// The native Layer_Menu capture has TargetIndex=-1 after its transition has
// finished. StackIndex identifies the current item; TargetIndex describes a
// transition request and must never select a page for publication or teardown.
inline std::optional<unsigned> active_activity_stack_item(int stack_index, unsigned count, bool active) noexcept {
    if(!active || stack_index<0 || static_cast<unsigned>(stack_index)>=count)return std::nullopt;
    return static_cast<unsigned>(stack_index);
}

// Model creation returns an independently owned root. Dropping the C++ handle
// does not destroy it. Track partial builds and old pause-menu generations too.
template<class Model> class OwnedMenuModels {
    std::vector<Model> models_;
public:
    bool empty() const { return models_.empty(); }
    std::size_t checkpoint() const noexcept { return models_.size(); }
    void track(Model value) {
        if (std::none_of(models_.begin(), models_.end(), [&](const auto& existing) {
                return existing.handle == value.handle && existing.type == value.type;
            })) models_.push_back(value);
    }
    // For an unmounted build: release only the roots created after its start.
    // The caller must detach any published references first. Pop only after a
    // successful destroy so an interrupted rollback can safely be retried.
    template<class Destroy> void rollback(std::size_t checkpoint, Destroy&& destroy) {
        if(checkpoint>models_.size())throw std::out_of_range("Native menu model checkpoint is no longer valid.");
        while(models_.size()>checkpoint) {
            destroy(models_.back());
            models_.pop_back();
        }
    }
    template<class Detach, class Destroy> void release(Detach&& detach, Destroy&& destroy) {
        if (empty()) return;
        detach();
        while (!models_.empty()) {
            destroy(models_.back());
            models_.pop_back(); // Keep unfinished cleanup available on failure.
        }
    }
    // Use only after the native manager itself has been replaced.
    void manager_replaced() { models_.clear(); }
};

// A command one of a page's native buttons runs: the pause-menu generation it was
// last handed out in, and the render pass that last asked for it.
struct MenuAction { std::uint64_t generation{}; std::string command, argument; std::uint64_t pass{}; };
inline bool waiting_card_action(std::string_view command) noexcept {
    return command=="flag-wait-start" || command=="flag-wait-leave";
}
inline bool action_generation_current(const MenuAction& action,std::uint64_t owner) noexcept {
    // Waiting actions must also pass the exact visible card/flag check when
    // dispatched. Picker and match-specific actions retain their old guard.
    return action.generation && (action.generation==owner || waiting_card_action(action.command));
}

// The slot a button gets for a command. A slot is a native callback whose
// descriptor the game borrows for the rest of the process, so a page has a
// fixed number of them and they are reused, never added to: a command keeps
// the slot it has, whichever generation of the menu gave it out (every map
// load starts a new one); a new command takes the next unused slot; and with
// none left, the slot asked for longest ago. Every button asks on every render
// pass, so a slot not asked for in this pass or the one before is on no button.
// Empty when every slot is on a button.
template<class At> std::optional<std::size_t> action_slot(std::size_t used, std::size_t capacity, At&& at,
        std::string_view command, std::string_view argument, std::uint64_t pass) {
    std::optional<std::size_t> idle;
    for (std::size_t i = 0; i < used; ++i) {
        const MenuAction& action = at(i);
        if (action.command == command && action.argument == argument) return i;
        if (action.pass + 1 < pass && (!idle || action.pass < at(*idle).pass)) idle = i;
    }
    return used < capacity ? used : idle;
}
} // namespace dingosdk::multiplayer::menu_data
