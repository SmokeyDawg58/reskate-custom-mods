#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,19> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"c9b58792bdf4358324880fb604531945f5aebf8ffd98091f43028ba9ebf7f97a"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",338783860ULL,"e6e3b62b533195c770fe1ea308215cb9bdcee43b7c6bf2dfeffd922c30df749e"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",94301279ULL,"fed903909e6fe696e49174d6a0172094c6e26ac7d08f7d32069240c97e7b517f"},
    {"Win32/customization.toc",1824ULL,"f1f514ba83d9ee677e079eedba100d9e10b4b4864087efc3c06f6e7c3ab113f8"},
    {"Win32/globals.toc",104232ULL,"1129c974b9877e2c38ac9f1ec932411f356a190efd8350a88254fcd43aceb64c"},
    {"Win32/items.toc",227500ULL,"f73ae1dddbf5b9ebc853a62c3a22e717f633f5006a82f6e9f254151605bd6d1f"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1084380ULL,"5376a8633e048d3e747e57a84e5f70ef43d013f9ba4df39f89c8385e68268036"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",865208ULL,"fdae3f1d2e2672fccf5754c99b05345314b41f4d23021ee9eab01912645f2f74"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",892872ULL,"41245efc52122dd039c383f070995693da00c727304165a5dd0c3220b1d42904"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",893904ULL,"5df213280e54c3768df30ecdb618bdbc37d3cff57234148149c7efae2d794550"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420764ULL,"66c227390dc0db72625a3b9d4c8be3f5324073621733f7e75be2e0e4651dd99a"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",888456ULL,"157dba6abded22fffdc1e8e97fd074fd2f4762b0ab2c821c0ac391a3577a95a8"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",888572ULL,"2475976f4f18e92cf904b6684e00994bbf4836509071899f4f08693388f7d53c"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131352ULL,"9c08412af0ae8a004c133d7e68000f8a8d4b2f5645e4f6baf3914bfe8e2ddc2a"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
    {"Audio/gate-required.wav",131568ULL,"91cd788b631c01ccf343a6835343e9ec71c12f7abbd00a60416e264d10c3bf6d"},
    {"Audio/race-finish.wav",363040ULL,"e1625c1fc3d40f706848f9463f8940f92fdc206de7ece55bb890d1f336dae51c"},
}};
}
