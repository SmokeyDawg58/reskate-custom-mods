// Offline asset author. Never opens or attaches to the game.
#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/maps/ui/ui_assets.h"
#include "modding/maps/ebx/ebx_partition.h"
#include "modding/models/mesh_writer.h"
#include "sdk/resources/mesh_set.h"
#include "sdk/ebx/boxed_value.h"
#include "sdk/ebx/query.h"
#include "sdk/hash/hash.h"
#include "../../Extension/UI/Overlay/deathrace_skate2_art.h"
#include <algorithm>
#include <cmath>
#include <cstring>
#include <iostream>
#include <fstream>
#include <map>
#include <set>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;namespace m=reskate::modding::models;namespace e=reskate::modding::maps::ebx;namespace ui=reskate::modding::maps::ui;
using V=std::array<float,3>;using Guid=s::io::Guid;
V plus(V a,V b){for(unsigned i=0;i<3;++i)a[i]+=b[i];return a;}V times(V a,float t){for(auto& x:a)x*=t;return a;}
V sample(std::span<const V> c,float t){const auto i=std::min<std::size_t>(std::size_t(t),c.size()-4);const float u=t-float(i);const std::array w{(1-u)*(1-u)*(1-u),4-6*u*u+3*u*u*u,1+3*u+3*u*u-3*u*u*u,u*u*u};V out{};for(unsigned j=0;j<4;++j)out=plus(out,times(c[i+j],w[j]/6));return out;}
m::MeshBuildInput geometry(bool arrow){
 namespace art=dingosdk::deathrace_ui;
 m::MeshBuildInput in;in.chunkId=e::random_guid();in.sectionFlag=0x0a;in.declaration={{1,8},{33,6},{52,13}};
 std::vector<V> cp;bool closed=arrow;
 if(arrow){
  // Original game serializes a closing edge twice, with endpoint padding.
  // Sample the unique closed loop periodically: one seamless edge, no hot box.
  cp.assign(art::skate2_chevron_right.begin()+4,art::skate2_chevron_right.begin()+33);
  for(auto& v:cp)v=times(v,1.f/3.f);
  const auto n=cp.size();for(unsigned j=0;j<3;++j)cp.push_back(cp[j%n]);
 }else{cp.assign(art::skate2_gate_controls.begin(),art::skate2_gate_controls.end());for(auto& v:cp){v[1]+=art::skate2_gate_half_height;v=times(v,1.f/art::skate2_gate_half_width);}}
 std::vector<V> path;const unsigned steps=unsigned(cp.size()-3)*4;
 for(unsigned i=0;i<(closed?steps:steps+1);++i){auto v=sample(cp,std::min(float(cp.size()-3)-.00001f,float(i)*.25f));if(path.empty() || std::hypot(v[0]-path.back()[0],v[1]-path.back()[1])>.00001f)path.push_back(v);}
 constexpr unsigned sides=6;const float radius=arrow?.016f:.009f;
 for(unsigned i=0;i<path.size();++i){const auto& prev=path[i?i-1:closed?path.size()-1:0];const auto& next=path[i+1<path.size()?i+1:closed?0:path.size()-1];V tangent{next[0]-prev[0],next[1]-prev[1],0};const float length=std::max(.000001f,std::hypot(tangent[0],tangent[1]));V normal{-tangent[1]/length,tangent[0]/length,0};
  for(unsigned j=0;j<sides;++j){const float angle=6.283185307f*float(j)/sides;V n{normal[0]*std::cos(angle),normal[1]*std::cos(angle),std::sin(angle)};in.positions.push_back(plus(path[i],times(n,radius)));in.normals.push_back(n);
   // Fixed non-edge mask coordinates give a uniform native FX core; there is
   // no UV seam or stacked cap to flare up at the arrow's closing segment.
   in.textureCoordinates.push_back({.5f,.09f});
  }
 }
 const unsigned links=closed?unsigned(path.size()):unsigned(path.size()-1);
 for(unsigned i=0;i<links;++i)for(unsigned j=0;j<sides;++j){const unsigned a=i*sides+j,b=i*sides+(j+1)%sides,c=((i+1)%path.size())*sides+j,d=((i+1)%path.size())*sides+(j+1)%sides;in.indices.insert(in.indices.end(),{a,b,c,b,d,c});}
 return in;
}
int main(int argc,char** argv){try{
 if(argc!=4)throw std::runtime_error("game input-project new-output-project");
 const auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=p::load_project(argv[2]);const auto before=project;
 const auto find=[&](std::string name)->p::ProjectResource {auto i=std::ranges::find(project.resources,name,&p::ProjectResource::name);if(i==project.resources.end())throw std::runtime_error("Missing private donor "+name);return *i;};
 const auto root=find("World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate");const auto bundles=root.bundles;
 auto blueprintDoc=s::ebx::read_document(root.payload);s::ebx::decode_boxed_values(blueprintDoc,root.payload);auto donorRef=blueprintDoc.imports.at(0);auto meshSource=std::ranges::find(project.resources,donorRef.fileGuid,&p::ProjectResource::assetGuid);if(meshSource==project.resources.end())throw std::runtime_error("Resident mesh donor missing");
 const auto donorDoc=s::ebx::read_document(meshSource->payload);const auto& meshRoot=*donorDoc.root()->object;auto meshField=std::ranges::find(meshRoot.fields,std::string("MeshSetResource"),&s::ebx::FieldValue::name);const auto resourceId=std::get<s::ebx::ResourceReference>(meshField->value.data).id;
 const auto resource=std::ranges::find(project.resources,resourceId,&p::ProjectResource::resourceId);if(resource==project.resources.end())throw std::runtime_error("Resident MeshSet missing");
 const auto decoded=s::resources::read_mesh_set(resource->name,resource->name,resource->payload,[&](auto id,auto){auto i=std::ranges::find(project.resources,id,&p::ProjectResource::chunkId);if(i==project.resources.end())throw std::runtime_error("Mesh chunk not resident");return i->payload;});
 const auto prototype=m::MeshTemplate::find(db);std::cout<<"TEMPLATE "<<prototype.source_name()<<"\n";
 struct Identity {Guid file,root;std::map<Guid,Guid> children;};std::array<Identity,2> ids;
 std::ofstream meshPreview(std::filesystem::path(argv[3]).parent_path()/"native-models.json");meshPreview<<"[";
 for(unsigned shape=0;shape<2;++shape){const std::string name=std::string("World/ReSkate/Deathrace/Skate2/")+(shape?"Chevron":"Gate");auto in=geometry(shape!=0);in.assetName=name+"_Mesh";in.materialName=decoded.sections.at(0).name;in.depotKey=decoded.sections.at(0).materialKey;
  auto built=prototype.build(in);e::Partition part(donorDoc);auto& id=ids[shape];id.file=part.document().fileGuid;id.root=part.root()->instanceGuid;for(unsigned i=0;i<donorDoc.instances.size();++i)id.children[donorDoc.instances[i].instanceGuid]=part.document().instances[i].instanceGuid;
  ui::require_field(*part.root()->object,"Name").value.data=in.assetName;ui::require_field(*part.root()->object,"NameHash").value.data=std::uint64_t([&]{std::uint32_t h=5381;for(unsigned char ch:in.assetName)h=h*33u^static_cast<unsigned char>(std::tolower(ch));return h;}());
  const auto rid=e::random_resource_id();ui::require_field(*part.root()->object,"MeshSetResource").value.data=s::ebx::ResourceReference{rid};
  if(auto* f=part.field(*part.root(),"StreamingEnable"))f->value.data=false;
  auto asset=ui::emit(in.assetName,part);p::ProjectResource mr;mr.kind=p::ResourceKind::ebx;mr.name=in.assetName;mr.assetGuid=asset.partitionGuid;mr.payload=asset.bytes;mr.added=true;mr.bundles=bundles;project.resources.push_back(mr);
  p::ProjectResource res;res.kind=p::ResourceKind::resource;res.name=in.assetName;res.resourceId=rid;res.resourceType=s::resources::meshSetResourceType;res.resourceMeta=built.resourceMeta;res.payload=built.resource;res.added=true;res.bundles=bundles;project.resources.push_back(res);
  p::ProjectResource ch;ch.kind=p::ResourceKind::chunk;ch.name=in.chunkId.string();ch.chunkId=in.chunkId;ch.assetGuid=in.chunkId;ch.logicalSize=std::uint32_t(built.chunk.size());ch.payload=built.chunk;ch.added=true;ch.bundles=bundles;project.resources.push_back(ch);
  const auto verify=s::resources::read_mesh_set(in.assetName,in.assetName,built.resource,[&](auto id,auto){if(id!=in.chunkId)throw std::runtime_error("Authored chunk mismatch");return built.chunk;});if(verify.sections.size()!=1 || verify.vertex_count()!=in.positions.size() || verify.triangle_count()!=in.indices.size()/3)throw std::runtime_error("Native mesh roundtrip mismatch");
  std::cout<<"MESH "<<in.assetName<<" VERTICES "<<verify.vertex_count()<<" TRIANGLES "<<verify.triangle_count()<<" CATEGORY "<<unsigned(verify.sections.front().renderCategories)<<"\n";
  if(shape)meshPreview<<",";meshPreview<<"{\"name\":\""<<in.assetName<<"\",\"positions\":[";bool comma=false;for(const auto& v:in.positions){if(comma)meshPreview<<",";comma=true;meshPreview<<"["<<v[0]<<","<<v[1]<<","<<v[2]<<"]";}meshPreview<<"],\"indices\":[";comma=false;for(auto x:in.indices){if(comma)meshPreview<<",";comma=true;meshPreview<<x;}meshPreview<<"]}";
 }
 meshPreview<<"]";
 // Keep compiled native shader/material mappings, with only mesh and material
 // identities rebound into the new private partition. No stock mesh is edited.
 e::Partition variation(s::ebx::read_document(db.read(ui::require_asset(db,"levels/game/bam_levelroot/gameplay_activities/meshvariationdb_win32"))));
 auto& entries=ui::require_array(ui::require_field(*variation.root()->object,"Entries").value,"variations");
 std::erase_if(entries,[&](auto& v){auto& o=ui::require_object(v,"variation");const auto* r=s::ebx::resolve_import(variation.document(),ui::require_pointer(ui::require_field(o,"Mesh").value,"mesh"));return !r || r->fileGuid!=donorDoc.fileGuid || s::ebx::integer(ui::require_field(o,"VariationAssetNameHash").value)!=0;});
 if(entries.size()!=1)throw std::runtime_error("Expected exact native default shader mapping");const auto original=e::clone_value_tree(entries[0]);entries.clear();
 for(const auto& id:ids){auto v=e::clone_value_tree(original);auto& o=ui::require_object(v,"variation");ui::require_field(o,"Mesh").value.data=variation.import(id.file,id.root);for(auto& mv:ui::require_array(ui::require_field(o,"Materials").value,"materials")){auto& material=ui::require_object(mv,"material");auto& ref=ui::require_pointer(ui::require_field(material,"Material").value,"material ref");const auto* old=s::ebx::resolve_import(variation.document(),ref);if(!old || old->fileGuid!=donorDoc.fileGuid || !id.children.contains(old->classGuid))throw std::runtime_error("Native material identity differs");ref=variation.import(id.file,id.children.at(old->classGuid));}entries.push_back(v);}
 variation.prune_unreachable();const std::string vname="World/ReSkate/Deathrace/Skate2/MeshVariationDb_Win32";ui::require_field(*variation.root()->object,"Name").value.data=vname;auto va=ui::emit(vname,variation);p::ProjectResource vr;vr.name=vname;vr.kind=p::ResourceKind::ebx;vr.assetGuid=va.partitionGuid;vr.payload=va.bytes;vr.bundles=bundles;vr.added=true;project.resources.push_back(vr);
 for(unsigned kind=0;kind<4;++kind){e::Partition bp(blueprintDoc);const auto name=std::string("World/ReSkate/Deathrace/")+std::array{"SM_FX_GP_Checkpoint_A_AT_Gate","SM_FX_GP_Checkpoint_A_AT_Finish","SM_FX_GP_Checkpoint_A_AT_Guide","FloatingArrow_AT"}[kind];const auto prior=s::ebx::read_document(find(name).payload);bp.document().fileGuid=prior.fileGuid;bp.root()->instanceGuid=prior.root()->instanceGuid;ui::require_field(*bp.root()->object,"Name").value.data=name;
  const auto id=ids[kind==3?1:0];auto* model=bp.find_instance("StaticModelEntityData");if(!model)throw std::runtime_error("Native model missing");ui::require_field(*model->object,"Mesh").value.data=bp.import(id.file,id.root);
  const std::array<float,4> rgba=kind==0?std::array{.8f,.66f,.01f,0.f}:kind==1?std::array{.85f,.035f,.015f,0.f}:kind==2?std::array{.25f,.27f,.28f,0.f}:std::array{.75f,.20f,.015f,0.f};
  for(auto& box:bp.document().boxedValues)if(box.rawBytes.size()==16){std::memcpy(box.rawBytes.data(),rgba.data(),16);if(box.decoded){auto& o=ui::require_object(*box.decoded,"color");for(unsigned axis=0;axis<3;++axis)ui::require_field(o,std::array{"x","y","z"}[axis]).value.data=double(rgba[axis]);}}
  // The native entity gets bounds for the whole authored model, not its quad donor.
  auto& bounds=ui::require_array(ui::require_field(*model->object,"PartBoundingBoxes").value,"bounds");for(auto& v:bounds){auto& o=ui::require_object(v,"box");for(const auto* key:{"min","max"}){auto& vec=ui::require_object(ui::require_field(o,key).value,"bound");for(unsigned axis=0;axis<3;++axis)ui::require_field(vec,std::array{"x","y","z"}[axis]).value.data=std::string_view(key)=="min"?-2.0:2.0;}}
  bp.prune_unreachable();auto a=ui::emit(name,bp);p::ProjectResource rr;rr.kind=p::ResourceKind::ebx;rr.name=name;rr.assetGuid=a.partitionGuid;rr.payload=a.bytes;auto existing=std::ranges::find(project.resources,name,&p::ProjectResource::name);existing->payload=a.bytes;
 }
 unsigned changed{};for(std::size_t i=0;i<before.resources.size();++i){auto old=before.resources[i],now=project.resources[i];if(old!=now){if(old.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate" && old.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish" && old.name!="World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide" && old.name!="World/ReSkate/Deathrace/FloatingArrow_AT")throw std::runtime_error("Unrelated asset changed");now.payload=old.payload;if(now!=old)throw std::runtime_error("Original metadata changed");++changed;}}if(changed!=4)throw std::runtime_error("Expected four private roots");
 project.metadata.version="1.0.22";p::save_project(project,argv[3]);auto path=std::filesystem::path(argv[3]);path.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,path,argv[1],&db);
 std::cout<<"PASS "<<before.resources.size()<<" existing identities and metadata preserved; 4 private blueprints updated, all other payloads preserved; 7 private resources added; native depth-tested mesh geometry, seamless right-facing chevron.\n";
}catch(const std::exception& ex){std::cerr<<ex.what()<<"\n";return 1;}}
