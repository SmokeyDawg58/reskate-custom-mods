-- Supported plugin APIs only. A player teleport is not a native flag move.
local M = require("td/core")
local RR = require("td/h/race_route")
local API = {}
M.sdk = API
local restoring, arrival = nil, nil
function API.can_restore_flag()
    return type(reskate.skater.teleport)=="function" and type(reskate.throwdowns.flag.restore)=="function"
end
function API.status() return restoring and restoring.status or "" end
function API.automatic_flag_restore() return type(reskate.throwdowns.flag.restore)=="function" end
local function current(request)
    local s=M.state()
    return s.world==request.world and s.game and s.game.id==request.game and s.game.leading
end
local function finite(n) return type(n)=="number" and n==n and math.abs(n)<100000 end
local function pose(at,yaw)
    return typeof(at)=="vector" and finite(at.x) and finite(at.y) and finite(at.z) and finite(yaw)
end
-- A sent teleport is asynchronous. Read the real pose and pending flag before advancing.
function API.travel_to(at,yaw,done,failed,objects_ready)
    local s=M.state()
    if not s.game or not pose(at,yaw) then return false,"The saved start is invalid or the course has closed." end
    if arrival then return false,"A move to the course is already pending." end
    arrival={world=s.world,game=s.game.id,position=at,yaw=yaw,done=done,failed=failed,
        deadline=M.now_ms()+12000,retry=0,sent=false,objects_ready=objects_ready}
    return true
end
function API.arrive(route)
    return API.travel_to(RR.race_start_position(route,0,1),RR.race_start_heading(route),nil,function(why)
        M.popup(M.state(),"Course loaded. "..why)
    end,true)
end
local function stop_restore(why)
    local request=restoring;restoring=nil;arrival=nil
    local s=M.state();s.route_restore_pending=false
    if not request or not current(request) then return end
    s.placing=request.placing;s.route_dialog=request.dialog;s.route_dialog_error=why
    s.route_input_release:arm()
    M.set_placement_flight(s,s.placing~="none")
    M.popup(s,why)
end
function API.restore_flag(mode,game,position,yaw,players,seconds,commit)
    local s=M.state();local g=s.game
    if restoring then return false,"The saved start is already loading." end
    if not g or not g.leading or g.id~=game or g.settings.mode~=mode or (g.state and g.state.phase~="setup") then
        return false,"Only the host can load a course before its race starts."
    end
    if not pose(position,yaw) then return false,"The saved starting flag has an invalid position or facing." end
    -- Stock 2.1.2's begin(false) destroys the current flag before trying to open
    -- placement from freeroam. It can time out with no replacement. Fail before
    -- changing the player, course or registration when the general API is absent.
    if not API.automatic_flag_restore() then
        return false,"Saved-flag loading needs the updated ReSkate Lua flag API. Your current flag and course were kept."
    end
    local flag=reskate.throwdowns.flag.state()
    if flag.busy and (not flag.mine or flag.mode~=mode or flag.game~=game or flag.placing) then
        return false,"Finish the current native flag placement before loading a course."
    end
    arrival=nil -- a newly selected load replaces an unfinished post-load line-up move
    restoring={world=s.world,game=game,mode=mode,position=position,yaw=yaw,players=players,seconds=seconds,
        commit=commit,placing=s.placing,dialog=s.route_dialog,stage="travel",status="Moving to the saved start..."}
    s.route_restore_pending=true;s.route_dialog=0;s.placing="none"
    s.route_input_release:arm();M.set_placement_flight(s,false)
    if M.route_screen and M.route_screen.open then M.route_screen.open=false;reskate.ui.close() end
    local request=restoring
    if API.automatic_flag_restore() then
        local called,accepted,id=pcall(reskate.throwdowns.flag.restore,mode,game,{position=position,yaw=yaw})
        if not called or not accepted then
            local why=called and tostring(id) or tostring(accepted)
            stop_restore(why.." Your current course was kept.");return false,why
        end
        if type(id)~="number" or id<1 or id%1~=0 then
            stop_restore("The SDK did not return a valid flag request id.");return false,"Invalid SDK flag request."
        end
        request.stage="native";request.token=id;request.deadline=M.now_ms()+11000
        request.status="Restoring the saved starting flag..."
        return true,request.status
    end
end
local native_event=M.on_throwdown_ui
function M.on_throwdown_ui(e)
    local request=restoring;local args=e.args or {}
    if request and e.mode==request.mode and tonumber(args[1])==request.game then
        if request.stage=="native" then
            if e.action=="flag_restore_failed" and tonumber(args[3])==request.token then stop_restore((args[2] or "Flag restoration failed.").." Your current course was kept.");return end
            if e.action=="flag_restored" and tonumber(args[6])==request.token and current(request) then
                local at=vector.create(tonumber(args[2]) or 0/0,tonumber(args[3]) or 0/0,tonumber(args[4]) or 0/0)
                local yaw=tonumber(args[5])
                if not pose(at,yaw) or vector.magnitude(at-request.position)>.1 or math.abs((yaw-request.yaw+180)%360-180)>.1 then
                    stop_restore("The SDK returned a different flag pose. Your current course was kept.");return
                end
                restoring=nil;M.state().route_restore_pending=false
                if request.commit then request.commit(at,yaw) end
                return
            end
            if e.action=="flag_restored" or e.action=="flag_restore_failed" then return end
        end
        if e.action=="flag_left" or e.action=="setup_back" then
            stop_restore("Flag placement cancelled. Your course data was kept; load again to place its start.")
            return
        end
    end
    if request and (e.action=="quit" or e.action=="leave") then restoring=nil;arrival=nil;M.state().route_restore_pending=false end
    return native_event(e)
end
local function travel_tick(now)
    local request=arrival;if not request then return end
    if not current(request) then arrival=nil;return end
    local function failed(why) arrival=nil;if request.failed then request.failed(why) end end
    if now>request.deadline then return failed("Could not reach the saved start. Try loading again.") end
    local focus=reskate.input.focus()
    if not focus.focused or focus.interface_open or not focus.gameplay then return end
    if request.objects_ready and M.race_world_start_error(request.world,request.game)~="" then return end
    local skater=reskate.skater.get();local telemetry=M.telemetry()
    if request.sent and skater and not telemetry.teleport_pending then
        local delta=skater.position-request.position
        local heading=math.abs((math.deg(skater.heading)-request.yaw+180)%360-180)
        if math.sqrt(delta.x*delta.x+delta.z*delta.z)<1.25 and math.abs(delta.y)<3 and heading<12 then
            arrival=nil;if request.done then request.done() end;return
        end
    end
    if telemetry.teleport_pending or now<request.retry then return end
    request.retry=now+1000
    local ok,why=M.teleport_to(request.position,request.yaw)
    request.sent=request.sent or ok
    if not ok and why=="the host has turned teleporting off" then return failed("The host has turned teleporting off.") end
end
reskate.on("tick",function()
    local s,now=M.state(),M.now_ms()
    if restoring and not current(restoring) then restoring=nil;arrival=nil;s.route_restore_pending=false end
    travel_tick(now)
    local request=restoring;if not request then return end
    if request.stage=="native" and now>request.deadline then
        stop_restore("The SDK did not complete the flag update. Your current course was kept.")
    end
end)
reskate.on("level_unloading",function() arrival=nil;restoring=nil;M.state().route_restore_pending=false end)
return API
