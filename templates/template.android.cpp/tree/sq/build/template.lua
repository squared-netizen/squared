-- sq/build/template.lua: template.android.cpp's build, for sqmake (0.8.1 or
-- later): squared, the app's native library and its APK, with no shell;
-- sqmake's verbs
-- (install, run) and Android's own words (logcat, devices, uninstall) from
-- squared's Android platform (squared/build/android.lua). `sqmake --debug
-- VERB` is the debug build (below). A project's build.lua says what
-- is its own and includes this:
--
--   app = {name = "mygame"}
--   include("sq/build/template.lua")
--
-- The settings (all but name optional):
--   app = {
--     name = "mygame",            -- libNAME.so, build/NAME.apk
--     squared = nil,              -- which squared (below): a version
--                                 -- ("1.0.0"), or a folder of its own
--                                 -- ("squared": vendored, the expert's way)
--     min_sdk = 28,               -- the sysroot package's API level
--     target_sdk = 28,
--     background = false,         -- the background service (BACKGROUND=1)
--     debuggable = false,         -- a release build that run-as reaches
--     optimise = true,            -- false: -O0 -g (squared's too) instead of -O2
--     console = true,             -- the debug build's Console (CONSOLE=app)
--     dev_log = 5555,             -- and its log, to this UDP port (DEV_LOG=1)
--     exclude = {},               -- sources under sq_app/src not to build
--     flags = {},                 -- the app's own compile flags
--     link = {},                  -- and link flags
--     libraries = {},             -- squared's libraries outside squared.all
--                                 -- the app links, by name: "settings" (the
--                                 -- Settings screen; brings "dialogs" and
--                                 -- "preferences"), "dialogs", "preferences"
--     shared = {},                -- what the app and squared must agree on:
--                                 -- -DSQUARED_TELEGRAPH_...=, -DSQUARED_CONSOLE=1
--     key = nil,                  -- apkpack's key (~/.android/apkpack.pem)
--     package = nil,              -- the manifest's package: for run, uninstall
--     device = {"bash", "tools/device.sh"},
--                                 -- what talks to the device (build/android.lua)
--     sqgui_gen = "sqgui-gen",    -- for sq_app/names.json, sq_app/layouts/
--     sqconsole = "sqconsole",    -- `sqmake console`: a debug build's Console
--   }
--
-- Kits and packages add to it through their own sq/build/kit_*.lua and
-- sq/build/package_*.lua (below).
--
-- The template's layout: sq_app/src/**/*.cpp and *.cppm (the app: its own
-- modules too, ordered by what they import), sq_android/*.cpp
-- (the platform side), sq_app/third_party/**/*.c, sq_android/AndroidManifest.xml,
-- res/ and assets/. classes.dex is squared's shims (tools/dexgen/shims.dex,
-- committed): nothing is built to run during the build. An app with
-- sq_app/names.json or sq_app/layouts/ needs sqgui-gen, the SDK's (pkg
-- install sqgui).

local a = app or error("sq/build/template.lua: set app = {name = ...} before including it")
local name = a.name or error("sq/build/template.lua: app.name")
-- Which squared: the SDK's, installed by version (pkg install squared:
-- $PREFIX/share/squared/<version>/squared, the newest unless app.squared
-- names one), so every workspace on the device shares one copy and one
-- build cache. The escape hatch, for a workspace that changes squared
-- itself: a copy of its own, a squared/ folder beside build.lua (squared-pg
-- adds one when asked; app.squared = "squared" or any path says so too),
-- which is used whenever it is there.
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
  sq = a.squared                                  -- a folder: vendored
elseif not a.squared and #glob("squared/build/squared.lua") > 0 then
  sq = "squared"                                  -- a squared/ of its own
else
  sq = installed_squared(a.squared)               -- the SDK's, by version
end
local api = a.min_sdk or 28

-- POSIX cksum (what the Console's port is made from: the package's).
local function cksum(s)
  local t = {}
  for i = 0, 255 do
    local c = i << 24
    for _ = 1, 8 do
      if c & 0x80000000 ~= 0 then c = ((c << 1) ~ 0x04C11DB7) & 0xffffffff else c = (c << 1) & 0xffffffff end
    end
    t[i] = c
  end
  local crc = 0
  local function add(b) crc = ((crc << 8) & 0xffffffff) ~ t[((crc >> 24) ~ b) & 0xff] end
  for i = 1, #s do add(s:byte(i)) end
  local n = #s
  while n > 0 do add(n & 0xff); n = n >> 8 end
  return (~crc) & 0xffffffff
end

-- The debug build (`sqmake --debug`, in build/debug): a
-- CONSOLE=app DEV_LOG=1 build, debuggable so run-as reaches it. Its Console
-- and its log are the app's (squared's libraries are the same either way).
local debug_flags = {}
if mode.debug then
  if a.console ~= false then
    -- Its sizes are squared's own defaults (console/src/limits.cppm).
    debug_flags[#debug_flags + 1] = "-DSQUARED_CONSOLE=1"
    -- Its bridge, which sqconsole reaches (`sqmake console`): a port from
    -- the package and a secret made once, in build/dev/bridge.conf, the
    -- file the Console client reads (sqconsole: the same port, POSIX cksum).
    if a.package then
      local port = 20000 + cksum(a.package) % 20000
      local conf = "build/dev/bridge.conf"
      local text = read_file(conf) or ""
      local secret = text:match("secret=(%x+)")
      if not text:find("port=" .. port .. "\n", 1, true) or not secret or #secret ~= 32 then
        local bytes = read_file("/dev/urandom", 16)
        if not bytes or #bytes ~= 16 then error("sq/build/template.lua: no /dev/urandom for the Console's secret") end
        secret = bytes:gsub(".", function(c) return string.format("%02x", c:byte()) end)
        write_file(conf, string.format("port=%d\nsecret=%s\npackage=%s\n", port, secret, a.package))
      end
      debug_flags[#debug_flags + 1] = {"-DSQUARED_BRIDGE_PORT=" .. port,
                                       "-DSQUARED_BRIDGE_SECRET_HIGH=0x" .. secret:sub(1, 16) .. "ULL",
                                       "-DSQUARED_BRIDGE_SECRET_LOW=0x" .. secret:sub(17, 32) .. "ULL"}
    end
  end
  if a.dev_log ~= false then debug_flags[#debug_flags + 1] = "-DSQUARED_DEV_LOG=" .. tostring(a.dev_log or 5555) end
end
local optimise = a.optimise ~= false

local t = toolchain.sdk(sq .. "/build/toolchain.lua")
squared_settings = {root = sq, platform = "android", graphics = "gles", debug = not optimise, flags = a.shared or {}}
local squared = include(sq .. "/build/squared.lua")

-- The app's sources: its own, the platform side, the C it carries; and
-- native-app-glue (squared's copy), whose entry point nothing else names.
local excluded = {}
for _, f in ipairs(a.exclude or {}) do excluded[f] = true end
local srcs = {}
local function add(list)
  for _, f in ipairs(list) do
    if not excluded[f] then srcs[#srcs + 1] = f end
  end
end
add(glob("sq_app/src/**/*.cppm"))
add(glob("sq_app/src/**/*.cpp"))
add(glob("sq_android/*.cpp"))
add(glob("sq_app/third_party/**/*.c"))
local glue = sq .. "/third_party/native_app_glue"
-- Google's code, with its own warnings: compiled without the app's (squared's
-- build compiles it so too), and linked as an object, not from an archive.
local glue_obj = rules.cc(glue .. "/android_native_app_glue.c", {optimise and {"-O2"} or {"-O0", "-g"}, "-fPIC", "-w", "-I" .. glue})

-- Kits and packages: every sq/build/kit_*.lua (a kit's, generated;
-- kit.opengl's, say), then every sq/build/package_*.lua (a package's),
-- returns what it adds, {id, flags, link, srcs}, in the order of their
-- names: square adds and updates one by its file, never this one.
local kit_flags, kit_link = {}, {}
local parts = glob("sq/build/kit_*.lua")
for _, f in ipairs(glob("sq/build/package_*.lua")) do parts[#parts + 1] = f end
for _, file in ipairs(parts) do
  local k = include(file)
  if type(k) ~= "table" then error(file .. ": a kit's or package's build file returns {id, flags, link, srcs}", 0) end
  kit_flags[#kit_flags + 1] = k.flags or {}
  kit_link[#kit_link + 1] = k.link or {}
  add(k.srcs or {})
end

-- sqgui-gen (sqgui's; once squared's sqgen): sq_app/names.json and sq_app/layouts/*.json
-- into C++ (names.hpp, layouts.hpp, layouts.cpp in build/gen), for an app
-- with either; the app's sources include them, so every compile waits. An
-- app with neither builds without it. sqgui-gen is the SDK's, on PATH (or
-- app.sqgui_gen; app.sqgen, its old name, still says so), as sqedit is.
local gen_flags, after = {}, {}
local names = glob("sq_app/names.json")
local layouts = glob("sq_app/layouts/*.json")
if #names + #layouts > 0 then
  local gen = toolchain.build .. "/gen"
  -- Which files there are, so adding or removing one regenerates too.
  local inputs = {}
  for _, f in ipairs(names) do inputs[#inputs + 1] = f end
  for _, f in ipairs(layouts) do inputs[#inputs + 1] = f end
  local listed = write_file(gen .. "/inputs", table.concat(inputs, "\n") .. "\n")
  local cmd = {a.sqgui_gen or a.sqgen or "sqgui-gen"}
  if #names > 0 then cmd[#cmd + 1] = {"--names", names[1]} end
  if #layouts > 0 then cmd[#cmd + 1] = {"--layouts", "sq_app/layouts"} end
  cmd[#cmd + 1] = {"--out", gen}
  local cpp = gen .. "/layouts.cpp"
  -- touch: sqgui-gen leaves an unchanged file as it was.
  target{out = cpp, ins = {inputs, listed}, desc = "sqgui-gen", cmds = {cmd, {"touch", cpp}}}
  -- What layouts.cpp imports (sqgui-gen writes these three).
  srcs[#srcs + 1] = {cpp, imports = {"sq.gui", "sq.messaging", "std"}}
  gen_flags = {"-I" .. gen}
  after = {cpp}
end

-- FreeType's API (FT_EXPORT: default visibility) stays inside: it is
-- squared_font's, not the app's to export; nor is the C++ runtime's (the
-- runtimes package's libc++.a carries the unwinder too).
-- Android's libraries, named by path in the sysroot (a bare -lEGL could bind
-- another EGL, such as libglvnd's X11 one on a desktop sysroot).
local platform = t.sysroot .. "/usr/lib/aarch64-linux-android/" .. api
-- squared's libraries outside squared.all the app asked for (app.libraries),
-- before it: what they use is in it.
local deps = {}
for _, lib in ipairs(a.libraries or {}) do
  deps[#deps + 1] = squared[lib] or error("app.libraries: squared has no library \"" .. tostring(lib) .. "\"", 0)
end
deps[#deps + 1] = squared.all
local so = shlib{
  name = name,
  srcs = srcs,
  objs = {glue_obj},
  deps = deps,
  after = after,
  flags = {gen_flags, debug_flags, optimise and {"-O2"} or {"-O0", "-g"}, "-Wall", "-Wextra", "-Wpedantic", "-fPIC", "-fvisibility=hidden", "-DANDROID", "-I" .. glue, kit_flags, a.flags or {}},
  link = {"-Wl,--hash-style=both", "-Wl,--gc-sections", "-Wl,--no-undefined",
          "-Wl,--exclude-libs,libsquared_font.a",
          "-Wl,--exclude-libs,libsquared_freetype.a", "-Wl,--exclude-libs,libc++.a:libc++abi.a:libunwind.a", "-pthread", "-landroid", "-llog",
          platform .. "/libEGL.so", platform .. "/libGLESv3.so", kit_link, a.link or {}},
}

-- The APK's copy is stripped (--strip-unneeded keeps the dynamic symbols the
-- loader and dladdr() read); build/libNAME.so keeps its symbols for addr2line.
-- llvm-objcopy, which sqterm's llvm package has (llvm-strip it may not).
local stripped = toolchain.build .. "/apk/" .. name .. "/lib" .. name .. ".so"
target{out = stripped, ins = {so}, desc = "strip lib" .. name .. ".so", cmds = {
  {t.llvm_bin .. "/llvm-objcopy", "--strip-unneeded", so, stripped},
}}

-- The manifest: as written, or with the background service added
-- (app.background), each change made once.
local manifest = "sq_android/AndroidManifest.xml"
if a.background then
  local out = toolchain.build .. "/background/AndroidManifest.xml"
  local sqedit = {"sqedit", "sub", "--in-place", "--expect", "1"}
  target{out = out, ins = {manifest}, desc = "manifest (background service)", cmds = {
    {"cp", manifest, out},
    {sqedit, "-x", '{%s*} "<application" {%s*}',
     '{1}<uses-permission android:name="android.permission.FOREGROUND_SERVICE" />\\n' ..
     '{1}<uses-permission android:name="android.permission.FOREGROUND_SERVICE_SPECIAL_USE" />\\n' ..
     '{1}<uses-permission android:name="android.permission.WAKE_LOCK" />\\n' ..
     '{1}<uses-permission android:name="android.permission.POST_NOTIFICATIONS" />\\n{1}<application{2}', out},
    -- hasCode: true, whether it said false or true already.
    {sqedit, [['android:hasCode="' ("false" / "true") '"']], 'android:hasCode="true"', out},
    {sqedit, "-x", '{%s*} "</application>" {.*}',
     '{1}    <service android:name="dev.squared.ModelService" android:exported="false" ' ..
     'android:foregroundServiceType="specialUse">\\n' ..
     '{1}        <property android:name="android.app.PROPERTY_SPECIAL_USE_FGS_SUBTYPE" ' ..
     'android:value="Keeps the app running in the background when the user asks it to (app.keep_running)" />\\n' ..
     '{1}    </service>\\n{1}</application>{2}', out},
  }}
  manifest = out
end

-- squared's asset index (assets/SQ-INF/index): every asset path, sorted,
-- written only when it changes; apkpack link checks it against the files.
local assets = "sq_android/assets"
local listed = {}
for _, f in ipairs(glob(assets .. "/**/*")) do
  local rel = f:sub(#assets + 2)
  if not rel:match("^SQ%-INF/") then listed[#listed + 1] = rel end
end
table.sort(listed)
if #listed > 0 then write_file(assets .. "/SQ-INF/index", table.concat(listed, "\n") .. "\n") end

local built = apk{
  name = name,
  manifest = manifest,
  res = "sq_android/res",
  assets = #listed > 0 and assets or nil,
  min_sdk = api,
  target_sdk = a.target_sdk or 28,
  debuggable = a.debuggable or mode.debug,
  key = a.key,
  files = {{"lib/arm64-v8a/lib" .. name .. ".so", stripped}, {"classes.dex", sq .. "/tools/dexgen/shims.dex"}},
}

-- The verbs and Android's own words: squared's Android platform says what
-- each does (squared/build/android.lua); a verb the build file says after
-- this include wins.
include(sq .. "/build/android.lua").verbs{apk = built, name = name, package = a.package, device = a.device,
                                          sqconsole = a.sqconsole, bridge = "build/dev/bridge.conf"}

return built
