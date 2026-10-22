#pragma once
#include "race_route.h"
#include <array>
#include <cmath>
namespace dingosdk::modes {
// One fixed placement for the native EffectBlueprint. Its three GPU emitters
// own animation; the world root never jitters, changes size or drifts with time.
inline std::array<float,16> race_native_effect_pose(Vec3 at,float yaw,float pitch,float roll,
        float half_width) noexcept {
    const auto axes=race_gate_axes(yaw,pitch);
    const float a=roll*3.14159265359f/180.f,cr=std::cos(a),sr=std::sin(a);
    const float scale=half_width;
    std::array<float,16> result{};
    for(unsigned i=0;i<3;++i){result[i]=(axes.right[i]*cr+axes.up[i]*sr)*scale;result[4+i]=(-axes.right[i]*sr+axes.up[i]*cr)*scale;result[8+i]=axes.forward[i]*scale;result[12+i]=at[i];}
    result[15]=1;return result;
}
}
