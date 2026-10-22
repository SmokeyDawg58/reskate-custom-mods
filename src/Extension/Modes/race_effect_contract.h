#pragma once
#include "Engine/Game/Build/20260929/race_effects.h"
#include <array>
#include <cstdint>
#include <optional>

namespace dingosdk::modes::race_world::detail {
// EffectManager creates its GPU emitters asynchronously. The simple-effect
// transform message updates their shared world parent. Children retain their
// local pose; applying world placement to them again moves the effect twice.
// Never interpret a pending/released handle, or another helper type, as a GPU emitter.
template<class Pointer,class U32,class U8>
std::optional<std::array<std::uint32_t,3>> checkpoint_particle_handles(
        std::uintptr_t base,std::uintptr_t entity,Pointer pointer,U32 u32,U8 u8,unsigned expected=3) {
    namespace fx=game::build::v20260929::race_effects;
    if(pointer(entity)!=base+fx::effect_entity_vtable)return {};
    const auto handle=pointer(entity+0xd0);
    if(!handle || u8(handle+4)!=1)return {};
    const auto instance=pointer(handle+8);
    if((expected!=1 && expected!=3) || !instance || pointer(instance)!=handle || u32(instance+0x40)!=expected)return {};
    std::array<std::uint32_t,3> result{};
    for(unsigned i=0;i<expected;++i) {
        const auto helper=instance+0x48+i*0x20;
        if(pointer(helper)!=base+fx::mesh_emitter_helper_vtable || !pointer(helper+0x10))return {};
        result[i]=u32(helper+8);
        if(result[i]<2)return {};
        for(unsigned j=0;j<i;++j)if(result[j]==result[i])return {};
    }
    return result;
}
}
