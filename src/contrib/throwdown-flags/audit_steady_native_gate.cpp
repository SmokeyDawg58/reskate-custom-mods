#include "modding/projects/project.h"
#include "modding/maps/ui/ui_assets.h"
#include "modding/materials/native/shader_resources.h"
#include "sdk/ebx/ebx_writer.h"
#include "sdk/ebx/query.h"
#include <algorithm>
#include <cstring>
#include <iostream>
namespace p=reskate::modding::projects;namespace s=reskate::sdk;namespace u=reskate::modding::maps::ui;namespace n=reskate::modding::materials::native;
void check(bool b,const char* message){if(!b)throw std::runtime_error(message);}
template<class T>T get(const std::vector<std::byte>&b,std::size_t at){check(at+sizeof(T)<=b.size(),"bounded resource read");T v{};std::memcpy(&v,b.data()+at,sizeof v);return v;}
int main(int argc,char**argv){try{
 check(argc==3,"before-project after-project");const auto before=p::load_project(argv[1]),after=p::load_project(argv[2]);
 check(after.resources.size()==before.resources.size()+3,"private resource count differs");
 const auto find=[&](const auto& project,std::string_view name)->const p::ProjectResource&{auto it=std::ranges::find(project.resources,name,&p::ProjectResource::name);check(it!=project.resources.end(),"required resource missing");return *it;};
 const auto& graph=find(after,"World/ReSkate/Deathrace/Skate2NativeFX/SteadyGate_Graph");const auto d=s::ebx::read_document(graph.payload);
 const auto& originalGraph=find(before,"World/ReSkate/Deathrace/Skate2NativeFX/MeshLoop_Graph");const auto original=s::ebx::read_document(originalGraph.payload);
 unsigned changes{},meshPreserved{},roots{};
 for(const auto& old:before.resources){auto current=std::ranges::find_if(after.resources,[&](const auto& r){return r.name==old.name && r.kind==old.kind && r.resourceId==old.resourceId && r.chunkId==old.chunkId;});check(current!=after.resources.end(),"original resource identity missing");const auto& now=*current;if(old==now){if(old.name.starts_with("World/ReSkate/Deathrace/Skate2NativeFX/"))++meshPreserved;continue;}
  if(old.kind!=p::ResourceKind::ebx || !old.name.starts_with("World/ReSkate/Deathrace/"))throw std::runtime_error("shared resource changed: "+old.name);
  auto restored=s::ebx::read_document(now.payload);unsigned refs{};
  for(auto& ref:restored.imports)if(ref.fileGuid==d.fileGuid){check(ref.classGuid==d.root()->instanceGuid,"wrong private graph instance");ref.fileGuid=original.fileGuid;ref.classGuid=original.root()->instanceGuid;++refs;}
  check(refs==1,"graph import not unique");const auto oldDoc=s::ebx::read_document(old.payload);
  check(restored.fileGuid==oldDoc.fileGuid && restored.root()->instanceGuid==oldDoc.root()->instanceGuid,"gate identity changed");
  const auto entity=std::ranges::find_if(restored.instances,[&](const auto& i){return s::ebx::type_name(restored,i)=="EffectEntityData";});check(entity!=restored.instances.end(),"gate entity absent");
  auto& components=u::require_array(u::require_field(*entity->object,"Components").value,"components");check(components.size()==1,"inner gate emitters remain");
  const auto ptr=u::require_pointer(components[0],"outer component");check(ptr.kind==s::ebx::PointerKind::internal && ptr.index>=0 && std::size_t(ptr.index)<restored.instances.size(),"outer reference invalid");
  const auto& outer=restored.instances[ptr.index];check(s::ebx::type_name(restored,outer)=="EmitterGraphEntityData","outer is not native mesh emitter");
  check(*s::ebx::number(u::require_field(*outer.object,"StartDelay").value)==0.,"gate outer has staggered delay");
  check(std::ranges::count_if(restored.instances,[&](const auto& i){return s::ebx::type_name(restored,i)=="EmitterGraphEntityData";})==1,"orphan inner emitters remain");
  auto metadata=now;metadata.payload=old.payload;check(metadata==old,"effect metadata or bundles changed");++changes;++roots;
 }
 check(changes==3 && roots==3 && meshPreserved==7,"wrong effect or mesh scope");
 auto restored=d;restored.fileGuid=original.fileGuid;check(restored.instances.size()==original.instances.size(),"graph instances changed");
 for(unsigned i=0;i<restored.instances.size();++i){restored.instances[i].instanceGuid=original.instances[i].instanceGuid;
  if(auto* f=s::ebx::find_field(*restored.instances[i].object,"Flags")){const auto* old=s::ebx::find_field(*original.instances[i].object,"Flags");check(old,"missing original identity flags");f->value=old->value;}}
 auto& root=*restored.root()->object;u::require_field(root,"Name").value=u::require_field(*original.root()->object,"Name").value;
 check(*s::ebx::number(u::require_field(root,"MinScreenArea").value)==0.,"gesture screen-area cull retained");
 u::require_field(root,"MinScreenArea").value=u::require_field(*original.root()->object,"MinScreenArea").value;
 const auto sim=std::get<s::ebx::ResourceReference>(u::require_field(root,"SimulateComputeShaderResource").value.data).id;
 u::require_field(root,"SimulateComputeShaderResource").value=u::require_field(*original.root()->object,"SimulateComputeShaderResource").value;
 check(s::ebx::write_document(restored)==s::ebx::write_document(original),"graph changed beyond private name, simulation and screen-area threshold");
 const auto& compute=find(after,"World/ReSkate/Deathrace/Skate2NativeFX/SteadyGate_Sim");check(compute.resourceId==sim,"private simulation not connected");
 const auto& oldCompute=find(before,"World/ReSkate/Deathrace/Skate2NativeFX/MeshLoop_Sim");auto restoredCompute=compute.payload;std::string shader;unsigned connected{};
 const auto oldGuid=s::io::Guid::parse("fbd63449-666c-4376-b0fc-b9faaa31ef60");
 for(const auto& code:after.resources)if(code.kind==p::ResourceKind::resource && code.resourceType==n::bytecodeType && code.name.starts_with("shaders/bytecode/") && std::ranges::find(before.resources,code.name,&p::ProjectResource::name)==before.resources.end()){
  const auto guid=s::io::Guid::parse(std::string_view(code.name).substr(17));unsigned matches{};
  for(std::size_t at=0;at+16<=restoredCompute.size();++at)if(std::equal(guid.bytes.begin(),guid.bytes.end(),restoredCompute.begin()+at)){std::copy(oldGuid.bytes.begin(),oldGuid.bytes.end(),restoredCompute.begin()+at);++matches;at+=15;}
  check(matches==1,"private bytecode not connected exactly once");const auto container=n::read_shader_container(code.payload);check(container.dxil && container.bytes.size()==8040,"validated private shader differs");
  const auto data=get<std::uint32_t>(code.resourceMeta,0),relocs=get<std::uint32_t>(code.resourceMeta,4);
  check(data+relocs==code.payload.size() && get<std::uint32_t>(code.payload,16)==data && get<std::uint32_t>(code.payload,12)==data-36 && get<std::uint32_t>(code.payload,136)==container.bytes.size(),"private wrapper extents invalid");
  for(std::size_t at=data;at<code.payload.size();at+=4){auto pointer=get<std::uint32_t>(code.payload,at);check(pointer+8<=data && get<std::uint64_t>(code.payload,pointer)<=data,"native relocation out of bounds");}
  shader=guid.string();++connected;
 }
 check(connected==1 && restoredCompute==oldCompute.payload && compute.resourceMeta==oldCompute.resourceMeta,"compute interface changed beyond bytecode GUID");
 check(graph.bundles==compute.bundles,"private graph and compute ownership differs");
 for(const auto* name:{"World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate","World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish","World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide"})for(const auto& bundle:find(after,name).bundles)
  check(std::ranges::find(graph.bundles,bundle)!=graph.bundles.end(),"private shader missing an owning gate/arrow bundle");
 std::cout<<"{\"result\":\"PASS\",\"unchanged_original_resources\":"<<before.resources.size()-changes<<",\"changed_private_effects\":3,\"added_private_resources\":3,\"native_mesh_resources_preserved\":"<<meshPreserved<<",\"private_graph_only_expected_fields\":true,\"compute_binding_metadata_preserved\":true,\"native_wrapper_relocations_valid\":true,\"private_shader\":\""<<shader<<"\",\"live_validation\":\"PENDING\"}\n";
}catch(const std::exception&ex){std::cerr<<ex.what()<<"\n";return 1;}}

