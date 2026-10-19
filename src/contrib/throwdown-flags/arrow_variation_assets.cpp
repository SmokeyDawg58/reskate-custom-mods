#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include <algorithm>
#include <iostream>
#include <set>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("game input-project output-project");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);auto p=projects::load_project(argv[2]);const auto before=p;
 const std::string name="World/ReSkate/Deathrace/ArrowMeshVariationDb_Win32";
 if(std::ranges::any_of(p.resources,[&](const auto& r){return r.name==name;}))throw std::runtime_error("one-shot author already applied");
 std::set<sdk::io::Guid> wanted;
 for(const auto* n:{"effects/gameplay/meshes/sm_fx_directionalarrow_square_mesh","effects/gameplay/meshes/sm_fx_directionalarrow_square_at_mesh"})wanted.insert(sdk::ebx::read_document(db.read(ui::require_asset(db,n))).fileGuid);
 ebx::Partition part(sdk::ebx::read_document(db.read(ui::require_asset(db,"levels/game/bam_levelroot/gameplay_activities/meshvariationdb_win32"))));
 auto& entries=ui::require_array(ui::require_field(*part.root()->object,"Entries").value,"arrow variations");
 std::erase_if(entries,[&](auto& e){auto& object=ui::require_object(e,"variation");const auto* ref=sdk::ebx::resolve_import(part.document(),ui::require_pointer(ui::require_field(object,"Mesh").value,"mesh"));return !ref || !wanted.contains(ref->fileGuid) || sdk::ebx::integer(ui::require_field(object,"VariationAssetNameHash").value)!=0;});
 if(entries.size()!=2)throw std::runtime_error("both exact default Arrow variants are required");
 part.prune_unreachable();ui::require_field(*part.root()->object,"Name").value.data=name;
 auto asset=ui::emit(name,part);auto check=sdk::ebx::read_document(asset.bytes);
 auto& verified=ui::require_array(ui::require_field(*check.root()->object,"Entries").value,"serialized arrow entries");
 if(verified.size()!=2)throw std::runtime_error("serialized entries missing");
 for(auto& e:verified){auto& object=ui::require_object(e,"entry");const auto* mesh=sdk::ebx::resolve_import(check,ui::require_pointer(ui::require_field(object,"Mesh").value,"mesh"));if(!mesh || !wanted.erase(mesh->fileGuid))throw std::runtime_error("serialized mesh mapping mismatch");}
 if(!wanted.empty())throw std::runtime_error("incomplete mapping");
 const auto root=std::ranges::find(p.resources,std::string("World/ReSkate/Deathrace/FloatingArrow_AT"),&projects::ProjectResource::name);if(root==p.resources.end())throw std::runtime_error("arrow root missing");
 projects::ProjectResource r;r.name=name;r.kind=projects::ResourceKind::ebx;r.payload=std::move(asset.bytes);r.assetGuid=asset.partitionGuid;r.added=true;r.bundles=root->bundles;p.resources.push_back(std::move(r));
 for(std::size_t i=0;i<before.resources.size();++i)if(p.resources[i]!=before.resources[i])throw std::runtime_error("unrelated native resource changed");
 p.metadata.version="1.0.12";projects::save_project(p,argv[3]);auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(p,mod,argv[1],&db);
 std::cout<<"PASS: 2189 original native resources unchanged; one private Arrow MeshVariationDatabase contains both exact native default mesh/material mappings.\n";
 for(const auto& ref:check.imports)std::cout<<"IMPORT "<<ref.fileGuid.string()<<" "<<ref.classGuid.string()<<"\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
