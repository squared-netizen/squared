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
#     zip                          -> inject lib<name>.so and libc++_shared.so
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

.PHONY: apk install uninstall android-status android-help keystore

apk: $(SQ_APK)

$(SQ_APK_DIR)/resources.zip: $(shell find $(SQ_RES_DIR) -type f 2>/dev/null)
	@mkdir -p $(SQ_APK_DIR)
	@echo "  aapt2 compile  $(SQ_RES_DIR)"
	@$(SQ_AAPT2) compile --dir $(SQ_RES_DIR) -o $@

# ---------------------------------------------------------------------------
# Assets
#
# SFML reads every file it opens on Android through AAssetManager, which sees
# only what packaging placed under assets/ in the APK. Without the -A below,
# sq_android/assets/ exists, looks right, and reaches nothing: sf::Font,
# sf::Texture and sf::Music all fail to open at runtime with no build-time
# complaint.
#
# The stamp exists because a file list is an unreliable prerequisite. Removing
# the last asset changes no prerequisite at all, so make would relink nothing
# and the APK would keep shipping a file the tree no longer has. A stamp
# holding names and mtimes, sorted, flips on add, rename, remove and touch
# alike.
#
# -A is passed only when the directory holds something: aapt2 rejects an empty
# asset directory, and a freshly generated project has one.

SQ_ASSETS_DIR  ?= sq_android/assets
SQ_ASSET_FILES := $(shell find $(SQ_ASSETS_DIR) -type f 2>/dev/null)
SQ_ASSET_STAMP := $(SQ_APK_DIR)/.assets.stamp

.PHONY: FORCE
FORCE:

$(SQ_ASSET_STAMP): FORCE
	@mkdir -p $(SQ_APK_DIR)
	@list=$$(find $(SQ_ASSETS_DIR) -type f -printf '%T@ %p\n' 2>/dev/null | sort); \
	 old=$$(cat $(SQ_ASSET_STAMP) 2>/dev/null || true); \
	 if [ "$$old" != "$$list" ]; then printf '%s\n' "$$list" > $@; fi

$(SQ_APK_DIR)/base.apk: $(SQ_APK_DIR)/resources.zip $(SQ_MANIFEST) $(SQ_ASSET_STAMP)
	@if [ -z "$(SQ_ANDROID_JAR)" ]; then \
	  echo "no android.jar found. Set SQ_ANDROID_JAR, or see 'make android-help'." >&2; \
	  exit 1; \
	fi
	@echo "  aapt2 link     $(notdir $@)"
	@$(SQ_AAPT2) link -o $@ \
	    -I $(SQ_ANDROID_JAR) \
	    --manifest $(SQ_MANIFEST) \
	    --min-sdk-version $(SQ_MIN_SDK) \
	    --target-sdk-version $(SQ_TARGET_SDK) \
            $(if $(strip $(SQ_ASSET_FILES)),-A $(SQ_ASSETS_DIR),) \
	    $(SQ_APK_DIR)/resources.zip

# The libraries the APK carries. libc++_shared.so is not optional: Termux's
# clang links the shared C++ runtime unconditionally -- -static-libstdc++ fails
# with "unable to find library -lc++_shared" -- and Android does not supply it.
# Without it the APK installs and then dies on launch before any of your code
# runs.
#
# It is stripped on the way in. The NDK ships it unstripped at ~9 MB, of which
# the overwhelming majority is debug information; stripping brings it under
# half a megabyte. Shipping 9 MB of someone else's debug symbols in every APK
# would be careless.
$(SQ_APK_DIR)/lib/$(SQ_ABI)/libc++_shared.so: $(SQ_LIBCXX)
	@mkdir -p $(dir $@)
	@if [ ! -f "$(SQ_LIBCXX)" ]; then \
	  echo "libc++_shared.so not found at $(SQ_LIBCXX)" >&2; \
	  echo "the APK would install and crash on launch; refusing to build it" >&2; \
	  exit 1; \
	fi
	@cp $(SQ_LIBCXX) $@
	@$(SQ_STRIP) --strip-unneeded $@ 2>/dev/null || true
	@echo "  bundled        libc++_shared.so ($$(wc -c < $@) bytes, stripped)"

$(SQ_APK_DIR)/lib/$(SQ_ABI)/lib$(SQ_PROJECT).so: $(SQ_SO)
	@mkdir -p $(dir $@)
	@cp $< $@

$(SQ_APK_UNSIGNED): $(SQ_APK_DIR)/base.apk \
                    $(SQ_APK_DIR)/lib/$(SQ_ABI)/lib$(SQ_PROJECT).so \
                    $(SQ_APK_DIR)/lib/$(SQ_ABI)/libc++_shared.so
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

install: $(SQ_APK)
	@if command -v termux-open >/dev/null 2>&1; then \
	  echo "  handing $(SQ_APK) to the package installer"; \
	  termux-open $(SQ_APK); \
	elif command -v adb >/dev/null 2>&1; then \
	  adb install -r $(SQ_APK); \
	else \
	  echo "no installer found. The APK is at $(SQ_APK)." >&2; \
	fi

uninstall:
	@if command -v pm >/dev/null 2>&1; then pm uninstall $(SQ_PACKAGE); \
	elif command -v adb >/dev/null 2>&1; then adb uninstall $(SQ_PACKAGE); \
	else echo "no uninstaller available" >&2; fi

android-status:
	@echo "toolchain"
	@printf '  %-14s %s\n' "ndk" "$(if $(SQ_NDK),$(SQ_NDK),NOT FOUND)"
	@printf '  %-14s %s\n' "sysroot" "$(if $(wildcard $(SQ_NDK_INC)),$(SQ_NDK_INC),NOT FOUND)"
	@printf '  %-14s %s\n' "stub libs" "$(if $(wildcard $(SQ_NDK_LIB)),$(SQ_NDK_LIB),NOT FOUND)"
	@printf '  %-14s %s\n' "glue" "$(if $(wildcard $(SQ_GLUE_SRC)),$(SQ_GLUE_SRC),NOT FOUND)"
	@printf '  %-14s %s\n' "libc++" "$(if $(wildcard $(SQ_LIBCXX)),$(SQ_LIBCXX),NOT FOUND)"
	@echo "packaging"
	@printf '  %-14s %s\n' "android.jar" "$(if $(SQ_ANDROID_JAR),$(SQ_ANDROID_JAR),NOT FOUND)"
	@printf '  %-14s %s\n' "aapt2" "$$(command -v $(SQ_AAPT2) || echo 'NOT FOUND')"
	@printf '  %-14s %s\n' "apksigner" "$$(command -v $(SQ_APKSIGNER) || echo 'NOT FOUND')"
	@printf '  %-14s %s\n' "keystore" "$(if $(wildcard $(SQ_KEYSTORE)),$(SQ_KEYSTORE),will be generated)"
	@echo "target"
	@printf '  %-14s %s\n' "abi" "$(SQ_ABI) ($(SQ_TRIPLE))"
	@printf '  %-14s %s\n' "sdk" "min $(SQ_MIN_SDK), target $(SQ_TARGET_SDK)"
	@printf '  %-14s %s\n' "kits" "$(if $(SQ_KITS_PRESENT),$(SQ_KITS_PRESENT),none)"
	@printf '  %-14s %s\n' "assets" "$(if $(strip $(SQ_ASSET_FILES)),$(words $(SQ_ASSET_FILES)) file(s),none - nothing will be packaged)"

android-help:
	@echo "$(SQ_PROJECT) — Android targets"
	@echo
	@echo "  make                build lib$(SQ_PROJECT).so"
	@echo "  make apk            package and sign"
	@echo "  make install        hand the APK to the package installer"
	@echo "  make uninstall      remove $(SQ_PACKAGE)"
	@echo "  make android-status what the build found on this host"
	@echo "  make logcat         follow this app's log"
	@echo
	@echo "overrides"
	@echo "  SQ_MIN_SDK=21       retarget without regenerating"
	@echo "  SQ_ABI=armeabi-v7a  a different ABI (needs a toolchain that targets it)"
	@echo "  SQ_ANDROID_JAR=...  a specific platform jar"
	@echo "  SQ_NDK=...          a specific NDK"
	@echo
	@echo "This host builds one ABI: $(SQ_ABI). Termux's clang targets aarch64"
	@echo "only. A desktop NDK can build others by overriding SQ_ABI, one"
	@echo "invocation each, then merging the lib/ directories before signing."

.PHONY: logcat
logcat:
	@logcat -s $(SQ_PROJECT)
