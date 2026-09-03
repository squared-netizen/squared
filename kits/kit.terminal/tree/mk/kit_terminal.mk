# kit.terminal build fragment.
#
# GENERATED FILE. kit.terminal owns this path and will replace it on update.
# Project-wide build settings belong in the Makefile, which is yours.
#
# The kit is header-only, so integration is one include path and the standard
# library. Nothing to link, nothing to install, nothing to detect -- which is
# why a generated workspace using only this kit builds on a bare Termux with
# clang and make.

SQ_KIT_TERMINAL_INCLUDE := -Isq_kit/include

SQ_KIT_CPPFLAGS += $(SQ_KIT_TERMINAL_INCLUDE)
SQ_KITS_PRESENT += kit.terminal
