-- Plugin-owned route UI. Native setup/join/Hold Start still belong to the SDK.
local M=require("td/core")
local P=require("td/presentation")
local U={selected=1,action=1,actions=false,first=1,name="",open=false,importing=false,serial=-1,kind=0}
M.route_screen=U
local title="Deathrace routes"
local previous_bits,previous_keys=0,{}
local direction,repeat_at=0,0
local erase_at=0
local pointer,clicked=nil,false
local actions={{"Load route","UI_PrimaryAction","load"},{"Rename","UI_SecondaryAction","rename"},
    {"Show folder","UI_TabPrimaryNavRight","reveal"},{"Delete","UI_TertiaryAction","delete"},
    {"Load .json","UI_TabPrimaryNavLeft","import"},{"Back to builder","UI_Back","cancel"}}
local function routes(s)
    local list={}
    if s.route_library then for _,route in ipairs(s.route_library:routes()) do if route.map==s.course_map then table.insert(list,route) end end end
    return list
end
local function send(verb,value)
    local result=M.on_ui_command("route-"..verb,{value or ""})
    if type(result)=="string" and M.starts_with(result,"error:") then M.state().route_dialog_error=string.sub(result,8) end
    return result
end
local function layout(width,list)
    local w=list and 980 or 800
    return {x=(width-w)*.5,y=list and 178 or 190,w=w,h=list and 614 or 466}
end
local function action_rect(r,i,list)
    if not list then return r.x+(i==1 and 28 or 418),r.y+394,354,46 end
    return r.x+28+310*((i-1)%3),r.y+451+53*math.floor((i-1)/3),298,45
end
local function row_rect(r,i) return r.x+28,r.y+146+70*(i-1),r.w-56,62 end
local function inside(x,y,rx,ry,w,h) return x>=rx and x<=rx+w and y>=ry and y<=ry+h end
function U.move(horizontal,vertical,count)
    if not U.actions then
        if horizontal~=0 then U.actions=true;U.action=horizontal>0 and 1 or 3
        elseif vertical>0 then
            if count==0 or U.selected>=count then U.actions=true;U.action=1 else U.selected+=1 end
        elseif vertical<0 then U.selected=math.max(1,U.selected-1) end
    else
        local col=(U.action-1)%3;local row=math.floor((U.action-1)/3)
        if horizontal~=0 then col=math.clamp(col+horizontal,0,2) end
        if vertical<0 and row==0 and count>0 then U.actions=false;U.selected=math.clamp(U.selected,1,count)
        elseif vertical~=0 then row=math.clamp(row+vertical,0,1) end
        U.action=1+row*3+col
    end
    U.first=math.max(1,math.min(U.first,U.selected));if U.selected>U.first+3 then U.first=U.selected-3 end
end
local function import_dialog()
    local ok,why=reskate.ui.dialog("route-import",{title="IMPORT DEATHRACE ROUTE",
        message="Choose a completed Deathrace JSON saved on this map. Its saved starting flag and all objects are kept.",
        file={label="Choose .json file",filter="*.json"},buttons={{id="cancel",label="Back to route library"}}})
    if ok then U.importing=true else M.state().route_dialog_error=tostring(why) end
end
local function activate(s,i)
    local item=actions[i];if not item then return end
    if item[3]=="cancel" then send("cancel")
    elseif s.route_library and s.route_library:loading() then s.route_dialog_error="Saved routes are still loading. Please wait a moment."
    elseif item[3]=="import" then import_dialog()
    else
        local selected=routes(s)[U.selected]
        if selected then send(item[3],tostring(selected.id)) else s.route_dialog_error="Choose a saved route first." end
    end
end
reskate.ui.screen(title,function() end,{panel=false})
reskate.on("screen_closed",function(name)
    if name==title and U.open then U.open=false;U.importing=false;send("cancel") end
end)
local old_dialog=M.on_route_dialog
function M.on_route_dialog(s,args)
    if args[1]~="route-import" then return old_dialog(s,args) end
    if not U.importing then return end
    if args[2]=="file" then send("import",args[5] or "")
    elseif args[2]=="file_error" then s.route_dialog_error=args[3] or "The route file could not be opened." end
    if args[2]=="file" or args[2]=="cancel" then
        reskate.ui.close_dialog("route-import");U.importing=false
    end
end
function M.route_dialog_tick(s)
    if s.route_library and s.route_library.read_error_ then s.route_dialog_error=s.route_library.read_error_ end
    local kind=s.route_dialog or 0
    local active=s.game and s.game.leading and s.game.settings.mode=="race" and s.placing=="points" and kind~=0
    if not active then
        if U.open then reskate.ui.close();U.open=false end
        if U.importing then reskate.ui.close_dialog("route-import");U.importing=false end
        U.serial=-1;erase_at=0;return
    end
    if not U.open then
        reskate.ui.open(title);U.open=true;U.serial=-1
        previous_bits=reskate.input.pad_bits(true) or 0;previous_keys={};direction=0;repeat_at=0
    end
    if U.serial~=s.route_dialog_serial then
        U.serial=s.route_dialog_serial;U.kind=kind
        local selected=s.route_library and s.route_library:find(s.route_rename_id)
        U.name=(kind==3 or kind==4) and selected and selected.name or s.route_name or ""
        if kind==2 then
            local list=routes(s);U.selected=math.clamp(U.selected,1,math.max(1,#list));U.actions=#list==0;U.action=1
        else U.action=kind==4 and 1 or 1 end
        previous_bits=reskate.input.pad_bits(true) or 0;previous_keys={};direction=0;erase_at=0
    end
end
local function editing() return U.kind==1 or U.kind==3 end
function U.tick(s,now)
    if not U.open or U.importing or not s.game then return end
    if s.route_restore_pending then return end
    local focus=reskate.input.focus()
    if not focus.focused or not focus.gameplay or (focus.interface_open and not focus.screen_open) then erase_at=0;return end
    local bits=reskate.input.pad_bits(true) or 0
    local pressed=bit32.band(bits,bit32.bnot(previous_bits));previous_bits=bits
    local names={"Up","Down","Left","Right","Enter","Escape","X","Y","Delete","F2","Backspace"}
    local key,down={},{}
    for _,name in ipairs(names) do down[name]=reskate.input.key(name=="F2" and 113 or name);key[name]=down[name] and not previous_keys[name] end
    previous_keys=down
    local pad=reskate.input.gamepad()
    local x=(bit32.band(bits,8)~=0 or down.Right or (pad and pad.left_x>.55)) and 1
        or (bit32.band(bits,4)~=0 or down.Left or (pad and pad.left_x<-.55)) and -1 or 0
    local y=(bit32.band(bits,2)~=0 or down.Down or (pad and pad.left_y<-.55)) and 1
        or (bit32.band(bits,1)~=0 or down.Up or (pad and pad.left_y>.55)) and -1 or 0
    local current=x~=0 and x or y*10
    if current~=0 and (current~=direction or now>=repeat_at) then
        if U.kind==2 then U.move(x,y,#routes(s)) else U.action=(x>0 or y>0) and 2 or 1 end
        repeat_at=now+(current==direction and 130 or 350)
    end
    direction=current
    if editing() then
        local typed=reskate.input.text(true)
        for _,code in utf8.codes(typed) do if code>=32 and code~=127 and utf8.len(U.name)<64 then U.name..=utf8.char(code) end end
        if down.Backspace then
            if erase_at==0 or key.Backspace then
                if #U.name>0 then U.name=string.sub(U.name,1,(utf8.offset(U.name,-1) or 1)-1) end
                erase_at=now+350
            elseif now>=erase_at then
                local count=math.min(8,1+math.floor((now-erase_at)/45))
                for _=1,count do if #U.name>0 then U.name=string.sub(U.name,1,(utf8.offset(U.name,-1) or 1)-1) end end
                erase_at=now+45
            end
        else erase_at=0 end
    end
    local cursor=reskate.input.cursor()
    if cursor and U.canvas then
        local mx,my=U.canvas:logical(cursor.position);local moved=not pointer or pointer.x~=mx or pointer.y~=my
        pointer={x=mx,y=my}
        local click=cursor.left and not clicked;clicked=cursor.left
        local r=layout(U.canvas.width,U.kind==2)
        if U.kind==2 then
            local list=routes(s)
            for i=U.first,math.min(#list,U.first+3) do
                if inside(mx,my,row_rect(r,i-U.first+1)) and (moved or click) then U.selected=i;U.actions=false;if click then activate(s,1) end end
            end
            for i=1,6 do if inside(mx,my,action_rect(r,i,true)) and (moved or click) then U.action=i;U.actions=true;if click then activate(s,i) end end end
        else
            for i=1,2 do if inside(mx,my,action_rect(r,i,false)) and (moved or click) then U.action=i;if click then pressed=bit32.bor(pressed,0x1000) end end end
        end
    else clicked=false end
    if key.Escape or bit32.band(pressed,0x2000)~=0 then send("cancel");return end
    if U.kind==2 then
        if key.X or key.F2 or bit32.band(pressed,0x4000)~=0 then activate(s,2)
        elseif key.Y or key.Delete or bit32.band(pressed,0x8000)~=0 then activate(s,4)
        elseif bit32.band(pressed,0x100)~=0 then activate(s,5)
        elseif bit32.band(pressed,0x200)~=0 then activate(s,3)
        elseif key.Enter or bit32.band(pressed,0x1000)~=0 then activate(s,U.actions and U.action or 1) end
    elseif key.Enter or bit32.band(pressed,0x1000)~=0 then
        if U.kind==4 then send(U.action==1 and "cancel" or "delete-confirm")
        elseif U.action==2 then send("cancel")
        else send(U.kind==3 and "rename-save" or "save",U.name) end
    end
end
reskate.on("tick",function() U.tick(M.state(),M.now_ms()) end)
function U.draw(d,s)
    if not U.open or U.importing then return false end
    -- Drawing persists while unfocused; U.tick still refuses background input.
    local focus=reskate.input.focus()
    if not focus.gameplay or (focus.interface_open and not focus.screen_open) then return true end
    local c=P.canvas(d);U.canvas=c
    local list=U.kind==2;local r=layout(c.width,list)
    c:rect(0,0,c.width,1080,"#00000066");c:panel(r.x,r.y,r.w,r.h)
    c:header(r.x,r.y,r.w,list and "ROUTE LIBRARY" or U.kind==4 and "DELETE ROUTE?" or U.kind==3 and "RENAME ROUTE" or "SAVE ROUTE",P.map_title(s.course_map))
    if list then
        local saved=routes(s)
        c:text(r.x+r.w-235,r.y+91,207,28,string.format("%d / %d ROUTES",#saved==0 and 0 or U.selected,#saved),20,"muted","#D3C9B8FF","right")
        if #saved==0 then
            c:text(r.x+28,r.y+205,r.w-56,38,(s.route_library and s.route_library:loading()) and "Loading saved routes..." or "No saved routes on this map.",26,"body",nil,"centre")
            c:control(r.x+210,r.y+256,r.w-390,"UI_Activities_AccessPause","","Build a course, then tap to save",23)
        end
        for i=U.first,math.min(#saved,U.first+3) do
            local route=saved[i];local x,y,w,h=row_rect(r,i-U.first+1)
            c:rect(x,y,w,h,i==U.selected and "#69181D99" or "#15121145")
            if i==U.selected then c:rect(x,y,2,h,"#D93338F2");c:text(x+14,y+17,24,30,">",25,"body","#FFE68BFF","centre") end
            c:text(x+46,y+2,w-60,34,route.name,29)
            c:text(x+46,y+35,w-60,24,string.format("%d Required + Finish  /  %d objects",M.mandatory_gates(route.settings,#route.settings.points),#route.settings.points-1),20,"muted","#D3C9B8FF")
        end
        for i,item in ipairs(actions) do local x,y,w,h=action_rect(r,i,true);c:button(x,y,w,h,item[1],item[2],U.actions and U.action==i) end
        if s.route_dialog_error=="" then
            c:control(r.x+28,r.y+573,390,"UI_DpadHorizontal","UI_LeftStick","Navigate",20)
            c:control(r.x+425,r.y+573,230,"UI_PrimaryAction","","Select",20)
            c:text(r.x+675,r.y+573,r.w-703,34,"Buttons are shortcuts",18,"muted","#D3C9B8FF","right")
        end
    else
        if U.kind==4 then
            c:text(r.x+28,r.y+160,r.w-56,65,U.name,34,"body",nil,"centre")
            c:text(r.x+28,r.y+243,r.w-56,84,"Delete this saved route?\nA recovery copy will be kept.",26,"body",nil,"centre")
        else
            local n=s.native
            c:text(r.x+28,r.y+156,r.w-56,35,string.format("%d Required gates + Finish",M.mandatory_gates(n.settings,#n.settings.points)),25)
            c:text(r.x+28,r.y+212,r.w-56,29,"Route name / type with your keyboard",21)
            c:rect(r.x+28,r.y+254,r.w-56,58,"#110D0CB8");c:rect(r.x+28,r.y+310,r.w-56,2,"#B42932F2")
            c:text(r.x+42,r.y+254,r.w-84,58,U.name=="" and "Name your route" or U.name,28,"body",U.name=="" and "#BFB4A4FF" or nil)
            c:text(r.x+28,r.y+339,r.w-56,31,"Shareable JSON / flag position and objects saved",21,"muted","#D3C9B8FF")
        end
        for i=1,2 do
            local x,y,w,h=action_rect(r,i,false)
            local caption=U.kind==4 and (i==1 and "Cancel" or "Delete route") or (i==2 and "Cancel" or U.kind==3 and "Save name" or "Save route")
            c:button(x,y,w,h,caption,(U.kind==4 and i==1 or U.kind~=4 and i==2) and "UI_Back" or "UI_PrimaryAction",U.action==i)
        end
    end
    if s.route_dialog_error~="" then
        local yy=list and r.y+556 or r.y+r.h+15;c:rect(r.x+28,yy,r.w-56,56,"#69181DE8")
        c:text(r.x+40,yy+3,r.w-80,50,s.route_dialog_error,23,"body",nil,"centre")
    end
    if s.route_restore_pending then
        c:rect(r.x,r.y,r.w,r.h,"#100D0DEB")
        c:text(r.x+28,r.y+r.h*.4,r.w-56,70,"Restoring the saved starting flag...",30,"marker",nil,"centre")
    end
    return true
end
return U
