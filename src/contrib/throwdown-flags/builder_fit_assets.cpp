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
 if(argc!=5)throw std::runtime_error("builder-fit game input-project original-ground-project output-project");
 auto project=projects::load_project(argv[2]);const auto ground=projects::load_project(argv[3]);
 const auto find=[&](auto& p,const std::string& name)->auto&{auto it=std::ranges::find(p.resources,name,&projects::ProjectResource::name);if(it==p.resources.end())throw std::runtime_error("missing "+name);return *it;};
 const std::string flag="World/ReSkate/Throwdowns/Flags/race_Placement";
 auto& mainFlag=find(project,flag);const auto& original=find(ground,flag);
 if(mainFlag.assetGuid!=original.assetGuid)throw std::runtime_error("main flag identity differs");
 mainFlag.payload=original.payload; // Restore exact original grounded placement validation.
 const std::string panel="UI/ReSkate/Throwdowns/EditorControlsPanel_Widget";
 auto& resource=find(project,panel);auto doc=sdk::ebx::read_document(resource.payload);sdk::ebx::decode_boxed_values(doc,resource.payload);
 ebx::Partition part(doc,false);unsigned changed{};
 for(auto& i:part.document().instances)if(i.object && sdk::ebx::type_name(part.document(),i).starts_with("Rime")){
  const auto type=sdk::ebx::type_name(part.document(),i);
  if(auto* f=sdk::ebx::find_field(*i.object,"Height"))if(sdk::ebx::number(f->value).value_or(-1)==1008.){f->value.data=1000.;++changed;}
  for(const auto* name:{"FitHeightToContent","FitWidthToContent"})if(auto* f=sdk::ebx::find_field(*i.object,name))f->value.data=false;
  if(type=="RimeWidgetEntityData" || type=="RimeContainerElementData" || type=="RimeRectangleElementData"){
   std::cout<<type;
   for(const auto* key:{"Width","Height","FitHeightToContent"})if(const auto* f=sdk::ebx::find_field(*i.object,key))std::cout<<" "<<key<<"="<<sdk::ebx::number(f->value).value_or(-1);
   std::cout<<'\n';
  }
 }
 if(changed<3)throw std::runtime_error("panel layers differ");
 auto out=ui::emit(panel,part);if(out.partitionGuid!=resource.assetGuid)throw std::runtime_error("panel identity changed");resource.payload=out.bytes;
 auto title=find(project,"UI/ReSkate/Throwdowns/EditorControlCaption_Widget");
 auto td=sdk::ebx::read_document(title.payload);sdk::ebx::decode_boxed_values(td,title.payload);ebx::Partition tp(td);
 for(auto& i:tp.document().instances)if(i.object){
  const auto type=sdk::ebx::type_name(tp.document(),i);
  if(type.starts_with("Rime")){
   if(auto* f=sdk::ebx::find_field(*i.object,"Width"))if(sdk::ebx::number(f->value).value_or(-1)==524.)f->value.data=656.;
   if(auto* f=sdk::ebx::find_field(*i.object,"Height"))if(sdk::ebx::number(f->value).value_or(-1)==56.)f->value.data=64.;
   for(const auto* name:{"FitHeightToContent","FitWidthToContent"})if(auto* f=sdk::ebx::find_field(*i.object,name))f->value.data=false;
  }
  if(type=="RimeLabelElementData"){ui::require_field(*i.object,"PointSize").value.data=42.;ui::require_field(*i.object,"HorizontalAlignment").value.data=std::int64_t{0};}
 }
 const std::string name="UI/ReSkate/Throwdowns/EditorBuilderTitle_Widget";
 ui::require_field(*tp.root()->object,"Name").value.data=name;out=ui::emit(name,tp);
 title.name=name;title.payload=out.bytes;title.assetGuid=out.partitionGuid;title.added=true;
 project.resources.push_back(std::move(title));
 project.metadata.version="1.0.11";projects::save_project(project,argv[4]);
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);auto mod=std::filesystem::path(argv[4]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 // Reopen saved project; verify dimensions and grounded flag byte-for-byte.
 const auto verified=projects::load_project(argv[4]);
 if(find(verified,flag).payload!=original.payload)throw std::runtime_error("grounded flag restoration differs");
 for(const auto [asset,w,h]:{std::tuple{panel,704.,1000.},std::tuple{name,656.,64.}}){
  const auto d=sdk::ebx::read_document(find(verified,asset).payload);bool checked{};
  for(const auto& i:d.instances)if(i.object && sdk::ebx::type_name(d,i)=="RimeWidgetEntityData"){
   if(sdk::ebx::number(ui::require_field(*i.object,"Width").value)!=w || sdk::ebx::number(ui::require_field(*i.object,"Height").value)!=h)throw std::runtime_error("serialized bounds differ");checked=true;
  }if(!checked)throw std::runtime_error("serialized widget absent");
 }
 std::cout<<"PASS builder panel704x1000 title656x64; original grounded main flag restored exactly\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
