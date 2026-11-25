-- v3/build.lua: sq.cart, the Squared Cartridge 3.0 reader and writer, for
-- sqmake: built on its own here, or included by a project that vendors it
-- (square, sqdrive, squared-pg).
--
--   sqmake -C v3          the libraries, sqcart3, sqcart3-read and the tests
--   sqmake -C v3 test     the tests
--
-- Included, it builds nothing by default and returns its libraries:
--
--   sqcart_settings = {root = "third_party/sqcart/v3", data = squared.data}
--   local cart = include("third_party/sqcart/v3/build.lua")
--   exe{..., deps = {cart.read}}    -- sq.cart: reading, zstd's decoder only
--   exe{..., deps = {cart.write}}   -- sq.cart.write too, all of zstd
--
--   root   where this folder is, from the including build file (default ".")
--   data   a library that provides sq.data (squared's own); without it the
--          vendored copy (../third_party/squared-data) is built, with yyjson
--
-- The compiler is $CXX (and $CC): clang and libc++ 19 or later (23 is what
-- sqterm and CI use) with `import std;`. Offline: zstd, yyjson, Monocypher
-- and squared's data module are vendored in ../third_party.

local s = sqcart_settings or {}
local top = s.root == nil  -- built on its own, not included
-- A path in this folder, from the including build file's.
local function here(path) return top and path or s.root .. "/" .. path end
local root = top and "." or s.root
local tp = here("../third_party")

if top then
  local opt = mode.debug and {"-O0", "-g"} or {"-O2", "-g"}
  toolchain.cxxflags = {"-std=c++23", "-stdlib=libc++", opt}
  toolchain.cflags = {opt}
  toolchain.ldflags = {"-stdlib=libc++", "-lpthread"}
end

local warn = {"-Wall", "-Wextra", "-Wpedantic", "-Wconversion", "-Wno-sign-conversion",
              "-Wno-missing-designated-field-initializers"}
local c_includes = {"-I" .. tp .. "/yyjson-0.12.0/src", "-I" .. tp .. "/zstd-1.5.7",
                    "-I" .. tp .. "/monocypher-4.0.3", "-I" .. tp .. "/monocypher-4.0.3/optional"}
local systems = {"-isystem", tp .. "/yyjson-0.12.0/src", "-isystem", tp .. "/zstd-1.5.7",
                 "-isystem", tp .. "/monocypher-4.0.3", "-isystem", tp .. "/monocypher-4.0.3/optional"}

-- sq.data: the including project's, or the vendored one with yyjson.
local data = s.data
if not data then
  local sqd = tp .. "/squared-data"
  local srcs = {tp .. "/yyjson-0.12.0/src/yyjson.c"}
  for _, f in ipairs(glob(sqd .. "/**/*.cppm")) do srcs[#srcs + 1] = f end
  for _, f in ipairs(glob(sqd .. "/src/*.cpp")) do srcs[#srcs + 1] = f end
  data = library{name = "sqcart_sqdata", srcs = srcs, flags = {"-w", systems}}
end

-- Monocypher (Ed25519 signatures): C, its own warnings left to it.
local monocypher = library{name = "sqcart_monocypher", flags = {"-w", c_includes},
                           srcs = {tp .. "/monocypher-4.0.3/monocypher.c",
                                   tp .. "/monocypher-4.0.3/optional/monocypher-ed25519.c"}}

-- sq.cart: every partition (sqmake orders them by their imports) but the
-- writer's module.
local cart_srcs = {}
for _, f in ipairs(glob(here("src/*.cppm"))) do
  if not f:match("/write%.cppm$") then cart_srcs[#cart_srcs + 1] = f end
end
local cart = library{name = "sqcart", srcs = cart_srcs, deps = {data, monocypher},
                     flags = {warn, systems, "-I" .. here("src")}}

-- zstd twice: its decoder alone for a reader, all of it for a writer. Link one.
local unzstd = library{name = "sqcart-unzstd", srcs = {tp .. "/zstd-1.5.7/zstddeclib.c"},
                       flags = {"-w", c_includes}}
local zstd = library{name = "sqcart-zstd", srcs = {tp .. "/zstd-1.5.7/zstd.c"},
                     flags = {"-w", c_includes, "-DZSTD_MULTITHREAD=0"}}
local writer = library{name = "sqcart-write", srcs = {here("src/write.cppm")}, deps = {cart},
                       flags = {warn, systems, "-I" .. here("src")}}

local libs = {
  data = data,
  read = library{name = "sqcart_read", srcs = {}, deps = {cart, unzstd}},
  write = library{name = "sqcart_write_all", srcs = {}, deps = {writer, cart, zstd}},
}

if top then
  local tool = here("tools/sqcart3.cpp")
  exe{name = "sqcart3", srcs = {tool}, deps = {libs.write}, flags = {warn, systems}}
  -- A reader alone links without the compressor: proved by linking
  -- sqcart3's read-only twin against the reading libraries.
  -- Its own source (one object a source): the tool, read-only.
  local read_only = write_file(toolchain.build .. "/gen/sqcart3-read.cpp",
                               "#define SQCART3_READ_ONLY 1\n#include \"../../tools/sqcart3.cpp\"\n")
  exe{name = "sqcart3-read", srcs = {{read_only, imports = {"std", "sq.cart"}}}, deps = {libs.read},
      flags = {warn, systems}}
  -- The tests share a module of helpers (tests/testing.cppm: it writes
  -- cartridges, byte by byte, for the reader to open).
  local testing = library{name = "sqcart_testing", srcs = {here("tests/testing.cppm")}, deps = {cart},
                          flags = {warn, systems}}
  local tests = {}
  for _, src in ipairs(glob(here("tests/test_*.cpp"))) do
    tests[#tests + 1] = exe{name = src:match("([^/]+)%.cpp$"), srcs = {src}, deps = {testing, libs.write},
                            flags = {warn, systems}}
  end
  verb("test", {needs = tests, call = function()
    local failed = 0
    for _, t in ipairs(tests) do
      if exec{t} ~= 0 then failed = failed + 1 end
    end
    print(failed == 0 and "all tests passed" or "FAILED")
    return failed == 0 and 0 or 1
  end})
end

return libs
