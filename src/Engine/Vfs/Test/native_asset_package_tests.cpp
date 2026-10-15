#include <Windows.h>
#include "Engine/Vfs/native_asset_mod.h"
#include "Engine/Vfs/native_asset_mod_manifest.h"
#include "Engine/Vfs/mod_catalog.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
bool rejected(const std::filesystem::path& root) {
    try {(void)dingosdk::mods::required_throwdown_asset_mod(root);return false;}
    catch(const std::runtime_error&) {return true;}
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"Expected a complete packaged client root and fixture directory.");
        const auto mod=dingosdk::mods::required_throwdown_asset_mod(argv[1]);
        require(mod.provides_layout && mod.directory==std::filesystem::path(argv[1])/L"Mods"/dingosdk::mods::native_package::folder,"Required assets mount the mod folder directly.");
        require(mod.studio_marker && mod.skate_sha256.size()==64,"Supported game stamp retained.");
        require(dingosdk::mods::native_package::files.size()==17,"All 17 native files retained.");
        const auto package=std::filesystem::path(argv[1]);
        require(!std::filesystem::exists(package/L"Data") && !std::filesystem::exists(package/L"Mods"/L"mods.json"),"Only an isolated package fixture may be used.");
        std::filesystem::create_directory(package/L"Data");
        {std::ofstream out(package/L"Data"/L"layout.toc");out<<"fixture";}
        auto catalog=dingosdk::mods::load_catalog(package,{},false);
        require(catalog.issue.empty() && catalog.mods.size()==1,"The required mod must mount exactly once.");
        {std::ofstream out(package/L"Mods"/L"mods.json");out<<R"({"schema":1,"mods":[{"name":"SmokeyDawg58-ReSkate_Throwdown_Assets","enabled":false}]})";}
        catalog=dingosdk::mods::load_catalog(package,{},false);
        require(catalog.issue.empty() && catalog.mods.size()==1 && catalog.disabled.empty(),"Required assets remain a single dependency when its optional duplicate is disabled.");
        std::filesystem::remove(package/L"Mods"/L"mods.json");
        std::filesystem::remove(package/L"Data"/L"layout.toc");std::filesystem::remove(package/L"Data");
        const auto fixture=std::filesystem::path(argv[2])/std::to_string(GetCurrentProcessId());
        std::filesystem::create_directories(fixture);
        require(rejected(fixture),"Missing mod must fail instead of silently losing logos.");
        const auto partial=fixture/L"Mods"/dingosdk::mods::native_package::folder;
        std::filesystem::create_directories(partial);
        const auto& first=dingosdk::mods::native_package::files.front();
        const auto file=partial/first.path;
        {std::ofstream out(file,std::ios::binary);out<<std::string(static_cast<std::size_t>(first.bytes),'x');}
        require(rejected(fixture),"Same-size corrupt native content must be rejected by hash.");
        std::filesystem::remove(file);std::filesystem::remove(partial);std::filesystem::remove(fixture/L"Mods");std::filesystem::remove(fixture);
        std::cout<<"Required asset mod: full payload validated; missing and corrupted packages rejected.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
