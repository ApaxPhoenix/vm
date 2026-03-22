local whitelist = {
    print = print,
    tostring = tostring,
    tonumber = tonumber,
    ipairs = ipairs,
    pairs = pairs,
    type = type,
    error = error,
    assert = assert,
    pcall = pcall,
    xpcall = xpcall,
    select = select,
    unpack = unpack,
    next = next,
    math = math,
    string = string,
    table = table,
    coroutine = coroutine,
    _G = _G,
}

for k in next, _G do
    if whitelist[k] == nil then
        _G[k] = nil
    end
end

whitelist._G = nil
_ENV = whitelist