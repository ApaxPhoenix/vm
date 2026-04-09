local modules = {}

local env = {
    ipairs=ipairs, pairs=pairs, next=next,
    select=select, type=type, tostring=tostring,
    tonumber=tonumber, unpack=table.unpack,
    error=error, assert=assert,
    pcall=pcall, xpcall=xpcall, print=print,
    math=math, string=string, table=table,
    coroutine=coroutine,
    os = { time=os.time, clock=os.clock },
    require = function(name)
        local module = modules[name]
        if module == nil then error("module not found: " .. tostring(name), 2) end
        return module
    end,
}

local function isolate()
    env._G = env
    setmetatable(env, {
        __index     = function(_, k) error("'" .. tostring(k) .. "' is not accessible", 2) end,
        __newindex  = function(_, k, _) error("'" .. tostring(k) .. "' cannot be defined", 2) end,
        __metatable = "restricted",
    })
    return env
end

return isolate