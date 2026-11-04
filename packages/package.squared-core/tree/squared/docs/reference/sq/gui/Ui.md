# class Ui

*Defined at [gui/src/detail/ui.cppm:35](../../../../gui/src/detail/ui.cppm#L35)*



**brief** Owns one widget tree and consumes the framework application event type.

 The Ui owns the actor root, one content widget, and the panes open over it. All input enters through event(), pointer(), and the key/text entry points. Not thread-safe; one thread must drive it.

 Internal (7.7): the Stage holds one per window; an app reaches widgets through `sq::Stage`.



## Functions

### Ui

*public void Ui(float width, float height, Skin skin)*



**brief** Construct a user interface.

**width** Initial virtual width in logical pixels.

**height** Initial virtual height in logical pixels.

**skin** Skin styling every widget; copied into the Ui.

### clear_focus

*public void clear_focus()*



**brief** Clear keyboard focus to no widget.

### close_pane

*public void close_pane(PaneFrame & pane)*



**brief** Close a pane now, freeing it and its widgets; the focus goes back to where it was before the pane took it.

**pane** Pane currently shown by this Ui.

### content

*public Widget * content()*

*Defined at [gui/src/detail/ui.cppm:79](../../../../gui/src/detail/ui.cppm#L79)*



**brief** Access the content widget.

**return** Pointer to the content widget, or null when unset.

### content

*public const Widget * content()*

*Defined at [gui/src/detail/ui.cppm:85](../../../../gui/src/detail/ui.cppm#L85)*



**brief** Access the content widget, read-only.

**return** Pointer to the content widget, or null when unset.

### event

*public bool event(const app::Event & event)*



**brief** Consume one framework application event.

**event** Event to translate and dispatch.

**return** true when the Ui handled the event.

### focused

*public Widget * focused()*

*Defined at [gui/src/detail/ui.cppm:223](../../../../gui/src/detail/ui.cppm#L223)*



**brief** Read the focused widget.

**return** Focused widget, or null when none is focused.

### key_down

*public bool key_down(app::Key key, app::KeyModifiers modifiers)*



**brief** Dispatch a key-down event.

**key** Portable key name.

**modifiers** Modifier state at the time of the event.

**return** true when the focused widget handled the key.

### key_up

*public bool key_up(app::Key key, app::KeyModifiers modifiers)*



**brief** Dispatch a key-up event.

**key** Portable key name.

**modifiers** Modifier state at the time of the event.

**return** true when the focused widget handled the key.

### layout

*public void layout(Painter & painter)*



**brief** Validate layout of every visible widget.

**painter** Painter used for measurement.

### navigation

*public bool navigation(app::Navigation navigation, std::int32_t input_device_id)*



**brief** Dispatch a navigation action, including focus moves.

**navigation** Semantic navigation action.

**input_device_id** Source device identifier.

**return** true when the navigation was handled.

### paint

*public void paint(Painter & painter)*



**brief** Paint the content and every open overlay.

**painter** Painter receiving all drawing.

### pane_count

*public std::size_t pane_count()*

*Defined at [gui/src/detail/ui.cppm:73](../../../../gui/src/detail/ui.cppm#L73)*



**brief** Count the panes shown.

**return** Number of open panes.

### pinch

*public bool pinch(const PinchEvent & event)*



**brief** Offer a two-finger pinch to the widgets (the Stage recognises it). At `begin`, to the enabled widget under the middle, then to its parents, until one's `Widget::pinch` takes it; the rest of the pinch goes to that one, its middle made local to it.

**event** The pinch, its middle in logical units from the top left.

**return** true when a widget has the pinch (at `end`: had it).

### pointer

*public bool pointer(PointerAction action, float x, float y, int button, std::int64_t pointer_id)*



**brief** Inject a pointer input.

**action** Pointer action being reported.

**x** Horizontal position in logical pixels.

**y** Vertical position in logical pixels.

**button** Button index; zero for the primary button.

**pointer_id** Stable identifier for the pointer contact.

**return** true when some widget handled the event.

### resize

*public void resize(float width, float height)*



**brief** Resize the virtual viewport.

**width** New width in logical pixels.

**height** New height in logical pixels.

### safe_area

*public Insets safe_area()*

*Defined at [gui/src/detail/ui.cppm:107](../../../../gui/src/detail/ui.cppm#L107)*



**brief** Report the insets last given to set_safe_area().

**return** The sanitized insets; all zero until one is set.

### set_content

*public Widget & set_content(std::unique_ptr<Widget> content)*



**brief** Replace the full-screen content widget.

**content** Widget to adopt; ownership transfers to the Ui.

**return** Reference to the adopted content widget.

### set_safe_area

*public void set_safe_area(Insets insets)*



**brief** Keep the content inside the part of the screen the user can see.

 A phone draws the interface edge to edge, so the status bar, the navigation bar and a camera cut-out sit on top of it. Insets move the content's rectangle in from each edge; panes are placed by their owner, who reads the insets too.



**insets** Distance to keep clear on each side, in logical units. Negative or non-finite values count as zero, and insets larger than the viewport leave the content empty rather than inverted.

**note** Survives resize() and set_content(); call it again only when the bars change.

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*



**brief** Install the platform text-input boundary.

**service** Non-owning service; may be null to disable. Must outlive the Ui while set.

### set_tooltip_config

*public void set_tooltip_config(TooltipConfig config)*



**brief** Replace tooltip timing and placement policy.

**config** Finite, non-negative delays and distances.

**throws**when any value is negative or non-finite.

### show_pane

*public PaneFrame & show_pane(std::unique_ptr<PaneFrame> pane)*



**brief** Show a pane over the content, above the panes already shown. Its owner places it (`set_bounds`); one with no size takes its preferred size at the next layout. A modal one takes the focus.

**pane** Pane to adopt; ownership transfers to the Ui.

**return** Reference to the shown pane.

### skin

*public Skin & skin()*

*Defined at [gui/src/detail/ui.cppm:273](../../../../gui/src/detail/ui.cppm#L273)*



**brief** Access the skin for mutation.

**return** Reference to the Ui's skin.

### skin

*public const Skin & skin()*

*Defined at [gui/src/detail/ui.cppm:267](../../../../gui/src/detail/ui.cppm#L267)*



**brief** Access the skin, read-only.

**return** Reference to the Ui's skin.

### text_editing

*public bool text_editing(std::string_view text, int start, int length)*



**brief** Forward composition updates to the focused widget.

**text** Current composition text.

**start** Start offset of the composition span.

**length** Length of the composition span.

**return** true when the focused widget handled the composition.

### text_input

*public bool text_input(std::string_view text, app::KeyModifiers modifiers)*



**brief** Forward committed text to the focused widget: as `Widget::text_input`, or with Ctrl, Alt or Meta held as `Widget::key_text`.

**text** Committed UTF-8 text.

**modifiers** The modifiers held when a key typed it.

**return** true when the focused widget handled the text.

### tooltip_bounds

*public std::optional<Rectangle> tooltip_bounds()*

*Defined at [gui/src/detail/ui.cppm:254](../../../../gui/src/detail/ui.cppm#L254)*



**brief** Return the visible tooltip rectangle in root coordinates.

**return** Bounds after layout, or no value when no tooltip is attached.

### tooltip_config

*public const TooltipConfig & tooltip_config()*

*Defined at [gui/src/detail/ui.cppm:235](../../../../gui/src/detail/ui.cppm#L235)*



**brief** Return the active tooltip policy.

**return** Reference valid for the lifetime of this Ui.

### tooltip_owner

*public const Widget * tooltip_owner()*

*Defined at [gui/src/detail/ui.cppm:248](../../../../gui/src/detail/ui.cppm#L248)*



**brief** Return the widget whose tooltip is visible, or null.

**return** Non-owning pointer invalidated when its owning tree is removed.

### tooltip_visible

*public bool tooltip_visible()*

*Defined at [gui/src/detail/ui.cppm:243](../../../../gui/src/detail/ui.cppm#L243)*



**brief** Return true while a tooltip subtree is attached to the root.

**return** true between materialization and dismissal.

### update

*public void update(double delta_seconds)*



**brief** Advance time-based state: actors' `act`, tooltips.

**delta_seconds** Elapsed time in seconds; non-negative.

**throws**exception raised by a tooltip factory, or std::invalid_argument when a factory returns a parented widget.

