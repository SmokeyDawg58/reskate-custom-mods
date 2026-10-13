// Use native Rime fills and label bindings, without the BasicNotice menu
// icon, conditional padding or width constraints. Keep the complete picker.
bool run_deathrace_hud_assets(int count,const char* const* args) {
    if(std::string_view(args[1])!="deathrace-hud-assets")return false;
    if(count!=5)throw std::runtime_error("deathrace-hud-assets <game-root> <input.fbproject> <output.fbproject>");
    namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);auto project=projects::load_project(args[3]);
    const auto contract=require_deathrace_picker_contract(project);
    const auto bundles=std::ranges::find(project.resources,"UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources",&projects::ProjectResource::name)->bundles;
    const auto donor=[&](const char* name){auto bytes=db.read(ui::require_asset(db,name));auto doc=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(doc,bytes);return doc;};
    const auto frame=[](sdk::ebx::Object& obj,double w,double h) {
        for(const auto [key,v]:{std::pair{"Width",w},std::pair{"Height",h}})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=v;
        for(const auto* key:{"FitWidthToContent","FitHeightToContent"})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=false;
        for(const auto* key:{"WidthConstraint","HeightConstraint"})if(auto* f=sdk::ebx::find_field(obj,key))for(auto* bound:{"Minimum","Maximum"})
            ui::require_field(ui::require_object(ui::require_field(ui::require_object(f->value,key),bound).value,bound),"UseConstraint").value.data=false;
        for(const auto* key:{"HorizontalLayout","VerticalLayout"})if(auto* f=sdk::ebx::find_field(obj,key)) {
            auto& layout=ui::require_object(f->value,key);
            for(auto* name:{"AnchorStart","AnchorEnd","OffsetStart","OffsetEnd","SizingPivot"})ui::require_field(layout,name).value.data=0.0;
            ui::require_field(layout,"SizingWeight").value.data=1.0;
        }
        if(auto* f=sdk::ebx::find_field(obj,"Padding"))for(auto* key:{"Top","Right","Bottom","Left"})ui::require_field(ui::require_object(f->value,"Padding"),key).value.data=0.0;
        if(auto* f=sdk::ebx::find_field(obj,"Visible"))f->value.data=true;
    };
    const auto emit=[&](std::string name,ebx::Partition& part) {
        ui::require_field(*part.root()->object,"Name").value.data=name;
        auto asset=ui::emit(name,part);auto it=std::ranges::find(project.resources,name,&projects::ProjectResource::name);
        if(it!=project.resources.end()){if(it->assetGuid!=asset.partitionGuid)throw std::runtime_error("asset identity changed");it->payload=asset.bytes;}
        else {projects::ProjectResource r;r.kind=projects::ResourceKind::ebx;r.name=name;r.payload=asset.bytes;r.assetGuid=asset.partitionGuid;r.bundles=bundles;r.added=true;project.resources.push_back(std::move(r));}
    };
    const auto rgb=[](sdk::ebx::Object& obj,double red,double green,double blue) {
        if(auto* f=sdk::ebx::find_field(obj,"Color")) {
            auto& value=ui::require_object(f->value,"Color");
            for(const auto [key,v]:{std::pair{"x",red},std::pair{"y",green},std::pair{"z",blue}})ui::require_field(value,key).value.data=v;
        }
        // Rime renders ColorValue.RawColor. The legacy Color alone leaves the
        // donor's white material and accessibility palette active.
        auto& value=ui::require_object(ui::require_field(obj,"ColorValue").value,"ColorValue");
        for(auto* key:{"Asset","AccessibleAsset"})ui::require_field(value,key).value.data=sdk::ebx::PointerReference{};
        ui::require_field(value,"SelectedColorId").value.data=std::string{};
        auto& raw=ui::require_object(ui::require_field(value,"RawColor").value,"RawColor");
        for(const auto [key,v]:{std::pair{"x",red},std::pair{"y",green},std::pair{"z",blue}})ui::require_field(raw,key).value.data=v;
    };
    const auto color=[&](sdk::ebx::Object& obj,double red,double green,double blue,double alpha) {
        rgb(obj,red,green,blue);ui::require_field(obj,"Alpha").value.data=alpha;
        ui::require_field(obj,"FillStyle").value.data=sdk::ebx::PointerReference{};
    };
    for(const auto [suffix,width,height,onlyFill]:std::array{
        std::tuple{"RaceRank_Widget",100.,76.,false},std::tuple{"RaceName_Widget",620.,76.,false},std::tuple{"RaceProgress_Widget",184.,76.,false},
        std::tuple{"RacePosition_Widget",240.,100.,false},std::tuple{"RaceWarning_Widget",1000.,96.,false},
        std::tuple{"EditorPanel_Widget",880.,930.,true},std::tuple{"EditorRowPanel_Widget",820.,56.,true},
        std::tuple{"RaceHeaderPanel_Widget",880.,180.,true},std::tuple{"RaceResultsBanner_Widget",1200.,130.,false},std::tuple{"ConfirmationPanel_Widget",880.,300.,true}}) {
        ebx::Partition part(donor("UI/Foundations/Components/Notices/BasicNotice_Widget"));auto& doc=part.document();
        const auto name=std::string("UI/ReSkate/Throwdowns/")+suffix;
        if(const auto old=std::ranges::find(project.resources,name,&projects::ProjectResource::name);old!=project.resources.end()) {
            const auto id=sdk::ebx::read_root_identity(old->payload);
            doc.fileGuid=id.fileGuid;part.root()->instanceGuid=id.classGuid;
        }
        doc.instances.reserve(doc.instances.size()+4);
        auto* label=part.find_instance("RimeLabelElementData");auto* fill=part.find_instance("RimeFillElementData");
        auto* layer=part.find_instance("RimeLayerEntityData");auto* widget=part.find_instance("RimeWidgetEntityData");
        if(!label || !fill || !layer || !widget)throw std::runtime_error("notice donor shape differs");
        frame(*widget->object,width,height);frame(*fill->object,width,height);color(*fill->object,.015,.015,.018,onlyFill?.65:.80);
        frame(*label->object,width-24,height);rgb(*label->object,1,1,1);ui::require_field(*label->object,"WordWrap").value.data=false;
        for(auto* key:{"HorizontalLayout"})ui::require_field(ui::require_object(ui::require_field(*label->object,key).value,key),"OffsetStart").value.data=12.0;
        ui::require_field(*label->object,"PointSize").value.data=suffix==std::string_view("RacePosition_Widget")?72.:38.;
        ui::require_field(*label->object,"HorizontalAlignment").value.data=std::int64_t{suffix==std::string_view("RaceName_Widget")?0:2};
        auto& connections=ui::require_array(ui::require_field(*part.root()->object,"PropertyConnections").value,"property connections");
        std::erase_if(connections,[&](auto& value) {
            if(onlyFill)return true;
            auto& obj=ui::require_object(value,"connection");const auto* target=sdk::ebx::pointer(ui::require_field(obj,"Target").value);
            if(!target || target->kind!=sdk::ebx::PointerKind::internal || target->index<0 || std::size_t(target->index)>=doc.instances.size())return true;
            const auto type=sdk::ebx::type_name(doc,doc.instances[std::size_t(target->index)]);
            if(type=="RimeLabelElementData") {
                // Label's native helper supplies text/font/alignment. Geometry,
                // visibility and colour are the private presenter's own.
                const auto field=std::uint32_t(sdk::ebx::integer(ui::require_field(obj,"TargetFieldId").value).value_or(0));
                return field==918277537 || field==246190218 || field==212387320 || field==4210703923U || field==205677681 || field==901540267;
            }
            return type.starts_with("Rime") || type.starts_with("Conditional");
        });
        auto& elements=ui::require_array(ui::require_field(*layer->object,"Elements").value,"layer");
        elements={sdk::ebx::Value{part.point_to(*fill)}};if(!onlyFill)elements.push_back(sdk::ebx::Value{part.point_to(*label)});
        // Four native fills form a sharp red outline around every panel.
        const auto fillIndex=std::size_t(fill-doc.instances.data());const auto layerIndex=std::size_t(layer-doc.instances.data());
        for(unsigned edge=0;edge<4;++edge) {
            auto& accent=part.clone_instance(doc.instances[fillIndex]);
            const bool horizontal=edge<2;
            frame(*accent.object,horizontal?width:3.,horizontal?3.:height);color(*accent.object,.88,.045,.035,.9);
            auto& x=ui::require_object(ui::require_field(*accent.object,"HorizontalLayout").value,"HorizontalLayout");
            auto& y=ui::require_object(ui::require_field(*accent.object,"VerticalLayout").value,"VerticalLayout");
            for(auto* key:{"OffsetStart","OffsetEnd"}) {
                ui::require_field(x,key).value.data=edge==3?width-3.:0.;
                ui::require_field(y,key).value.data=edge==1?height-3.:0.;
            }
            ui::require_array(ui::require_field(*doc.instances[layerIndex].object,"Elements").value,"layer").push_back(sdk::ebx::Value{part.point_to(accent)});
        }
        emit(std::string("UI/ReSkate/Throwdowns/")+suffix,part);
    }
    // Static native artwork avoids a texture model's default Visible=false or
    // dimensions being reapplied by its generated data-model graph.
    for(const auto [name,texture]:std::array{std::pair{"HUDLogo_Widget","UI/ReSkate/OneUp/img_OneUp_Blue_1024"},std::pair{"DeathraceLogo_Widget","UI/ReSkate/Throwdowns/img_race_Placeholder"}}) {
        const auto existing=std::ranges::find(project.resources,std::string("UI/ReSkate/Throwdowns/")+name,&projects::ProjectResource::name);
        auto doc=existing!=project.resources.end()?sdk::ebx::read_document(existing->payload):donor("UI/Foundations/Components/Media/Icons/Texture_Widget");
        if(existing!=project.resources.end())sdk::ebx::decode_boxed_values(doc,existing->payload);
        ebx::Partition part(doc,existing==project.resources.end());auto& d=part.document();
        auto textureIt=std::ranges::find(project.resources,std::string(texture),&projects::ProjectResource::name);
        if(textureIt==project.resources.end())throw std::runtime_error(std::string("missing private artwork ")+texture);
        const auto identity=sdk::ebx::read_root_identity(textureIt->payload);auto* layer=part.find_instance("RimeLayerEntityData");
        ui::require_array(ui::require_field(*part.root()->object,"PropertyConnections").value,"texture connections").clear();
        auto& elements=ui::require_array(ui::require_field(*layer->object,"Elements").value,"texture layer");elements.clear();
        bool rendered{};
        for(auto& instance:d.instances)if(instance.object) {
            const auto type=sdk::ebx::type_name(d,instance);
            if(type=="RimeWidgetEntityData" || type=="RimeTextureElementData")frame(*instance.object,96,96);
            if(type=="RimeTextureElementData" && !rendered) {
                ui::require_field(*instance.object,"Texture").value.data=part.import(identity.fileGuid,identity.classGuid);
                ui::require_field(*instance.object,"Alpha").value.data=1.0;
                elements.push_back(sdk::ebx::Value{part.point_to(instance)});rendered=true;
            }
        }
        emit(std::string("UI/ReSkate/Throwdowns/")+name,part);
    }
    const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());projects::save_project(project,out);
    if(require_deathrace_picker_contract(projects::load_project(out))!=contract)throw std::runtime_error("picker contract changed");
    auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"private_race_cells=6 editor_panels=2 race_header=1 confirmation_panel=1 static_logos=2 raw_color=verified black_red_outline=true picker_contract=preserved\n";return true;
}
