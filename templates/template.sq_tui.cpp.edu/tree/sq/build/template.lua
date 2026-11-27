-- sq/build/template.lua: template.sq_tui.cpp.edu's build, for sqmake (the
-- same as template.sq_tui.cpp's): a character-cell program on squared-tui
-- (sq.tui) and squared-core's sq.term.
-- GENERATED: the template owns this file and replaces it on update;
-- build.lua, which includes it, is yours.
--
--   app = {name = "hello"}
--   include("sq/build/template.lua")
--
-- The settings (all but name optional):
--   app = {
--     name = "hello",         -- build/NAME
--     squared = nil,          -- which squared: a version ("1.3.0") of the
--                             -- SDK's, or a folder; else a squared/ folder
--                             -- beside build.lua, else the SDK's newest
--     optimise = true,        -- false: -O0 -g (sqmake --debug says so too)
--     exclude = {},           -- sources under sq_app/src not to build
--     flags = {},             -- your own compile flags
--     link = {},              -- and link flags
--   }
--
-- The verbs: `sqmake` builds build/NAME; `sqmake run` runs it; `sqmake test`
-- builds and runs each sq_app/test/*.cpp, linked with the app's modules
-- (sq_app/src/**/*.cppm, and the packages': sq.tui), never its main. The
-- compiler is $CXX, else clang++: clang and libc++ 19 or later, with
-- `import std;` (sqterm's is 23).
--
-- Kits and packages: every sq/build/kit_*.lua, then every
-- sq/build/package_*.lua (package.squared-tui's among them), returns what it
-- adds ({id, flags, link, srcs}), in the order of their names.

local a = app or error("sq/build/template.lua: set app = {name = ...} before including it")
local name = a.name or error("sq/build/template.lua: app.name")
local optimise = a.optimise ~= false and not mode.debug
local opt = optimise and "-O2" or {"-O0", "-g", "-fno-omit-frame-pointer"}

-- squared's modules are C++23 with `import std;`, so the program is too.
toolchain.cxxflags = {"-std=c++23", "-stdlib=libc++", opt}
toolchain.cflags = {opt}
toolchain.ldflags = {"-stdlib=libc++", "-pthread"}

-- Which squared: the SDK's, installed by version (pkg install squared:
-- $PREFIX/share/squared/<version>/squared, the newest unless app.squared
-- names one), so every workspace on the device shares one copy and one
-- build cache; or a copy of the workspace's own, a squared/ folder beside
-- build.lua (app.squared = "squared" or any path says so too).
local function version_less(x, y)
  local xs, ys = {}, {}
  for n in x:gmatch("%d+") do xs[#xs + 1] = tonumber(n) end
  for n in y:gmatch("%d+") do ys[#ys + 1] = tonumber(n) end
  for i = 1, math.max(#xs, #ys) do
    if (xs[i] or 0) ~= (ys[i] or 0) then return (xs[i] or 0) < (ys[i] or 0) end
  end
  return false
end
local function installed_squared(version)
  local base = (os.getenv("PREFIX") or "") .. "/share/squared"
  if version then
    local root = base .. "/" .. version .. "/squared"
    if #glob(root .. "/build/squared.lua") == 0 then
      error("sq/build/template.lua: squared " .. version .. " is not installed (" .. root .. "): pkg install squared", 0)
    end
    return root
  end
  local newest
  for _, f in ipairs(glob(base .. "/*/squared/build/squared.lua")) do
    local v = f:sub(#base + 2):match("^([^/]+)/")
    if v and (not newest or version_less(newest, v)) then newest = v end
  end
  if not newest then
    error("sq/build/template.lua: no squared here: pkg install squared (or a squared/ folder of the workspace's own)", 0)
  end
  return base .. "/" .. newest .. "/squared"
end
local sq
if a.squared and not a.squared:match("^%d") then
  sq = a.squared
elseif not a.squared and #glob("squared/build/squared.lua") > 0 then
  sq = "squared"
else
  sq = installed_squared(a.squared)
end
if #glob(sq .. "/keys/module.cppm") == 0 then
  error("sq/build/template.lua: " .. sq .. " has no sq.keys: squared-tui needs squared-core 1.3.0 or later", 0)
end

-- squared for this machine: no graphics, no Android sources. Only what the
-- program links is built: sq.term and sq.keys.
squared_settings = {root = sq, platform = "host", graphics = "null", debug = not optimise}
local squared = include(sq .. "/build/squared.lua")

-- What the kits and packages add.
local kit_flags, kit_link, kit_srcs = {}, {}, {}
local parts = glob("sq/build/kit_*.lua")
for _, f in ipairs(glob("sq/build/package_*.lua")) do parts[#parts + 1] = f end
for _, file in ipairs(parts) do
  local k = include(file)
  if type(k) ~= "table" then error(file .. ": a kit's or package's build file returns {id, flags, link, srcs}", 0) end
  kit_flags[#kit_flags + 1] = k.flags or {}
  kit_link[#kit_link + 1] = k.link or {}
  for _, s in ipairs(k.srcs or {}) do kit_srcs[#kit_srcs + 1] = s end
end

local excluded = {}
for _, f in ipairs(a.exclude or {}) do excluded[f] = true end
-- The app's sources: its modules (*.cppm, ordered by what they import), built
-- once as a library the program and each test link (a test may import them),
-- and its *.cpp. The packages' modules (sq.tui) join them.
local modules, srcs = {}, {}
for _, f in ipairs(glob("sq_app/src/**/*.cppm")) do
  if not excluded[f] then modules[#modules + 1] = f end
end
for _, f in ipairs(glob("sq_app/src/**/*.cpp")) do
  if not excluded[f] then srcs[#srcs + 1] = f end
end
local pkg_srcs = {}
for _, f in ipairs(kit_srcs) do
  if f:match("%.cppm$") then modules[#modules + 1] = f else pkg_srcs[#pkg_srcs + 1] = f end
end
if #glob("sq/build/package_squared_tui.lua") == 0 then
  error("sq/build/template.lua: no package.squared-tui (sq/build/package_squared_tui.lua): make the workspace with it,"
        .. " sqpg new NAME --template TEMPLATE --package package.squared-tui", 0)
end

local flags = {"-Wall", "-Wextra", "-Wpedantic", kit_flags, a.flags or {}}
local link = {kit_link, a.link or {}}
-- The modules (the app's and sq.tui) and sq.tui's sources, one library, on
-- squared's sq.term (which brings sq.keys).
for _, f in ipairs(pkg_srcs) do modules[#modules + 1] = f end
local modules_lib = library{name = name .. "_modules", srcs = modules, deps = {squared.term},
                            flags = flags, default = false}
local bin = exe{name = name, srcs = srcs, deps = {modules_lib}, flags = flags, link = link}

verb("run", {needs = {bin}, call = function() return exec{bin} end})

-- Each test: one program from one source, with the modules (not the app's main).
local tests = {}
for _, src in ipairs(glob("sq_app/test/*.cpp")) do
  local t = src:match("([^/]+)%.cpp$")
  tests[#tests + 1] = exe{name = "test/" .. t, srcs = {src}, deps = {modules_lib}, flags = flags, link = link,
                          default = false}
end
verb("test", {needs = tests, call = function()
  if #tests == 0 then
    print("no tests: add .cpp files under sq_app/test/")
    return 0
  end
  local failed = 0
  for _, t in ipairs(tests) do
    if exec{t} ~= 0 then failed = failed + 1 end
  end
  print(failed == 0 and "tests passed" or (failed .. " test(s) failed"))
  return failed == 0 and 0 or 1
end})

return bin
