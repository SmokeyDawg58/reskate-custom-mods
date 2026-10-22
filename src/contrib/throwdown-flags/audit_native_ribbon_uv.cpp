#include "modding/projects/project.h"
#include "modding/maps/ui/ui_assets.h"
#include "sdk/resources/mesh_set.h"
#include <algorithm>
#include <iostream>
#include <cmath>
#include <set>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;namespace u=reskate::modding::maps::ui;
void check(bool b,const char* text){if(!b)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
    check(argc==4,"game-root previous-project repaired-project");
    const auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);
    const auto before=p::load_project(argv[2]),after=p::load_project(argv[3]);
    check(before.resources.size()==after.resources.size(),"resources added or removed");
    unsigned changes{},meshes{},oldMissing{},visibleVertices{};std::set<s::io::Guid> allowedChunks;
    for(unsigned i=0;i<before.resources.size();++i){
        if(before.resources[i]==after.resources[i])continue;
        ++changes;auto old=before.resources[i],now=after.resources[i];
        check((now.kind==p::ResourceKind::resource && now.name.starts_with("World/ReSkate/Deathrace/Skate2NativeFX/")) || now.kind==p::ResourceKind::chunk,"unrelated resource changed");
        now.payload=old.payload;now.resourceMeta=old.resourceMeta;now.logicalSize=old.logicalSize;
        check(now==old,"resource identity, bundle ownership or metadata changed");
    }
    const auto stock=s::resources::load_mesh_set(db,u::require_asset(db,"effects/characters/meshes/fxm_ch_gesture_airdj_speaker_ringsmall_mesh"));
    const auto& stockDecl=stock.sections.at(0).vertexElements;
    for(const auto& resource:after.resources){
        if(resource.kind!=p::ResourceKind::resource || !resource.name.starts_with("World/ReSkate/Deathrace/Skate2NativeFX/"))continue;
        const auto previous=std::ranges::find_if(before.resources,[&](const auto& r){return r.kind==p::ResourceKind::resource && r.resourceId==resource.resourceId;});
        check(previous!=before.resources.end(),"old private mesh missing");
        auto decode=[&](const auto& project,const auto& r){return s::resources::read_mesh_set(r.name,r.name,r.payload,[&](auto id,auto){if(&project==&after)allowedChunks.insert(id);auto c=std::ranges::find_if(project.resources,[&](const auto& v){return v.kind==p::ResourceKind::chunk && v.chunkId==id;});check(c!=project.resources.end(),"mesh chunk missing");return c->payload;});};
        const auto a=decode(before,*previous),b=decode(after,resource);check(a.sections.size()==1 && b.sections.size()==1,"section count changed");
        const auto& visible=b.sections[0];const auto& old=a.sections[0];
        oldMissing+=std::ranges::none_of(old.vertexElements,[](auto e){return e[0]==30;});
        check(visible.vertexElements.size()==stockDecl.size(),"native shader declaration count mismatch");
        for(unsigned i=0;i<stockDecl.size();++i)check(visible.vertexElements[i][0]==stockDecl[i][0] && visible.vertexElements[i][1]==stockDecl[i][1],"native shader declaration format mismatch");
        check(visible.colors.size()==visible.positions.size(),"Color0 absent");
        for(const auto& color:visible.colors)check(color==std::array<std::uint8_t,4>{255,255,255,255},"native Color0 is black or transparent");
        
        const bool closed=resource.name.find("Chevron")!=std::string::npos;
        const auto oldPairs=old.positions.size()/6,newPairs=visible.positions.size()/6;
        check(old.positions.size()%6==0 && newPairs==oldPairs+(closed?1:0),"unexpected ribbon vertex/seam count");
        check(visible.indices.size()==old.indices.size() && visible.materialKey==old.materialKey,"triangle count or material changed");
        for(unsigned plane=0;plane<3;++plane){
            for(unsigned i=0;i<oldPairs*2;++i)check(old.positions[plane*oldPairs*2+i]==visible.positions[plane*newPairs*2+i],"recovered Skate 2 silhouette changed");
            for(unsigned i=0;i<newPairs;++i){
                const auto v=plane*newPairs*2+i*2;
                check(visible.textureCoordinates[v][0]==0 && visible.textureCoordinates[v+1][0]==1,"beam must cover U across each ribbon");
                check(visible.textureCoordinates[v][1]==visible.textureCoordinates[v+1][1],"uneven longitudinal UVs distort the beam");
                if(i)check(visible.textureCoordinates[v][1]>=visible.textureCoordinates[v-2][1],"beam animation must run continuously along the path");
                if(closed && i+1==newPairs)check(visible.positions[v]==visible.positions[plane*newPairs*2] && visible.positions[v+1]==visible.positions[plane*newPairs*2+1],"chevron seam does not close on its original shape");
            }
        }
        check(visible.textureCoordinates[(newPairs-1)*2][1]>2.f,"ribbon longitudinal UV range collapsed");
        // Negative control: these exact previous UVs repeated the beam's bright
        // cross section along U and generated the reported separated bars.
        check(old.textureCoordinates[2][0]>old.textureCoordinates[0][0] && old.textureCoordinates[2][1]==old.textureCoordinates[0][1],"before mesh no longer reproduces the striped-axis regression");
        check(visible.textureCoordinates!=old.textureCoordinates,"striped UVs still present");

        check(visible.materialKey==stock.sections[0].materialKey,"shader material key differs from donor");
        visibleVertices+=unsigned(visible.positions.size());++meshes;
    }
    for(unsigned i=0;i<before.resources.size();++i)if(before.resources[i]!=after.resources[i] && after.resources[i].kind==p::ResourceKind::chunk)check(allowedChunks.contains(after.resources[i].chunkId),"unrelated native chunk changed");
    check(changes>=2 && changes<=4 && meshes==2 && oldMissing==0,"unexpected repair scope or Color0 regression");
    std::cout<<"{\"result\":\"PASS\",\"changed_payloads\":"<<changes<<",\"mesh_shapes\":2,\"visible_vertices\":"<<visibleVertices<<",\"native_shader_vertex_declarations_match\":true,\"all_color_channels_opaque_white\":true,\"recovered_silhouettes_preserved\":true,\"continuous_beam_uv_along_v\":true,\"old_striped_uv_negative_control\":true,\"closed_chevron_uv_seam\":true,\"native_material_keys_match_stock\":true,\"other_project_resources_unchanged\":true,\"in_game_retest\":\"REQUIRED\"}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
