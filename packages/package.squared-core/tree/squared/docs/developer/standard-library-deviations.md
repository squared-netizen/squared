# Standard library deviations

The project's rule: the standard library is the default, but priority 1
outranks it, and several facilities are disqualified on memory grounds in
framework internals.

This page is the audit of where the tree currently **keeps** a disqualified
facility, and why its footprint was or was not acceptable. It is a record of
the existing state, not of choices made during the file split.

## Kept, and why it is currently acceptable

| Facility | Site | Assessment |
|---|---|---|
| `std::unordered_map` | `Skin`, ten members | **Not acceptable.** Largest resident cost in the framework. See [priority-audit.md](priority-audit.md) item 4. |
| `std::unordered_map` | `Ui::captures_` (pointer id &rarr; widget) | Bounded by simultaneous contacts, typically &le; 10. A flat vector of pairs would be smaller and faster at this size. |
| `std::function` | 8 public members | Interaction paths, not per-frame. `Widget::TooltipFactory` is the exception worth fixing: every widget pays 32 bytes. |
| `std::string` | style names on 7 widget types | 32 bytes per widget for a short constant. Interning is the fix. |
| `std::string` | `FontResource::descriptor_path_` | One per font resource, not per widget. Acceptable. |
| `std::shared_ptr` | `DrawablePtr`, `FontPtr` | Genuine shared ownership across skins and widgets, but a handle into a skin-owned pool would remove the control block and the atomics. |
| `std::deque<Cell>` | `Table::cells_` | Chosen for reference stability: `Table::add` returns `Cell&` and later adds must not invalidate it. A chunked vector or a stable pool would give the same guarantee with better locality. |
| `std::vector` | throughout | Fine. This is the intended default. |
| `std::optional` | `drag_pointer_`, `content_insets` | Fine; no allocation. |
| `std::array` | `NinePatchDrawable::regions_` | Fine, and the right call: 9 regions inline, no allocation. |
| `std::span` | `FontResource::pages()` | Fine; non-owning view, exactly as intended. |

## Passed over

Nothing in this tree passes over a standard facility in favour of a hand-
rolled one. `sq::Result`, an interned string table, a flat hash map, and a
handle pool are all named in the project rules and none exist yet. That is the
gap between the rules and the code, and it is tracked in the priority audit.

## `NinePatchDrawable` is worth a note

`sizeof(NinePatchDrawable)` is 400 bytes, driven by `std::array<TextureRegion,
9>` at 40 bytes per region. The nine regions are derived from one source
region plus four split values, so they are recomputable from 56 bytes. Storing
them precomputed trades 344 bytes per drawable for nine multiply-adds per
draw &mdash; a priority-3 win bought with priority-1 currency, which the
priority order says is the wrong direction. Skins typically hold tens of
nine-patches, so this is kilobytes, not megabytes; recorded, not urgent.
