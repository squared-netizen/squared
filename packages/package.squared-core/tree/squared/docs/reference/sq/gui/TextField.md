# class TextField

*Defined at [gui/text_field.cppm:18](../../../../gui/text_field.cppm#L18)*



**brief** Single-line editable text input with cursor and composition support.



Inherits from Widget



## Members

public static const ActorInterfaceId actor_interface_id



## Functions

### TextField

*public void TextField(std::string text)*



**brief** Construct a text field.

**text** Initial text content.

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/text_field.cppm:28](../../../../gui/text_field.cppm#L28)*

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/text_field.cppm:23](../../../../gui/text_field.cppm#L23)*

### cursor

*public std::size_t cursor()*

*Defined at [gui/text_field.cppm:56](../../../../gui/text_field.cppm#L56)*



**brief** Read the cursor position.

**return** Byte offset into the UTF-8 text, or text length at the end.

### edited

*public bool edited()*

*Defined at [gui/text_field.cppm:79](../../../../gui/text_field.cppm#L79)*



**brief** Whether the text was changed and the change not finished yet (no Enter, no leaving): the edit `set_on_commit` has still to be told.

### focus_changed

*public void focus_changed(bool )*



**brief** Track focus to drive the focused style and underline state.

**note** The parameter matches Widget::focus_changed: true when the widget gained focus.

### focusable

*public bool focusable()*



**brief** Report focusability.

**return** true; text fields take keyboard focus.

### focused

*public bool focused()*

*Defined at [gui/text_field.cppm:73](../../../../gui/text_field.cppm#L73)*



**brief** Whether the field has focus: it is being edited.

**return** true while focused.

### key_down

*public bool key_down(app::Key , app::KeyModifiers )*



**brief** Move the cursor and edit the text with keys.

**note** Parameters match the Widget::key_down signature: a portable key name and the modifier state at the time of the event.

**return** true when the key was handled.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the style-driven minimum size.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing the text-field style.

**return** Minimum extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the field background, text, cursor, and composition.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the text-field style, and the stage-space origin.

### pointer_event

*public bool pointer_event(const PointerEvent & )*



**brief** Place the cursor from a pointer event.

**note** The parameter matches Widget::pointer_event: a pointer payload in widget-local coordinates.

**return** true when the field handled the event.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the text-plus-padding size.

**painter** Active painter used for measurement.

**skin** Skin providing the selected text-field style and font.

**return** Preferred extent in logical units.

### restore_edit

*public void restore_edit(std::string_view text)*



**brief** Put back an edit that was not finished, as `set_text` and `edited()` true: the Stage's, when a screen comes back. Told to `set_on_commit` once it is finished.

### set_on_commit

*public void set_on_commit(CommitCallback callback, void * context)*



**brief** What is told when an edit is finished: Enter (the keyboard's Done), or the field losing focus after its text changed.

**callback** Called with `context` and this field; nullptr for none.

**context** Passed back as it is.

### set_style

*public void set_style(std::string style)*



**brief** Change the applied text-field style.

**style** Named text-field style to apply.

### set_text

*public void set_text(std::string_view text)*



**brief** Replace the field text and reset the cursor; copied into the field's own, with no allocation when it is no longer than a text it held.

**text** New text content.

### text

*public const std::string & text()*

*Defined at [gui/text_field.cppm:50](../../../../gui/text_field.cppm#L50)*



**brief** Read the current text.

**return** Reference to the stored text.

### text_editing

*public bool text_editing(std::string_view , int , int )*



**brief** Update the in-progress composition span.

**note** Parameters match the Widget::text_editing signature: the current composition text, its start offset, and its length.

**return** true when the composition was handled.

### text_input

*public bool text_input(std::string_view )*



**brief** Insert committed UTF-8 text at the cursor.

**note** The parameter matches Widget::text_input: committed text to insert.

**return** true when the text was inserted.

### wants_text_input

*public bool wants_text_input()*

*Defined at [gui/text_field.cppm:165](../../../../gui/text_field.cppm#L165)*



**brief** Request the platform text-input service while focused.

**return** true; a focused text field always wants text input.

