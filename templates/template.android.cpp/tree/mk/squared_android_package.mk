# Squared generated packaging rules — Android.
#
# GENERATED FILE — DO NOT EDIT.
#
# Split from squared_generated.mk so that variables and rules are separable: a
# host that only builds the .so never evaluates any of this, and the detection
# above stays readable.
#
# The chain, verified end to end on Termux/aarch64 before this was written:
#
#     aapt2 compile --dir res      -> compiled resources
#     aapt2 link -I android.jar    -> base APK, binary manifest, resources.arsc
#     zip                          -> inject lib<name>.so
#     apksigner sign               -> installable
#
# Notably absent: zipalign. It is not packaged on Termux, and
# android:extractNativeLibs="true" in the manifest means the libraries are
# stored compressed and extracted by the installer, so page alignment never
# arises. Android 15+ wants 16 KiB alignment for uncompressed libraries, which
# is not something to improvise without the tool.

SQ_APK_DIR      := $(BUILD)/apk
SQ_APK_UNSIGNED := $(SQ_APK_DIR)/unsigned.apk
SQ_APK          := $(BUILD)/$(SQ_PROJECT).apk

.PHONY: apk install run devices uninstall console-only android-status android-help keystore

apk: $(SQ_APK)

# ---------------------------------------------------------------------------
# Debuggable development builds
#
# DEV_LOG and Console builds are linked with aapt2's --debug-mode, which writes
# android:debuggable="true" into the APK's manifest. Plain builds never are.
# A debuggable app is one `run-as` reaches (adb shell run-as <package> ...,
# the app's private files without root), and one a native debugger or
# simpleperf may attach to. Your manifest is not edited: a committed
# debuggable would ship in a release.
#
#   make SQ_DEBUGGABLE= install    a Console or DEV_LOG build that is not
#   make SQ_DEBUGGABLE=1 install   a plain build that is
#
# The cost is in the Java runtime, which runs a debuggable process with its
# debugging support on; this app's own code is the same machine code either
# way. Measure memory on a build that is not debuggable.
#
# The setting is stamped like the compiler flags, so switching it relinks the
# APK and rebuilds nothing else.
SQ_DEBUGGABLE ?= $(if $(or $(DEV_LOG),$(filter-out 0,$(CONSOLE))),1,)
SQ_AAPT2_LINK_FLAGS := $(if $(strip $(SQ_DEBUGGABLE)),--debug-mode,)
SQ_LINK_STAMP := $(SQ_APK_DIR)/.link-flags
$(shell mkdir -p '$(SQ_APK_DIR)'; [ -f '$(SQ_LINK_STAMP)' ] && [ "$$(cat '$(SQ_LINK_STAMP)')" = 'link $(SQ_AAPT2_LINK_FLAGS)' ] || printf '%s' 'link $(SQ_AAPT2_LINK_FLAGS)' > '$(SQ_LINK_STAMP)')

$(SQ_APK_DIR)/resources.zip: $(shell find $(SQ_RES_DIR) -type f 2>/dev/null)
	@mkdir -p $(SQ_APK_DIR)
	@echo "  aapt2 compile  $(SQ_RES_DIR)"
	@$(SQ_AAPT2) compile --dir $(SQ_RES_DIR) -o $@

# ---------------------------------------------------------------------------
# Assets
#
# squared reads bundled files through AAssetManager, which sees only what
# packaging placed under assets/ in the APK. Without the -A below,
# sq_android/assets/ exists, looks right, and reaches nothing:
# fs.internal(...) fails at runtime with no build-time complaint.
#
# Same mechanism as template.android.sfml, deliberately, so the two templates
# package assets identically.
#
# The stamp exists because a file list is an unreliable prerequisite. Removing
# the last asset changes no prerequisite at all, so make would relink nothing
# and the APK would keep shipping a file the tree no longer has. A stamp
# holding names and mtimes, sorted, flips on add, rename, remove and touch
# alike.
#
# -A is passed only when the directory holds something: aapt2 rejects an empty
# asset directory, and a freshly generated project may have one.

SQ_ASSETS_DIR  ?= sq_android/assets

SQ_ASSET_INDEX := $(SQ_ASSETS_DIR)/SQ-INF/index

# Real assets: everything except squared's own index, which is derived from
# them. Excluding it here is what stops the index counting towards "is there
# anything to package".
#
# SQ-INF/ is bundle metadata - the same convention sqcart uses for
# SQ-INF/manifest.json - and never content, so all of it is excluded. Metadata
# on its own is not "something to package".
SQ_ASSET_FILES := $(shell find $(SQ_ASSETS_DIR) -type f ! -path '$(SQ_ASSETS_DIR)/SQ-INF/*' 2>/dev/null)
SQ_ASSET_STAMP := $(SQ_APK_DIR)/.assets.stamp

# ---------------------------------------------------------------------------
# The asset index
#
# AAssetDir, the NDK's only way to walk assets, lists files and never
# subdirectories. squared answers directory questions from this index instead:
# every asset path, one per line, sorted. See the framework's
# docs/developer/asset-index.md.
#
# It lives inside the APK at assets/SQ-INF/index, so it is replaced wholesale
# on every update along with the assets it describes, and cannot go stale.
#
# Not a dot-directory: aapt2 skips hidden files and directories when packaging
# assets, without a warning. An index in a hidden directory was generated,
# passed to -A, and never reached the APK. aapt's default pattern also skips
# directories beginning with an underscore, so _SQ-INF/ would fail the same
# way.
#
# Written only when its content changes. The stamp below lists every file
# under assets/, the index included; rewriting it unconditionally would flip
# the stamp and relink the APK on every build.
#
# Removed when there are no real assets, so an empty project ships no index
# and no -A. Only the index file is removed, and SQ-INF/ only if that leaves
# it empty: nothing here may delete a file it did not write.

.PHONY: FORCE
FORCE:

$(SQ_ASSET_INDEX): FORCE
	@if [ -n "$(strip $(SQ_ASSET_FILES))" ]; then \
	  mkdir -p $(dir $@); \
	  new=$$(cd $(SQ_ASSETS_DIR) && find . -type f ! -path './SQ-INF/*' \
	         | sed 's|^\./||' | LC_ALL=C sort); \
	  old=$$(cat $@ 2>/dev/null || true); \
	  if [ "$$old" != "$$new" ]; then \
	    printf '%s\n' "$$new" > $@; \
	    echo "  asset index    $$(printf '%s\n' "$$new" | wc -l) path(s)"; \
	  fi; \
	else \
	  rm -f $(SQ_ASSET_INDEX); \
	  rmdir $(SQ_ASSETS_DIR)/SQ-INF 2>/dev/null || true; \
	fi

$(SQ_ASSET_STAMP): $(SQ_ASSET_INDEX) FORCE
	@mkdir -p $(SQ_APK_DIR)
	@list=$$(find $(SQ_ASSETS_DIR) -type f -printf '%T@ %p\n' 2>/dev/null | sort); \
	 old=$$(cat $(SQ_ASSET_STAMP) 2>/dev/null || true); \
	 if [ "$$old" != "$$list" ]; then printf '%s\n' "$$list" > $@; fi

# A debuggable link is checked, not assumed: the line below reads the linked
# manifest back. An aapt2 that cannot print it says so and the build goes on;
# one that prints a manifest without the attribute fails the build.
$(SQ_APK_DIR)/base.apk: $(SQ_APK_DIR)/resources.zip $(SQ_MANIFEST) $(SQ_ASSET_STAMP) $(SQ_LINK_STAMP)
	@if [ -z "$(SQ_ANDROID_JAR)" ]; then \
	  echo "no android.jar found. Set SQ_ANDROID_JAR, or see 'make android-help'." >&2; \
	  exit 1; \
	fi
	@echo "  aapt2 link     $(notdir $@)$(if $(SQ_AAPT2_LINK_FLAGS), (debuggable),)"
	@$(SQ_AAPT2) link -o $@ \
	    -I $(SQ_ANDROID_JAR) \
	    --manifest $(SQ_MANIFEST) \
	    --min-sdk-version $(SQ_MIN_SDK) \
	    --target-sdk-version $(SQ_TARGET_SDK) \
	    $(SQ_AAPT2_LINK_FLAGS) \
	    $(if $(strip $(SQ_ASSET_FILES)),-A $(SQ_ASSETS_DIR),) \
	    $(SQ_APK_DIR)/resources.zip
	@if [ -n "$(SQ_AAPT2_LINK_FLAGS)" ]; then \
	  manifest=$$($(SQ_AAPT2) dump xmltree --file AndroidManifest.xml $@ 2>/dev/null) || { \
	    echo "  debuggable     asked for; this aapt2 cannot print the manifest to check"; exit 0; }; \
	  case "$$manifest" in \
	    *debuggable*) echo "  debuggable     yes: run-as reaches it (SQ_DEBUGGABLE= for a build that is not)" ;; \
	    *) echo "aapt2 link --debug-mode left android:debuggable out of the manifest" >&2; rm -f $@; exit 1 ;; \
	  esac; \
	fi

# The one library the APK carries. The C++ runtime is linked into it (see
# SQ_CXX_RUNTIME in mk/squared_generated.mk), so there is no libc++_shared.so to
# bundle, and no chance of bundling one that disagrees with the headers.
#
# The APK's copy is stripped; build/lib$(SQ_PROJECT).so keeps its symbols for
# addr2line. --strip-unneeded keeps the dynamic symbol table, which the loader
# and dladdr() read, so the crash report names functions as before. Debug
# sections are never loaded into memory, so this saves APK and install space,
# not RAM; the statically linked runtime's archives carry most of them.
$(SQ_APK_DIR)/lib/$(SQ_ABI)/lib$(SQ_PROJECT).so: $(SQ_SO)
	@mkdir -p $(dir $@)
	@cp $< $@
	@$(SQ_STRIP) --strip-unneeded $@ 2>/dev/null || true
	@echo "  bundled        lib$(SQ_PROJECT).so ($$(wc -c < $@) bytes, stripped; C++ runtime inside)"

# A libc++_shared.so left in lib/ by a build from before the runtime was linked
# in is removed, so the zip below cannot pick it up.
$(SQ_APK_UNSIGNED): $(SQ_APK_DIR)/base.apk \
                    $(SQ_APK_DIR)/lib/$(SQ_ABI)/lib$(SQ_PROJECT).so
	@rm -f $(SQ_APK_DIR)/lib/$(SQ_ABI)/libc++_shared.so
	@cp $(SQ_APK_DIR)/base.apk $@
	@echo "  packaging      lib/$(SQ_ABI)/"
	@cd $(SQ_APK_DIR) && zip -q -X $(notdir $@) lib/$(SQ_ABI)/*.so

keystore: $(SQ_KEYSTORE)

$(SQ_KEYSTORE):
	@echo "  keytool        generating a debug keystore"
	@mkdir -p $(dir $@)
	@$(SQ_KEYTOOL) -genkeypair -keystore $@ -alias $(SQ_KEY_ALIAS) \
	    -storepass $(SQ_KEY_PASS) -keypass $(SQ_KEY_PASS) \
	    -keyalg RSA -keysize 2048 -validity 10000 \
	    -dname 'CN=Android Debug,O=Android,C=US'

$(SQ_APK): $(SQ_APK_UNSIGNED) $(SQ_KEYSTORE)
	@echo "  apksigner      $(notdir $@)"
	@$(SQ_APKSIGNER) sign --ks $(SQ_KEYSTORE) \
	    --ks-pass pass:$(SQ_KEY_PASS) --key-pass pass:$(SQ_KEY_PASS) \
	    --ks-key-alias $(SQ_KEY_ALIAS) --out $@ $(SQ_APK_UNSIGNED)
	@$(SQ_APKSIGNER) verify $@ >/dev/null && echo "  verified       $@ ($$(wc -c < $@) bytes)"

# ---------------------------------------------------------------------------
# Installing, starting, reading the log
#
# tools/device.sh decides where an APK goes; these targets only hand it the
# names. With adb connected to this phone (Wireless Debugging, see
# `make devices`), `make install` uses `adb install -r`: no installer prompt.
# Without it, the package installer, as before. Another device (the tablet) is
# used only when named: `make install DEVICE=<serial>`, as `make devices`
# lists it. SQ_ADB picks the adb (`SQ_ADB=termux-adb` for a USB device).
SQ_ADB ?= adb
DEVICE ?=
SQ_DEVICE_TOOL = SQ_ADB='$(SQ_ADB)' DEVICE='$(DEVICE)' bash tools/device.sh

install: $(SQ_APK)
	@$(SQ_DEVICE_TOOL) install '$(SQ_APK)'

# Install, then start it: needs adb, since the installer's prompt cannot be
# waited for.
run: $(SQ_APK)
	@$(SQ_DEVICE_TOOL) run '$(SQ_APK)' '$(SQ_PACKAGE)'

devices:
	@$(SQ_DEVICE_TOOL) devices

# A CONSOLE build, started with only its Console: your App is not created
# until `app start`, so an App that crashes at start can still be reached
# (`make console`, then `crash`, `log previous`, `app start`).
#
# Android only brings a running app to the front, and the extra never reaches
# it, so a running CONSOLE build is asked to quit first, through the bridge,
# and this waits until it has gone (the connection closing says so). With no
# app running, sqconsole fails at once (--wait 0) and nothing is waited for.
console-only: $(if $(SQ_BRIDGE_CONF),$(SQ_SQCONSOLE),)
	@if [ -f '$(SQ_BRIDGE_CONF)' ] && \
	   '$(SQ_SQCONSOLE)' --conf '$(SQ_BRIDGE_CONF)' --wait 0 --run quit --until-closed \
	     >/dev/null 2>&1; then \
	  printf '  %-14s %s\n' "quit" "the running app, so it starts again with only the Console"; \
	fi
	@$(SQ_DEVICE_TOOL) console-only '$(SQ_PACKAGE)'

uninstall:
	@$(SQ_DEVICE_TOOL) uninstall '$(SQ_PACKAGE)'

android-status:
	@echo "toolchain"
	@printf '  %-14s %s\n' "ndk" "$(if $(SQ_NDK),$(SQ_NDK),NOT FOUND)"
	@printf '  %-14s %s\n' "sysroot" "$(if $(wildcard $(SQ_NDK_INC)),$(SQ_NDK_INC),NOT FOUND)"
	@printf '  %-14s %s\n' "stub libs" "$(if $(wildcard $(SQ_NDK_LIB)),$(SQ_NDK_LIB),NOT FOUND)"
	@printf '  %-14s %s\n' "glue" "$(if $(wildcard $(SQ_GLUE_SRC)),$(SQ_GLUE_SRC),NOT FOUND)"
	@printf '  %-14s %s\n' "C++ runtime" "$(if $(wildcard $(SQ_NDK_LIB_ABI)/libc++_static.a),$(SQ_NDK_LIB_ABI)/libc++_static.a (linked in),NOT FOUND)"
	@printf '  %-14s %s\n' "unwinder" "$(if $(SQ_NDK_UNWIND),$(SQ_NDK_UNWIND),NOT FOUND)"
	@echo "packaging"
	@printf '  %-14s %s\n' "android.jar" "$(if $(SQ_ANDROID_JAR),$(SQ_ANDROID_JAR),NOT FOUND)"
	@printf '  %-14s %s\n' "aapt2" "$$(command -v $(SQ_AAPT2) || echo 'NOT FOUND')"
	@printf '  %-14s %s\n' "apksigner" "$$(command -v $(SQ_APKSIGNER) || echo 'NOT FOUND')"
	@printf '  %-14s %s\n' "keystore" "$(if $(wildcard $(SQ_KEYSTORE)),$(SQ_KEYSTORE),will be generated)"
	@printf '  %-14s %s\n' "debuggable" "$(if $(SQ_AAPT2_LINK_FLAGS),yes (a DEV_LOG or Console build; SQ_DEBUGGABLE= for no),no)"
	@printf '  %-14s %s\n' "adb" "$$(command -v $(SQ_ADB) || echo 'not found (make devices)')"
	@echo "target"
	@printf '  %-14s %s\n' "abi" "$(SQ_ABI) ($(SQ_TRIPLE))"
	@printf '  %-14s %s\n' "sdk" "min $(SQ_MIN_SDK) (compiled and linked for it), target $(SQ_TARGET_SDK)"
	@printf '  %-14s %s\n' "kits" "$(if $(SQ_KITS_PRESENT),$(SQ_KITS_PRESENT),none)"
	@printf '  %-14s %s\n' "assets" "$(if $(strip $(SQ_ASSET_FILES)),$(words $(SQ_ASSET_FILES)) file(s),none - nothing will be packaged)"

android-help:
	@echo "$(SQ_PROJECT) — Android targets"
	@echo
	@echo "  make                build lib$(SQ_PROJECT).so"
	@echo "  make apk            package and sign"
	@echo "  make install        adb install -r on this phone when adb reaches it,"
	@echo "                      else hand the APK to the package installer"
	@echo "  make run            install, then start it (needs adb)"
	@echo "  make logcat         this app's log, and Android's crash lines (needs adb)"
	@echo "  make devices        what adb reaches, which is this phone, how to connect"
	@echo "  make console        a CONSOLE build's Console in this terminal; CMD='...' runs one"
	@echo "  make console-only   start it with only the Console; app start starts your App"
	@echo "  make uninstall      remove $(SQ_PACKAGE)"
	@echo "  make android-status what the build found on this host"
	@echo
	@echo "overrides"
	@echo "  SQ_MIN_SDK=24       build for another level, without regenerating"
	@echo "                      (the NDK's oldest is the lowest; default 22)"
	@echo "  SQ_ABI=armeabi-v7a  a different ABI (needs a toolchain that targets it)"
	@echo "  SQ_ANDROID_JAR=...  a specific platform jar"
	@echo "  SQ_NDK=...          a specific NDK"
	@echo "  DEVICE=<serial>     install, run or read the log on that device (make devices)"
	@echo "  SQ_ADB=termux-adb   another adb, e.g. for a device on USB"
	@echo "  SQ_DEBUGGABLE=      a DEV_LOG or Console build that is not debuggable"
	@echo
	@echo "This host builds one ABI: $(SQ_ABI). Termux's clang targets aarch64"
	@echo "only. A desktop NDK can build others by overriding SQ_ABI, one"
	@echo "invocation each, then merging the lib/ directories before signing."

# Termux's own logcat shows only Termux's lines (an app reads only its own
# log), so this needs adb. The app's tag, plus where Android reports what the
# app could not: AndroidRuntime (a library that failed to load), DEBUG (the
# crash summary) and libc (an abort's message).
.PHONY: logcat
logcat:
	@$(SQ_DEVICE_TOOL) logcat '$(SQ_PROJECT)'
