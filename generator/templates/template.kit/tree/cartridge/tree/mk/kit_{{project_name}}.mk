# kit.{{project_name}} — build fragment.
#
# Included automatically: the workspace Makefile picks up mk/kit_*.mk by
# wildcard, so this fragment is live the moment the kit is installed. The name
# carries the kit's own name so two kits never collide.
#
# A kit ADDS to a workspace's build; it does not reorganise it
# (Generator Architecture 2.9). Two rules that came from real build failures:
#
#   - additional sysroots go in with -idirafter, never -I. -I prepends and
#     will displace the host's own C headers, at which point libc++ stops
#     finding them and the error appears far from its cause.
#   - name libraries by path rather than adding -L and using -l. An added -L
#     changes resolution for every -l in the link, including ones this kit
#     knows nothing about.
#
# The test: if your fragment only works when it comes first, it is
# reorganising the toolchain rather than adding to it.

# CXXFLAGS += -Isq_kit/{{project_name}}/include
# LDLIBS   += /usr/lib/lib{{project_name}}.a
