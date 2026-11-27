-- build.lua: {{project_name}}'s build, for sqmake. This file is yours; what
-- the template knows is in sq/build/template.lua, which says every setting.
--
--   sqmake          build/{{project_name}}
--   sqmake run      run it (in a terminal: it takes the whole screen)
--   sqmake test     build and run each sq_app/test/*.cpp
app = {name = "{{project_name}}"}
include("sq/build/template.lua")
