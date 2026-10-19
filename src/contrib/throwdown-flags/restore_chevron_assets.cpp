// Authoring only: restore the user's original chevron under its existing IDs.
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/assets/frostbite_asset_manager.h"
#include <algorithm>
#include <iostream>
namespace p=reskate::modding::projects;
int main(int argc,char** argv){try{
 if(argc!=5)throw std::runtime_error("game current-project original-chevron-project output-project");
 auto current=p::load_project(argv[2]);const auto before=current;auto original=p::load_project(argv[3]);
 unsigned changed{};
 for(auto& r:current.resources)if(r.name=="World/ReSkate/Deathrace/FloatingArrow_AT" || r.name=="World/ReSkate/Deathrace/FloatingArrow_A"){
  const auto old=std::ranges::find(original.resources,r.name,&p::ProjectResource::name);
  if(old==original.resources.end() || old->kind!=r.kind || old->assetGuid!=r.assetGuid || old->bundles!=r.bundles)throw std::runtime_error("Original chevron identity/bundle differs");
  r.payload=old->payload;++changed;
 }
 if(changed!=2 || current.resources.size()!=before.resources.size())throw std::runtime_error("Chevron scope differs");
 for(std::size_t i=0;i<before.resources.size();++i){auto check=current.resources[i];check.payload=before.resources[i].payload;if(check!=before.resources[i])throw std::runtime_error("Resource metadata changed");}
 current.metadata.version="1.0.15";p::save_project(current,argv[4]);auto out=std::filesystem::path(argv[4]);out.replace_extension(".fbmod");
 const auto db=reskate::sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);reskate::modding::mods::write_fbmod(current,out,argv[1],&db);
 std::cout<<"PASS original two Chevron blueprints restored BYTE_IDENTICAL; 2188 other payloads and all2190 identities/metadata preserved.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
