-- sq/build/template.lua: template.term.edu.cpp's build, for sqmake: the
-- program the Makefile builds (`make`, `make run`, `make test`), with no
-- make. GENERATED: the template owns this file and replaces it on update;
-- build.lua, which includes it, is yours.
--
--   app = {name = "hello"}
--   include("sq/build/template.lua")
--
-- The settings (all but name optional):
--   app = {
--     name = "hello",         -- build/NAME
--     std = "c++20",          -- the language standard
--     optimise = true,        -- false: -O0 -g (sqmake --debug says so too)
--     exclude = {},           -- sources under sq_app/src not to build
--     flags = {},             -- your own compile flags
--     link = {},              -- and link flags
--   }
--
-- The verbs: `sqmake` builds build/NAME; `sqmake run` runs it; `sqmake test`
-- builds and runs each sq_app/test/*.cpp, linked with the app's modules
-- (sq_app/src/**/*.cppm; never its main). The compiler is $CXX, else
-- clang++ (sqmake's toolchain).
--
-- Kits and packages: every sq/build/kit_*.lua (a kit's, generated), then
-- every sq/build/package_*.lua (a package's), returns what it adds ({id,
-- flags, link, srcs}); they are added in the order of their names.

local a = app or error("sq/build/template.lua: set app = {name = ...} before including it")
local name = a.name or error("sq/build/template.lua: app.name")
local optimise = a.optimise ~= false and not mode.debug

toolchain.cxxflags = {"-std=" .. (a.std or "c++20"), optimise and "-O2" or {"-O0", "-g", "-fno-omit-frame-pointer"}}

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
-- and its *.cpp.
local modules, srcs = {}, {}
for _, f in ipairs(glob("sq_app/src/**/*.cppm")) do
  if not excluded[f] then modules[#modules + 1] = f end
end
for _, f in ipairs(glob("sq_app/src/**/*.cpp")) do
  if not excluded[f] then srcs[#srcs + 1] = f end
end
-- A kit's or package's modules join the app's (a test may import them too).
for _, f in ipairs(kit_srcs) do
  if f:match("%.cppm$") then modules[#modules + 1] = f else srcs[#srcs + 1] = f end
end

local flags = {"-Isq_app/include", "-Wall", "-Wextra", "-Wpedantic", kit_flags, a.flags or {}}
local link = {kit_link, a.link or {}}
local deps = {}
if #modules > 0 then
  deps = {library{name = name .. "_modules", srcs = modules, flags = flags, default = false}}
end
local bin = exe{name = name, srcs = srcs, deps = deps, flags = flags, link = link}

verb("run", {needs = {bin}, call = function() return exec{bin} end})

-- Each test: one program from one source, with the kits (not the app's main).
local tests = {}
for _, src in ipairs(glob("sq_app/test/*.cpp")) do
  local t = src:match("([^/]+)%.cpp$")
  tests[#tests + 1] = exe{name = "test/" .. t, srcs = {src}, deps = deps, flags = flags, link = link, default = false}
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
