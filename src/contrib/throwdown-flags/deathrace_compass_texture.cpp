#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/textures/png_codec.h"
#include "sdk/ebx/query.h"
#include "sdk/io/binary_writer.h"
#include <algorithm>
#include <iostream>

namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace projects=reskate::modding::projects;namespace tex=reskate::modding::textures;
using Resource=projects::ProjectResource;
int main(int argc,char** argv){try{
 if(argc!=5)throw std::runtime_error("compass-texture game-root input.fbproject output.fbproject gate-compass.png");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 auto project=projects::load_project(argv[2]);const auto baseline=project;
 const auto donor=std::ranges::find(project.resources,std::string("UI/ReSkate/Throwdowns/GateCompass_Widget"),&Resource::name);
 if(donor==project.resources.end() || donor->bundles.empty())throw std::runtime_error("compass owning bundles unavailable");
 const auto bundles=donor->bundles;
 ui::UiAssets assets;
 const auto pixels=tex::load_artwork_image(argv[4]);
 const auto picture=ui::clone_rime_texture(db,"UI/Textures/Icons/Activities/Throwdowns/FullColor/img_Icon_Throwdown_Color_Full_1024",
     "UI/ReSkate/Throwdowns/img_GateCompass",pixels,assets);
 (void)picture;
 const auto add=[&](projects::ResourceKind kind,const std::string& name,std::vector<std::byte> payload)->Resource& {
  if(std::ranges::any_of(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;}))throw std::runtime_error("asset collision: "+name);
  Resource resource;resource.kind=kind;resource.name=name;resource.payload=std::move(payload);resource.bundles=bundles;resource.added=true;
  project.resources.push_back(std::move(resource));return project.resources.back();
 };
 for(auto& part:assets.partitions){auto& resource=add(projects::ResourceKind::ebx,part.name,std::move(part.bytes));resource.assetGuid=part.partitionGuid;}
 for(auto& texture:assets.textures){
  const sdk::resources::LoadedTexture encoded{sdk::resources::read_texture_resource(texture.texture.resource,texture.texture.resourceMeta),texture.texture.chunk};
  const auto decoded=tex::decode_png(tex::texture_to_png(encoded));
  std::size_t clear{},colored{};
  for(std::size_t p=0;p<decoded.rgba.size();p+=4){clear+=decoded.rgba[p+3]<16;colored+=decoded.rgba[p+3]>128 && decoded.rgba[p]!=decoded.rgba[p+2];}
  if(clear<decoded.rgba.size()/8 || !colored)throw std::runtime_error("gate texture lost its open passage, transparency or blue/yellow color");
  sdk::io::BinaryWriter preview;preview.bytes(tex::texture_to_png(encoded));preview.save(std::filesystem::path(argv[3]).parent_path()/"encoded-gate-preview.png");
  auto& ebx=add(projects::ResourceKind::ebx,texture.name,std::move(texture.ebx));ebx.assetGuid=texture.fileGuid;
  auto& res=add(projects::ResourceKind::resource,texture.name,std::move(texture.texture.resource));
  res.resourceId=texture.resourceId;res.resourceType=sdk::resources::textureResourceType;res.resourceMeta=std::move(texture.texture.resourceMeta);
  auto& chunk=add(projects::ResourceKind::chunk,texture.chunkId.string(),std::move(texture.texture.chunk));
  chunk.chunkId=texture.chunkId;chunk.logicalSize=static_cast<std::uint32_t>(chunk.payload.size());chunk.firstMip=0;
  chunk.h32=static_cast<std::int32_t>(tex::texture_name_hash(texture.name));
 }
 for(auto& extra:assets.resources){auto& resource=add(projects::ResourceKind::resource,extra.name,std::move(extra.payload));resource.resourceId=extra.id;resource.resourceType=extra.type;resource.resourceMeta=std::move(extra.metadata);}
 if(!assets.natives.empty())throw std::runtime_error("unexpected native dependency; inspect before export");
 for(const auto& old:baseline.resources){const auto it=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==old.kind && r.name==old.name;});if(it==project.resources.end() || *it!=old)throw std::runtime_error("unrelated asset changed: "+old.name);}
 project.metadata.version="1.0.6";projects::save_project(project,argv[3]);
 auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS transparent blue/yellow gate picture; original_assets=byte_exact added="<<project.resources.size()-baseline.resources.size()<<" version=1.0.6\n";
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
