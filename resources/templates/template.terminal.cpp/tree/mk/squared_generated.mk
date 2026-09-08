# Squared generated build settings.
#
# GENERATED FILE — DO NOT EDIT.
#
# The generator owns this path entirely and rewrites it on every update
# (ownership class: generated). Anything you write here is lost. Put your own
# settings in the Makefile, which is yours and which the generator will never
# touch.
#
# This is the split §2.7.10 prescribes instead of in-place region merging: a
# seeded file that includes a generated fragment. The generator gets a file it
# may rewrite freely; you get a file it will never rewrite at all.

SQ_PROJECT          := {{project_name}}
SQ_GENERATOR        := squared-pg
SQ_GENERATOR_VERSION := {{generator_version}}
SQ_TEMPLATE         := {{template_id}}@{{template_version}}
SQ_FRAMEWORK_VERSION := {{framework_version}}
SQ_WORKING_DIR      := {{working_directory}}

# Your own headers. Kit include paths are appended by the kit_*.mk fragments.
SQ_CPPFLAGS := -I$(SQ_WORKING_DIR)/include

# Accumulated by the kit fragments; declared here so the Makefile can reference
# them whether or not any kit was applied.
SQ_KIT_CPPFLAGS ?=
SQ_KIT_LDLIBS   ?=
SQ_KITS_PRESENT ?=

.PHONY: squared-info
squared-info:
	@echo "project    $(SQ_PROJECT)"
	@echo "generator  $(SQ_GENERATOR) $(SQ_GENERATOR_VERSION)"
	@echo "template   $(SQ_TEMPLATE)"
	@echo "framework  $(SQ_FRAMEWORK_VERSION)"
	@echo "kits       $(SQ_KITS_PRESENT)"
