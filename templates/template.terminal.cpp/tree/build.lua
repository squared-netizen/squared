-- build.lua: {{project_name}}'s build for sqmake: `sqmake` makes
-- build/{{project_name}}, `sqmake run` runs it, `sqmake test` runs the tests
-- (the same program make's Makefile builds). This file is yours; what the
-- template knows is in sq/build/template.lua, which says every setting.
app = {name = "{{project_name}}"}
include("sq/build/template.lua")
