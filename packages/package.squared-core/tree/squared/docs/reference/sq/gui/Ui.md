# class Ui

*Defined at [gui/src/detail/ui.cppm:34](../../../../gui/src/detail/ui.cppm#L34)*

## Functions

### Ui

*public void Ui(float width, float height, Skin skin)*

### set_content

*public Widget & set_content(std::unique_ptr<Widget> content)*

### show_pane

*public PaneFrame & show_pane(std::unique_ptr<PaneFrame> pane)*

### close_pane

*public void close_pane(PaneFrame & pane)*

### content

*public Widget * content()*

*Defined at [gui/src/detail/ui.cppm:78](../../../../gui/src/detail/ui.cppm#L78)*

### content

*public const Widget * content()*

*Defined at [gui/src/detail/ui.cppm:84](../../../../gui/src/detail/ui.cppm#L84)*

### focused

*public Widget * focused()*

*Defined at [gui/src/detail/ui.cppm:210](../../../../gui/src/detail/ui.cppm#L210)*

### set_safe_area

*public void set_safe_area(Insets insets)*

### resize

*public void resize(float width, float height)*

### update

*public void update(double delta_seconds)*

### layout

*public void layout(Painter & painter)*

### paint

*public void paint(Painter & painter)*

### event

*public bool event(const app::Event & event)*

### pointer

*public bool pointer(PointerAction action, float x, float y, int button, std::int64_t pointer_id)*

### key_down

*public bool key_down(app::Key key, app::KeyModifiers modifiers)*

### key_up

*public bool key_up(app::Key key, app::KeyModifiers modifiers)*

### navigation

*public bool navigation(app::Navigation navigation, std::int32_t input_device_id)*

### safe_area

*public Insets safe_area()*

*Defined at [gui/src/detail/ui.cppm:106](../../../../gui/src/detail/ui.cppm#L106)*

### text_input

*public bool text_input(std::string_view text)*

### text_editing

*public bool text_editing(std::string_view text, int start, int length)*

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*

### clear_focus

*public void clear_focus()*

### pane_count

*public std::size_t pane_count()*

*Defined at [gui/src/detail/ui.cppm:72](../../../../gui/src/detail/ui.cppm#L72)*

### set_tooltip_config

*public void set_tooltip_config(TooltipConfig config)*

### tooltip_config

*public const TooltipConfig & tooltip_config()*

*Defined at [gui/src/detail/ui.cppm:222](../../../../gui/src/detail/ui.cppm#L222)*

### tooltip_visible

*public bool tooltip_visible()*

*Defined at [gui/src/detail/ui.cppm:230](../../../../gui/src/detail/ui.cppm#L230)*

### tooltip_owner

*public const Widget * tooltip_owner()*

*Defined at [gui/src/detail/ui.cppm:235](../../../../gui/src/detail/ui.cppm#L235)*

### tooltip_bounds

*public std::optional<Rectangle> tooltip_bounds()*

*Defined at [gui/src/detail/ui.cppm:241](../../../../gui/src/detail/ui.cppm#L241)*

### skin

*public const Skin & skin()*

*Defined at [gui/src/detail/ui.cppm:254](../../../../gui/src/detail/ui.cppm#L254)*

### skin

*public Skin & skin()*

*Defined at [gui/src/detail/ui.cppm:260](../../../../gui/src/detail/ui.cppm#L260)*



