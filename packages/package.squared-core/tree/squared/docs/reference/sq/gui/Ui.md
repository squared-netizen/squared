# class Ui

*Defined at [gui/ui.hpp:42](../../../../gui/ui.hpp#L42)*



**brief** Owns one widget tree and consumes the framework application event type.

 The Ui owns the stage, one content widget, and the set of open windows. All input enters through event(), pointer(), and the key/text entry points. Not thread-safe; one thread must drive it.



## Functions

### Ui

*public void Ui(float width, float height, Skin skin)*



**brief** Construct a user interface.

**width** Initial virtual width in logical pixels.

**height** Initial virtual height in logical pixels.

**skin** Skin styling every widget; copied into the Ui.

### set_content

*public Widget & set_content(int content)*



**brief** Replace the full-screen content widget.

**content** Widget to adopt; ownership transfers to the Ui.

**return** Reference to the adopted content widget.

### show_window

*public Window & show_window(int window, bool center)*



**brief** Show a floating window.

**window** Window to adopt; ownership transfers to the Ui.

**center** true centers the window over the current viewport.

**return** Reference to the shown window.

### show_dialog

*public Dialog & show_dialog(int dialog, bool center)*



**brief** Show a modal dialog.

**dialog** Dialog to adopt; ownership transfers to the Ui.

**center** true centers the dialog over the current viewport.

**return** Reference to the shown dialog.

### close_window

*public void close_window(Window & window)*



**brief** Mark a window closed and remove it on the next update.

**window** Window currently shown by this Ui.

### content

*public Widget * content()*

*Defined at [gui/ui.hpp:92](../../../../gui/ui.hpp#L92)*



**brief** Access the content widget.

**return** Pointer to the content widget, or null when unset.

### content

*public const Widget * content()*

*Defined at [gui/ui.hpp:98](../../../../gui/ui.hpp#L98)*



**brief** Access the content widget, read-only.

**return** Pointer to the content widget, or null when unset.

### event

*public bool event(const app::Event & event)*



**brief** Consume one framework application event.

**event** Event to translate and dispatch.

**return** true when the Ui handled the event.

### focused

*public Widget * focused()*

*Defined at [gui/ui.hpp:224](../../../../gui/ui.hpp#L224)*



**brief** Read the focused widget.

**return** Focused widget, or null when none is focused.

### set_safe_area

*public void set_safe_area(Insets insets)*



**brief** Keep the content inside the part of the screen the user can see.

 A phone draws the interface edge to edge, so the status bar, the navigation bar and a camera cut-out sit on top of it. Insets move the content's rectangle in from each edge; windows and dialogs are still free to use the whole viewport.



**insets** Distance to keep clear on each side, in logical units. Negative or non-finite values count as zero, and insets larger than the viewport leave the content empty rather than inverted.

**note** Survives resize() and set_content(); call it again only when the bars change.

### resize

*public void resize(float width, float height)*



**brief** Resize the virtual viewport.

**width** New width in logical pixels.

**height** New height in logical pixels.

### update

*public void update(double delta_seconds)*



**brief** Advance time-based state and prune close-requested windows.

**delta_seconds** Elapsed time in seconds; non-negative.

**throws**exception raised by a tooltip factory, or std::invalid_argument when a factory returns a parented widget.

### layout

*public void layout(Painter & painter)*



**brief** Validate layout of every visible widget.

**painter** Painter used for measurement.

### paint

*public void paint(Painter & painter)*



**brief** Paint the content and every open overlay.

**painter** Painter receiving all drawing.

### pointer

*public bool pointer(PointerAction action, float x, float y, int button, int pointer_id)*



**brief** Inject a pointer input.

**action** Pointer action being reported.

**x** Horizontal position in logical pixels.

**y** Vertical position in logical pixels.

**button** Button index; zero for the primary button.

**pointer_id** Stable identifier for the pointer contact.

**return** true when some widget handled the event.

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

### navigation

*public bool navigation(app::Navigation navigation, int input_device_id)*



**brief** Dispatch a navigation action, including focus moves.

**navigation** Semantic navigation action.

**input_device_id** Source device identifier.

**return** true when the navigation was handled.

### safe_area

*public Insets safe_area()*

*Defined at [gui/ui.hpp:120](../../../../gui/ui.hpp#L120)*



**brief** Report the insets last given to set_safe_area().

**return** The sanitized insets; all zero until one is set.

### text_input

*public bool text_input(int text)*



**brief** Forward committed text to the focused widget.

**text** Committed UTF-8 text.

**return** true when the focused widget handled the text.

### text_editing

*public bool text_editing(int text, int start, int length)*



**brief** Forward composition updates to the focused widget.

**text** Current composition text.

**start** Start offset of the composition span.

**length** Length of the composition span.

**return** true when the focused widget handled the composition.

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*



**brief** Install the platform text-input boundary.

**service** Non-owning service; may be null to disable. Must outlive the Ui while set.

### clear_focus

*public void clear_focus()*



**brief** Clear keyboard focus to no widget.

### set_tooltip_config

*public void set_tooltip_config(TooltipConfig config)*



**brief** Replace tooltip timing and placement policy.

**config** Finite, non-negative delays and distances.

**throws**when any value is negative or non-finite.

### window_count

*public int window_count()*

*Defined at [gui/ui.hpp:86](../../../../gui/ui.hpp#L86)*



**brief** Count currently shown windows and dialogs.

**return** Number of open overlays.

### tooltip_config

*public const TooltipConfig & tooltip_config()*

*Defined at [gui/ui.hpp:236](../../../../gui/ui.hpp#L236)*



**brief** Return the active tooltip policy.

**return** Reference valid for the lifetime of this Ui.

### tooltip_visible

*public bool tooltip_visible()*

*Defined at [gui/ui.hpp:244](../../../../gui/ui.hpp#L244)*



**brief** Return true while a tooltip subtree is attached to the Stage.

**return** true between materialization and dismissal.

### tooltip_owner

*public const Widget * tooltip_owner()*

*Defined at [gui/ui.hpp:249](../../../../gui/ui.hpp#L249)*



**brief** Return the widget whose tooltip is visible, or null.

**return** Non-owning pointer invalidated when its owning tree is removed.

### tooltip_bounds

*public int tooltip_bounds()*

*Defined at [gui/ui.hpp:255](../../../../gui/ui.hpp#L255)*



**brief** Return the visible tooltip rectangle in Stage coordinates.

**return** Bounds after layout, or no value when no tooltip is attached.

### skin

*public const Skin & skin()*

*Defined at [gui/ui.hpp:268](../../../../gui/ui.hpp#L268)*



**brief** Access the skin, read-only.

**return** Reference to the Ui's skin.

### skin

*public Skin & skin()*

*Defined at [gui/ui.hpp:274](../../../../gui/ui.hpp#L274)*



**brief** Access the skin for mutation.

**return** Reference to the Ui's skin.



