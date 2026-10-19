#include "race_route_library.h"
#include "race_route_json.h"
#include <cctype>
#include <set>
#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>
#endif

namespace dingosdk::modes {
namespace {
std::filesystem::path utf8_path(std::string_view value) {return std::filesystem::path(std::u8string(value.begin(),value.end()));}
std::string file_component(std::string value) {
    for(auto& ch:value)if(static_cast<unsigned char>(ch)<32 || std::string_view("<>:\"/\\|?*").find(ch)!=std::string_view::npos)ch='_';
    while(!value.empty() && (value.back()=='.' || value.back()==' '))value.pop_back();
    if(value.empty() || value=="." || value=="..")value="Route";
    auto stem=value.substr(0,value.find('.'));for(auto& c:stem)c=static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
    if(stem=="CON" || stem=="PRN" || stem=="AUX" || stem=="NUL" ||
       (stem.size()==4 && (stem.starts_with("COM") || stem.starts_with("LPT")) && stem[3]>='1' && stem[3]<='9'))value.insert(value.begin(),'_');
    return value;
}
std::string read_text(const std::filesystem::path& file) {
    if(!std::filesystem::is_regular_file(file) || std::filesystem::is_symlink(file) || std::filesystem::file_size(file)>65536)
        throw std::runtime_error("Choose a readable Deathrace JSON file smaller than 64 KB.");
    std::ifstream in(file,std::ios::binary);std::string text{std::istreambuf_iterator<char>(in),{}};
    if(!in && !in.eof())throw std::runtime_error("The route file could not be read.");return text;
}
void atomic_write(const std::filesystem::path& file,const std::string& text,bool replace) {
    auto temporary=file;temporary+=".tmp";
    if(std::filesystem::is_symlink(temporary) || std::filesystem::is_symlink(file))throw std::runtime_error("A route file cannot be a link.");
    std::ofstream output(temporary,std::ios::binary|std::ios::trunc);output.write(text.data(),static_cast<std::streamsize>(text.size()));output.close();
    if(!output)throw std::runtime_error("The route could not be written. Your editor stays open.");
#ifdef _WIN32
    if(!MoveFileExW(temporary.c_str(),file.c_str(),MOVEFILE_WRITE_THROUGH|(replace?MOVEFILE_REPLACE_EXISTING:0)))
        throw std::system_error(static_cast<int>(GetLastError()),std::system_category(),"The route file could not be saved");
#else
    if(!replace && std::filesystem::exists(file))throw std::runtime_error("That route filename already exists.");
    std::filesystem::rename(temporary,file);
#endif
}
}
std::filesystem::path RaceRouteLibrary::path(std::uint64_t id) const {
    const auto it=paths_.find(id);if(it==paths_.end())throw std::runtime_error("This saved route is no longer available.");return it->second;
}
std::uint64_t RaceRouteLibrary::save(NamedRaceRoute route) {
    route.name=race_route_name(std::move(route.name));
    const auto old=route.id?std::find_if(routes_.begin(),routes_.end(),[&](const auto& value){return value.id==route.id;}):routes_.end();
    if(route.id && old==routes_.end())throw std::runtime_error("This saved route is no longer available.");
    if(!route.id && routes_.size()>=256)throw std::runtime_error("The route library is full.");
    if(std::any_of(routes_.begin(),routes_.end(),[&](const auto& value){return value.id!=route.id && value.map==route.map && value.name==route.name;}))
        throw std::runtime_error("That route name already exists on this map. Choose another name.");
    if(!route.id)route.id=next_++;
    const auto text=encode_named_race_json(route);if(text.size()>65536)throw std::runtime_error("This route is too large to save.");
    const auto map_name=route.map.substr(route.map.find_last_of("/\\")==std::string::npos?0:route.map.find_last_of("/\\")+1);
    const auto folder=directory_/utf8_path(file_component(map_name));
    if(std::filesystem::is_symlink(folder))throw std::runtime_error("The map's route folder cannot be a link.");
    std::filesystem::create_directories(folder);
    const auto final=folder/utf8_path(file_component(route.name)+".json");
    const auto previous=old==routes_.end()?std::filesystem::path{}:path(route.id);
    const bool exists=std::filesystem::exists(final);
    const bool same=exists && !previous.empty() && std::filesystem::exists(previous) && std::filesystem::equivalent(final,previous);
    if(exists && !same)throw std::runtime_error("That route filename already exists. Choose another name.");
    atomic_write(final,text,same);
    if(!previous.empty() && previous.extension()==".json" && previous!=final && !same) {
        std::error_code error;std::filesystem::remove(previous,error);
        if(error){std::filesystem::remove(final);throw std::runtime_error("The original route file could not be renamed.");}
    }
    paths_[route.id]=final;
    const auto id=route.id;if(old!=routes_.end())*old=std::move(route);else routes_.push_back(std::move(route));return id;
}
void RaceRouteLibrary::read() {
    routes_.clear();paths_.clear();next_=1;
    if(!std::filesystem::exists(directory_)) {
        if(legacy_directory_.empty() || !std::filesystem::exists(legacy_directory_))return;
        std::filesystem::create_directories(directory_);
    }
    if(std::filesystem::is_symlink(directory_))throw std::runtime_error("The route folder cannot be a link.");
    std::vector<std::filesystem::path> files,legacy;
    for(std::filesystem::recursive_directory_iterator it(directory_),end;it!=end;++it) {
        if(it->is_symlink()){if(it->is_directory())it.disable_recursion_pending();continue;}
        if(it.depth()>1){it.disable_recursion_pending();continue;}
        if(!it->is_regular_file())continue;
        if(it->path().extension()==".json")files.push_back(it->path());
        if(it.depth()==0 && it->path().extension()==".race")legacy.push_back(it->path());
        if(files.size()+legacy.size()>4096)break;
    }
    std::sort(files.begin(),files.end());std::sort(legacy.begin(),legacy.end());
    for(const auto& file:files)try {
        if(routes_.size()>=256)break;
        auto route=decode_named_race_json(read_text(file));if(!route)continue;
        if(route->id && route->id<UINT64_MAX)next_=std::max(next_,route->id+1);
        if(!route->id || find(route->id))route->id=next_++;
        paths_[route->id]=file;routes_.push_back(std::move(*route));
    }catch(const std::exception&){}
    for(const auto& file:legacy)try {
        if(routes_.size()>=256)break;
        const auto stem=file.stem().string();std::uint64_t id{};const auto parsed=std::from_chars(stem.data(),stem.data()+stem.size(),id);
        if(parsed.ec!=std::errc{} || parsed.ptr!=stem.data()+stem.size() || !id || id==UINT64_MAX || find(id))continue;
        const auto text=read_text(file);const auto bytes=std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size());
        if(auto route=decode_named_race(bytes)) {
            route->id=id;next_=std::max(next_,id+1);paths_[id]=file;routes_.push_back(*route);
            save(*route); // Preserve the original .race file as the migration backup.
        }
    }catch(const std::exception&){}
    migrate_legacy_directory();
    std::sort(routes_.begin(),routes_.end(),[](const auto& a,const auto& b){return std::tie(a.map,a.name,a.id)<std::tie(b.map,b.name,b.id);});
}
void RaceRouteLibrary::migrate_legacy_directory() {
    if(legacy_directory_.empty() || !std::filesystem::exists(legacy_directory_))return;
    const auto marker=directory_/".legacy-root-migration-v1";
    if(std::filesystem::exists(marker))return;
    if(std::filesystem::is_symlink(legacy_directory_))throw std::runtime_error("The previous route folder cannot be a link.");
    std::vector<std::filesystem::path> files,legacy;
    for(std::filesystem::recursive_directory_iterator it(legacy_directory_),end;it!=end;++it) {
        if(it->is_symlink()){if(it->is_directory())it.disable_recursion_pending();continue;}
        if(it.depth()>1){it.disable_recursion_pending();continue;}
        if(!it->is_regular_file())continue;
        if(it->path().extension()==".json")files.push_back(it->path());
        if(it.depth()==0 && it->path().extension()==".race")legacy.push_back(it->path());
        if(files.size()+legacy.size()>4096)throw std::runtime_error("Too many files in the previous route folder.");
    }
    std::sort(files.begin(),files.end());std::sort(legacy.begin(),legacy.end());
    std::set<std::uint64_t> json_ids;
    const auto copy=[&](NamedRaceRoute route) {
        if(std::any_of(routes_.begin(),routes_.end(),[&](const auto& r){return r.name==route.name && r.map==route.map && r.settings==route.settings;}))return;
        route.id=0;save(std::move(route));
    };
    for(const auto& file:files) {
        std::optional<NamedRaceRoute> route;
        try {route=decode_named_race_json(read_text(file));}catch(const std::exception&){}
        if(route){json_ids.insert(route->id);copy(std::move(*route));}
    }
    for(const auto& file:legacy) {
        const auto stem=file.stem().string();std::uint64_t id{};const auto parsed=std::from_chars(stem.data(),stem.data()+stem.size(),id);
        if(parsed.ec!=std::errc{} || parsed.ptr!=stem.data()+stem.size() || !id || json_ids.contains(id))continue;
        std::optional<NamedRaceRoute> route;
        try {const auto text=read_text(file);route=decode_named_race(std::span(reinterpret_cast<const std::uint8_t*>(text.data()),text.size()));}catch(const std::exception&){}
        if(route)copy(std::move(*route));
    }
    // Leave the old files as backups. This receipt prevents later renames in
    // Mods from resurrecting the original root-folder names on the next read.
    atomic_write(marker,"complete\n",false);
}
std::uint64_t RaceRouteLibrary::import_json(const std::filesystem::path& source,const std::string& current_map) {
    auto route=decode_named_race_json(read_text(source));if(!route)throw std::runtime_error("This is not a valid completed Deathrace route JSON file.");
    if(route->map!=current_map)throw std::runtime_error("This route belongs to another map. Switch to that map before loading it.");
    for(const auto& local:routes_) {
        const auto file=path(local.id);
        if(std::filesystem::exists(file) && std::filesystem::equivalent(source,file)) {
            route->id=local.id;return save(std::move(*route));
        }
        if(local.map==route->map && local.name==route->name && local.settings==route->settings)return local.id;
    }
    route->id=0;return save(std::move(*route));
}
}
