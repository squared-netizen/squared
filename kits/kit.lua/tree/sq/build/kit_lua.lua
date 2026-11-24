-- sq/build/kit_lua.lua: kit.lua's part of the build, for sqmake.
-- GENERATED: kit.lua owns this file and replaces it on update.
--
-- Lua 5.4 is the system's (the manifest's external.acquisition: "system"),
-- so this finds it; a build file reads no shell, so not with pkg-config but
-- by looking, first hit wins:
--   1. app.lua = {flags = {...}, link = {...}} in build.lua: always respected
--   2. $PREFIX/include (lua5.4/ or lua.h) with liblua5.4 or liblua in $PREFIX/lib
--   3. /usr/include/lua5.4 with /usr/lib/liblua5.4.so or /usr/lib/*/ (Debian's)
-- Found: -DSQ_HAVE_LUA and its flags. Not found: the project still builds,
-- and lua_host.hpp compiles to a stub that says at run time what is missing.
local given = app and app.lua
local flags, link = {"-Isq_kit/include"}, {}
local function first(patterns)
  for _, p in ipairs(patterns) do
    local found = glob(p)
    if #found > 0 then return found[1] end
  end
end
if given then
  flags[#flags + 1] = {"-DSQ_HAVE_LUA", given.flags or {}}
  link = given.link or {}
else
  local prefix = os.getenv("PREFIX") or "/usr/local"
  local header = first({prefix .. "/include/lua5.4/lua.h", prefix .. "/include/lua.h", "/usr/include/lua5.4/lua.h"})
  local library = first({prefix .. "/lib/liblua5.4.so", prefix .. "/lib/liblua5.4.a", prefix .. "/lib/liblua.so",
                         prefix .. "/lib/liblua.a", "/usr/lib/liblua5.4.so", "/usr/lib/*/liblua5.4.so"})
  if header and library then
    flags[#flags + 1] = {"-DSQ_HAVE_LUA", "-I" .. header:match("^(.*)/lua%.h$")}
    link = {library}
  end
end
return {
  id = "kit.lua",
  flags = flags,
  link = link,
}
