-- build.lua: this app's build for sqmake: `sqmake` makes build/{{project_name}}.apk,
-- signed; `sqmake install`, `run`, `logcat`, `devices`, `uninstall` put it on
-- a device. What is the app's own goes
-- here; sq/build/template.lua says every setting.
app = {name = "{{project_name}}", package = "{{package_name}}"}
include("sq/build/template.lua")
