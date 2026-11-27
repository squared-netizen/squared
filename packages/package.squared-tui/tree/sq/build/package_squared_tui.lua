-- sq/build/package_squared_tui.lua: package.squared-tui's part of the build,
-- for sqmake. GENERATED: package.squared-tui owns this file and replaces it on
-- update.
--
-- The template's build (sq/build/template.lua) includes every
-- sq/build/package_*.lua, after the kits', and adds what each returns: sq.tui's
-- interfaces and sources, compiled with the app's. sq.tui imports sq.term and
-- sq.keys, squared-core's: the template links squared's `term` library (which
-- brings `keys`), as template.sq_tui.cpp's does.
local dir = "sq_pkg/squared_tui"
local srcs = {}
for _, f in ipairs(glob(dir .. "/*.cppm")) do srcs[#srcs + 1] = f end
for _, f in ipairs(glob(dir .. "/src/*.cpp")) do srcs[#srcs + 1] = f end
return {
  id = "package.squared-tui",
  srcs = srcs,
}
