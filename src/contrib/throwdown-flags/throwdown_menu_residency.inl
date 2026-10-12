// Persist the picker's dynamic model types in its authored resource container.
// The native model registry alone only knows types instantiated by other UI.
bool run_throwdown_menu_residency(int count,const char* const* args) {
    if(std::string_view(args[1])!="throwdown-menu-residency")return false;
    if(count!=5)throw std::runtime_error("throwdown-menu-residency <game-root> <input.fbproject> <output.fbproject>");
    namespace sdk=reskate::sdk;
    namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;
    namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
    auto project=projects::load_project(args[3]);
    auto content=std::ranges::find(project.resources,
        "UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources",&projects::ProjectResource::name);
    if(content==project.resources.end())throw std::runtime_error("private Throwdown content resource is missing");
    auto document=sdk::ebx::read_document(content->payload);
    sdk::ebx::decode_boxed_values(document,content->payload);
    const auto defaults=native_defaults(document);
    ebx::Partition partition(document,false);auto& doc=partition.document();
    std::optional<std::size_t> prototype;
    for(std::size_t i=0;i<doc.instances.size();++i)if(doc.instances[i].object) {
        const auto* name=sdk::ebx::find_field(*doc.instances[i].object,"DebugName");
        const auto* text=name?std::get_if<std::string>(&name->value.data):nullptr;
        if(text && text->ends_with("/LogoGrid"))prototype=i;
    }
    if(!prototype)throw std::runtime_error("private native grid prototype is missing");
    const auto reference=std::get<sdk::ebx::BoxedReference>(ui::require_field(*doc.instances[*prototype].object,"DefaultValue").value.data);
    const auto* box=sdk::ebx::resolve_boxed(doc,reference);
    const auto* pointer=box && box->decoded?sdk::ebx::pointer(*box->decoded):nullptr;
    if(!pointer || pointer->kind!=sdk::ebx::PointerKind::internal)throw std::runtime_error("grid prototype has no typed default");
    const auto modelIndex=static_cast<std::size_t>(pointer->index);
    auto next=doc.boxedValuesOffset;
    for(const auto& old:doc.boxedValues)next=std::max(next,old.offset+static_cast<std::uint32_t>(old.rawBytes.size()));
    doc.instances.reserve(doc.instances.size()+4);
    constexpr std::array models{
        std::pair{"LogoTexture","UI/Foundations/Components/Media/Icons/TextureViewModel"},
        std::pair{"LogoContents","UI/Foundations/Components/Modals/ContentPresenterCombinationViewModel"}};
    for(const auto& [key,name]:models) {
        const auto type=sdk::ebx::read_document(db.read(ui::require_asset(db,name)));
        auto& model=partition.clone_instance(doc.instances.at(modelIndex),true);
        auto& typeRef=ui::require_object(ui::require_field(*model.object,"TypeRef").value,"model type");
        ui::require_field(typeRef,"Asset").value.data=partition.import(type.fileGuid,type.instances.front().instanceGuid);
        ui::require_field(typeRef,"TypeInfo").value.data=sdk::ebx::TypeReference{};
        ui::require_field(*model.object,"FieldValues").value.data=sdk::ebx::Value::Array{};
        auto clone=*sdk::ebx::resolve_boxed(doc,reference);
        clone.offset=(next+7u)&~7u;next=clone.offset+static_cast<std::uint32_t>(clone.rawBytes.size());
        clone.decoded=std::make_shared<sdk::ebx::Value>(sdk::ebx::Value{partition.point_to(model)});
        auto defaultValue=reference;defaultValue.dataOffset=clone.offset;defaultValue.needsFixup=true;
        doc.boxedValues.push_back(std::move(clone));
        auto& record=partition.clone_instance(doc.instances[*prototype],true);
        ui::require_field(*record.object,"DebugName").value.data=std::string("Throwdown_ModeSelect_ContentResources/")+key;
        ui::require_field(*record.object,"DefaultValue").value.data=defaultValue;
        ui::require_array(ui::require_field(*partition.root()->object,"Resources").value,"Resources").push_back({partition.point_to(record)});
    }
    const auto emitted=ui::emit(content->name,partition);content->payload=emitted.bytes;
    auto verified=sdk::ebx::read_document(emitted.bytes);sdk::ebx::decode_boxed_values(verified,emitted.bytes);
    const auto updated=native_defaults(verified);
    for(const auto& [name,target]:defaults)if(updated.find(name)==updated.end() || updated.at(name)!=target)
        throw std::runtime_error("menu residency changed an existing native default: "+name);
    if(updated.size()!=defaults.size()+2)throw std::runtime_error("private model templates were not serialized");
    const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());
    projects::save_project(project,out);auto mod=out;mod.replace_extension(".fbmod");
    reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"resident_picker_types=2 existing_defaults_preserved="<<defaults.size()<<" resources="<<project.resources.size()<<"\nmod="<<mod.string()<<'\n';
    return true;
}
