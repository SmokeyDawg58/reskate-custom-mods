// Included inside one_up_menu's private namespace. All views are native Rime
// models, with the same checked ownership/publication helpers as 1-Up.
constexpr std::int32_t mode_hud_id=0x4d4f4400;
void mode_close_phone(const Context& c) {
    // Read the stock card page's own Back navigation rather than inventing a
    // close-state name. The native state machine retains its normal cleanup.
    const auto name=c.text(c.field(state().original_back,0xb1e14cd6));
    if(name.empty())return;
    const auto roots=c.roots({0xcc4776b5});
    if(roots.size()!=1)return;
    const auto navigation=c.field(roots.front().model,0xf3ba9cf8);
    if(read<std::uint32_t>(c.address(navigation)))return;
    c.array(c.field(roots.front().model,0xf437b255),std::vector<std::int32_t>{-1});
    c.set(navigation,game::native_name_hash(name));
}
void mode_art(const Context& c,std::span<const Value> fields,const std::string& logo) {
    if(logo.empty())return;
    try {
        // Texture registration is lazy. The private exported category record
        // already retains the loaded image; use it before the first draw can
        // register that image in the texture-renderer's lookup table.
        std::string key;
        if(logo=="UI/ReSkate/OneUp/img_OneUp_Blue_1024")key="OneUpBadge";
        else for(const auto& d:throwdowns::registry().definitions())if(d.logo==logo) {key=d.key;break;}
        if(!key.empty()) {
            const auto name=std::string("Throwdown_ModeSelect_ContentResources/")+key;
            const auto record=asset(c,name.c_str());
            const auto type=read<Address>(record+0x18),data=read<Address>(record+0x20);
            require(read<Address>(record+8)==c.base+build::native_menu::asset_record_type && type && data &&
                    read<std::uint32_t>(read<Address>(type))==category_schema.hash,"Throwdown logo record schema differs.");
            const auto image=read<Address>(data+c.member(type,0x189084da).offset);
            require(image && (read<Address>(image+8)==c.base+build::native_menu::texture_asset_type ||
                             read<Address>(image+8)==c.base+build::native_menu::image_asset_type),"Throwdown logo record image is not loaded.");
            for(const auto field:fields)if(read<Address>(c.address(field))!=image)c.set(field,image);
            return;
        }
        require(publish_texture(c,fields,logo),"Throwdown logo has not registered yet.");
    }
    catch(const std::exception& error) {
        static std::set<std::string> reported;
        if(reported.insert(logo).second)logging::log(logging::Level::warning,logging::Channel::ui,
            "Throwdown optional artwork {} is unavailable; retaining native fallback: {}",logo,error.what());
    }
}
void mode_cards(const Context& c) {
    auto& s=state(); if(!s.cards.handle || s.originals.size()!=3)return;
    for(const auto& d:throwdowns::registry().definitions()) {
        if(d.adapter!=throwdowns::Adapter::game_mode)continue;
        auto [it,fresh]=s.mode_cards.try_emplace(d.id);auto& card=it->second;
        if(fresh) {
            const auto& donor=s.originals.at(d.donor);
            card.tile=make(c,tile_schema,donor.tile.type);c.copy(card.tile,c.address(donor.tile));
            card.category=make(c,category_schema,donor.category.type);c.copy(card.category,c.address(donor.category));
            card.description=make(c,anchor_schema,donor.description.type);c.copy(card.description,c.address(donor.description));
            const auto label=make(c,label_schema,s.source_label.type);c.copy(label,c.address(s.source_label));plain_label(c,label,d.description);
            auto desc=read<Widget>(c.address(c.field(card.description,widget)));desc.data={0,label.handle};c.set(c.field(card.description,widget),desc);
            auto cat=read<Widget>(c.address(c.field(card.category,widget)));cat.data={0,card.description.handle};c.set(c.field(card.category,widget),cat);
            auto mounted=read<Widget>(c.address(c.path(card.tile,{widget,widget})));mounted.data={0,card.category.handle};c.set(c.path(card.tile,{widget,widget}),mounted);
            plain_label(c,c.field(card.category,0x77b8ed0b),d.name);
            auto dimensions=donor.tile_size;dimensions[0]*=.74f;c.set(c.field(card.tile,0x868043e1),dimensions);
            auto description_size=donor.description_size;description_size[0]*=.74f;c.set(c.field(card.description,0x1cff7243),description_size);
            c.set(c.field(card.category,0x495ffd43),donor.icon_width*.74f);c.set(c.field(card.category,0x24fb5ca8),donor.icon_height*.74f);
            logging::log(logging::Level::info,logging::Channel::ui,"Throwdowns: registered native {} card ({}).",d.name,d.id);
        }
        // Reserve callbacks each pass, so reopening never recycles a live card's
        // delegate. Optional artwork leaves the authored donor intact.
        c.text(c.field(primary(c,card.tile),0xb1e14cd6),"");
        c.set(c.field(primary(c,card.tile),0x9c76b86c),callback(c,"mode-open",d.id));
        mode_art(c,std::array{c.field(card.category,0x189084da),c.field(card.category,0xc2917efc)},d.logo);
    }
}

Value picker_logo(const Context& c,Value tile,Value cat,bool stock,const std::string& name) {
    // Private style copies retain the native button/focus graph. Every fill
    // background state is empty. Only the native circular outline changes on
    // hover/focus; artwork and lettering retain their original colors.
    const auto button_style=record_model(c,"ButtonStyleList/TileButton.RoughPartial.NoDefaultBG");
    const auto empty=c.path(button_style,{0x2d393a3a,0xb6c6aace});
    for(const auto key:{0x24996c65U,0x69cf50caU,0xe8f882b7U,0x182004ffU,0xb0db14beU,0xfadedb6aU})
        c.copy(c.path(button_style,{0x2d393a3a,key}),c.address(empty));
    // Hover decoration is optional; failure to stream a style must never
    // remove all custom challenges from the native Throwdown menu.
    try {
        const auto ring=asset(c,"UI/ReSkate/Throwdowns/LogoHoverRing_FillStyle");
        for(const auto key:{0xf622ea66U,0x943abf2dU,0x287f4815U})
            c.set(c.path(button_style,{0x343abac3,key}),ring);
    } catch(const std::exception& e) {
        static bool reported{};
        if(!std::exchange(reported,true))logging::log(logging::Level::warning,logging::Channel::ui,
            "Throwdown hover ring unavailable; retaining the native focus style: {}",e.what());
    }
    for(const auto flag:{0xc02028cfU,0xa45fc55dU,0xb811842aU,0xe69a9d99U})c.set(c.field(button_style,flag),false);
    c.set(c.path(tile,{widget,0x0fb0d794,0x8cc042ef}),Ref{0,button_style.handle});
    c.set(c.path(tile,{widget,0xa04998ca}),false);
    const auto colors=record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite");
    c.set(c.field(button_style,0xcd4a53c0),colors);
    const auto logo=record_model(c,"Throwdown_ModeSelect_ContentResources/LogoTexture",texture_schema);
    if(stock)c.copy(c.field(logo,0x6e469d2a),c.address(c.field(cat,0x189084da)));
    c.set(c.field(logo,0xf524c836),320.f);c.set(c.field(logo,0x6fbd254e),320.f);
    for(const auto flag:{0x81c94d8aU,0x190e039cU})c.set(c.field(logo,flag),false);
    c.set(c.field(logo,0xd1997d0c),false);c.set(c.field(logo,0xcd4a53c0),colors);
    const auto caption=make(c,label_schema,state().source_label.type);c.copy(caption,c.address(state().source_label));
    plain_label(c,caption,name);
    label_styles(c,caption,"TextStylesList/H52-XBold_White_DropShadow75","TextStylesList/H52-XBold_White_DropShadow75");
    const auto outlined_name=record_model(c,"TextStylesList/H52-XBold_White_DropShadow75");
    c.set(c.field(outlined_name,0x51332fc1),asset(c,"UI/Foundations/Fonts/Styles/Dingo_2pxOutline_Aqua_FontEffect"));
    for(const auto key:{0xa1e84eb1U,0x25189231U})c.set(c.field(caption,key),Ref{0,outlined_name.handle});
    c.set(c.field(caption,0x87ad624c),2);c.set(c.field(caption,0x3d8639d0),1); // Center vertically, right aligned.
    c.set(c.field(caption,0xc13ba637),false);c.set(c.field(caption,0xfc53d427),false);
    c.set(c.field(caption,0xcd71a279),true);c.set(c.field(caption,0x3fa0b887),false);
    const auto contents=record_model(c,"Throwdown_ModeSelect_ContentResources/LogoContents",{0x9ade4bc7,208});
    c.set(c.field(contents,0xa98391bd),Widget{asset(c,"UI/ReSkate/Throwdowns/LogoTexture_Widget"),{0,logo.handle}});
    c.set(c.field(contents,0x759c54dd),Widget{asset(c,"UI/ReSkate/Throwdowns/LogoName_Widget"),{0,caption.handle}});
    // The private native blueprint owns fixed regions: logo (52,16,320,320)
    // and transparent, tilted name (52,341,360,56). Neither is measured by a scrolling
    // list or can displace the other when its font/texture finishes loading.
    // Retain the native tile button, focus behavior and its individual action;
    // replace only its display content with the private native presenter.
    c.set(c.path(tile,{widget,widget}),Widget{asset(c,"UI/ReSkate/Throwdowns/LogoCaptionTile_Widget"),{0,contents.handle}});
    c.set(c.field(tile,0x868043e1),std::array<float,2>{424.f,424.f});
    return logo;
}
void logo_picker(const Context& c) {
    auto& s=state();if(s.originals.size()!=3)return;
    if(!s.picker_grid.handle) {
        s.picker_grid=record_model(c,"Throwdown_ModeSelect_ContentResources/LogoGrid");
        require(read<std::uint32_t>(read<Address>(s.picker_grid.type))==1948428891U,"Throwdown native grid schema differs.");
        c.set(c.field(s.picker_grid,0x55ca89f4),asset(c,"UI/ReSkate/Throwdowns/LogoPickerListItem_Widget"));
        c.set(c.field(s.picker_grid,1913319895U),424.f); // ColumnSize
        c.set(c.field(s.picker_grid,2171233779U),424.f); // RowSize
        c.set(c.field(s.picker_grid,2454275348U),32.f); // ColumnSpacing
        c.set(c.field(s.picker_grid,2923226299U),32.f); // RowSpacing
        c.set(c.field(s.picker_grid,3625715088U),4); // StaticSegmentItemCount
        c.set(c.field(s.picker_grid,3831727006U),0); // RimeUniformGridSegmentCountMode_StaticCount
        c.set(c.field(s.picker_grid,1287003308U),1); // Vertical rows, four columns
        c.set(c.field(s.picker_grid,369153166U),0); // DefaultIndex
        c.set(c.field(s.picker_grid,3523782405U),0); // Inset
        for(const auto flag:{1972029470U,133293336U,3885835458U,3339695017U,235921311U})c.set(c.field(s.picker_grid,flag),false);
        c.set(c.field(s.picker_grid,1163163164U),true); // Clip to the reserved content area.
        c.set(c.field(s.picker_grid,3275567233U),true); // Remember selection on Back.
        s.picker_anchor=record_model(c,"Throwdown_ModeSelect_ContentResources/Throwdown_ModeSelect_LinearList_AnchoredContent",anchor_schema);
        c.set(c.field(s.picker_anchor,widget),Widget{asset(c,"UI/Foundations/Components/Lists/GridList/Widget/GridList_Widget"),{0,s.picker_grid.handle}});
        c.set(c.field(s.picker_anchor,0x1cff7243),std::array<float,2>{1792.f,1336.f});
        c.set(c.field(s.picker_anchor,0x6efc1a61),false);c.set(c.field(s.picker_anchor,0xbe5683d9),false);
        const auto horizontal=c.field(s.picker_anchor,0xde7d30c7);
        c.set(c.field(horizontal,0x99bbed5a),.5f);c.set(c.field(horizontal,0x49943bed),.5f);
        c.set(c.field(horizontal,0x40bfbff4),0.f);c.set(c.field(horizontal,0x1d233266),0.f);
        c.set(c.field(horizontal,0xfec5e0b1),.5f);c.set(c.field(horizontal,0xd01f4a21),1.f);
        for(unsigned i=0;i<3;++i) {
            s.picker_stock[i]=make(c,tile_schema,s.originals[i].tile.type);c.copy(s.picker_stock[i],c.address(s.originals[i].tile));
            const auto names=std::array{"S.K.A.T.E.","Spot Battle","Skate Jam"};
            s.picker_stock_logos[i]=picker_logo(c,s.picker_stock[i],s.originals[i].category,true,names[i]);
        }
        s.oneup_logo=picker_logo(c,s.card,s.category,false,"1-UP");
    }
    std::vector<Handle> refs;
    // ContentPresenterTileListItem consumes a ContentPresenterTileViewModel,
    // not its TileButtonListItemViewModel wrapper. Both begin at byte zero,
    // but the wrapper adds one entry to the engine's flattened field table.
    // Passing its root shifts every cached input scope by one field, turning
    // ScrollViewModel.Text into the adjacent delegate. Keep the owner alive
    // and publish the typed content-tile field handle for each grid cell.
    const auto append_tile=[&](Value tile) {
        const auto inner=c.field(tile,widget);
        require(read<std::uint32_t>(read<Address>(inner.type))==content_tile_schema.hash &&
                size(inner.type)==content_tile_schema.size,"Throwdown grid tile model differs.");
        refs.push_back(inner.handle);
    };
    auto& reskate=s.mode_cards.at("native.skate");
    if(!reskate.logo.handle)reskate.logo=picker_logo(c,reskate.tile,reskate.category,false,"R.E.S.K.A.T.E.");
    mode_art(c,std::array{c.field(reskate.logo,0x6e469d2a)},throwdowns::registry().find("native.skate")->logo);
    append_tile(reskate.tile);
    append_tile(s.picker_stock[1]);append_tile(s.picker_stock[2]);
    append_tile(s.card);
    mode_art(c,std::array{c.field(s.oneup_logo,0x6e469d2a)},"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
    for(const auto& d:throwdowns::registry().definitions())if(d.adapter==throwdowns::Adapter::game_mode && d.id!="native.skate") {
        auto& card=s.mode_cards.at(d.id);
        if(!card.logo.handle)card.logo=picker_logo(c,card.tile,card.category,false,d.name);
        mode_art(c,std::array{c.field(card.logo,0x6e469d2a)},d.logo);
        append_tile(card.tile);
    }
    unsigned count{},stride{};const auto previous=c.array(c.field(s.picker_grid,0xf2c90867),64,count,stride);
    require(stride==sizeof(Handle),"Throwdown grid item handle schema differs.");
    if(previous!=std::vector<std::byte>(std::as_bytes(std::span(refs)).begin(),std::as_bytes(std::span(refs)).end()))
        c.array(c.field(s.picker_grid,0xf2c90867),refs);
    // Only replace the mode selector. Native navigation may reuse this page
    // for another view, or temporarily leave its content array empty.
    if(!s.details && card_list(c,s.page)) {
        const auto slot=mounted_body(c,s.page);if(!slot)return;
        const auto mounted=read<Widget>(c.address(*slot));
        if(mounted.data.handle!=s.picker_anchor.handle)
            c.set(*slot,Widget{asset(c,"UI/Foundations/Templates/Layouts/Anchored/AnchoredContentPresenter_Widget"),{0,s.picker_anchor.handle}});
    }
}
unsigned mode_selector(const Context& c,std::vector<std::string>& shown,const std::string& id,
                       const std::string& title,const std::vector<unsigned>& options,unsigned selected,const char* suffix) {
    auto& s=state();auto [it,fresh]=s.rows.try_emplace(id);auto& r=it->second;
    if(fresh) {
        r.model=make(c,setting_schema);c.copy(c.field(r.model,0x58c9d354),c.address(s.source_label));
        label_styles(c,c.field(r.model,0x58c9d354),"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_Black");
        c.set(c.field(r.model,0xc9bf367d),record_ref(c,"SettingWrapperStylesList/Horizontal.WithDivider"));
        r.selector=record_model(c,"TD_HostParameters_ContentResource/TD_Timer_BasicSelector",selector_schema);
        for(const auto field:{0x4256fd65U,0x85b8c3e6U,0xe88c0f54U})clear_value(c,c.field(r.selector,field));
        c.set(c.field(r.selector,0xeb92bf1c),Ref{});
        for(const auto field:{0x41b10c27U,0xd637e1f4U})c.set(c.field(r.selector,field),Address{});
        const auto list=c.field(r.selector,0x760fc12b);c.set(c.field(list,0x340cfb95),Address{});
        for(const auto value:options) {
            const auto text=make(c,label_schema),choice=make(c,selector_item_schema);
            c.copy(text,c.address(s.source_label));label_styles(c,text,"TextStylesList/H64-XBold_White","TextStylesList/H64-XBold_Black");
            const auto caption=std::to_string(value)+suffix;plain_label(c,text,caption);
            c.text(c.field(choice,0x58c9d354),caption);c.set(c.field(choice,0x34c8bcc2),Ref{0,text.handle});r.choices.push_back(choice.handle);
        }
        const auto found=std::find(options.begin(),options.end(),selected);
        const unsigned index=found==options.end()?0U:static_cast<unsigned>(found-options.begin());
        c.array(c.field(list,rows_field),std::vector<Ref>{});c.array(c.field(list,0xf2c90867),r.choices);
        c.set(c.field(list,0x55511280),static_cast<int>(index));c.set(c.field(list,0x783c4b49),r.choices.at(index));
        c.set(c.field(r.selector,0x369babfe),true);
        c.set(c.field(r.model,content),Widget{asset(c,"UI/Foundations/Components/Settings/Widgets/BasicSelector_Widget"),{0,r.selector.handle}});
        r.tile=make(c,content_tile_schema);
        c.set(c.field(r.tile,widget),Widget{asset(c,"UI/Foundations/Components/Settings/Widgets/SettingWrapper_Widget"),{0,r.model.handle}});
        c.set(c.field(r.tile,0xa04998ca),false);c.set(c.field(r.tile,0xbc750e6a),true);
        c.set(c.path(r.tile,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/TileButton.RoughPartial.Default"));
        c.set(c.path(r.tile,{0x0fb0d794,0xa704272a,0xc52416ef,0x0abf7c31}),Ref{0,r.model.handle});
        r.anchor=compact_anchor(c,r.tile,"UI/Foundations/Components/Buttons/ContentPresenterTile/ContentPresenterTile",172.f,true);
    }
    plain_label(c,c.field(r.model,0x58c9d354),title);
    const auto list=c.field(r.selector,0x760fc12b);unsigned count{},stride{};c.array(c.field(list,0xf2c90867),128,count,stride);
    if(count!=r.choices.size())c.array(c.field(list,0xf2c90867),r.choices);
    const int index=std::clamp(read<int>(c.address(c.field(list,0x55511280))),0,static_cast<int>(options.size()-1));
    shown.push_back(id);r.callback=1; // Selectable row; preserves native focus.
    return options.at(static_cast<unsigned>(index));
}
void mode_publish_rows(const Context& c,std::vector<std::string> shown) {
    auto& s=state();unsigned count{},stride{};c.array(c.field(s.details_list,0xf2c90867),64,count,stride);
    c.array(c.field(s.footer_list,rows_field),std::vector<Ref>{});
    c.array(c.field(s.footer_list,0xf2c90867),std::vector<Handle>{s.footer_back.handle,s.footer_confirm.handle});
    if(shown==s.displayed && count==shown.size())return;
    const auto focus=read<int>(c.address(c.field(s.details_list,0x55511280)));
    const auto focused=focus>=0 && static_cast<std::size_t>(focus)<s.displayed.size()?s.displayed[focus]:std::string{};
    std::vector<Handle> handles;int first=-1,keep=-1;
    for(const auto& id:shown) {
        const auto& r=s.rows.at(id);if(r.callback) {if(first<0)first=static_cast<int>(handles.size());if(focused==id)keep=static_cast<int>(handles.size());}
        handles.push_back(r.anchor.handle);
    }
    c.array(c.field(s.details_list,rows_field),std::vector<Ref>{});
    c.array(c.field(s.details_list,0xf2c90867),handles);c.set(c.field(s.details_list,0x55511280),keep>=0?keep:first);
    s.displayed=std::move(shown);
}
void mode_render(const Context& c) {
    auto& s=state();const auto* d=throwdowns::registry().find(s.selected_mode);if(!d)return;
    const auto v=modes::native_view();const auto h=modes::hud();
    plain_label(c,c.field(s.header,0x00f3b15e),d->name);
    plain_label(c,c.field(s.panel_notice,0x94703efc),d->description);
    mode_art(c,std::array{c.field(s.badge,0x6e469d2a)},d->logo);
    std::vector<std::string> shown{"rules"};
    const bool same=v.active && static_cast<unsigned>(v.settings.mode)==d->mode;
    const bool setup=same && (!v.state || v.state->phase==modes::Phase::setup);
    footer_action(c,s.footer_back,"BACK","mode-back");
    c.set(c.field(back(c,s.page),0x9c76b86c),callback(c,"mode-back"));
    if(setup && v.leading) {
        const auto capacity=mode_selector(c,shown,"mode-players","Players",{1,2,3,4,5,6},v.capacity,"");
        if(capacity!=v.capacity)s.mode_status=modes::command("players",{std::to_string(capacity)});
        if(v.settings.mode==modes::Mode::race) {
            const auto deadline=mode_selector(c,shown,"mode-time","Finish deadline",{30,60,90,120,180,300,600,900,1800,3600},v.settings.duration_s," sec");
            if(deadline!=v.settings.duration_s)s.mode_status=modes::command("time",{std::to_string(deadline)});
        }
        else {
            const auto duration=mode_selector(c,shown,"mode-time","Duration",{30,60,120,180,300,600,900,1800,3600},v.settings.duration_s," sec");
            if(duration!=v.settings.duration_s)s.mode_status=modes::command("time",{std::to_string(duration)});
        }
        if(d->supports_turns) {
            const auto turn=mode_selector(c,shown,"mode-hide-time",v.settings.mode==modes::Mode::skate?"Turn Timer":"Hide Time",{10,20,30,45,60,90,120},v.settings.turn_s," sec");
            if(turn!=v.settings.turn_s)s.mode_status=modes::command("turn",{std::to_string(turn)});
        }
        if(d->needs_points)row(c,shown,"mode-points",d->key=="race"?"Place race route":"Place capture spots","mode-place","points");
        // Optional areas reuse the supplied in-world placement/input logic.
        // They do not constrain scoring unless deliberately placed.
        if(!d->needs_points)row(c,shown,"mode-area","Set optional area","mode-place","circle");
        footer_action(c,s.footer_confirm,"START","mode-start");
        auto offer_mode=d->name;
        std::transform(offer_mode.begin(),offer_mode.end(),offer_mode.begin(),[](unsigned char ch){return static_cast<char>(std::toupper(ch));});
        for(const auto& offer:h.offers)if(!offer.own && offer.open && offer.mode==offer_mode)
            row(c,shown,"mode-join-"+std::to_string(offer.id),"Join "+offer.host,"mode-join",std::to_string(offer.id));
    } else {
        row(c,shown,"mode-state",same?h.status:"This challenge is not running.");
        if(same)for(unsigned i=0;i<h.rows.size() && i<6;++i)row(c,shown,"mode-standing-"+std::to_string(i),h.rows[i].name+"  "+h.rows[i].value);
        footer_action(c,s.footer_confirm,same?"QUIT MATCH":"SET UP",same?"mode-quit":"mode-new");
    }
    if(!s.mode_status.empty() && s.mode_status.starts_with("error:"))row(c,shown,"mode-error",s.mode_status);
    mode_publish_rows(c,std::move(shown));
}
void mode_action(const Context& c,const Action& a) {
    auto& s=state();
    if(a.command=="mode-back") {discard_mode_setup();switch_page(c,false);s.selected_mode.clear();return;}
    if(a.command=="mode-open") {
        const auto* d=throwdowns::registry().find(a.argument);
        if(!d || d->adapter!=throwdowns::Adapter::game_mode)return;
        one_up::retire_unconfirmed_native_setup();
        auto v=modes::native_view();
        s.mode_status.clear();
        if(v.active && (static_cast<unsigned>(v.settings.mode)!=d->mode || (v.state && v.state->phase==modes::Phase::results))) {
            if(!discard_mode_setup()) {s.mode_status="error: quit the active challenge first.";return;}
            v.active=false;
        }
        if(!v.active)s.mode_status=modes::command("new",{d->key});
        if(s.mode_status.starts_with("error:"))return;
        s.entry_guard_until=GetTickCount64()+350;
        // Every custom mode enters the game's real HostParameters flow. The
        // private adaptation retains its native layout, phone pose and input;
        // Confirm enters native flag placement and waiting before explicit Start.
        s.mode_status=modes::command("setup",{});
        if(s.mode_status.starts_with("error:"))logging::log(logging::Level::warning,logging::Channel::ui,"Throwdown setup: {}",s.mode_status);
        s.selected_mode.clear();return;
    }
    const auto* d=throwdowns::registry().find(s.selected_mode);if(!d)return;
    if(a.command=="mode-new")s.mode_status=modes::command("new",{d->key});
    else if(a.command=="mode-start") {s.mode_status=modes::command("start",{});if(!s.mode_status.starts_with("error:")) {switch_page(c,false);mode_close_phone(c);}}
    else if(a.command=="mode-quit") {s.mode_status=modes::command("quit",{});switch_page(c,false);}
    else if(a.command=="mode-place") {s.mode_status=modes::command("place",{a.argument});if(!s.mode_status.starts_with("error:")) {switch_page(c,false);mode_close_phone(c);}}
    else if(a.command=="mode-join") {
        const auto v=modes::native_view();
        if(v.active && v.leading && (!v.state || v.state->phase==modes::Phase::setup))modes::command("stop",{});
        s.mode_status=modes::command("join",{a.argument});s.displayed.clear();
    }
}
void mode_hud_publish(const Context& c,bool show,bool countdown,bool intro,bool blind,bool results,bool race) {
    auto& h=state().mode_hud;if(!h.stack.handle || c.type_of(h.stack.handle)!=h.stack.type)return;
    const bool ready=hud_publication_allowed(read<bool>(c.address(c.field(h.stack,0x369babfe))),state().in_menu,
        (show || countdown || intro || blind || results || race) && state().match_panel_open.load());
    const bool settled=h.publication.allow(h.stack.handle,h.game,ready,GetTickCount64());
    if(!ready || ((show || countdown || intro || blind || results || race) && !settled)) {
        if(const auto found=state().hud_surfaces.find(h.stack.handle);found!=state().hud_surfaces.end()) {
            if(!ready)found->second.suspended=true;
            hide_hud_surface_content(c,found->second);
        }
        return;
    }
    std::vector<Value> visible;
    const auto append=[&](Value item) {if(item.handle)visible.push_back(item);};
    if(show && h.placing_layout)append(h.editor_panel_item);
    if(race && h.gate_compass_visible)append(h.gate_compass_item);
    // Challenge titles now sit compactly above the compass; no large racing header panel.
    if(results)append(h.results_panel_item);
    for(const auto item:{blind?h.blind_item:Value{},show?h.item:Value{},show?h.badge_item:Value{},show && h.placing_layout?h.placement_item:Value{},show && !h.placing_layout && h.clock_visible?h.clock_item:Value{},show && !h.placing_layout && !h.race_layout?h.score_item:Value{},countdown?h.countdown_item:Value{},intro?h.intro_item:Value{},results?h.results_item:Value{},race?h.race_item:Value{},race?h.race_position_item:Value{},race?h.race_elapsed_item:Value{},race?h.race_deadline_item:Value{},race?h.race_deadline_caption_item:Value{},race?h.race_progress_item:Value{},show && h.race_warning_visible?h.race_warning_item:Value{},show && h.placing_layout && h.clear_ring_visible?h.clear_ring_item:Value{},show && h.exit_visible?h.exit_panel_item:Value{},show && h.exit_visible?h.exit_item:Value{}})append(item);
    if(show && !blind && !results)for(const auto item:h.world_items)append(item);
    publish_hud_surface(c,h.stack,visible,true);
}
Address mode_hud_art(const Context& c,modes::Mode mode,const char* part) {
    return asset(c,(std::string("UI/ReSkate/Throwdowns/HUD_")+std::string(native_hud_theme_key(modes::mode_key(mode)))+"_"+part+"_Widget").c_str());
}
std::uint64_t mode_racer_elapsed(const modes::NativeView& v,const modes::Standing& row,std::uint64_t now) {
    const bool running=v.state && v.state->phase==modes::Phase::playing && !row.out &&
        static_cast<std::size_t>(std::max<std::int64_t>(0,row.score))<v.settings.points.size();
    return native_hud_race_elapsed(static_cast<std::uint64_t>(std::max(0,row.aux)),
        v.state_sampled_ms,now,running,static_cast<std::uint64_t>(v.settings.duration_s)*1000);
}
std::string mode_racer_time(const modes::NativeView& v,const modes::Standing& row,std::uint64_t now) {
    return (row.out?"DNF  ":"")+native_hud_race_time(mode_racer_elapsed(v,row,now));
}
void mode_race_clocks(const Context& c,const modes::NativeView& v,std::uint64_t now) {
    auto& h=state().mode_hud;
    if(v.settings.mode!=modes::Mode::race || !v.state || h.game!=v.game)return;
    const auto clocks=native_hud_race_clocks(v.state->remaining_ms,v.state_sampled_ms,now,
        v.state->phase==modes::Phase::playing,static_cast<std::uint64_t>(v.settings.duration_s)*1000);
    if(h.race_deadline.handle && c.type_of(h.race_deadline.handle)==h.race_deadline.type)
        plain_label(c,c.field(h.race_deadline,0x94703efc),native_hud_race_time(clocks.time_to_beat));
    for(const auto& row:v.state->standings) {
        const auto found=h.race_players.find(row.player);
        if(found!=h.race_players.end() && found->second.timer.handle &&
           c.type_of(found->second.timer.handle)==found->second.timer.type)
            plain_label(c,c.field(found->second.timer,0x94703efc),mode_racer_time(v,row,now));
        if(row.player==v.local && h.race_elapsed.handle && c.type_of(h.race_elapsed.handle)==h.race_elapsed.type)
            plain_label(c,c.field(h.race_elapsed,0x94703efc),native_hud_race_time(clocks.countdown));
    }
}
void mode_hud_cell(const Context& c,Value value,const std::string& text,float width,float height,const char* font) {
    match_panel_table_style(c,value);
    c.set_if_changed(c.field(value,0x13be0d51),std::array<float,2>{width,height});
    // The donor notice carries its own label constraint. Retaining it clips
    // narrow position cells even when the surrounding frame is wide enough.
    match_panel_notice_width(c,value,std::max(1.f,width-24.f));
    c.set_if_changed(c.field(value,0x54258200),false);
    const auto label=c.field(value,0x94703efc);plain_label(c,label,text);
    label_styles(c,label,font,font);c.set_if_changed(c.field(label,0xc13ba637),false);c.set_if_changed(c.field(label,0xfc53d427),false);
    c.set_if_changed(c.field(label,0x3d8639d0),2);c.set_if_changed(c.field(label,0x87ad624c),2);
}
void mode_hud_list_cell_size(const Context& c,Value list,unsigned i,float width,float height,Widget display={}) {
    const auto handle=read<Handle>(c.address(c.element(c.field(list,0xf2c90867),i)));
    const Value anchor{handle,c.type_of(handle)};
    c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{width,height});
    // Weight 1 retains the explicit measured bounds. Zero collapsed the list
    // cells and separated the rendered frames from their labels.
    for(const auto axis:{0xde7d30c7U,0x185a5736U})c.set_if_changed(c.path(anchor,{axis,0xd01f4a21}),1.f);
    c.set_if_changed(c.field(anchor,0x6efc1a61),false);
    c.set_if_changed(c.field(anchor,0xbe5683d9),false);
    if(display.blueprint) {
        const auto slot=c.field(anchor,widget);const auto old=read<Widget>(c.address(slot));
        if(old.blueprint!=display.blueprint || old.data.handle!=display.data.handle)c.set_if_changed(slot,display);
    }
}
void challenge_header(const Context& c,ChallengeHeader& h,std::string_view theme,const std::string& name,
        const std::string& logo,Value status,std::int32_t frame_id,std::int32_t content_id) {
    const auto size=native_hud_header_geometry(name);
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const auto label_widget=asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget");
    const auto texture_widget=asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget");
    if(!h.frame.handle)h.frame=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.frame,"",float(size.frame_width),float(size.height),"TextStylesList/D78-Caps-Bold_Bright");
    const auto frame_art=asset(c,("UI/ReSkate/Throwdowns/HUD_"+std::string(theme)+"_Title_"+std::to_string(size.frame_width)+"_Widget").c_str());
    mount_hud(c,h.frame,frame_art,h.frame_anchor,h.frame_item,frame_id,false);
    c.set_if_changed(c.field(h.frame_anchor,widget),Widget{frame_art,{0,h.frame.handle}});
    plain_label(c,h.title,native_hud_title_text(name));label_styles(c,h.title,"TextStylesList/D78-Caps-Bold_Bright","TextStylesList/D78-Caps-Bold_Bright");
    c.set_if_changed(c.field(h.title,0x3d8639d0),0);c.set_if_changed(c.field(h.title,0x87ad624c),2);
    c.set_if_changed(c.field(h.title,0xc13ba637),false);c.set_if_changed(c.field(h.title,0xfc53d427),false);
    // Use the native texture view's measured dimensions. Private static logo
    // widgets carry a 144px image and ignore an 80px surrounding anchor.
    c.set_if_changed(c.field(h.badge,0xf524c836),float(size.icon));c.set_if_changed(c.field(h.badge,0x6fbd254e),float(size.icon));
    c.set_if_changed(c.field(h.badge,0xd1997d0c),false);
    c.set_if_changed(c.field(h.badge,0xcd4a53c0),record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite"));
    mode_art(c,std::array{c.field(h.badge,0x6e469d2a)},logo);
    if(!hud_list_has_items(c,h.row,2))hud_list(c,h.row,{{texture_widget,{0,h.badge.handle}},{label_widget,{0,h.title.handle}}},true,float(size.content_height),float(size.gap));
    mode_hud_list_cell_size(c,h.row,0,float(size.icon),float(size.icon),{texture_widget,{0,h.badge.handle}});
    const auto badge_cell_handle=read<Handle>(c.address(c.element(c.field(h.row,0xf2c90867),0)));
    const Value badge_cell{badge_cell_handle,c.type_of(badge_cell_handle)};
    const auto badge_offset=native_hud_translation(float(size.content_height-size.icon)*.5f+size.badge_y_shift);
    c.set_if_changed(c.path(badge_cell,{0x185a5736U,0x40bfbff4U}),badge_offset[0]);
    c.set_if_changed(c.path(badge_cell,{0x185a5736U,0x1d233266U}),badge_offset[1]);
    mode_hud_list_cell_size(c,h.row,1,float(size.title_width),float(size.text_height),{label_widget,{0,h.title.handle}});
    const auto title_cell_handle=read<Handle>(c.address(c.element(c.field(h.row,0xf2c90867),1)));
    const Value title_cell{title_cell_handle,c.type_of(title_cell_handle)};
    const auto title_offset=native_hud_translation(size.title_y_shift);
    c.set_if_changed(c.path(title_cell,{0x185a5736U,0x40bfbff4U}),title_offset[0]);
    c.set_if_changed(c.path(title_cell,{0x185a5736U,0x1d233266U}),title_offset[1]);
    const unsigned count=status.handle?2:1;
    if(!hud_list_has_items(c,h.content,count)) {
        std::vector<Widget> rows{{list_widget,{0,h.row.handle}}};if(status.handle)rows.push_back({label_widget,{0,status.handle}});
        hud_list(c,h.content,rows,false,float(size.content_height),8.f,0.f,float(size.content_width));
    }
    mode_hud_list_cell_size(c,h.content,0,float(size.content_width),float(size.content_height));
    if(status.handle)mode_hud_list_cell_size(c,h.content,1,float(size.content_width),48.f);
    mount_hud(c,h.content,list_widget,h.anchor,h.item,content_id,false);
    for(const auto anchor:{h.frame_anchor,h.anchor}) {
        const bool frame=anchor.handle==h.frame_anchor.handle;
        c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{float(frame?size.frame_width:size.content_width),frame?float(size.height):status.handle?float(size.content_height)+56.f:float(size.content_height)});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool vertical=axis==0x185a5736U;
            for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(anchor,{axis,key}),vertical?native_hud_screen_anchor(.008f):.5f);
            c.set_if_changed(c.path(anchor,{axis,0xfec5e0b1}),vertical?0.f:.5f);
            c.set_if_changed(c.path(anchor,{axis,0xd01f4a21}),1.f);
            const bool race=theme=="race";
            const float move=vertical?(frame?0.f:float(size.height-size.content_height)*.5f)-(race?6.f:0.f):(race?16.f:0.f);
            const auto offsets=native_hud_translation(move);
            c.set_if_changed(c.path(anchor,{axis,0x40bfbff4U}),offsets[0]);
            c.set_if_changed(c.path(anchor,{axis,0x1d233266U}),offsets[1]);
        }
    }
}
Address mode_hud_caption_art(const Context& c,float width) {
    return asset(c,("UI/ReSkate/Throwdowns/Caption_"+std::to_string(static_cast<unsigned>(width))+"_Widget").c_str());
}
Address mode_hud_heading_art(const Context& c,float width,unsigned height) {
    return asset(c,("UI/ReSkate/Throwdowns/Heading_"+std::to_string(static_cast<unsigned>(width))+"_"+std::to_string(height)+"_Widget").c_str());
}
void mode_board_columns(const Context& c,Value& list,std::array<Value,3>& labels,const std::array<float,3>& widths,bool race) {
    std::vector<Widget> cells;
    for(unsigned i=0;i<3;++i) {
        if(!labels[i].handle)labels[i]=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        mode_hud_cell(c,labels[i],i==0?"POS":i==1?"NAME":race?"TIME":"SCORE",widths[i],52.f,"TextStylesList/H52-XBold_White");
        cells.push_back({mode_hud_caption_art(c,widths[i]),{0,labels[i].handle}});
    }
    if(!hud_list_has_items(c,list,3))hud_list(c,list,cells,true,52.f,8.f);
    for(unsigned i=0;i<3;++i)mode_hud_list_cell_size(c,list,i,widths[i],52.f,cells[i]);
}
void mode_race_standings(const Context& c,const modes::NativeView& v,const overlay::ModesHud& feed,ModeHud* target=nullptr) {
    auto& h=target?*target:state().mode_hud;
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const bool race=v.settings.mode==modes::Mode::race;
    float name_width=224.f,value_width=race?320.f:224.f;
    const auto now=GetTickCount64();
    for(const auto& row:feed.rows)name_width=std::max(name_width,float(native_hud_name_width(row.name)));
    for(unsigned i=0;v.state && i<v.state->standings.size() && i<feed.rows.size();++i)
        value_width=std::max(value_width,float(native_hud_value_width(race?mode_racer_time(v,v.state->standings[i],now):feed.rows[i].value)));
    // Reserve DNF/deadline width before a running clock reaches that state.
    if(race)value_width=std::max(value_width,float(native_hud_value_width("DNF  "+native_hud_race_time(static_cast<std::uint64_t>(v.settings.duration_s)*1000))));
    h.race_widths={100.f,name_width,value_width};const float width=116.f+name_width+value_width;
    constexpr float height=76.f,pitch=84.f;
    std::vector<Widget> refs;std::vector<Handle> order;unsigned local_place{};
    for(unsigned i=0;v.state && i<v.state->standings.size() && i<feed.rows.size() && i<6;++i) {
        const auto& player=v.state->standings[i];auto& row=h.race_players[player.player];
        if(player.player==v.local && !player.out)local_place=i+1;
        std::array<Value*,3> values{&row.rank,&row.name,&row.timer};std::vector<Widget> cells;
        for(unsigned column=0;column<3;++column) {
            if(!values[column]->handle)*values[column]=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
            const std::string text=column==0?(player.out?"--":std::to_string(i+1)+(i==0?"st":i==1?"nd":i==2?"rd":"th")):
                column==1?feed.rows[i].name:race?mode_racer_time(v,player,now):player.out?"OUT":feed.rows[i].value;
            mode_hud_cell(c,*values[column],text,h.race_widths[column],height,"TextStylesList/H52-XBold_White");
            std::string art;
            if(column==0)art="UI/ReSkate/Throwdowns/"+std::string(race?"RaceRank":"HUD_"+std::string(native_hud_theme_key(modes::mode_key(v.settings.mode)))+"_Rank")+"_Widget";
            else if(column==1)art="UI/ReSkate/Throwdowns/"+std::string(race?"RaceName":"HUD_"+std::string(native_hud_theme_key(modes::mode_key(v.settings.mode)))+"_Name")+"_"+std::to_string(static_cast<unsigned>(name_width))+"_Widget";
            else art="UI/ReSkate/Throwdowns/"+std::string(race?"RaceTime":"HUD_"+std::string(native_hud_theme_key(modes::mode_key(v.settings.mode)))+"_Value")+"_"+std::to_string(static_cast<unsigned>(value_width))+"_Widget";
            cells.push_back({asset(c,art.c_str()),{0,values[column]->handle}});
        }
        if(!hud_list_has_items(c,row.list,3))hud_list(c,row.list,cells,true,height,8.f);
        for(unsigned col=0;col<3;++col)mode_hud_list_cell_size(c,row.list,col,h.race_widths[col],height,cells[col]);
        order.push_back(player.player);refs.push_back({list_widget,{0,row.list.handle}});
    }
    if(h.race_player_order!=order || !hud_list_has_items(c,h.race_rows,static_cast<unsigned>(refs.size())))
        hud_list(c,h.race_rows,refs,false,height,8.f,0.f,width);
    h.race_player_order=order;
    for(unsigned i=0;i<refs.size();++i)mode_hud_list_cell_size(c,h.race_rows,i,width,height);
    if(feed.results)return; // Results owns these rows inside its single board.
    if(!h.race_title.handle)h.race_title=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.race_title,"LEADERBOARD",width,64.f,"TextStylesList/H52-XBold_White");
    mode_board_columns(c,h.race_columns,h.race_captions,h.race_widths,race);
    const auto heading=mode_hud_heading_art(c,width,64);
    if(!hud_list_has_items(c,h.race_board,3))hud_list(c,h.race_board,{{heading,{0,h.race_title.handle}},
        {list_widget,{0,h.race_columns.handle}},{list_widget,{0,h.race_rows.handle}}},false,64.f,12.f,0.f,width);
    const float rows_height=pitch*static_cast<float>(refs.size());
    mode_hud_list_cell_size(c,h.race_board,0,width,64.f,{heading,{0,h.race_title.handle}});
    mode_hud_list_cell_size(c,h.race_board,1,width,52.f);
    mode_hud_list_cell_size(c,h.race_board,2,width,rows_height);
    mount_hud(c,h.race_board,list_widget,h.race_anchor,h.race_item,mode_hud_id+6,false);
    c.set_if_changed(c.field(h.race_anchor,0x1cff7243),std::array<float,2>{width,140.f+rows_height});
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const bool vertical=axis==0x185a5736U;
        for(const auto field:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(h.race_anchor,{axis,field}),vertical?native_hud_screen_anchor(native_hud_side_center):native_hud_screen_anchor(.994f));
        c.set_if_changed(c.path(h.race_anchor,{axis,0xfec5e0b1}),vertical?.5f:1.f);
        c.set_if_changed(c.path(h.race_anchor,{axis,0xd01f4a21}),1.f);
        for(const auto field:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(h.race_anchor,{axis,field}),0.f);
    }
    if(!race)return;
    if(!h.race_position.handle)h.race_position=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.race_position,local_place?std::to_string(local_place)+(local_place==1?"ST":local_place==2?"ND":local_place==3?"RD":"TH"):"--",480.f,300.f,"TextStylesList/D94-Caps-Bold_White");
    c.set_if_changed(c.path(h.race_position,{0x94703efc,0x3d8639d0}),2);
    const auto position_art=asset(c,("UI/ReSkate/Throwdowns/RacePosition_"+std::string(local_place==1?"Gold":local_place==2?"Silver":local_place==3?"Bronze":"White")+"_Widget").c_str());
    mount_hud(c,h.race_position,position_art,h.race_position_anchor,h.race_position_item,mode_hud_id+41,false);
    c.set_if_changed(c.field(h.race_position_anchor,widget),Widget{position_art,{0,h.race_position.handle}});
    if(!h.race_elapsed.handle)h.race_elapsed=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    constexpr float clock_width=384.f;
    mode_hud_cell(c,h.race_elapsed,native_hud_race_time(native_hud_race_remaining(
        v.state->remaining_ms,v.state_sampled_ms,now,v.state->phase==modes::Phase::playing)),clock_width,100.f,"TextStylesList/H64-XBold_White");
    mount_hud(c,h.race_elapsed,asset(c,"UI/ReSkate/Throwdowns/RaceElapsed_Widget"),h.race_elapsed_anchor,h.race_elapsed_item,mode_hud_id+61,false);
    if(!h.race_deadline.handle)h.race_deadline=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.race_deadline,native_hud_race_time(static_cast<std::uint64_t>(v.settings.duration_s)*1000),clock_width,100.f,"TextStylesList/H64-XBold_White");
    mount_hud(c,h.race_deadline,asset(c,"UI/ReSkate/Throwdowns/RaceElapsed_Widget"),h.race_deadline_anchor,h.race_deadline_item,mode_hud_id+62,false);
    if(!h.race_deadline_caption.handle)h.race_deadline_caption=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
    plain_label(c,h.race_deadline_caption,"TIME TO BEAT");
    label_styles(c,h.race_deadline_caption,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
    c.set_if_changed(c.field(h.race_deadline_caption,0x3d8639d0),2);
    mount_hud(c,h.race_deadline_caption,asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),h.race_deadline_caption_anchor,h.race_deadline_caption_item,mode_hud_id+63,false);
    const auto place=[&](Value anchor,float w,float height,float x,float y) {
        c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{w,height});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool vertical=axis==0x185a5736U;
            for(const auto field:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(anchor,{axis,field}),vertical?native_hud_screen_anchor(.008f):native_hud_screen_anchor(.006f));
            c.set_if_changed(c.path(anchor,{axis,0xfec5e0b1}),0.f);
            c.set_if_changed(c.path(anchor,{axis,0xd01f4a21}),1.f);
            for(const auto field:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(anchor,{axis,field}),vertical?y:x);
        }
    };
    place(h.race_position_anchor,480.f,300.f,24.f,0.f);
    place(h.race_elapsed_anchor,clock_width,100.f,520.f,0.f);
    place(h.race_deadline_caption_anchor,clock_width,56.f,520.f,110.f);
    place(h.race_deadline_anchor,clock_width,100.f,520.f,172.f);
    if(!h.race_progress.handle)h.race_progress=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    std::size_t cleared{};
    for(const auto& row:v.state->standings)if(row.player==v.local)
        cleared=modes::mandatory_gates(v.settings,static_cast<std::size_t>(std::max(0,row.score)));
    mode_hud_cell(c,h.race_progress,"REQUIRED "+std::to_string(cleared)+" / "+
        std::to_string(modes::mandatory_gates(v.settings,v.settings.points.size())),480.f,72.f,"TextStylesList/H52-XBold_White");
    mount_hud(c,h.race_progress,mode_hud_art(c,modes::Mode::race,"Score"),h.race_progress_anchor,h.race_progress_item,mode_hud_id+46,false);
    // Slot 46 is already reserved in the fixed 64-slot mode catalog. Revealing
    // it updates Content only; the stable native stack is never rebuilt.
    place(h.race_progress_anchor,480.f,72.f,24.f,324.f);
    mode_race_clocks(c,v,now);
}
void mode_results(const Context& c,const modes::NativeView& v,const overlay::ModesHud& feed,const throwdowns::Definition& d,ModeHud* target=nullptr) {
    auto& h=target?*target:state().mode_hud;mode_race_standings(c,v,feed,&h);
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const float row_width=h.race_widths[0]+h.race_widths[1]+h.race_widths[2]+16.f;
    const auto panel_width=static_cast<unsigned>(std::ceil(std::max(1164.f,row_width+64.f)/32.f))*32u;
    const float inside_width=static_cast<float>(panel_width)-64.f;
    const float title_width=inside_width-168.f;
    const auto count=std::clamp<unsigned>(static_cast<unsigned>(feed.rows.size()),1,6);
    const float panel_height=432.f+84.f*count,rows_height=84.f*static_cast<float>(count);
    if(!h.results_title.handle)h.results_title=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.results_title,d.name+" RESULTS",title_width,144.f,"TextStylesList/D94-Caps-Bold_White");
    c.set_if_changed(c.path(h.results_title,{0x94703efc,0x3d8639d0}),0);
    const auto heading=mode_hud_heading_art(c,title_width,144);
    const auto logo=mode_hud_art(c,v.settings.mode,"Logo");
    if(!hud_list_has_items(c,h.results_header,2))hud_list(c,h.results_header,{{logo,{}},{heading,{0,h.results_title.handle}}},true,144.f,24.f);
    mode_hud_list_cell_size(c,h.results_header,0,144.f,144.f,{logo,{}});
    mode_hud_list_cell_size(c,h.results_header,1,title_width,144.f,{heading,{0,h.results_title.handle}});
    if(!h.results_notice.handle)h.results_notice=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    mode_hud_cell(c,h.results_notice,feed.winner.empty()?(v.settings.mode==modes::Mode::race?"NO FINISHERS / DNF":"MATCH COMPLETE"):feed.winner+" WINS!",inside_width,96.f,"TextStylesList/H52-XBold_White");
    const auto notice=asset(c,("UI/ReSkate/Throwdowns/HUD_"+std::string(native_hud_theme_key(modes::mode_key(v.settings.mode)))+"_Results_"+std::to_string(panel_width-64)+"_Widget").c_str());
    mode_board_columns(c,h.results_columns,h.results_captions,h.race_widths,v.settings.mode==modes::Mode::race);
    if(!hud_list_has_items(c,h.results,4))hud_list(c,h.results,{{list_widget,{0,h.results_header.handle}},
        {notice,{0,h.results_notice.handle}},{list_widget,{0,h.results_columns.handle}},{list_widget,{0,h.race_rows.handle}}},false,144.f,24.f,0.f,inside_width);
    mode_hud_list_cell_size(c,h.results,0,inside_width,144.f);
    mode_hud_list_cell_size(c,h.results,1,inside_width,96.f,{notice,{0,h.results_notice.handle}});
    mode_hud_list_cell_size(c,h.results,2,row_width,52.f);
    mode_hud_list_cell_size(c,h.results,3,row_width,rows_height);
    const auto panel=asset(c,("UI/ReSkate/Throwdowns/ResultsPanel"+std::to_string(count)+"_"+std::to_string(panel_width)+"_Widget").c_str());
    mount_hud(c,{},panel,h.results_panel_anchor,h.results_panel_item,mode_hud_id+60,true);
    c.set_if_changed(c.field(h.results_panel_anchor,widget),Widget{panel,{}});
    mount_hud(c,h.results,list_widget,h.results_anchor,h.results_item,mode_hud_id+5,true);
    const auto place=[&](Value anchor,float width,float height,float padding) {
        c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{width,height});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool vertical=axis==0x185a5736U;
            for(const auto field:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(anchor,{axis,field}),.5f);
            c.set_if_changed(c.path(anchor,{axis,0xfec5e0b1}),0.f);
            c.set_if_changed(c.path(anchor,{axis,0xd01f4a21}),1.f);
            for(const auto field:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(anchor,{axis,field}),
                (vertical?-panel_height*.5f:-static_cast<float>(panel_width)*.5f)+padding);
        }
    };
    place(h.results_panel_anchor,static_cast<float>(panel_width),panel_height,0.f);
    place(h.results_anchor,inside_width,panel_height-64.f,32.f);
    h.results_game=v.game;h.results_theme=static_cast<unsigned>(v.settings.mode);
}

void mode_world_markers(const Context& c,const overlay::ModesHud& feed) {
    auto& h=state().mode_hud;h.world_items.clear();
    // Native arches already identify the route. Screen-space notice markers
    // obscure their center and show the donor notice's unbound rectangle.
    if(feed.route || !(feed.vertical_fov>1 && feed.vertical_fov<175) || !feed.blind.empty() || feed.results)return;
    const auto& camera=feed.camera;
    static float aspect=16.f/9.f;
    DWORD process{};const auto window=GetForegroundWindow();GetWindowThreadProcessId(window,&process);
    RECT client{};
    if(process==GetCurrentProcessId() && GetClientRect(window,&client) && client.bottom>client.top)
        aspect=static_cast<float>(client.right-client.left)/static_cast<float>(client.bottom-client.top);
    const auto position=[&](const std::array<float,3>& p)->std::optional<std::array<float,2>> {
        const std::array<float,3> delta{p[0]-camera[12],p[1]+1.5f-camera[13],p[2]-camera[14]};
        const auto dot=[&](unsigned i){return delta[0]*camera[i]+delta[1]*camera[i+1]+delta[2]*camera[i+2];};
        const float depth=-dot(8);if(depth<=.1f)return {};
        const float focal=1.f/(2.f*std::tan(feed.vertical_fov*3.14159265f/360.f));
        const float x=.5f+dot(0)*focal/depth/aspect,y=.5f-dot(4)*focal/depth;
        if(!std::isfinite(x) || !std::isfinite(y))return {};
        return std::array<float,2>{std::clamp(x,.08f,.92f),std::clamp(y,.08f,.9f)};
    };
    const auto mark=[&](const std::array<float,3>& at,const std::string& text) {
        if(h.world_items.size()>=h.markers.size())return;
        const auto xy=position(at);if(!xy)return;
        const auto index=static_cast<unsigned>(h.world_items.size());auto& marker=h.markers[index];
        if(!marker.notice.handle)marker.notice=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        auto caption=text;
        if(feed.have_me) {
            float distance{};for(unsigned axis=0;axis<3;++axis)distance+=(at[axis]-feed.me[axis])*(at[axis]-feed.me[axis]);
            caption+="  "+std::to_string(static_cast<int>(std::sqrt(distance)))+" m";
        }
        plain_label(c,c.field(marker.notice,0x94703efc),caption);
        label_styles(c,c.field(marker.notice,0x94703efc),"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
        c.set(c.field(marker.notice,0x13be0d51),std::array<float,2>{320.f,82.f});
        mount_hud(c,marker.notice,asset(c,"UI/Foundations/Components/Notices/BasicNotice_Widget"),marker.anchor,marker.item,mode_hud_id+8+index,true);
        c.set(c.field(marker.anchor,0x1cff7243),std::array<float,2>{320.f,82.f});
        for(unsigned axis=0;axis<2;++axis)for(const auto field:{0x99bbed5aU,0x49943bedU})
            c.set(c.path(marker.anchor,{axis?0x185a5736U:0xde7d30c7U,field}),(*xy)[axis]);
        h.world_items.push_back(marker.item);
    };
    for(unsigned i=0;i<feed.points.size();++i) {
        if(feed.route && !feed.placing && static_cast<int>(i)<feed.next_point)continue;
        std::string label=feed.route?(i==0?"START":i+1==feed.points.size() && modes::native_view().route_finish?"FINISH":"CHECKPOINT "+std::to_string(i)):"CAPTURE "+std::to_string(i+1);
        if(static_cast<int>(i)==feed.next_point)label="NEXT: "+label;
        mark(feed.points[i],label);
    }
    const auto placement=modes::native_view();
    if(feed.placing && placement.cursor_valid)mark(feed.cursor,feed.route?"PLACE: "+std::string(placement.route_object==0?"START FLAG":placement.route_object==2?"FINISH":placement.route_object==3?"GUIDE":placement.route_object==4?"CHEVRON":"REQUIRED"):"PLACE HERE");
    for(const auto& player:feed.players)if(!player.self && player.it)mark(player.at,player.name+(feed.infection?"  INFECTED":"  IT"));
    for(const auto& tag:feed.tags)if(!tag.path.empty())mark(tag.path[tag.path.size()/2],"TAGGED");
}
void mode_gate_compass_position(const Context& c,ModeHud& h,float offset) {
    constexpr float size=modes::race_compass_icon_size;
    c.set_if_changed(c.field(h.gate_compass_anchor,0x1cff7243),std::array<float,2>{size,size});
    c.set_if_changed(c.field(h.gate_compass_anchor,0x6efc1a61),false);
    c.set_if_changed(c.field(h.gate_compass_anchor,0xbe5683d9),false);
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const bool vertical=axis==0x185a5736U;
        // Center the larger visible gate on the native compass strip.
        for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(h.gate_compass_anchor,{axis,key}),vertical?0.f:.5f);
        const auto offsets=native_hud_translation(vertical?modes::race_compass_center_y:offset);
        c.set_if_changed(c.path(h.gate_compass_anchor,{axis,0x40bfbff4U}),offsets[0]);
        c.set_if_changed(c.path(h.gate_compass_anchor,{axis,0x1d233266U}),offsets[1]);
        c.set_if_changed(c.path(h.gate_compass_anchor,{axis,0xfec5e0b1}),.5f);
        c.set_if_changed(c.path(h.gate_compass_anchor,{axis,0xd01f4a21}),1.f);
    }
}
void mode_gate_compass_motion(const Context& c,const modes::NativeView& view,const overlay::ModesHud& feed) {
    auto& h=state().mode_hud;
    if(!h.gate_compass_visible || !h.gate_compass_anchor.handle || view.placing || !view.state ||
       c.type_of(h.gate_compass_anchor.handle)!=h.gate_compass_anchor.type)return;
    const auto next=modes::race_compass_gate(view.settings,*view.state,view.local);if(!next)return;
    const auto offset=modes::race_compass_offset(view.settings.points[*next],feed.camera);if(!offset)return;
    mode_gate_compass_position(c,h,*offset);
}
void mode_gate_compass(const Context& c,const modes::NativeView& view,const overlay::ModesHud& feed) {
    auto& h=state().mode_hud;h.gate_compass_visible=false;
    if(view.placing || !view.state)return;
    const auto next=modes::race_compass_gate(view.settings,*view.state,view.local);if(!next)return;
    const auto offset=modes::race_compass_offset(view.settings.points[*next],feed.camera);if(!offset)return;
    if(!h.gate_compass.handle)h.gate_compass=make(c,texture_schema);
    constexpr float size=modes::race_compass_icon_size;
    c.set_if_changed(c.field(h.gate_compass,0xf524c836),size);c.set_if_changed(c.field(h.gate_compass,0x6fbd254e),size);
    c.set_if_changed(c.field(h.gate_compass,0xd1997d0c),false);
    c.set_if_changed(c.field(h.gate_compass,0xcd4a53c0),record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite"));
    mode_art(c,std::array{c.field(h.gate_compass,0x6e469d2a)},"UI/ReSkate/Throwdowns/img_GateCompass");
    const auto texture_widget=asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget");
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    if(!hud_list_has_items(c,h.gate_compass_list,1))hud_list(c,h.gate_compass_list,{{texture_widget,{0,h.gate_compass.handle}}},true,size,0.f);
    mode_hud_list_cell_size(c,h.gate_compass_list,0,size,size,{texture_widget,{0,h.gate_compass.handle}});
    mount_hud(c,h.gate_compass_list,list_widget,h.gate_compass_anchor,h.gate_compass_item,mode_hud_id+57,false);
    mode_gate_compass_position(c,h,*offset);
    h.gate_compass_visible=true;
}
void mode_hud_transients(const Context& c,const modes::NativeView& view,std::uint64_t now) {
    auto& h=state().mode_hud;
    const auto* challenge=view.active && view.state?&*view.state:nullptr;
    h.transients=h.countdown_lifecycle.update(view.active?view.game:0,
        challenge && challenge->phase==modes::Phase::countdown,
        challenge && challenge->phase==modes::Phase::playing,challenge?challenge->remaining_ms:0,now);
    if(!h.stack.handle || c.type_of(h.stack.handle)!=h.stack.type ||
       !read<bool>(c.address(c.field(h.stack,0x369babfe))))return;
    for(const auto [item,keep]:std::array<std::pair<Value,bool>,2>{{
        {h.countdown_item,h.transients.countdown},{h.intro_item,h.transients.intro}}}) {
        if(!keep && item.handle && c.type_of(item.handle)==item.type)
            c.set_if_changed(c.field(item,0x369babfe),false);
    }
}
void mode_hud(const Context& c) {
    auto& s=state();auto& h=s.mode_hud;const auto v=modes::native_view();const auto feed=modes::hud();
    const auto* d=v.active?throwdowns::registry().mode(static_cast<unsigned>(v.settings.mode)):nullptr;
    // The native creation page and waiting notification own setup. Gameplay
    // widgets must not mount into a retiring HUD while the phone is opening.
    if(h.game && h.game!=v.game)mode_hud_publish(c,false);
    if(!d || (!v.placing && (!v.state || v.state->phase==modes::Phase::setup))) {
        mode_hud_publish(c,false);return;
    }
    const auto label_widget=asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget");
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const bool race=v.settings.mode==modes::Mode::race;
    mode_gate_compass(c,v,feed);
    if(!h.title.handle) {
        h.title=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");label_styles(c,h.title,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
        h.status=make(c,label_schema);c.copy(h.status,c.address(h.title));
        h.score=make(c,linear_list);
        h.target=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        h.current=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        for(const auto value:{h.target,h.current}) {
            c.set_if_changed(c.field(value,0x13be0d51),std::array<float,2>{480.f,72.f});
            const auto label=c.field(value,0x94703efc);label_styles(c,label,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
            c.set_if_changed(c.field(label,0x3d8639d0),0);c.set_if_changed(c.field(label,0x87ad624c),2);c.set_if_changed(c.field(label,0xc13ba637),false);
        }
        h.clock=record_model(c,"Activities_HudElementData_ContentResource/A_ActivityCountdown");
    }
    plain_label(c,h.title,race?"DEATHRACE":d->name);
    const auto title_style="TextStylesList/H64-XBold_White";
    label_styles(c,h.title,title_style,title_style);
    c.set_if_changed(c.field(h.title,0x3d8639d0),2);
    plain_label(c,h.status,feed.placing && race?"":feed.placing?feed.hint:feed.status);
    label_styles(c,h.status,"TextStylesList/SP38-SemiB_White",
                 "TextStylesList/SP38-SemiB_White");
    for(const auto value:{h.title,h.status}) {
        c.set_if_changed(c.field(value,0xc13ba637),false);c.set_if_changed(c.field(value,0xfc53d427),false);
        c.set_if_changed(c.field(value,0x87ad624c),2);
    }
    h.placing_layout=feed.placing && race;
    h.exit_visible=h.placing_layout && v.exit_confirm;
    if(h.placing_layout) {
        mount_hud(c,{},asset(c,"UI/ReSkate/Throwdowns/EditorPanel_Widget"),h.editor_panel_anchor,h.editor_panel_item,mode_hud_id+43,false);
        c.set_if_changed(c.field(h.editor_panel_anchor,0x1cff7243),std::array<float,2>{880.f,1150.f});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(h.editor_panel_anchor,{axis,key}),axis==0xde7d30c7U?native_hud_screen_anchor(.006f):native_hud_screen_anchor(.008f));
            for(const auto key:{0x40bfbff4U,0x1d233266U,0xfec5e0b1U})c.set_if_changed(c.path(h.editor_panel_anchor,{axis,key}),0.f);
        }
    }
    if(feed.placing && race) {
        // LabelButton resolves the actual game's device glyphs and text style.
        // Each action gets its own bounded row instead of a concatenated line.
        constexpr std::array actions{
            std::pair{"UI_TabSecondaryNavLeft",""},
            std::pair{"UI_TabSecondaryNavRight","Gate types"},
            std::pair{"UI_TabPrimaryNavLeft",""},
            std::pair{"UI_TabPrimaryNavRight","Camera down / up"},
            std::pair{"UI_PrimaryAction","Place object"},
            std::pair{"UI_SecondaryAction","Rotate"},
            std::pair{"UI_DpadLeft",""},
            std::pair{"UI_DpadRight","Undo / Redo"},
            std::pair{"UI_DpadUp",""},
            std::pair{"UI_DpadDown","Size"},
            std::pair{"UI_Activities_AccessPause","Save route"},
            std::pair{"UI_Back","Close editor"},
            std::pair{"UI_TertiaryAction","Toggle snapping"},
            std::pair{"UI_L3Click",""},
            std::pair{"UI_R3Click","Snap angle - / +"},
            std::pair{"UI_SelectButton","HOLD: Clear all gates"}};
        bool created{};
        std::vector<Widget> prompts;
        for(unsigned i=0;i<actions.size();++i) {
            auto& prompt=h.placement_prompts[i];
            if(!prompt.handle) {
                const auto checkpoint=s.owned.checkpoint();
                try {
                // Construct independent controls, borrowing only immutable
                // native styling/layout. The details-button donor carries a
                // backend caption and action graph for "Throwdown Panel".
                const auto donor=record_model(c,"Throwdown_DetailsMenu_ContentResource/ThrowdownDetails_HUDButton",button);
                const auto candidate=make(c,button);
                for(const auto key:{0x8cc042efU,0xf9515817U,0xf343d160U})
                    c.copy(c.field(candidate,key),c.address(c.field(donor,key)));
                c.copy(c.path(candidate,{0x0fb0d794,0x8cc042ef}),c.address(c.path(donor,{0x0fb0d794,0x8cc042ef})));
                PrivateUiGraph graph{c};graph.walk(candidate,0);
                clear_value(c,c.field(candidate,0x67212c85));
                const auto action=c.path(candidate,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
                clear_value(c,c.field(action,0x9c76b86c));
                c.text(c.field(action,0xb1e14cd6),"");
                const auto interact=c.path(candidate,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a});
                // DingoInteractable reflection fields, checked against the
                // game's native type map; graph property hashes differ.
                c.set_if_changed(c.field(interact,0x38f7355f),true);
                c.set_if_changed(c.field(interact,0x4b3146e9),false);
                label_styles(c,c.field(candidate,0x58c9d354),"TextStylesList/SP48-SemiB_White_DropShadow75","TextStylesList/SP48-SemiB_White_DropShadow75");
                plain_label(c,c.field(candidate,0x58c9d354),"");
                const auto prompt_input=asset(c,(std::string("Configurations/Input/UI/")+actions[i].first).c_str());
                c.set_if_changed(c.field(action,0x3d00de27),prompt_input);
                c.set_if_changed(c.field(action,0x0169c2db),prompt_input);
                prompt=candidate;created=true; // Publish only a complete prompt.
                } catch(...) {
                    s.owned.rollback(checkpoint,[&](Value value){c.destroy(value);});
                    throw;
                }
            }
            // Native input/device changes may rebind the widget. Reconcile our
            // private caption and PIA without recreating its presenter/list.
            plain_label(c,c.field(prompt,0x58c9d354),"");
            const auto action=c.path(prompt,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
            const auto prompt_input=asset(c,(std::string("Configurations/Input/UI/")+actions[i].first).c_str());
            for(const auto key:{0x3d00de27U,0x0169c2dbU})
                if(read<Address>(c.address(c.field(action,key)))!=prompt_input)c.set_if_changed(c.field(action,key),prompt_input);
            prompts.push_back({asset(c,"UI/ReSkate/Throwdowns/EditorGlyph_Widget"),{0,prompt.handle}});
        }
        std::vector<Widget> lines;
        for(unsigned i=0;i<prompts.size();++i) {
            const int pair=i==0?0:i==2?1:i==6?2:i==8?3:i==13?4:-1;
            Widget icons=prompts[i];
            const char* caption=actions[i].second;
            if(pair>=0) {
                auto& list=h.placement_pairs[pair];
                if(!list.handle || created) {
                    hud_list(c,list,{prompts[i],prompts[i+1]},true,56.f,8.f,0.f,120.f,56.f);
                }
                icons={list_widget,{0,list.handle}};
                caption=actions[++i].second;
            }
            const auto line_index=lines.size();
            auto& label=h.placement_captions[line_index];
            if(!label.handle) {
                label=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
                label_styles(c,label,"TextStylesList/SP48-SemiB_White_DropShadow75","TextStylesList/SP48-SemiB_White_DropShadow75");
                c.set_if_changed(c.field(label,0xc13ba637),false);c.set_if_changed(c.field(label,0xfc53d427),false);
                c.set_if_changed(c.field(label,0x87ad624c),2);c.set_if_changed(c.field(label,0x3d8639d0),0);
            }
            std::string dynamic_caption=caption;
            if(i==1)dynamic_caption=std::string("Object: ")+(v.route_object==2?"Finish":v.route_object==3?"Guide":v.route_object==4?"Chevron":"Required");
            if(i==5)dynamic_caption=v.rotating?"Rotate: ON (X to finish)":"Rotate";
            if(i==7)dynamic_caption=v.rotating?"Yaw - / +":"Undo / Redo";
            if(i==10 && !v.editor_warning.empty())dynamic_caption=v.route_finish?"Check route":"Finish line required";
            if(i==9)dynamic_caption=v.rotating?"Tilt up / down":"Size "+std::to_string(static_cast<int>(std::lround(v.cursor_width/6.f*100.f)))+"%";
            if(i==12)dynamic_caption=v.route_snap?(v.arrow_height_snapped?"Snap ON (Chevron line locked)":std::format("Snap ON ({:.1f}m reach)",std::clamp(v.snap_degrees*.1f,.5f,6.f))):"Snap OFF";
            if(i==14)dynamic_caption="Rotation step: "+std::to_string(static_cast<int>(v.snap_degrees))+" deg";
            plain_label(c,label,dynamic_caption);
            auto& line=h.placement_lines[line_index];
            if(!line.handle || created) {
                const auto text=Widget{asset(c,"UI/ReSkate/Throwdowns/EditorCaption_Widget"),{0,label.handle}};
                std::vector<Widget> cells{icons,text};
                hud_list(c,line,cells,true,56.f,12.f,0.f,820.f,pair>=0?120.f:56.f);
                const auto refs=c.field(line,0xf2c90867);
                for(unsigned cell=0;cell<cells.size();++cell) {
                    const auto handle=read<Handle>(c.address(c.element(refs,cell)));
                    const float width=cell+1==cells.size()?500.f:pair>=0?120.f:56.f;
                    c.set_if_changed(c.field({handle,c.type_of(handle)},0x1cff7243),std::array<float,2>{width,56.f});
                }
            }
            lines.push_back({list_widget,{0,line.handle}});
        }
        h.clear_ring_visible=false; // Progress is a child of Select, not a screen item.
        if(!h.exit_visible && v.clear_progress>0 && !lines.empty()) {
            const auto frame=std::clamp(static_cast<unsigned>(std::ceil(std::clamp(v.clear_progress,0.f,1.f)*32.f)),1u,32u);
            const auto ring=asset(c,("UI/ReSkate/Throwdowns/EditorClearGlyph"+std::to_string(frame)+"_Widget").c_str());
            const auto refs=c.field(h.placement_lines[lines.size()-1],0xf2c90867);
            const auto handle=read<Handle>(c.address(c.element(refs,0)));
            const auto slot=c.field({handle,c.type_of(handle)},widget);
            c.set_if_changed(slot,Widget{ring,{0,h.placement_prompts.back().handle}});
        } else if(!lines.empty()) {
            const auto refs=c.field(h.placement_lines[lines.size()-1],0xf2c90867);
            const auto handle=read<Handle>(c.address(c.element(refs,0)));
            c.set_if_changed(c.field({handle,c.type_of(handle)},widget),prompts.back());
        }
        if(h.exit_visible) {
            mount_hud(c,{},asset(c,"UI/ReSkate/Throwdowns/ConfirmationPanel_Widget"),h.exit_panel_anchor,h.exit_panel_item,mode_hud_id+44,true);
            if(!h.exit_text.handle){h.exit_text=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");label_styles(c,h.exit_text,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");}
            plain_label(c,h.exit_text,"LEAVE THE EDITOR?\nYour course is saved.");
            c.set_if_changed(c.field(h.exit_text,0xc13ba637),true);
            std::vector<Widget> modal{{label_widget,{0,h.exit_text.handle}}};
            for(unsigned i=0;i<2;++i) {
                auto& caption=h.exit_captions[i];
                if(!caption.handle){caption=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");label_styles(c,caption,"TextStylesList/SP48-SemiB_White_DropShadow75","TextStylesList/SP48-SemiB_White_DropShadow75");}
                plain_label(c,caption,i==0?"Leave editor":"Keep editing");
                if(!hud_list_has_items(c,h.exit_actions[i],2))hud_list(c,h.exit_actions[i],{{asset(c,"UI/ReSkate/Throwdowns/EditorGlyph_Widget"),{0,h.placement_prompts[i==0?4:11].handle}},
                    {asset(c,"UI/ReSkate/Throwdowns/EditorCaption_Widget"),{0,caption.handle}}},true,56.f,20.f,0.f,800.f,56.f);
                modal.push_back({list_widget,{0,h.exit_actions[i].handle}});
            }
            if(!hud_list_has_items(c,h.exit_list,3))hud_list(c,h.exit_list,modal,false,76.f,10.f,0.f,820.f);
            const auto modalRows=c.field(h.exit_list,0xf2c90867);
            const auto header=read<Handle>(c.address(c.element(modalRows,0)));
            c.set_if_changed(c.field({header,c.type_of(header)},0x1cff7243),std::array<float,2>{820.f,120.f});
            mount_hud(c,h.exit_list,list_widget,h.exit_anchor,h.exit_item,mode_hud_id+45,true);
            for(const auto anchor:{h.exit_panel_anchor,h.exit_anchor}) {
                c.set_if_changed(c.field(anchor,0x1cff7243),std::array<float,2>{880.f,300.f});
                for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
                    for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(anchor,{axis,key}),.5f);
                    c.set_if_changed(c.path(anchor,{axis,0xfec5e0b1}),.5f);
                    for(const auto key:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(anchor,{axis,key}),0.f);
                }
            }
        }
        if(created || !hud_list_has_items(c,h.placement_list,static_cast<unsigned>(lines.size())))
            hud_list(c,h.placement_list,lines,false,56.f,6.f,0.f,820.f);
        mount_hud(c,h.placement_list,list_widget,h.placement_anchor,h.placement_item,mode_hud_id+7,false);
        c.set_if_changed(c.field(h.placement_anchor,0x1cff7243),std::array<float,2>{820.f,930.f});
        for(const auto field:{0x99bbed5aU,0x49943bedU}) {
            c.set_if_changed(c.path(h.placement_anchor,{0xde7d30c7,field}),native_hud_screen_anchor(.006f));
            c.set_if_changed(c.path(h.placement_anchor,{0x185a5736,field}),native_hud_screen_anchor(.008f));
        }
        c.set_if_changed(c.path(h.placement_anchor,{0xde7d30c7,0xfec5e0b1}),0.f);
        c.set_if_changed(c.path(h.placement_anchor,{0x185a5736,0xfec5e0b1}),0.f);
        for(const auto axis:{0xde7d30c7U,0x185a5736U})for(const auto field:{0x40bfbff4U,0x1d233266U})
            c.set_if_changed(c.path(h.placement_anchor,{axis,field}),axis==0xde7d30c7U?24.f:24.f);

    }
    if(!feed.blind.empty())plain_label(c,h.status,"Hiding time: "+feed.blind);
    else if(feed.heat>=0)plain_label(c,h.status,feed.status+"  |  "+feed.heat_label);
    if(h.theme_mode!=static_cast<unsigned>(v.settings.mode) || !hud_list_has_items(c,h.score,2)) {
        h.theme_mode=static_cast<unsigned>(v.settings.mode);h.race_layout=race;
        hud_list(c,h.score,{{mode_hud_art(c,v.settings.mode,"Score"),{0,h.target.handle}},
            {mode_hud_art(c,v.settings.mode,"Score"),{0,h.current.handle}}},false,72.f,8.f,0.f,480.f);
    }
    if(!h.badge.handle)h.badge=make(c,texture_schema);
    h.header.title=h.title;h.header.badge=h.badge;
    challenge_header(c,h.header,native_hud_theme_key(modes::mode_key(v.settings.mode)),race?"DEATHRACE":d->name,d->logo,
        race?Value{}:h.status,mode_hud_id,mode_hud_id+40);
    h.anchor=h.header.frame_anchor;h.item=h.header.frame_item;
    h.badge_anchor=h.header.anchor;h.badge_item=h.header.item;
    h.race_warning_visible=race && v.missed_gate>0;
    if(h.race_warning_visible) {
        if(!h.race_warning.handle)h.race_warning=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
        for(const auto key:{0x8cc042efU,0xa1e84eb1U})c.set_if_changed(c.field(h.race_warning,key),record_ref(c,"NoticeStylesList/BasicNotice_Alert"));
        const auto label=c.field(h.race_warning,0x94703efc);
        plain_label(c,label,"MISSED GATE "+std::to_string(v.missed_gate)+(v.recovering?" - RETURNING TO CHECKPOINT":" - CHECKPOINT RECOVERY"));
        label_styles(c,label,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
        c.set_if_changed(c.field(h.race_warning,0x13be0d51),std::array<float,2>{1000.f,96.f});
        mount_hud(c,h.race_warning,asset(c,"UI/ReSkate/Throwdowns/RaceWarning_Widget"),h.race_warning_anchor,h.race_warning_item,mode_hud_id+42,true);
        c.set_if_changed(c.field(h.race_warning_anchor,0x1cff7243),std::array<float,2>{1000.f,96.f});
        for(const auto field:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(h.race_warning_anchor,{0x185a5736,field}),.3f);
    }
    const auto state=v.state;std::int32_t best{},mine{};
    if(state)for(const auto& standing:state->standings) {best=std::max(best,standing.score);if(standing.player==v.local)mine=standing.score;}
    plain_label(c,c.field(h.target,0x94703efc),"LEADER   "+std::to_string(best));
    plain_label(c,c.field(h.current,0x94703efc),"SCORE   "+std::to_string(mine));
    if(!race) {
        mount_hud(c,h.score,list_widget,h.score_anchor,h.score_item,mode_hud_id+59,false);
        c.set_if_changed(c.field(h.score_anchor,0x1cff7243),std::array<float,2>{480.f,152.f});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const bool y=axis==0x185a5736U;
            for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.path(h.score_anchor,{axis,key}),y?native_hud_screen_anchor(native_hud_side_center):native_hud_screen_anchor(.994f));
            c.set_if_changed(c.path(h.score_anchor,{axis,0xfec5e0b1}),1.f);
            for(const auto key:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.path(h.score_anchor,{axis,key}),y?-(140.f+84.f*static_cast<float>(std::min<std::size_t>(6,feed.rows.size())))*.5f-24.f:0.f);
            c.set_if_changed(c.path(h.score_anchor,{axis,0xd01f4a21}),1.f);
        }
    }
    if(h.stack.handle!=s.hud_stack.handle) {mode_hud_publish(c,false);h.stack=s.hud_stack;}
    mount_hud(c,h.clock,asset(c,"UI/Foundations/Components/Countdown/Countdown_Widget"),h.clock_anchor,h.clock_item,mode_hud_id+1,false);
    c.set_if_changed(c.field(h.clock_anchor,0x1cff7243),std::array<float,2>{320.f,90.f});
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const auto layout=c.field(h.clock_anchor,axis);
        const bool vertical=axis==0x185a5736U;
        for(const auto key:{0x99bbed5aU,0x49943bedU})c.set_if_changed(c.field(layout,key),vertical?native_hud_screen_anchor(.988f):.5f);
        c.set_if_changed(c.field(layout,0xfec5e0b1),vertical?1.f:.5f);
        c.set_if_changed(c.field(layout,0xd01f4a21),1.f);
        for(const auto key:{0x40bfbff4U,0x1d233266U})c.set_if_changed(c.field(layout,key),0.f);
    }
    c.set_if_changed(c.field(h.clock,0x703c4d93),false);c.set_if_changed(c.field(h.clock,0x167131bb),true);c.set_if_changed(c.field(h.clock,0x6186003c),true);c.set_if_changed(c.field(h.clock,0xc11082ee),true);
    h.clock_visible=!race && !feed.clock.empty();
    native_timer_seconds(c,h.clock,0x4cc5ec89,race?modes::race_warning_ms:static_cast<std::uint64_t>(v.settings.duration_s)*1000);
    auto remaining=state?state->remaining_ms:0;
    if(race && state && state->phase==modes::Phase::playing) {
        const auto me=std::find_if(state->standings.begin(),state->standings.end(),[&](const auto& row){return row.player==v.local;});
        remaining=me!=state->standings.end()?me->remaining_ms:0;
    }
    native_timer_seconds(c,h.clock,0x0ec4a8c0,remaining);
    const bool counting=state && state->phase==modes::Phase::countdown;
    const bool intro=h.transients.intro;
    if(v.game!=h.game)h.game=v.game;
    const bool go=h.transients.go;
    const bool blind=!feed.blind.empty();
    if(blind) {
        mount_hud(c,{},asset(c,"UI/ReSkate/Throwdowns/SeekerBlackout_Widget"),h.blind_anchor,h.blind_item,mode_hud_id+4,true);
        c.set_if_changed(c.field(h.blind_anchor,0x1cff7243),std::array<float,2>{1920.f,1080.f});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const auto layout=c.field(h.blind_anchor,axis);
            c.set_if_changed(c.field(layout,0x99bbed5a),0.f);c.set_if_changed(c.field(layout,0x49943bed),1.f);
            c.set_if_changed(c.field(layout,0x40bfbff4),0.f);c.set_if_changed(c.field(layout,0x1d233266),0.f);
            c.set_if_changed(c.field(layout,0xfec5e0b1),.5f);
        }
    }
    if(intro) {
        if(!h.intro.handle) {
            h.intro=record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner");
            c.set_if_changed(c.field(h.intro,0xf79313be),record_ref(c,"ActivityBannerStylesList/ActivityBannerCenterFullWidthStyle.Throwdown.Default"));
            c.copy(c.field(h.intro,0xcbc57608),c.address(record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner_ContentWithTitle")));
            h.intro_badge=make(c,texture_schema);c.set_if_changed(c.field(h.intro_badge,0xf524c836),256.f);c.set_if_changed(c.field(h.intro_badge,0x6fbd254e),256.f);
            c.set_if_changed(c.path(h.intro,{0xcbc57608,content}),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,h.intro_badge.handle}});
        }
        plain_label(c,c.path(h.intro,{0xcbc57608,0x44688629}),d->name);plain_label(c,c.path(h.intro,{0xcbc57608,0x00f3b15e}),d->description);
        mode_art(c,std::array{c.field(h.intro_badge,0x6e469d2a)},d->logo);
        mount_hud(c,h.intro,asset(c,"UI/Features/Activities/Blueprints/ActivityBanner_CenterFullWidth_Widget"),h.intro_anchor,h.intro_item,mode_hud_id+3,true);
        c.set_if_changed(c.field(h.intro_anchor,0x1cff7243),std::array<float,2>{1920.f,600.f});
    }
    if((counting && !intro) || go) {
        if(!h.countdown.handle)h.countdown=record_model(c,"Activities_HudElementData_ContentResource/Activities_CountdownItemHUD");
        plain_label(c,c.field(h.countdown,0x58c9d354),go?"GO!":std::to_string(native_hud_seconds(state->remaining_ms)));
        c.set_if_changed(c.field(h.countdown,1698918111U),go?0.f:std::clamp(static_cast<float>(state->remaining_ms)/3000.f,0.f,1.f));
        mount_hud(c,h.countdown,asset(c,"UI/Features/Activities/Widgets/System/ActivityCountdown_Widget"),h.countdown_anchor,h.countdown_item,mode_hud_id+2,true);
        c.set_if_changed(c.field(h.countdown_anchor,0x1cff7243),std::array<float,2>{512.f,512.f});
    }
    mode_world_markers(c,feed);
    if(state && state->phase==modes::Phase::playing && !feed.results && !feed.placing)mode_race_standings(c,v,feed);
    if(feed.results)mode_results(c,v,feed,*d);
    mode_hud_publish(c,!feed.results,(counting && !intro)||go,intro,blind,feed.results,state && state->phase==modes::Phase::playing && !feed.results && !feed.placing);
}
