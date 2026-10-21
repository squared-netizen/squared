# class Slider

*Defined at [gui/slider.hpp:21](../../../../gui/slider.hpp#L21)*



**brief** Draggable one-axis value selector with an optional gradient fill. 



Inherits from Widget



## Functions

### Slider

*public void Slider(float minimum, float maximum, float value)*



**brief** Construct a slider.

**minimum** Smallest selectable value.

**maximum** Largest selectable value.

**value** Initial value; clamped into the range.

### set_range

*public void set_range(float minimum, float maximum)*



**brief** Set the selectable range.

**minimum** Smallest selectable value.

**maximum** Largest selectable value; with minimum must be ordered.

### set_value

*public void set_value(float value)*



**brief** Set the current value.

**value** New value; clamped into the range and snapped to the step.

### set_step

*public void set_step(float step)*



**brief** Set the value granularity.

**step** Snap increment in value units; zero disables snapping.

### set_on_change

*public void set_on_change(ChangeCallback callback)*



**brief** Set the change callback.

**callback** Action copied into the slider; invoked with each new value.

### set_style

*public void set_style(std::string style)*



**brief** Change the applied slider style.

**style** Named slider style to apply.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the style-driven minimum size.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing the slider style.

**return** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the track-driven preferred size.

**painter** Active painter used for measurement.

**skin** Active skin; unused by slider measurement.

**return** Preferred extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the track, gradient fill, and knob.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the slider style, and the stage-space origin.

### pointer_event

*public bool pointer_event(const PointerEvent & )*



**brief** Drag the knob to set the value.

**note** The parameter matches Widget::pointer_event: a pointer payload in widget-local coordinates.

**return** true when the event was handled.

### key_down

*public bool key_down(app::Key , app::KeyModifiers )*



**brief** Adjust the value with arrow keys.

**note** Parameters match the Widget::key_down signature: a portable key name and the modifier state at the time of the event.

**return** true when the key adjusted the value.

### focus_changed

*public void focus_changed(bool )*



**brief** Track the focused state.

**note** The parameter matches Widget::focus_changed: true when the widget gained focus.

### focusable

*public bool focusable()*



**brief** Report focusability.

**return** true; sliders take keyboard focus.

### value

*public float value()*

*Defined at [gui/slider.hpp:52](../../../../gui/slider.hpp#L52)*



**brief** Read the current value.

**return** Current value in the configured range.



