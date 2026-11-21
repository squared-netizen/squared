-- build/squared.lua: squared's own libraries, for sqmake (0.5.0 or later).
-- A workspace's build includes it, after toolchain.sdk(), and links what it
-- returns:
--
--   toolchain.sdk("squared/build/toolchain.lua")
--   local squared = include("squared/build/squared.lua")
--   shlib{name = "game", srcs = glob("sq_app/src/*.cpp"), deps = {squared.all}}
--
-- The same libraries make's build makes (Makefile, build/common.mk, each
-- module's Makefile), from the same sources: each module's interfaces
-- (*.cppm, not its tests') and src/*.cpp, the graphics backend's folder,
-- the Android-only sources on Android, and the C code squared carries
-- (yyjson, stb_image, FreeType) compiled as C with squared's warnings off.
-- Module order is sqmake's (it reads each source's import lines); the
-- dependencies below give each link its order.
--
-- Settings, from the including file (all optional):
--   squared_settings = {
--     root = "squared",       -- this folder, from the build file's
--     platform = "android",   -- or "host": no Android-only sources
--     graphics = "gles",      -- or "null"; on Android, gles by default
--     debug = false,          -- -O0 -g instead of -O2 -DNDEBUG
--   }

local s = squared_settings or {}
local root = s.root or "squared"
local platform = s.platform or "android"
local graphics = s.graphics or (platform == "android" and "gles" or "null")

-- Headers are included as <squared/...>: the folder above squared/.
local public_root = root:match("^(.*)/[^/]+$") or "."

-- build/common.mk's flags (the toolchain gives the standard, the target and
-- the rest that changes a compiled module).
local optimise = s.debug and {"-O0", "-g"} or {"-O2", "-DNDEBUG"}
local warnings = {"-Wall", "-Wextra", "-Wpedantic", "-Wshadow", "-Wconversion", "-Wsign-conversion"}
local codegen = {"-fno-common", "-ffunction-sections", "-fdata-sections", "-fvisibility=hidden"}
local cxx = {optimise, warnings, codegen, "-I" .. public_root, "-Xclang", "-fmodules-embed-all-files"}
-- Third-party C: its own warnings are not squared's to keep.
local c = {"-x", "c", "-std=c11", optimise, codegen, "-w"}

local function without_tests(files)
  local out = {}
  for _, f in ipairs(files) do
    if not f:find("/test/", 1, true) then out[#out + 1] = f end
  end
  return out
end
local function join(...)
  local out = {}
  for _, list in ipairs({...}) do
    for _, f in ipairs(list) do out[#out + 1] = f end
  end
  return out
end

-- The C squared carries, a library each.
local tp = root .. "/third_party"
local yyjson = library{name = "squared_yyjson", srcs = {tp .. "/yyjson/yyjson.c"}, flags = c,
                       public = {flags = {"-I" .. tp .. "/yyjson"}}}
local stb = library{name = "squared_stb", srcs = {tp .. "/stb/stb_image_impl.c"}, flags = c,
                    public = {flags = {"-I" .. tp .. "/stb"}}}
local freetype_includes = {"-I" .. root .. "/font/src/freetype", "-I" .. tp .. "/freetype/include"}
local freetype_sources = {}
for _, f in ipairs({"base/ftbase.c", "base/ftinit.c", "base/ftsystem.c", "base/ftdebug.c",
                    "base/ftbitmap.c", "base/ftsynth.c", "base/ftglyph.c", "base/ftmm.c",
                    "truetype/truetype.c", "sfnt/sfnt.c", "smooth/smooth.c", "autofit/autofit.c",
                    "cff/cff.c", "psaux/psaux.c", "pshinter/pshinter.c", "psnames/psnames.c"}) do
  freetype_sources[#freetype_sources + 1] = tp .. "/freetype/src/" .. f
end
local freetype = library{name = "squared_freetype", srcs = freetype_sources,
                         flags = {c, "-DFT2_BUILD_LIBRARY"}, public = {flags = freetype_includes}}

-- One module: its interfaces, src/*.cpp, and what else it names.
local libs = {}
local function module(name, deps, extra, more)
  local dir = root .. "/" .. name
  local srcs = join(without_tests(glob(dir .. "/**/*.cppm")), glob(dir .. "/src/*.cpp"), extra or {})
  local use = {}
  for _, d in ipairs(deps) do use[#use + 1] = libs[d] or d end
  libs[name] = library{name = "squared_" .. name, srcs = srcs, deps = use,
                       flags = {cxx, (more or {}).flags or {}, "-I" .. dir .. "/src"},
                       public = (more or {}).public}
  return libs[name]
end
local function only(cond, files) return cond and files or {} end

-- The order is make's (Makefile): what a module depends on comes first.
module("math", {})
module("data", {yyjson})
module("time", {})
module("dev", {})
local files_srcs = {}
for _, f in ipairs(glob(root .. "/files/src/*.cpp")) do
  if platform == "android" or not f:match("/android_[^/]*$") then files_srcs[#files_srcs + 1] = f end
end
libs.files = library{name = "squared_files",
                     srcs = join(without_tests(glob(root .. "/files/**/*.cppm")), files_srcs),
                     flags = {cxx, "-I" .. root .. "/files/src"}}
module("assets", {"files"})
module("graphics", {"dev", "math", "files", stb}, glob(root .. "/graphics/src/" .. graphics .. "/*.cpp"))
if graphics == "gles" then module("gles", {"graphics"}) end
module("app", {"graphics", "files", "assets"})
module("messaging", {"time"})
module("gui", {"app", "graphics", "math", "data", "dev", "files", "messaging"})
module("console", {"app", "dev", "messaging", "time"},
       only(graphics == "gles", glob(root .. "/console/src/gles/*.cpp")))
module("stage", {"app", "gui", "graphics", "assets", "dev", "files", "messaging", "data", "time", "math"})
module("term", {"app"})
module("font", {freetype}, {}, {flags = freetype_includes})

-- The umbrella: `import squared;`, and every library in one dependency.
local every = {}
for _, name in ipairs({"stage", "console", "term", "font", "gui", "messaging", "time", "app",
                       "gles", "assets", "graphics", "files", "dev", "data", "math"}) do
  if libs[name] then every[#every + 1] = libs[name] end
end
libs.all = library{name = "squared", srcs = {root .. "/squared.cppm"}, deps = every, flags = cxx}
return libs
