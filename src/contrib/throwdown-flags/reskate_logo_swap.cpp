#include "modding/maps/ui/ui_assets.h"
#include "modding/mods/fbmod.h"
#include "modding/projects/project.h"
#include "modding/textures/png_codec.h"
#include "modding/textures/rime_texture.h"
#include "sdk/ebx/query.h"
#include "sdk/io/binary_writer.h"
#include <algorithm>
#include <iostream>
#include <stdexcept>

namespace sdk=reskate::sdk;
namespace ui=reskate::modding::maps::ui;
namespace tex=reskate::modding::textures;
namespace projects=reskate::modding::projects;

projects::ProjectResource& resource(projects::Project& project,projects::ResourceKind kind,const std::string& name) {
    const auto it=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;});
    if(it==project.resources.end())throw std::runtime_error("Existing private resource missing: "+name);
    return *it;
}
void preview(const std::filesystem::path& path,const sdk::resources::LoadedTexture& texture) {
    sdk::io::BinaryWriter file;file.bytes(tex::texture_to_png(texture));file.save(path);
}
int main(int count,const char* const* args) {
    try {
        std::cout<<std::unitbuf;
        if(count!=5)throw std::runtime_error("logo-swap <game-root> <current.fbproject> <logo.png> <updated.fbproject>");
        const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[1]);
        auto project=projects::load_project(args[2]);
        const auto pixels=tex::load_artwork_image(args[3]);
        const std::filesystem::path output=args[4];
        std::filesystem::create_directories(output.parent_path());
        auto& picture=resource(project,projects::ResourceKind::ebx,"UI/ReSkate/Throwdowns/img_skate_Reskate_1024");
        const auto document=sdk::ebx::read_document(picture.payload);
        const auto& owner=ui::require_asset(db,"UI/Textures/Icons/Activities/Throwdowns/FullColor/img_Icon_Throwdown_Color_Full_1024");
        const auto variants=tex::rime_texture_variants(db,project,owner,document);
        auto changed=tex::replace_rime_textures(db,project,variants,0,&pixels,{});
        for(auto& replacement:changed.resources) {
            auto& original=resource(project,replacement.kind,replacement.name);
            // Equal dimensions/format/mips retain chunk size and every identity.
            if(original.payload.size()!=replacement.payload.size())throw std::runtime_error("Logo replacement changed native chunk dimensions");
            original.payload=std::move(replacement.payload);
        }
        preview(output.parent_path()/"reskate-logo-native-preview.png",changed.preview);
        if(tex::decode_png(tex::texture_to_png(changed.preview)).rgba.empty())throw std::runtime_error("Logo preview is empty");
        for(const auto& variant:variants)
            std::cout<<"logo_resolution="<<variant.width<<'x'<<variant.height<<" resource="<<variant.resourceId<<" chunk="<<variant.chunkId.string()<<'\n';

        // The physical flag and placement preview share this existing cloth
        // texture. Rebuild pixels only; keep its material/mesh/network graph.
        const auto source=tex::read_clone_source(db,ui::require_asset(db,"world/materials/signage/svcc/signage_svcc_flag_td_spotbattle_1x4_01_c"));
        const auto stock=tex::decode_png(tex::texture_to_png(source.loaded));
        if(stock.width!=256 || stock.height!=1024)throw std::runtime_error("Native flag UV dimensions differ");
        auto cloth=stock;
        for(unsigned y=96;y<360;++y)for(unsigned x=0;x<256;++x) {
            const auto from=(std::size_t(32+(y-96)%56)*256+x)*4,to=(std::size_t(y)*256+x)*4;
            std::copy_n(stock.rgba.begin()+from,4,cloth.rgba.begin()+to);
        }
        // Match the supplied pink badge while retaining the stock cloth wear.
        constexpr std::array<unsigned,3> rgb{238,3,209};
        for(std::size_t at=0;at<cloth.rgba.size();at+=4) {
            const auto red=cloth.rgba[at],green=cloth.rgba[at+1],blue=cloth.rgba[at+2];
            if(red>80 && blue>80 && green<std::max(red,blue)*.82 && std::abs(int(red)-int(blue))<100) {
                const auto value=std::max(red,blue);
                for(unsigned channel=0;channel<3;++channel)cloth.rgba[at+channel]=static_cast<std::uint8_t>(rgb[channel]*value/238);
            }
        }
        cloth=tex::resize_stretch(cloth,1024,4096);
        const auto badge=tex::resize_stretch(pixels,928,928);
        for(unsigned y=0;y<928;++y)for(unsigned x=0;x<928;++x) {
            const auto from=(std::size_t(y)*928+x)*4,to=(std::size_t(y+448)*1024+x+48)*4;
            const unsigned alpha=badge.rgba[from+3];
            for(unsigned channel=0;channel<3;++channel)
                cloth.rgba[to+channel]=static_cast<std::uint8_t>((badge.rgba[from+channel]*alpha+cloth.rgba[to+channel]*(255-alpha)+127)/255);
        }
        auto& fabric=resource(project,projects::ResourceKind::ebx,"World/ReSkate/Throwdowns/Flags/skate_Fabric");
        const auto fabricDoc=sdk::ebx::read_document(fabric.payload);
        const auto& textureRef=std::get<sdk::ebx::ResourceReference>(ui::require_field(*fabricDoc.root()->object,"Resource").value.data);
        auto at=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::resource && r.resourceId==textureRef.id;});
        if(at==project.resources.end())throw std::runtime_error("Flag texture resource missing");
        const auto info=sdk::resources::read_texture_resource(at->payload,at->resourceMeta);
        auto& chunk=resource(project,projects::ResourceKind::chunk,info.chunkId.string());
        auto encoded=tex::encode_surface(cloth,info,true,false);
        if(encoded.replacedFormat || encoded.payload.size()!=chunk.payload.size())throw std::runtime_error("Flag replacement changed format/mip layout");
        chunk.payload=std::move(encoded.payload);
        preview(output.parent_path()/"reskate-flag-native-preview.png",{info,chunk.payload});
        project.metadata.version="1.0.2";
        projects::save_project(project,output);
        auto mod=output;mod.replace_extension(".fbmod");
        reskate::modding::mods::write_fbmod(project,mod,args[1],&db);
        std::cout<<"resources="<<project.resources.size()<<" logo_variants="<<variants.size()
            <<" flag_chunk="<<info.chunkId.string()<<" identities_and_bundle_membership_preserved=true\n";
        return 0;
    }catch(const std::exception& error){std::cerr<<error.what()<<'\n';return 1;}
}
