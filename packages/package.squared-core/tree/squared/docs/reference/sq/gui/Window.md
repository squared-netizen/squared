# class Window

*Defined at [gui/window.hpp:24](../../../../gui/window.hpp#L24)*



**brief** Floating table-backed panel with a draggable title bar.

 Windows are hosted by Ui; ownership transfers to the Ui when shown.



Inherits from Widget



## Functions

### Window

*public void Window(int title, int style)*



**brief** Construct a window.

**title** Title-bar text.

**style** Named window style to apply.

### content_table

*public Table & content_table()*

*Defined at [gui/window.hpp:38](../../../../gui/window.hpp#L38)*



**brief** Access the window's content table.

**return** Reference to the content table for adding children.

### content_table

*public const Table & content_table()*

*Defined at [gui/window.hpp:44](../../../../gui/window.hpp#L44)*



**brief** Access the window's content table, read-only.

**return** Reference to the content table.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the minimum size constrained by layout.

**note** Parameters match Widget::minimum_size: an active painter and the skin providing the window style.

**return** Minimum extent in logical units.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the title bar, background, and content.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the window style, and the stage-space origin.

### pointer_event

*public bool pointer_event(const PointerEvent & )*



**brief** Handle drag, resize, and close-button events.

**note** The parameter matches Widget::pointer_event: a pointer payload in widget-local coordinates.

**return** true when the event was handled.

### title

*public const int & title()*

*Defined at [gui/window.hpp:56](../../../../gui/window.hpp#L56)*



**brief** Read the title text.

**return** Reference to the stored title.

### set_modal

*public void set_modal(bool modal)*

*Defined at [gui/window.hpp:68](../../../../gui/window.hpp#L68)*



**brief** Control modal blocking.

**modal** true dims the background outside the window.

### modal

*public bool modal()*

*Defined at [gui/window.hpp:74](../../../../gui/window.hpp#L74)*



**brief** Read the modal state.

**return** true while the window blocks background interaction.

### set_movable

*public void set_movable(bool movable)*

*Defined at [gui/window.hpp:80](../../../../gui/window.hpp#L80)*



**brief** Control title-bar dragging.

**movable** false keeps the window at its placed position.

### movable

*public bool movable()*

*Defined at [gui/window.hpp:86](../../../../gui/window.hpp#L86)*



**brief** Read the movable state.

**return** true while the window can be dragged by its title bar.

### closable

*public bool closable()*

*Defined at [gui/window.hpp:98](../../../../gui/window.hpp#L98)*



**brief** Read the closable state.

**return** true while the window shows a close button.

### set_closable

*public void set_closable(bool closable)*



**brief** Control the close button.

**closable** true shows the close button.

### set_minimum_window_size

*public void set_minimum_window_size(Size size)*



**brief** Set the smallest allowed window size.

**size** Minimum extent in logical units.

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** Report the content-driven preferred size.

**painter** Active painter used for measurement.

**skin** Skin providing window metrics and the title font.

**return** Preferred extent in logical units.

### layout

*public void layout(Painter & , const Skin & )*



**brief** Lay out the title bar and content table.

**note** Parameters match Widget::layout: an active painter and the skin providing the window style.

### set_resizable

*public void set_resizable(bool resizable)*

*Defined at [gui/window.hpp:104](../../../../gui/window.hpp#L104)*



**brief** Control edge resizing.

**resizable** true enables resize grabs on the window border.

### resizable

*public bool resizable()*

*Defined at [gui/window.hpp:110](../../../../gui/window.hpp#L110)*



**brief** Read the resizable state.

**return** true while the window can be resized.

### minimum_window_size

*public Size minimum_window_size()*

*Defined at [gui/window.hpp:122](../../../../gui/window.hpp#L122)*



**brief** Read the minimum window size.

**return** Requested minimum extent in logical units.

### request_close

*public void request_close()*

*Defined at [gui/window.hpp:129](../../../../gui/window.hpp#L129)*



**brief** Request the window to close.

 The host Ui prunes windows marked close-requested on the next update.

### close_requested

*public bool close_requested()*

*Defined at [gui/window.hpp:135](../../../../gui/window.hpp#L135)*



**brief** Read the close-requested flag.

**return** true after request_close() was called.

### escape_closes

*protected bool escape_closes()*

*Defined at [gui/window.hpp:180](../../../../gui/window.hpp#L180)*



**brief** Decide whether Escape closes the window.

**return** false by default; overridden by Dialog.

### set_title

*public void set_title(int title)*



**brief** Replace the title-bar text.

**title** New title text.

### set_style

*public void set_style(int style)*



**brief** Change the applied window style.

**style** Named window style to apply.



