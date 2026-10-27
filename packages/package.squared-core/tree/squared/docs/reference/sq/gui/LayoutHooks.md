# struct LayoutHooks

*Defined at [gui/instantiate.hpp:21](../../../../gui/instantiate.hpp#L21)*



**brief** What `instantiate` hands back to its caller (the Stage): taps, bound nodes and custom types. Function pointers and one context, so nothing is allocated to call them. Any can be left out.



## Members

public void (*)(void *, std::string_view) noexcept tapped

public void (*)(void *, const Layout &, const LayoutNode &, Widget &) noexcept bound

public std::unique_ptr<Widget> (*)(void *, std::string_view) custom

public void * context

public void (*)(void *, std::string_view) noexcept tapped_row

public void (*)(void *, const Layout &, const LayoutNode &, List &) noexcept listed



