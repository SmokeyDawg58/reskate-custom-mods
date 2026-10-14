#pragma once
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <map>
#include <optional>
#include <span>
#include <stdexcept>
#include <type_traits>
#include <algorithm>
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

// Rebuilding the HUD array must not restart existing items' native fade/slide
// tracks. Reuse the live item by id AND complete presenter identity, including
// when a confirmation or countdown adds/removes another item beside it.
struct HudVisibilityField { std::size_t offset{}, size{}; };
struct HudVisibilityContract {
    std::size_t active_offset{};
    std::span<const std::int32_t> owned_ids;
    std::span<const HudVisibilityField> restore_on_show;
};
template<class Presenter> std::optional<std::vector<std::byte>> reconcile_hud_items(
        std::span<const std::byte> live,std::span<const std::byte> desired,
        std::size_t stride,std::size_t id_offset,std::size_t presenter_offset,
        const HudVisibilityContract* visibility=nullptr) {
    static_assert(std::is_trivially_copyable_v<Presenter>);
    if(!stride || id_offset>stride || sizeof(std::int32_t)>stride-id_offset ||
       presenter_offset>stride || sizeof(Presenter)>stride-presenter_offset || live.size()%stride || desired.size()%stride)
        throw std::invalid_argument("Native HUD item layout differs.");
    if(visibility) {
        if(visibility->active_offset>=stride)throw std::invalid_argument("Native HUD active field differs.");
        for(const auto& field:visibility->restore_on_show)
            if(field.offset>stride || field.size>stride-field.offset)
                throw std::invalid_argument("Native HUD visibility field differs.");
    }
    const auto needs_show=[&](const std::byte* current,const std::byte* expected) {
        if(!visibility || current[visibility->active_offset]!=std::byte{} || expected[visibility->active_offset]==std::byte{})return false;
        std::int32_t id{};std::memcpy(&id,expected+id_offset,sizeof(id));
        return std::find(visibility->owned_ids.begin(),visibility->owned_ids.end(),id)!=visibility->owned_ids.end();
    };
    const auto same=[&](const std::byte* a,const std::byte* b) {
        std::int32_t ai{},bi{};Presenter ap{},bp{};
        std::memcpy(&ai,a+id_offset,sizeof(ai));std::memcpy(&bi,b+id_offset,sizeof(bi));
        std::memcpy(&ap,a+presenter_offset,sizeof(ap));std::memcpy(&bp,b+presenter_offset,sizeof(bp));
        return ai==bi && ap.blueprint==bp.blueprint && ap.data.record==bp.data.record && ap.data.handle==bp.data.handle;
    };
    bool unchanged=live.size()==desired.size();
    if(unchanged)for(std::size_t at=0;at<live.size();at+=stride)
        if(!same(live.data()+at,desired.data()+at) || needs_show(live.data()+at,desired.data()+at)) {unchanged=false;break;}
    if(unchanged)return std::nullopt;
    std::vector<std::byte> next(desired.begin(),desired.end());
    for(std::size_t at=0;at<next.size();at+=stride) {
        std::optional<std::size_t> found;bool exact{},ambiguous{};
        for(std::size_t old=0;old<live.size();old+=stride) {
            if(std::memcmp(next.data()+at,live.data()+old,stride)==0) {exact=true;break;}
            if(same(next.data()+at,live.data()+old)) {
                if(found)ambiguous=true;else found=old;
            }
        }
        if(!exact && !ambiguous && found) {
            const bool show=needs_show(live.data()+*found,desired.data()+at);
            std::memcpy(next.data()+at,live.data()+*found,stride);
            if(show) {
                next[at+visibility->active_offset]=desired[at+visibility->active_offset];
                for(const auto& field:visibility->restore_on_show)
                    std::memcpy(next.data()+at+field.offset,desired.data()+at+field.offset,field.size);
            }
        }
    }
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
