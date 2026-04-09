local Signal = {}

local listeners = {}

local function ensure(name)
    if listeners[name] == nil then
        listeners[name] = {}
    end
end

function Signal.Connect(name, fn)
    assert(type(name) == "string", "signal name must be a string")
    assert(type(fn)   == "function", "signal callback must be a function")
    ensure(name)
    table.insert(listeners[name], { fn = fn, once = false })
end

function Signal.Once(name, fn)
    assert(type(name) == "string", "signal name must be a string")
    assert(type(fn)   == "function", "signal callback must be a function")
    ensure(name)
    table.insert(listeners[name], { fn = fn, once = true })
end

function Signal.Disconnect(name, fn)
    assert(type(name) == "string", "signal name must be a string")
    assert(type(fn)   == "function", "signal callback must be a function")
    if listeners[name] == nil then return end
    for i, listener in ipairs(listeners[name]) do
        if listener.fn == fn then
            table.remove(listeners[name], i)
            return
        end
    end
end

function Signal.Wait(name)
    assert(type(name) == "string", "signal name must be a string")
    local thread = coroutine.running()
    assert(thread ~= nil, "Signal.Wait must be called inside a coroutine")
    ensure(name)
    table.insert(listeners[name], {
        fn   = function(...) coroutine.resume(thread, ...) end,
        once = true
    })
    return coroutine.yield()
end

function Signal.Fire(name, ...)
    assert(type(name) == "string", "signal name must be a string")
    if listeners[name] == nil then return end
    local arguments    = { ... }
    local active  = {}
    for _, listener in ipairs(listeners[name]) do
        if not listener.once then
            table.insert(active, listener)
        end
        local ok, error = pcall(listener.fn, table.unpack(arguments))
        if not ok then
            print("signal error [" .. name .. "]: " .. tostring(error))
        end
    end
    listeners[name] = active
end

return Signal