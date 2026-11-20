# build/toolchain.mk: the SDK's toolchain, for make. One definition,
# build/toolchain.sh's: this only reads it ("One C++23 toolchain:
# transition plan"). Included, it sets
#
#   SQ_TOOLCHAIN_FINGERPRINT   names every folder of BMIs this build makes
#   SQ_TOOLCHAIN_FLAGS         on every compile and link (C and C++)
#   SQ_TOOLCHAIN_CXXFLAGS      on every C++ compile
#   SQ_TOOLCHAIN_LDFLAGS       on every link
#   SQ_LLVM_BIN, SQ_SYSROOT, SQ_RUNTIMES, SQ_TRIPLE, SQ_STD_CPPM
#
# and, for a build that wants the wrapped compilers, the rule
# $(SQ_TOOLCHAIN_DIR)/clang++ (and clang), made by toolchain.sh wrappers.
# It fails at once, saying what is missing, when the toolchain is not there.

SQEDIT ?= sqedit

SQ_TOOLCHAIN_SH := $(abspath $(dir $(lastword $(MAKEFILE_LIST))))/toolchain.sh
SQ_TOOLCHAIN_ENV := $(shell bash '$(SQ_TOOLCHAIN_SH)' env 2>&1 || echo 'SQ_TOOLCHAIN_FAILED=1')
ifneq ($(findstring SQ_TOOLCHAIN_FAILED,$(SQ_TOOLCHAIN_ENV)),)
  $(error $(SQ_TOOLCHAIN_ENV))
endif
sq_toolchain = $(shell bash '$(SQ_TOOLCHAIN_SH)' env | $(SQEDIT) pick -x '"$(1)=" {.*}')
SQ_LLVM_BIN := $(call sq_toolchain,SQ_LLVM_BIN)
SQ_SYSROOT := $(call sq_toolchain,SQ_SYSROOT)
SQ_RUNTIMES := $(call sq_toolchain,SQ_RUNTIMES)
SQ_TRIPLE := $(call sq_toolchain,SQ_TRIPLE)
SQ_STD_CPPM := $(call sq_toolchain,SQ_STD_CPPM)
SQ_TOOLCHAIN_FLAGS := $(call sq_toolchain,SQ_TOOLCHAIN_FLAGS)
SQ_TOOLCHAIN_CXXFLAGS := $(call sq_toolchain,SQ_TOOLCHAIN_CXXFLAGS)
SQ_TOOLCHAIN_LDFLAGS := $(call sq_toolchain,SQ_TOOLCHAIN_LDFLAGS)
SQ_TOOLCHAIN_FINGERPRINT := $(call sq_toolchain,SQ_TOOLCHAIN_FINGERPRINT)

# The wrappers' rule does not become the including makefile's default goal.
sq_toolchain_goal := $(.DEFAULT_GOAL)
SQ_TOOLCHAIN_DIR ?= $(BUILD_DIR)/toolchain-$(SQ_TOOLCHAIN_FINGERPRINT)
$(SQ_TOOLCHAIN_DIR)/clang++ $(SQ_TOOLCHAIN_DIR)/clang &: $(SQ_TOOLCHAIN_SH)
	bash '$(SQ_TOOLCHAIN_SH)' wrappers '$(SQ_TOOLCHAIN_DIR)'
.DEFAULT_GOAL := $(sq_toolchain_goal)
