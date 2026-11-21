-- sq/build/template.lua: template.android.cpp's build, for sqmake (0.7.0 or
-- later): squared, the app's native library and its APK, as the template's
-- make files build them (Makefile, mk/), with no shell (the device
-- actions, at the end, run whatever app.device names). A project's
-- build.lua says what is its own and includes this:
--
--   app = {name = "mygame"}
--   include("sq/build/template.lua")
--
-- The settings (all but name optional):
--   app = {
--     name = "mygame",            -- libNAME.so, build/NAME.apk
--     squared = "squared",        -- the framework's folder
--     min_sdk = 28,               -- the sysroot package's API level
--     target_sdk = 28,
--     background = false,         -- the background service (BACKGROUND=1)
--     debuggable = false,
--     debug = false,              -- -O0 -g (squared's too) instead of -O2
--     exclude = {},               -- sources under sq_app/src not to build
--     flags = {},                 -- the app's own compile flags
--     link = {},                  -- and link flags
--     shared = {},                -- what the app and squared must agree on:
--                                 -- -DSQUARED_TELEGRAPH_...=, -DSQUARED_CONSOLE=1
--     key = nil,                  -- apkpack's key (~/.android/apkpack.pem)
--     package = nil,              -- the manifest's package: for run, uninstall
--     device = {"bash", "tools/device.sh"},
--                                 -- what talks to the device (below)
--     sqgen = "sqgen",            -- for sq_app/names.json, sq_app/layouts/
--   }
--
-- The template's layout: sq_app/src/**/*.cpp (the app), sq_android/*.cpp
-- (the platform side), sq_app/third_party/**/*.c, sq_android/AndroidManifest.xml,
-- res/ and assets/. classes.dex is squared's shims (tools/dexgen/shims.dex,
-- committed): nothing is built to run during the build. An app with
-- sq_app/names.json or sq_app/layouts/ needs sqgen, the SDK's.

local a = app or error("sq/build/template.lua: set app = {name = ...} before including it")
local name = a.name or error("sq/build/template.lua: app.name")
local sq = a.squared or "squared"
local api = a.min_sdk or 28

local t = toolchain.sdk(sq .. "/build/toolchain.lua")
squared_settings = {root = sq, platform = "android", graphics = "gles", debug = a.debug, flags = a.shared or {}}
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
add(glob("sq_app/src/**/*.cpp"))
add(glob("sq_android/*.cpp"))
add(glob("sq_app/third_party/**/*.c"))
local glue = sq .. "/third_party/native_app_glue"
-- Google's code, with its own warnings: compiled without the app's (make's
-- build compiles it so too), and linked as an object, not from an archive.
local glue_obj = rules.cc(glue .. "/android_native_app_glue.c", {a.debug and {"-O0", "-g"} or {"-O2"}, "-fPIC", "-w", "-I" .. glue})

-- sqgen (squared/tools/sqgen): sq_app/names.json and sq_app/layouts/*.json
-- into C++ (names.hpp, layouts.hpp, layouts.cpp in build/gen), for an app
-- with either; the app's sources include them, so every compile waits. An
-- app with neither builds without it. sqgen is the SDK's, on PATH (or
-- app.sqgen), as sqedit is.
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
  local cmd = {a.sqgen or "sqgen"}
  if #names > 0 then cmd[#cmd + 1] = {"--names", names[1]} end
  if #layouts > 0 then cmd[#cmd + 1] = {"--layouts", "sq_app/layouts"} end
  cmd[#cmd + 1] = {"--out", gen}
  local cpp = gen .. "/layouts.cpp"
  -- touch: sqgen leaves an unchanged file as it was.
  target{out = cpp, ins = {inputs, listed}, desc = "sqgen", cmds = {cmd, {"touch", cpp}}}
  -- What layouts.cpp imports (tools/sqgen writes these three).
  srcs[#srcs + 1] = {cpp, imports = {"sq.gui", "sq.messaging", "std"}}
  gen_flags = {"-I" .. gen}
  after = {cpp}
end

-- FreeType's API (FT_EXPORT: default visibility) stays inside: it is
-- squared_font's, not the app's to export; nor is the C++ runtime's (the
-- runtimes package's libc++.a carries the unwinder too).
-- Android's libraries, named by path in the sysroot (a bare -lEGL could bind
-- another EGL; mk/pkg_squared_core.mk says why).
local platform = t.sysroot .. "/usr/lib/aarch64-linux-android/" .. api
local so = shlib{
  name = name,
  srcs = srcs,
  objs = {glue_obj},
  deps = {squared.all},
  after = after,
  flags = {gen_flags, a.debug and {"-O0", "-g"} or {"-O2"}, "-Wall", "-Wextra", "-Wpedantic", "-fPIC", "-fvisibility=hidden", "-DANDROID", "-I" .. glue, a.flags or {}},
  link = {"-Wl,--hash-style=both", "-Wl,--gc-sections", "-Wl,--no-undefined",
          "-Wl,--exclude-libs,libsquared_font.a",
          "-Wl,--exclude-libs,libsquared_freetype.a", "-Wl,--exclude-libs,libc++.a:libc++abi.a:libunwind.a", "-pthread", "-landroid", "-llog",
          platform .. "/libEGL.so", platform .. "/libGLESv3.so", a.link or {}},
}

-- The APK's copy is stripped (--strip-unneeded keeps the dynamic symbols the
-- loader and dladdr() read); build/libNAME.so keeps its symbols for addr2line.
-- llvm-objcopy, which sqterm's llvm package has (llvm-strip it may not).
local stripped = toolchain.build .. "/apk/" .. name .. "/lib" .. name .. ".so"
target{out = stripped, ins = {so}, desc = "strip lib" .. name .. ".so", cmds = {
  {t.llvm_bin .. "/llvm-objcopy", "--strip-unneeded", so, stripped},
}}

-- The manifest: as written, or with the background service added
-- (mk/squared_android_package.mk, BACKGROUND=1), each change made once.
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
  debuggable = a.debuggable,
  key = a.key,
  files = {{"lib/arm64-v8a/lib" .. name .. ".so", stripped}, {"classes.dex", sq .. "/tools/dexgen/shims.dex"}},
}

-- Actions (sqmake 0.7.0): `sqmake install`, `run`, `logcat`, `devices`,
-- `uninstall`, each handed to app.device, which says where the APK goes
-- and how. The device tool's commands are the interface (tools/device.sh's,
-- which make's build calls too): install APK, run APK PACKAGE, logcat TAG,
-- devices, uninstall PACKAGE; DEVICE=<serial> and SQ_ADB reach it from the
-- environment. Another tool that answers them (one of the SDK's, or the
-- app's own) is one setting away; neither sqmake nor this file changes.
local device = a.device or {"bash", "tools/device.sh"}
action{name = "install", needs = {built}, desc = "install " .. name .. ".apk", cmd = {device, "install", built}}
action{name = "logcat", desc = "logcat " .. name, cmd = {device, "logcat", name}}
action{name = "devices", desc = "devices", cmd = {device, "devices"}}
if a.package then
  action{name = "run", needs = {built}, desc = "run " .. a.package, cmd = {device, "run", built, a.package}}
  action{name = "uninstall", desc = "uninstall " .. a.package, cmd = {device, "uninstall", a.package}}
end

return built
