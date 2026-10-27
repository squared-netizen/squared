# class List

*Defined at [gui/list.hpp:39](../../../../gui/list.hpp#L39)*



**brief** A list of any length that holds widgets only for the rows it shows: rows of one height, each a table built from one row of a layout, reused as it scrolls. Dragging scrolls it from anywhere in it, a row's buttons included: a finger that moves more than `drag_slop` on a button scrolls instead of tapping.

 What a row shows is its owner's to set: `set_on_row` is told each time a row's table starts showing another row, or none. The Stage binds a list to a model's list (`"type": "list"` in a layout); the list itself knows only rows by their index, from 0 to `count()`.

 Allocates when it builds row tables (its first layout, and a taller one: a rotation), and copies its row's nodes and words when it is made; a scroll allocates nothing.



Inherits from Widget



## Members

public static const ActorInterfaceId actor_interface_id

public static const std::uint32_t no_row

public static const std::size_t max_rows_built

public static const float drag_slop

public static const std::uint32_t preferred_rows



## Functions

### List

*public void List(const List & )*

*Defined at [gui/list.hpp:70](../../../../gui/list.hpp#L70)*

### List

*public void List(const Layout & layout, std::size_t row, const LayoutHooks & hooks)*



**brief** A list whose row is `layout`'s node `row`, with the nodes in it: copied, with the layout's words and numbers, so `layout` need not outlive the list. Rows are built with `hooks` until `set_row_hooks`.

### List

*public void List(List && )*

*Defined at [gui/list.hpp:72](../../../../gui/list.hpp#L72)*

### actor_interface

*public void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/list.hpp:75](../../../../gui/list.hpp#L75)*

### actor_interface

*public const void * actor_interface(ActorInterfaceId id)*

*Defined at [gui/list.hpp:80](../../../../gui/list.hpp#L80)*

### count

*public std::uint32_t count()*

*Defined at [gui/list.hpp:102](../../../../gui/list.hpp#L102)*

### ~List

*public void ~List()*

### set_row_hooks

*public void set_row_hooks(const LayoutHooks & hooks, void * owner)*



**brief** Build rows built from now on with `hooks`, each called with the row's table as its context (`slot_of` says which row table it is); `owner` is kept for them to find (`owner()`). Rows already built keep the hooks they were built with: call it before the first layout.

### set_on_row

*public void set_on_row(RowShown shown, void * context)*



**brief** Be told when a row's table shows another row; nullptr for none. 

### set_count

*public void set_count(std::uint32_t count)*



**brief** How many rows there are: the scroll stops at the last. 

### first_row

*public std::uint32_t first_row()*



**brief** The first row on screen, at least partly. 

### operator=

*public List & operator=(const List & )*

*Defined at [gui/list.hpp:71](../../../../gui/list.hpp#L71)*

### operator=

*public List & operator=(List && )*

*Defined at [gui/list.hpp:73](../../../../gui/list.hpp#L73)*

### owner

*public void * owner()*

*Defined at [gui/list.hpp:95](../../../../gui/list.hpp#L95)*



**brief** What `set_row_hooks` was given; nullptr before. 

### row_height

*public float row_height()*

*Defined at [gui/list.hpp:105](../../../../gui/list.hpp#L105)*



**brief** A row's height, measured from the first row built; 0 before. 

### rows_built

*public std::size_t rows_built()*

*Defined at [gui/list.hpp:114](../../../../gui/list.hpp#L114)*



**brief** How many row tables there are: the rows on screen and one more, at most `max_rows_built`. 

### rows_on_screen

*public std::uint32_t rows_on_screen()*



**brief** How many rows the list's height shows, a part one counted; 0 before its first layout. 

### table

*public Table & table(std::size_t slot)*

*Defined at [gui/list.hpp:117](../../../../gui/list.hpp#L117)*



**brief** Row table number `slot`. 

### row_at

*public std::uint32_t row_at(std::size_t slot)*

*Defined at [gui/list.hpp:120](../../../../gui/list.hpp#L120)*



**brief** The row table number `slot` shows: an index, or `no_row`. 

### scroll_y

*public float scroll_y()*

*Defined at [gui/list.hpp:133](../../../../gui/list.hpp#L133)*

### slot_of

*public std::size_t slot_of(const Table & row)*



**brief** Which row table `row` is; `rows_built()` when it is none of them. 

### slot_showing

*public std::size_t slot_showing(std::uint32_t index)*



**brief** The row table showing row `index`; `rows_built()` when none does. 

### set_scroll_y

*public void set_scroll_y(float offset)*



**brief** Scroll to `offset`, in logical units from the top; kept inside the rows. 

### restore_scroll

*public void restore_scroll(float offset)*



**brief** Scroll to `offset` once the rows reach it: what the Stage puts back when a screen comes back, before the model has sent the count again. Scrolled as far as the rows go meanwhile; a drag forgets it.

### row

*public Layout row()*



**brief** The row, as a layout: its first node is the row's. 

### preferred_size

*public Size preferred_size(Painter & painter, const Skin & skin)*



**brief** The row's width, and its height for as many rows as there are, up to `preferred_rows`: a list that should fill its cell grows.

### layout

*public void layout(Painter & painter, const Skin & skin)*



**brief** Build the row tables it needs, and place the rows on screen. 

### pointer_event

*public bool pointer_event(const PointerEvent & event)*



**brief** Drag to scroll, where no row's widget took the pointer. 

### short_of_rows

*public bool short_of_rows()*

*Defined at [gui/list.hpp:144](../../../../gui/list.hpp#L144)*



**brief** Whether more rows fit than `max_rows_built`: those stay empty. 



