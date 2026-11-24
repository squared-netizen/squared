-- sq/build/kit_opengl.lua: kit.opengl's part of the build, for sqmake.
-- GENERATED: kit.opengl owns this file and replaces it on update.
--
-- The template's build (sq/build/template.lua) includes every
-- sq/build/kit_*.lua and adds what each returns. EGL and GLES 3 are already
-- linked by the template, from the sysroot by path (a bare -lEGL could bind
-- another EGL), so this kit adds only its header.
return {
  id = "kit.opengl",
  flags = {"-Isq_kit/include"},
}
