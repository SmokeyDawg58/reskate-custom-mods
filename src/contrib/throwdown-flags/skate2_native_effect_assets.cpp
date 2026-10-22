// Offline candidate author: native EffectBlueprint + looping GPU mesh emitters.
// Does not open, control or attach to the running game.
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/models/mesh_writer.h"
#include "modding/materials/shader_depot.h"
#include "modding/materials/shader_lookup.h"
#include "sdk/resources/mesh_set.h"
#include "sdk/ebx/boxed_value.h"
#include "sdk/ebx/query.h"
#include "sdk/assets/ebx_identity_resolver.h"
#include "../../Extension/UI/Overlay/deathrace_skate2_art.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;namespace m=reskate::modding::models;
namespace e=reskate::modding::maps::ebx;namespace u=reskate::modding::maps::ui;namespace mat=reskate::modding::materials;
using V=std::array<float,3>;using Guid=s::io::Guid;using Resource=p::ProjectResource;
void need(bool b,const char* why){if(!b)throw std::runtime_error(why);}
V add(V a,V b){for(unsigned k=0;k<3;++k)a[k]+=b[k];return a;}V mul(V a,float b){for(auto& x:a)x*=b;return a;}
V spline(std::span<const V> c,float t){const auto i=std::min<std::size_t>(std::size_t(t),c.size()-4);const float a=t-float(i);const std::array w{(1-a)*(1-a)*(1-a),4-6*a*a+3*a*a*a,1+3*a+3*a*a-3*a*a*a,a*a*a};V v{};for(unsigned k=0;k<4;++k)v=add(v,mul(c[i+k],w[k]/6));return v;}
std::uint32_t namehash(std::string_view name){std::uint32_t h=5381;for(auto c:name)h=h*33u^static_cast<unsigned char>(std::tolower(c));return h;}
template<class T>T read(const std::vector<std::byte>& b,std::size_t at){need(at<=b.size() && sizeof(T)<=b.size()-at,"native buffer read out of range");T x{};std::memcpy(&x,b.data()+at,sizeof x);return x;}
template<class T>void put(std::vector<std::byte>& b,std::size_t at,T x){need(at<=b.size() && sizeof(T)<=b.size()-at,"native buffer write out of range");std::memcpy(b.data()+at,&x,sizeof x);}
void xyz(s::ebx::Object& object,V v){for(unsigned k=0;k<3;++k)u::require_field(object,std::array{"x","y","z"}[k]).value.data=double(v[k]);}
void quality(s::ebx::Object& object,const char* field,double value){auto& q=u::require_object(u::require_field(object,field).value,field);for(auto key:{"Low","Medium","High","Ultra"}){auto& v=u::require_field(q,key).value; if(std::holds_alternative<double>(v.data))v.data=value;else if(std::holds_alternative<std::int64_t>(v.data))v.data=std::int64_t(value);else if(std::holds_alternative<std::uint64_t>(v.data))v.data=std::uint64_t(value);else throw std::runtime_error("unexpected native quality type");}}
void identity(s::ebx::Object& object){auto& tr=u::require_object(u::require_field(object,"Transform").value,"native transform");xyz(u::require_object(u::require_field(tr,"right").value,"right"),{1,0,0});xyz(u::require_object(u::require_field(tr,"up").value,"up"),{0,1,0});xyz(u::require_object(u::require_field(tr,"forward").value,"forward"),{0,0,1});xyz(u::require_object(u::require_field(tr,"trans").value,"trans"),{});}
m::MeshBuildInput geometry(bool arrow){namespace art=dingosdk::deathrace_ui;
 m::MeshBuildInput in;in.chunkId=e::random_guid();in.sectionFlag=0x0a;in.declaration={{1,8},{30,13},{33,6},{52,13}};
 std::vector<V> cp;if(arrow){cp.assign(art::skate2_chevron_right.begin()+4,art::skate2_chevron_right.begin()+33);for(auto& v:cp)v=mul(v,1.f/3);for(unsigned i=0;i<3;++i)cp.push_back(cp[i]);}
 else{cp.assign(art::skate2_gate_controls.begin(),art::skate2_gate_controls.end());for(auto& v:cp){v[1]+=art::skate2_gate_half_height;v=mul(v,1.f/art::skate2_gate_half_width);}}
 const unsigned steps=unsigned(cp.size()-3)*10;std::vector<V> path;for(unsigned i=0;i<=(arrow?steps-1:steps);++i)path.push_back(spline(cp,std::min(float(cp.size()-3)-.00001f,float(i)*.1f)));
 if(arrow)path.push_back(path.front()); // Duplicate UV seam avoids stretching the final segment back to zero.
 std::vector<float> along(path.size());for(unsigned i=1;i<path.size();++i){float d{};for(unsigned k=0;k<3;++k)d+=(path[i][k]-path[i-1][k])*(path[i][k]-path[i-1][k]);along[i]=along[i-1]+std::sqrt(d);}
 const float half=arrow?.05f:.0333333f;
 // Three intersecting soft ribbon planes retain volume at side angles. They
 // are one mesh in the native particle pipeline, never opaque coincident tubes.
 for(unsigned plane=0;plane<3;++plane){const unsigned base=unsigned(in.positions.size());const float a=float(plane)*3.14159265f/3;
  for(unsigned i=0;i<path.size();++i){const auto sample=arrow && i+1==path.size()?0u:i;const auto p=path[sample?sample-1:arrow?path.size()-2:0],n=path[sample+1<path.size()?sample+1:path.size()-1];const float dx=n[0]-p[0],dy=n[1]-p[1],length=std::max(.00001f,std::hypot(dx,dy));const V side{-dy/length*std::cos(a),dx/length*std::cos(a),std::sin(a)};const V normal{-dy/length*-std::sin(a),dx/length*-std::sin(a),std::cos(a)};
   for(unsigned edge=0;edge<2;++edge){in.positions.push_back(add(path[i],mul(side,half*(edge?1:-1))));in.normals.push_back(normal);in.textureCoordinates.push_back({float(edge),along[i]});}
  }
  const unsigned links=unsigned(path.size()-1);for(unsigned i=0;i<links;++i){const unsigned a0=base+i*2,b=a0+1,c=base+(i+1)*2,d=c+1;in.indices.insert(in.indices.end(),{a0,b,c,b,d,c,c,b,a0,c,d,b});}
 }
 return in;
}
int main(int argc,char** argv){try{
 need(argc==4,"game-root input-project output-project");std::cout<<std::unitbuf;
 auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=p::load_project(argv[2]);const auto before=project;
 auto find=[&](std::string_view n)->Resource {auto i=std::ranges::find(project.resources,n,&Resource::name);need(i!=project.resources.end(),"private root missing");return *i;};
 const auto gate=find("World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate");const auto bundles=gate.bundles;
 const auto& effectAsset=u::require_asset(db,"effects/gestures/effectblueprints/ebp_gesture_airdj_turntable");auto effectDoc=s::ebx::read_document(db.read(effectAsset));
 s::assets::EbxIdentityResolver resolver(db,effectAsset);const auto* meshAsset=resolver.find(effectDoc.imports.at(4).fileGuid,effectDoc.imports.at(4).classGuid);need(meshAsset,"native looping mesh not resolved");
 const auto meshDoc=s::ebx::read_document(db.read(*meshAsset));const auto rid=std::get<s::ebx::ResourceReference>(u::require_field(*meshDoc.root()->object,"MeshSetResource").value.data).id;const auto* sourceMesh=db.find_resource(rid);need(sourceMesh,"looping mesh resource missing");
 const auto mesh=s::resources::read_mesh_set(meshAsset->name,sourceMesh->name,db.read(*sourceMesh),[&](auto id,auto){auto* chunk=db.find_chunk(id);need(chunk,"native mesh chunk missing");return db.read(*chunk);});
 std::cout<<"NATIVE_EFFECT "<<effectAsset.name<<" NATIVE_MESH "<<meshAsset->name<<"\n";
 // Include the original compiled native VFX bundle unchanged. These private
 // emitter instances use its original expression shaders and compute kernels.
 std::size_t addedDependencies{},dependencyBytes{};
 for(const auto& a:db.assets){if(!std::ranges::any_of(a.bundles,[&](auto b){return std::ranges::find(effectAsset.bundles,b)!=effectAsset.bundles.end();}))continue;
  if(a.kind!=s::assets::AssetKind::ebx && a.kind!=s::assets::AssetKind::resource && a.kind!=s::assets::AssetKind::chunk)continue;
  auto same=[&](const Resource& r){return a.kind==s::assets::AssetKind::ebx?r.kind==p::ResourceKind::ebx && s::ebx::equal_name(r.name,a.name):a.kind==s::assets::AssetKind::resource?r.kind==p::ResourceKind::resource && r.resourceId==a.resourceId:r.kind==p::ResourceKind::chunk && r.chunkId==a.guid;};
  if(std::ranges::any_of(project.resources,same))continue;
  Resource r;r.kind=a.kind==s::assets::AssetKind::ebx?p::ResourceKind::ebx:a.kind==s::assets::AssetKind::resource?p::ResourceKind::resource:p::ResourceKind::chunk;r.name=a.name;r.payload=db.read(a);r.bundles=bundles;r.resourceId=a.resourceId;r.resourceType=a.resourceType;r.resourceMeta=a.resourceMeta;r.assetGuid=a.guid;r.chunkId=a.guid;r.logicalOffset=a.logicalOffset;r.logicalSize=a.logicalSize;if(r.kind==p::ResourceKind::ebx)r.assetGuid=s::ebx::read_document(r.payload).fileGuid;
  // A byte-identical resident copy belongs only to the mod's existing owning
  // bundles. Do not classify it as a replacement of the stock shared asset:
  // that would fan out into hundreds of unrelated native map/effect bundles.
  r.added=true;dependencyBytes+=r.payload.size();project.resources.push_back(std::move(r));++addedDependencies;
 }
 need(dependencyBytes<128*1024*1024,"native VFX dependencies unexpectedly exceed 128 MiB");
 auto addAsset=[&](std::string name,e::Partition& part){auto a=u::emit(name,part);Resource r;r.kind=p::ResourceKind::ebx;r.name=name;r.assetGuid=a.partitionGuid;r.payload=std::move(a.bytes);r.added=true;r.bundles=bundles;project.resources.push_back(r);return s::ebx::ImportReference{a.partitionGuid,a.rootGuid};};
 const auto prototype=m::MeshTemplate::find(db);std::array<s::ebx::ImportReference,2> meshes;std::array<std::map<Guid,Guid>,2> meshChildren;
 for(unsigned shape=0;shape<2;++shape){const auto name=std::string("World/ReSkate/Deathrace/Skate2NativeFX/")+(shape?"Chevron":"Gate");auto in=geometry(shape!=0);in.assetName=name+"_Mesh";in.materialName=mesh.sections.at(0).name;in.depotKey=mesh.sections.at(0).materialKey;auto built=prototype.build(in);
  // The native mesh-effect shader was compiled for Position/Color0/UV0/
  // tangent-frame. The generic mesh writer zero-fills Color0: make it opaque
  // white explicitly, or the native shader receives a zero color/alpha mask.
  // MeshTemplate emits these as consecutive SoA streams (half4, ubyte4n,...).
  const auto colorBegin=in.positions.size()*8,colorEnd=colorBegin+in.positions.size()*4;
  need(colorEnd<=built.vertexBufferSize,"native Color0 stream outside vertex buffer");
  std::fill(built.chunk.begin()+colorBegin,built.chunk.begin()+colorEnd,std::byte{0xff});
  // The game chooses native particle draw passes, rather than opaque world
  // depth/shadow categories. This mesh is consumed only by EmitterGraph.
  e::Partition part(meshDoc);for(std::size_t i=0;i<meshDoc.instances.size();++i)meshChildren[shape][meshDoc.instances[i].instanceGuid]=part.document().instances[i].instanceGuid;
  u::require_field(*part.root()->object,"Name").value.data=in.assetName;u::require_field(*part.root()->object,"NameHash").value.data=std::uint64_t(namehash(in.assetName));const auto id=e::random_resource_id();u::require_field(*part.root()->object,"MeshSetResource").value.data=s::ebx::ResourceReference{id};u::require_field(*part.root()->object,"StreamingEnable").value.data=false;meshes[shape]=addAsset(in.assetName,part);
  Resource r;r.kind=p::ResourceKind::resource;r.name=in.assetName;r.resourceId=id;r.resourceType=s::resources::meshSetResourceType;r.resourceMeta=built.resourceMeta;r.payload=built.resource;r.added=true;r.bundles=bundles;project.resources.push_back(r);r={};r.kind=p::ResourceKind::chunk;r.name=in.chunkId.string();r.assetGuid=r.chunkId=in.chunkId;r.logicalSize=std::uint32_t(built.chunk.size());r.payload=built.chunk;r.added=true;r.bundles=bundles;project.resources.push_back(r);
  const auto roundtrip=s::resources::read_mesh_set(in.assetName,in.assetName,built.resource,[&](auto guid,auto){need(guid==in.chunkId,"geometry chunk identity differs");return built.chunk;});need(roundtrip.sections.size()==1 && roundtrip.vertex_count()==in.positions.size() && roundtrip.triangle_count()==in.indices.size()/3,"native ribbon geometry roundtrip failed");
  need(roundtrip.sections[0].colors.size()==in.positions.size() && std::ranges::all_of(roundtrip.sections[0].colors,[](const auto& c){return c==std::array<std::uint8_t,4>{255,255,255,255};}),"native Color0 stream missing or transparent");
  std::cout<<"MESH "<<in.assetName<<" VERTICES "<<in.positions.size()<<" TRIANGLES "<<in.indices.size()/3<<"\n";
 }
 // Rebind native mesh/material identities, preserving the donor's compiled
 // geometry declaration and shader mapping rather than relying on residency.
 bool variationAdded{};for(const auto& a:db.assets){if(a.kind!=s::assets::AssetKind::ebx || a.name.find("meshvariationdb")==std::string::npos)continue;auto d=s::ebx::read_document(db.read(a));if(!std::ranges::any_of(d.imports,[&](auto& r){return r.fileGuid==meshDoc.fileGuid;}))continue;
  e::Partition variation(d);auto& entries=u::require_array(u::require_field(*variation.root()->object,"Entries").value,"native mesh variations");std::erase_if(entries,[&](auto& v){auto& ob=u::require_object(v,"variation");auto* ref=s::ebx::resolve_import(variation.document(),u::require_pointer(u::require_field(ob,"Mesh").value,"mesh"));return !ref || ref->fileGuid!=meshDoc.fileGuid || s::ebx::integer(u::require_field(ob,"VariationAssetNameHash").value)!=0;});if(entries.empty())continue;need(entries.size()==1,"ambiguous native effect mesh variation");const auto original=e::clone_value_tree(entries[0]);entries.clear();
  for(unsigned shape=0;shape<2;++shape){auto value=e::clone_value_tree(original);auto& ob=u::require_object(value,"variation");u::require_field(ob,"Mesh").value.data=variation.import(meshes[shape].fileGuid,meshes[shape].classGuid);for(auto& v:u::require_array(u::require_field(ob,"Materials").value,"materials")){auto& material=u::require_object(v,"material");auto& ref=u::require_pointer(u::require_field(material,"Material").value,"material");auto* old=s::ebx::resolve_import(variation.document(),ref);need(old && old->fileGuid==meshDoc.fileGuid && meshChildren[shape].contains(old->classGuid),"native material child identity missing");ref=variation.import(meshes[shape].fileGuid,meshChildren[shape].at(old->classGuid));}entries.push_back(value);}
  variation.prune_unreachable();const std::string name="World/ReSkate/Deathrace/Skate2NativeFX/MeshVariationDb_Win32";u::require_field(*variation.root()->object,"Name").value.data=name;addAsset(name,variation);variationAdded=true;break;
 }need(variationAdded,"no native variation mapping for the looping particle mesh");
 const std::array names{"SM_FX_GP_Checkpoint_A_AT_Gate","SM_FX_GP_Checkpoint_A_AT_Finish","SM_FX_GP_Checkpoint_A_AT_Guide","FloatingArrow_AT"};
 for(unsigned kind=0;kind<4;++kind){const std::string name=std::string("World/ReSkate/Deathrace/")+names[kind];const auto old=s::ebx::read_document(find(name).payload);e::Partition effect(effectDoc);
  effect.document().fileGuid=old.fileGuid;effect.root()->instanceGuid=old.root()->instanceGuid;u::require_field(*effect.root()->object,"Name").value.data=name;
  auto* root=effect.find_instance("EffectEntityData");need(root,"native effect entity missing");identity(*root->object);quality(*root->object,"MaxActiveInstanceCount",4096);u::require_field(*root->object,"KillOnMaxCount").value.data=false;u::require_field(*root->object,"KillByWater").value.data=false;
  auto& components=u::require_array(u::require_field(*root->object,"Components").value,"effect components");std::erase_if(components,[&](auto& v){auto ptr=u::require_pointer(v,"component");need(ptr.kind==s::ebx::PointerKind::internal,"native component isn't internal");auto& i=effect.document().instances.at(ptr.index);if(s::ebx::type_name(effect.document(),i)!="EmitterGraphEntityData")return true;auto ref=u::require_pointer(u::require_field(*i.object,"EmitterGraph").value,"graph");auto* imported=s::ebx::resolve_import(effect.document(),ref);return !imported || imported->fileGuid!=effectDoc.imports.at(0).fileGuid;});
  need(components.size()==3,"native loop must have exactly three mesh emitters");
  const V bright=kind==0?V{3.4f,2.9f,.28f}:kind==1?V{3.5f,.16f,.08f}:kind==2?V{.42f,.46f,.5f}:V{3.5f,1.25f,.12f};
  unsigned n{};for(auto& v:components){auto ptr=u::require_pointer(v,"component");auto& object=*effect.document().instances.at(ptr.index).object;identity(object);
   u::require_field(object,"StartDelay").value.data=double(n++)*.07;u::require_field(object,"DrawCameraOffset").value.data=0.;u::require_field(object,"OverrideDrawOrder").value.data=false;
   auto& over=u::require_object(u::require_field(object,"EmitterGraphOverrides").value,"emitter overrides");quality(over,"ParticleMaxCount",1);quality(over,"InitialParticleCount",1);quality(over,"ParticleLifeSpan",.8);quality(over,"EmitterLifeSpan",0);quality(over,"SpawnRate",0);
   for(auto key:{"IsParticleMaxCountOverrideSet","IsInitialParticleCountOverrideSet","IsParticleLifeSpanOverrideSet","IsEmitterLifeSpanOverrideSet","IsSpawnRateOverrideSet","IsBoundingBoxMinSet","IsBoundingBoxMaxSet"})u::require_field(over,key).value.data=true;
   xyz(u::require_object(u::require_field(over,"BoundingBoxMin").value,"min"),{-2,-2,-2});xyz(u::require_object(u::require_field(over,"BoundingBoxMax").value,"max"),{2,2,2});
   auto& parameters=u::require_array(u::require_field(object,"EmitterGraphParams").value,"parameters");for(auto& value:parameters){auto& param=u::require_object(value,"parameter");const auto id=std::uint32_t(*s::ebx::integer(u::require_field(param,"PropertyId").value));auto& vec=u::require_object(u::require_field(param,"Value").value,"parameter value");
    if(id==3450751526u)xyz(vec,{1,1,1}); // Native SpawnScale (DJB2 of exact exposed name).
    if(id==802351446u)xyz(vec,{});       // Native RotationSpeed: authored direction stays fixed.
    if(id==3656522166u)xyz(vec,mul(bright,.025f)); // Color_1.
    if(id==3656522165u)xyz(vec,bright);           // Color_2, native animated HDR emissive.
    if(id==205677681u){xyz(vec,{.72f,0,0});} // Alpha.
   }
   auto& model=u::require_object(u::require_field(object,"EmitterGraphMesh").value,"native effect mesh");const auto meshRef=meshes[kind==3?1:0];u::require_field(model,"Mesh").value.data=effect.import(meshRef.fileGuid,meshRef.classGuid);
  }
  // Exported donor instances survive ordinary reachability pruning. Explicitly
  // retire the unrelated fourth (sprite) emitter before serializing this FX.
  std::set<std::int32_t> used;for(auto& v:components)used.insert(u::require_pointer(v,"component").index);
  std::vector<std::uint8_t> drop(effect.document().instances.size());for(std::size_t i=0;i<drop.size();++i)if(s::ebx::type_name(effect.document(),effect.document().instances[i])=="EmitterGraphEntityData" && !used.contains(std::int32_t(i)))drop[i]=1;
  effect.drop_marked(drop);effect.prune_unreachable();effect.sort_exported_instances();auto a=u::emit(name,effect);auto target=std::ranges::find(project.resources,name,&Resource::name);target->payload=std::move(a.bytes);
  auto check=s::ebx::read_document(target->payload);need(check.rootType=="EffectBlueprint" && check.fileGuid==old.fileGuid && check.root()->instanceGuid==old.root()->instanceGuid,"native effect identity or type mismatch");need(std::ranges::count_if(check.instances,[&](auto& i){return s::ebx::type_name(check,i)=="EmitterGraphEntityData";})==3,"serialized native layer count differs");std::cout<<"EFFECT "<<name<<" LAYERS 3 PARTICLES 3 NATIVE_LOOP true\n";
 }
 // This candidate changes only the four registered private world roots. All
 // existing payloads, metadata, map registries, logos and HUD assets survive.
 unsigned changed{};for(std::size_t i=0;i<before.resources.size();++i)if(project.resources[i]!=before.resources[i]){auto check=project.resources[i];need(std::ranges::any_of(names,[&](auto n){return check.name==std::string("World/ReSkate/Deathrace/")+n;}),"unrelated asset changed");check.payload=before.resources[i].payload;need(check==before.resources[i],"existing resource metadata changed");++changed;}need(changed==4,"exactly four private roots must change");
 project.metadata.version="1.0.25-native-scale-uv-fix";p::save_project(project,argv[3]);auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
 std::cout<<"PASS NATIVE_EFFECT_CANDIDATE dependencies="<<addedDependencies<<" bytes="<<dependencyBytes<<" existing_payload_changes=4 native_runtime_playtest_required=true\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<"\n";return 1;}}
