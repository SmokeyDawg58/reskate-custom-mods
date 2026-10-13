// Gate-only authoring must use the latest complete UI pack. An older project
// can contain the custom logo grid without its resident model templates, making
// the runtime fall back to the three stock cards even though compilation succeeds.
std::map<std::string,std::string> require_deathrace_picker_contract(
    const reskate::modding::projects::Project& project) {
    namespace sdk=reskate::sdk;
    namespace ui=reskate::modding::maps::ui;
    namespace projects=reskate::modding::projects;
    const auto content=std::ranges::find(project.resources,
        "UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources",&projects::ProjectResource::name);
    if(content==project.resources.end() || content->kind!=projects::ResourceKind::ebx)
        throw std::runtime_error("complete Throwdown picker content resource is required");
    auto doc=sdk::ebx::read_document(content->payload);
    sdk::ebx::decode_boxed_values(doc,content->payload);
    const auto defaults=native_defaults(doc);
    const auto& exports=ui::require_array(ui::require_field(*doc.root()->object,"Resources").value,"picker exports");
    for(const auto* suffix:{"LogoGrid","LogoTexture","LogoContents"}) {
        const auto key=std::string("Throwdown_ModeSelect_ContentResources/")+suffix;
        if(!defaults.contains(key))throw std::runtime_error("complete Throwdown picker is required; missing typed default: "+key);
        unsigned exported{};
        for(std::size_t i=0;i<doc.instances.size();++i) {
            const auto& instance=doc.instances[i];
            if(!instance.object || sdk::ebx::type_name(doc,instance)!="DataModelObject")continue;
            const auto* field=sdk::ebx::find_field(*instance.object,"DebugName");
            const auto* name=field?sdk::ebx::text(field->value):nullptr;
            if(!name || *name!=key)continue;
            for(const auto& value:exports) {
                const auto* pointer=sdk::ebx::pointer(value);
                if(pointer && pointer->kind==sdk::ebx::PointerKind::internal && pointer->index==static_cast<std::int64_t>(i))++exported;
            }
        }
        if(exported!=1)throw std::runtime_error("picker record must be exported exactly once: "+key);
    }
    return defaults;
}

#include "cli/deathrace_visual_assets.inl"
#include "cli/deathrace_hud_assets.inl"
bool run_deathrace_guide_assets(int count,const char* const* args) {
    if(run_deathrace_hud_assets(count,args))return true;
    if(run_deathrace_visual_assets(count,args))return true;
    if(std::string_view(args[1])!="deathrace-guide-assets")return false;
    if(count!=5)throw std::runtime_error("deathrace-guide-assets <game-root> <input.fbproject> <output.fbproject>");
    namespace sdk=reskate::sdk;
    namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;
    namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
    auto project=projects::load_project(args[3]);
    const auto pickerDefaults=require_deathrace_picker_contract(project);
    std::vector<projects::ProjectResource> guides;
    for(const auto& source:project.resources) {
        if(source.kind!=projects::ResourceKind::ebx || !source.name.starts_with("World/ReSkate/Deathrace/") || !source.name.ends_with("_Gate"))continue;
        const auto name=source.name.substr(0,source.name.size()-5)+"_Guide";
        if(std::any_of(project.resources.begin(),project.resources.end(),[&](const auto& r){return r.name==name;}))throw std::runtime_error("guide asset already present");
        auto doc=sdk::ebx::read_document(source.payload);sdk::ebx::decode_boxed_values(doc,source.payload);
        ebx::Partition copy(doc);
        auto* block=copy.find_instance("ExpressionShaderParameterBlockEntityData");
        if(!block || !block->object)throw std::runtime_error("guide shader block missing");
        unsigned vectors{};
        for(auto& value:ui::require_array(ui::require_field(*block->object,"EntryData").value,"guide colors")) {
            auto& entry=ui::require_object(value,"guide color");
            auto* box=sdk::ebx::resolve_boxed(copy.document(),std::get<sdk::ebx::BoxedReference>(ui::require_field(entry,"Value").value.data));
            if(!box)throw std::runtime_error("guide color box missing");
            if(box->rawBytes.size()==1)continue;
            if(box->rawBytes.size()!=16 || sdk::ebx::boolean(ui::require_field(entry,"Static").value)!=true)throw std::runtime_error("guide static color ABI differs");
            const std::array<float,4> color{1.f,1.f,1.f,0.f};
            std::memcpy(box->rawBytes.data(),color.data(),sizeof(color));
            if(box->decoded) {
                auto& rgb=ui::require_object(*box->decoded,"guide RGB");
                for(const auto* axis:{"x","y","z"})ui::require_field(rgb,axis).value.data=1.0;
            }
            ++vectors;
        }
        if(!vectors)throw std::runtime_error("guide has no color vectors");
        ui::require_field(*copy.root()->object,"Name").value.data=name;
        auto authored=ui::emit(name,copy);
        const auto check=sdk::ebx::read_document(authored.bytes);
        if(*sdk::ebx::text(ui::require_field(*check.root()->object,"Name").value)!=name)throw std::runtime_error("guide name did not serialize");
        auto resource=source;resource.name=name;resource.payload=std::move(authored.bytes);resource.assetGuid=authored.partitionGuid;resource.added=true;
        guides.push_back(std::move(resource));
    }
    if(guides.size()!=3)throw std::runtime_error("expected three private guide roots");
    for(auto& guide:guides)project.resources.push_back(std::move(guide));
    const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());projects::save_project(project,out);
    if(require_deathrace_picker_contract(projects::load_project(out))!=pickerDefaults)
        throw std::runtime_error("guide authoring changed the Throwdown picker defaults");
    auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"guide_roots=3 guide_color=white existing_payloads_unchanged=true picker_contract=preserved resident_picker_types=2\nmod="<<mod.string()<<'\n';
    return true;
}
