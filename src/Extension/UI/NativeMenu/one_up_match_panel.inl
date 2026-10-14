// Private instances of the game's Throwdown details page, panel and scoreboard.
// The pause state still owns the phone pose, input context, Back and teardown.
struct MatchPanelRow {
    std::uint64_t id{};
    std::string name,value;
};
struct MatchPanelView {
    bool available{};
    std::uint64_t local{};
    std::string title,description,logo,timer_caption,timer,score_caption;
    std::string open_command,quit_command,argument;
    std::vector<MatchPanelRow> players;
};
MatchPanelView match_panel_view() {
    MatchPanelView result;
    const auto oneup=one_up::view();
    if(oneup.state.match && oneup.participant && oneup.state.phase!=one_up::Phase::lobby &&
       oneup.state.phase!=one_up::Phase::cancelled && oneup.state.phase!=one_up::Phase::finished) {
        result.available=true;result.local=oneup.local;
        result.title="1-UP";result.description="Beat the previous turn total. Three penalties and you're out!";
        result.logo="UI/ReSkate/OneUp/img_OneUp_Blue_1024";
        result.timer_caption="Turn Timer";result.timer=std::to_string(oneup.state.config.turn_ms/1000)+"s";
        result.score_caption="1-UP";result.open_command="throwdown-panel";result.quit_command="panel-quit";
        result.argument=std::to_string(oneup.state.match);
        for(const auto& player:oneup.state.players)
            result.players.push_back({player.id,oneup.name(player.id),!player.connected?"LEFT":
                !player.eligible()?"OUT":std::string("1UP").substr(0,player.penalties)});
        return result;
    }
    const auto mode=modes::native_view();
    if(!mode.active || mode.placing || !mode.state || mode.state->phase==modes::Phase::setup || mode.state->phase==modes::Phase::results ||
       std::none_of(mode.state->standings.begin(),mode.state->standings.end(),
                    [&](const modes::Standing& row){return row.player==mode.local;}))return result;
    const auto definition=throwdowns::registry().mode(static_cast<unsigned>(mode.settings.mode));
    if(!definition)return result;
    const auto feed=modes::hud();
    result.available=true;result.local=mode.local;result.title=definition->name;
    result.description=definition->description;result.logo=definition->logo;
    result.timer_caption=definition->supports_turns?"Turn Timer":"Timer";
    result.timer=std::to_string(definition->supports_turns?mode.settings.turn_s:mode.settings.duration_s)+"s";
    result.score_caption=mode.settings.mode==modes::Mode::race?"GATES / TIME":"SCORE";
    result.open_command="mode-throwdown-panel";result.quit_command="mode-panel-quit";
    result.argument=std::to_string(mode.game);
    for(unsigned i=0;i<mode.state->standings.size();++i) {
        const auto& standing=mode.state->standings[i];
        const auto name=std::find_if(mode.names.begin(),mode.names.end(),
                                    [&](const auto& player){return player.first==standing.player;});
        result.players.push_back({standing.player,i<feed.rows.size()?feed.rows[i].name:
            name!=mode.names.end()?name->second:std::to_string(standing.player),
            i<feed.rows.size()?feed.rows[i].value:std::to_string(standing.score)});
    }
    return result;
}
// The activity flow owns the phone pose and input context. Its menu lives on
// Layer_Menu, not on the separate Core pause-menu tab stack.
struct MatchPanelLease {
    Address manager{};
    Handle generation_page{};
    ActivityDetailsOwnership details;
    Value selection;
    Address page_blueprint{};
    Value shortcut_stack,saved_shortcut,shortcut_content,highlight;
    std::string back_navigation;
    std::string wait_reason;
    std::uint64_t wait_logged{};
};
MatchPanelLease& match_panel_lease(const Context& c) {
    static MatchPanelLease lease;
    if(lease.manager!=c.manager || (lease.generation_page && lease.generation_page!=state().match_panel.page.handle))
        lease={c.manager};
    return lease;
}
void match_panel_wait(const Context& c,std::uint64_t now,const char* reason) {
    if(!state().match_panel.requested_until)return;
    auto& lease=match_panel_lease(c);
    if(lease.wait_reason==reason && now<lease.wait_logged+2000)return;
    lease.wait_reason=reason;lease.wait_logged=now;
    logging::log(logging::Level::warning,logging::Channel::ui,"Throwdowns: requested native details waiting for {}.",reason);
}
bool same_native_model_schema(Address authored,Address runtime,unsigned depth=0) {
    if(authored==runtime)return true;
    if(!authored || !runtime || depth>16 || kind(authored)!=kind(runtime) || size(authored)!=size(runtime))return false;
    const auto a=read<Address>(authored),b=read<Address>(runtime);
    if(read<std::uint32_t>(a)!=read<std::uint32_t>(b))return false;
    if(kind(authored)==4)return same_native_model_schema(read<Address>(a+0x30),read<Address>(b+0x30),depth+1);
    if(kind(authored)!=2)return true;
    const auto count=read<std::uint16_t>(a+0x2a);
    if(count>128 || count!=read<std::uint16_t>(b+0x2a))return false;
    const auto af=read<Address>(a+0x60),bf=read<Address>(b+0x60);
    if(count && (!af || !bf))return false;
    for(unsigned i=0;i<count;++i) {
        const auto x=af+i*24,y=bf+i*24;
        if(read<std::uint32_t>(x)!=read<std::uint32_t>(y) || read<std::uint16_t>(x+8)!=read<std::uint16_t>(y+8) ||
           !same_native_model_schema(read<Address>(x+16),read<Address>(y+16),depth+1))return false;
    }
    return true;
}
Value native_record_model(const Context& c,Address record,std::uint32_t expected_hash) {
    require(record && read<Address>(record+8)==c.base+build::native_menu::asset_record_type,
            "Native activity model record differs.");
    const auto type=read<Address>(record+0x18);
    require(type && kind(type)==2 && read<std::uint32_t>(read<Address>(type))==expected_hash,
            "Native activity model TypeInfo differs.");
    // Resolve in the same manager whose write lock and registry Context owns.
    // The expression wrapper at 0x192ca80 chooses a manager through TLS; its
    // handle can belong to another realm and cannot be read in this registry.
    // Local-profile initialization validates this ABI before installing its
    // record guard. The live entry now contains that detour, so comparing it
    // with the original prologue rejects our own hook. Keep the guard in this
    // call path and use its validated original trampoline.
    require(model_record_load_original!=nullptr,"Native activity record resolver is not initialized.");
    const auto handle=model_record_load_hook(c.manager,record,false);
    if(!handle)return {};
    const auto actual_type=c.type_of(handle);
    if(!same_native_model_schema(type,actual_type)) {
        logging::log(logging::Level::error,logging::Channel::ui,
            "Native activity record {:#x} manager {:#x} returned handle {:#x}: expected TypeInfo {:#x} hash {:#x}, actual {:#x} hash {:#x}.",
            record,c.manager,handle,type,expected_hash,actual_type,
            actual_type?read<std::uint32_t>(read<Address>(actual_type)):0);
        require(false,"Native activity model handle differs from its authored TypeInfo.");
    }
    // The native registry may canonicalize an authored dynamic TypeInfo.
    // Accept only identical reflected schemas and retain the registry's type.
    const Value result{handle,actual_type};c.address(result);return result;
}
Value activity_menu_selection(const Context& c) {
    const auto model=native_record_model(c,asset(c,"Activities_Shared_ContentResource/Activities_SelectedMenu"),0xaf4c67d7);
    return model.handle?c.field(model,0x88391637):Value{};
}
bool request_activity_details(const Context& c) {
    const auto selected=activity_menu_selection(c);
    if(!selected.handle)return false;
    const auto current=read<std::int32_t>(c.address(selected));
    // Rewards, celebrations and other native activity pages keep their owner.
    if(current!=0 && current!=1)return false;
    auto& lease=match_panel_lease(c);lease.selection=selected;lease.details.request();
    if(!current) {
        c.set(selected,std::int32_t{1}); // ActivityMenus.Activities_Details.
        logging::write(logging::Level::info,logging::Channel::ui,"Throwdowns: native activity menu selected Activities_Details.");
    }
    return true;
}
bool request_native_navigation(const Context& c,const char* action) {
    const auto roots=c.roots({0xcc4776b5});
    if(roots.size()!=1)return false;
    const auto model=roots.front().model;
    require(model.type==c.type({0xcc4776b5,static_cast<std::uint16_t>(size(model.type))}),
            "Native menu navigation TypeInfo differs.");
    const auto navigation=c.field(roots.front().model,0xf3ba9cf8);
    if(read<std::uint32_t>(c.address(navigation)))return false;
    c.array(c.field(roots.front().model,0xf437b255),std::vector<std::int32_t>{-1});
    c.set(navigation,game::native_name_hash(action));
    return true;
}
Value active_match_panel_layer_item(const Context& c,Value layer) {
    const auto list=c.field(layer,items);
    unsigned count{},stride{};c.array(list,32,count,stride);
    if(stride!=stack_item.size)return {};
    const auto selected=active_activity_stack_item(
        read<int>(c.address(c.field(layer,0x49f87835))),count,
        read<bool>(c.address(c.field(layer,0x369babfe))));
    if(!selected)return {};
    const auto item=c.element(list,*selected);
    return read<bool>(c.address(c.field(item,0x369babfe)))?item:Value{};
}
bool owns_selected_match_panel(const Context& c) {
    const auto& lease=match_panel_lease(c);const auto& p=state().match_panel;
    if(!lease.details.requested || !lease.details.page || !lease.details.layer || !p.mounted_slot.handle ||
       c.type_of(p.mounted_slot.handle)!=p.mounted_slot.type)return false;
    const auto layer=native_record_model(c,asset(c,"UI_Layer_Data/Layer_Menu"),0xd71ee0a1);
    if(layer.handle!=lease.details.layer)return false;
    const auto item=active_match_panel_layer_item(c,layer);
    if(!item.handle)return false;
    const auto current=read<Widget>(c.address(c.field(item,content)));
    return current.blueprint==lease.page_blueprint &&
        lease.details.owns(current.data.handle,layer.handle,read<int>(c.address(c.field(item,0x1e95752c))));
}
bool request_menu_toggle(const Context& c) {
    const auto& lease=match_panel_lease(c);
    if(lease.details.requested)
        return owns_selected_match_panel(c) && !lease.back_navigation.empty() &&
            request_native_navigation(c,lease.back_navigation.c_str());
    return request_native_navigation(c,"ToggleMenu");
}
void detach_match_panel_prompt(const Context& c) {
    state().match_panel.prompt_visible=false;
    auto& lease=match_panel_lease(c);
    if(!lease.shortcut_stack.handle || !lease.saved_shortcut.handle)return;
    if(c.type_of(lease.shortcut_stack.handle)!=lease.shortcut_stack.type) {
        lease.shortcut_stack={};return;
    }
    const auto list=c.field(lease.shortcut_stack,items);
    unsigned count{},stride{};const auto old=c.array(list,32,count,stride);
    require(stride==stack_item.size,"Native Throwdown shortcut stack differs during teardown.");
    const auto type=c.type(stack_item);
    const auto id_offset=c.member(type,0x1e95752c).offset;
    const auto content_offset=c.member(type,content).offset;
    std::vector<std::byte> retained;
    unsigned removed{};
    for(unsigned i=0;i<count;++i) {
        int id{};Widget display{};
        std::memcpy(&id,old.data()+i*stride+id_offset,sizeof(id));
        std::memcpy(&display,old.data()+i*stride+content_offset,sizeof(display));
        if(id==hud_panel_id && display.data.handle==lease.shortcut_content.handle) {++removed;continue;}
        retained.insert(retained.end(),old.begin()+i*stride,old.begin()+(i+1)*stride);
    }
    if(removed) {
        if(removed==count) {
            // Installation only borrows an empty authored shortcut stack.
            // Restore it once while its sole mounted item is still ours.
            c.copy(lease.shortcut_stack,c.address(lease.saved_shortcut));
        } else {
            // Native UI has since mounted another shortcut. Remove only our
            // item; leave that newer owner's active/focus/transition fields.
            // Context::array retains the source snapshot in a transient typed
            // owner before changing the destination's counted native array.
            c.array(list,retained,static_cast<unsigned>(retained.size()/stride));
        }
    }
    lease.shortcut_stack={};
}
void restore_match_panel_mount(const Context& c) {
    auto& s=state();auto& p=s.match_panel;
    if(p.mounted_slot.handle && p.saved_slot.handle && c.type_of(p.mounted_slot.handle)==p.mounted_slot.type &&
       game::native_data().models.value(c.manager,p.mounted_slot.handle,0,1)) {
        const auto mounted=read<Widget>(c.address(p.mounted_slot));
        // A tab change or native page retirement already released our mount.
        // Never overwrite that newer content with a saved, unrelated page.
        if(mounted.data.handle==p.page.handle)c.copy(p.mounted_slot,c.address(p.saved_slot));
    }
    p.mounted_slot={};s.match_panel_open.store(false,std::memory_order_release);
    if(s.in_menu || p.quitting || !match_panel_view().available)detach_match_panel_prompt(c);
    auto& lease=match_panel_lease(c);lease.details.clear_mount();lease.back_navigation.clear();lease.page_blueprint=0;
}
void restore_match_panel(const Context& c) {
    restore_match_panel_mount(c);
    auto& lease=match_panel_lease(c);lease.details.retire();lease.selection={};
    auto& p=state().match_panel;p.requested_until=0;p.request_after=0;
}
bool pending_match_panel_request(const Context&,std::uint64_t now) {
    const auto& p=state().match_panel;
    return p.requested_until && now<p.requested_until && (state().in_menu || p.request_after);
}
Value panel_stack_item(const Context& c,Value stack,Widget display,int id) {
    const auto item=make(c,stack_item);
    c.set(c.field(item,content),display);c.set(c.field(item,0x1e95752c),id);
    c.copy(c.field(item,0xd78d240b),c.address(c.field(stack,0x54c9abee)));
    c.set(c.field(item,0x369babfe),true);
    for(const auto track:{0x8c0583eeU,0xc5c7cc11U}) {
        c.set(c.path(item,{track,0x20247753}),1.f);
        c.set(c.path(item,{track,0x2426103f}),1.f);
        c.set(c.path(item,{track,0x1e3ce013}),true);
    }
    return item;
}
void panel_stack_content(const Context& c,Value stack,Widget display,int id) {
    const auto item=panel_stack_item(c,stack,display,id);
    std::vector<std::byte> bytes(stack_item.size);
    require(memory::peek_bytes(c.address(item),bytes.data(),bytes.size()),"Native Throwdown Panel content unavailable.");
    c.array(c.field(stack,items),bytes,1);
    c.set(c.field(stack,0x369babfe),true);
    c.set(c.field(stack,0xc912aa1d),false);
    c.set(c.field(stack,0x2a98baa3),false);
    c.set(c.field(stack,0x18f8355b),0);
    c.set(c.field(stack,0x55511280),0);
}
bool mount_match_panel_prompt(const Context& c) {
    auto& s=state();auto& p=s.match_panel;auto& lease=match_panel_lease(c);
    if(s.in_menu) {detach_match_panel_prompt(c);return false;}
    // The authored Activity HUD renders ActivityShortcutButtonStack through
    // ActivityPauseButton_Stack. FreeRoamHUD, which hosts our custom matches,
    // omits that renderer. Mount the same native StackScreen/control privately
    // on the visible HUD rather than writing into an unrendered native stack.
    require(s.hud_root.handle && c.type_of(s.hud_root.handle)==s.hud_root.type,
            "Native gameplay shortcut HUD is not mounted.");
    const auto parent=c.field(s.hud_root,0x0bcb2ef8);
    if(lease.shortcut_stack.handle)detach_match_panel_prompt(c);
    if(!lease.shortcut_content.handle) {
        lease.shortcut_content=make(c,{0xd71ee0a1,80},parent.type);
        panel_stack_content(c,lease.shortcut_content,
            {asset(c,"UI/Foundations/Components/Buttons/LabelButton/LabelButton_Widget"),{0,p.prompt.handle}},hud_panel_id+4);
        c.set(c.field(lease.shortcut_content,0x18f8355b),-1);
        c.set(c.field(lease.shortcut_content,0x1fbbb5da),true);
        c.set(c.field(lease.shortcut_content,0x3b14d43c),true);
        c.set(c.field(lease.shortcut_content,0xc912aa1d),true);
        c.set(c.field(lease.shortcut_content,0xd9fc931c),0.f);
        c.set(c.field(lease.shortcut_content,0x2a98baa3),true);
    }
    const bool fresh=!p.prompt_item.handle;
    mount_hud(c,lease.shortcut_content,asset(c,"UI/Foundations/Components/Stack/StackScreen"),
        p.prompt_anchor,p.prompt_item,hud_panel_id,false);
    c.set(c.field(p.prompt_anchor,0x1cff7243),std::array<float,2>{440.f,80.f});
    for(const auto axis:{0xde7d30c7U,0x185a5736U}) {
        const float at=axis==0xde7d30c7?.95f:.12f;
        for(const auto field:{0x99bbed5aU,0x49943bedU})c.set(c.path(p.prompt_anchor,{axis,field}),at);
        for(const auto field:{0x40bfbff4U,0x1d233266U})c.set(c.path(p.prompt_anchor,{axis,field}),0.f);
        c.set(c.path(p.prompt_anchor,{axis,0xfec5e0b1}),axis==0xde7d30c7?1.f:0.f);
    }
    p.prompt_visible=true;
    if(fresh)logging::write(logging::Level::info,logging::Channel::ui,
        "Throwdowns: native Activity pause control mounted on the visible custom HUD.");
    return true;
}
Value match_panel_input_action(const Context& c,Value button_value) {
    return c.path(button_value,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
}
void match_panel_input(const Context& c,Value button_value,Address input_asset,bool prompt) {
    require(input_asset!=0,"Native Throwdown Panel input unavailable.");
    const auto action=match_panel_input_action(c,button_value);
    // The native glyph and shortcut resolve different input fields. Supplying
    // both keeps the controller glyph and keyboard binding on the same action.
    for(const auto key:{0x3d00de27U,0x0169c2dbU}) {
        const auto input_field=c.field(action,key);
        if(read<Address>(c.address(input_field))!=input_asset)c.set(input_field,input_asset);
    }
    const auto interactable=c.path(button_value,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a});
    const auto is_prompt=c.field(interactable,0x38f7355f);
    if(read<bool>(c.address(is_prompt))!=prompt)c.set(is_prompt,prompt);
    const auto clickable=c.field(interactable,0x4b3146e9);
    if(read<bool>(c.address(clickable))==prompt)c.set(clickable,!prompt);
}
void match_panel_action(const Context& c,Value button_value,const char* caption,const MatchPanelView& view,bool quit) {
    plain_label(c,c.field(button_value,0x58c9d354),caption);
    const auto action=match_panel_input_action(c,button_value);
    const auto delegate=c.field(action,0x9c76b86c);
    const auto target=callback(c,quit?view.quit_command:view.open_command,view.argument);
    // Keep the installed handler stable while held/pressed state is running.
    // Calling callback still retains the descriptor for this UI generation.
    if(read<Address>(c.address(delegate))!=target)c.set(delegate,target);
    const auto navigation=c.field(action,0xb1e14cd6);
    if(!c.text(navigation).empty())c.text(navigation,"");
}
void update_match_panel_quit(const Context& c,Value page_value,Value quit,const MatchPanelView& view) {
    match_panel_action(c,quit,"QUIT",view,true);
    const auto actions=c.field(page_value,0xd2118ca8);
    unsigned count{},stride{};c.array(actions,8,count,stride);
    require(count==1 && stride==page_action_schema.size,"Native Throwdown Panel Quit shortcut differs.");
    const auto mounted=c.path(c.element(actions,0),{0xa704272a,0xc52416ef,0xa704272a,0xa2b71c93});
    const auto source=match_panel_input_action(c,quit);
    const auto page_action=c.element(actions,0);
    const auto bar=c.field(page_action,0xb6e9bfa0); // Native generated ButtonBarLabelButton context handle.
    if(size(bar.type)==sizeof(Handle)) {
        const auto handle=read<Handle>(c.address(bar));
        if(handle && game::native_data().models.value(c.manager,handle,0,1)) {
            const Value hint{handle,c.type_of(handle)};
            if(hint.type==c.type(button))plain_label(c,c.field(hint,0x58c9d354),"QUIT");
        }
    }

    if(read<Address>(c.address(c.field(mounted,0x9c76b86c)))!=read<Address>(c.address(c.field(source,0x9c76b86c))))
        c.copy(mounted,c.address(source));
}
void build_match_panel(const Context& c,const MatchPanelView& view) {
    auto& p=state().match_panel;if(p.page.handle)return;
    const auto checkpoint=state().owned.checkpoint();
    try {
    // Copy the complete mutable input graph once. Backend styles and immutable
    // assets stay borrowed; no stock mode's model is changed by this page.
    const auto page_value=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_Page",page);
    PrivateUiGraph graph{c};graph.walk(page_value,0);
    p.panel=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_StackPanel",panel_schema);
    graph.walk(p.panel,0);
    p.header=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdown_Details_StackPanel_HeaderContent",header_schema);
    graph.walk(p.header,0);
    plain_label(c,c.field(p.header,0x44688629),"Throwdown");
    plain_label(c,c.field(p.header,0x00f3b15e),"1-UP");
    label_styles(c,c.field(p.header,0x44688629),"TextStylesList/B52-SemiB_Bright","TextStylesList/B52-SemiB_Bright");
    label_styles(c,c.field(p.header,0x00f3b15e),"TextStylesList/D180-Caps_White","TextStylesList/H72-XBold_White");
    c.set(c.path(p.header,{0x00f3b15e,0xfc53d427}),false);
    c.set(c.field(p.header,0x2840ecf6),Widget{});
    p.badge=make(c,texture_schema);
    c.set(c.field(p.badge,0xf524c836),256.f);c.set(c.field(p.badge,0x6fbd254e),256.f);
    c.set(c.field(p.header,content),Widget{asset(c,"UI/Foundations/Components/Media/Icons/Texture_Widget"),{0,p.badge.handle}});
    c.set(c.field(p.panel,0xc0f1b449),Widget{asset(c,"UI/Foundations/Components/ContentPresenterVariations/ContentWithTitle_Widget"),{0,p.header.handle}});
    p.notice=record_model(c,"TD_Page_ContentResources/TD_Notice_MenuDescriptionTitle",notice_schema);
    plain_label(c,c.field(p.notice,0x94703efc),"Beat the previous turn total. Three penalties and you're out!");
    c.set(c.field(p.notice,0x13be0d51),std::array<float,2>{1120.f,200.f});
    label_styles(c,c.field(p.notice,0x94703efc),"TextStylesList/B52-SemiB_Bright","TextStylesList/B52-SemiB_Bright");
    c.set(c.field(p.panel,0x4fd7853d),Widget{});
    std::vector<Widget> settings{{asset(c,"UI/Foundations/Components/Notices/BasicNotice_Widget"),{0,p.notice.handle}}};
    for(unsigned i=0;i<p.setting_rows.size();++i) {
        p.setting_rows[i]=make(c,setting_schema);
        c.set(c.field(p.setting_rows[i],0xc9bf367d),record_ref(c,"SettingWrapperStylesList/Horizontal.WithDivider"));
        c.set(c.field(p.setting_rows[i],0xf3d74cdd),0);
        const auto caption=c.field(p.setting_rows[i],0x58c9d354);
        c.copy(caption,c.address(state().source_label));
        plain_label(c,caption,i?"Turn Timer":"Players");
        label_styles(c,caption,"TextStylesList/B52-SemiB_Black","TextStylesList/B52-SemiB_Black");
        p.setting_values[i]=make(c,label_schema);
        c.copy(p.setting_values[i],c.address(state().source_label));
        c.set(c.field(p.setting_values[i],0xc13ba637),false);
        c.set(c.field(p.setting_values[i],0xfc53d427),false);
        label_styles(c,p.setting_values[i],"TextStylesList/B52-SemiB_Black","TextStylesList/B52-SemiB_Black");
        c.set(c.field(p.setting_values[i],0x3d8639d0),1);
        c.set(c.field(p.setting_rows[i],content),Widget{asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,p.setting_values[i].handle}});
        settings.push_back({asset(c,"UI/Foundations/Components/Settings/Widgets/SettingWrapper_Widget"),{0,p.setting_rows[i].handle}});
    }
    hud_list(c,p.settings,settings,false,144.f,16.f,0.f,1120.f);
    const auto rules_anchor=read<Handle>(c.address(c.element(c.field(p.settings,0xf2c90867),0)));
    c.set(c.field({rules_anchor,c.type_of(rules_anchor)},0x1cff7243),std::array<float,2>{1120.f,200.f});
    panel_stack_content(c,c.field(p.panel,0x43967ece),
        {asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,p.settings.handle}},hud_panel_id+1);
    // This authored footer is an empty list; the stock challenge rule supplies
    // its buttons later. Instantiate the separately exported native Quit button
    // instead of looking for a button that is absent from the template graph.
    // Resolve the exported footer list and anchor independently. The panel's
    // inline footer graph contains ContentWithTitle and presenter wrappers;
    // treating its first referenced model as an anchor asks for a field it
    // does not have and aborts construction before a Page can be mounted.
    const auto footer_list=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_StackPanel_FooterContent",linear_list);
    const auto footer_anchor=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_StackPanel_Footer_AnchoredContentPresenter",anchor_schema);
    graph.walk(footer_list,0);graph.walk(footer_anchor,0);
    c.set(c.field(footer_anchor,widget),Widget{asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,footer_list.handle}});
    c.set(c.field(p.panel,0xbfc64535),Widget{asset(c,native_menu_detail::anchored_widget),{0,footer_anchor.handle}});
    p.quit=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_Quit_Button",button);
    graph.walk(p.quit,0);
    clear_value(c,c.field(p.quit,0x67212c85));
    // The donor Quit focus predicate queries its native challenge backend. This
    // private page validates availability through our own match coordinator.
    clear_value(c,c.path(p.quit,{0x0fb0d794,0xa704272a,0xc52416ef,0x66a997b9,0x66a997b9}));
    c.set(c.field(p.quit,0x659db23e),false);
    const auto quit_input=read<Address>(c.address(c.field(match_panel_input_action(c,p.quit),0x0169c2db)));
    match_panel_input(c,p.quit,quit_input,false);
    c.copy(c.field(p.quit,0x58c9d354),c.address(state().source_label));
    label_styles(c,c.field(p.quit,0x58c9d354),"TextStylesList/B52-SemiB-Caps_Dark","TextStylesList/B52-SemiB_Black");
    c.set(c.path(p.quit,{0x58c9d354,0x3d8639d0}),2);
    match_panel_action(c,p.quit,"QUIT",view,true);
    c.array(c.field(footer_list,0xf2c90867),std::vector<Handle>{});
    c.array(c.field(footer_list,rows_field),std::vector<Ref>{{0,p.quit.handle}});
    // A native PageAction keeps Quit available while a leaderboard row has
    // focus, just as the original details page does for its shortcut controls.
    const auto page_actions=c.field(page_value,0xd2118ca8);
    const auto action_type=read<Address>(read<Address>(page_actions.type)+0x30);
    const auto quit_action=make(c,page_action_schema,action_type);
    c.copy(c.field(quit_action,0xa704272a),c.address(c.path(p.quit,{0x0fb0d794,0xa704272a})));
    c.text(c.field(quit_action,0x6c0c58da),"Quit"); // PageAction.FilterKey is required by the button bar.
    c.set(c.field(quit_action,0xb3da8457),true); // CanClick.

    std::vector<std::byte> action_bytes(page_action_schema.size);
    require(memory::peek_bytes(c.address(quit_action),action_bytes.data(),action_bytes.size()),
            "Native Throwdown Panel Quit shortcut unavailable.");
    c.array(page_actions,action_bytes,1);
    p.board=make(c,linear_list);
    p.columns=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_HorizontalPartition",native_menu_detail::horizontal_panel);
    graph.walk(p.columns,0);
    c.set(c.field(p.columns,0xf750f0a2),Widget{asset(c,"UI/Foundations/Components/StackVariations/StackPanel/StackPanel_Widget"),{0,p.panel.handle}});
    c.set(c.field(p.columns,0x29023866),Widget{asset(c,"UI/Foundations/Components/Lists/LinearList/Widget/LinearList_Widget"),{0,p.board.handle}});
    p.outer_anchor=record_model(c,"Throwdown_DetailsMenu_ContentResource/Throwdowns_Details_AnchoredContentPresenter",anchor_schema);
    c.set(c.field(p.outer_anchor,widget),Widget{asset(c,native_menu_detail::horizontal_widget),{0,p.columns.handle}});
    panel_stack_content(c,c.field(page_value,content),
        {asset(c,native_menu_detail::anchored_widget),{0,p.outer_anchor.handle}},hud_panel_id+2);
    const auto back_action=back(c,page_value);
    clear_value(c,c.field(back_action,0x9c76b86c));
    c.text(c.field(back_action,0xb1e14cd6),"ThrowdownDetailsBack");
    const auto back_input=asset(c,"Configurations/Input/UI/UI_Back");
    c.set(c.field(back_action,0x3d00de27),back_input);c.set(c.field(back_action,0x0169c2db),back_input);
    p.page=page_value; // Publish the completed page only after every edge exists.
    match_panel_lease(c).generation_page=p.page.handle;
    logging::log(logging::Level::info,logging::Channel::ui,"{}: private native Throwdown details Page, Quit and scoreboard constructed.",view.title);
    } catch(...) {
        const auto prompt=p.prompt,prompt_anchor=p.prompt_anchor,prompt_item=p.prompt_item;
        const auto requested=p.requested_until;
        const auto menu_was_open=p.menu_was_open;
        p={};p.prompt=prompt;p.prompt_anchor=prompt_anchor;p.prompt_item=prompt_item;p.requested_until=requested;
        p.menu_was_open=menu_was_open;
        state().owned.rollback(checkpoint,[&](Value value){c.destroy(value);});
        throw;
    }
}
void native_match_panel(const Context& c,std::uint64_t now) {
    auto& s=state();auto& p=s.match_panel;const auto view=match_panel_view();
    const auto identity=view.available?view.open_command+":"+view.argument:std::string{};
    if(view.available && p.match_key!=identity) {
        // Restore the native phone slot before retiring the old match's graph.
        // Every match gets fresh press/hold state and callbacks, even when no
        // unavailable frame occurred between the previous Quit and this match.
        restore_match_panel(c);detach_match_panel_prompt(c);
        p={};p.match_key=identity;p.menu_was_open=s.in_menu;
        match_panel_lease(c)={c.manager};
    }
    p.menu_was_open=s.in_menu;
    if(!view.available)p.quitting=false;
    if(!view.available || p.quitting) {
        if(owns_selected_match_panel(c))request_menu_toggle(c);
        restore_match_panel(c);p.requested_until=0;return;
    }
    // A generic menu-open transition includes every D-pad phone shortcut.
    // Borrow the phone only after an actual Start/Menu (or Escape) edge for
    // this match, captured before the UI refresh throttle.
    bool start_requested{};
    {
        std::lock_guard input_lock(s.panel_input_mutex);
        start_requested=!s.panel_start_request.empty() && s.panel_start_request==identity;
        if(start_requested)s.panel_start_request.clear();
    }
    if(start_requested) {
        p.requested_until=now+4000;
        p.request_after=s.in_menu?0:now+80;
        request_activity_details(c);
        logging::write(logging::Level::info,logging::Channel::ui,
            "Throwdowns: Start/Menu pressed for this custom match; requesting its native Details panel.");
    }
    if(!p.prompt.handle) {
        auto& lease=match_panel_lease(c);
        lease.highlight=record_model(c,"Throwdown_DetailsMenu_ContentResource/ThrowdownDetails_HUDButton_Highlightable",{0x510bdc2e,24});
        PrivateUiGraph graph{c};graph.walk(lease.highlight,0);
        p.prompt=reference(c,read<Ref>(c.address(c.field(lease.highlight,0xde29cf1b))));
        require(p.prompt.type==c.type(button),"Native Throwdown shortcut HUDButton differs.");
        clear_value(c,c.field(p.prompt,0x67212c85));
        // The donor's focus predicate queries a native activity backend.
        // Custom match availability is checked above; an absent donor backend
        // must not hide or disable this private shortcut.
        clear_value(c,c.path(p.prompt,{0x0fb0d794,0xa704272a,0xc52416ef,0x66a997b9,0x66a997b9}));
        c.set(c.field(p.prompt,0x659db23e),false);
    }
    plain_label(c,c.field(p.prompt,0x58c9d354),"Throwdown Panel");
    match_panel_input(c,p.prompt,asset(c,"Configurations/Input/UI/UI_Activities_AccessPause"),true);
    const auto prompt_action=match_panel_input_action(c,p.prompt);
    clear_value(c,c.field(prompt_action,0x9c76b86c));
    c.text(c.field(prompt_action,0xb1e14cd6),"");
    c.set(c.path(p.prompt,{0x0fb0d794,0xa704272a,0xc52416ef,0xa704272a,0x85a0b4e8}),!s.in_menu);
    // The official Start graph selects Activities_Details. The native state
    // machine then owns the phone pose, input, Layer_Menu and Back teardown.
    if(p.requested_until) {
        if(now>=p.requested_until) {
            logging::log(logging::Level::warning,logging::Channel::ui,"Throwdowns: native details request expired; native menu open={}, last wait={}.",s.in_menu,match_panel_lease(c).wait_reason);
            match_panel_lease(c).details.retire();match_panel_lease(c).selection={};
            p.requested_until=0;p.request_after=0;
        }
        else if(p.request_after && now>=p.request_after) {
            // Our private action replaces the donor's navigation action.
            // After a real button press, request the native phone transition
            // explicitly if the global pause action has not already opened it.
            const bool opening=s.in_menu || request_native_navigation(c,"ToggleMenu");
            if(opening)p.request_after=0;
        }
    }
    if(now<p.retry_after) {restore_match_panel(c);return;}
    build_match_panel(c,view);
    mount_match_panel_prompt(c);
    if(!s.in_menu) {
        match_panel_wait(c,now,"the native activity pause state");
        if(pending_match_panel_request(c,now))restore_match_panel_mount(c);
        else restore_match_panel(c);
        return;
    }
    plain_label(c,c.field(p.header,0x00f3b15e),view.title);
    plain_label(c,c.field(p.notice,0x94703efc),view.description);
    mode_art(c,std::array{c.field(p.badge,0x6e469d2a)},view.logo);
    plain_label(c,p.setting_values[0],std::to_string(view.players.size()));
    plain_label(c,c.field(p.setting_rows[1],0x58c9d354),view.timer_caption);
    plain_label(c,p.setting_values[1],view.timer);
    update_match_panel_quit(c,p.page,p.quit,view);
    std::vector<Widget> rows;unsigned rank{};
    for(const auto& player:view.players) {
        auto& row=p.players[player.id];
        if(!row.outcome.handle) {
            row.outcome=record_model(c,"TD_Hud_ContentResources/TD_HUD_Header_Label");
            label_styles(c,row.outcome,"TextStylesList/B52-SemiB_Black","TextStylesList/B52-SemiB_Black");
        }
        plain_label(c,row.outcome,std::to_string(++rank)+". "+player.name+"    "+player.value);
        rows.push_back({asset(c,"UI/Foundations/Components/Text/Label/Widget/Label_Widget"),{0,row.outcome.handle}});
    }
    // Publish handles matching the label blueprint, rather than feeding a
    // profile/score model into the legacy scoreboard's cached bindings.
    if(!hud_list_has_items(c,p.board,static_cast<unsigned>(rows.size())))hud_list(c,p.board,rows,false,72.f,12.f,0.f,1120.f);
    const auto layer=native_record_model(c,asset(c,"UI_Layer_Data/Layer_Menu"),0xd71ee0a1);
    if(!layer.handle) {match_panel_wait(c,now,"the native Layer_Menu model");return;}
    const auto item=active_match_panel_layer_item(c,layer);
    if(!item.handle) {match_panel_wait(c,now,"an active current native menu layer item");return;}
    const auto slot=c.field(item,content);
    const auto item_id=read<int>(c.address(c.field(item,0x1e95752c)));
    const auto current=read<Widget>(c.address(slot));
    // Foundation blueprints can have separate loaded instances in the
    // gameplay and activity UI domains. The retained native slot supplies the
    // actual instance; identify it by its validated asset type and exact name,
    // then retain that same instance for the private page and ownership check.
    auto& lease=match_panel_lease(c);
    const auto page_blueprint=asset(c,"UI/Foundations/Templates/Page/Page_Widget");
    // Never replace a live popup or another owner's transition. This request
    // must come from the real pause opening during this exact custom match.
    if(current.data.handle!=p.page.handle && !pending_match_panel_request(c,now))return;
    if(!current.blueprint || read<Address>(current.blueprint+8)!=c.base+build::native_menu::widget_blueprint_type) {
        match_panel_wait(c,now,"an initialized native phone page");return;
    }
    const auto current_name=string(read<Address>(current.blueprint+0x18),256);
    if(current.data.handle!=p.page.handle &&
       current_name!="UI/Foundations/Templates/Page/Page_Widget" &&
       current_name!="UI/Foundations/Components/Stack/StackScreen") {
        match_panel_wait(c,now,"the phone's native Page or StackScreen");return;
    }
    if(current.data.handle==p.page.handle && p.mounted_slot.handle!=slot.handle) {
        // A native array relocation can give this same item a fresh field
        // handle. Retain its original saved Content; never recapture our page.
        if(lease.details.layer!=layer.handle || lease.details.item!=item_id)return;
        p.mounted_slot=slot;
    } else if(current.data.handle!=p.page.handle) {
        restore_match_panel_mount(c);
        if(!p.saved_slot.handle)p.saved_slot=make(c,presenter);
        c.copy(p.saved_slot,c.address(slot));p.mounted_slot=slot;
        // The phone's native ToggleMenu flow owns input/pose/close. The private
        // footer alone owns Quit; no global ThrowdownDetailsBack interception.
        const auto close=back(c,p.page);
        clear_value(c,c.field(close,0x9c76b86c));c.text(c.field(close,0xb1e14cd6),"ToggleMenu");
        lease.back_navigation="ToggleMenu";
        c.set(slot,Widget{page_blueprint,{0,p.page.handle}});
        logging::log(logging::Level::info,logging::Channel::ui,"{}: native activity details layer mounted with Quit and leaderboard.",view.title);
    }
    lease.details.mount(p.page.handle,layer.handle,item_id);lease.selection={};lease.page_blueprint=page_blueprint;
    lease.wait_reason.clear();lease.wait_logged=0;
    // Publish only Content once. StackIndex, target/focus and every animation
    // track remain exactly as the activity flow authored them.
    p.requested_until=0;s.match_panel_open.store(true,std::memory_order_release);
}
