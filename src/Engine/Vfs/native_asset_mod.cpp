#include "native_asset_mod.h"
#include "native_asset_mod_manifest.h"
#include "Engine/Core/Platform/launcher_support.h"
#include <algorithm>
#include <stdexcept>
#include <vector>

namespace dingosdk::mods {
namespace {
std::wstring lower(std::wstring_view name) {
    std::wstring result(name);
    for(auto& ch:result)if(ch>=L'A' && ch<=L'Z')ch+=L'a'-L'A';
    return result;
}
std::string asset_issue(const std::filesystem::path& directory) {
    for(const auto& file:native_package::files) {
        const auto path=directory/file.path;
        std::error_code error;
        if(!std::filesystem::is_regular_file(path,error) || error)
            return "The ReSkate Throwdown Assets mod is incomplete: "+directory.filename().string()+"/"+file.path+" is missing. Extract the complete asset ZIP into your active Mods folder.";
        if(std::filesystem::file_size(path,error)!=file.bytes || error || launcher::sha256_file(path)!=file.sha256)
            return "The ReSkate Throwdown Assets mod does not match this DLL: "+directory.filename().string()+"/"+file.path+". Install the complete matching playtest package.";
    }
    return {};
}
Mod verified_mod(const std::filesystem::path& directory) {
    Mod core;core.name=directory.filename().string();core.directory=directory;core.provides_layout=true;
    core.title="ReSkate Throwdown Assets";core.author="SmokeyDawg58";core.version="1.0.0";
    core.description="Required native logos, gates, flags, menus and HUD assets.";
    core.studio_marker=true;core.skate_sha256=std::string(launcher::expected_game_sha256);
    return core;
}
}
bool throwdown_asset_folder_name(std::wstring_view name) {
    if(name.size()>maximum_mod_name)return false;
    const auto base=lower(native_package::folder),normalized=lower(name);
    if(normalized==base)return true;
    if(!normalized.starts_with(base+L"-"))return false;
    const auto suffix=std::wstring_view(normalized).substr(base.size()+1);
    unsigned dots=0,digits=0;
    for(const auto ch:suffix) {
        if(ch>=L'0' && ch<=L'9'){++digits;continue;}
        if(ch!=L'.' || !digits || ++dots>2)return false;
        digits=0;
    }
    return dots==2 && digits!=0;
}
Mod required_throwdown_asset_mod(const std::filesystem::path& game) {
    const auto mods=game/L"Mods",canonical=mods/native_package::folder;
    // Prefer the installer's exact destination. Do not hide a broken canonical
    // installation by selecting another copy behind the user's back.
    if(std::filesystem::exists(canonical)) {
        if(auto issue=asset_issue(canonical);!issue.empty())throw std::runtime_error(issue);
        return verified_mod(canonical);
    }
    std::vector<std::filesystem::path> candidates;
    std::error_code error;
    for(std::filesystem::directory_iterator at(mods,error),end;!error && at!=end;at.increment(error))
        if(at->is_directory(error) && throwdown_asset_folder_name(at->path().filename().wstring()))candidates.push_back(at->path());
    std::sort(candidates.begin(),candidates.end());
    std::string issue;
    for(const auto& directory:candidates) {
        auto current=asset_issue(directory);
        if(current.empty())return verified_mod(directory);
        if(issue.empty())issue=std::move(current);
    }
    if(!issue.empty())throw std::runtime_error(issue);
    throw std::runtime_error("The required ReSkate Throwdown Assets mod is missing. Extract its ZIP into the active Mods folder, with layout.toc and Win32 directly inside SmokeyDawg58-ReSkate_Throwdown_Assets or SmokeyDawg58-ReSkate_Throwdown_Assets-1.0.0.");
}
}
