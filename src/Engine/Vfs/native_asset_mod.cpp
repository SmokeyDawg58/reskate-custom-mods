#include "native_asset_mod.h"
#include "native_asset_mod_manifest.h"
#include "Engine/Core/Platform/launcher_support.h"
#include <stdexcept>

namespace dingosdk::mods {
Mod required_throwdown_asset_mod(const std::filesystem::path& game) {
    const auto directory=game/L"Mods"/native_package::folder;
    for(const auto& file:native_package::files) {
        const auto path=directory/file.path;
        std::error_code error;
        if(!std::filesystem::is_regular_file(path,error) || error)
            throw std::runtime_error("The required ReSkate Throwdown Assets mod is missing or incomplete. Extract the complete playtest package beside Skate.exe.");
        if(std::filesystem::file_size(path,error)!=file.bytes || error || launcher::sha256_file(path)!=file.sha256)
            throw std::runtime_error("The ReSkate Throwdown Assets mod does not match this DLL. Install the complete matching playtest package.");
    }
    Mod core;core.name="SmokeyDawg58-ReSkate_Throwdown_Assets";core.directory=directory;core.provides_layout=true;
    core.title="ReSkate Throwdown Assets";core.author="SmokeyDawg58";core.version="1.0.0";
    core.description="Required native logos, gates, flags, menus and HUD assets.";
    core.studio_marker=true;core.skate_sha256=std::string(launcher::expected_game_sha256);
    return core;
}
}
