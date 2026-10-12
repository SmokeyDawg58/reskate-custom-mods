#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <utility>
#include <vector>

namespace dingosdk::multiplayer::menu_data {
// Register the private identity before following references. Native menus can
// share mutable labels and refer back to a model already being copied.
template<class Key,class Model> class UiCloneCache {
    std::map<Key,Model> copies_;
public:
    template<class Create,class Follow> Model clone(const Key& key,Create&& create,Follow&& follow) {
        if(const auto found=copies_.find(key);found!=copies_.end())return found->second;
        const auto model=create();copies_.emplace(key,model);follow(model);return model;
    }
};

// Native array copies have independent storage and native stack items animate
// between menu ticks. Only the Content presenter's identities prove a remount.
template<class Presenter> bool matching_presenters(std::span<const std::byte> live,
        std::span<const std::byte> saved,std::size_t stride,std::size_t offset) {
    static_assert(std::is_trivially_copyable_v<Presenter>);
    if(!stride || offset>stride || sizeof(Presenter)>stride-offset || live.size()%stride || saved.size()%stride)
        throw std::invalid_argument("Native presenter array layout differs.");
    if(live.size()!=saved.size())return false;
    for(std::size_t i=0;i<live.size();i+=stride) {
        Presenter current{},expected{};
        std::memcpy(&current,live.data()+i+offset,sizeof(current));
        std::memcpy(&expected,saved.data()+i+offset,sizeof(expected));
        if(current.blueprint!=expected.blueprint || current.data.record!=expected.data.record || current.data.handle!=expected.data.handle)return false;
    }
    return true;
}
template<class Presenter> std::optional<std::vector<std::byte>> remount_presenters(
        std::span<const std::byte> live,std::span<const std::byte> saved,std::size_t stride,std::size_t offset) {
    if(matching_presenters<Presenter>(live,saved,stride,offset))return std::nullopt;
    // A different number of authored slots needs the saved item array. With
    // the same slots, retain every live visibility/transition/focus byte.
    if(live.size()!=saved.size())return std::vector<std::byte>(saved.begin(),saved.end());
    std::vector<std::byte> next(live.begin(),live.end());
    for(std::size_t i=0;i<next.size();i+=stride)
        std::memcpy(next.data()+i+offset,saved.data()+i+offset,sizeof(Presenter));
    return next;
}

struct OwnedNativeAction { std::size_t row{}; std::uintptr_t callback{}; };

// A native page may replace its header before it replaces PageActions. Only
// our callback identity authorizes restoration; unrelated native actions and
// the current array's length must survive cleanup.
inline std::optional<std::vector<std::byte>> restore_owned_actions(
        std::span<const std::byte> live,std::span<const std::byte> original,
        std::size_t stride,std::size_t callback_offset,std::span<const OwnedNativeAction> owned) {
    if(!stride || callback_offset>stride || sizeof(std::uintptr_t)>stride-callback_offset ||
       live.size()%stride || original.size()%stride)
        throw std::invalid_argument("Native action array layout differs.");
    std::vector<std::byte> next(live.begin(),live.end());bool changed{};
    for(const auto& action:owned) {
        if(!action.callback || action.row>=live.size()/stride || action.row>=original.size()/stride)continue;
        std::uintptr_t current{};
        std::memcpy(&current,live.data()+action.row*stride+callback_offset,sizeof(current));
        if(current!=action.callback)continue;
        const auto at=action.row*stride;
        if(std::memcmp(next.data()+at,original.data()+at,stride)) {
            std::memcpy(next.data()+at,original.data()+at,stride);changed=true;
        }
    }
    return changed?std::optional{std::move(next)}:std::nullopt;
}
} // namespace dingosdk::multiplayer::menu_data
