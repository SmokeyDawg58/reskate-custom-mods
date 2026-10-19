#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/boxed_value.h"
#include <algorithm>
#include <iostream>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("editor-assets game input-project output-project");
 auto project=projects::load_project(argv[2]);const std::string prefix="UI/ReSkate/Throwdowns/";
 const auto clone=[&](std::string donor,std::string suffix,double width,double height,double points=0.,bool centered=false){
  auto found=std::ranges::find(project.resources,prefix+donor,&projects::ProjectResource::name);
  if(found==project.resources.end())throw std::runtime_error("donor absent: "+donor);
  auto resource=*found;auto doc=sdk::ebx::read_document(resource.payload);sdk::ebx::decode_boxed_values(doc,resource.payload);
  ebx::Partition part(doc);auto& d=part.document();auto* widget=part.find_instance("RimeWidgetEntityData");
  if(!widget)throw std::runtime_error("widget absent");
  const double oldw=*sdk::ebx::number(ui::require_field(*widget->object,"Width").value);
  const double oldh=*sdk::ebx::number(ui::require_field(*widget->object,"Height").value);
  for(auto& instance:d.instances)if(instance.object){
   const auto type=sdk::ebx::type_name(d,instance);
   if(type.starts_with("Rime"))for(const auto [key,oldv,newv]:{std::tuple{"Width",oldw,width},std::tuple{"Height",oldh,height}})
    if(auto* f=sdk::ebx::find_field(*instance.object,key))if(const auto value=sdk::ebx::number(f->value)){
     if(*value==oldv)f->value.data=newv;
     else if(type=="RimeLabelElementData" && std::string_view(key)=="Width" && *value==oldv-24.)f->value.data=width-24.;
    }
   if(type=="RimeLabelElementData"){
    if(points>0)ui::require_field(*instance.object,"PointSize").value.data=points;
    ui::require_field(*instance.object,"WordWrap").value.data=false;
    ui::require_field(*instance.object,"HorizontalAlignment").value.data=std::int64_t{centered?2:0};
   }
  }
  if(points>0){
   auto& links=ui::require_array(ui::require_field(*part.root()->object,"PropertyConnections").value,"links");
   std::erase_if(links,[&](auto& value){auto& link=ui::require_object(value,"link");
    const auto* pointer=sdk::ebx::pointer(ui::require_field(link,"Target").value);
    const auto* target=pointer?sdk::ebx::resolve_internal(d,*pointer):nullptr;
    return target && sdk::ebx::type_name(d,*target)=="RimeLabelElementData" &&
     std::uint32_t(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0))==1834692108U;
   });
  }
  const auto name=prefix+suffix;ui::require_field(*part.root()->object,"Name").value.data=name;
  const auto out=ui::emit(name,part);resource.name=name;resource.assetGuid=out.partitionGuid;resource.payload=out.bytes;resource.added=true;
  if(std::ranges::find(project.resources,name,&projects::ProjectResource::name)!=project.resources.end())throw std::runtime_error("new resource collision");
  // Read the serialized result, not just the author model.
  const auto verified=sdk::ebx::read_document(out.bytes);
  for(const auto& i:verified.instances)if(i.object && sdk::ebx::type_name(verified,i)=="RimeWidgetEntityData")
   if(*sdk::ebx::number(sdk::ebx::find_field(*i.object,"Width")->value)!=width ||
      *sdk::ebx::number(sdk::ebx::find_field(*i.object,"Height")->value)!=height)throw std::runtime_error("serialized bounds differ");
  project.resources.push_back(std::move(resource));std::cout<<name<<" "<<width<<"x"<<height<<" point="<<points<<'\n';
 };
 clone("EditorPanel_Widget","EditorControlsPanel_Widget",704.,744.);
 clone("ConfirmationPanel_Widget","EditorInfoPanel_Widget",600.,492.);
 clone("EditorCaption_Widget","EditorControlCaption_Widget",524.,56.,38.);
 clone("Heading_952_144_Widget","ResultsHeading_Widget",784.,144.,78.);
 clone("Heading_952_144_Widget","RaceResultsHeading_Widget",608.,144.,78.);
 for(const auto* key:{"oneup","jam","meat","race","domination","graffiti","skate","tag","infection","hide"})
  clone(std::string("HUD_")+key+"_Title_1312_Widget",std::string("IntroPanel_")+key+"_Widget",1088.,324.);
 project.metadata.version="1.0.8";projects::save_project(project,argv[3]);
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS original assets preserved; dedicated bounded editor, intro and results resources added\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
