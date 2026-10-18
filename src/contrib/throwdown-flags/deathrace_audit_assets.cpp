#include "modding/maps/ui/ui_assets.h"
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/textures/png_codec.h"
#include "sdk/resources/texture_resource.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/boxed_value.h"
#include <algorithm>
#include <iostream>
#include <functional>
#include <cstring>
namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
namespace tex=reskate::modding::textures;using Resource=projects::ProjectResource;
const sdk::ebx::Document* inspected{};
void show(const sdk::ebx::Value& value,const std::string& path) {
 if(const auto n=sdk::ebx::number(value))std::cout<<path<<'='<<*n<<'\n';
 if(const auto t=sdk::ebx::text(value))std::cout<<path<<"='"<<*t<<"'\n";
 if(const auto* b=std::get_if<sdk::ebx::BoxedReference>(&value.data);b && inspected)
  if(const auto* box=sdk::ebx::resolve_boxed(*inspected,*b);box && box->decoded){std::cout<<path<<".boxbytes="<<box->rawBytes.size()<<'\n';show(*box->decoded,path+".box");}
 if(const auto* p=std::get_if<std::shared_ptr<sdk::ebx::Object>>(&value.data);p && *p)
  for(const auto& f:(*p)->fields)show(f.value,path+"."+f.name);
 if(const auto* a=std::get_if<sdk::ebx::Value::Array>(&value.data);a)
  for(unsigned i=0;i<a->size();++i)show((*a)[i],path+"["+std::to_string(i)+"]");
}
int main(int argc,char** argv){try{
 std::cout<<std::unitbuf;if(argc!=5)throw std::runtime_error("audit-assets game input-project finish.png output-project (or inspect)");
 auto project=projects::load_project(argv[2]);
 if(std::string_view(argv[4])=="inspect") {
  for(const auto& r:project.resources)if(r.kind==projects::ResourceKind::ebx && r.name.find("/Flags/")!=std::string::npos &&
    (r.name.find("race_")!=std::string::npos || r.name.ends_with("_Registration"))) {
   std::cout<<"RESOURCE "<<r.name<<'\n';
   auto d=sdk::ebx::read_document(r.payload);sdk::ebx::decode_boxed_values(d,r.payload);
   inspected=&d;
   for(const auto& i:d.instances)if(i.object && r.name.find("race_")!=std::string::npos) {
    const auto type=sdk::ebx::type_name(d,i);
    std::cout<<" INSTANCE "<<type<<'\n';
    for(const auto& f:i.object->fields){std::cout<<" FIELD "<<f.name<<'\n';show(f.value,std::string(type)+"."+f.name);}
   }
  }
  return 0;
 }
 const auto db=sdk::assets::FrostbiteAssetManager::load_sync(argv[1]);
 // Only the private flag mesh's LocalTransform changes. Do not scale the
 // activity's bounds, spawn/teleport marker, HUD icon or stock flag prefab.
 unsigned raised{};
 for(auto& r:project.resources)if(r.kind==projects::ResourceKind::ebx &&
   (r.name=="World/ReSkate/Throwdowns/Flags/race_Registration" || r.name=="World/ReSkate/Throwdowns/Flags/race_Placement")) {
  auto d=sdk::ebx::read_document(r.payload);sdk::ebx::decode_boxed_values(d,r.payload);ebx::Partition p(d,false);
  auto& doc=p.document();unsigned changed{};
  const auto visit=[&](sdk::ebx::Object& root,auto&& callback){
   std::function<void(sdk::ebx::Value&)> walk;
   walk=[&](sdk::ebx::Value& v){
    if(auto* object=std::get_if<std::shared_ptr<sdk::ebx::Object>>(&v.data);object && *object){callback(**object);for(auto& f:(*object)->fields)walk(f.value);}
    if(auto* rows=std::get_if<sdk::ebx::Value::Array>(&v.data))for(auto& row:*rows)walk(row);
   };
   callback(root);for(auto& f:root.fields)walk(f.value);
  };
  for(auto& instance:doc.instances)if(instance.object && sdk::ebx::type_name(doc,instance)=="EcsVariantEdit") {
   bool mesh{};
   visit(*instance.object,[&](auto& object){if(const auto* key=sdk::ebx::find_field(object,"Key");key)if(const auto* text=sdk::ebx::text(key->value);text && *text=="EcsMeshComponentAsset")mesh=true;});
   if(!mesh)continue;
   visit(*instance.object,[&](auto& object){
    const auto* key=sdk::ebx::find_field(object,"Key");if(!key)return;
    const auto* text=sdk::ebx::text(key->value);if(!text || *text!="LocalTransform")return;
    const auto ref=std::get<sdk::ebx::BoxedReference>(ui::require_field(object,"Value").value.data);
    auto* box=sdk::ebx::resolve_boxed(doc,ref);if(!box || !box->decoded || box->rawBytes.size()!=48)throw std::runtime_error("flag LocalTransform ABI differs");
    auto& transform=ui::require_object(*box->decoded,"flag transform");auto& scale=ui::require_object(ui::require_field(transform,"scale").value,"flag scale");
    auto& y=ui::require_field(scale,"y").value;float raw{};std::memcpy(&raw,box->rawBytes.data()+4,4);
    if(*sdk::ebx::number(y)!=1. || raw!=1.f)throw std::runtime_error("private flag height donor differs");
    constexpr float height=1.4f;y.data=double(height);std::memcpy(box->rawBytes.data()+4,&height,4);++changed;
   });
  }
  if(changed!=1)throw std::runtime_error("private flag mesh transform not unique: "+r.name+" count="+std::to_string(changed));
  const auto out=ui::emit(r.name,p);if(out.partitionGuid!=r.assetGuid)throw std::runtime_error("flag identity changed");
  r.payload=out.bytes;++raised;std::cout<<"raised_flag_mesh="<<r.name<<" height=140%\n";
 }
 if(raised!=2)throw std::runtime_error("registration and placement must share the raised flag height");
 const auto donor=std::ranges::find(project.resources,std::string("UI/ReSkate/Throwdowns/img_GateCompass"),&Resource::name);
 if(donor==project.resources.end())throw std::runtime_error("current compass donor absent");
 const auto bundles=donor->bundles;const auto superbundles=donor->superbundles;
 ui::UiAssets assets;
 const auto finish_identity=ui::clone_rime_texture(db,"UI/Textures/Icons/Activities/Throwdowns/FullColor/img_Icon_Throwdown_Color_Full_1024",
    "UI/ReSkate/Throwdowns/img_FinishGateCompass",tex::load_artwork_image(argv[3]),assets);
 const auto add=[&](projects::ResourceKind kind,const std::string& name,std::vector<std::byte> bytes)->Resource& {
  if(std::ranges::any_of(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;}))throw std::runtime_error("new asset collision");
  Resource r;r.kind=kind;r.name=name;r.payload=std::move(bytes);r.added=true;r.bundles=bundles;r.superbundles=superbundles;
  project.resources.push_back(std::move(r));return project.resources.back();
 };
 for(auto& p:assets.partitions)add(projects::ResourceKind::ebx,p.name,std::move(p.bytes)).assetGuid=p.partitionGuid;
 for(auto& r:assets.resources){auto& out=add(projects::ResourceKind::resource,r.name,std::move(r.payload));out.resourceId=r.id;out.resourceType=r.type;out.resourceMeta=std::move(r.metadata);}
 for(auto& t:assets.textures){
  add(projects::ResourceKind::ebx,t.name,std::move(t.ebx)).assetGuid=t.fileGuid;
  auto& r=add(projects::ResourceKind::resource,t.name,std::move(t.texture.resource));r.resourceId=t.resourceId;r.resourceType=sdk::resources::textureResourceType;r.resourceMeta=std::move(t.texture.resourceMeta);
  auto& c=add(projects::ResourceKind::chunk,t.chunkId.string(),std::move(t.texture.chunk));c.chunkId=t.chunkId;c.logicalSize=static_cast<std::uint32_t>(c.payload.size());c.firstMip=0;c.h32=static_cast<std::int32_t>(tex::texture_name_hash(t.name));
 }
 project.metadata.version="1.0.7";projects::save_project(project,argv[4]);
 auto mod=std::filesystem::path(argv[4]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS finish compass texture added; existing resources retained\n";
}catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}}
