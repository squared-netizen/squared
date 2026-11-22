-- build/android.lua: what sqmake's verbs do on Android (sqmake 0.8.0 or
-- later), for an app whose build makes an APK. squared's Android platform
-- says it here, once, and nowhere else: sqmake knows no platform, and a
-- template or an app says only what is its own.
--
--   local android = include("squared/build/android.lua")
--   android.verbs{apk = built, name = "mygame", package = "dev.me.mygame"}
--
-- The verbs:
--   install   the APK put on the device
--   run       installed, then started (the launcher's activity)
-- and Android's own words, as actions:
--   logcat    the app's log and Android's crash lines
--   devices   what can be reached, and how to connect
--   uninstall the app removed
--
-- How the device is reached is the device tool's: settings.device, a
-- command that answers install APK, run APK PACKAGE, logcat TAG, devices
-- and uninstall PACKAGE (DEVICE=<serial> and SQ_ADB from the environment).
-- The default is the template's tools/device.sh, through adb or the
-- package installer; another (sqterm's own, one day) is that one setting.
-- A build file that says a verb after this one wins (sqmake's rule).

local android = {}

function android.verbs(s)
  local apk = s.apk or error("android.verbs: apk")
  local name = s.name or error("android.verbs: name")
  local device = s.device or {"bash", "tools/device.sh"}
  verb("install", {needs = {apk}, desc = "install " .. name .. ".apk", cmd = {device, "install", apk}})
  action{name = "logcat", desc = "logcat " .. name, cmd = {device, "logcat", name}}
  action{name = "devices", desc = "devices", cmd = {device, "devices"}}
  if s.package then
    verb("run", {needs = {apk}, desc = "run " .. s.package, cmd = {device, "run", apk, s.package}})
    action{name = "uninstall", desc = "uninstall " .. s.package, cmd = {device, "uninstall", s.package}}
  end
end

return android
