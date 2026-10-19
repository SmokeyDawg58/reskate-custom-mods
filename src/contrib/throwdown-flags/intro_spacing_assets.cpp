#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/boxed_value.h"
#include <algorithm>
#include <iostream>
#include <set>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("intro-spacing game input-project output-project");
 auto project=projects::load_project(argv[2]);unsigned panels{};
 for(auto& r:project.resources)if(r.name.starts_with("UI/ReSkate/Throwdowns/IntroPanel_")){
  auto d=sdk::ebx::read_document(r.payload);sdk::ebx::decode_boxed_values(d,r.payload);ebx::Partition p(d,false);unsigned changed{};
  for(auto& i:p.document().instances)if(i.object && sdk::ebx::type_name(p.document(),i).starts_with("Rime"))
   if(auto* f=sdk::ebx::find_field(*i.object,"Height"))if(sdk::ebx::number(f->value).value_or(-1)==304.){f->value.data=448.;++changed;}
  if(changed<3)throw std::runtime_error("intro panel bounds differ");
  const auto a=ui::emit(r.name,p);if(a.partitionGuid!=r.assetGuid)throw std::runtime_error("intro identity changed");r.payload=a.bytes;++panels;
 }
 if(panels!=10)throw std::runtime_error("expected ten private intro panels");
 auto found=std::ranges::find(project.resources,"UI/ReSkate/Throwdowns/RaceResultsHeading_Widget",&projects::ProjectResource::name);
 if(found==project.resources.end())throw std::runtime_error("native notice label donor absent");
 auto resource=*found;auto d=sdk::ebx::read_document(resource.payload);sdk::ebx::decode_boxed_values(d,resource.payload);ebx::Partition p(d);
 const auto name="UI/ReSkate/Throwdowns/IntroRules_Widget";
 ui::require_field(*p.root()->object,"Name").value.data=std::string(name);
 for(auto& i:p.document().instances)if(i.object){
  const auto type=sdk::ebx::type_name(p.document(),i);
  if(type.starts_with("Rime")){
   for(const auto [key,oldv,newv]:{std::tuple{"Width",608.,960.},std::tuple{"Height",144.,128.}})
    if(auto* f=sdk::ebx::find_field(*i.object,key))if(sdk::ebx::number(f->value).value_or(-1)==oldv)f->value.data=newv;
   for(const auto* key:{"FitWidthToContent","FitHeightToContent"})if(auto* f=sdk::ebx::find_field(*i.object,key))f->value.data=false;
  }
  if(type=="RimeLabelElementData"){
   ui::require_field(*i.object,"Width").value.data=960.;ui::require_field(*i.object,"Height").value.data=128.;
   ui::require_field(*i.object,"PointSize").value.data=36.;
   ui::require_field(*i.object,"WordWrap").value.data=true;
   ui::require_field(*i.object,"HorizontalAlignment").value.data=std::int64_t{2};
   ui::require_field(*i.object,"VerticalAlignment").value.data=std::int64_t{2};
   for(const auto* axis:{"HorizontalLayout","VerticalLayout"}){
    auto& l=ui::require_object(ui::require_field(*i.object,axis).value,axis);
    for(const auto* key:{"AnchorStart","AnchorEnd","OffsetStart","OffsetEnd","SizingPivot"})ui::require_field(l,key).value.data=0.;
    ui::require_field(l,"SizingWeight").value.data=1.;
   }
  }
 }
 // Keep text, style and colour bindings, but never let the inherited notice
 // model change the fixed label rectangle or its explicit point size.
 std::set<std::uint32_t> fixedFields;
 auto* label=p.find_instance("RimeLabelElementData");if(!label)throw std::runtime_error("rules label absent");
 for(const auto* key:{"PointSize","Width","Height","FitWidthToContent","FitHeightToContent","HorizontalLayout","VerticalLayout","HorizontalAlignment","VerticalAlignment","WordWrap"})
  fixedFields.insert(p.document().fields.at(ui::require_field(*label->object,key).descriptor).nameHash);
 auto& links=ui::require_array(ui::require_field(*p.root()->object,"PropertyConnections").value,"notice links");
 std::erase_if(links,[&](auto& v){auto& link=ui::require_object(v,"link");const auto* ptr=sdk::ebx::pointer(ui::require_field(link,"Target").value);auto* target=ptr?sdk::ebx::resolve_internal(p.document(),*ptr):nullptr;
  if(!target || sdk::ebx::type_name(p.document(),*target)!="RimeLabelElementData")return false;
  const auto hash=std::uint32_t(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0));
  return fixedFields.contains(hash);
 });
 const auto a=ui::emit(name,p);
 const auto verified=sdk::ebx::read_document(a.bytes);
 for(const auto& i:verified.instances)if(i.object && (sdk::ebx::type_name(verified,i)=="RimeWidgetEntityData" || sdk::ebx::type_name(verified,i)=="RimeLabelElementData")){
  for(const auto [key,value]:{std::pair{"Width",960.},std::pair{"Height",128.}})
   if(sdk::ebx::number(ui::require_field(*i.object,key).value)!=value)throw std::runtime_error("serialized rules bounds differ");
  for(const auto* key:{"FitWidthToContent","FitHeightToContent"})
   if(sdk::ebx::boolean(ui::require_field(*i.object,key).value)!=false)throw std::runtime_error("serialized rules resize themselves");
 }
 resource.name=name;resource.assetGuid=a.partitionGuid;resource.payload=a.bytes;resource.added=true;
 project.resources.push_back(std::move(resource));
 project.metadata.version="1.0.10";projects::save_project(project,argv[3]);
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");
 reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS ten1088x448 intro panels; one960x128 fixed rules widget; all other resources retained\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
