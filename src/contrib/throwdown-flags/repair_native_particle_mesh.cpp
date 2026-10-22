#include "modding/projects/project.h"
#include "modding/mods/fbmod.h"
#include "modding/models/mesh_writer.h"
#include "modding/maps/ui/ui_assets.h"
#include "sdk/ebx/query.h"
#include "sdk/resources/mesh_set.h"
#include <algorithm>
#include <iostream>
#include <set>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;
namespace m=reskate::modding::models;namespace u=reskate::modding::maps::ui;
void need(bool value,const char* text){if(!value)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
    need(argc==4,"game-root existing-project output-project");std::cout<<std::unitbuf;
    auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);auto project=p::load_project(argv[2]);const auto before=project;
    const auto prototype=m::MeshTemplate::find(db);
    const auto& donor=u::require_asset(db,"effects/characters/meshes/fxm_ch_gesture_airdj_speaker_ringsmall_mesh");
    const auto stock=s::resources::load_mesh_set(db,donor);
    need(stock.sections.size()>=1,"stock mesh sections missing");
    const auto& declaration=stock.sections[0].vertexElements;
    need(declaration.size()==4 && declaration[0][0]==1 && declaration[0][1]==8 &&
         declaration[1][0]==30 && declaration[1][1]==13 && declaration[2][0]==33 && declaration[2][1]==6 &&
         declaration[3][0]==52 && declaration[3][1]==13,"native particle shader declaration differs");
    unsigned repaired{};
    for(const auto shape:{"Gate","Chevron"}){
        const std::string name=std::string("World/ReSkate/Deathrace/Skate2NativeFX/")+shape+"_Mesh";
        auto resource=std::ranges::find_if(project.resources,[&](const auto& r){return r.name==name && r.kind==p::ResourceKind::resource;});
        need(resource!=project.resources.end(),"private mesh resource missing");
        auto chunk=project.resources.end();
        const auto mesh=s::resources::read_mesh_set(name,name,resource->payload,[&](auto id,auto){
            chunk=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==p::ResourceKind::chunk && r.chunkId==id;});
            need(chunk!=project.resources.end(),"private mesh chunk missing");return chunk->payload;
        });
        need(mesh.sections.size()==1 && chunk!=project.resources.end(),"private mesh section/chunk count differs");
        const auto& section=mesh.sections[0];m::MeshBuildInput in;
        in.assetName=name;in.materialName=section.name;in.depotKey=section.materialKey;in.chunkId=chunk->chunkId;
        in.positions=section.positions;in.normals=section.normals;in.textureCoordinates=section.textureCoordinates;in.indices=section.indices;
        in.declaration.clear();for(const auto& e:declaration)in.declaration.push_back({e[0],e[1]});
        auto built=prototype.build(in);
        const auto colorBegin=in.positions.size()*8,colorEnd=colorBegin+in.positions.size()*4;
        need(colorEnd<=built.vertexBufferSize,"Color0 stream outside vertex buffer");
        std::fill(built.chunk.begin()+colorBegin,built.chunk.begin()+colorEnd,std::byte{0xff});
        const auto checked=s::resources::read_mesh_set(name,name,built.resource,[&](auto id,auto){need(id==in.chunkId,"private chunk identity changed");return built.chunk;});
        const auto& visible=checked.sections.at(0);
        need(visible.vertexElements.size()==declaration.size(),"native declaration count mismatch");
        for(unsigned i=0;i<declaration.size();++i)need(visible.vertexElements[i][0]==declaration[i][0] && visible.vertexElements[i][1]==declaration[i][1],"native shader vertex input mismatch");
        need(visible.colors.size()==in.positions.size() && std::ranges::all_of(visible.colors,[](const auto& c){return c==std::array<std::uint8_t,4>{255,255,255,255};}),"Color0 alpha is transparent or missing");
        need(visible.positions==section.positions && visible.textureCoordinates==section.textureCoordinates && visible.indices==section.indices,"existing native shape or UV changed");
        resource->payload=std::move(built.resource);resource->resourceMeta=std::move(built.resourceMeta);
        chunk->payload=std::move(built.chunk);chunk->logicalSize=static_cast<std::uint32_t>(chunk->payload.size());
        ++repaired;std::cout<<"REPAIRED "<<name<<" vertices="<<visible.positions.size()<<" Color0=255,255,255,255 donor_shader_declaration_matches=true\n";
    }
    unsigned changed{};for(unsigned i=0;i<before.resources.size();++i)if(project.resources[i]!=before.resources[i]){
        const auto& r=project.resources[i];need(r.kind==p::ResourceKind::chunk || (r.kind==p::ResourceKind::resource && r.name.starts_with("World/ReSkate/Deathrace/Skate2NativeFX/")),"unrelated resource changed");++changed;
    }
    need(repaired==2 && changed==4,"repair must change two mesh resources and two geometry chunks only");
    project.metadata.version="1.0.24-native-color0-fix";p::save_project(project,argv[3]);auto mod=std::filesystem::path(argv[3]);mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,argv[1],&db);
    std::cout<<"PASS repaired_meshes=2 changed_resources=4 existing_identity_preserved=true\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
