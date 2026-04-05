local modules = {
    ["core.math"] = {
        Vector2    = Vector2,
        Vector3    = Vector3,
        Vector4    = Vector4,
        Quaternion = Quaternion,
        Matrix3    = Matrix3,
        Matrix4    = Matrix4,
        Transform  = Transform,
        Color3     = Color3,
        Color4     = Color4,
    },
}

local base = {
    ipairs=ipairs, pairs=pairs, next=next,
    select=select, type=type, tostring=tostring,
    tonumber=tonumber, unpack=table.unpack,
    error=error, assert=assert,
    pcall=pcall, xpcall=xpcall, print=print,
    math=math, string=string,
    table=table, coroutine=coroutine,
    io=io, os = { time=os.time, clock=os.clock, date=os.date },
}

local function isolate()
    base._G = base
    base.require = function(name)
        local module = modules[name]
        if module == nil then error("module not found: " .. tostring(name), 2) end
        return module
    end

    setmetatable(base, {
        __index     = function(_, k) error("'" .. tostring(k) .. "' is not accessible", 2) end,
        __newindex  = function(_, k) error("'" .. tostring(k) .. "' cannot be defined", 2) end,
        __metatable = "restricted",
    })

    return base
end

return isolate