-- build.lua: this app's build for sqmake (`sqmake`: build/{{project_name}}.apk,
-- signed), the same app make's Makefile builds. What is the app's own goes
-- here; sq/build/template.lua says every setting.
app = {name = "{{project_name}}"}
include("sq/build/template.lua")
