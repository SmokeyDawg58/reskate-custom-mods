#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/mods/fbmod.h"
#include "modding/projects/project.h"
#include "modding/materials/shader_depot.h"
#include "modding/materials/material_resolver.h"
#include "modding/materials/shader_lookup.h"
#include "sdk/ebx/boxed_value.h"
#include "sdk/ebx/query.h"
#include "sdk/resources/mesh_set.h"
#include "sdk/resources/texture_resource.h"
#include "modding/textures/png_codec.h"
#include "modding/textures/rime_texture.h"
#include "sdk/io/binary_writer.h"
#include <algorithm>
#include <array>
#include <cstring>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <stdexcept>
namespace sdk=reskate::sdk;
namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;
namespace projects=reskate::modding::projects;
namespace mat=reskate::modding::materials;
using Resource=projects::ProjectResource;
std::string lower(std::string s){std::ranges::transform(s,s.begin(),[](unsigned char c){return char(std::tolower(c));});return s;}
constexpr const char* registries[]{
 "levels/game/bam_levelroot/bam_coregameassets_networkregistry_win32",
 "levels/game/dingolevel_ftue_island/dingolevel_ftue_island_networkregistry_win32",
 "levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom_networkregistry_win32",
 "levels/game/dingolevel_mpr/dingolevel_mpr_networkregistry_win32",
 "levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001_networkregistry_win32",
 "levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002_networkregistry_win32"};
void merge(std::vector<std::string>& to,const std::vector<std::string>& from){for(const auto& s:from)if(std::ranges::find(to,s)==to.end())to.push_back(s);}
bool native_mesh(const std::string& name){auto n=lower(name);return
 n.starts_with("effects/gameplay/meshes/sm_fx_directionalarrow_square") ||
 n.starts_with("effects/gameplay/textures/t_fx_directionalarrow_l03");}


// Kept outside the runtime: authoring reads native palette data and emits
// independent constants, with no global activity data-model bindings.
using CheckpointColors = std::map<std::uint32_t,std::array<float,4>>;

CheckpointColors palette(const reskate::sdk::assets::AssetDatabase& database) {
    namespace sdk = reskate::sdk;
    namespace ui = reskate::modding::maps::ui;
    const auto styleBytes=database.read(ui::require_asset(database,
        "Effects/UI/DataModels/Checkpoint/CheckpointVFXStylelist"));
    const auto style=sdk::ebx::read_document(styleBytes);
    const auto palette=sdk::ebx::read_document(database.read(ui::require_asset(database,
        "UI/StyleGuide/Colors/VFXColorList")));
    auto& colors=ui::require_array(ui::require_field(*palette.root()->object,"AccessibleColorTypes").value,"native VFX palette");
    const sdk::ebx::InstanceRecord* inactive{};
    for(const auto& instance:style.instances) {
        // Select the concrete authored style identity; don't depend on an
        // exported-record array index or on a boxed default's ordering.
        if(instance.object && sdk::ebx::type_name(style,instance)=="DataDefinedTypeInstance" &&
            instance.instanceGuid.string()=="5d5cb22e-9753-45ee-9e24-848e6219d1a0")inactive=&instance;
    }
    if(!inactive || !inactive->object)throw std::runtime_error("native inactive checkpoint style missing");
    auto& values=ui::require_array(ui::require_field(*inactive->object,"FieldValues").value,"native checkpoint style colors");
    // SPF_UI_Checkpoint_Base binds the inactive tutorial palette to these
    // exact mesh inputs. Preserve its yellow/blue/darker-blue split rather
    // than applying one uniform colour to the whole checkpoint.
    const std::map<std::uint32_t,std::pair<std::uint32_t,std::string>> bindings{
        {838132978u,{1216985609u,"Checkpoint Yellow"}},
        {2793603224u,{3689566086u,"Checkpoint Blue"}},
        {3230013911u,{4003928840u,"CheckpointDarkerBlue"}}};
    CheckpointColors result;
    for(auto& value:values) {
        auto& entry=ui::require_object(value,"native checkpoint style entry");
        const auto field=static_cast<std::uint32_t>(*sdk::ebx::integer(ui::require_field(entry,"FieldNameHash").value));
        const auto expected=bindings.find(field);
        if(expected==bindings.end())throw std::runtime_error("native checkpoint style color binding differs");
        const auto* box=sdk::ebx::resolve_boxed(style,std::get<sdk::ebx::BoxedReference>(ui::require_field(entry,"Value").value.data));
        if(!box || box->rawBytes.size()!=48)throw std::runtime_error("native checkpoint DingoColor ABI differs");
        std::uint64_t asset{};std::int64_t relative{};
        std::memcpy(&asset,box->rawBytes.data()+16,sizeof(asset));
        std::memcpy(&relative,box->rawBytes.data()+24,sizeof(relative));
        if(!(asset&1) || (asset>>1)>=style.imports.size() ||
            style.imports[static_cast<std::size_t>(asset>>1)].fileGuid!=palette.fileGuid)
            throw std::runtime_error("native checkpoint palette reference differs: field="+std::to_string(field)+
                " encoded="+std::to_string(asset)+" palette="+palette.fileGuid.string()+
                " actual="+(((asset>>1)<style.imports.size())?style.imports[static_cast<std::size_t>(asset>>1)].fileGuid.string():"out-of-range"));
        const auto at=static_cast<std::int64_t>(style.dataStart)+box->offset+24+relative;
        if(at<0 || static_cast<std::uint64_t>(at)+37>styleBytes.size())
            throw std::runtime_error("native checkpoint palette color ID exceeds bounds");
        const std::string selected(reinterpret_cast<const char*>(styleBytes.data()+at),36);
        bool found{};
        for(auto& color:colors) {
            auto& item=ui::require_object(color,"native palette color");
            if(*sdk::ebx::text(ui::require_field(item,"ColorId").value)!=selected)continue;
            if(*sdk::ebx::text(ui::require_field(item,"Name").value)!=expected->second.second)
                throw std::runtime_error("native checkpoint palette name differs");
            auto& rgb=ui::require_object(ui::require_field(item,"DefaultColorValue").value,"native checkpoint RGB");
            std::array<float,4> channels{};
            for(std::size_t axis=0;axis<3;++axis)
                channels[axis]=static_cast<float>(*sdk::ebx::number(ui::require_field(rgb,std::array{"x","y","z"}[axis]).value));
            result.emplace(expected->second.first,channels);found=true;
            std::cout<<"checkpoint_palette="<<expected->second.second<<" input="<<expected->second.first
                <<" rgb="<<channels[0]<<','<<channels[1]<<','<<channels[2]<<'\n';
        }
        if(!found)throw std::runtime_error("native checkpoint palette color missing");
    }
    if(result.size()!=3)throw std::runtime_error("native checkpoint palette is incomplete");
    return result;
}
int main(int argc,char** argv){try{
 std::cout<<std::unitbuf;
 if(argc!=4)throw std::runtime_error("builder-assets <game-root> <input.fbproject> <output.fbproject>");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 auto project=projects::load_project(argv[2]);const auto before=project;
 std::vector<std::string> bundles;
 for(const auto* n:registries)for(const auto b:ui::require_asset(db,n).bundles)merge(bundles,{db.bundles.at(b).name});
 if(bundles.empty())throw std::runtime_error("map registry bundles missing");
 const auto include=[&](const sdk::assets::NativeAsset& a)->Resource&{
  auto found=std::ranges::find_if(project.resources,[&](const auto& r){
   if(a.kind==sdk::assets::AssetKind::resource)return r.kind==projects::ResourceKind::resource && r.resourceId==a.resourceId;
   if(a.kind==sdk::assets::AssetKind::chunk)return r.kind==projects::ResourceKind::chunk && r.chunkId==a.guid;
   return r.kind==projects::ResourceKind::ebx && lower(r.name)==lower(a.name);
  });
  if(found==project.resources.end()){
   Resource r;r.kind=a.kind==sdk::assets::AssetKind::ebx?projects::ResourceKind::ebx:a.kind==sdk::assets::AssetKind::resource?projects::ResourceKind::resource:projects::ResourceKind::chunk;
   r.name=a.name;r.payload=db.read(a);r.resourceId=a.resourceId;r.resourceType=a.resourceType;r.resourceMeta=a.resourceMeta;
   r.assetGuid=a.guid;r.chunkId=a.guid;r.logicalOffset=a.logicalOffset;r.logicalSize=a.logicalSize;
   if(r.kind==projects::ResourceKind::ebx)r.assetGuid=sdk::ebx::read_document(r.payload).fileGuid;
   project.resources.push_back(std::move(r));found=std::prev(project.resources.end());
  }
  merge(found->bundles,bundles);return *found;
 };

 for(const auto* source:{"Effects/Gameplay/Meshes/SM_FX_DirectionalArrow_Square_AT","Effects/Gameplay/Meshes/SM_FX_DirectionalArrow_Square"}){
  const auto& original=ui::require_asset(db,source);const auto bytes=db.read(original);
  auto doc=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(doc,bytes);ebx::Partition copy(doc);
  // This native stunt-style chevron blueprint has a static model and no
  // animation/billboard schematics or parameter controllers to face the camera.
  if(!copy.find_instance("StaticModelEntityData") || !sdk::ebx::array(ui::require_field(*copy.root()->object,"PropertyConnections").value)->empty())
      throw std::runtime_error("chevron has unexpected dynamic bindings");
  const std::string name=std::string("World/ReSkate/Deathrace/")+(std::string_view(source).ends_with("_AT")?"FloatingArrow_AT":"FloatingArrow_A");
  auto previous=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::ebx && r.name==name;});
  if(previous==project.resources.end())throw std::runtime_error("existing private arrow missing");
  const auto old=sdk::ebx::read_document(previous->payload);
  copy.document().fileGuid=old.fileGuid;copy.root()->instanceGuid=old.root()->instanceGuid;
  ui::require_field(*copy.root()->object,"Name").value.data=name;auto authored=ui::emit(name,copy);
  if(authored.partitionGuid!=old.fileGuid || authored.rootGuid!=old.root()->instanceGuid)throw std::runtime_error("arrow registration identity changed");
  previous->payload=std::move(authored.bytes);merge(previous->bundles,bundles);

 }
 // Explicit native mesh, texture, streamed chunk, and isolated material
 // residency. Never register a whole shared world shader depot.
 std::map<const sdk::assets::NativeAsset*,std::set<std::uint64_t>> materials;
 for(const auto& a:db.assets)if(native_mesh(a.name)){
  include(a);if(a.kind!=sdk::assets::AssetKind::ebx)continue;
  const auto doc=sdk::ebx::read_document(db.read(a));
  std::function<void(const sdk::ebx::Value&)> collect;
  collect=[&](const auto& v){
   if(const auto* ref=std::get_if<sdk::ebx::ResourceReference>(&v.data);ref && ref->id){
    const auto* r=db.find_resource(ref->id);if(!r)throw std::runtime_error("missing native arrow/gate resource");include(*r);
    if(r->resourceType==sdk::resources::meshSetResourceType){
     const auto bytes=db.read(*r);
     if(lower(a.name).find("arrow")!=std::string::npos){
      auto keys=sdk::resources::mesh_material_keys(bytes);
      if(const auto depots=db.resourceTypeIndex.find(mat::shaderBlockDepotResourceType);depots!=db.resourceTypeIndex.end())for(const auto index:depots->second){
       if(keys.empty())break;const auto& depot=db.assets.at(index);
       if(!std::ranges::any_of(depot.bundles,[&](auto b){return std::ranges::find(r->bundles,b)!=r->bundles.end();}))continue;
       const auto bindings=mat::read_material_bindings(db.read(depot),depot.resourceMeta,keys);
       for(const auto& [key,binding]:bindings)materials[&depot].insert(key);
       std::erase_if(keys,[&](auto key){return bindings.contains(key);});
      }
      if(!keys.empty())throw std::runtime_error("unresolved native arrow material");
     }
     for(unsigned lod=0;lod<8;++lod){try{(void)sdk::resources::read_mesh_set(a.name,r->name,bytes,[&](const auto& id,std::size_t){const auto* c=db.find_chunk(id);if(!c)throw std::runtime_error("mesh chunk missing");include(*c);return db.read(*c);},false,lod);}catch(const std::exception&){if(lod==0)throw;break;}}
    }else if(r->resourceType==sdk::resources::textureResourceType){
     const auto texture=sdk::resources::read_texture_resource(db.read(*r),r->resourceMeta);const auto* c=db.find_chunk(texture.chunkId);if(!c)throw std::runtime_error("texture chunk missing");include(*c);
    }
   }
   if(const auto* o=sdk::ebx::object(v))for(const auto& f:o->fields)collect(f.value);
   if(const auto* ar=sdk::ebx::array(v))for(const auto& x:*ar)collect(x);
  };
  for(const auto& i:doc.instances)if(i.object)for(const auto& f:i.object->fields)collect(f.value);
 }
 for(const auto& [depot,set]:materials){
  const std::vector<std::uint64_t> keys(set.begin(),set.end());
  const auto author=mat::ShaderDepotAuthor::find(db,keys.front(),depot->bundles,mat::TextureBindingMode::preserveNative);
  auto isolated=author.build_subset("world/reskate/deathrace/fixedchevrons",keys);
  const auto actual=mat::read_material_bindings(isolated.resource,isolated.resourceMeta,keys),original=mat::read_material_bindings(db.read(*depot),depot->resourceMeta,keys);
  if(actual.size()!=keys.size())throw std::runtime_error("arrow material export missing");
  for(const auto key:keys){const auto& x=actual.at(key).parameters;const auto& y=original.at(key).parameters;if(x.size()!=y.size())throw std::runtime_error("arrow shader changed");for(const auto& [id,v]:y)if(x.at(id).type!=v.type || x.at(id).value!=v.value)throw std::runtime_error("arrow shader parameter changed");}
  std::uint32_t exports{};std::memcpy(&exports,isolated.resourceMeta.data()+12,4);if(exports!=keys.size())throw std::runtime_error("unrelated shader exports");
  Resource r;r.kind=projects::ResourceKind::resource;r.name=isolated.name;r.payload=std::move(isolated.resource);r.resourceMeta=std::move(isolated.resourceMeta);r.resourceId=ebx::random_resource_id();r.resourceType=mat::shaderBlockDepotResourceType;r.added=true;r.bundles=bundles;project.resources.push_back(std::move(r));
 }
 std::vector<std::pair<sdk::io::Guid,sdk::io::Guid>> roots;
 for(auto& r:project.resources)if(lower(r.name).starts_with("world/reskate/deathrace/")){
  merge(r.bundles,bundles);
  if(r.kind!=projects::ResourceKind::ebx)continue;
  const auto doc=sdk::ebx::read_document(r.payload);
  if(sdk::ebx::type_name(doc,*doc.root())=="ObjectBlueprint")roots.emplace_back(doc.fileGuid,doc.root()->instanceGuid);
 }
 if(roots.size()!=11)throw std::runtime_error("expected nine checkpoint and two arrow blueprints");
 // Existing map registrations refer to the same private arrow identities.
 // Keep all six map loader payloads exactly as the working 1.0.3 pack.

 for(const auto& old:before.resources){
  const auto it=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==old.kind && r.name==old.name;});
  if(it==project.resources.end())throw std::runtime_error("existing resource lost");
  auto check=*it;check.bundles=old.bundles;
  if(old.name=="World/ReSkate/Deathrace/FloatingArrow_AT" || old.name=="World/ReSkate/Deathrace/FloatingArrow_A")check.payload=old.payload;
  if(check!=old)throw std::runtime_error("unrelated existing asset changed: "+old.name);
 }
 project.metadata.version="1.0.4";const std::filesystem::path out=argv[3];projects::save_project(project,out);
 auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 namespace tex=reskate::modding::textures;
 const auto texture=tex::read_clone_source(db,ui::require_asset(db,"effects/gameplay/textures/t_fx_directionalarrow_l03_v02"));
 sdk::io::BinaryWriter png;png.bytes(tex::texture_to_png(texture.loaded));png.save(out.parent_path()/"straight-arrow-texture.png");
 std::cout<<"PASS: two private arrow layers replaced with a native fixed stunt-style chevron; all original gate/HUD/map-loader payloads preserved.\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
