// Native queue registration selects a prefab by Series + InstanceId. Each
// custom entry keeps the shipped gameplay prefab and selects its own flag.
bool run_native_throwdown_flags(int count,const char* const* args) {
    if(std::string_view(args[1])=="flag-preview-info") {
        if(count!=4)throw std::runtime_error("flag-preview-info <game-root> <output-dir>");
        namespace sdk=reskate::sdk;
        namespace ui=reskate::modding::maps::ui;
        const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
        const auto asset=db.read(ui::require_asset(db,"ecs/activities/throwdowns/spotbattlethrowdownerinteractablevalidator_ecsprefab"));
        const auto preview=sdk::ebx::read_document(asset);
        const auto& registrationOwner=ui::require_asset(db,"gameplay/activity/prefabs/throwdowns/spotbattlethrowdownregistrationpoint_ecsprefab");
        const auto registration=sdk::ebx::read_document(db.read(registrationOwner));
        sdk::assets::EbxIdentityResolver identities(db,registrationOwner);
        for(const auto i:{1U,4U}) {
            const auto guid=registration.imports.at(i).fileGuid;
            if(const auto* a=identities.find(guid)) {
                const auto doc=sdk::ebx::read_document(db.read(*a));
                std::cout<<"flag_icon_import="<<i<<" name="<<a->name<<" type="<<sdk::ebx::type_name(doc,*doc.root())<<'\n';
            } else throw std::runtime_error("native flag icon identity unresolved");
        }
        const std::filesystem::path out(args[3]);std::filesystem::create_directories(out);
        for(const auto& a:db.assets)if(a.kind==sdk::assets::AssetKind::ebx && a.name.starts_with("flow/statemachines/gamemodes/flow_throwdowner/")) {
            const auto bytes=db.read(a);auto doc=sdk::ebx::read_document(bytes);
            if(std::ranges::none_of(doc.imports,[&](const auto& ref){return ref.fileGuid==preview.fileGuid;}))continue;
            std::cout<<"preview_consumer="<<a.name<<'\n';
            const auto* res=db.find(a.name,sdk::assets::AssetKind::resource);
            if(res) {sdk::io::BinaryWriter file;file.bytes(db.read(*res));file.save(out/(std::filesystem::path(a.name).filename().string()+".bin"));}
        }
        return true;
    }
    if(std::string_view(args[1])!="native-throwdown-flags")return false;
    if(count!=6)throw std::runtime_error("native-throwdown-flags <game-root> <input.fbproject> <badge-dir> <output.fbproject>");
    namespace sdk=reskate::sdk;
    namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;
    namespace tex=reskate::modding::textures;
    namespace mat=reskate::modding::materials;
    namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
    auto project=projects::load_project(args[3]);
    const auto content=std::find_if(project.resources.begin(),project.resources.end(),[](const auto& r){
        return r.name=="UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources";
    });
    if(content==project.resources.end() || content->bundles.empty())throw std::runtime_error("native Throwdown bundles missing");
    auto bundles=content->bundles;
    constexpr auto previewName="ecs/activities/throwdowns/spotbattlethrowdownerinteractablevalidator_ecsprefab";
    std::vector<std::string> previewBundles;
    for(const auto index:ui::require_asset(db,previewName).bundles) {
        const auto& bundle=db.bundles.at(index).name;
        previewBundles.push_back(bundle);
        if(std::ranges::find(bundles,bundle)==bundles.end())bundles.push_back(bundle);
    }
    if(previewBundles.empty())throw std::runtime_error("placement preview has no native owning bundle");
    constexpr const char* networkNames[]{
        "levels/game/bam_levelroot/bam_coregameassets_networkregistry_win32",
        "levels/game/dingolevel_ftue_island/dingolevel_ftue_island_networkregistry_win32",
        "levels/game/dingolevel_isle_of_grom/dingolevel_isle_of_grom_networkregistry_win32",
        "levels/game/dingolevel_mpr/dingolevel_mpr_networkregistry_win32",
        "levels/game/dingolevel_sdm/dingolevel_sdm_int_001/dingolevel_sdm_int_001_networkregistry_win32",
        "levels/game/dingolevel_sdm/dingolevel_sdm_int_002/dingolevel_sdm_int_002_networkregistry_win32"};
    // NetworkRegistryAsset's loader eagerly dereferences every Objects entry.
    // Its private prefab imports must be resident before the registry loads,
    // not deferred until the player opens the Throwdown picker UI bundle.
    for(const auto name:networkNames)for(const auto index:ui::require_asset(db,name).bundles) {
        const auto& bundle=db.bundles.at(index).name;
        if(std::ranges::find(bundles,bundle)==bundles.end())bundles.push_back(bundle);
    }
    const auto donor=[&](std::string_view name){const auto bytes=db.read(ui::require_asset(db,name));
        auto doc=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(doc,bytes);return doc;};
    const auto add=[&](projects::ResourceKind kind,const std::string& name,std::vector<std::byte> bytes,bool added)->projects::ProjectResource& {
        if(std::ranges::any_of(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;}))
            throw std::runtime_error("flag author would replace an existing project resource: "+name);
        projects::ProjectResource r;r.kind=kind;r.name=name;r.payload=std::move(bytes);r.bundles=bundles;r.added=added;
        project.resources.push_back(std::move(r));return project.resources.back();
    };
    const auto emit=[&](const std::string& name,ebx::Partition& part,bool added=true){
        ui::require_field(*part.root()->object,"Name").value.data=name;
        auto authored=ui::emit(name,part);
        auto& resource=add(projects::ResourceKind::ebx,name,authored.bytes,added);
        resource.assetGuid=authored.partitionGuid;
        if(!added) {
            // A level registry must never be copied into another level's
            // bundle: its native Objects imports are scoped to its own world.
            resource.bundles.clear();
            for(const auto index:ui::require_asset(db,name).bundles)resource.bundles.push_back(db.bundles.at(index).name);
        }
        return authored;
    };
    constexpr auto registrationName="gameplay/activity/prefabs/throwdowns/spotbattlethrowdownregistrationpoint_ecsprefab";
    constexpr auto meshComponentName="500a7393-be99-b44e-045e-e59a39fc60491826542287_ecsmeshcomponentasset10287781863110467137";
    constexpr auto variationName="world/props/common/throwdown/cmn_prp_throwdown_banner_01/cmn_prp_svcc_banner_01_v_throwdown_spotbattle_01";
    constexpr auto fabricName="world/materials/signage/svcc/signage_svcc_flag_td_spotbattle_1x4_01_c";
    const auto registrationDoc=donor(registrationName),componentDoc=donor(meshComponentName),variationDoc=donor(variationName);
    const auto previewDoc=donor(previewName);
    sdk::assets::EbxIdentityResolver flagIcons(db,ui::require_asset(db,registrationName));
    for(const auto i:{1U,4U}) {
        const auto* native=flagIcons.find(registrationDoc.imports.at(i).fileGuid,registrationDoc.imports.at(i).classGuid);
        if(!native)throw std::runtime_error("native registration icon import missing");
        const auto doc=donor(native->name);
        if(sdk::ebx::type_name(doc,*doc.root())!="RimeTextureAsset")throw std::runtime_error("native registration icon type differs");
    }
    // Level network registries load these prefabs before the picker UI. Make
    // the chosen Rime picture, texture resources and chunks resident there.
    std::set<std::pair<projects::ResourceKind,std::string>> logoDependencies;
    std::function<void(projects::ResourceKind,const std::string&)> includeLogo;
    includeLogo=[&](const auto kind,const auto& name) {
        if(!logoDependencies.emplace(kind,name).second)return;
        auto resource=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;});
        if(resource==project.resources.end())throw std::runtime_error("custom logo dependency missing: "+name);
        for(const auto& bundle:bundles)if(std::ranges::find(resource->bundles,bundle)==resource->bundles.end())resource->bundles.push_back(bundle);
        if(kind==projects::ResourceKind::ebx) {
            const auto doc=sdk::ebx::read_document(resource->payload);
            for(const auto& imported:doc.imports) {
                const auto child=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::ebx && r.assetGuid==imported.fileGuid;});
                if(child==project.resources.end())throw std::runtime_error("custom logo picture imports an unavailable texture");
                includeLogo(child->kind,child->name);
            }
            std::function<void(const sdk::ebx::Value&)> collect;
            collect=[&](const auto& value) {
                if(const auto* ref=std::get_if<sdk::ebx::ResourceReference>(&value.data);ref && ref->id) {
                    const auto child=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::resource && r.resourceId==ref->id;});
                    if(child==project.resources.end())throw std::runtime_error("custom logo texture resource unavailable");
                    includeLogo(child->kind,child->name);
                }
                if(const auto* obj=sdk::ebx::object(value))for(const auto& field:obj->fields)collect(field.value);
                if(const auto* array=sdk::ebx::array(value))for(const auto& item:*array)collect(item);
            };
            for(const auto& instance:doc.instances)if(instance.object)for(const auto& field:instance.object->fields)collect(field.value);
        } else if(kind==projects::ResourceKind::resource && resource->resourceType==sdk::resources::textureResourceType) {
            const auto texture=sdk::resources::read_texture_resource(resource->payload,resource->resourceMeta);
            const auto child=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::chunk && r.chunkId==texture.chunkId;});
            if(child==project.resources.end())throw std::runtime_error("custom logo texture chunk unavailable");
            includeLogo(child->kind,child->name);
        }
    };
    const auto source=tex::read_clone_source(db,ui::require_asset(db,fabricName));
    const auto nativePixels=tex::decode_png(tex::texture_to_png(source.loaded));
    if(nativePixels.width!=256 || nativePixels.height!=1024)throw std::runtime_error("native flag UV dimensions differ");
    std::cout<<"native_flag_texture="<<source.loaded.resource.format.name<<" first_mip="<<unsigned(source.loaded.resource.firstMip)<<'\n';
    const auto flagTexture=[&](const std::string& name,const tex::Image& pixels) {
        auto texture=tex::clone_texture_image(source,name,nativePixels,-1,true);
        auto info=source.loaded.resource;
        info.width=1024;info.height=4096;info.firstMip=0;info.mipCount=13;
        info.mipSizes.fill(0);
        unsigned width=info.width,height=info.height;
        for(unsigned mip=0;mip<info.mipCount;++mip) {
            info.mipSizes[mip]=static_cast<std::uint32_t>(info.format.mip_size(width,height));
            width=std::max(1U,width/2);height=std::max(1U,height/2);
        }
        auto encoded=tex::encode_surface(pixels,info,true,false);
        texture.texture.chunk=std::move(encoded.payload);
        texture.texture.width=info.width;texture.texture.height=info.height;
        texture.texture.mipSizes.assign(info.mipSizes.begin(),info.mipSizes.begin()+info.mipCount);
        const auto write=[&](std::size_t at,auto value) {
            if(at+sizeof(value)>texture.texture.resource.size())throw std::runtime_error("flag texture header truncated");
            std::memcpy(texture.texture.resource.data()+at,&value,sizeof(value));
        };
        const std::size_t dimensions=info.version>=12?0x16:0x12;
        write(dimensions,info.width);write(dimensions+2,info.height);
        texture.texture.resource[dimensions+8]=static_cast<std::byte>(info.mipCount);
        texture.texture.resource[dimensions+9]=std::byte{};
        for(unsigned mip=0;mip<info.mipSizes.size();++mip)
            write((info.extendedLayout?0x38:0x30)+mip*4,info.mipSizes[mip]);
        if(info.extendedLayout)texture.texture.resource[0x21]=static_cast<std::byte>(info.mipCount);
        write(info.extendedLayout?0x74:0x6c,static_cast<std::uint32_t>(texture.texture.chunk.size()));
        const auto verified=sdk::resources::read_texture_resource(texture.texture.resource,texture.texture.resourceMeta);
        if(verified.width!=1024 || verified.height!=4096 || verified.firstMip || verified.mipCount!=13 || verified.mipSizes!=info.mipSizes)
            throw std::runtime_error("high resolution resident flag mip layout differs");
        return texture;
    };
    const mat::TextureBinding original{source.document.fileGuid,source.document.root()->instanceGuid};
    constexpr std::uint64_t opaqueBase=0xac2054d49ce97317ULL,shadowBase=0xfd8dbcc823ff91d5ULL;
    constexpr std::uint32_t stockHash=2602416751U;
    auto opaque=mat::ShaderDepotAuthor::find(db,opaqueBase+stockHash,{},mat::TextureBindingMode::preserveNative);
    auto shadow=mat::ShaderDepotAuthor::find(db,shadowBase+stockHash,{},mat::TextureBindingMode::preserveNative);
    struct Mode {const char* key;const char* badge;std::uint32_t rgba;};
    // These RGBA bytes match challenge_registry.cpp and the Throwdown atlas.
    constexpr Mode modes[]{
        {"oneup","img_OneUp_Blue_1024_1024_1024-preview.png",0xffef8b22U},
        {"jam","img_jam_Placeholder_1024_1024-preview.png",0xffab4fffU},
        {"meat","img_meat_Placeholder_1024_1024-preview.png",0xff218affU},
        {"race","img_race_Placeholder_1024_1024-preview.png",0xff3434ffU},
        {"domination","img_domination_Placeholder_1024_1024-preview.png",0xffff558cU},
        {"graffiti","img_graffiti_Placeholder_1024_1024-preview.png",0xff9fd929U},
        {"tag","img_tag_Placeholder_1024_1024-preview.png",0xff31d3ffU},
        {"infection","img_infection_Placeholder_1024_1024-preview.png",0xff24d675U},
        {"hide","img_hide_Placeholder_1024_1024-preview.png",0xffffbd39U}};
    const std::filesystem::path output(args[5]);std::filesystem::create_directories(output.parent_path());
    std::vector<std::uint64_t> opaqueKeys,shadowKeys;
    std::vector<mat::MaterialKeyAlias> aliases;
    std::vector<mat::TextureBinding> fabricBindings;
    std::vector<ebx::AuthoredPartition> registrations;
    std::vector<std::uint32_t> variationHashes;
    for(const auto& mode:modes) {
        const auto stem="World/ReSkate/Throwdowns/Flags/"+std::string(mode.key);
        auto pixels=nativePixels;
        // Pack the existing approved badge into the native cloth UV region.
        // Fill the old badge from a clean fabric strip before recoloring; keep
        // the native distress, SVCC graphics, alpha, pole and fabric normals.
        for(unsigned y=96;y<360;++y)for(unsigned x=0;x<256;++x) {
            const auto from=(std::size_t(32+(y-96)%56)*256+x)*4,to=(std::size_t(y)*256+x)*4;
            std::copy_n(nativePixels.rgba.begin()+from,4,pixels.rgba.begin()+to);
        }
        const std::array<unsigned,3> rgb{mode.rgba&255,(mode.rgba>>8)&255,(mode.rgba>>16)&255};
        const auto peak=*std::max_element(rgb.begin(),rgb.end());
        for(std::size_t p=0;p<pixels.rgba.size();p+=4) {
            const auto red=pixels.rgba[p],green=pixels.rgba[p+1],blue=pixels.rgba[p+2];
            if(red>80 && blue>80 && green<std::max(red,blue)*.82 && std::abs(int(red)-int(blue))<100) {
                const auto value=std::max(red,blue);
                for(unsigned c=0;c<3;++c)pixels.rgba[p+c]=static_cast<std::uint8_t>(rgb[c]*value/peak);
            }
        }
        pixels=tex::resize_stretch(pixels,1024,4096);
        const auto badge=tex::resize_stretch(tex::load_artwork_image(std::filesystem::path(args[4])/mode.badge),928,928);
        for(unsigned y=0;y<928;++y)for(unsigned x=0;x<928;++x) {
            const auto from=(std::size_t(y)*928+x)*4,to=(std::size_t(y+448)*1024+x+48)*4;
            const unsigned alpha=badge.rgba[from+3];
            for(unsigned c=0;c<3;++c)pixels.rgba[to+c]=static_cast<std::uint8_t>((badge.rgba[from+c]*alpha+pixels.rgba[to+c]*(255-alpha)+127)/255);
        }
        auto texture=flagTexture(stem+"_Fabric",pixels);
        const sdk::resources::LoadedTexture encoded{sdk::resources::read_texture_resource(texture.texture.resource,texture.texture.resourceMeta),texture.texture.chunk};
        sdk::io::BinaryWriter preview;preview.bytes(tex::texture_to_png(encoded));preview.save(output.parent_path()/(std::string(mode.key)+"-flag-preview.png"));
        const mat::TextureBinding replacement{texture.fileGuid,texture.rootGuid};
        fabricBindings.push_back(replacement);
        add(projects::ResourceKind::ebx,texture.name,std::move(texture.ebx),true).assetGuid=texture.fileGuid;
        auto& res=add(projects::ResourceKind::resource,texture.name,std::move(texture.texture.resource),true);
        res.resourceId=texture.resourceId;res.resourceType=sdk::resources::textureResourceType;res.resourceMeta=std::move(texture.texture.resourceMeta);
        auto& chunk=add(projects::ResourceKind::chunk,texture.chunkId.string(),std::move(texture.texture.chunk),true);
        chunk.chunkId=texture.chunkId;chunk.logicalSize=static_cast<std::uint32_t>(chunk.payload.size());chunk.firstMip=0;chunk.h32=static_cast<std::int32_t>(tex::texture_name_hash(texture.name));
        ebx::Partition variation(variationDoc);
        const auto variationAssetName=stem+"_Variation";
        std::uint32_t hash=5381;for(unsigned char c:variationAssetName)hash=hash*33U^static_cast<unsigned char>(std::tolower(c));
        if(hash==stockHash || std::ranges::find(variationHashes,hash)!=variationHashes.end())throw std::runtime_error("flag variation hash collision");
        variationHashes.push_back(hash);ui::require_field(*variation.root()->object,"NameHash").value.data=std::uint64_t(hash);
        const auto variationAsset=emit(variationAssetName,variation);
        ebx::Partition component(componentDoc);
        ui::require_field(*component.root()->object,"ObjectVariation").value.data=component.import(variationAsset.partitionGuid,variationAsset.rootGuid);
        const auto componentAsset=emit(stem+"_MeshComponent",component);
        // ECS entity/path GUIDs are scoped by prefab and must continue to
        // match EntityTable entries. Give the partition/root a private identity
        // while retaining the native internal path GUIDs and variant graph.
        ebx::Partition registration(registrationDoc,false);
        registration.document().fileGuid=ebx::random_guid();registration.root()->instanceGuid=ebx::random_guid();
        const auto oldComponent=componentDoc.root()->instanceGuid;
        std::size_t replaced{};
        for(auto& imported:registration.document().imports)if(imported.fileGuid==componentDoc.fileGuid && imported.classGuid==oldComponent) {
            imported.fileGuid=componentAsset.partitionGuid;imported.classGuid=componentAsset.rootGuid;++replaced;
        }
        if(replaced!=1)throw std::runtime_error("native registration flag mesh override missing");
        const auto logoName=std::string(mode.key)=="oneup"?std::string("UI/ReSkate/OneUp/img_OneUp_Blue_1024"):
            "UI/ReSkate/Throwdowns/img_"+std::string(mode.key)+"_Placeholder";
        const auto logo=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==projects::ResourceKind::ebx && r.name==logoName;});
        if(logo==project.resources.end())throw std::runtime_error("custom flag logo missing: "+logoName);
        const auto logoDoc=sdk::ebx::read_document(logo->payload);
        if(sdk::ebx::type_name(logoDoc,*logoDoc.root())!="RimeTextureAsset")throw std::runtime_error("custom flag logo type differs");
        includeLogo(projects::ResourceKind::ebx,logoName);
        for(const auto i:{1U,4U})registration.document().imports.at(i)={logoDoc.fileGuid,logoDoc.root()->instanceGuid};
        const auto branded=emit(stem+"_Registration",registration);
        const auto verifiedFlag=sdk::ebx::read_document(branded.bytes);
        if(verifiedFlag.instances.size()!=registrationDoc.instances.size() || verifiedFlag.imports.size()!=registrationDoc.imports.size())
            throw std::runtime_error("branded flag registration layout differs");
        for(const auto i:{1U,4U})if(verifiedFlag.imports.at(i).fileGuid!=logoDoc.fileGuid || verifiedFlag.imports.at(i).classGuid!=logoDoc.root()->instanceGuid)
            throw std::runtime_error("serialized distant flag logo identity differs");
        registrations.push_back(branded);
        // Placement has a client-only validator prefab, separate from the
        // networked registration. Keep its native validation/physics graph;
        // only the cloth mesh variation changes. Load it with the stock
        // preview's bundles so all native validation imports are resident.
        ebx::Partition placementPreview(previewDoc,false);
        placementPreview.document().fileGuid=ebx::random_guid();
        placementPreview.root()->instanceGuid=ebx::random_guid();
        std::size_t previewReplaced{};
        for(auto& imported:placementPreview.document().imports)
            if(imported.fileGuid==componentDoc.fileGuid && imported.classGuid==oldComponent) {
                imported.fileGuid=componentAsset.partitionGuid;imported.classGuid=componentAsset.rootGuid;++previewReplaced;
            }
        if(previewReplaced!=1)throw std::runtime_error("native placement preview mesh override missing");
        const auto previewAsset=emit(stem+"_Placement",placementPreview);
        project.resources.back().bundles=previewBundles;
        const auto verifiedPreview=sdk::ebx::read_document(previewAsset.bytes);
        if(verifiedPreview.instances.size()!=previewDoc.instances.size() ||
           verifiedPreview.imports.size()!=previewDoc.imports.size() ||
           std::ranges::count_if(verifiedPreview.imports,[&](const auto& ref) {
               return ref.fileGuid==componentAsset.partitionGuid && ref.classGuid==componentAsset.rootGuid;
           })!=1)throw std::runtime_error("serialized placement preview differs");
        opaqueKeys.push_back(opaque.add_material_replacing(stem+"_Opaque",opaqueBase+hash,original,replacement));
        shadowKeys.push_back(shadow.add_material_replacing(stem+"_Shadow",shadowBase+hash,original,replacement));
        aliases.push_back({opaqueBase+stockHash,opaqueKeys.back()});aliases.push_back({shadowBase+stockHash,shadowKeys.back()});
        std::cout<<"custom_flag="<<mode.key<<" rgba="<<std::hex<<mode.rgba<<std::dec<<" variation="<<hash<<" instance=ReSkate."<<mode.key<<'\n';
    }
    for(const auto& pair:{std::pair{&opaque,&opaqueKeys},std::pair{&shadow,&shadowKeys}}) {
        auto depot=pair.first->build_subset(pair.first==&opaque?"world/reskate/throwdowns/flags/opaque":"world/reskate/throwdowns/flags/shadow",*pair.second);
        const auto* stockDepot=db.find(pair.first->donor_name(),sdk::assets::AssetKind::resource);
        if(!stockDepot)throw std::runtime_error("flag shader donor missing");
        const std::array stockKey{pair.first->source_key()};
        const auto stock=mat::read_material_bindings(db.read(*stockDepot),stockDepot->resourceMeta,stockKey).at(stockKey.front());
        const auto verified=mat::read_material_bindings(depot.resource,depot.resourceMeta,*pair.second);
        if(verified.size()!=std::size(modes))throw std::runtime_error("private flag shader row count differs");
        for(std::size_t i=0;i<std::size(modes);++i) {
            const auto& actual=verified.at(pair.second->at(i));std::size_t badgeParameters{};
            if(actual.parameters.size()!=stock.parameters.size())throw std::runtime_error("flag shader parameter count differs");
            for(const auto& [guid,before]:stock.parameters) {
                const auto& after=actual.parameters.at(guid);
                if(before.type!=after.type)throw std::runtime_error("flag shader parameter type differs");
                bool badge{};
                if(before.type==0x14c5fc30 && before.value.size()>=32) {
                    const auto target=mat::decode_texture_parameter(before);
                    badge=target.fileGuid==original.partition && target.classGuid==original.instance;
                }
                if(badge) {
                    const auto target=mat::decode_texture_parameter(after);
                    if(target.fileGuid!=fabricBindings[i].partition || target.classGuid!=fabricBindings[i].instance)
                        throw std::runtime_error("private fabric texture binding differs");
                    ++badgeParameters;
                } else if(before.value!=after.value)throw std::runtime_error("flag clone changed a pole/fabric shader parameter");
            }
            if(badgeParameters!=1)throw std::runtime_error("flag badge parameter verification differs");
        }
        auto& res=add(projects::ResourceKind::resource,depot.name,std::move(depot.resource),true);
        res.resourceId=ebx::random_resource_id();res.resourceType=mat::shaderBlockDepotResourceType;res.resourceMeta=std::move(depot.resourceMeta);
    }
    ebx::Partition variationDb(donor("levels/game/bam_levelroot/gameplay_activities/meshvariationdb_win32"));
    auto& entries=ui::require_array(ui::require_field(*variationDb.root()->object,"Entries").value,"flag variation database");
    const auto entry=std::find_if(entries.begin(),entries.end(),[&](auto& value){return sdk::ebx::integer(ui::require_field(ui::require_object(value,"variation"),"VariationAssetNameHash").value)==stockHash;});
    if(entry==entries.end())throw std::runtime_error("native flag mesh variation missing");
    const auto prototype=ebx::clone_value_tree(*entry);entries.clear();
    for(const auto hash:variationHashes) {
        auto value=ebx::clone_value_tree(prototype);ui::require_field(ui::require_object(value,"variation"),"VariationAssetNameHash").value.data=std::uint64_t(hash);entries.push_back(std::move(value));
    }
    variationDb.prune_unreachable();emit("World/ReSkate/Throwdowns/Flags/MeshVariationDb_Win32",variationDb);
    constexpr auto registryName="activities/throwdowns/prefabs/throwdownsprefabregistrator_ecsprefab.nopolicy";
    ebx::Partition registry(donor(registryName),false);
    std::size_t extended{};
    for(auto& box:registry.document().boxedValues)if(box.decoded) {
        if(!sdk::ebx::object(*box.decoded))continue;
        auto* object=&ui::require_object(*box.decoded,"prefab registry boxed value");
        const auto* series=sdk::ebx::find_field(*object,"MpEventSeriesId");
        if(!series || !sdk::ebx::text(series->value) || *sdk::ebx::text(series->value)!="SpotBattle")continue;
        auto& prefabs=ui::require_array(ui::require_field(*object,"MpEventPrefabs").value,"flag prefab registry");
        if(prefabs.size()!=1)throw std::runtime_error("stock Spot Battle prefab registry differs");
        const auto stock=ebx::clone_value_tree(prefabs.front());
        for(std::size_t i=0;i<registrations.size();++i) {
            auto value=ebx::clone_value_tree(stock);auto& fields=ui::require_object(value,"custom flag prefab");
            ui::require_field(fields,"InstanceId").value.data="ReSkate."+std::string(modes[i].key);
            ui::require_field(fields,"RegistrationPrefab").value.data=registry.import(registrations[i].partitionGuid,registrations[i].rootGuid);
            prefabs.push_back(std::move(value));
        }
        ++extended;
    }
    if(extended!=1)throw std::runtime_error("must extend exactly one Spot Battle prefab series");
    const auto registryAsset=emit(registryName,registry,false);
    auto verifiedRegistry=sdk::ebx::read_document(registryAsset.bytes);sdk::ebx::decode_boxed_values(verifiedRegistry,registryAsset.bytes);
    std::size_t verifiedEntries{};
    for(auto& box:verifiedRegistry.boxedValues)if(box.decoded && sdk::ebx::object(*box.decoded)) {
        auto& object=ui::require_object(*box.decoded,"verified registry");const auto* field=sdk::ebx::find_field(object,"MpEventSeriesId");
        if(!field || !sdk::ebx::text(field->value) || *sdk::ebx::text(field->value)!="SpotBattle")continue;
        auto& values=ui::require_array(ui::require_field(object,"MpEventPrefabs").value,"serialized flag prefabs");
        if(values.size()!=10)throw std::runtime_error("serialized registry lost private prefabs");
        for(std::size_t i=0;i<values.size();++i) {
            auto& value=ui::require_object(values[i],"serialized prefab");
            const auto expected=i?"ReSkate."+std::string(modes[i-1].key):std::string("SpotBattle");
            if(*sdk::ebx::text(ui::require_field(value,"InstanceId").value)!=expected)throw std::runtime_error("serialized flag instance selector differs");
            const auto* target=sdk::ebx::resolve_import(verifiedRegistry,ui::require_pointer(ui::require_field(value,"RegistrationPrefab").value,"registration"));
            const auto file=i?registrations[i-1].partitionGuid:registrationDoc.fileGuid;
            const auto root=i?registrations[i-1].rootGuid:registrationDoc.root()->instanceGuid;
            if(!target || target->fileGuid!=file || target->classGuid!=root)throw std::runtime_error("serialized flag registration identity differs");
            ++verifiedEntries;
        }
    }
    if(verifiedEntries!=10)throw std::runtime_error("serialized registry verification incomplete");
    for(const auto name:networkNames) {
        ebx::Partition network(donor(name),false);
        auto& objects=ui::require_array(ui::require_field(*network.root()->object,"Objects").value,"native network registry");
        for(const auto& registration:registrations)objects.push_back(sdk::ebx::Value{network.import(registration.partitionGuid,registration.rootGuid)});
        emit(name,network,false);
    }
    for(const auto type:{mat::shaderProgramLookupResourceType,mat::shaderTextureLookupResourceType}) {
        const sdk::assets::NativeAsset* asset{};
        for(const auto& candidate:db.assets)if(candidate.kind==sdk::assets::AssetKind::resource && candidate.resourceType==type &&
            candidate.name.starts_with("levels/game/dingolevel_root/dingolevel_root_win32_shaderstate_")) {if(asset)throw std::runtime_error("ambiguous root shader lookup");asset=&candidate;}
        if(!asset)throw std::runtime_error("root shader lookup missing");
        auto lookup=type==mat::shaderProgramLookupResourceType?mat::alias_program_lookup(db.read(*asset),asset->resourceMeta,aliases):mat::alias_texture_lookup(db.read(*asset),asset->resourceMeta,aliases);
        if(lookup.aliasesAdded!=aliases.size())throw std::runtime_error("private flag material lookup incomplete");
        auto& resource=add(projects::ResourceKind::resource,asset->name,std::move(lookup.resource),false);
        resource.resourceType=asset->resourceType;resource.resourceId=asset->resourceId;resource.resourceMeta=std::move(lookup.resourceMeta);
        resource.bundles.clear();for(const auto index:asset->bundles)resource.bundles.push_back(db.bundles.at(index).name);
    }
    // Native dependencies join the UI bundle unchanged. Include only the flag
    // mesh/fabric/pole dependencies, never an entire shared world shader depot.
    std::vector<const sdk::assets::NativeAsset*> pending;
    std::set<std::pair<sdk::assets::AssetKind,std::string>> included;
    const auto include=[&](const sdk::assets::NativeAsset* asset){if(!asset)throw std::runtime_error("native flag dependency missing");
        if(included.emplace(asset->kind,asset->kind==sdk::assets::AssetKind::chunk?asset->guid.string():asset->name).second)pending.push_back(asset);};
    include(&ui::require_asset(db,"world/props/common/svcc/cmn_prp_svcc_banner_01/cmn_prp_svcc_banner_01_mesh"));
    for(const auto key:{opaqueBase+stockHash,shadowBase+stockHash})for(const auto& parameter:mat::dump_block_parameters(db,key))
        if(!parameter.texture.empty())include(&ui::require_asset(db,parameter.texture));
    for(std::size_t i=0;i<pending.size();++i) {
        const auto* asset=pending[i];
        if(asset->kind==sdk::assets::AssetKind::ebx) {
            const auto doc=donor(asset->name);
            std::function<void(const sdk::ebx::Value&)> collect;
            collect=[&](const auto& value){
                if(const auto* ref=std::get_if<sdk::ebx::ResourceReference>(&value.data);ref && ref->id)include(db.find_resource(ref->id));
                if(const auto* obj=sdk::ebx::object(value))for(const auto& f:obj->fields)collect(f.value);
                if(const auto* array=sdk::ebx::array(value))for(const auto& v:*array)collect(v);
            };
            for(const auto& instance:doc.instances)if(instance.object)for(const auto& field:instance.object->fields)collect(field.value);
        } else if(asset->kind==sdk::assets::AssetKind::resource && asset->resourceType==sdk::resources::textureResourceType) {
            include(db.find_chunk(sdk::resources::read_texture_resource(db.read(*asset),asset->resourceMeta).chunkId));
        } else if(asset->kind==sdk::assets::AssetKind::resource && asset->resourceType==sdk::resources::meshSetResourceType) {
            for(unsigned lod=0;lod<3;++lod)(void)sdk::resources::read_mesh_set("native flag",asset->name,db.read(*asset),[&](const auto& id,std::size_t){const auto* chunk=db.find_chunk(id);include(chunk);return db.read(*chunk);},false,lod);
        }
        const auto kind=asset->kind==sdk::assets::AssetKind::ebx?projects::ResourceKind::ebx:asset->kind==sdk::assets::AssetKind::resource?projects::ResourceKind::resource:projects::ResourceKind::chunk;
        const auto name=kind==projects::ResourceKind::chunk?asset->guid.string():asset->name;
        if(std::ranges::any_of(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;}))continue;
        auto& resource=add(kind,name,db.read(*asset),false);
        resource.resourceType=asset->resourceType;resource.resourceId=asset->resourceId;resource.resourceMeta=asset->resourceMeta;
        resource.logicalOffset=asset->logicalOffset;resource.logicalSize=asset->logicalSize;
        if(kind==projects::ResourceKind::ebx)resource.assetGuid=sdk::ebx::read_document(resource.payload).fileGuid;
        if(kind==projects::ResourceKind::chunk)resource.chunkId=asset->guid;
    }
    for(const auto& [kind,name]:logoDependencies) {
        const auto resource=std::ranges::find_if(project.resources,[&](const auto& r){return r.kind==kind && r.name==name;});
        for(const auto& bundle:bundles)if(resource==project.resources.end() || std::ranges::find(resource->bundles,bundle)==resource->bundles.end())
            throw std::runtime_error("custom flag logo is not resident in an owning registry bundle");
    }
    projects::save_project(project,output);auto mod=output;mod.replace_extension(".fbmod");
    for(const auto& registration:registrations) {
        const auto resource=std::ranges::find(project.resources,registration.name,&projects::ProjectResource::name);
        if(resource==project.resources.end() || resource->bundles!=bundles)throw std::runtime_error("private flag not resident in every owning registry bundle");
    }
    reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"custom_flags=9 distant_logos=9 logo_dependencies="<<logoDependencies.size()<<" private_materials=18 shader_parameters_verified=true serialized_registry_verified=true native_spot_battle_unchanged=true resources="<<project.resources.size()<<"\nmod="<<mod.string()<<'\n';
    return true;
}
