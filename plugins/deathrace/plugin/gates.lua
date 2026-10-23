-- The exact native effect roots from the confirmed 1.0.27 build. The general SDK
-- owns their typed lifecycle, transforms, scene depth, animation and cleanup.
local gates = {}
gates.assets={"World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Gate","World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Finish","World/ReSkate/Deathrace/SM_FX_GP_Checkpoint_A_AT_Guide","World/ReSkate/Deathrace/FloatingArrow_AT"}
function gates.supported() return type(reskate.entities.info)=="function" end
function gates.ready(id,arrow)
    if not gates.supported() then return false end
    local info=reskate.entities.info(id)
    local count=arrow and 3 or 1
    return info~=nil and info.status=="ready" and info.kind=="effect" and info.applied and info.playing
        and info.children==count and info.ready_mesh_emitters==count
end
local rad=math.pi/180
function gates.axes(yaw,pitch,roll)
    local y,p,r=(yaw or 0)*rad,(pitch or 0)*rad,(roll or 0)*rad
    local cy,sy,cp,sp,cr,sr=math.cos(y),math.sin(y),math.cos(p),math.sin(p),math.cos(r),math.sin(r)
    local right,up,forward={cy,0,-sy},{sy*sp,cp,cy*sp},{sy*cp,-sp,cy*cp}
    local a,b={},{}
    for i=1,3 do a[i]=right[i]*cr+up[i]*sr;b[i]=-right[i]*sr+up[i]*cr end
    return a,b,forward
end
function gates.mesh_pose(gate,part)
    assert(part==0,"a native gate has one spatial parent")
    local right,up,forward=gates.axes(gate.yaw,gate.pitch,gate.arrow and gate.roll or 0)
    local scale=gate.width
    local m={}
    for i=1,3 do m[i]=right[i]*scale;m[i+4]=up[i]*scale;m[i+8]=forward[i]*scale;m[i+12]=gate.at[i] end
    m[4],m[8],m[12],m[16]=0,0,0,1
    return m
end
function gates.asset_of(gate,part)
    assert(part==0,"a native gate has one spatial parent")
    return gates.assets[gate.arrow and 4 or gate.finish and 2 or gate.guiding and 3 or 1]
end
function gates.parts(gate) return gate.hidden and 0 or 1 end

-- Every gate of the list in the world, under entity ids from `base` (8 per gate); the gates that are
-- no longer there are taken away. `shown` remembers what was published between calls.
function gates.publish(shown, base, list)
    if not gates.supported() then return false,"This SDK does not support native effects." end
    shown.count = shown.count or 0
    shown.poses = shown.poses or {}
    for g, gate in ipairs(list) do
        for part = 0, 3 do
            local id = base + (g - 1) * 8 + part
            if part < gates.parts(gate) then
                local asset, pose = gates.asset_of(gate, part), gates.mesh_pose(gate, part)
                local key = asset .. table.concat(pose, ",")
                if shown.poses[id] ~= key then
                    shown.poses[id] = key
                    local ok, why = reskate.entities.set(id, asset, {matrix = pose})
                    if not ok then reskate.log("Deathrace: a gate could not be placed: " .. tostring(why)) end
                end
            elseif shown.poses[id] then
                shown.poses[id] = nil
                reskate.entities.remove(id)
            end
        end
    end
    for g = #list + 1, shown.count do
        for part = 0, 3 do
            local id = base + (g - 1) * 8 + part
            if shown.poses[id] then
                shown.poses[id] = nil
                reskate.entities.remove(id)
            end
        end
    end
    shown.count = #list
end
function gates.clear(shown, base)
    gates.publish(shown, base, {})
end

return gates
