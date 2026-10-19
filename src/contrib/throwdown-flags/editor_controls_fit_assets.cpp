#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/boxed_value.h"
#include <algorithm>
#include <cmath>
#include <set>
#include <cstring>
#include <iostream>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("controls-fit game input-project output-project");
 auto project=projects::load_project(argv[2]);
 const std::string name="UI/ReSkate/Throwdowns/EditorControlsPanel_Widget";
 auto found=std::ranges::find(project.resources,name,&projects::ProjectResource::name);
 if(found==project.resources.end())throw std::runtime_error("controls panel absent");
 auto doc=sdk::ebx::read_document(found->payload);sdk::ebx::decode_boxed_values(doc,found->payload);
 ebx::Partition part(doc,false);auto& d=part.document();unsigned changed{};
 for(auto& instance:d.instances)if(instance.object && sdk::ebx::type_name(d,instance).starts_with("Rime"))
  if(auto* f=sdk::ebx::find_field(*instance.object,"Height"))
   if(sdk::ebx::number(f->value).value_or(-1)==744.){f->value.data=1008.;++changed;}
 if(changed<3)throw std::runtime_error("expected widget/container/fill bounds");
 auto out=ui::emit(name,part);found->payload=out.bytes;
 // Existing asset identity and width must survive the in-place height repair.
 if(out.partitionGuid!=found->assetGuid)throw std::runtime_error("panel identity changed");
 const auto verified=sdk::ebx::read_document(found->payload);
 for(const auto& instance:verified.instances)if(instance.object && sdk::ebx::type_name(verified,instance)=="RimeWidgetEntityData")
  if(sdk::ebx::number(sdk::ebx::find_field(*instance.object,"Width")->value)!=704. ||
     sdk::ebx::number(sdk::ebx::find_field(*instance.object,"Height")->value)!=1008.)throw std::runtime_error("serialized panel bounds differ");
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 unsigned intros{};
 for(auto& r:project.resources)if(r.name.starts_with("UI/ReSkate/Throwdowns/IntroPanel_")){
  auto doc=sdk::ebx::read_document(r.payload);sdk::ebx::decode_boxed_values(doc,r.payload);ebx::Partition p(doc,false);
  for(auto& i:p.document().instances)if(i.object && sdk::ebx::type_name(p.document(),i).starts_with("Rime"))
   if(auto* f=sdk::ebx::find_field(*i.object,"Height"))if(sdk::ebx::number(f->value).value_or(-1)==324.)f->value.data=304.;
  const auto a=ui::emit(r.name,p);if(a.partitionGuid!=r.assetGuid)throw std::runtime_error("intro identity changed");r.payload=a.bytes;++intros;
 }
 if(intros!=10)throw std::runtime_error("expected ten intro panel themes");
 const auto original=[&](const char* name){const auto bytes=db.read(ui::require_asset(db,name));auto d=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(d,bytes);return d;};
 const auto effect=original("effects/ui/effectblueprints/epb_decal_beacon_a");
 const auto beacon=original("effects/ui/prefabs/spf_ui_beacon_activity");
 const auto linear=[](double c){return c<=.04045?c/12.92:std::pow((c+.055)/1.055,2.4);};
 struct Theme {const char* key;std::uint32_t rgba;};
 constexpr Theme themes[]{{"oneup",0xffef8b22U},{"jam",0xffab4fffU},{"meat",0xff218affU},{"race",0xff3434ffU},{"domination",0xffff558cU},{"graffiti",0xff9fd929U},{"tag",0xff31d3ffU},{"infection",0xff24d675U},{"hide",0xffffbd39U},{"skate",0xffd9df00U}};
 const auto add=[&](const std::string& name,ebx::Partition& p,const projects::ProjectResource& source){
  ui::require_field(*p.root()->object,"Name").value.data=name;const auto a=ui::emit(name,p);
  projects::ProjectResource r;r.kind=projects::ResourceKind::ebx;r.name=name;r.payload=a.bytes;r.assetGuid=a.partitionGuid;r.added=true;r.bundles=source.bundles;r.superbundles=source.superbundles;
  project.resources.push_back(std::move(r));return a;
 };
 for(const auto& theme:themes){
  const std::string stem="World/ReSkate/Throwdowns/Flags/"+std::string(theme.key);
  const auto index=std::ranges::find(project.resources,stem+"_Registration",&projects::ProjectResource::name)-project.resources.begin();
  if(index==project.resources.size())throw std::runtime_error("theme registration missing");
  const auto source=project.resources[index]; // additions can invalidate vector references
  const std::array<double,3> rgb{linear(double(theme.rgba&255)/255.),linear(double((theme.rgba>>8)&255)/255.),linear(double((theme.rgba>>16)&255)/255.)};
  ebx::Partition ep(effect);std::set<std::uint32_t> pins;
  auto* interface=ep.find_instance("InterfaceDescriptorData");if(!interface)throw std::runtime_error("beacon interface missing");
  for(auto& v:ui::require_array(ui::require_field(*interface->object,"Fields").value,"beacon interface fields")){
   auto& f=ui::require_object(v,"interface field");auto* ref=std::get_if<sdk::ebx::BoxedReference>(&ui::require_field(f,"BoxedValue").value.data);
   auto* box=ref?sdk::ebx::resolve_boxed(ep.document(),*ref):nullptr;if(!box || !box->decoded)continue;
   const auto* check=sdk::ebx::object(*box->decoded);if(!check || !sdk::ebx::find_field(*check,"x") || !sdk::ebx::find_field(*check,"y") || !sdk::ebx::find_field(*check,"z") || sdk::ebx::find_field(*check,"w"))continue;
   auto& color=ui::require_object(*box->decoded,"beacon color");
   const auto gain=std::max({sdk::ebx::number(ui::require_field(color,"x").value).value_or(0),sdk::ebx::number(ui::require_field(color,"y").value).value_or(0),sdk::ebx::number(ui::require_field(color,"z").value).value_or(0)});
   if(gain<=0 || gain>1.01 || box->rawBytes.size()<12)throw std::runtime_error("beacon RGB default differs");
   std::array<float,3> raw{};for(unsigned c=0;c<3;++c){raw[c]=float(rgb[c]*gain);ui::require_field(color,std::array{"x","y","z"}[c]).value.data=double(raw[c]);}
   std::memcpy(box->rawBytes.data(),raw.data(),12);pins.insert(std::uint32_t(sdk::ebx::integer(ui::require_field(f,"Id").value).value_or(0)));
  }
  if(pins.size()!=3)throw std::runtime_error("expected three beacon RGB inputs");
  const auto authoredEffect=add(stem+"_BeaconEffect",ep,source);
  ebx::Partition sp(beacon);
  for(auto& i:sp.document().imports)if(i.fileGuid==effect.fileGuid)i={authoredEffect.partitionGuid,authoredEffect.rootGuid};
  auto& links=ui::require_array(ui::require_field(*sp.root()->object,"PropertyConnections").value,"beacon property links");unsigned disconnected{};
  std::erase_if(links,[&](auto& v){auto& link=ui::require_object(v,"link");const auto* ptr=sdk::ebx::pointer(ui::require_field(link,"Target").value);auto* target=ptr?sdk::ebx::resolve_internal(sp.document(),*ptr):nullptr;
   if(!target)return false;const auto type=sdk::ebx::type_name(sp.document(),*target);const auto pin=std::uint32_t(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0));
   const bool remove=(type=="EffectReferenceObjectData" && pins.contains(pin)) || type=="PbrSphereLightEntityData";if(remove)++disconnected;return remove;});
  for(auto& i:sp.document().instances)if(i.object && sdk::ebx::type_name(sp.document(),i)=="PbrSphereLightEntityData"){
   auto& color=ui::require_object(ui::require_field(*i.object,"Color").value,"beacon light");for(unsigned c=0;c<3;++c)ui::require_field(color,std::array{"x","y","z"}[c]).value.data=rgb[c];
  }
  if(disconnected<3)throw std::runtime_error("beacon palette binding shape differs");
  const auto authoredBeacon=add(stem+"_Beacon",sp,source);
  auto rd=sdk::ebx::read_document(source.payload);sdk::ebx::decode_boxed_values(rd,source.payload);ebx::Partition registration(rd,false);unsigned rewired{};
  for(auto& i:registration.document().imports)if(i.fileGuid==beacon.fileGuid){i={authoredBeacon.partitionGuid,authoredBeacon.rootGuid};++rewired;}
  if(rewired!=1)throw std::runtime_error("registration beacon import differs");
  const auto a=ui::emit(source.name,registration);if(a.partitionGuid!=source.assetGuid)throw std::runtime_error("flag registration identity changed");project.resources[index].payload=a.bytes;
  std::cout<<"themed_beacon="<<theme.key<<" colour_inputs=3 disconnected_stock_links="<<disconnected<<'\n';
 }
 project.metadata.version="1.0.9";projects::save_project(project,argv[3]);
 auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");
 reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS controls704x1008; intro panels1088x304; ten independent themed beacon/ring/light effects\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
