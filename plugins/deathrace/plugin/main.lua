-- SmokeyDawg58's ReSkate Throwdowns as a plugin mod, on ReSkate's throwdown kit. His game layer is
-- this plugin: td/ is his game_modes.cpp in Lua, td/oneup/ his 1-UP runtime, td/world.lua his
-- race_world.cpp (gates, cones, the beacon), td/h/ his helper headers, td/routes.lua his route
-- library, all line for line. His native UI's mode branches are td/ui*.lua: they turn his game layer
-- into the kit's documents (the setup page, the waiting card, the HUD, the match panel, his flags and
-- what a match asks of the game), which ReSkate's general native UI draws with his widgets, and turn
-- what the player does there (the `throwdown_ui` events) into his commands.
local modes = require("modes")
-- His game layer is about 470 KB of Lua: compiling it all in one call would pass a call's 50 ms, so
-- it loads one part a tick (the modes are on the page at once; a game starts only after).
local parts = {"td/skatepedia", "td/rules", "td/h/race_route", "td/h/race_editor", "td/h/race_arrow_placement", "td/h/race_momentum", "td/h/race_sound",
               "td/h/mode_announcements", "td/h/mode_lobby", "td/h/mode_participation", "td/h/placement_camera", "td/h/race_navigation",
               "td/h/race_group_reconciliation", "td/h/race_asset_contract", "td/routes", "td/referee", "td/wire", "td/core", "td/skater", "td/lobby",
               "td/placing", "td/play", "td/hud", "td/command", "td/tick", "td/world", "td/ui", "td/ui_hud", "td/ui_events",
               "td/sdk", "td/presentation", "td/route_screen"}
local loaded = 0
local M, ready -- Deathrace shares the supplied rule helpers, without starting other modes.
local function load_next()
    loaded += 1
    local part = parts[loaded]
    local result = require(part)
    if part == "td/core" then M = result end
    if loaded == #parts then
        -- Keep the shared helpers, but expose only Deathrace through every entry point.
        local parse_mode=M.parse_mode
        M.parse_mode=function(word) return parse_mode(word)=="race" and "race" or nil end
        M.help=function() return "Deathrace: open its native Throwdowns card. Console: tdm new race, setup, time <seconds>, players <count>, start, status, stop or leave." end
        local command=M.command
        M.command=function(verb,args)
            if verb=="new" and M.parse_mode(args and args[1])~="race" then return "error: this mod contains Deathrace only." end
            if verb=="bonecam" or verb=="stance" or verb=="flicktest" then return "error: this mod contains Deathrace only." end
            return command(verb,args)
        end
        ready=true
        reskate.log("Deathrace is ready")
        reskate.log("Deathrace saved flag API: "..(M.sdk.automatic_flag_restore() and "native relocation available" or "unavailable; existing flag is preserved"))
    end
end

-- Add the Deathrace card without replacing any stock card.
for _, mode in ipairs(modes) do
    if mode.id ~= "race" then continue end
    local ok, why = reskate.throwdowns.mode({id = mode.id, name = mode.name, description = mode.description, art = mode.art, color = mode.color,
                                             card = mode.card, replaces = mode.replaces, order = mode.order, flag = mode.flag, players = mode.players,
                                             cursor_height = mode.cursor_height})
    if not ok then reskate.log("could not add " .. mode.name .. ": " .. tostring(why), "warn") end
end

-- ---- the session ------------------------------------------------------------------------------------
-- His session tick's input (game_modes.h SessionInput).
local function session_input()
    local active = reskate.session.active()
    local me = active and reskate.session.me() or nil
    local peers = {}
    if active then
        for _, player in ipairs(reskate.session.players()) do
            if not player.me then table.insert(peers, {id = tostring(player.id), name = player.name}) end
        end
    end
    local world = reskate.session.world()
    return {["local"] = me and tostring(me.id) or "0", local_name = me and me.name or "", peers = peers,
            in_world = reskate.level.current() ~= nil, world = world.world, barred = world.barred, map = world.map}
end

-- What goes out: his packets, in order, within a plugin's 16 KB a second.
local outgoing, second, spent, retry_at = {}, 0, 0, 0
local function flush(now)
    if now - second >= 1000 then
        second = now
        spent = 0
    end
    if now<retry_at then return end
    while #outgoing > 0 and spent + #outgoing[1] + 14 <= 15000 do
        local packet = outgoing[1]
        spent += #packet + 14 -- Failed attempts still consume the host's budget.
        local called, accepted = pcall(reskate.net.send, "all", packet)
        if not called or not accepted then
            retry_at=now+250
            reskate.log("Deathrace network send was refused; keeping the packet for retry.","warn")
            break
        end
        table.remove(outgoing,1)
    end
    if #outgoing>256 then
        table.clear(outgoing)
        M.end_game(M.state(),"Network transfer could not keep up; the challenge was stopped safely.")
        reskate.ui.toast("Deathrace transfer failed. Please reconnect and try again.","warn")
    end
end

-- His UI's documents for the kit, every tick (the SDK compares them with the last).
local refused = nil
local publish_at = 0
local function publish(now)
    local s = M.state()
    M.route_dialog_tick(s)
    -- Native documents persist and their clocks interpolate. Rebuilding/parsing them
    -- at render frequency wastes the host's budget; input and the overlay still tick normally.
    if now < publish_at then return end
    publish_at = now + 50
    local ok, done, why = pcall(function() return reskate.throwdowns.ui(M.build_ui(s, now)) end)
    local refusal = not ok and tostring(done) or (done == false and tostring(why) or nil)
    if refusal ~= refused then
        refused = refusal
        if refusal then reskate.log("Game modes: the documents were not taken: " .. refusal, "warn") end
    end
end

local rules_at = 0
reskate.on("tick", function()
    if not ready then return load_next() end
    local now = M.now_ms()
    local library=M.state().route_library
    if library then library:poll() end
    -- Simulation uses monotonic millisecond deadlines and swept gate crossings.
    -- Poll its Lua bookkeeping at 30 Hz; native physics/VFX and overlay drawing
    -- keep their normal frame rate, and route-menu input is sampled every tick.
    if now >= rules_at then
        rules_at = now + 1000 / 30 - .1
        for _, packet in ipairs(M.tick(session_input())) do table.insert(outgoing, packet) end
    end
    M.ui_tick(M.state(), now)
    flush(now)
    for _, text in ipairs(M.take_notices()) do reskate.chat.notice(text) end
    publish(now)
end)

reskate.on("message", function(from, payload)
    if not ready then return end
    local sender = tostring(from.id)
    if M.receive(sender, payload) then return end
end)

-- What the player does in the native Throwdowns UI (the kit's events), and the trick list's writes.
reskate.on("throwdown_ui", function(e)
    if not ready then
        -- Still loading: a card opened now is refused, as his UI refused a busy setup.
        if e.action == "open" then reskate.throwdowns.flag.leave() end
        return
    end
    -- Native stock cards and any other plugin's modes do not belong to this mod.
    if e.mode~="race" then return end
    local ok, why = pcall(M.on_throwdown_ui, e)
    rules_at = 0
    publish_at = 0 -- Native setup/join actions get a fresh document on the next tick.
    if not ok then reskate.log("Game modes: " .. tostring(e.action) .. " failed: " .. tostring(why), "error") end
end)
-- His builder's gate numbers and rotation gizmo, over the world.
reskate.on("draw", function(d) if ready then M.draw(d) end end)
-- Deathrace settings page in the plugin menu.
reskate.menu.page("Deathrace", function(ui)
    ui:card("DEATHRACE", "Course builder, saved routes and races")
    ui:note("Open the game's Throwdowns page to set up a challenge.")
    if M and M.sdk then
        ui:info("Plugin API", tostring(reskate.api_version))
        ui:info("Saved start load", M.sdk.automatic_flag_restore() and "Native repositioning" or "Requires updated Lua flag API")
    end
end)

-- His `mode` console verbs, under "tdm" (ReSkate's own "mode" is the game's).
reskate.console.command("tdm", "Deathrace: tdm <verb> [arguments] (tdm help)", function(args)
    if not ready then return reskate.console.print("tdm: Deathrace is still loading") end
    local verb = args[1] or "help"
    local rest = {}
    for i = 2, #args do rest[i - 1] = args[i] end
    reskate.console.print(M.command(verb, rest))
end)
