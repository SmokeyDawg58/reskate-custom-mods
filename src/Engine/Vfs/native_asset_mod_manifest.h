#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,19> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"22e61dec3c24419302d2f41d424d4d78bd63f36e195e8db0ffeb0e31fc3b09bf"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",381685536ULL,"cdda82307677ad5246e39fef890c4e24aeabcef1209155664ee11a4b740175ab"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",95235355ULL,"7a395700eed2d24e0cd4ecfbc1d1205c5d50ee234442249a7b1bd79cc82518c9"},
    {"Win32/customization.toc",1824ULL,"27744d7777f2a8b5ec8ceea36de84645594eec2c53c4fe2d19bdd149ffcd13d2"},
    {"Win32/globals.toc",104232ULL,"1129c974b9877e2c38ac9f1ec932411f356a190efd8350a88254fcd43aceb64c"},
    {"Win32/items.toc",227500ULL,"2b55135fcbf80fc54c9526883b3f6c2c9f00fc4068cc44a9b2772d482aa48c84"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1094128ULL,"9c203347c55b105d5da3922bb84e52be4ad9774e68a0938326b10a23e62d450e"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",870244ULL,"e6f804e646bc43c9818a0d71bdc20f9cf985af1317835e83ecfcf63857749870"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",937400ULL,"c5da688e84fc6deeeba29448621af113306bdf47b037892b34d617d06b4b954f"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",929560ULL,"f3576cb95ac67e3b5c56b5808c175ab7d018357a3231ce28c7e32f16f495fc37"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420764ULL,"c530b98fc3a403aa0c33e37b3a8da96efacea83c570284eaa5fcfc153e2667b0"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",923228ULL,"1b2c00c7bc050c1a33ba1bb3c7b754c91a359b45141fa40a2bfe030f515ca582"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",922380ULL,"b7e616da3257716f17c9dcf325fb9287eff4fde1bf98a1048c984d2f669286fb"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131352ULL,"61925e569415f8a85c7672cbb76b7c9e8d02b0026f3640a90b97e0b1667722a7"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
    {"Audio/gate-required.wav",131568ULL,"91cd788b631c01ccf343a6835343e9ec71c12f7abbd00a60416e264d10c3bf6d"},
    {"Audio/race-finish.wav",363040ULL,"e1625c1fc3d40f706848f9463f8940f92fdc206de7ece55bb890d1f336dae51c"},
}};
}
