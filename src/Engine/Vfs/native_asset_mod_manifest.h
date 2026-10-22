#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,25> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"b1a58ba92f6507aadaaa3df35efbf14c523d872b8282088f2bbfe3f6eed2178c"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",115444174ULL,"09eab6ab7ceebbc0b80ad3a4a22002b9efb1a2b07c2264c1b80580145fc762a3"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",84473045ULL,"eb425465ac48f9e857428e8c2e1b67047d94a37fa53b82d8e02ff44c400e6989"},
    {"Win32/customization.toc",1824ULL,"df05c557fad90a3b20492b4c9db002a384bd69278b3751414d5973a21d89e1bb"},
    {"Win32/globals.toc",104328ULL,"4a0e510e106e41a10aacd3a23b152a2df60f2eb9ff57dcb419e4d2898e986582"},
    {"Win32/items.toc",227500ULL,"a98dc1d8f03b6dc897660632db16eb024b72f7ff699f41aa4481b69583676432"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1100372ULL,"25f3149d722aba2fa662d1379c43fbd2eb207d64c99ffebff5c36f4d58e87d67"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",873640ULL,"d8a5210673f293806d8ea9a62d71d415f575326bb65f3bde5f4e18abac7c7899"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",940796ULL,"5448735b5d89a5d45999c09249e38373632559a316132235dda69725f11844a9"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",932948ULL,"67ecaea6121e6545a8d93258ccf049fd4d2ffc7537904b1703e16e5d6ee7d270"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420796ULL,"f09c42bd7190eddda579ce0c35387e46b9e865b51280a6cecf7874eb38af08c2"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",926624ULL,"de0172051dad23ce1f8f874677c5ac925cc1285e71adbe2f7500e473bde22606"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",925776ULL,"768a0de25b6c6abe1f1557ab780739b2f289c0a518657109d5b57a4d275c84d4"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131448ULL,"4008a86ee0d229140576f5c025ac7ac5801806fc4e9caa9a307f50dad2936beb"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
    {"Audio/gate-required.wav",131568ULL,"91cd788b631c01ccf343a6835343e9ec71c12f7abbd00a60416e264d10c3bf6d"},
    {"Audio/race-finish.wav",363040ULL,"e1625c1fc3d40f706848f9463f8940f92fdc206de7ece55bb890d1f336dae51c"},
    {"UI/Deathrace/gilroy-extrabold.ttf",303176ULL,"d957934829d360f7ca7d237e4e48b2fa971f395a9f3e26d81e72103ab32a1c1f"},
    {"UI/Deathrace/gilroy-semibold.ttf",305120ULL,"d4f9d09340c0fb180265a0a5ef26a31e06de7fbd9bf0d509d861287060e5e617"},
    {"UI/Deathrace/native-hud-atlas.png",246238ULL,"36418630535fe5dc374be971114b77afcbe295e6088c1f23c960272da4d7daa8"},
    {"UI/Deathrace/provenance.json",11204ULL,"be815c933970ee5e68d0b05e0fc249c6f74d354289c7321cb79db8c18766f6be"},
    {"UI/Deathrace/skatemarker-caps.ttf",2464688ULL,"e3c4442777bec5dc86ff421fa6e4bc10cd8e55b06d0b00e8889e0973e0fdac91"},
    {"UI/Deathrace/skatemarker-names.ttf",2483648ULL,"83d2e1948ecdd6fd5de2ce5a209ec068efbc7da1bef11aa9f33bf328de340f59"},
}};
}
