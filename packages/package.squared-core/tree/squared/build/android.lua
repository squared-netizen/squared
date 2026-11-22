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
--   console   a debug build's Console, here in this terminal (sqconsole,
--             the SDK's, through the bridge file the debug build keeps;
--             CMD='...' runs one command and exits)
--
-- How the device is reached is the device tool's: settings.device, a
-- command that answers install APK, run APK PACKAGE, logcat TAG, devices
-- and uninstall PACKAGE (DEVICE=<serial> and SQ_ADB from the environment).
-- The default is the template's tools/device.sh, through adb or the
-- package installer.
--
-- Inside sqterm (SQTERM=1, its sqterm-* commands installed) with no device
-- named (DEVICE unset) and no settings.device, install, run and uninstall
-- go through sqterm itself: sqterm-install, sqterm-launch, sqterm-uninstall
-- (sqterm's docs/install-api.md). No adb, nothing to pair; Android asks the
-- person on each install. logcat and devices stay the device tool's.
-- A build file that says a verb after this one wins (sqmake's rule).

local android = {}

-- sqterm's own commands, when this build runs inside sqterm and nothing
-- asks for adb.
local function sqterm_commands()
  if os.getenv("SQTERM") ~= "1" then return false end
  local device = os.getenv("DEVICE")
  if device and device ~= "" then return false end
  -- read_file: sqmake's (a build file has no io); nil when it is not there.
  return read_file((os.getenv("PREFIX") or "") .. "/bin/sqterm-install", 1) ~= nil
end

function android.verbs(s)
  local apk = s.apk or error("android.verbs: apk")
  local name = s.name or error("android.verbs: name")
  local device = s.device or {"bash", "tools/device.sh"}
  local sqterm = s.device == nil and sqterm_commands()
  if sqterm then
    verb("install", {needs = {apk}, desc = "install " .. name .. ".apk", cmd = {"sqterm-install", apk}})
  else
    verb("install", {needs = {apk}, desc = "install " .. name .. ".apk", cmd = {device, "install", apk}})
  end
  action{name = "logcat", desc = "logcat " .. name, cmd = {device, "logcat", name}}
  action{name = "devices", desc = "devices", cmd = {device, "devices"}}
  if s.bridge then
    local sqconsole = s.sqconsole or "sqconsole"
    action{name = "console", desc = "console " .. name,
           call = function()
             local cmd = {sqconsole, "--conf", s.bridge, "--into", "build/pulled"}
             local one = os.getenv("CMD")
             if one and one ~= "" then cmd[#cmd + 1] = {"--run", one} end
             return exec(cmd)
           end}
  end
  if s.package then
    if sqterm then
      verb("run", {needs = {apk}, desc = "run " .. s.package,
                   cmds = {{"sqterm-install", apk}, {"sqterm-launch", s.package}}})
      action{name = "uninstall", desc = "uninstall " .. s.package, cmd = {"sqterm-uninstall", s.package}}
    else
      verb("run", {needs = {apk}, desc = "run " .. s.package, cmd = {device, "run", apk, s.package}})
      action{name = "uninstall", desc = "uninstall " .. s.package, cmd = {device, "uninstall", s.package}}
    end
  end
end

return android
