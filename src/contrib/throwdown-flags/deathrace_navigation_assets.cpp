#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/materials/shader_depot.h"
#include "sdk/ebx/boxed_value.h"
#include "sdk/ebx/query.h"
#include "sdk/resources/mesh_set.h"
#include <algorithm>
#include <array>
#include <fstream>
#include <iostream>
#include <functional>

namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
using Resource=projects::ProjectResource;
void frame(sdk::ebx::Object& obj,double width,double height,double x=0,double y=0) {
 for(const auto [key,value]:{std::pair{"Width",width},std::pair{"Height",height}})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=value;
 for(const auto* key:{"FitWidthToContent","FitHeightToContent"})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=false;
 for(const auto* key:{"HorizontalLayout","VerticalLayout"})if(auto* f=sdk::ebx::find_field(obj,key)){
  auto& axis=ui::require_object(f->value,key);const double offset=std::string_view(key)=="HorizontalLayout"?x:y;
  for(const auto* n:{"AnchorStart","AnchorEnd","SizingPivot"})ui::require_field(axis,n).value.data=0.;
  for(const auto* n:{"OffsetStart","OffsetEnd"})ui::require_field(axis,n).value.data=offset;
  ui::require_field(axis,"SizingWeight").value.data=1.;
 }
 if(auto* f=sdk::ebx::find_field(obj,"Visible"))f->value.data=true;
}
void white(sdk::ebx::Object& obj,bool blue=false) {
 ui::require_field(obj,"FillStyle").value.data=sdk::ebx::PointerReference{};
 ui::require_field(obj,"Alpha").value.data=1.;
 auto& color=ui::require_object(ui::require_field(obj,"ColorValue").value,"color");
 for(const auto* key:{"Asset","AccessibleAsset"})ui::require_field(color,key).value.data=sdk::ebx::PointerReference{};
 ui::require_field(color,"SelectedColorId").value.data=std::string{};
 auto& raw=ui::require_object(ui::require_field(color,"RawColor").value,"raw");
 ui::require_field(raw,"x").value.data=blue?.05:1.;ui::require_field(raw,"y").value.data=blue?.8:1.;ui::require_field(raw,"z").value.data=1.;
}
int main(int argc,char** argv){try{
 std::cout<<std::unitbuf;if(argc!=4)throw std::runtime_error("navigation-assets game-root input.fbproject output.fbproject");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 auto project=projects::load_project(argv[2]);const auto baseline=project;
 const std::string prefix="UI/ReSkate/Throwdowns/";
 const auto source=[&](const std::string& name){const auto it=std::ranges::find(project.resources,prefix+name,&Resource::name);if(it==project.resources.end())throw std::runtime_error("missing donor "+name);return *it;};
 unsigned added{};
 const auto emit=[&](Resource donor,const std::string& suffix,ebx::Partition& part){
  const auto name=prefix+suffix;if(std::ranges::find(project.resources,name,&Resource::name)!=project.resources.end())throw std::runtime_error("asset exists: "+name);
  ui::require_field(*part.root()->object,"Name").value.data=name;auto authored=ui::emit(name,part);
  donor.name=name;donor.payload=std::move(authored.bytes);donor.assetGuid=authored.partitionGuid;donor.added=true;
  project.resources.push_back(std::move(donor));++added;
 };
 for(const auto* theme:{"oneup","jam","meat","race","domination","graffiti","skate","tag","infection","hide"})
  for(unsigned width=320;width<=1792;width+=32){
   auto donor=source("HUD_"+std::string(theme)+"_Name_640_Widget");auto doc=sdk::ebx::read_document(donor.payload);
   sdk::ebx::decode_boxed_values(doc,donor.payload);ebx::Partition part(doc);
   for(auto& instance:part.document().instances)if(instance.object){
    const auto type=sdk::ebx::type_name(part.document(),instance);
    if(type!="RimeWidgetEntityData" && type!="RimeContainerElementData" && type!="RimeFillElementData" && type!="RimeLabelElementData")continue;
    for(const auto [key,old_value,new_value]:{std::tuple{"Width",640.,double(width)},std::tuple{"Height",76.,112.}})
     if(auto* f=sdk::ebx::find_field(*instance.object,key)){
      const auto value=sdk::ebx::number(f->value);
      if(value && *value==old_value)f->value.data=new_value;
      else if(type=="RimeLabelElementData" && std::string_view(key)=="Width" && value && *value==616.)f->value.data=double(width)-24.;
     }
   }
   emit(donor,"HUD_"+std::string(theme)+"_Title_"+std::to_string(width)+"_Widget",part);
  }
 {
  auto donor=source("HUD_race_Name_224_Widget");auto doc=sdk::ebx::read_document(donor.payload);sdk::ebx::decode_boxed_values(doc,donor.payload);ebx::Partition part(doc);
  auto& document=part.document();document.instances.reserve(document.instances.size()+10);
  auto* root=part.find_instance("RimeWidgetEntityData");auto* bounds=part.find_instance("RimeContainerElementData");auto* fill=part.find_instance("RimeFillElementData");
  if(!root || !bounds || !fill)throw std::runtime_error("gate icon donor differs");
  frame(*root->object,64,64);frame(*bounds->object,64,64);
  const auto parent_index=std::size_t(bounds-document.instances.data()),fill_index=std::size_t(fill-document.instances.data());
  ui::require_array(ui::require_field(*part.root()->object,"PropertyConnections").value,"icon bindings").clear();
  ui::require_array(ui::require_field(*bounds->object,"Elements").value,"icon children").clear();
  // A native vector gate pictogram: two posts, top beam and inset bright trim.
  for(const auto rect:std::array<std::array<double,4>,7>{{{8,12,9,44},{47,12,9,44},{8,8,48,9},{4,54,17,5},{43,54,17,5},{17,17,3,35},{44,17,3,35}}}){
   auto& stroke=part.clone_instance(document.instances[fill_index]);frame(*stroke.object,rect[2],rect[3],rect[0],rect[1]);white(*stroke.object,added%2==1);
   ui::require_array(ui::require_field(*document.instances[parent_index].object,"Elements").value,"gate strokes").push_back({part.point_to(stroke)});
  }
  emit(donor,"GateCompass_Widget",part);
 }
 for(const auto& old:baseline.resources){const auto it=std::ranges::find_if(project.resources,[&](const auto& current){return current.kind==old.kind && current.name==old.name;});if(it==project.resources.end() || *it!=old)throw std::runtime_error("baseline asset changed: "+old.name);}
 project.metadata.version="1.0.5";projects::save_project(project,argv[3]);
 auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS new_native_ui_assets="<<added<<" original_assets=byte_exact version=1.0.5\n";
 // Read-only evidence for the arrow glow investigation: exact native keys,
 // texture bindings and compiled constants, without changing shared shaders.
 for(const auto* name:{"effects/gameplay/meshes/sm_fx_directionalarrow_square_mesh","effects/gameplay/meshes/sm_fx_directionalarrow_square_at_mesh"}){
  const auto document=sdk::ebx::read_document(db.read(ui::require_asset(db,name)));
  const auto* field=sdk::ebx::find_field(*document.root()->object,"MeshSetResource");if(!field)continue;
  const auto id=std::get<sdk::ebx::ResourceReference>(field->value.data).id;const auto* resource=db.find_resource(id);if(!resource)continue;
  const auto keys=sdk::resources::mesh_material_keys(db.read(*resource));
  for(const auto key:keys){std::cout<<"ARROW_MATERIAL "<<name<<" key="<<key<<'\n';
   try{for(const auto& parameter:reskate::modding::materials::dump_block_parameters(db,key)){
    std::cout<<"PARAM "<<std::hex<<parameter.hash<<std::dec<<" type="<<parameter.type;
    if(!parameter.texture.empty())std::cout<<" texture="<<parameter.texture;
    for(auto value:parameter.values)std::cout<<' '<<value;std::cout<<'\n';
   }}catch(const std::exception& error){std::cout<<"PARAM_UNAVAILABLE "<<error.what()<<'\n';}
  }
 }
}catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}}
