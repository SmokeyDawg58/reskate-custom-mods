// Use native Rime fills and label bindings, without the BasicNotice menu
// icon, conditional padding or width constraints. Keep the complete picker.
bool run_deathrace_hud_assets(int count,const char* const* args) {
    if(std::string_view(args[1])=="deathrace-clock-board-assets") {
        if(count!=5)throw std::runtime_error("deathrace-clock-board-assets <game-root> <input.fbproject> <output.fbproject>");
        namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
        namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
        const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
        auto project=projects::load_project(args[3]);const auto contract=require_deathrace_picker_contract(project);
        const std::string prefix="UI/ReSkate/Throwdowns/";
        const auto source=[&](const std::string& suffix) {
            const auto found=std::ranges::find(project.resources,prefix+suffix,&projects::ProjectResource::name);
            if(found==project.resources.end())throw std::runtime_error("HUD template missing: "+suffix);
            return *found;
        };
        const auto tint=[](sdk::ebx::Object& object,std::array<double,3> rgb) {
            for(const auto* key:{"Color","ColorValue"})if(auto* field=sdk::ebx::find_field(object,key)) {
                auto* value=&ui::require_object(field->value,key);
                if(std::string_view(key)=="ColorValue") {
                    for(const auto* asset:{"Asset","AccessibleAsset"})ui::require_field(*value,asset).value.data=sdk::ebx::PointerReference{};
                    ui::require_field(*value,"SelectedColorId").value.data=std::string{};
                    value=&ui::require_object(ui::require_field(*value,"RawColor").value,"RawColor");
                }
                for(unsigned axis=0;axis<3;++axis)ui::require_field(*value,axis==0?"x":axis==1?"y":"z").value.data=rgb[axis];
            }
        };
        unsigned added{};
        const auto clone=[&](const std::string& donor,const std::string& suffix,double width,double height,
                             bool text_only=false,double point_size=0.,std::array<double,3> rgb={1.,1.,1.}) {
            if(std::ranges::find(project.resources,prefix+suffix,&projects::ProjectResource::name)!=project.resources.end())return;
            auto resource=source(donor);auto doc=sdk::ebx::read_document(resource.payload);
            sdk::ebx::decode_boxed_values(doc,resource.payload);ebx::Partition part(doc);
            auto* widget=part.find_instance("RimeWidgetEntityData");
            if(!widget)throw std::runtime_error("HUD widget missing");
            const auto old_width=sdk::ebx::number(ui::require_field(*widget->object,"Width").value).value();
            const auto old_height=sdk::ebx::number(ui::require_field(*widget->object,"Height").value).value();
            auto& document=part.document();
            for(auto& instance:document.instances)if(instance.object) {
                const auto type=sdk::ebx::type_name(document,instance);
                if(type=="RimeWidgetEntityData" || type=="RimeContainerElementData" || type=="RimeFillElementData" || type=="RimeLabelElementData") {
                    for(const auto [key,old_value,new_value]:{std::tuple{"Width",old_width,width},std::tuple{"Height",old_height,height}})
                        if(auto* field=sdk::ebx::find_field(*instance.object,key)) {
                            const auto value=sdk::ebx::number(field->value);
                            if(value && *value==old_value)field->value.data=new_value;
                            else if(type=="RimeLabelElementData" && std::string_view(key)=="Width" && value && *value==old_value-24.)field->value.data=width-24.;
                        }
                }
                if(type=="RimeLabelElementData") {
                    tint(*instance.object,rgb);
                    ui::require_field(*instance.object,"WordWrap").value.data=false;
                    if(point_size>0)ui::require_field(*instance.object,"PointSize").value.data=point_size;
                }
                if(text_only && type=="RimeContainerElementData") {
                    auto& children=ui::require_array(ui::require_field(*instance.object,"Elements").value,"caption children");
                    std::erase_if(children,[&](const auto& value) {
                        const auto* pointer=sdk::ebx::pointer(value);
                        const auto* child=pointer?sdk::ebx::resolve_internal(document,*pointer):nullptr;
                        return child && sdk::ebx::type_name(document,*child)=="RimeFillElementData";
                    });
                }
            }
            if(point_size>0) {
                // Font family still comes from the native text style, while
                // the three-times position size belongs to this private glyph.
                auto& links=ui::require_array(ui::require_field(*part.root()->object,"PropertyConnections").value,"position font bindings");
                std::erase_if(links,[&](auto& value) {
                    auto& link=ui::require_object(value,"font binding");
                    const auto* pointer=sdk::ebx::pointer(ui::require_field(link,"Target").value);
                    const auto* target=pointer?sdk::ebx::resolve_internal(document,*pointer):nullptr;
                    return target && sdk::ebx::type_name(document,*target)=="RimeLabelElementData" &&
                        std::uint32_t(sdk::ebx::integer(ui::require_field(link,"TargetFieldId").value).value_or(0))==1834692108U;
                });
            }
            const auto name=prefix+suffix;ui::require_field(*part.root()->object,"Name").value.data=name;
            auto authored=ui::emit(name,part);resource.name=name;resource.assetGuid=authored.partitionGuid;
            resource.payload=authored.bytes;resource.added=true;project.resources.push_back(std::move(resource));++added;
        };
        const std::array keys{"oneup","jam","meat","race","domination","graffiti","tag","infection","hide"};
        for(unsigned width=224;width<=1280;width+=32) {
            clone("RaceName_640_Widget","RaceName_"+std::to_string(width)+"_Widget",width,76.);
            clone("OneUpName_640_Widget","OneUpName_"+std::to_string(width)+"_Widget",width,76.);
            clone("OneUpEliminated_640_Widget","OneUpEliminated_"+std::to_string(width)+"_Widget",width,76.);
            clone("RaceTime_Widget","Caption_"+std::to_string(width)+"_Widget",width,52.,true);
            for(const auto* key:keys)clone(std::string("HUD_")+key+"_Name_640_Widget",std::string("HUD_")+key+"_Name_"+std::to_string(width)+"_Widget",width,76.);
        }
        clone("RaceTime_Widget","Caption_100_Widget",100.,52.,true);
        for(unsigned width=224;width<=640;width+=32) {
            clone("RaceTime_Widget","RaceTime_"+std::to_string(width)+"_Widget",width,76.);
            for(const auto* key:keys)clone(std::string("HUD_")+key+"_Value_Widget",std::string("HUD_")+key+"_Value_"+std::to_string(width)+"_Widget",width,76.);
        }
        for(unsigned width=564;width<=2036;width+=32)
            clone("RaceTime_Widget","Heading_"+std::to_string(width)+"_64_Widget",width,64.,true);
        for(unsigned width=1184;width<=2240;width+=32) {
            clone("RaceTime_Widget","Heading_"+std::to_string(width-232)+"_144_Widget",width-232,144.,true);
            for(unsigned rows=1;rows<=6;++rows)clone("ResultsPanel"+std::to_string(rows)+"_Widget",
                "ResultsPanel"+std::to_string(rows)+"_"+std::to_string(width)+"_Widget",width,432.+84.*rows);
            for(const auto* key:keys)clone(std::string("HUD_")+key+"_Results_Widget",
                std::string("HUD_")+key+"_Results_"+std::to_string(width-64)+"_Widget",width-64,96.);
        }
        clone("RacePosition_Widget","RacePosition_Gold_Widget",480.,300.,false,216.,{1.,.78,.19});
        clone("RacePosition_Widget","RacePosition_Silver_Widget",480.,300.,false,216.,{.78,.82,.88});
        clone("RacePosition_Widget","RacePosition_Bronze_Widget",480.,300.,false,216.,{.80,.49,.27});
        clone("RacePosition_Widget","RacePosition_White_Widget",480.,300.,false,216.);
        clone("RaceTime_Widget","RaceElapsed_Widget",384.,100.,false,62.);
        const std::filesystem::path output(args[4]);std::filesystem::create_directories(output.parent_path());
        projects::save_project(project,output);
        if(require_deathrace_picker_contract(projects::load_project(output))!=contract)throw std::runtime_error("picker identities changed");
        auto mod=output;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
        std::cout<<"clock_board_variants="<<added<<" existing_assets=unchanged picker_contract=preserved\n";
        return true;
    }

    if(std::string_view(args[1])=="deathrace-results-layout") {
        if(count!=5)throw std::runtime_error("deathrace-results-layout <game-root> <input.fbproject> <output.fbproject>");
        namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;namespace projects=reskate::modding::projects;
        const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);
        auto project=projects::load_project(args[3]);const auto contract=require_deathrace_picker_contract(project);
        for(unsigned rows=1;rows<=6;++rows) {
            const auto name="UI/ReSkate/Throwdowns/ResultsPanel"+std::to_string(rows)+"_Widget";
            auto resource=std::ranges::find(project.resources,name,&projects::ProjectResource::name);
            if(resource==project.resources.end())throw std::runtime_error("results panel missing");
            auto doc=sdk::ebx::read_document(resource->payload);sdk::ebx::decode_boxed_values(doc,resource->payload);
            unsigned changed{};
            for(auto& instance:doc.instances)if(instance.object) {
                const auto type=sdk::ebx::type_name(doc,instance);
                const auto* debug=sdk::ebx::find_field(*instance.object,"DebugInstanceName");
                const auto* text=debug?std::get_if<std::string>(&debug->value.data):nullptr;
                if(type=="RimeWidgetEntityData" || type=="RimeContainerElementData" ||
                   (type=="RimeFillElementData" && text && *text=="Notice_Background")) {
                    const auto* width_field=sdk::ebx::find_field(*instance.object,"Width");
                    const auto* width=width_field?std::get_if<double>(&width_field->value.data):nullptr;
                    if(!width || *width!=1164.)continue; // Dormant donor subcontainers keep their own bounds.
                    auto& value=ui::require_field(*instance.object,"Height").value;
                    const auto* height=std::get_if<double>(&value.data);
                    if(type=="RimeFillElementData" && height && *height==2.)continue; // The approved top trim shares the donor debug name.
                    if(!height || *height!=352.+84.*rows)throw std::runtime_error("results source geometry differs: "+name+" "+std::string(type)+" height="+(height?std::to_string(*height):"non-float"));
                    value.data=400.+84.*rows;++changed;
                }
            }
            if(changed!=3)throw std::runtime_error("results panel geometry owners differ");
            resource->payload=sdk::ebx::write_document(doc);
        }
        const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());projects::save_project(project,out);
        if(require_deathrace_picker_contract(projects::load_project(out))!=contract)throw std::runtime_error("picker contract changed");
        auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
        std::cout<<"results_panels=6 geometry_owners=18 picker_contract=preserved other_payloads=untouched\n";return true;
    }
    if(std::string_view(args[1])!="deathrace-hud-assets")return false;
    if(count!=5)throw std::runtime_error("deathrace-hud-assets <game-root> <input.fbproject> <output.fbproject>");
    namespace sdk=reskate::sdk;namespace ui=reskate::modding::maps::ui;
    namespace ebx=reskate::modding::maps::ebx;namespace projects=reskate::modding::projects;
    const auto db=sdk::assets::FrostbiteAssetManager::load_sync(args[2]);auto project=projects::load_project(args[3]);
    const auto contract=require_deathrace_picker_contract(project);
    const auto bundles=std::ranges::find(project.resources,"UI/Features/Throwdowns/ContentResources/Throwdown_ModeSelect_ContentResources",&projects::ProjectResource::name)->bundles;
    const auto donor=[&](const char* name){auto bytes=db.read(ui::require_asset(db,name));auto doc=sdk::ebx::read_document(bytes);sdk::ebx::decode_boxed_values(doc,bytes);return doc;};
    const auto frame=[](sdk::ebx::Object& obj,double w,double h,bool child=true) {
        for(const auto [key,v]:{std::pair{"Width",w},std::pair{"Height",h}})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=v;
        for(const auto* key:{"FitWidthToContent","FitHeightToContent"})if(auto* f=sdk::ebx::find_field(obj,key))f->value.data=false;
        for(const auto* key:{"WidthConstraint","HeightConstraint"})if(auto* f=sdk::ebx::find_field(obj,key))for(auto* bound:{"Minimum","Maximum"})
            ui::require_field(ui::require_object(ui::require_field(ui::require_object(f->value,key),bound).value,bound),"UseConstraint").value.data=false;
        for(const auto* key:{"HorizontalLayout","VerticalLayout"})if(auto* f=sdk::ebx::find_field(obj,key)) {
            auto& layout=ui::require_object(f->value,key);
            for(auto* name:{"AnchorStart","AnchorEnd","OffsetStart","OffsetEnd"})ui::require_field(layout,name).value.data=0.0;
            // A root element has no layout parent. Stretching it collapses its
            // measured width; children alone stretch inside the fixed root.
            ui::require_field(layout,"AnchorEnd").value.data=child?1.0:0.0;
            ui::require_field(layout,"SizingPivot").value.data=child?.5:0.0;
            ui::require_field(layout,"SizingWeight").value.data=child?0.0:1.0;
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
    const std::array themes{
        std::tuple{"oneup",.133,.545,.937,"UI/ReSkate/OneUp/img_OneUp_Blue_1024"},
        std::tuple{"jam",1.,.310,.671,"UI/ReSkate/Throwdowns/img_jam_Placeholder"},
        std::tuple{"meat",1.,.541,.129,"UI/ReSkate/Throwdowns/img_meat_Placeholder"},
        std::tuple{"race",1.,.204,.204,"UI/ReSkate/Throwdowns/img_race_Placeholder"},
        std::tuple{"domination",.549,.333,1.,"UI/ReSkate/Throwdowns/img_domination_Placeholder"},
        std::tuple{"graffiti",.161,.851,.624,"UI/ReSkate/Throwdowns/img_graffiti_Placeholder"},
        std::tuple{"tag",1.,.827,.192,"UI/ReSkate/Throwdowns/img_tag_Placeholder"},
        std::tuple{"infection",.459,.839,.141,"UI/ReSkate/Throwdowns/img_infection_Placeholder"},
        std::tuple{"hide",.224,.741,1.,"UI/ReSkate/Throwdowns/img_hide_Placeholder"}};
    std::vector<std::tuple<std::string,double,double,bool>> notices{
        {"RaceRank_Widget",100.,76.,false},{"RaceName_Widget",500.,76.,false},{"RaceProgress_Widget",184.,76.,false},
        {"RacePosition_Widget",240.,100.,false},{"RaceWarning_Widget",1000.,96.,false},
        {"EditorPanel_Widget",880.,1150.,true},{"EditorRowPanel_Widget",820.,56.,true},
        {"RaceHeaderPanel_Widget",880.,180.,true},{"RaceResultsBanner_Widget",1100.,96.,false},{"ConfirmationPanel_Widget",880.,300.,true},
        {"RaceTime_Widget",280.,76.,false},{"OneUpName_Widget",620.,76.,false},{"OneUpEliminated_Widget",620.,76.,false},{"OneUpScore_Widget",480.,72.,false}};
    for(unsigned rows=1;rows<=6;++rows)
        notices.emplace_back("ResultsPanel"+std::to_string(rows)+"_Widget",1164.,400.+84.*rows,true);
    for(const auto& [key,r,g,b,logo]:themes)for(const auto& [cell,w,h]:std::array{
        std::tuple{"Rank",100.,76.},std::tuple{"Name",620.,76.},std::tuple{"Value",184.,76.},
        std::tuple{"Score",480.,72.},std::tuple{"Results",1100.,96.}})
        notices.emplace_back(std::string("HUD_")+key+"_"+cell+"_Widget",w,h,false);
    // Discrete native name widths retain bounded parents and their grain/trim.
    // Runtime selects the smallest frame fitting the displayed gamertag.
    for(unsigned width=224;width<=640;width+=32) {
        for(const auto* cell:{"OneUpName","OneUpEliminated","RaceName"})
            notices.emplace_back(std::string(cell)+"_"+std::to_string(width)+"_Widget",double(width),76.,false);
        for(const auto& [key,r,g,b,logo]:themes)
            notices.emplace_back(std::string("HUD_")+key+"_Name_"+std::to_string(width)+"_Widget",double(width),76.,false);
    }
    for(const auto& [suffix,width,height,onlyFill]:notices) {
        std::array<double,3> tint{1.,.204,.204};
        for(const auto& [key,r,g,b,logo]:themes)
            if(suffix.starts_with(std::string("HUD_")+key+"_") || (suffix.starts_with("OneUp") && std::string_view(key)=="oneup"))tint={r,g,b};
        ebx::Partition part(donor("UI/Foundations/Components/Notices/BasicNotice_Widget"));auto& doc=part.document();
        const auto name=std::string("UI/ReSkate/Throwdowns/")+suffix;
        if(const auto old=std::ranges::find(project.resources,name,&projects::ProjectResource::name);old!=project.resources.end()) {
            const auto id=sdk::ebx::read_root_identity(old->payload);
            doc.fileGuid=id.fileGuid;part.root()->instanceGuid=id.classGuid;
        }
        doc.instances.reserve(doc.instances.size()+5);
        auto* label=part.find_instance("RimeLabelElementData");
        auto* fill=part.find_instance("RimeFillElementData");
        for(auto& instance:doc.instances)if(instance.object && sdk::ebx::type_name(doc,instance)=="RimeFillElementData") {
            auto* debug=sdk::ebx::find_field(*instance.object,"DebugInstanceName");
            const auto* name=debug?std::get_if<std::string>(&debug->value.data):nullptr;
            if(name && *name=="Notice_Background")fill=&instance;
        }
        auto* layer=part.find_instance("RimeLayerEntityData");auto* widget=part.find_instance("RimeWidgetEntityData");
        auto* bounds=part.find_instance("RimeContainerElementData");
        if(!label || !fill || !layer || !widget || !bounds)throw std::runtime_error("notice donor shape differs");
        frame(*widget->object,width,height,false);frame(*bounds->object,width,height,false);rgb(*bounds->object,1,1,1);
        ui::require_field(*bounds->object,"Alpha").value.data=1.0;
        const auto grain=ui::require_field(*fill->object,"FillStyle").value;
        frame(*fill->object,width,height);color(*fill->object,.015,.015,.015,onlyFill?.78:1.0);
        if(!onlyFill)ui::require_field(*fill->object,"FillStyle").value=grain;
        frame(*label->object,width-24,height);rgb(*label->object,1,1,1);ui::require_field(*label->object,"Alpha").value.data=1.0;ui::require_field(*label->object,"WordWrap").value.data=false;
        auto& labelX=ui::require_object(ui::require_field(*label->object,"HorizontalLayout").value,"HorizontalLayout");
        ui::require_field(labelX,"OffsetStart").value.data=12.0;ui::require_field(labelX,"OffsetEnd").value.data=-12.0;
        ui::require_field(*label->object,"PointSize").value.data=suffix==std::string_view("RacePosition_Widget")?72.:38.;
        ui::require_field(*label->object,"HorizontalAlignment").value.data=std::int64_t{2};
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
        // A Rime layer is a render list, not a layout parent. Every fill must
        // share a bounded container. Otherwise an accent inherits screen bounds.
        ui::require_field(*layer->object,"Elements").value.data=sdk::ebx::Value::Array{{part.point_to(*bounds)}};
        auto& children=ui::require_array(ui::require_field(*bounds->object,"Elements").value,"bounds");
        children={sdk::ebx::Value{part.point_to(*fill)}};
        const auto fillIndex=std::size_t(fill-doc.instances.data());
        const auto boundsIndex=std::size_t(bounds-doc.instances.data());
        const auto labelIndex=std::size_t(label-doc.instances.data());
        for(unsigned edge=0;edge<1;++edge) {
            auto& accent=part.clone_instance(doc.instances[fillIndex]);
            const bool horizontal=edge<2;
            frame(*accent.object,horizontal?width:2.,horizontal?2.:height);color(*accent.object,tint[0]*.65,tint[1]*.65,tint[2]*.65,.65);
            auto& axis=ui::require_object(ui::require_field(*accent.object,horizontal?"VerticalLayout":"HorizontalLayout").value,"edge axis");
            const bool far=edge==1 || edge==3;
            ui::require_field(axis,"AnchorStart").value.data=far?1.0:0.0;
            ui::require_field(axis,"AnchorEnd").value.data=far?1.0:0.0;
            ui::require_field(axis,"OffsetStart").value.data=far?-2.0:0.0;
            ui::require_field(axis,"OffsetEnd").value.data=far?0.0:2.0;
            ui::require_array(ui::require_field(*doc.instances[boundsIndex].object,"Elements").value,"bounds").push_back({part.point_to(accent)});
        }
        // Text is last in the native render order, with opaque white colour.
        if(!onlyFill)ui::require_array(ui::require_field(*doc.instances[boundsIndex].object,"Elements").value,"bounds").push_back({part.point_to(doc.instances[labelIndex])});
        if(suffix.starts_with("OneUpEliminated")) {
            // The name and its stroke share this exact native layout parent.
            auto& strike=part.clone_instance(doc.instances[fillIndex]);
            frame(*strike.object,width,4.);color(*strike.object,tint[0],tint[1],tint[2],1.);
            auto& x=ui::require_object(ui::require_field(*strike.object,"HorizontalLayout").value,"strike x");
            ui::require_field(x,"OffsetStart").value.data=22.;ui::require_field(x,"OffsetEnd").value.data=-22.;
            auto& y=ui::require_object(ui::require_field(*strike.object,"VerticalLayout").value,"strike y");
            ui::require_field(y,"AnchorStart").value.data=.5;ui::require_field(y,"AnchorEnd").value.data=.5;
            ui::require_field(y,"OffsetStart").value.data=-2.;ui::require_field(y,"OffsetEnd").value.data=2.;
            ui::require_array(ui::require_field(*doc.instances[boundsIndex].object,"Elements").value,"bounds").push_back({part.point_to(strike)});
        }
        // Validate serialized geometry rather than merely finding colour bytes.
        auto check=ui::emit(name,part);auto verified=sdk::ebx::read_document(check.bytes);
        auto container=std::ranges::find_if(verified.instances,[&](const auto& instance){return instance.object && sdk::ebx::type_name(verified,instance)=="RimeLayerEntityData";});
        const auto& entries=ui::require_array(ui::require_field(*container->object,"Elements").value,"verified layer");
        if(entries.size()!=1)throw std::runtime_error("HUD layer has unbounded children");
        const auto* parent=sdk::ebx::resolve_internal(verified,*sdk::ebx::pointer(entries.front()));
        if(!parent || sdk::ebx::type_name(verified,*parent)!="RimeContainerElementData")throw std::runtime_error("HUD layout parent missing");
        for(const auto* key:{"HorizontalLayout","VerticalLayout"}) {
            auto& axis=ui::require_object(ui::require_field(*parent->object,key).value,"root axis");
            if(sdk::ebx::number(ui::require_field(axis,"SizingWeight").value)!=1.0 ||
               sdk::ebx::number(ui::require_field(axis,"AnchorEnd").value)!=0.0)
                throw std::runtime_error("HUD root is not fixed-size");
        }
        const auto& rendered=ui::require_array(ui::require_field(*parent->object,"Elements").value,"verified bounds");
        if(rendered.size()!=(suffix.starts_with("OneUpEliminated")?4u:onlyFill?2u:3u))throw std::runtime_error("HUD render order differs");
        for(unsigned i=0;i<rendered.size();++i) {
            const auto* child=sdk::ebx::resolve_internal(verified,*sdk::ebx::pointer(rendered[i]));
            if(!child || !child->object)throw std::runtime_error("HUD layout child missing");
            for(const auto* key:{"HorizontalLayout","VerticalLayout"}) {
                auto& axis=ui::require_object(ui::require_field(*child->object,key).value,"verified axis");
                for(const auto* field:{"AnchorStart","AnchorEnd"}) {
                    const auto n=sdk::ebx::number(ui::require_field(axis,field).value).value_or(-1);
                    if(n<0 || n>1)throw std::runtime_error("HUD child extends outside layout parent");
                }
                if(sdk::ebx::number(ui::require_field(axis,"SizingWeight").value).value_or(-1)!=0)throw std::runtime_error("HUD child ignores its layout bounds");
            }
            if(i==2 && !onlyFill && sdk::ebx::type_name(verified,*child)!="RimeLabelElementData")throw std::runtime_error("HUD text is not on top");
        }
        emit(std::string("UI/ReSkate/Throwdowns/")+suffix,part);
    }
    for(unsigned step=1;step<=32;++step) {
        ebx::Partition ring(donor("UI/Foundations/Components/Buttons/InputIcon/HoldButtonRadialFill_Widget"));
        const auto ringName="UI/ReSkate/Throwdowns/EditorHoldRing"+std::to_string(step)+"_Widget";
        if(const auto old=std::ranges::find(project.resources,ringName,&projects::ProjectResource::name);old!=project.resources.end()) {
            const auto id=sdk::ebx::read_root_identity(old->payload);ring.document().fileGuid=id.fileGuid;ring.root()->instanceGuid=id.classGuid;
        }
        auto* arc=ring.find_instance("RimeArcProgressBarElementData");auto* bounds=ring.find_instance("RimeContainerElementData");
        auto* layer=ring.find_instance("RimeLayerEntityData");auto* widget=ring.find_instance("RimeWidgetEntityData");
        if(!arc || !bounds || !layer || !widget)throw std::runtime_error("native hold ring donor differs");
        frame(*widget->object,64,64,false);frame(*bounds->object,64,64,false);rgb(*bounds->object,1,1,1);
        ui::require_field(*bounds->object,"Alpha").value.data=1.0;
        frame(*arc->object,64,64);rgb(*arc->object,.95,.08,.055);ui::require_field(*arc->object,"Alpha").value.data=1.0;
        for(const auto [key,value]:{std::pair{"LineThickness",4.0},std::pair{"LineProportion",.0625},std::pair{"InnerRadius",28.0},std::pair{"GlowSize",0.0},std::pair{"Progress",double(step)/32.0}})
            ui::require_field(*arc->object,key).value.data=value;
        ui::require_field(*bounds->object,"Elements").value.data=sdk::ebx::Value::Array{{ring.point_to(*arc)}};
        ui::require_field(*layer->object,"Elements").value.data=sdk::ebx::Value::Array{{ring.point_to(*bounds)}};
        ui::require_field(*widget->object,"Components").value.data=sdk::ebx::Value::Array{};
        for(const auto* key:{"PropertyConnections","EventConnections","LinkConnections"})if(auto* field=sdk::ebx::find_field(*ring.root()->object,key))field->value.data=sdk::ebx::Value::Array{};
        emit("UI/ReSkate/Throwdowns/EditorHoldRing"+std::to_string(step)+"_Widget",ring);
    }
    // Static native artwork avoids a texture model's default Visible=false or
    // dimensions being reapplied by its generated data-model graph.
        std::vector<std::pair<std::string,std::string>> logos{{"HUDLogo_Widget","UI/ReSkate/OneUp/img_OneUp_Blue_1024"},{"DeathraceLogo_Widget","UI/ReSkate/Throwdowns/img_race_Placeholder"}};
    for(const auto& [key,r,g,b,logo]:themes)logos.emplace_back(std::string("HUD_")+key+"_Logo_Widget",logo);
    for(const auto& [name,texture]:logos) {
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
            if(type=="RimeWidgetEntityData" || type=="RimeTextureElementData")frame(*instance.object,144,144,false);
            if(type=="RimeWidgetEntityData") {
                ui::require_field(*instance.object,"Components").value.data=sdk::ebx::Value::Array{};
                ui::require_field(*instance.object,"WidgetVisible").value.data=true;
            }
            if(type=="RimeTextureElementData" && !rendered) {
                ui::require_field(*instance.object,"Texture").value.data=part.import(identity.fileGuid,identity.classGuid);
                ui::require_field(*instance.object,"Alpha").value.data=1.0;
                elements.push_back(sdk::ebx::Value{part.point_to(instance)});rendered=true;
            }
        }
        for(const auto* key:{"PropertyConnections","EventConnections","LinkConnections"})
            if(auto* f=sdk::ebx::find_field(*part.root()->object,key))f->value.data=sdk::ebx::Value::Array{};
        emit(std::string("UI/ReSkate/Throwdowns/")+name,part);
    }
    // Draw Clear All's progress in the Select glyph's own render layer. A
    // separate screen anchor cannot track native font/device-dependent rows.
    for(unsigned step=1;step<=32;++step) {
        const auto glyphIt=std::ranges::find(project.resources,"UI/ReSkate/Throwdowns/EditorGlyph_Widget",&projects::ProjectResource::name);
        if(glyphIt==project.resources.end())throw std::runtime_error("native Select glyph missing");
        auto source=sdk::ebx::read_document(glyphIt->payload);sdk::ebx::decode_boxed_values(source,glyphIt->payload);
        ebx::Partition combined(source);auto& doc=combined.document();
        const auto name="UI/ReSkate/Throwdowns/EditorClearGlyph"+std::to_string(step)+"_Widget";
        if(const auto old=std::ranges::find(project.resources,name,&projects::ProjectResource::name);old!=project.resources.end()) {
            const auto id=sdk::ebx::read_root_identity(old->payload);doc.fileGuid=id.fileGuid;combined.root()->instanceGuid=id.classGuid;
        }
        auto* ref=combined.find_instance("RimeWidgetReferenceElementData");
        if(!ref)throw std::runtime_error("Select glyph render reference missing");
        const auto refIndex=std::size_t(ref-doc.instances.data());
        auto& ringRef=combined.clone_instance(doc.instances[refIndex]);
        const auto ringName="UI/ReSkate/Throwdowns/EditorHoldRing"+std::to_string(step)+"_Widget";
        const auto ringIt=std::ranges::find(project.resources,ringName,&projects::ProjectResource::name);
        if(ringIt==project.resources.end())throw std::runtime_error("native clear progress frame missing");
        const auto identity=sdk::ebx::read_root_identity(ringIt->payload);
        ui::require_field(*ringRef.object,"Blueprint").value.data=combined.import(identity.fileGuid,identity.classGuid);
        frame(*ringRef.object,64,64,false);
        // Glyph is 56 square. The 64 square radial stroke shares its centre.
        for(const auto* axis:{"HorizontalLayout","VerticalLayout"}) {
            auto& layout=ui::require_object(ui::require_field(*ringRef.object,axis).value,"radial glyph axis");
            ui::require_field(layout,"OffsetStart").value.data=-4.0;
            ui::require_field(layout,"OffsetEnd").value.data=-4.0;
        }
        const auto ringPointer=combined.point_to(ringRef);
        auto* layer=combined.find_instance("RimeLayerEntityData");
        ui::require_array(ui::require_field(*layer->object,"Elements").value,"Select glyph layer").push_back({ringPointer});
        emit(name,combined);
    }
    const std::filesystem::path out(args[4]);std::filesystem::create_directories(out.parent_path());projects::save_project(project,out);
    if(require_deathrace_picker_contract(projects::load_project(out))!=contract)throw std::runtime_error("picker contract changed");
    auto mod=out;mod.replace_extension(".fbmod");reskate::modding::mods::write_fbmod(project,mod,args[2],&db);
    std::cout<<"private_race_cells=6 editor_panels=2 race_header=1 confirmation_panel=1 static_logos=11 raw_color=verified bounded_native_parents=58 name_cell_elimination=true single_theme_accent=true fixed_root_geometry=true native_hold_ring_steps=32 picker_contract=preserved\n";return true;
}
