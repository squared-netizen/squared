# struct LayoutHooks

*Defined at [gui/instantiate.cppm:19](../../../../gui/instantiate.cppm#L19)*

## Members

public void (*)(void *, std::string_view) noexcept tapped

public void (*)(void *, const Layout &, const LayoutNode &, Widget &) noexcept bound

public std::unique_ptr<Widget> (*)(void *, std::string_view) custom

public void * context

public void (*)(void *, std::string_view) noexcept tapped_row

public void (*)(void *, const Layout &, const LayoutNode &, List &) noexcept listed



