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
    const auto ring=asset(c,"UI/ReSkate/Throwdowns/LogoHoverRing_FillStyle");
    for(const auto key:{0xf622ea66U,0x943abf2dU,0x287f4815U})
        c.set(c.path(button_style,{0x343abac3,key}),ring);
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
    for(const auto tile:s.picker_stock)append_tile(tile);
    append_tile(s.card);
    mode_art(c,std::array{c.field(s.oneup_logo,0x6e469d2a)},"UI/ReSkate/OneUp/img_OneUp_Blue_1024");
    for(const auto& d:throwdowns::registry().definitions())if(d.adapter==throwdowns::Adapter::game_mode) {
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
        const auto duration=mode_selector(c,shown,"mode-time","Duration",{30,60,120,180,300,600,900,1800,3600},v.settings.duration_s," sec");
        if(duration!=v.settings.duration_s)s.mode_status=modes::command("time",{std::to_string(duration)});
        if(d->supports_turns) {
            const auto turn=mode_selector(c,shown,"mode-hide-time","Hide Time",{10,20,30,45,60,90,120},v.settings.turn_s," sec");
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
    const auto target=c.field(h.stack,items);unsigned count{},stride{};const auto old=c.array(target,64,count,stride);
    require(stride==stack_item.size,"Native mode HUD stack ABI differs.");
    const auto offset=c.member(c.type(stack_item),0x1e95752c).offset;std::vector<std::byte> next;
    for(unsigned i=0;i<count;++i) {
        std::int32_t id{};std::memcpy(&id,old.data()+i*stride+offset,4);
        if(id<mode_hud_id || id>=mode_hud_id+64)next.insert(next.end(),old.begin()+i*stride,old.begin()+(i+1)*stride);
    }
    for(const auto item:{blind?h.blind_item:Value{},show?h.item:Value{},show && h.race_layout?h.badge_item:Value{},show && h.placing_layout?h.placement_item:Value{},show && !h.placing_layout?h.clock_item:Value{},countdown?h.countdown_item:Value{},intro?h.intro_item:Value{},results?h.results_item:Value{},race?h.race_item:Value{}})if(item.handle) {
        const auto at=next.size();next.resize(at+stride);require(memory::peek_bytes(c.address(item),next.data()+at,stride),"Native mode HUD item unavailable.");
    }
    if(show && !blind && !results)for(const auto item:h.world_items) {
        const auto at=next.size();next.resize(at+stride);require(memory::peek_bytes(c.address(item),next.data()+at,stride),"Native objective marker unavailable.");
    }
    if(next!=old)c.array(target,next,static_cast<unsigned>(next.size()/stride));
}
void mode_race_standings(const Context& c,const modes::NativeView& v,const overlay::ModesHud& feed) {
    auto& h=state().mode_hud;
    const auto label_widget=asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget");
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    if(!h.crown.handle) {
        h.crown=make(c,texture_schema);
        c.set(c.field(h.crown,0x6e469d2a),asset(c,"ui/textures/icons/activities/throwdowns/icon/img_icon_throwdown_crown_512"));
        c.set(c.field(h.crown,0xf524c836),40.f);c.set(c.field(h.crown,0x6fbd254e),40.f);
        c.set(c.field(h.crown,0xd1997d0c),false);
        c.set(c.field(h.crown,0xcd4a53c0),record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite"));
        h.crown_space=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");plain_label(c,h.crown_space,"");
    }
    std::vector<Widget> refs;
    std::vector<Handle> order;
    for(unsigned i=0;v.state && i<v.state->standings.size() && i<feed.rows.size() && i<6;++i) {
        const auto& player=v.state->standings[i];auto& row=h.race_players[player.player];
        if(!row.rank.handle) {
            for(auto* label:{&row.rank,&row.name,&row.timer}) {
                *label=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
                label_styles(c,*label,"TextStylesList/SP38-SemiB_White","TextStylesList/SP38-SemiB_White");
                c.set(c.field(*label,0xc13ba637),false);c.set(c.field(*label,0xfc53d427),false);
                c.set(c.field(*label,0x87ad624c),2);
            }
        }
        plain_label(c,row.rank,std::to_string(i+1)+(i==0?"st":i==1?"nd":i==2?"rd":"th"));
        plain_label(c,row.name,feed.rows[i].name);
        c.set(c.field(row.name,0xfc53d427),true);
        c.set(c.field(row.name,0x3d8639d0),2);
        const bool finished=static_cast<std::size_t>(player.score)>=v.settings.points.size();
        const auto seconds=native_hud_seconds(finished?static_cast<std::uint64_t>(std::max(0,player.aux)):player.remaining_ms);
        plain_label(c,row.timer,player.out?"DNF":std::to_string(seconds/60)+":"+(seconds%60<10?"0":"")+std::to_string(seconds%60));
        order.push_back(player.player);
        if(!row.list.handle || row.place!=i) {
            std::vector<Widget> cells{{label_widget,{0,row.rank.handle}},{label_widget,{0,row.name.handle}},{label_widget,{0,row.timer.handle}}};
            cells.insert(cells.begin(),i==0?Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,h.crown.handle}}:Widget{label_widget,{0,h.crown_space.handle}});
            hud_list(c,row.list,cells,true,40.f,8.f,0.f,500.f);
            unsigned count{},stride{};c.array(c.field(row.list,0xf2c90867),4,count,stride);
            for(unsigned col=0;col<count;++col) {
                const auto ref=read<Handle>(c.address(c.element(c.field(row.list,0xf2c90867),col)));
                const Value anchor{ref,c.type_of(ref)};
                const float width=col==0?40.f:col==1?60.f:col==2?260.f:100.f;
                c.set(c.field(anchor,0x1cff7243),std::array<float,2>{width,56.f});
                c.set(c.field(anchor,0x6efc1a61),col==2);
                // The native rough strip sits above the row origin. Keep
                // text centered inside it and the crown on the row baseline.
                for(const auto key:{0x40bfbff4U,0x1d233266U})
                    c.set(c.path(anchor,{0x185a5736,key}),col==0?0.f:-14.f);
            }
            row.place=i;
            if(!row.background.handle) {
                row.background=make(c,content_tile_schema);
                c.set(c.field(row.background,widget),Widget{list_widget,{0,row.list.handle}});
                c.set(c.field(row.background,0xa04998ca),false);c.set(c.field(row.background,0xbc750e6a),false);
                c.set(c.field(row.background,0xe14b64f1),std::array<float,4>{0.f,0.f,0.f,0.f});
                c.set(c.path(row.background,{0x0fb0d794,0x8cc042ef}),record_ref(c,"ButtonStyleList/TileButton.RoughPartial.Default"));
            }
        }
        refs.push_back({asset(c,"UI/Foundations/Components/Buttons/ContentPresenterTile/ContentPresenterTile"),{0,row.background.handle}});
    }
    if(h.race_player_order!=order || !hud_list_has_items(c,h.race_rows,static_cast<unsigned>(refs.size())))
        hud_list(c,h.race_rows,refs,false,56.f,6.f,0.f,500.f);
    h.race_player_order=order;
    unsigned row_count{},row_stride{};
    const auto row_refs=c.field(h.race_rows,0xf2c90867);
    c.array(row_refs,6,row_count,row_stride);
    for(unsigned i=0;i<row_count;++i) {
        const auto handle=read<Handle>(c.address(c.element(row_refs,i)));
        c.set(c.field({handle,c.type_of(handle)},0x6efc1a61),true);
        c.set(c.field({handle,c.type_of(handle)},0xbe5683d9),false);
    }
    mount_hud(c,h.race_rows,list_widget,h.race_anchor,h.race_item,mode_hud_id+6,false);
    c.set(c.field(h.race_anchor,0x1cff7243),std::array<float,2>{500.f,62.f*static_cast<float>(refs.size())});
    for(const auto field:{0x99bbed5aU,0x49943bedU}) {
        c.set(c.path(h.race_anchor,{0xde7d30c7,field}),.95f);
        c.set(c.path(h.race_anchor,{0x185a5736,field}),.48f);
    }
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        c.set(c.path(h.race_anchor,{axis,0xfec5e0b1}),axis==0x185a5736?.5f:1.f);
        c.set(c.path(h.race_anchor,{axis,0xd01f4a21}),1.f);
        for(const auto key:{0x40bfbff4U,0x1d233266U})c.set(c.path(h.race_anchor,{axis,key}),0.f);
    }
    c.set(c.field(h.race_anchor,0x6efc1a61),true);
}
void mode_results(const Context& c,const modes::NativeView& v,const overlay::ModesHud& feed,const throwdowns::Definition& d) {
    auto& h=state().mode_hud;
    if(h.results_game!=v.game) {h.results_game=v.game;h.result_players.clear();}
    if(!h.results_title.handle)h.results_title=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
    plain_label(c,h.results_title,d.name+" RESULTS");
    label_styles(c,h.results_title,"TextStylesList/D94-Caps-Bold_White","TextStylesList/D94-Caps-Bold_White");
    if(!h.results_notice.handle)h.results_notice=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    plain_label(c,c.field(h.results_notice,0x94703efc),feed.winner.empty()?"MATCH COMPLETE":feed.winner+" WINS!");
    c.set(c.field(h.results_notice,0x13be0d51),std::array<float,2>{1200.f,130.f});
    if(v.settings.mode==modes::Mode::race) {
        mode_race_standings(c,v,feed);
        const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
        hud_list(c,h.results,{{asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,h.results_title.handle}},
            {asset(c,"UI/Foundations/Components/Notices/BasicNotice_Widget"),{0,h.results_notice.handle}},
            {list_widget,{0,h.race_rows.handle}}},false,144.f,16.f,62.f*static_cast<float>(feed.rows.size()),1200.f);
        mount_hud(c,h.results,list_widget,h.results_anchor,h.results_item,mode_hud_id+5,true);
        c.set(c.field(h.results_anchor,0x1cff7243),std::array<float,2>{1200.f,920.f});
        for(const auto field:{0x99bbed5aU,0x49943bedU})c.set(c.path(h.results_anchor,{0x185a5736,field}),.5f);
        return;
    }
    if(!h.results_rows.handle)h.results_rows=record_model(c,"TD_Scoreboard_ContentResources/LEGACY_TD_Scoreboard");
    c.set(c.field(h.results_rows,0x5a7cec85),true);c.set(c.field(h.results_rows,0xb7e92e95),true);
    plain_label(c,c.element(c.field(h.results_rows,0x3d98d397),2),"RESULT");
    std::vector<Ref> refs;
    for(unsigned i=0;v.state && i<v.state->standings.size() && i<feed.rows.size() && i<6;++i) {
        const auto id=v.state->standings[i].player;auto& row=h.result_players[id];
        if(!row.row.handle) {
            row.row=record_model(c,i==0?"TD_Scoreboard_ContentResources/TD_WinnerProfileInline":"TD_Scoreboard_ContentResources/TD_PlayerProfileInline");
            row.rank=record_model(c,i==0?"TD_Scoreboard_ContentResources/TD_WinnerRank":"TD_Scoreboard_ContentResources/TD_DefaultRank");
            row.outcome=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
            auto rank_widget=read<Widget>(c.address(c.field(row.row,0xc76d8b73)));rank_widget.data={0,row.rank.handle};c.set(c.field(row.row,0xc76d8b73),rank_widget);
            const auto scores=c.field(row.row,0xa3d2bba9);
            c.array(c.field(scores,0xf2c90867),std::vector<Handle>{});c.array(c.field(scores,rows_field),std::vector<Ref>{{0,row.outcome.handle}});
        }
        c.set(c.field(row.row,0x8810a01e),id==v.local);
        plain_label(c,c.path(row.row,{0xaed127c3,profile_name_tag_field}),feed.rows[i].name);
        plain_label(c,c.field(row.rank,0x2718c842),std::to_string(i+1));plain_label(c,row.outcome,feed.rows[i].value);
        refs.push_back({0,row.row.handle});
    }
    const auto players=c.field(h.results_rows,0xfcdd0d32);
    c.array(c.field(players,0xf2c90867),std::vector<Handle>{});c.array(c.field(players,rows_field),refs);
    hud_list(c,h.results,{{asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,h.results_title.handle}},
        {asset(c,"UI/Foundations/Components/Notices/BasicNotice_Widget"),{0,h.results_notice.handle}},
        {asset(c,"UI/Features/CommunityEvent/LEGACY_Scoreboard_Widget"),{0,h.results_rows.handle}}},false,144.f,16.f,140.f*static_cast<float>(refs.size())+100.f,1200.f);
    mount_hud(c,h.results,asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),h.results_anchor,h.results_item,mode_hud_id+5,true);
    c.set(c.field(h.results_anchor,0x1cff7243),std::array<float,2>{1200.f,920.f});
    for(const auto field:{0x99bbed5aU,0x49943bedU})c.set(c.path(h.results_anchor,{0x185a5736,field}),.5f);
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
    if(feed.placing && placement.cursor_valid)mark(feed.cursor,feed.route?"PLACE: "+std::string(placement.route_object==0?"START FLAG":placement.route_object==2?"RED FINISH":"CHECKPOINT +2s"):"PLACE HERE");
    for(const auto& player:feed.players)if(!player.self && player.it)mark(player.at,player.name+(feed.infection?"  INFECTED":"  IT"));
    for(const auto& tag:feed.tags)if(!tag.path.empty())mark(tag.path[tag.path.size()/2],"TAGGED");
}
void mode_hud(const Context& c) {
    auto& s=state();auto& h=s.mode_hud;const auto v=modes::native_view();const auto feed=modes::hud();
    const auto* d=v.active?throwdowns::registry().mode(static_cast<unsigned>(v.settings.mode)):nullptr;
    // The native creation page and waiting notification own setup. Gameplay
    // widgets must not mount into a retiring HUD while the phone is opening.
    if(!d || (!v.placing && (!v.state || v.state->phase==modes::Phase::setup))) {
        mode_hud_publish(c,false);return;
    }
    const auto label_widget=asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget");
    const auto list_widget=asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget");
    const bool race=v.settings.mode==modes::Mode::race;
    if(!h.title.handle) {
        h.title=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");label_styles(c,h.title,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");
        h.status=make(c,label_schema);c.copy(h.status,c.address(h.title));
        h.score=record_model(c,"Activities_HudElementData_ContentResource/Activity_HUD_TargetScore_ContentCombination");
        h.target=clone_widget_model(c,c.field(h.score,0xa98391bd));h.current=clone_widget_model(c,c.field(h.score,0x759c54dd));
        h.clock=record_model(c,"Activities_HudElementData_ContentResource/A_ActivityCountdown");
    }
    plain_label(c,h.title,race?"DEATHRACE":d->name);
    label_styles(c,h.title,race?"TextStylesList/D94-Caps-Bold_White":"TextStylesList/H52-XBold_White",
                 race?"TextStylesList/D94-Caps-Bold_White":"TextStylesList/H52-XBold_White");
    plain_label(c,h.status,feed.placing && race?(v.route_object==2?"Red finish":"Checkpoint +2s"):feed.placing?feed.hint:race?"":feed.status);
    label_styles(c,h.status,race?"TextStylesList/SP38-SemiB_White":"TextStylesList/H52-XBold_White",
                 race?"TextStylesList/SP38-SemiB_White":"TextStylesList/H52-XBold_White");
    if(race)for(const auto value:{h.title,h.status}) {
        c.set(c.field(value,0xc13ba637),false);c.set(c.field(value,0xfc53d427),false);
        c.set(c.field(value,0x87ad624c),2);
    }
    h.placing_layout=feed.placing && race;
    if(feed.placing && race) {
        // LabelButton resolves the actual game's device glyphs and text style.
        // Each action gets its own bounded row instead of a concatenated line.
        constexpr std::array actions{
            std::pair{"UI_TabSecondaryNavLeft",""},
            std::pair{"UI_TabSecondaryNavRight","Checkpoint / Finish"},
            std::pair{"UI_TabPrimaryNavLeft",""},
            std::pair{"UI_TabPrimaryNavRight","Camera down / up"},
            std::pair{"UI_PrimaryAction","Place gate"},
            std::pair{"UI_SecondaryAction","Undo last gate"},
            std::pair{"UI_DpadLeft",""},
            std::pair{"UI_DpadRight","Rotate"},
            std::pair{"UI_DpadUp",""},
            std::pair{"UI_DpadDown","Size"},
            std::pair{"UI_Activities_AccessPause","Save route"},
            std::pair{"UI_Back","Close editor"}};
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
                c.set(c.field(interact,0x38f7355f),true);
                c.set(c.field(interact,0x4b3146e9),false);
                label_styles(c,c.field(candidate,0x58c9d354),"TextStylesList/SP48-SemiB_White_DropShadow75","TextStylesList/SP48-SemiB_White_DropShadow75");
                plain_label(c,c.field(candidate,0x58c9d354),"");
                const auto prompt_input=asset(c,(std::string("Configurations/Input/UI/")+actions[i].first).c_str());
                c.set(c.field(action,0x3d00de27),prompt_input);
                c.set(c.field(action,0x0169c2db),prompt_input);
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
                if(read<Address>(c.address(c.field(action,key)))!=prompt_input)c.set(c.field(action,key),prompt_input);
            prompts.push_back({asset(c,"UI/ReSkate/Throwdowns/EditorGlyph_Widget"),{0,prompt.handle}});
        }
        std::vector<Widget> lines;
        for(unsigned i=0;i<prompts.size();++i) {
            const int pair=i==0?0:i==2?1:i==6?2:i==8?3:-1;
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
                c.set(c.field(label,0xc13ba637),false);c.set(c.field(label,0xfc53d427),false);
                c.set(c.field(label,0x87ad624c),2);c.set(c.field(label,0x3d8639d0),0);
            }
            plain_label(c,label,caption);
            auto& line=h.placement_lines[line_index];
            if(!h.placement_size.handle) {
                h.placement_size=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
                label_styles(c,h.placement_size,"TextStylesList/SP48-SemiB_White_DropShadow75","TextStylesList/SP48-SemiB_White_DropShadow75");
                c.set(c.field(h.placement_size,0xc13ba637),false);c.set(c.field(h.placement_size,0xfc53d427),false);
                c.set(c.field(h.placement_size,0x87ad624c),2);c.set(c.field(h.placement_size,0x3d8639d0),0);
            }
            plain_label(c,h.placement_size,std::to_string(static_cast<int>(std::lround(v.cursor_width/6.f*100.f)))+"%");
            if(!line.handle || created) {
                const auto text=Widget{asset(c,"UI/ReSkate/Throwdowns/EditorCaption_Widget"),{0,label.handle}};
                std::vector<Widget> cells{icons,text};
                if(pair==3)cells.insert(cells.begin(),{asset(c,"UI/ReSkate/Throwdowns/EditorPercent_Widget"),{0,h.placement_size.handle}});
                hud_list(c,line,cells,true,56.f,12.f,0.f,820.f,pair==3?180.f:pair>=0?120.f:56.f);
                const auto refs=c.field(line,0xf2c90867);
                for(unsigned cell=0;cell<cells.size();++cell) {
                    const auto handle=read<Handle>(c.address(c.element(refs,cell)));
                    const float width=cell+1==cells.size()?500.f:pair==3 && cell==1?120.f:cell==0 && pair==3?180.f:pair>=0?120.f:56.f;
                    c.set(c.field({handle,c.type_of(handle)},0x1cff7243),std::array<float,2>{width,56.f});
                }
            }
            lines.push_back({list_widget,{0,line.handle}});
        }
        if(created || !hud_list_has_items(c,h.placement_list,static_cast<unsigned>(lines.size())))
            hud_list(c,h.placement_list,lines,false,56.f,6.f,0.f,720.f);
        mount_hud(c,h.placement_list,list_widget,h.placement_anchor,h.placement_item,mode_hud_id+7,false);
        c.set(c.field(h.placement_anchor,0x1cff7243),std::array<float,2>{720.f,490.f});
        for(const auto field:{0x99bbed5aU,0x49943bedU}) {
            c.set(c.path(h.placement_anchor,{0xde7d30c7,field}),.04f);
            c.set(c.path(h.placement_anchor,{0x185a5736,field}),.25f);
        }
        c.set(c.path(h.placement_anchor,{0xde7d30c7,0xfec5e0b1}),0.f);
        c.set(c.path(h.placement_anchor,{0x185a5736,0xfec5e0b1}),0.f);
        for(const auto axis:{0xde7d30c7U,0x185a5736U})for(const auto field:{0x40bfbff4U,0x1d233266U})
            c.set(c.path(h.placement_anchor,{axis,field}),0.f);
    }
    if(!feed.blind.empty())plain_label(c,h.status,"Hiding time: "+feed.blind);
    else if(feed.heat>=0)plain_label(c,h.status,feed.status+"  |  "+feed.heat_label);
    std::vector<Widget> rows;
    for(unsigned i=0;!race && i<feed.rows.size() && i<6;++i) {
        auto& label=h.labels[i];if(!label.handle) {label=make(c,label_schema);c.copy(label,c.address(h.title));label_styles(c,label,"TextStylesList/H52-XBold_White","TextStylesList/H52-XBold_White");}
        plain_label(c,label,(feed.rows[i].up?"> ":"")+feed.rows[i].name+"   "+feed.rows[i].value+(feed.rows[i].out?"  OUT":""));
        rows.push_back({label_widget,{0,label.handle}});
    }
    if(h.row_count!=rows.size() || h.race_layout!=race || !hud_list_has_items(c,h.rows,static_cast<unsigned>(rows.size()))) {
        h.race_layout=race;
        hud_list(c,h.rows,rows,false,54.f,6.f,0.f,680.f);h.row_count=static_cast<unsigned>(rows.size());
        std::vector<Widget> content_rows;
        if(race) {
            if(!h.badge.handle) {
                h.badge=make(c,texture_schema);
                c.set(c.field(h.badge,0xf524c836),96.f);c.set(c.field(h.badge,0x6fbd254e),96.f);
                c.set(c.field(h.badge,0xd1997d0c),false);
                c.set(c.field(h.badge,0xcd4a53c0),record_ref(c,"InteractivityColorStylesList/InteractiveColors.Core.AllWhite"));
            }
            // A stable native texture presenter gives the badge its own
            // measured frame, independent of the header list's label layout.
            content_rows.push_back({label_widget,{0,h.title.handle}});
            content_rows.push_back({label_widget,{0,h.status.handle}});
        } else content_rows={{label_widget,{0,h.title.handle}},{label_widget,{0,h.status.handle}}};
        if(!race) {
            content_rows.push_back({asset(c,"UI/Foundations/Components/Modals/ContentPresenterCombination_Widget"),{0,h.score.handle}});
            content_rows.push_back({list_widget,{0,h.rows.handle}});
        }
        hud_list(c,h.content,content_rows,false,race?100.f:62.f,8.f,race?48.f:60.f*static_cast<float>(rows.size()),680.f);
    }
    if(race) {
        mode_art(c,std::array{c.field(h.badge,0x6e469d2a)},d->logo);
        mount_hud(c,h.badge,asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),h.badge_anchor,h.badge_item,mode_hud_id+40,false);
        c.set(c.field(h.badge_anchor,0x1cff7243),std::array<float,2>{96.f,96.f});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const auto layout=c.field(h.badge_anchor,axis);
            for(const auto field:{0x99bbed5aU,0x49943bedU})c.set(c.field(layout,field),axis==0xde7d30c7U?.04f:.14f);
            for(const auto field:{0x40bfbff4U,0x1d233266U})c.set(c.field(layout,field),0.f);
            c.set(c.field(layout,0xfec5e0b1),0.f);
        }
    }
    const auto state=v.state;std::int32_t best{},mine{};
    if(state)for(const auto& standing:state->standings) {best=std::max(best,standing.score);if(standing.player==v.local)mine=standing.score;}
    plain_label(c,c.field(h.target,0x58c9d354),std::to_string(best));plain_label(c,c.field(h.current,0x94703efc),std::to_string(mine));
    mount_hud(c,h.content,list_widget,h.anchor,h.item,mode_hud_id,false);
    if(h.stack.handle!=s.hud_stack.handle) {mode_hud_publish(c,false);h.stack=s.hud_stack;}
    c.set(c.field(h.anchor,0x1cff7243),std::array<float,2>{720.f,race?168.f:240.f+60.f*static_cast<float>(rows.size())});
    // This anchor is reused after the editor closes. Reconcile both layouts
    // so another mode cannot inherit the editor's compact header position.
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const bool vertical=axis==0x185a5736U;
        const float at=race?(vertical?.14f:.04f):(vertical?.12f:0.f);
        const float offset=!vertical && race?110.f:0.f;
        for(const auto field:{0x99bbed5aU,0x49943bedU})c.set(c.path(h.anchor,{axis,field}),at);
        for(const auto field:{0x40bfbff4U,0x1d233266U})c.set(c.path(h.anchor,{axis,field}),offset);
        c.set(c.path(h.anchor,{axis,0xfec5e0b1}),0.f);
    }
    mount_hud(c,h.clock,asset(c,"UI/Foundations/Components/Countdown/Countdown_Widget"),h.clock_anchor,h.clock_item,mode_hud_id+1,false);
    c.set(c.field(h.clock_anchor,0x1cff7243),std::array<float,2>{320.f,90.f});
    const auto vertical=c.field(h.clock_anchor,0x185a5736);
    c.set(c.field(vertical,0x99bbed5a),race?.9f:.83f);c.set(c.field(vertical,0x49943bed),race?.9f:.83f);
    const auto horizontal=c.field(h.clock_anchor,0xde7d30c7);
    for(const auto key:{0x99bbed5aU,0x49943bedU})c.set(c.field(horizontal,key),race?.5f:0.f);
    c.set(c.field(horizontal,0xfec5e0b1),race?.5f:0.f);
    for(const auto key:{0x40bfbff4U,0x1d233266U})c.set(c.field(horizontal,key),race?0.f:110.f);
    c.set(c.field(h.clock,0x703c4d93),false);c.set(c.field(h.clock,0x167131bb),true);c.set(c.field(h.clock,0x6186003c),true);c.set(c.field(h.clock,0xc11082ee),true);
    native_timer_seconds(c,h.clock,0x4cc5ec89,static_cast<std::uint64_t>(v.settings.duration_s)*1000);
    auto remaining=state?state->remaining_ms:0;
    if(race && state && state->phase==modes::Phase::playing) {
        const auto me=std::find_if(state->standings.begin(),state->standings.end(),[&](const auto& row){return row.player==v.local;});
        remaining=me!=state->standings.end()?me->remaining_ms:0;
    }
    native_timer_seconds(c,h.clock,0x0ec4a8c0,remaining);
    const bool counting=state && state->phase==modes::Phase::countdown;
    const bool intro=counting && state->remaining_ms>3000;
    const auto now=GetTickCount64();if(v.game!=h.game) {h.game=v.game;h.go_until=0;}
    if(counting && !intro)h.go_until=now+900;
    const bool go=state && state->phase==modes::Phase::playing && now<h.go_until;
    const bool blind=!feed.blind.empty();
    if(blind) {
        mount_hud(c,{},asset(c,"UI/ReSkate/Throwdowns/SeekerBlackout_Widget"),h.blind_anchor,h.blind_item,mode_hud_id+4,true);
        c.set(c.field(h.blind_anchor,0x1cff7243),std::array<float,2>{1920.f,1080.f});
        for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
            const auto layout=c.field(h.blind_anchor,axis);
            c.set(c.field(layout,0x99bbed5a),0.f);c.set(c.field(layout,0x49943bed),1.f);
            c.set(c.field(layout,0x40bfbff4),0.f);c.set(c.field(layout,0x1d233266),0.f);
            c.set(c.field(layout,0xfec5e0b1),.5f);
        }
    }
    if(intro) {
        if(!h.intro.handle) {
            h.intro=record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner");
            c.set(c.field(h.intro,0xf79313be),record_ref(c,"ActivityBannerStylesList/ActivityBannerCenterFullWidthStyle.Throwdown.Default"));
            c.copy(c.field(h.intro,0xcbc57608),c.address(record_model(c,"TD_Hud_ContentResources/TD_ActivityBanner_ContentWithTitle")));
            h.intro_badge=make(c,texture_schema);c.set(c.field(h.intro_badge,0xf524c836),256.f);c.set(c.field(h.intro_badge,0x6fbd254e),256.f);
            c.set(c.path(h.intro,{0xcbc57608,content}),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,h.intro_badge.handle}});
        }
        plain_label(c,c.path(h.intro,{0xcbc57608,0x44688629}),d->name);plain_label(c,c.path(h.intro,{0xcbc57608,0x00f3b15e}),d->description);
        mode_art(c,std::array{c.field(h.intro_badge,0x6e469d2a)},d->logo);
        mount_hud(c,h.intro,asset(c,"UI/Features/Activities/Blueprints/ActivityBanner_CenterFullWidth_Widget"),h.intro_anchor,h.intro_item,mode_hud_id+3,true);
        c.set(c.field(h.intro_anchor,0x1cff7243),std::array<float,2>{1920.f,600.f});
    }
    if((counting && !intro) || go) {
        if(!h.countdown.handle)h.countdown=record_model(c,"Activities_HudElementData_ContentResource/Activities_CountdownItemHUD");
        plain_label(c,c.field(h.countdown,0x58c9d354),go?"GO!":std::to_string(native_hud_seconds(state->remaining_ms)));
        c.set(c.field(h.countdown,1698918111U),go?0.f:std::clamp(static_cast<float>(state->remaining_ms)/3000.f,0.f,1.f));
        mount_hud(c,h.countdown,asset(c,"UI/Features/Activities/Widgets/System/ActivityCountdown_Widget"),h.countdown_anchor,h.countdown_item,mode_hud_id+2,true);
        c.set(c.field(h.countdown_anchor,0x1cff7243),std::array<float,2>{512.f,512.f});
    }
    mode_world_markers(c,feed);
    if(race && state && !feed.results && !feed.placing)mode_race_standings(c,v,feed);
    if(feed.results)mode_results(c,v,feed,*d);
    mode_hud_publish(c,!feed.results,(counting && !intro)||go,intro,blind,feed.results,race && state && !feed.results && !feed.placing);
}
