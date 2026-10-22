#pragma once
#include <imgui.h>
#include <array>
#include <cstring>
#include <string>
namespace dingosdk::deathrace_ui {
struct NativeFonts {ImFont *body{},*secondary{},*marker{},*names{},*display{};std::array<ImFont*,3> clocks{};};
// Shared by the runtime and offscreen visual checks, including exact native
// TrueType bytes, physical bake sizes and Unicode symbol fallback.
template<class Read> NativeFonts load_native_fonts(Read&& read,const std::string& symbol_path) {
    NativeFonts f;auto* atlas=ImGui::GetIO().Fonts;
    static const ImWchar native[]{0x20,0xffff,0},digits[]{0x20,0x3a,0},symbols[]{0x2600,0x27bf,0};
    const auto add=[&](const wchar_t* key,float size,const ImWchar* ranges){
        auto bytes=read(key);if(bytes.empty())return static_cast<ImFont*>(nullptr);
        auto* copy=IM_ALLOC(bytes.size());std::memcpy(copy,bytes.data(),bytes.size());
        ImFontConfig config;config.FontDataOwnedByAtlas=true;config.OversampleH=1;config.OversampleV=1;config.PixelSnapH=false;
        return atlas->AddFontFromMemoryTTF(copy,static_cast<int>(bytes.size()),size,&config,ranges);
    };
    const auto merge=[&](ImFont* target,float size){if(target && !symbol_path.empty()){
        ImFontConfig config;config.MergeMode=true;config.DstFont=target;config.OversampleH=1;config.OversampleV=1;
        atlas->AddFontFromFileTTF(symbol_path.c_str(),size,&config,symbols);
    }};
    f.body=add(L"DEATHRACE_FONT",72,native);merge(f.body,72);
    f.secondary=add(L"DEATHRACE_SECONDARY",64,native);merge(f.secondary,64);
    f.marker=add(L"DEATHRACE_MARKER",128,native);merge(f.marker,128);
    f.names=add(L"DEATHRACE_NAMES",72,native);merge(f.names,72);
    f.display=add(L"DEATHRACE_FONT",224,digits);
    for(unsigned i=0;i<3;++i)f.clocks[i]=add(L"DEATHRACE_FONT",std::array{96.f,144.f,192.f}[i],digits);
    return f;
}
}
