#!/usr/bin/env bash
# tools/device.sh: put this project's APK on an Android device, start it,
# read its log, through adb. `make install`, `run`, `logcat`, `devices` and
# `uninstall` call it; you can too.
#
#   tools/device.sh devices                 what adb reaches, and which is this phone
#   tools/device.sh connect [PORT]          adb to this phone over Wireless Debugging
#   tools/device.sh install APK             adb install -r, else the package installer
#   tools/device.sh run APK PACKAGE         install, then start it (needs adb)
#   tools/device.sh logcat TAG              the app's log and Android's crash lines
#   tools/device.sh run-as PACKAGE CMD...   a command as the app, in its private
#                                           directory (debuggable builds only)
#   tools/device.sh uninstall PACKAGE
#   tools/device.sh console-only PACKAGE    start it with only the Console running
#                                           (a CONSOLE build; adb, or Termux's am)
#
# Which device: DEVICE=<serial> when set, as `devices` lists it; otherwise
# this phone, and only when adb's server is already running and lists it.
# "This phone" is the attached device whose build fingerprint is the one
# Termux reads here. Nothing else is ever chosen by default: the tablet, or
# anything else on USB or Wi-Fi, is used only when named. With no device,
# `install` falls back to the package installer, as before adb.
#
# Off Android (desktop Linux) there is no "this phone": the one attached
# device is used, as `adb install` itself would.
#
# Environment: SQ_ADB (default adb; termux-adb for a USB device), DEVICE,
# ANDROID_ADB_SERVER_PORT (adb's own, default 5037).
#
# It never starts adb's server to look for a device (that would wake whatever
# is plugged in); `connect` and a named DEVICE do, because you asked. No
# network beyond adb's own connection to the device.
set -euo pipefail

ADB=${SQ_ADB:-adb}
DEVICE=${DEVICE:-}
SERVER_PORT=${ANDROID_ADB_SERVER_PORT:-5037}

say() { printf '  %-14s %s\n' "$1" "$2"; }
die() {
  printf '%s\n' "$@" >&2
  exit 1
}

have_adb() { command -v "$ADB" >/dev/null 2>&1; }

# A connection to the server's port, opened and closed; bash's /dev/tcp, so
# no nc is needed.
server_up() { (exec 3<>"/dev/tcp/127.0.0.1/$SERVER_PORT") 2>/dev/null; }

# Serials in the "device" state (not offline, not unauthorized).
attached() { "$ADB" devices 2>/dev/null | awk 'NR > 1 && $2 == "device" { print $1 }'; }

getprop_cmd() {
  if command -v getprop >/dev/null 2>&1; then
    echo getprop
  elif [ -x /system/bin/getprop ]; then
    echo /system/bin/getprop
  fi
}

# This device's build fingerprint, or nothing off Android.
local_fingerprint() {
  local cmd
  cmd=$(getprop_cmd)
  [ -n "$cmd" ] || return 0
  "$cmd" ro.build.fingerprint 2>/dev/null || true
}

remote_fingerprint() { "$ADB" -s "$1" shell getprop ro.build.fingerprint 2>/dev/null | tr -d '\r' || true; }

# The serial to use, or nothing.
pick() {
  local list mine serial
  if [ -n "$DEVICE" ]; then
    have_adb || die "DEVICE=$DEVICE, but there is no $ADB (pkg install android-tools)"
    list=$(attached)
    grep -qxF -- "$DEVICE" <<<"$list" ||
      die "DEVICE=$DEVICE is not attached and ready. Attached:" "${list:-  (none)}" \
        "See: bash tools/device.sh devices"
    printf '%s\n' "$DEVICE"
    return 0
  fi
  have_adb || return 0
  server_up || return 0
  list=$(attached)
  [ -n "$list" ] || return 0
  mine=$(local_fingerprint)
  if [ -z "$mine" ]; then
    # Not on Android: the one attached device, if there is exactly one.
    [ "$(wc -l <<<"$list")" -eq 1 ] && printf '%s\n' "$list"
    return 0
  fi
  while IFS= read -r serial; do
    if [ "$(remote_fingerprint "$serial")" = "$mine" ]; then
      printf '%s\n' "$serial"
      return 0
    fi
  done <<<"$list"
  return 0
}

need_device() {
  local serial
  serial=$(pick)
  [ -n "$serial" ] || die "$1 needs adb connected to this phone (or DEVICE=<serial>)." \
    "  bash tools/device.sh devices    shows what adb reaches, and how to connect"
  printf '%s\n' "$serial"
}

connect_help() {
  cat <<'HELP'
To reach this phone with adb (Android 11 and later, on Wi-Fi):
  1. pkg install android-tools
  2. Settings > Developer options > Wireless debugging: on.
  3. Once only: "Pair device with pairing code". Keep that dialog open (Termux
     beside it in split screen, or in a floating window) and run
       adb pair localhost:<pairing port>
     then type the 6-digit code.
  4. Each time Wireless debugging is turned on, it has a new port, shown on
     its screen as "IP address & Port":
       bash tools/device.sh connect <port>
     or with no port, to find it on the network:
       bash tools/device.sh connect
Wireless debugging turns itself off when Wi-Fi does; reconnect after.
Another device: DEVICE=<serial> as listed above (SQ_ADB=termux-adb for USB).
HELP
}

cmd_devices() {
  local mine serial state line chosen
  if ! have_adb; then
    say "adb" "not found"
    connect_help
    return 0
  fi
  say "adb" "$(command -v "$ADB")"
  if ! server_up; then
    say "server" "not running, so nothing is connected"
    connect_help
    return 0
  fi
  mine=$(local_fingerprint)
  chosen=$(pick)
  local any=0
  while IFS= read -r line; do
    serial=${line%%[[:space:]]*}
    state=$(awk '{ print $2 }' <<<"$line")
    [ -n "$serial" ] || continue
    any=1
    local note=""
    if [ "$state" = device ] && [ -n "$mine" ] && [ "$(remote_fingerprint "$serial")" = "$mine" ]; then
      note="this phone"
    fi
    [ "$serial" = "$chosen" ] && note="${note:+$note, }used by make install"
    say "$state" "$serial${note:+  ($note)}"
  done < <("$ADB" devices 2>/dev/null | awk 'NR > 1 && NF >= 2')
  if [ "$any" = 0 ]; then
    say "devices" "none"
    connect_help
  elif [ -z "$chosen" ]; then
    say "make install" "the package installer (no device is this phone; DEVICE=<serial> for one)"
  fi
}

cmd_connect() {
  have_adb || die "no $ADB: pkg install android-tools" "$(connect_help)"
  if [ $# -ge 1 ]; then
    "$ADB" connect "localhost:$1"
  else
    local found=0 address
    while IFS= read -r address; do
      [ -n "$address" ] || continue
      found=1
      "$ADB" connect "$address" || true
    done < <("$ADB" mdns services 2>/dev/null | awk '/_adb-tls-connect/ { print $NF }')
    [ "$found" = 1 ] || die "adb found no Wireless debugging service on the network." \
      "Give the port from its screen: bash tools/device.sh connect <port>"
  fi
  cmd_devices
}

cmd_install() {
  local apk=$1 serial
  [ -f "$apk" ] || die "no APK at $apk (make apk)"
  serial=$(pick)
  if [ -n "$serial" ]; then
    say "adb install" "$(basename "$apk") -> $serial"
    "$ADB" -s "$serial" install -r "$apk"
  elif command -v termux-open >/dev/null 2>&1; then
    say "installer" "handing $apk to the package installer"
    say "" "(no prompt with adb connected: make devices)"
    termux-open "$apk"
  elif have_adb; then
    "$ADB" install -r "$apk"
  else
    die "no installer found. The APK is at $apk."
  fi
}

cmd_run() {
  local apk=$1 package=$2 serial out
  serial=$(need_device "make run")
  [ -f "$apk" ] || die "no APK at $apk (make apk)"
  say "adb install" "$(basename "$apk") -> $serial"
  "$ADB" -s "$serial" install -r "$apk"
  # The launcher's own intent, so it starts whichever activity is the launcher.
  out=$("$ADB" -s "$serial" shell monkey -p "$package" -c android.intent.category.LAUNCHER 1 2>&1 | tr -d '\r' || true)
  case "$out" in
    *"Events injected: 1"*) say "started" "$package" ;;
    *) die "could not start $package:" "$out" ;;
  esac
}

cmd_logcat() {
  local tag=$1 serial
  serial=$(need_device "make logcat")
  say "logcat" "$serial: $tag, AndroidRuntime, DEBUG, libc (Ctrl-C to stop)"
  exec "$ADB" -s "$serial" logcat -v time -s "$tag:V" AndroidRuntime:E DEBUG:F libc:F
}

cmd_run_as() {
  local package=$1 serial
  shift
  [ $# -ge 1 ] || die "usage: tools/device.sh run-as PACKAGE CMD..."
  serial=$(need_device "run-as")
  exec "$ADB" -s "$serial" shell run-as "$package" "$@"
}

cmd_uninstall() {
  local package=$1 serial
  serial=$(pick)
  if [ -n "$serial" ]; then
    say "adb uninstall" "$package on $serial"
    "$ADB" -s "$serial" uninstall "$package"
  elif have_adb && [ -z "$(local_fingerprint)" ]; then
    "$ADB" uninstall "$package"
  else
    die "uninstalling needs adb (make devices); or long-press the app's icon, or" \
      "Settings > Apps > $package > Uninstall."
  fi
}

# The launcher activity with one extra: a CONSOLE build reads it at start and
# leaves your App uncreated (`app start` creates it). Through adb when it
# reaches the device, else through Termux's own `am` (termux-am), which needs
# no adb: Termux is in front, so Android lets it start an activity.
cmd_console_only() {
  local package=$1 serial
  local intent=(start -n "$package/android.app.NativeActivity" --ez squared.console_only true)
  serial=$(pick)
  if [ -n "$serial" ]; then
    say "adb am start" "$package, with only the Console"
    "$ADB" -s "$serial" shell am "${intent[@]}"
  elif command -v am >/dev/null 2>&1; then
    say "am start" "$package, with only the Console"
    am "${intent[@]}"
  else
    die "console-only needs adb or Termux's am: pkg install termux-am"
  fi
  say "" "(if it was already running it carries on as it was: quit it first, or app stop)"
}

usage() { sed -n '2,16p' "$0" | sed 's/^# \{0,1\}//'; }

[ $# -ge 1 ] || {
  usage
  exit 2
}
command=$1
shift
args() { # args <wanted count, or "+1" for at least one> <usage> <given count>
  case $1 in
    +1) [ "$3" -ge 1 ] ;;
    *) [ "$3" -eq "$1" ] ;;
  esac || die "usage: tools/device.sh $2"
}
case $command in
  devices) cmd_devices ;;
  connect) cmd_connect "$@" ;;
  install)
    args 1 "install APK" $#
    cmd_install "$1"
    ;;
  run)
    args 2 "run APK PACKAGE" $#
    cmd_run "$1" "$2"
    ;;
  logcat)
    args 1 "logcat TAG" $#
    cmd_logcat "$1"
    ;;
  run-as)
    args +1 "run-as PACKAGE CMD..." $#
    cmd_run_as "$@"
    ;;
  uninstall)
    args 1 "uninstall PACKAGE" $#
    cmd_uninstall "$1"
    ;;
  console-only)
    args 1 "console-only PACKAGE" $#
    cmd_console_only "$1"
    ;;
  -h | --help | help) usage ;;
  *)
    usage >&2
    exit 2
    ;;
esac
