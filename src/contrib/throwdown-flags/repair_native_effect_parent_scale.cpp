// Offline repair: keep the stock animation but inherit the native world parent's
// scale in a private simulation shader. Never modifies a stock shared resource.
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/materials/native/shader_resources.h"
#include "sdk/ebx/query.h"
#include "sdk/ebx/ebx_writer.h"
#include <algorithm>
#include <cstring>
#include <fstream>
#include <iostream>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;namespace u=reskate::modding::maps::ui;
namespace e=reskate::modding::maps::ebx;namespace n=reskate::modding::materials::native;
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
template<class T>T get(const std::vector<std::byte>&b,std::size_t at){need(at+sizeof(T)<=b.size(),"read outside resource");T v{};std::memcpy(&v,b.data()+at,sizeof v);return v;}
template<class T>void put(std::vector<std::byte>&b,std::size_t at,T v){need(at+sizeof(T)<=b.size(),"write outside resource");std::memcpy(b.data()+at,&v,sizeof v);}
int main(int argc,char**argv){try{
 need(argc==5,"game-root existing-project validated-private-sim.dxbc output-project");std::cout<<std::unitbuf;
 auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=p::load_project(argv[2]);const auto before=project;
 const auto find=[&](std::string_view name){auto i=std::ranges::find(project.resources,name,&p::ProjectResource::name);need(i!=project.resources.end(),"source resource missing");return *i;};
 auto graph=find("effects/gestures/emittergraphs/eg_mesh_crosssection_a_loop");
 const auto oldGraph=s::ebx::read_document(graph.payload);e::Partition privateGraph(oldGraph);
 const std::string prefix="World/ReSkate/Deathrace/Skate2NativeFX/";
 graph.name=prefix+"MeshLoop_Graph";
 u::require_field(*privateGraph.root()->object,"Name").value.data=graph.name;
 // Large route markers should use their normal native bounds/depth tests,
 // without the gesture donor's small-screen-area disappearance threshold.
 u::require_field(*privateGraph.root()->object,"MinScreenArea").value.data=0.;
 auto compute=find("effects/gestures/emittergraphs/eg_mesh_crosssection_a_loop_sim");
 const auto oldSim=compute.resourceId;compute.resourceId=e::random_resource_id();compute.name=prefix+"MeshLoop_Sim";
 const auto oldCode=s::io::Guid::parse("87d5c37c-1d7d-86db-28da-69eafbe34667"),newCode=e::random_guid();
 auto bytecode=find("shaders/bytecode/"+oldCode.string());
 std::size_t replacements{};
 for(std::size_t i=0;i+16<=compute.payload.size();++i)if(std::equal(oldCode.bytes.begin(),oldCode.bytes.end(),compute.payload.begin()+i)){
  std::copy(newCode.bytes.begin(),newCode.bytes.end(),compute.payload.begin()+i);++replacements;i+=15;
 }
 need(replacements==1,"compute resource must reference its bytecode exactly once");
 auto& simField=u::require_field(*privateGraph.root()->object,"SimulateComputeShaderResource").value;
 need(std::get<s::ebx::ResourceReference>(simField.data).id==oldSim,"wrong native simulation resource");simField.data=s::ebx::ResourceReference{compute.resourceId};
 auto emitted=u::emit(graph.name,privateGraph);graph.assetGuid=emitted.partitionGuid;graph.payload=std::move(emitted.bytes);
 const auto oldContainer=n::read_shader_container(bytecode.payload);
 const auto at=std::search(bytecode.payload.begin(),bytecode.payload.end(),oldContainer.bytes.begin(),oldContainer.bytes.begin()+32)-bytecode.payload.begin();
 need(at==140 && get<std::uint32_t>(bytecode.payload,136)==oldContainer.bytes.size(),"native simulation wrapper differs");
 const auto oldData=get<std::uint32_t>(bytecode.resourceMeta,0),relocations=get<std::uint32_t>(bytecode.resourceMeta,4);
 need(oldData+relocations==bytecode.payload.size() && relocations==8 && get<std::uint32_t>(bytecode.payload,16)==oldData && get<std::uint32_t>(bytecode.payload,12)==oldData-36,"native wrapper extents differ");
 std::ifstream f(argv[3],std::ios::binary);std::vector<char> code((std::istreambuf_iterator<char>(f)),{});need(code.size()>32 && std::memcmp(code.data(),"DXBC",4)==0,"validated shader container missing");
 const auto newData=std::uint32_t((at+code.size()+7)&~std::size_t(7));
 std::vector<std::byte> payload(newData+relocations);
 std::copy_n(bytecode.payload.begin(),at,payload.begin());std::memcpy(payload.data()+at,code.data(),code.size());
 std::copy(bytecode.payload.begin()+oldData,bytecode.payload.end(),payload.begin()+newData);
 put(payload,12,newData-36);put(payload,16,newData);put(payload,136,std::uint32_t(code.size()));
 bytecode.payload=std::move(payload);put(bytecode.resourceMeta,0,newData);bytecode.name="shaders/bytecode/"+newCode.string();bytecode.resourceId=e::random_resource_id();
 need(n::read_shader_container(bytecode.payload).bytes.size()==code.size(),"private container roundtrip failed");
 auto bundles=find("World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate").bundles;
 for(const auto* name:{"World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish","World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide","World/ReSkate/Deathrace/FloatingArrow_AT"})for(const auto& bundle:find(name).bundles)
  if(std::ranges::find(bundles,bundle)==bundles.end())bundles.push_back(bundle);
 for(auto* resource:{&graph,&compute,&bytecode}){resource->added=true;resource->bundles=bundles;project.resources.push_back(*resource);}
 unsigned repaired{};
 for(auto& resource:project.resources){
  if(resource.kind!=p::ResourceKind::ebx || (resource.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate" && resource.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish" && resource.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide" && resource.name!="World/ReSkate/Deathrace/FloatingArrow_AT"))continue;
  auto doc=s::ebx::read_document(resource.payload);unsigned references{};
  for(auto& ref:doc.imports)if(ref.fileGuid==oldGraph.fileGuid){need(ref.classGuid==oldGraph.root()->instanceGuid,"unexpected donor graph child reference");ref.fileGuid=emitted.partitionGuid;ref.classGuid=emitted.rootGuid;++references;}
  need(references==1,"private effect graph import differs");resource.payload=s::ebx::write_document(doc);++repaired;
 }
 unsigned changed{};for(unsigned i=0;i<before.resources.size();++i)if(project.resources[i]!=before.resources[i]){need(project.resources[i].kind==p::ResourceKind::ebx && project.resources[i].name.starts_with("World/ReSkate/Deathrace/"),"unrelated existing resource changed");++changed;}
 need(repaired==4 && changed==4 && project.resources.size()==before.resources.size()+3,"unexpected repair scope");
 project.metadata.version="1.0.26-native-parent-scale-fix";p::save_project(project,argv[4]);auto mod=std::filesystem::path(argv[4]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"{\"result\":\"PASS\",\"changed_private_effects\":4,\"added_private_resources\":3,\"shared_resources_unchanged\":true,\"geometry_unchanged\":true,\"native_parent_pose\":true,\"shader_guid\":\""<<newCode.string()<<"\",\"compute_resource\":\""<<std::hex<<compute.resourceId<<"\",\"in_game_retest\":\"REQUIRED\"}\n";
}catch(const std::exception&ex){std::cerr<<ex.what()<<"\n";return 1;}}
