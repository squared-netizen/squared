# kit.{{project_name}} — authoring workspace.
#
# This file is YOURS (ownership class: seeded). The generator wrote it once and
# will not overwrite it.
#
# The deliverable of this workspace is a cartridge, not a binary. Everything
# under cartridge/ is the kit; everything beside it is scaffolding that helps
# you build and check it.
#
#   make check      validate the cartridge without packing it
#   make dist       pack cartridge/ into dist/kit.{{project_name}}-<version>.sq
#   make install    copy the cartridge into your environment's resources
#   make clean
#
# There is no `format` or `lint` target here. Those are contributed by kits
# (kit.format, kit.lint) through mk/kit_*.mk, and this workspace resolved
# none. `sqpg lint --explain` will show you the command it would run and
# `sqpg lint` will tell you no kit provides it, which is the honest answer
# rather than a target that silently does nothing.
#
# `make` with no target runs check, because the only thing you can "build"
# here is confidence that the cartridge is well-formed.

.DEFAULT_GOAL := check

include mk/squared_generated.mk
-include $(wildcard mk/kit_*.mk)

# ---------------------------------------------------------------------------
# check
# ---------------------------------------------------------------------------
#
# This is the resource-in-isolation check that Generator Architecture 2.16.5
# names as a gap. `sqcart verify` reads the manifest and the payload of the
# cartridge you are editing, at the moment you are editing it -- which is a
# better place for the check than anywhere the container library could have
# put it, because here the answer arrives before the cartridge ships.

.PHONY: check
check:
	@if [ ! -x "$(SQCART)" ]; then \
	  echo "make: sqcart not found."; \
	  echo "      looked in:"; \
	  echo "        \$$SQCART                      $(if $(SQCART),$(SQCART),(unset))"; \
	  echo "        \$$SQSYSROOT/.sqpg/bin/sqcart  $(SQSYSROOT)/.sqpg/bin/sqcart"; \
	  echo "        \$$PATH"; \
	  echo ""; \
	  echo "      run \`sqpg initialize\` to install it, or set SQCART=/path/to/sqcart"; \
	  exit 1; \
	fi
	@echo "  verify         cartridge/"
	@$(SQCART) verify cartridge
	@echo "  ok             $(SQ_CART_ID)"

# ---------------------------------------------------------------------------
# dist
# ---------------------------------------------------------------------------
#
# The version comes out of the manifest rather than being repeated here.
# Two places to change one number is one place too many, and the manifest is
# the one that ships.

.PHONY: dist
dist: check
	@mkdir -p $(DIST)
	@$(SQCART) pack cartridge -o $(DIST)/$(SQ_CART_ID)-$(SQ_CART_VERSION).sq $(SQ_PACK_FLAGS)
	@echo "  packed         $(DIST)/$(SQ_CART_ID)-$(SQ_CART_VERSION).sq"

# ---------------------------------------------------------------------------
# install
# ---------------------------------------------------------------------------
#
# Exploded, not packed. An exploded cartridge is byte-identical in meaning to
# an archive (cartridge format 3.5) and the engine reads either, so installing
# the directory means an edit here is visible to `sqpg` on the next run with
# no repack step. That shortens the loop from edit-pack-install-test to
# edit-test.

.PHONY: install
install: check
	@if [ -z "$(SQ_RESOURCES)" ]; then \
	  echo "make: no environment found"; \
	  echo "      run \`sqpg initialize\`, or set SQ_RESOURCES=/path/to/resources"; \
	  exit 1; \
	fi
	@mkdir -p $(SQ_RESOURCES)/kits/$(SQ_CART_ID)
	@cp -a cartridge/. $(SQ_RESOURCES)/kits/$(SQ_CART_ID)/
	@echo "  installed      $(SQ_RESOURCES)/kits/$(SQ_CART_ID)"

.PHONY: clean
clean:
	@rm -rf $(DIST)
	@echo "  cleaned        $(DIST)"
