-- Approved Skate 2 presentation, expressed as plugin-owned layout and art.
-- All rendering goes through the SDK overlay. World geometry never enters it.
local M = require("td/core")
local RR = require("td/h/race_route")
local RE = require("td/h/race_editor")
local RN = require("td/h/race_navigation")
local P = {}
M.presentation = P
local art = reskate.json.decode(assert(reskate.package.read("art/index.json"), "HUD art index is missing"))
local V = vector.create
local paper, muted, gold = "#FFF4DCFF", "#D3C9B8FF", "#FFE68BFF"
local inputs = {
    UI_PrimaryAction = "a", UI_Back = "b", UI_SecondaryAction = "x", UI_TertiaryAction = "y",
    UI_TabSecondaryNavLeft = "ltrigger", UI_TabSecondaryNavRight = "rtrigger",
    UI_TabPrimaryNavLeft = "lbumper", UI_TabPrimaryNavRight = "rbumper",
    UI_DpadLeft = "dpad-left", UI_DpadRight = "dpad-right", UI_DpadUp = "dpad-up", UI_DpadDown = "dpad-down",
    UI_DpadHorizontal = "dpad-horizontal", UI_LeftStick = "lstick", UI_L3Click = "lstick-click",
    UI_R3Click = "rstick-click", UI_Activities_AccessPause = "menu", UI_SelectButton = "options",
}
local keys = {UI_PrimaryAction="Enter",UI_Back="Esc",UI_SecondaryAction="R",UI_TertiaryAction="Y",
    UI_TabSecondaryNavLeft="Q",UI_TabSecondaryNavRight="E",UI_TabPrimaryNavLeft="Ctrl",UI_TabPrimaryNavRight="Space",
    UI_DpadLeft="PgUp",UI_DpadRight="PgDn",UI_DpadUp="]",UI_DpadDown="[",UI_L3Click="L3",UI_R3Click="R3",
    UI_Activities_AccessPause="Start",UI_SelectButton="Back",UI_LeftStick="WASD",UI_DpadHorizontal="Arrows"}

local Canvas = {}; Canvas.__index = Canvas
function P.canvas(d)
    local size = d:size()
    local actual = 1080 * size.x / math.max(1, size.y)
    local width = math.clamp(actual, 1440, 3840)
    local scale = math.min(1, actual / width)
    local pad = reskate.input.gamepad()
    return setmetatable({d=d,width=width,k=size.y/1080*scale,
        ox=size.x*.5-width*.5*size.y/1080*scale,oy=size.y*.5-540*size.y/1080*scale,pad=pad},Canvas)
end
function Canvas:xy(x,y) return V(self.ox+x*self.k,self.oy+y*self.k,0) end
function Canvas:logical(point) return (point.x-self.ox)/self.k,(point.y-self.oy)/self.k end
function Canvas:rect(x,y,w,h,colour,outline,rounding)
    self.d:rect(self:xy(x,y),self:xy(x+w,y+h),{colour=colour,fill=not outline,thickness=1,rounding=rounding or 0})
end
function Canvas:image(ref,x,y,w,h,colour,rounding)
    self.d:image(art.pages[ref[1]+1],self:xy(x,y),self:xy(x+w,y+h),
        {colour=colour or "#FFFFFFFF",rounding=rounding or 0,uv={ref[2],ref[3],ref[4],ref[5]}})
end
function Canvas:sprite(name,x,y,w,h,colour,rounding)
    local ref=art.sprites[name]
    if ref then self:image(ref,x,y,w,h,colour,rounding) end
end
function Canvas:panel(x,y,w,h) self:sprite("panel",x,y,w,h,"#FFFFFFBF",10) end
function Canvas:rule(x,y,w) self:rect(x,y,w,1,"#DCC4A033") end
local characters,character_order={},{}
local function pieces(text)
    if characters[text] then return characters[text] end
    local result={}
    for _,code in utf8.codes(text) do table.insert(result,utf8.char(code)) end
    if #character_order>=512 then characters[table.remove(character_order,1)]=nil end
    table.insert(character_order,text);characters[text]=result
    return result
end
-- All font pages come from the exact native TrueType files used by the published build.
-- Glyphs are baked at 64/72/192/224 px, then fitted down; no enlarged countdown bitmap.
function Canvas:measure(text,size,role)
    local font=art.fonts[role or "body"];local scale=size/font.size
    local width,bottom,top,line=0,0,0,0
    local x=0
    for _,ch in ipairs(pieces(tostring(text))) do
        if ch=="\n" then width=math.max(width,x);x=0;line+=size*1.15
        else
            local g=font.glyphs[ch]
            if g then
                top=math.min(top,line+g.top*scale);bottom=math.max(bottom,line+(g.top+g.height)*scale)
                x+=g.advance*scale
            else x+=size*.6;top=math.min(top,line-size*.8);bottom=math.max(bottom,line+size*.2) end
        end
    end
    return math.max(width,x),math.max(size*.75,bottom-top),top
end
function Canvas:text(x,y,w,h,text,size,role,colour,align,shadow)
    text=tostring(text or "");if text=="" or w<=0 or h<=0 then return end
    role=role or "body";colour=colour or paper;align=align or "left"
    local width,height,top=self:measure(text,size,role)
    local fit=math.min(1,w/math.max(1,width),h/math.max(1,height))
    size*=fit;width*=fit;height*=fit;top*=fit
    local font=art.fonts[role];local unit=size/font.size
    local origin=x+(align=="right" and w-width or align=="centre" and (w-width)*.5 or 0)
    local xx,baseline=origin,y+(h-height)*.5-top
    local line_y=baseline
    self.d:clip(self:xy(x,y),self:xy(x+w,y+h))
    for _,ch in ipairs(pieces(text)) do
        if ch=="\n" then xx=origin;line_y+=size*1.15
        else
            local g=font.glyphs[ch]
            if g then
                if g.image then
                    if shadow then self:image(g.image,xx+g.left*unit+1,line_y+g.top*unit+1,g.width*unit,g.height*unit,"#080503D9") end
                    self:image(g.image,xx+g.left*unit,line_y+g.top*unit,g.width*unit,g.height*unit,colour)
                end
                xx+=g.advance*unit
            else
                -- Preserve unsupported Unicode through the SDK font; never rewrite a player's name.
                self.d:text(self:xy(xx,line_y-size*.8),ch,{size=size*self.k/(self.d:size().y/1080),colour=colour,font="body",shadow=false})
                xx+=size*.6
            end
        end
    end
    self.d:clip()
end
function Canvas:glyph(input,x,y,size)
    local name=inputs[input]
    if self.pad then
        -- Xbox glyphs are native extracted art. PS face buttons use clean geometric symbols.
        local face=self.pad.playstation and (name=="a" or name=="b" or name=="x" or name=="y")
        if face then
            self.d:circle(self:xy(x+size*.5,y+size*.5),size*.44*self.k,{colour="#101010FF",fill=true})
            self.d:circle(self:xy(x+size*.5,y+size*.5),size*.41*self.k,{colour=paper,thickness=2})
            local a,b,c=.3*size,.5*size,.7*size
            local function line(ax,ay,bx,by) self.d:line(self:xy(x+ax,y+ay),self:xy(x+bx,y+by),{colour=paper,thickness=2}) end
            if name=="a" then line(a,a,c,c);line(c,a,a,c)
            elseif name=="b" then self.d:circle(self:xy(x+b,y+b),size*.21*self.k,{colour=paper,thickness=2})
            elseif name=="x" then self:rect(x+a,y+a,c-a,c-a,paper,true)
            else line(b,a,c,c);line(c,c,a,c);line(a,c,b,a) end
        elseif name then self:sprite(name,x,y,size,size) end
    else
        self:rect(x+1,y+size*.1,size-2,size*.8,"#F4F0E6FF")
        self:rect(x+1,y+size*.1,size-2,size*.8,"#15120FFF",true)
        self:text(x+3,y+size*.1,size-6,size*.8,keys[input] or "?",size*.36,"body","#16120FFF","centre")
    end
end
function Canvas:control(x,y,w,input,second,caption,size)
    self:glyph(input,x,y-3,40)
    if second and second~="" then self:glyph(second,x+42,y-3,40) end
    self:text(x+94,y,w-94,34,caption,size or 23)
end
function Canvas:button(x,y,w,h,caption,input,selected)
    self:rect(x,y,w,h,selected and "#69181DDB" or "#151211C7",false,3)
    self:rect(x,y,w,h,selected and "#E27665A6" or "#DCC4A05C",true,3)
    if selected then self:rect(x,y,2,h,"#D93338F2") end
    self:glyph(input or "UI_PrimaryAction",x+12,y+(h-40)*.5,40)
    self:text(x+62,y+(h-34)*.5,w-76,34,caption,23)
end
function Canvas:header(x,y,w,title,sub)
    self:sprite("logo",x+25,y+22,56,56)
    self:text(x+104,y+20,w-132,56,title,40,"marker")
    if sub and sub~="" then self:text(x+28,y+87,w-56,33,sub,25) end
    self:rule(x+28,y+132,w-56)
end
function P.suffix(n)
    local last=n%100
    return last>=11 and last<=13 and "th" or n%10==1 and "st" or n%10==2 and "nd" or n%10==3 and "rd" or "th"
end
function Canvas:ordinal(x,y,w,h,rank,size,total)
    local colour=rank==1 and "#FFE071FF" or rank==2 and "#DFE6E9FF" or rank==3 and "#E4B384FF" or paper
    local digit,suffix=tostring(rank),P.suffix(rank)
    local number_w=self:measure(digit,size,"digits")
    local suffix_w=self:measure(suffix,size*.30,"body")
    local total_text=total and "/"..tostring(total)..P.suffix(total) or ""
    local total_w=self:measure(total_text,size*.34,"body")
    local fit=math.min(1,w/math.max(1,number_w+suffix_w+total_w+19),h/size)
    size*=fit;number_w=self:measure(digit,size,"digits");suffix_w=self:measure(suffix,size*.30,"body")
    self:text(x,y,number_w,h,digit,size,"digits",colour,nil,true)
    self:text(x+number_w+4,y+size*.12,suffix_w+4,size*.35,suffix,size*.30,"body",colour)
    if total then self:text(x+number_w+suffix_w+15,y+size*.58,math.max(1,w-number_w-suffix_w-15),size*.4,total_text,size*.34,"body",colour) end
end
function P.map_title(map)
    local current=reskate.level.current()
    if type(current)=="table" and current.display_name and current.display_name~="" then return current.display_name end
    local title=string.match(map or "","([^/\\]*)$") or map or ""
    title=string.gsub(title,"^dingolevel_reskate_","");title=string.gsub(title,"^dingolevel_","")
    return string.gsub(title,"_"," ")
end
function P.builder(c,n)
    local x,y,w=52,184,420
    c:panel(x,y,w,604);c:sprite("logo",x+20,y+20,56,56)
    c:text(x+90,y+18,w-110,52,n.rotating and "ROTATE OBJECT" or "DEATHRACE",35,"marker")
    c:text(x+93,y+68,w-116,23,n.rotating and (n.route_object==4 and (n.editor_roll_axis and "ARROW / ROLL" or "ARROW / TILT") or "GATE ORIENTATION") or "BUILDER",19)
    c:rule(x+24,y+94,w-48)
    if n.rotating then
        for i,row in ipairs({{"YAW",n.cursor_yaw},{"TILT",n.cursor_pitch},{"ROLL",n.cursor_roll}}) do
            c:text(x+25+127*(i-1),y+113,115,25,row[1],19,"body",paper,"centre")
            c:text(x+25+127*(i-1),y+149,115,47,string.format("%.0f°",row[2]),34,"body",paper,"centre")
        end
        c:control(x+23,y+225,w-46,"UI_DpadLeft","UI_DpadRight","Yaw left / right")
        c:control(x+23,y+279,w-46,"UI_DpadUp","UI_DpadDown",n.editor_roll_axis and "Roll up / down" or "Tilt up / down")
        c:control(x+23,y+338,w-46,"UI_L3Click","","Reset original orientation")
        c:control(x+23,y+393,w-46,"UI_R3Click","",n.route_object==4 and "Switch tilt / roll axis" or "Original tilt and yaw")
        c:control(x+23,y+448,w-46,"UI_SecondaryAction","","Finish rotating")
        c:rule(x+24,y+516,w-48);c:text(x+25,y+534,w-50,28,"5-degree steps",23)
        c:control(x+25,y+567,w-50,"UI_DpadHorizontal","","Hold / turn faster",19)
    else
        local rows={
            {"UI_TabSecondaryNavLeft","UI_TabSecondaryNavRight","Object: "..RE.race_editor_object_name(n.route_object)},
            {"UI_TabPrimaryNavLeft","UI_TabPrimaryNavRight","Camera down / up"},
            {"UI_PrimaryAction","","Place object"},{"UI_SecondaryAction","","Rotate / 5 degrees"},
            {"UI_DpadLeft","UI_DpadRight","Undo / Redo"},
            {"UI_DpadUp","UI_DpadDown",string.format("Size %.0f%%",n.cursor_width/6*100)},
            {"UI_Activities_AccessPause","",n.route_saved and "Tap: Play / Hold: Load" or "Tap: Save / Hold: Load"},
            {"UI_Back","","Close editor"},{"UI_TertiaryAction","",n.route_snap and "Snap ON" or "Snap OFF"},
            {"UI_R3Click","",string.format("Placement snap: %.0f deg",n.snap_degrees)},
            {"UI_SelectButton","","Hold: Clear route"}}
        for i,row in ipairs(rows) do c:control(x+23,y+108+37*(i-1),w-46,row[1],row[2],row[3]) end
        if n.save_progress>0 then c:rect(x+111,y+108+37*6+32,275*n.save_progress,3,"#B32731FF") end
        if n.clear_progress>0 then c:rect(x+111,y+108+37*10+32,275*n.clear_progress,3,"#B32731FF") end
        c:rule(x+24,y+528,w-48);c:text(x+25,y+535,w-50,26,"MOVEMENT",20)
        c:control(x+23,y+567,w-46,"UI_L3Click","","Hold / faster flight",23)
    end
    local required,guides,arrows,length,previous=0,0,0,0,1
    local st=n.settings
    for i=2,#st.points do
        if M.arrow_marker(st,i-1) then arrows+=1
        else
            if M.guiding_gate(st,i-1) then guides+=1 elseif not (st.race_finish and i==#st.points) then required+=1 end
            length+=vector.magnitude(st.points[i]-st.points[previous]);previous=i
        end
    end
    local ix,iy,iw=c.width-340,184,288;c:panel(ix,iy,iw,334);c:text(ix+12,iy+12,iw-24,36,"ROUTE INFO",28,"marker")
    local info={{"Required gates",tostring(required)},{"Guide gates",tostring(guides)},{"Arrows",tostring(arrows)},
        {"Finish gate",n.route_finish and "Placed" or "Needed"},{"Height",string.format("%.1f m",n.cursor.y)},
        {"Size",string.format("%.0f%%",n.cursor_width/6*100)},{"Route length",string.format("%.0f m",length)}}
    for i,row in ipairs(info) do c:text(ix+15,iy+67+32*(i-1),167,29,row[1],23);c:text(ix+183,iy+67+32*(i-1),90,29,row[2],23,"body",paper,"right") end
end

local previous_game,previous_phase,go_until=nil,nil,0
local function race_rows(c,n,now,x,y,w,final)
    local state=n.state
    for i,row in ipairs(state.standings) do
        if i>6 then break end
        local yy=y+50*(i-1)
        c:rect(x,yy,w,44,row.player==n["local"] and "#69181D99" or "#15121145")
        if row.out then c:text(x+10,yy+5,final and 70 or 48,34,"DNF",21,"body",paper,"centre")
        else c:ordinal(x+12,yy+2,final and 70 or 48,40,i,final and 35 or 30) end
        local name=row.player
        for _,known in ipairs(n.names) do if known.id==row.player then name=known.name;break end end
        local nx=x+(final and 100 or 65);local tw=final and 168 or 105
        c:text(nx,yy+2,w-(nx-x)-tw-24,40,name,final and 28 or 24,"names")
        local running=state.phase=="playing" and not row.out and row.score<#n.settings.points
        local elapsed=math.max(0,row.aux)+(running and math.max(0,now-n.state_sampled_ms) or 0)
        c:text(x+w-tw-12,yy+1,tw,final and 40 or 23,M.race_time(elapsed),final and 25 or 21,"body",paper,"right")
        if not final then c:text(x+w-tw-12,yy+25,tw,19,string.format("%d / %d gates",M.mandatory_gates(n.settings,math.max(0,row.score)),M.mandatory_gates(n.settings,#n.settings.points)),16,"muted",muted,"right") end
    end
end
function P.race(c,n,feed,now)
    local st=n.state;if not st or st.phase=="setup" then return end
    if n.game~=previous_game then previous_game=n.game;previous_phase=nil;go_until=0 end
    if st.phase=="playing" and previous_phase=="countdown" then go_until=now+1000 end
    previous_phase=st.phase
    local sampled=math.max(0,now-n.state_sampled_ms)
    local remaining=math.max(0,st.remaining_ms-((st.phase=="playing" or st.phase=="countdown") and sampled or 0))
    if st.phase=="countdown" and remaining>3000 then
        local x,y,w=(c.width-780)*.5,190,780;c:panel(x,y,w,335);c:header(x,y,w,"DEATHRACE",n.route_name)
        c:text(x+28,y+155,w-56,84,"Pass Required gates in order, then the red Finish.\nGuide gates are optional. Off-board is allowed.",26,"body",paper,"centre")
        c:control(x+165,y+268,w-300,"UI_DpadUp","","Return behind previous gate",23);return
    end
    if feed.results or st.phase=="results" then
        local x,y,w=(c.width-900)*.5,178,900;c:panel(x,y,w,610);c:header(x,y,w,"DEATHRACE RESULTS",n.route_name.." / "..P.map_title(n.route_map))
        c:text(x+28,y+151,w-56,52,feed.winner=="" and "NO FINISHERS" or feed.winner.." WINS!",40,"names",gold,"centre")
        c:text(x+40,y+210,65,26,"POS",19);c:text(x+128,y+210,400,26,"PLAYER",19);c:text(x+w-200,y+210,160,26,"TIME",19,"body",paper,"right")
        race_rows(c,n,now,x+28,y+247,w-56,true);return
    end
    local me,place=nil,0
    for i,row in ipairs(st.standings) do if row.player==n["local"] then me=row;place=i;break end end
    local done=me and (me.out or me.score>=#n.settings.points)
    local elapsed=me and math.max(0,me.aux)+(not done and st.phase=="playing" and sampled or 0) or 0
    local cleared=me and M.mandatory_gates(n.settings,math.max(0,me.score)) or 0
    local required=M.mandatory_gates(n.settings,#n.settings.points)
    local brand_w=math.min(296,(c.width-800)*.5-65)
    c:panel(46,44,brand_w,104);c:sprite("logo",58,55,48,48);c:text(118,55,brand_w-84,48,"DEATHRACE",31,"marker");c:text(60,108,brand_w-28,29,n.route_name,21)
    c:panel(46,190,296,520);c:text(58,204,260,36,"YOUR POSITION",26,"marker")
    if place>0 and not (me and me.out) then c:ordinal(60,233,260,107,place,101,#st.standings) else c:text(60,235,150,95,"DNF",54,"marker") end
    c:text(58,361,260,36,done and "FINAL TIME" or "TIME LEFT",26,"marker")
    local race_remaining=st.phase=="countdown" and n.settings.duration_s*1000 or remaining
    c:text(58,403,270,59,M.race_time(done and elapsed or race_remaining),51,"digits")
    c:text(60,470,260,30,"LIMIT  "..M.race_time(n.settings.duration_s*1000),21);c:rule(58,509,245)
    c:text(58,529,260,36,"REQUIRED GATES",26,"marker");c:text(58,579,263,52,string.format("%d / %d",cleared,required),39,"body",gold)
    c:text(60,641,260,51,cleared>=required and "Red Finish unlocked" or "Follow the yellow markers",20)
    if #st.standings>0 then
        local bx,by,bw=c.width-404,190,350;c:panel(bx,by,bw,76+50*math.min(6,#st.standings))
        c:text(bx+18,by+14,bw-36,36,"LIVE STANDINGS",27,"marker");race_rows(c,n,now,bx+10,by+70,bw-20,false)
    end
    if st.phase=="countdown" or now<go_until then
        local go=st.phase=="playing";local cw,ch=go and 180 or 112,go and 123 or 154
        c:text((c.width-cw)*.5,1080*.42,cw,ch,go and "GO!" or tostring(math.clamp(math.ceil(remaining/1000),1,3)),go and 86 or 112,go and "marker" or "digits",paper,"centre",true)
    end
    -- No race gate labels, duplicate Previous gate prompt or bottom GO toast.
    local notice=feed.banner or ""
    if notice=="GO!" or string.find(string.lower(notice),"passed",1,true) then notice="" end
    if notice~="" then c:rect((c.width-620)*.5,750,620,67,"#69181D99");c:text((c.width-620)*.5+12,750,596,67,notice,26,"body",paper,"centre") end
end

local native_hud=M.ui_game_hud
function M.ui_game_hud(s,now)
    local n=s.native
    if not n or not n.active or n.settings.mode~="race" then return native_hud(s,now) end
    -- The waiting document keeps the flag alive while our overlay editor is open.
    -- A non-editor native HUD here would tell the SDK that the match had started.
    local phase=n.state and n.state.phase
    -- Native flag placement can publish one tick before the editor snapshot catches up.
    -- The SDK treats ANY non-editor HUD as a started match, even an empty theme table.
    local g=s.game
    local current_phase=g and g.state and g.state.phase
    if n.placing or s.route_restore_pending or not g or g.id~=n.game or current_phase==nil or current_phase=="setup"
        or (phase~="countdown" and phase~="playing" and phase~="results") then return nil end
    -- Keep the native compass. Setup, join, waiting and panel prompts remain in td/ui.lua.
    local hud={theme="race"}
    if n.state and not n.placing then
        local next_gate=RN.race_compass_gate(n.settings,n.state,n["local"])
        if next_gate then hud.compass={at=n.settings.points[next_gate+1],
            art=n.settings.race_finish and next_gate+1==#n.settings.points and "UI/ReSkate/Throwdowns/img_FinishGateCompass" or "UI/ReSkate/Throwdowns/img_GateCompass"} end
    end
    return hud
end
local world_guides=M.draw
local last_visibility=""
local function visibility(reason)
    if reason~=last_visibility then last_visibility=reason;reskate.log("Deathrace HUD: "..reason) end
end
function M.draw(d)
    local s=M.state();local n=s.native;local focus=reskate.input.focus()
    if not n or not n.active or n.settings.mode~="race" then return world_guides(d) end
    if M.route_screen and M.route_screen.draw(d,s) then return end
    if focus.interface_open or not focus.gameplay then
        visibility(focus.interface_open and "hidden by the overlay menu" or "hidden by a native menu or loading")
        return
    end
    visibility(n.placing and "drawing builder" or "drawing "..(n.state and n.state.phase or "waiting"))
    local c=P.canvas(d)
    if n.placing then
        if n.exit_confirm then
            local x,y,w=(c.width-760)*.5,190,760;c:panel(x,y,w,340);c:header(x,y,w,"LEAVE BUILDER?",n.route_name)
            c:text(x+28,y+151,w-56,82,n.route_saved and "Your saved route is ready to play." or "Your unfinished draft stays private.\nSave the route before sharing it with other players.",26,"body",paper,"centre")
            c:button(x+28,y+268,337,49,"Leave editor","UI_PrimaryAction");c:button(x+395,y+268,337,49,"Keep editing","UI_Back")
        else P.builder(c,n);world_guides(d) end
    else P.race(c,n,s.hud,M.now_ms()) end
end
return P
