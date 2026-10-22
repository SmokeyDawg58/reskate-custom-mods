// Authoring only: replace compass pictures without changing their asset IDs,
// dimensions, formats, bundle ownership, or any world gate/Arrow resource.
#include "modding/maps/ui/ui_assets.h"
#include "modding/textures/rime_texture.h"
#include "modding/textures/png_codec.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include "sdk/io/binary_writer.h"
#include <algorithm>
#include <array>
#include <iostream>
#include <set>
namespace p=reskate::modding::projects;namespace ui=reskate::modding::maps::ui;
namespace tex=reskate::modding::textures;namespace sdk=reskate::sdk;
using Resource=p::ProjectResource;
void png(const std::filesystem::path& path,const std::vector<std::byte>& bytes) {sdk::io::BinaryWriter w;w.bytes(bytes);w.save(path);}
int main(int argc,char** argv){try{
    if(argc!=5)throw std::runtime_error("game-root input.fbproject output.fbproject artwork-folder");
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
    auto project=p::load_project(argv[2]);const auto baseline=project;
    const auto& donor=ui::require_asset(db,"UI/Textures/Icons/Activities/Throwdowns/FullColor/img_Icon_Throwdown_Color_Full_1024");
    std::set<std::string> changed;
    for(const auto* name:{"UI/ReSkate/Throwdowns/img_GateCompass","UI/ReSkate/Throwdowns/img_FinishGateCompass"}) {
        const auto owner=std::ranges::find(project.resources,std::string(name),&Resource::name);
        if(owner==project.resources.end())throw std::runtime_error("Existing compass owner missing");
        auto doc=sdk::ebx::read_document(owner->payload);
        const auto variants=tex::rime_texture_variants(db,project,donor,doc);
        const auto pixels=tex::load_artwork_image(std::filesystem::path(argv[4])/(std::string(name).find("Finish")!=std::string::npos?"compass-finish-gate.png":"gate-compass.png"));auto edits=tex::replace_rime_textures(db,project,variants,0,&pixels);
        for(auto& resource:edits.resources) {
            const auto old=std::ranges::find(project.resources,resource.name,&Resource::name);
            if(old==project.resources.end() || old->kind!=resource.kind)throw std::runtime_error("Compass chunk identity missing");
            auto metadata=resource;metadata.payload=old->payload;metadata.logicalSize=old->logicalSize;
            if(metadata!=*old || resource.payload.size()!=old->payload.size())throw std::runtime_error("Compass layout changed");
            changed.insert(resource.name);*old=std::move(resource);
        }
        png(std::filesystem::path(argv[3]).parent_path()/(std::string(name).find("Finish")!=std::string::npos?"finish-triangle.png":"required-triangle.png"),tex::texture_to_png(edits.preview));
    }
    for(const auto& old:baseline.resources) {
        auto now=*std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==old.kind && r.name==old.name;});
        if(changed.contains(old.name)){now.payload=old.payload;now.logicalSize=old.logicalSize;}
        if(now!=old)throw std::runtime_error("Unrelated asset changed: "+old.name);
    }
    project.metadata.version="1.0.17";p::save_project(project,argv[3]);
    auto out=std::filesystem::path(argv[3]);out.replace_extension(".fbmod");
    reskate::modding::mods::write_fbmod(project,out,argv[1],&db);
    std::cout<<"PASS changed_compass_pixel_chunks="<<changed.size()<<" added_resources="<<project.resources.size()-baseline.resources.size()<<" other_assets=BYTE_IDENTICAL version=1.0.17\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
