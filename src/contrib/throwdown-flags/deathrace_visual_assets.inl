// Refine only the six private guide/required gate roots and measured HUD logo.
// The gate shader uses additive colour: scaling its RGB also reduces its
// apparent opacity, without introducing an unsupported alpha shader input.
bool run_deathrace_visual_assets(int count,const char* const* args) {
    if(std::string_view(args[1])!="deathrace-visual-assets")return false;
    if(count!=5)throw std::runtime_error("deathrace-visual-assets <game-root> <input.fbproject> <output.fbproject>");
    namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
    auto project=projects::load_project(args[3]);
    const auto defaults=require_deathrace_picker_contract(project);
    unsigned gates{},guides{},logos{};
    for(auto& resource:project.resources) {
        const bool guide=resource.name.starts_with("World/ReSkate/Deathrace/") && resource.name.ends_with("_Guide");
        const bool gate=resource.name.starts_with("World/ReSkate/Deathrace/") && resource.name.ends_with("_Gate");
        const bool logo=resource.name=="UI/ReSkate/Throwdowns/HUDLogo_Widget";
        if(resource.kind!=projects::ResourceKind::ebx || !(guide || gate || logo))continue;
        auto doc=sdk::ebx::read_document(resource.payload);sdk::ebx::decode_boxed_values(doc,resource.payload);
        ebx::Partition copy(doc,false);
        if(guide || gate) {
            auto* block=copy.find_instance("ExpressionShaderParameterBlockEntityData");
            if(!block || !block->object)throw std::runtime_error("private gate shader block missing");
            unsigned vectors{};
            for(auto& value:ui::require_array(ui::require_field(*block->object,"EntryData").value,"gate parameters")) {
                auto& entry=ui::require_object(value,"parameter");
                auto* box=sdk::ebx::resolve_boxed(copy.document(),std::get<sdk::ebx::BoxedReference>(ui::require_field(entry,"Value").value.data));
                if(!box)throw std::runtime_error("gate parameter missing");
                if(box->rawBytes.size()==1)continue;
                if(box->rawBytes.size()!=16 || sdk::ebx::boolean(ui::require_field(entry,"Static").value)!=true)
                    throw std::runtime_error("private gate static colour ABI differs");
                std::array<float,4> color{};std::memcpy(color.data(),box->rawBytes.data(),sizeof(color));
                for(unsigned i=0;i<3;++i)color[i]=guide?.22f:color[i]*.18f;
                std::memcpy(box->rawBytes.data(),color.data(),sizeof(color));
                if(box->decoded) {
                    auto& rgb=ui::require_object(*box->decoded,"gate RGB");
                    for(unsigned i=0;i<3;++i)ui::require_field(rgb,std::array{"x","y","z"}[i]).value.data=double(color[i]);
                }
                ++vectors;
            }
            if(!vectors)throw std::runtime_error("gate colour not authored");
            guide?++guides:++gates;
        } else {
            unsigned frames{};
            for(auto& instance:copy.document().instances)if(instance.object) {
                const auto type=sdk::ebx::type_name(copy.document(),instance);
                if(type!="RimeWidgetEntityData" && type!="RimeTextureElementData")continue;
                for(const auto* key:{"Width","Height"})ui::require_field(*instance.object,key).value.data=96.0;
                for(const auto* key:{"FitWidthToContent","FitHeightToContent"})ui::require_field(*instance.object,key).value.data=false;
                ++frames;
            }
            if(frames!=3)throw std::runtime_error("private HUD logo needs root and both native texture frames");
            ++logos;
        }
        const auto authored=ui::emit(resource.name,copy);
        if(authored.partitionGuid!=resource.assetGuid)throw std::runtime_error("private asset identity changed");
        resource.payload=authored.bytes;
    }
    if(gates!=3 || guides!=3 || logos!=1)throw std::runtime_error("expected six gate roots and one HUD logo");
    const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());projects::save_project(project,out);
    if(require_deathrace_picker_contract(projects::load_project(out))!=defaults)throw std::runtime_error("Throwdown picker changed");
    auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"guide_roots=3 gray_additive_gain=0.22 mandatory_roots=3 additive_gain=0.18 logo_frames=3x96 picker_contract=preserved\nmod="<<mod.string()<<'\n';
    return true;
}
