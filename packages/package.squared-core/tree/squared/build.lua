-- build.lua: squared's own build, for sqmake: the libraries (build/squared.lua,
-- the same ones an app links), for this machine, and their tests.
--
--   sqmake          every library, sqconsole, dexgen and every test's program
--   sqmake test     the tests, each run from its module's folder
--   sqmake --debug  -O0 (build/sqmake-debug; else build/sqmake)
--
--   SQUARED_TARGET=android sqmake
--                   the libraries for Android (the gles backend and the
--                   Android-only sources), by the SDK's toolchain
--                   (build/toolchain.lua: sqterm's), in build/sqmake-android;
--                   what tools/lint.sh analyses those sources with
--
-- The compiler is $CXX (and $CC): clang and libc++ 23 with `import std;`. The
-- tests are the host's (platform "host", the null graphics backend): an app's
-- build for Android includes build/squared.lua itself, with its template's
-- toolchain. Offline: everything it compiles is in this folder (third_party/
-- included). Every build writes its compile_commands.json (sqmake 0.13.0).

if os.getenv("SQUARED_TARGET") == "android" then
  toolchain.build = mode.debug and "build/sqmake-android-debug" or "build/sqmake-android"
  toolchain.sdk("build/toolchain.lua")
  squared_settings = {root = "../squared", platform = "android", graphics = "gles", debug = mode.debug}
  default(include("build/squared.lua").all.out)
  return
end

local opt = mode.debug and {"-O0", "-g"} or {"-O1", "-g"}
toolchain.cxxflags = {"-std=c++23", "-stdlib=libc++", opt}
toolchain.cflags = {opt}
toolchain.ldflags = {"-stdlib=libc++", "-pthread", "-ldl"}
-- build/ holds this build's own Lua (squared.lua, toolchain.lua): the
-- outputs go in a folder of their own there.
toolchain.build = mode.debug and "build/sqmake-debug" or "build/sqmake"

-- Headers are <squared/...>: the root is named from the folder above.
squared_settings = {root = "../squared", platform = "host", graphics = "null", debug = mode.debug}
local squared = include("build/squared.lua")
local root = "../squared"

-- The tests are told where things are as absolute paths: each runs from its
-- module's folder (as make's did), not this one.
local here = os.getenv("PWD") or "."
local function quoted(path) return "\"" .. here .. "/" .. path .. "\"" end

local warn = {"-Wall", "-Wextra", "-Wpedantic"}

-- sqconsole, the Console's host client (console/client/sqconsole.cpp).
local sqconsole = exe{name = "sqconsole", srcs = {root .. "/console/client/sqconsole.cpp"},
                      deps = {squared.console}, flags = warn}

-- dexgen, the shim dex's writer (tools/dexgen.sh), and its self-test: the
-- same source twice, so the self-test's is a source of its own.
local dexgen = exe{name = "dexgen", srcs = {root .. "/tools/dexgen/dexgen.cpp"}, flags = warn}
local self_test_src = write_file(toolchain.build .. "/gen/dexgen-self-test.cpp",
                                 "#define DEXGEN_SELF_TEST 1\n#include \"" .. here .. "/tools/dexgen/dexgen.cpp\"\n")
local dexgen_self_test = exe{name = "dexgen-self-test", srcs = {{self_test_src, imports = {"std"}}}, flags = warn}

-- The core types' sizes, for docs/developer/memory-profile.md: `sqmake sizes`.
local sizes = exe{name = "measure_sizes", srcs = {root .. "/tools/sizes/measure_sizes.cpp"},
                  deps = {squared.all}, default = false}
action{name = "sizes", needs = {sizes}, desc = "the core types' sizes (docs/developer/memory-profile.md)",
       cmd = {sizes}}

-- Every test: {module, name, deps (squared's libraries by name), and what
-- else it compiles (its stubs, a source the host's library leaves out) or
-- says}. The source is <module>/test/<name>.cpp unless `src` says.
local tests = {
  {"dev", "dev_log_test", {"dev"}},
  {"dev", "frame_allocations_test", {"dev"}},
  {"dev", "log_listener_test", {"dev"}},

  {"files", "android_asset_test", {"files"},
   srcs = {"files/test/asset_stub.cpp", "files/src/android_asset_file_system.cpp"}},
  {"files", "posix_file_system_test", {"files"}},
  {"files", "embedded_file_system_test", {"files"}},

  {"time", "time_test", {"time"}},

  {"messaging", "address_test", {"messaging"}},
  {"messaging", "composite_test", {"messaging"}},
  {"messaging", "list_test", {"messaging"}},
  {"messaging", "snapshot_test", {"messaging"}},
  {"messaging", "telegram_ring_test", {"messaging"}},
  {"messaging", "telegram_test", {"messaging"}},
  {"messaging", "telegraph_test", {"messaging"}},
  {"messaging", "telegraph_thread_test", {"messaging"}},
  {"messaging", "value_cache_test", {"messaging"}},
  {"messaging", "watch_test", {"messaging"}},

  {"preferences", "preferences_test", {"preferences"}},

  {"settings", "settings_test", {"settings"}},

  {"dialogs", "file_dialog_test", {"dialogs"}},

  {"stage", "stage_test", {"stage"}, flags = {"-DSTAGE_SKIN_DIR=" .. quoted("stage/skin/default/skin")}},
  {"stage", "stage_alone_test", {"stage"}},

  {"gui", "batch_painter_test", {"gui"}},
  {"gui", "ui_safe_area_test", {"gui"}},
  {"gui", "skin_loader_test", {"gui"}},
  {"gui", "ui_keys_test", {"gui"}},
  {"gui", "pane_frame_test", {"gui"}},
  {"gui", "screen_layout_test", {"gui"}},
  {"gui", "layout_file_test", {"gui"}, flags = {"-DLAYOUT_FILE_CASES=" .. quoted("gui/test/layout_file_cases.txt")}},

  {"font", "font_test", {"font"},
   flags = {"-I" .. root .. "/font/src/freetype", "-I" .. root .. "/third_party/freetype/include"}},

  -- gles_test (and console_renderer_test, below) are the gles backend's: an
  -- Android build's, as they were make's.

  {"graphics", "image_decode_test", {"graphics"}, flags = {"-I" .. root .. "/third_party/stb"}},
  {"graphics", "texture_test", {"graphics"}},
  {"graphics", "texture_atlas_test", {"graphics"}},
  {"graphics", "sprite_batch_test", {"graphics"}},
  {"graphics", "glyph_layout_test", {"graphics"}},
  {"graphics", "bitmap_font_error_test", {"graphics"}},

  {"console", "scrollback_test", {"console"}},
  {"console", "console_test", {"console"}},
  {"console", "layout_test", {"console"}},
  {"console", "dump_test", {"console"}},
  {"console", "telegraph_tools_test", {"console", "messaging"}},
  {"console", "crash_file_test", {"console"}},
  {"console", "bridge_test", {"console"}, needs = {sqconsole},
   flags = {"-DSQCONSOLE_PATH=\"" .. here .. "/" .. sqconsole .. "\""}},

  -- term_test and text_test count allocations with an operator new of their
  -- own. sq.term links sq.dev (through sq.app), whose operator new lld also
  -- takes, for the forms the test leaves to it (libc++ names them): the
  -- test's, first on the line, is the one used.
  {"term", "term_test", {"term"}, link = {"-Wl,--allow-multiple-definition"}},
  {"term", "pty_test", {"term"}},
  {"term", "key_test", {"term"}},
  {"term", "text_test", {"term"}, link = {"-Wl,--allow-multiple-definition"}},

  {"tui", "tui_test", {"tui"}},
}

-- A list of paths, nested lists flattened (srcs are not flattened by sqmake).
local function flat(list, out)
  out = out or {}
  for _, v in ipairs(list) do
    if type(v) == "table" then flat(v, out) else out[#out + 1] = v end
  end
  return out
end
local function rooted(paths)
  local out = {}
  for _, p in ipairs(flat(paths)) do
    out[#out + 1] = p:sub(1, #root + 1) == root .. "/" and p or root .. "/" .. p
  end
  return out
end

local programs = {}
local runs = {}
for _, t in ipairs(tests) do
  local mod, name = t[1], t[2]
  local deps = {}
  for _, d in ipairs(t[3]) do deps[#deps + 1] = assert(squared[d], "no library " .. d) end
  local srcs = rooted({mod .. "/test/" .. name .. ".cpp", t.srcs or {}})
  local program = exe{name = mod .. "/" .. name, srcs = srcs, deps = deps,
                      flags = {warn, "-I" .. root .. "/" .. mod .. "/src", "-I" .. root .. "/" .. mod .. "/test",
                               t.flags or {}}, link = t.link}
  programs[#programs + 1] = program
  runs[#runs + 1] = {mod = mod, name = name, program = program, needs = t.needs}
end
for _, r in ipairs(runs) do
  if r.needs then for _, n in ipairs(r.needs) do programs[#programs + 1] = n end end
end

verb("test", {needs = {programs, dexgen, dexgen_self_test}, call = function()
  local failed = {}
  for _, r in ipairs(runs) do
    print("  test " .. r.mod .. "/" .. r.name)
    if exec{"sh", "-c", "cd \"$1\" && exec \"$2\"", "sh", r.mod, here .. "/" .. r.program} ~= 0 then
      failed[#failed + 1] = r.mod .. "/" .. r.name
    end
  end
  -- dexgen (as tools/dexgen.sh checks it): the committed shim dex written
  -- again byte for byte, and the self-test's dex read back.
  print("  test tools/dexgen")
  local dex, test_dex = toolchain.build .. "/shims.dex", toolchain.build .. "/dexgen-self-test.dex"
  if exec{"sh", "-c", "\"$1\" \"$2\" >/dev/null && cmp -s \"$2\" tools/dexgen/shims.dex", "sh", dexgen, dex} ~= 0 then
    print("tools/dexgen/shims.dex differs from what dexgen writes now: tools/dexgen.sh --write, then commit")
    failed[#failed + 1] = "tools/dexgen"
  end
  if exec{"sh", "-c", "\"$1\" \"$2\" && \"$1\" --dump \"$2\" >/dev/null", "sh", dexgen_self_test, test_dex} ~= 0 then
    failed[#failed + 1] = "tools/dexgen-self-test"
  end
  if #failed == 0 then
    print("all tests passed")
    return 0
  end
  print("FAILED: " .. table.concat(failed, ", "))
  return 1
end})

-- What make's `headers` checked (tools/check_modules.sh): FreeType's pinned
-- sources, every import covered by its library's dependencies, every
-- interface unit named for its path. Nothing to build first.
action{name = "headers", desc = "check the pinned sources and the modules' imports and names",
       cmd = {"bash", "tools/check_modules.sh"}}

-- What make's `lint` ran (tools/lint.sh, docs/developer/lint.md): format,
-- the source rules, clang-tidy on every unit with this build's commands
-- (its compile_commands.json). tools/lint.sh itself takes --changed and the
-- rest.
action{name = "lint", desc = "format, source-rule and clang-tidy checks (tools/lint.sh)",
       cmd = {"bash", "tools/lint.sh"}}

-- The API reference, docs/reference/ (tools/docs.sh, clang-doc), and its
-- check that what is committed is what the sources say.
action{name = "docs", desc = "the API reference, written again (tools/docs.sh)", cmd = {"bash", "tools/docs.sh"}}
action{name = "docs-check", desc = "the API reference is what the sources say", cmd = {"bash", "tools/docs.sh", "--check"}}
