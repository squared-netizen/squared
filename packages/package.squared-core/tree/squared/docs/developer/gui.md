# gui &mdash; internals

The libGDX-flavoured widget set, skinning, and the Ui host.

Programmer counterpart: [../programmer/gui.md](../programmer/gui.md)

- Public types: 57
- Translation units: 30

## Layout

```
gui/
  include/squared/gui/   one header per public type, plus gui.hpp
  src/                        one translation unit per type with definitions
  Makefile
```

Types with no out-of-line definitions have no `.cpp`. Adding one means adding
its file to `SOURCES` in the module's `Makefile`.

## Notes

`Skin` is the module's largest resident cost: ten
`std::unordered_map<std::string, T>` members, 688 bytes of object plus a heap
node per entry. See [standard-library-deviations.md](
standard-library-deviations.md).

`Ui` owns the `scene2d::Stage`, the content widget, and the overlay stack. It
is the only type that knows about `sq::app`, which keeps the
widget set free of any platform dependency.

`gui/src/detail/gui_detail.hpp` holds the helpers that used to sit in
`gui.cpp`'s anonymous namespace: colour multiplication, dimension clamping,
UTF-8 cursor stepping, font-path validation, and the `style_or_default`
template. They are `inline` in `sq::gui::detail` because several
translation units now need them. The header is not installed.

`Widget` publishes `actor_interface_id` so `scene2d::actor_cast<Widget>()`
recovers it from an `Actor&` during tree traversal. `Ui`, `Widget` and `Stack`
all walk Scene2D trees this way; none of them uses RTTI.

`Widget::wants_text_input()` decides whether focusing a widget raises the
platform soft keyboard. `TextField` returns true; everything else returns
false. `Ui` reads it in three places and no longer names `TextField` at all,
which is why `ui.cpp` does not include `text_field.hpp`. Any custom text-entry
widget can opt in by overriding it.
