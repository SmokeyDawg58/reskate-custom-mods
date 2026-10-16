#include <Windows.h>
#include "Engine/Vfs/native_asset_mod.h"
#include "Engine/Vfs/native_asset_mod_manifest.h"
#include "Engine/Vfs/mod_catalog.h"
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace {
namespace fs=std::filesystem;
namespace mods=dingosdk::mods;
void require(bool value,const char* message) {if(!value)throw std::runtime_error(message);}
bool rejected(const fs::path& root) {
    try {(void)mods::required_throwdown_asset_mod(root);return false;}
    catch(const std::runtime_error&) {return true;}
}
struct Fixture {
    fs::path parent,path;
    explicit Fixture(const fs::path& directory):parent(fs::absolute(directory).lexically_normal()),
        path(parent/(std::to_string(GetCurrentProcessId())+"-"+std::to_string(GetTickCount64()))) {
        require(path.parent_path()==parent && !fs::exists(path),"Use a fresh child fixture only.");
        fs::create_directories(path);
    }
    ~Fixture() {
        // Only this freshly created absolute child is ever removed. Payload
        // hard links below are read-only; tests never write through a link.
        if(path.parent_path()==parent) {std::error_code error;fs::remove_all(path,error);}
    }
};
fs::path pack(const fs::path& root,std::wstring_view name,const fs::path& source) {
    const auto directory=root/L"Mods"/name;
    for(const auto& file:mods::native_package::files) {
        const auto destination=directory/file.path;
        fs::create_directories(destination.parent_path());
        std::error_code error;fs::create_hard_link(source/file.path,destination,error);
        if(error)fs::copy_file(source/file.path,destination);
    }
    fs::create_directories(root/L"Data");
    {std::ofstream out(root/L"Data"/L"layout.toc");out<<"fixture";}
    return directory;
}
void single_catalog(const fs::path& root,const fs::path& expected) {
    const auto catalog=mods::load_catalog(root,{},false);
    require(catalog.issue.empty() && catalog.mods.size()==1 && catalog.disabled.empty(),"Required assets mount once, even with disabled or duplicate copies.");
    require(catalog.mods.front().directory==expected,"The catalogue must mount the resolved asset folder.");
}
void disable(const fs::path& root,std::string_view name) {
    std::ofstream out(root/L"Mods"/L"mods.json");
    out<<"{\"schema\":1,\"mods\":[{\"name\":\""<<name<<"\",\"enabled\":false}]}";
}
}
int main(int argc,char** argv) {
    try {
        require(argc==3,"Expected a complete packaged client root and fixture directory.");
        const auto source=mods::required_throwdown_asset_mod(argv[1]);
        require(source.provides_layout && source.directory==fs::path(argv[1])/L"Mods"/mods::native_package::folder,"The standard package mounts directly.");
        require(source.studio_marker && source.skate_sha256.size()==64,"Supported game stamp retained.");
        require(mods::native_package::files.size()==17,"All 17 native files retained.");
        const auto base=std::wstring(mods::native_package::folder),versioned=base+L"-1.0.0";
        require(mods::throwdown_asset_folder_name(base) && mods::throwdown_asset_folder_name(versioned),"Plain and versioned extraction names are recognized.");
        require(mods::throwdown_asset_folder_name(L"smokeydawg58-reskate_throwdown_assets-12.34.56"),"Windows folder matching is case insensitive.");
        for(const auto suffix:{L"-backup",L"-1",L"-1.0",L"-1..0",L"-1.0.0.1",L"-1.0.0-extra"})
            require(!mods::throwdown_asset_folder_name(base+suffix),"Unrelated folders must not be claimed as required assets.");

        Fixture fixture(argv[2]);
        const auto missing=fixture.path/L"missing";
        require(rejected(missing),"Missing mod must fail instead of silently losing logos.");
        const auto partial=missing/L"Mods"/base;
        fs::create_directories(partial);
        const auto& first=mods::native_package::files.front();
        {std::ofstream out(partial/first.path,std::ios::binary);out<<std::string(static_cast<std::size_t>(first.bytes),'x');}
        require(rejected(missing),"Same-size corrupt native content must be rejected by hash.");

        const auto canonical_root=fixture.path/L"canonical";
        const auto canonical=pack(canonical_root,base,source.directory);
        disable(canonical_root,canonical.filename().string());
        single_catalog(canonical_root,canonical);

        const auto versioned_root=fixture.path/L"versioned";
        const auto installed=pack(versioned_root,versioned,source.directory);
        const auto resolved=mods::required_throwdown_asset_mod(versioned_root);
        require(resolved.directory==installed && resolved.name==installed.filename().string(),"Windows Extract All version suffix mounts with the real folder identity.");
        disable(versioned_root,installed.filename().string());
        single_catalog(versioned_root,installed);

        const auto duplicate_root=fixture.path/L"duplicates";
        const auto preferred=pack(duplicate_root,base,source.directory);
        pack(duplicate_root,versioned,source.directory);
        require(mods::required_throwdown_asset_mod(duplicate_root).directory==preferred,"Canonical installation has deterministic priority.");
        single_catalog(duplicate_root,preferred);

        // A complete versioned pack must not conceal a broken canonical install.
        pack(missing,versioned,source.directory);
        require(rejected(missing),"Corrupt canonical assets still fail when a valid versioned copy exists.");
        const auto wrong=fixture.path/L"unrelated";
        pack(wrong,base+L"-backup",source.directory);
        require(rejected(wrong),"Backup directories must not be silently mounted.");
        std::cout<<"PASS: canonical/versioned packs, disabled dependency, duplicate suppression, integrity and missing/backup errors.\n";
        return 0;
    } catch(const std::exception& error) {std::cerr<<error.what()<<'\n';return 1;}
}
