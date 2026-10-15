#pragma once
#include <array>
#include <cstdint>
#include <string_view>
namespace dingosdk::mods::native_package {
inline constexpr auto folder=L"SmokeyDawg58-ReSkate_Throwdown_Assets";
struct File {const char* path;std::uint64_t bytes;std::string_view sha256;};
inline constexpr std::array<File,17> files{{
    {".reskate-studio-patch",109ULL,"7251fea422094e6868c47471d14dd268819dc084e04e0778c48d2451242b591d"},
    {"Win32/buildkititems.toc",61492ULL,"2c69b4b401da5f4a8f6f5928a8eecd628bb0f46096596c85e40858dca0d9cf7b"},
    {"Win32/configurations/layout/defaultinstallpackage/cas_01.cas",313145664ULL,"ad7d967bc8a0cf8d676865a704e6324990eebf625b04ff1419aea613daa0f00f"},
    {"Win32/configurations/layout/initialinstallpackage/cas_01.cas",84738515ULL,"0412de0be24f471110f6849eda646683070999da60f207ac3bf85add8becd45f"},
    {"Win32/customization.toc",1824ULL,"d66e34d5f0e4f41330ec615c3df208573e8472f395fffa3f3cca67914de03c16"},
    {"Win32/globals.toc",104232ULL,"1129c974b9877e2c38ac9f1ec932411f356a190efd8350a88254fcd43aceb64c"},
    {"Win32/items.toc",227500ULL,"96e09788435dad844a0801cbc2441a6ffde452f4f592308b4de6a57cbc3f2e2c"},
    {"Win32/levels/game/bam_levelroot/bam_levelroot.toc",1082472ULL,"6e8a46be8e31309762ec9da7228eb03c0e0b0431eb64816ab48833a26d195864"},
    {"Win32/levels/game/dingolevel_ftue_island/dingolevel_ftue_island.toc",864208ULL,"f646b816fae14ccc515fe5f1f208c6248a553d6f3edd9b931e0c307ab0d4e25d"},
    {"Win32/levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom.toc",891876ULL,"2ce3b198ec18a7896e113aa6ba277309d0a4c7007e1e594387bc122783aae522"},
    {"Win32/levels/game/dingolevel_mpr/dingolevel_mpr.toc",892908ULL,"1c39979c8d8d73e92091d7fe0dabb39150e6f8dfa5ccb3c6be53f97b5e9c3623"},
    {"Win32/levels/game/dingolevel_root/dingolevel_root.toc",420764ULL,"0dfb07e0180705e75b6fca022e30f251fc1b7e253ecf888985c22c7b37d5fb99"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001.toc",887460ULL,"c5383ea80d13e3a71126770131b0645a75fa37c725b0491c33d21510dbe33bcb"},
    {"Win32/levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002.toc",887576ULL,"e50bc6225d89bd9acabcd974d637c3067d630b392dd8e354ecae7efc8b97f857"},
    {"Win32/levels/game/dingolevel_splash/dingolevel_splash.toc",131352ULL,"7159de0ea5f2cbe4978c674047e9893b18eb1f9b1b9fa688ae2fc3fed998e4ff"},
    {"initfs_win32",743439ULL,"b73e95d63933f2e75fca464bc91c00d99c53e23fb392113d868b8379f975eae4"},
    {"layout.toc",14663ULL,"ca1a8c1a8f0d58be15fd82787231783d071b744f77e9cb84e6abcc5201bc22f0"},
}};
}
