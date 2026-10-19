#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,19> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"b1a58ba92f6507aadaaa3df35efbf14c523d872b8282088f2bbfe3f6eed2178c"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",110549874ULL,"df4d586eac057c693d2d96ed94e1dbf97102a1fe94f472792d378e621571b64b"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",79706160ULL,"bb56a57d740ce424fb736197d313a627b488242f0810fa362bd3c5602d0c848c"},
    {"Win32/customization.toc",1824ULL,"df05c557fad90a3b20492b4c9db002a384bd69278b3751414d5973a21d89e1bb"},
    {"Win32/globals.toc",104232ULL,"613c33048c064818d783df5ed34749190c9d57556300100f062282a7d48f607e"},
    {"Win32/items.toc",227500ULL,"424759a7e40da473991ee5ffb1372c64b559f9a913b2b781e80a4d7246c43497"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1094932ULL,"cbccc9bdc243a96f9fde01640984e953fe8f5cc9db6f0092e99a637ab5e6cc95"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",870668ULL,"8e97620c7488fd35a2739e1382357de76adeca2950878121847fde6bb2d89210"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",937828ULL,"6070c04905e1f6c56836464d839a106a6766e44d6c02cbe553cacf05f33c44b7"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",929988ULL,"0131400c1b0d26619851c053442981530eea5bdf2d173050637fd0c8de3d4a41"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420764ULL,"52badde3f629d98216fc7527364e2596d7c81349254fe833a73899d2e00a85f5"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",923656ULL,"e2554f4e62df9170d82c66e8d3aa9f41af3ca39e52bd22e0ae83fe281f2db06d"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",922808ULL,"539d3e2c0cd2e1b58a124a418cb182b2711b1735782fbabfa6defc460237d522"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131352ULL,"7de88c2545097b11d2c9a97f1be70fbec2db842d12d7e319d9ddac4c15383670"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
    {"Audio/gate-required.wav",131568ULL,"91cd788b631c01ccf343a6835343e9ec71c12f7abbd00a60416e264d10c3bf6d"},
    {"Audio/race-finish.wav",363040ULL,"e1625c1fc3d40f706848f9463f8940f92fdc206de7ece55bb890d1f336dae51c"},
}};
}
