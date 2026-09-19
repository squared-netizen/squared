# Refactor notes

## What changed

- The archive's flattened layout was restored to
  `<module>/include/squared/<module>/`, which is what every `#include` in the
  tree already expected.
- Headers were moved to the module matching their namespace. `gui/include/`
  had been holding headers for four namespaces: `graphics`, `graphics2d`,
  `scene2d` and `gui`.
- `gui.hpp` (2,719 lines, 55 types) and `gui.cpp` (3,281 lines) were split
  into one file per type, along with `bitmap_font.hpp` (13 types),
  `texture.hpp` (7), `input.hpp` (9), `skin_loader.hpp` (6) and
  `timepiece.hpp` (3).
- Include sets were recomputed per file, forward declaring where the rule in
  [file-layout.md](file-layout.md) allows it.
- Anonymous-namespace helpers shared by several now-separate translation units
  moved to `src/detail/` as `inline` entities in a `detail` namespace.
- A make/g++ build and two bash scripts were added; there was no build system
  in the archive.

## What did not change

No declaration and no definition was rewritten, reordered within its own body,
reformatted, or renamed. Behaviour is identical.

This is verified rather than asserted. The splitting tools parse each original
file into namespace-scope segments and re-emit each segment verbatim; a
round-trip check reassembles every segment and compares against the original
namespace body. All 19 headers and 10 sources round-trip exactly.

## Files kept whole

`gui/src/skin_loader.cpp` and `gui/src/skin_atlas_resolver.cpp` are copied
byte for byte. They contain free functions and one 781-line
single-translation-unit helper namespace, not classes, so there is nothing to
split.

## How the include sets were proved

The forward-declare-or-include rule is a heuristic and cannot be sound on its
own, so the compiler decides. `tools/converge.py` generates the tree, compiles
every header standalone and every source, parses the errors, records each
demotion the compiler rejected, and re-runs. It converged in two passes and
found four pairs that need a definition despite naming the type only by
reference; they are listed in [file-layout.md](file-layout.md).

Final state: 113 public headers compile standalone and 40 translation units
compile, with `-std=c++20 -Wall -Wextra -Wpedantic`, zero failures and zero
warnings.

## Verification performed

| Check | Result |
|---|---|
| Segment round-trip against originals | 19 headers, 10 sources, 0 mismatches |
| Every public header compiled standalone | 113 headers, 0 failures |
| Every translation unit compiled | 40 sources, 0 failures |
| Full `make` build | 5 archives, 0 warnings |
| `-fno-rtti` build | clean, 0 errors, 0 warnings |
| `-fno-exceptions` probe | 21 errors, catalogued by kind |

## RTTI removal pass

A separate pass after the split, touching nine files:

- new `scene2d/include/squared/scene2d/actor_interface_id.hpp`
- `Actor` gains `actor_interface()` and the `actor_cast` templates
- `Group` and `gui::Widget` publish identifiers and override the query
- `Widget` gains `wants_text_input()`; `TextField` overrides it
- 17 `dynamic_cast` call sites replaced across `ui.cpp`, `widget.cpp` and
  `stack.cpp`
- `ui.cpp` drops `#include <squared/gui/text_field.hpp>`, now unused

Verified: `make SQUARED_ABI=-fno-rtti` builds all five archives with zero
warnings, 114 headers remain self-contained, and a behavioural test links
under `-fno-rtti` and asserts `actor_cast` answers exactly what `dynamic_cast`
answered. No `dynamic_cast` or `typeid` remains in the tree.

## Not done

- **clang-format and clang-tidy were not run.** Neither tool was available.
- The priority-order findings in [priority-audit.md](priority-audit.md) are
  reported, not fixed. Each one is an API or behavioural change and belongs in
  its own reviewable commit.
- No test suite was present in the archive, so behavioural equivalence rests on
  the round-trip check and the compiler, not on tests.
