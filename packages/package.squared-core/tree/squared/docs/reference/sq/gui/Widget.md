# class Widget

*Defined at [gui/widget.cppm:27](../../../../gui/widget.cppm#L27)*



**brief** Base class for every GUI node. Widgets may own other widgets.

 Widget inherits the composite ownership model: children owned via Group. Widgets carry layout, painting, and input behavior.



Inherits from Group



## Members

public static const ActorInterfaceId actor_interface_id



## Functions

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/widget.cppm:55](../../../../gui/widget.cppm#L55)*



**brief** Publish the Widget interface to actor_cast(), read-only.

**id** Identifier of the requested interface.

**return** This widget for the Widget identifier, otherwise the Group answer.

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/widget.cppm:44](../../../../gui/widget.cppm#L44)*



**brief** Publish the Widget interface to actor_cast().

**id** Identifier of the requested interface.

**return** This widget for the Widget identifier, otherwise the Group answer, so actor_cast<Group>() still resolves.

### clear_tooltip

*public void clear_tooltip()*



**brief** Remove the tooltip declaration from this widget. 

### enabled

*public bool enabled()*

*Defined at [gui/widget.cppm:199](../../../../gui/widget.cppm#L199)*



**brief** Read the enabled state.

**return** true while the widget is enabled.

### focus_changed

*public void focus_changed(bool focused)*



**brief** Notify the widget of a focus change.

**focused** true when the widget gained focus.

### focusable

*public bool focusable()*



**brief** Report whether this widget can take keyboard focus.

**return** true when the widget is focusable.

### has_tooltip

*public bool has_tooltip()*

*Defined at [gui/widget.cppm:221](../../../../gui/widget.cppm#L221)*



**brief** Return true when this widget declares tooltip content. 

### input_event

*protected void input_event(InputEvent & event)*



**brief** Bridge actor dispatch into widget input handlers.

**event** GUI input event to adapt.

### invalidate_layout

*public void invalidate_layout()*



**brief** Invalidate cached layout so it is recomputed next pass.

### key_down

*public bool key_down(app::Key key, app::KeyModifiers modifiers)*



**brief** Handle a key-down event.

**key** Portable key name.

**modifiers** Modifier state at the time of the event.

**return** true when the key was handled.

### key_text

*public bool key_text(std::string_view text, app::KeyModifiers modifiers)*



**brief** Handle what a key typed with Ctrl, Alt or Meta held: `c` with Ctrl for Ctrl-C. Text fields leave it (false, the default), as a desktop's do; a terminal makes control characters of it.

**text** The character the key types without those modifiers.

**modifiers** Which were held: at least one of control, alt, meta.

**return** true when the key was handled.

### layout

*public void layout(Painter & painter, const Skin & skin)*



**brief** Position and size this widget, then lay out children.

**painter** Active painter used for measurement.

**skin** Skin providing style and spacing values.

### layout_valid

*public bool layout_valid()*

*Defined at [gui/widget.cppm:108](../../../../gui/widget.cppm#L108)*



**brief** Check whether cached layout is current.

**return** true while the layout is valid.

### maximum_size

*public Size maximum_size(Painter & painter, const Skin & skin)*



**brief** Report the largest acceptable size.

**painter** Active painter used for measurement.

**skin** Skin providing style and spacing values.

**return** Maximum extent in logical units.

### minimum_size

*public Size minimum_size(Painter & painter, const Skin & skin)*



**brief** Report the smallest acceptable size.

**painter** Active painter used for measurement.

**skin** Skin providing style and spacing values.

**return** Minimum extent in logical units.

### paint

*public void paint(Painter & painter, const Skin & skin, float stage_x, float stage_y)*



**brief** Paint this widget and, typically, its children.

**painter** Active painter; valid for the call.

**skin** Skin providing style resources.

**stage_x** Stage-space origin associated with this widget.

**stage_y** Stage-space origin associated with this widget.

### pointer_event

*public bool pointer_event(const PointerEvent & event)*



**brief** Handle a pointer event.

**event** Pointer payload in widget-local coordinates.

**return** true when the event was handled and should stop propagating.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the preferred size.

**painter** Active painter used for measurement.

**skin** Skin providing style, font, and spacing values.

**return** Preferred extent in logical units.

### set_enabled

*public void set_enabled(bool enabled)*

*Defined at [gui/widget.cppm:193](../../../../gui/widget.cppm#L193)*



**brief** Control the enabled state.

**enabled** false disables interaction and dims rendering.

### set_tooltip

*public void set_tooltip(std::string text)*



**brief** Attach a plain-text tooltip composed from standard GUI widgets.

**text** UTF-8 tooltip text. An empty string clears the tooltip.

**post** A fresh Stack/Panel/MarginContainer/Label subtree is created each time the tooltip is shown.

### set_tooltip_factory

*public void set_tooltip_factory(TooltipFactory factory)*



**brief** Attach a custom tooltip widget factory.

**factory** Copyable callback returning a fresh unparented widget subtree. An empty callback clears the tooltip.

**note** The Ui temporarily owns each returned subtree while it is shown.

### size_hints

*public SizeHints size_hints(Painter & painter, const Skin & skin)*



**brief** Compute the full size-hint set.

**painter** Active painter used for measurement.

**skin** Skin providing style and spacing values.

**return** Minimum, preferred, and maximum sizes.

### text_editing

*public bool text_editing(std::string_view , int , int )*



**brief** Handle composition-edit updates.

**note** Arguments are the current composition text, its start offset, and its length, matching the virtual signature.

**return** true when the composition was handled.

### text_input

*public bool text_input(std::string_view text)*



**brief** Handle committed UTF-8 text input.

**text** Committed text to insert.

**return** true when the text was handled.

### validate_layout

*public void validate_layout(Painter & painter, const Skin & skin)*



**brief** Recompute layout if currently invalid.

**painter** Active painter used for measurement.

**skin** Skin providing style and spacing values.

### wants_text_input

*public bool wants_text_input()*



**brief** Report whether focusing this widget should raise the platform text-input service.

**return** false by default; text-entry widgets override it.

**note** The Ui starts, moves, and stops the platform soft keyboard from this answer. A custom text-entry widget opts in by overriding it, which the previous TextField-specific check did not allow.

### ~Widget

*public void ~Widget()*

*Defined at [gui/widget.cppm:36](../../../../gui/widget.cppm#L36)*

