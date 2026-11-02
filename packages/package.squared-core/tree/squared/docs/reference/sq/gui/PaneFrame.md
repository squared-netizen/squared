# class PaneFrame

*Defined at [gui/src/detail/pane_frame.cppm:24](../../../../gui/src/detail/pane_frame.cppm#L24)*



**brief** A pane's frame: the skin's window background around a table, over the screen. What the Stage opens a pane in; the Ui keeps it above the content, and a modal one takes every tap and dims what is beneath.

 Internal (7.7): it took the place of `gui::Window`, of which a pane used only the frame. No title, no close button, no dragging or resizing: the Stage places a pane.



Inherits from Widget



## Members

public static const ActorInterfaceId actor_interface_id



## Functions

### PaneFrame

*public void PaneFrame(std::string_view style)*



**brief** A frame drawn with the skin's window style `style`, which must outlive it (a literal does); a style the skin lacks is its `default`.

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/src/detail/pane_frame.cppm:34](../../../../gui/src/detail/pane_frame.cppm#L34)*

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/src/detail/pane_frame.cppm:29](../../../../gui/src/detail/pane_frame.cppm#L29)*

### content_table

*public Table & content_table()*

*Defined at [gui/src/detail/pane_frame.cppm:47](../../../../gui/src/detail/pane_frame.cppm#L47)*



**brief** The table inside the frame: the pane's content. 

### content_table

*public const Table & content_table()*

*Defined at [gui/src/detail/pane_frame.cppm:48](../../../../gui/src/detail/pane_frame.cppm#L48)*

### layout

*public void layout(Painter & painter, const Skin & skin)*



**brief** The content inside the style's insets. 

### minimum_size

*public Size minimum_size(Painter & painter, const Skin & skin)*



**brief** The content's, and the style's insets around it. 

### modal

*public bool modal()*

*Defined at [gui/src/detail/pane_frame.cppm:55](../../../../gui/src/detail/pane_frame.cppm#L55)*

### paint

*public void paint(Painter & painter, const Skin & skin, float x, float y)*



**brief** The style's background, behind the content. 

### pointer_event

*public bool pointer_event(const PointerEvent & event)*



**brief** A tap on the frame stays there; a modal frame takes any. 

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*

### set_modal

*public void set_modal(bool modal)*

*Defined at [gui/src/detail/pane_frame.cppm:54](../../../../gui/src/detail/pane_frame.cppm#L54)*



**brief** Modal: it takes every tap, and the Ui dims what is beneath it. 

### style

*public std::string_view style()*

*Defined at [gui/src/detail/pane_frame.cppm:51](../../../../gui/src/detail/pane_frame.cppm#L51)*



**brief** The skin's window style it is drawn with. 

