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
 n.starts_with("effects/_global/meshes/sm_fx_gp_checkpoint_") ||
 n.starts_with("effects/_global/meshes/sm_fx_gp_lightningbolt_a") ||
 n.starts_with("effects/gameplay/textures/t_fx_checkpoint_") ||
 n.starts_with("effects/_global/meshes/sm_fx_1m_arrow_sch_a") ||
 n.starts_with("effects/gameplay/textures/t_fx_arrow_");}

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
 if(argc!=4)throw std::runtime_error("game input-project output-project");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=projects::load_project(argv[2]);const auto before=project;
 const auto colors=palette(db);unsigned changed{};
 for(const auto* source:{"Effects/_Global/Meshes/SM_FX_1m_Arrow_Sch_A_AT","Effects/_Global/Meshes/SM_FX_1m_Arrow_Sch_A"}){
  const auto bytes=db.read(ui::require_asset(db,source));auto doc=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(doc,bytes);ebx::Partition copy(doc);
  const std::string name=std::string("World/ReSkate/Deathrace/")+(std::string_view(source).ends_with("_AT")?"FloatingArrow_AT":"FloatingArrow_A");
  auto previous=std::ranges::find(project.resources,name,&Resource::name);if(previous==project.resources.end())throw std::runtime_error("Arrow identity missing");
  const auto identity=sdk::ebx::read_document(previous->payload);
  auto* block=copy.find_instance("ExpressionShaderParameterBlockEntityData");if(!block)throw std::runtime_error("gate-style Arrow parameter component missing");
  for(auto& value:ui::require_array(ui::require_field(*block->object,"EntryData").value,"Arrow constants")){
   auto& entry=ui::require_object(value,"Arrow constant");const auto hash=std::uint32_t(*sdk::ebx::integer(ui::require_field(entry,"InputHash").value));
   auto* box=sdk::ebx::resolve_boxed(copy.document(),std::get<sdk::ebx::BoxedReference>(ui::require_field(entry,"Value").value.data));if(!box)throw std::runtime_error("native constant missing");
   ui::require_field(entry,"Dynamic").value.data=false;ui::require_field(entry,"Static").value.data=true;
   if(hash==103992823u){if(box->rawBytes.size()!=1)throw std::runtime_error("visibility ABI mismatch");box->rawBytes[0]=std::byte{1};if(box->decoded)box->decoded->data=true;}
   else if(colors.contains(hash)){
    if(box->rawBytes.size()!=16)throw std::runtime_error("color ABI mismatch");const auto& c=colors.at(hash);std::memcpy(box->rawBytes.data(),c.data(),16);
    if(box->decoded){auto& o=ui::require_object(*box->decoded,"color");for(unsigned i=0;i<3;++i)ui::require_field(o,std::array{"x","y","z"}[i]).value.data=double(c[i]);}
   }else if(hash!=151970902u && hash!=2563060036u)throw std::runtime_error("unknown native Arrow input");
  }
  auto& root=*copy.root()->object;
  ui::require_field(root,"PropertyConnections").value.data=sdk::ebx::Value::Array{};
  for(const auto* field:{"Interface","ClientSchematics","ServerSchematics"})ui::require_field(root,field).value.data=sdk::ebx::PointerReference{};
  ui::require_field(ui::require_object(ui::require_field(root,"SchematicsBusFieldOffset").value,"schematics offset"),"OffsetOrHash").value.data=std::uint64_t{65535};
  // Keep the native model-to-parameter-block link, exactly as on working gates.
  if(ui::require_array(ui::require_field(root,"LinkConnections").value,"model parameter link").size()!=1)throw std::runtime_error("native parameter link missing");
  copy.document().fileGuid=identity.fileGuid;copy.root()->instanceGuid=identity.root()->instanceGuid;ui::require_field(root,"Name").value.data=name;copy.prune_unreachable();
  auto asset=ui::emit(name,copy);auto check=sdk::ebx::read_document(asset.bytes);sdk::ebx::decode_boxed_values(check,asset.bytes);
  if(asset.partitionGuid!=identity.fileGuid || asset.rootGuid!=identity.root()->instanceGuid || check.imports.size()!=1)throw std::runtime_error("Arrow registration or dependency isolation changed");
  for(const auto& ref:check.imports){if(std::ranges::none_of(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::ebx && r.assetGuid==ref.fileGuid;}))throw std::runtime_error("resident native mesh missing");std::cout<<"ARROW "<<name<<" MESH "<<ref.fileGuid.string()<<"\n";}
  previous->payload=std::move(asset.bytes);++changed;
 }
 for(std::size_t i=0;i<before.resources.size();++i){auto compare=project.resources.at(i);if(compare.name=="World/ReSkate/Deathrace/FloatingArrow_AT" || compare.name=="World/ReSkate/Deathrace/FloatingArrow_A")compare.payload=before.resources[i].payload;if(compare!=before.resources[i])throw std::runtime_error("unrelated resource changed");}
 if(changed!=2 || project.resources.size()!=before.resources.size())throw std::runtime_error("unexpected project scope");
 project.metadata.version="1.0.13";projects::save_project(project,argv[3]);auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS: two gate-style Arrow blueprints; fixed colors and visible constant; no camera schematic; all other payloads and all resource metadata unchanged.\n";
}catch(const std::exception& e){std::cerr<<e.what();return 1;}}
