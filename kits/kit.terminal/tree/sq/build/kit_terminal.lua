-- sq/build/kit_terminal.lua: kit.terminal's part of the build, for sqmake.
-- GENERATED: kit.terminal owns this file and replaces it on update.
--
-- The template's build (sq/build/template.lua) includes every
-- sq/build/kit_*.lua and adds what each returns: here, one header folder.
return {
  id = "kit.terminal",
  flags = {"-Isq_kit/include"},
}
