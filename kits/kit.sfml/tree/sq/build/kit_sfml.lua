-- sq/build/kit_sfml.lua: kit.sfml's part of the build, for sqmake.
-- GENERATED: kit.sfml owns this file and replaces it on update.
--
-- SFML 3.1's static archives, built for Android and installed with the kit
-- (sq_kit/lib/: its payload, BUILD-INFO says how), linked in this order:
-- SFML's own first, then what they depend on, as one group (they refer to
-- each other), then the NDK's libraries they call, from the sysroot by path.
local libs = {}
for _, name in ipairs({"main", "graphics-s", "audio-s", "window-s", "system-s"}) do
  libs[#libs + 1] = "sq_kit/lib/libsfml-" .. name .. ".a"
end
local own = {}
for _, l in ipairs(libs) do own[l] = true end
local deps = {}
for _, a in ipairs(glob("sq_kit/lib/*.a")) do
  if not own[a] then deps[#deps + 1] = a end
end
return {
  id = "kit.sfml",
  flags = {"-Isq_kit/include"},
  link = {libs, "-Wl,--start-group", deps, "-Wl,--end-group", "-lGLESv1_CM", "-lz", "-lOpenSLES"},
}
