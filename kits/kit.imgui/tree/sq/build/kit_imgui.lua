-- sq/build/kit_imgui.lua: kit.imgui's part of the build, for sqmake.
-- GENERATED: kit.imgui owns this file and replaces it on update.
--
-- Dear ImGui's sources and its OpenGL 3 backend (GLES 3, "#version 300 es"),
-- compiled into the app with it.
return {
  id = "kit.imgui",
  flags = {"-Isq_kit/include", "-Isq_kit/include/imgui"},
  srcs = {
    "sq_kit/include/imgui/imgui.cpp",
    "sq_kit/include/imgui/imgui_demo.cpp",
    "sq_kit/include/imgui/imgui_draw.cpp",
    "sq_kit/include/imgui/imgui_tables.cpp",
    "sq_kit/include/imgui/imgui_widgets.cpp",
    "sq_kit/include/imgui/backends/imgui_impl_opengl3.cpp",
    "sq_kit/include/imgui/sq_imgui.cpp",
  },
}
