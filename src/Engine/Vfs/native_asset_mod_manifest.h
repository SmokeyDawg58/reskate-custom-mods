#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,19> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"c9557ebedb0c1a3323ae289956daaf79176c744d31806a51a9541a8e009d1877"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",393020603ULL,"392aca148449eabe63314d0db7764bfd6f89fd8f094462a454227568bfecf90f"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",101666123ULL,"8fb17015829cf407a7166fc0c6255d69396f4ded4fdfeaf55865fb80914f5a51"},
    {"Win32/customization.toc",1824ULL,"b62a60e86c7566239c29fa37b50febcf6d3ab46bc90bcda634eee5d997b27436"},
    {"Win32/globals.toc",104232ULL,"1129c974b9877e2c38ac9f1ec932411f356a190efd8350a88254fcd43aceb64c"},
    {"Win32/items.toc",227500ULL,"2ef3d8cf3a107a8afecfb5663223189debabde0674f7f63b398c04bfc5797ee4"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1094256ULL,"484b82c0bd0d495fd19c4269e4a3d6f17fd481dbb897bea2cf26a5ab41ea9709"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",870328ULL,"5d6d4203b747135e49b78cb9d49d4444a26f670152254ae50e64628fbd19d6d7"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",937484ULL,"08c22d7a414883e5957f318a4f02a726abbe6fa586158b9f8b929f97154bd1fe"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",929644ULL,"a2fe28724b6decb5f71156ffa3a80f77557ad20d356df4297a0fd8b63ce3886f"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420764ULL,"47889759f3134884bf26f13b843ace4b7525ec0307a485132c8486c5492bbcf4"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",923312ULL,"14bcebf064336c29cca7ffe1562d6ca25d474ac34ab4fac12581e90c8f535327"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",922464ULL,"49592fb9c62a0728125cdb97d8481b28487c22692eb31333ddf647e6dfbdc9c8"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131352ULL,"158bc16fe71b2572cad8c4b254e39f6db300151452e7c7b2952e62ba448a510c"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
    {"Audio/gate-required.wav",131568ULL,"91cd788b631c01ccf343a6835343e9ec71c12f7abbd00a60416e264d10c3bf6d"},
    {"Audio/race-finish.wav",363040ULL,"e1625c1fc3d40f706848f9463f8940f92fdc206de7ece55bb890d1f336dae51c"},
}};
}
