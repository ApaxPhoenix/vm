local modules = {}

local env = {
    ipairs=ipairs, pairs=pairs, next=next,
    select=select, type=type, tostring=tostring,
    tonumber=tonumber, unpack=table.unpack,
    error=error, assert=assert,
    pcall=pcall, xpcall=xpcall, print=print,
    math=math, string=string, table=table,
    coroutine=coroutine, io=io,
    os = { time=os.time, clock=os.clock, date=os.date },
    require = function(name)
        local module = modules[name]
        if module == nil then error("module not found: " .. tostring(name), 2) end
        return module
    end,
}

local function isolate()
    env._G = env
    return env
end

return isolate