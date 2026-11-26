-- build/squared.lua: squared's own libraries, for sqmake (0.6.0 or later:
-- squared's headers import its modules, which 0.6.0 reads).
-- A workspace's build includes it, after toolchain.sdk(), and links what it
-- returns:
--
--   toolchain.sdk("squared/build/toolchain.lua")
--   local squared = include("squared/build/squared.lua")
--   shlib{name = "game", srcs = glob("sq_app/src/*.cpp"), deps = {squared.all}}
--
-- The same libraries make's build makes (Makefile, build/common.mk, each
-- module's Makefile), from the same sources: each module's interfaces
-- (*.cppm, not its tests') and src/**/*.cpp, the graphics backend's folder,
-- the Android-only sources on Android, and the C code squared carries
-- (yyjson, stb_image, FreeType) compiled as C with squared's warnings off.
-- Module order is sqmake's (it reads each source's import lines); the
-- dependencies below give each link its order.
--
-- Settings, from the including file (all optional):
--   squared_settings = {
--     root = "squared",       -- this folder, from the build file's
--     platform = "android",   -- or "host": no Android-only sources (a
--                             -- build of squared's tools, such as sqconsole,
--                             -- for this machine)
--     graphics = "gles",      -- or "null"; on Android, gles by default
--     debug = false,          -- -O0 -g instead of -O2 -DNDEBUG
--     flags = {},             -- what the app and squared must agree on: the
--                             -- Telegraph's and the Console's sizes
--                             -- (-DSQUARED_TELEGRAPH_RING=..., -DSQUARED_CONSOLE=1)
--   }
--
-- Every library gives its users the include root (<squared/...>) and those
-- shared flags; the modules themselves reach them through sqmake's module
-- path.

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
-- -fPIC: the libraries go into the app's shared library.
local codegen = {"-fno-common", "-ffunction-sections", "-fdata-sections", "-fvisibility=hidden", "-fPIC"}
local shared = {"-I" .. public_root, s.flags or {}}
local cxx = {optimise, warnings, codegen, shared, "-Xclang", "-fmodules-embed-all-files"}
-- Third-party C: its own warnings are not squared's to keep.
local c = {"-x", "c", "-std=c11", optimise, codegen, "-w"}

local function without_tests(files)
  local out = {}
  for _, f in ipairs(files) do
    if not f:find("/test/", 1, true) then out[#out + 1] = f end
  end
  return out
end
-- A module's sources: everything under src/ (src/common/, src/detail/ and
-- the like included), but the backends' folders (src/gles/, src/null/),
-- which the backend chosen adds back, and tests.
local function sources(dir)
  local out = {}
  for _, f in ipairs(without_tests(glob(dir .. "/src/**/*.cpp"))) do
    if not f:find("/src/gles/", 1, true) and not f:find("/src/null/", 1, true) then out[#out + 1] = f end
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
-- zstd: squared's one copy (sq.data's compress and decompress, sq.cart's
-- cartridges); what else wants it links this library, not a copy of its own.
local zstd = library{name = "squared_zstd", srcs = {tp .. "/zstd/zstd.c"},
                     flags = {c, "-DZSTD_MULTITHREAD=0"}, public = {flags = {"-I" .. tp .. "/zstd"}}}
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
  local srcs = join(without_tests(glob(dir .. "/**/*.cppm")), sources(dir), extra or {})
  local use = {}
  for _, d in ipairs(deps) do use[#use + 1] = libs[d] or d end
  libs[name] = library{name = "squared_" .. name, srcs = srcs, deps = use,
                       flags = {cxx, (more or {}).flags or {}, "-I" .. dir .. "/src"},
                       public = {flags = shared}}
  return libs[name]
end
local function only(cond, files) return cond and files or {} end

-- The order is make's (Makefile): what a module depends on comes first.
module("math", {})
module("data", {yyjson, zstd})
libs.zstd = zstd
module("time", {})
module("dev", {})
local files_srcs = {}
for _, f in ipairs(sources(root .. "/files")) do
  if platform == "android" or not f:match("/android_[^/]*$") then files_srcs[#files_srcs + 1] = f end
end
libs.files = library{name = "squared_files",
                     srcs = join(without_tests(glob(root .. "/files/**/*.cppm")), files_srcs),
                     flags = {cxx, "-I" .. root .. "/files/src"}, public = {flags = shared}}
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
module("tui", {"term", "app"})
module("font", {freetype}, {}, {flags = freetype_includes})
-- sq.drive and sq.cart (third_party/sqdrive, third_party/sqcart:
-- tools/vendor_sqdrive.sh), for the preferences' export and import: sq.cart
-- with squared's own sq.data and zstd, so there is one of each.
-- Its libraries take squared's optimisation and code generation (-fPIC: they
-- go into the app's shared library).
sqcart_settings = {root = tp .. "/sqcart/v3", data = libs.data, zstd = zstd, flags = {optimise, codegen}}
local cart = include(tp .. "/sqcart/v3/build.lua")
local drive = library{name = "squared_sqdrive", srcs = {tp .. "/sqdrive/src/drive.cppm"}, deps = {cart.write},
                      flags = {cxx, "-w"}}
-- An app's preferences, one cartridge: `import sq.preferences;` (not in the
-- umbrella's import, nor linked with it, since it brings sq.cart and zstd:
-- an app that keeps preferences links squared.preferences, or
-- squared.settings, which brings it).
module("preferences", {"files", "data", drive})
-- Dialogs any app shows over its screen (the file dialog): `import
-- sq.dialogs;` (not in the umbrella's import, nor linked with it).
module("dialogs", {"stage", "gui", "files"})
-- Every app's Settings screen, bound to its preferences: `import sq.settings;`
-- (not in the umbrella's import, nor linked with it: an app that has the
-- screen links squared.settings, which brings squared.dialogs).
module("settings", {"stage", "gui", "preferences", "data", "files", "dialogs"})

-- The umbrella: `import squared;`, and every library in one dependency.
local every = {}
for _, name in ipairs({"stage", "console", "term", "font", "gui", "messaging", "time", "app",
                       "gles", "assets", "graphics", "files", "dev", "data", "math"}) do
  if libs[name] then every[#every + 1] = libs[name] end
end
libs.all = library{name = "squared", srcs = {root .. "/squared.cppm"}, deps = every, flags = cxx,
                   public = {flags = shared}}
return libs
