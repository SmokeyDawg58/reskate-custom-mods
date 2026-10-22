#include "modding/projects/project.h"
#include "modding/maps/ui/ui_assets.h"
#include "sdk/resources/mesh_set.h"
#include <algorithm>
#include <iostream>
namespace s=reskate::sdk;namespace p=reskate::modding::projects;namespace u=reskate::modding::maps::ui;
void check(bool b,const char* text){if(!b)throw std::runtime_error(text);}
int main(int argc,char** argv){try{
    check(argc==4,"game-root previous-project repaired-project");
    const auto db=s::assets::FrostbiteAssetManager::load_sync(argv[1]);
    const auto before=p::load_project(argv[2]),after=p::load_project(argv[3]);
    check(before.resources.size()==after.resources.size(),"resources added or removed");
    unsigned changes{},meshes{},oldMissing{},visibleVertices{};
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
        auto decode=[&](const auto& project,const auto& r){return s::resources::read_mesh_set(r.name,r.name,r.payload,[&](auto id,auto){auto c=std::ranges::find_if(project.resources,[&](const auto& v){return v.kind==p::ResourceKind::chunk && v.chunkId==id;});check(c!=project.resources.end(),"mesh chunk missing");return c->payload;});};
        const auto a=decode(before,*previous),b=decode(after,resource);check(a.sections.size()==1 && b.sections.size()==1,"section count changed");
        const auto& visible=b.sections[0];const auto& old=a.sections[0];
        oldMissing+=std::ranges::none_of(old.vertexElements,[](auto e){return e[0]==30;});
        check(visible.vertexElements.size()==stockDecl.size(),"native shader declaration count mismatch");
        for(unsigned i=0;i<stockDecl.size();++i)check(visible.vertexElements[i][0]==stockDecl[i][0] && visible.vertexElements[i][1]==stockDecl[i][1],"native shader declaration format mismatch");
        check(visible.colors.size()==visible.positions.size(),"Color0 absent");
        for(const auto& color:visible.colors)check(color==std::array<std::uint8_t,4>{255,255,255,255},"native Color0 is black or transparent");
        check(visible.positions==old.positions && visible.indices==old.indices && visible.textureCoordinates==old.textureCoordinates && visible.materialKey==old.materialKey,"shape, material or UV changed");
        check(visible.materialKey==stock.sections[0].materialKey,"shader material key differs from donor");
        visibleVertices+=unsigned(visible.positions.size());++meshes;
    }
    check(changes==4 && meshes==2 && oldMissing==2,"unexpected regression/repair scope");
    std::cout<<"{\"result\":\"PASS\",\"changed_payloads\":4,\"mesh_shapes\":2,\"old_meshes_missing_native_Color0\":2,\"visible_vertices\":"<<visibleVertices<<",\"shader_vertex_declarations_match_stock\":true,\"all_color_channels_opaque_white\":true,\"geometry_and_uv_unchanged\":true,\"native_material_keys_match_stock\":true,\"all_other_project_resources_unchanged\":true,\"in_game_retest\":\"REQUIRED\"}\n";
}catch(const std::exception& e){std::cerr<<e.what()<<"\n";return 1;}}
