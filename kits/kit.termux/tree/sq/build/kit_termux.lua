-- sq/build/kit_termux.lua: kit.termux's part of the build, for sqmake.
-- GENERATED: kit.termux owns this file and replaces it on update.
--
-- The template's build (sq/build/template.lua) includes every
-- sq/build/kit_*.lua and adds what each returns: here, one header folder.
-- The termux-api programs it runs are found at run time, not at build.
return {
  id = "kit.termux",
  flags = {"-Isq_kit/include"},
}
