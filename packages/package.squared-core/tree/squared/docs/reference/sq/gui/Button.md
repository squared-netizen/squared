# class Button

*Defined at [gui/button.hpp:21](../../../../gui/button.hpp#L21)*



**brief** Clickable, focusable control with an optional label and icon. 



Inherits from Widget



## Functions

### Button

*public void Button(std::string text, Callback callback)*



**brief** Construct a button.

**text** Optional label text.

**callback** Click action; copied into the button.

### set_text

*public void set_text(std::string text)*



**brief** Replace the label text.

**text** New label text.

### set_on_click

*public void set_on_click(Callback callback)*



**brief** Set the click action.

**callback** Action copied into the button; may be empty.

### set_style

*public void set_style(std::string style)*



**brief** Change the applied button style.

**style** Named button style to apply.

### set_icon

*public void set_icon(DrawablePtr drawable)*



**brief** Display a drawable before the label.

**drawable** Shared immutable drawable; empty clears the icon.

**post** Any previously configured glyph is cleared.

### set_glyph

*public void set_glyph(std::string glyph, FontPtr font)*



**brief** Display one UTF-8 glyph before the label.

**glyph** UTF-8 glyph text; empty clears the icon.

**font** Optional immutable font, or empty for the button style font.

**post** Any previously configured drawable icon is cleared.

### clear_icon

*public void clear_icon()*



**brief** Remove either kind of icon while preserving the label. 

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the style-driven minimum size.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing the button style.

**return** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the label-plus-padding size.

**painter** Active painter used for measurement.

**skin** Skin providing the selected button style and font.

**return** Preferred extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the styled button background and label.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the button style, and the stage-space origin.

### pointer_event

*public bool pointer_event(const PointerEvent & )*



**brief** Track press, hover, and release onto the button.

**note** The parameter matches Widget::pointer_event: a pointer payload in widget-local coordinates.

**return** true when the event was handled.

### key_down

*public bool key_down(app::Key , app::KeyModifiers )*



**brief** Activate on Enter or Space.

**note** Parameters match the Widget::key_down signature: a portable key name and the modifier state at the time of the event.

**return** true when the key activated the button.

### focus_changed

*public void focus_changed(bool )*



**brief** Track the focused state.

**note** The parameter matches Widget::focus_changed: true when the widget gained focus.

### focusable

*public bool focusable()*



**brief** Report focusability.

**return** true; buttons take keyboard focus.

### activate

*protected void activate()*



**brief** Perform the button's activation action.

 Base invokes the click callback. Subclasses override to toggle state.

### text

*public const std::string & text()*

*Defined at [gui/button.hpp:44](../../../../gui/button.hpp#L44)*



**brief** Read the label text.

**return** Reference to the stored label.

### pressed

*public bool pressed()*

*Defined at [gui/button.hpp:133](../../../../gui/button.hpp#L133)*



**brief** Read the pressed state: public, so a container that takes the pointer from it (a List scrolling) can be seen to have let it go.

**return** true while the pointer presses the button.

### hovered

*protected bool hovered()*

*Defined at [gui/button.hpp:153](../../../../gui/button.hpp#L153)*



**brief** Read the hovered state.

**return** true while the pointer hovers the button.

### focused

*protected bool focused()*

*Defined at [gui/button.hpp:159](../../../../gui/button.hpp#L159)*



**brief** Read the focused state.

**return** true while the button owns keyboard focus.

### selected

*protected bool selected()*



**brief** Report the selected state used for styling.

**return** true when the button is rendered as selected.



