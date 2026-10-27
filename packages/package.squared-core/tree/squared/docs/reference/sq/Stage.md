# class Stage

*Defined at [stage/stage.hpp:134](../../../stage/stage.hpp#L134)*



**brief** The view: a stack of screens, each a layout or a C++ screen, with the skin, the drawing, input, and the UI end of the Telegraph.

 Only the top screen has widgets: a screen that comes back (`back`) is built again (a layout from its table, a C++ screen by its `build`). Panes open over the top screen and belong to it: leaving the screen closes them. A button's tap is sent as it reads: a telegram to the model, or a navigation to this stack. The App owns the Stage and calls the rest; an app calls `push`, `replace`, `back`, `open`, `close`, and `add` and `custom` to name what its layout files reach.

 Navigation asked for while an event is being delivered (a tap) waits until delivery has finished: building the next screen replaces the widgets, and the tapped button is one of them.



## Members

public static const std::size_t max_depth

public static const std::size_t max_added

public static const std::size_t max_panes

public static const std::size_t max_bindings

public static const std::size_t scratch_nodes

public static const std::size_t scratch_text

public static const std::size_t scratch_numbers



## Functions

### Stage

*public void Stage(const Stage & )*

*Defined at [stage/stage.hpp:156](../../../stage/stage.hpp#L156)*

### Stage

*public void Stage(Stage && )*

*Defined at [stage/stage.hpp:158](../../../stage/stage.hpp#L158)*

### Stage

*public void Stage(messaging::TelegraphUi & telegraph)*

### add

*public bool add()*

*Defined at [stage/stage.hpp:221](../../../stage/stage.hpp#L221)*



**brief** Let taps reach the C++ screen `S` by its name (`push about`), as names.json's `screens` lists it. In `squared_main`. False, and reported, when `max_added` are already added or another has the name.

### ~Stage

*public void ~Stage()*

### push

*public bool push(const gui::Layout & layout)*



**brief** Show `layout` over the screen there is, which keeps nothing but its layout, and closes its panes. False when the stack is full (`max_depth`) or `layout` is a pane; the problem is reported. `layout` must outlive its place on the stack: a compiled one (`layouts::settings`) does.

### replace

*public bool replace(const gui::Layout & layout)*



**brief** Show `layout` instead of the screen there is (or as the first). False, and reported, when `layout` is a pane.

### back

*public bool back()*



**brief** What the back button does: close the top pane if one is open, or else go back to the screen beneath. False at the root with no pane open: nothing to go back to.

### open

*public bool open(const gui::Layout & layout)*



**brief** Open the pane `layout` over the screen shown: its file's `place` and `modal` say where and how. False, and reported, when `max_panes` are open or `layout` is a screen. The pane is built when it opens and freed when it closes; it outlives nothing.

### close

*public bool close()*



**brief** Close the top pane. False when none is open. 

### custom

*public bool custom(std::string_view name, CustomMaker make)*



**brief** Make a layout's custom node `name` with `make`. `name` must outlive the Stage: a literal does. False, and reported, when `max_added` are already there or `name` is.

### depth

*public std::size_t depth()*



**brief** How many screens there are, once what waits has happened. 

### operator=

*public Stage & operator=(const Stage & )*

*Defined at [stage/stage.hpp:157](../../../stage/stage.hpp#L157)*

### operator=

*public Stage & operator=(Stage && )*

*Defined at [stage/stage.hpp:159](../../../stage/stage.hpp#L159)*

### push

*public bool push()*

*Defined at [stage/stage.hpp:178](../../../stage/stage.hpp#L178)*



**brief** Show the C++ screen `S` over the screen there is; as `push( layout )`. 

### replace

*public void replace()*

*Defined at [stage/stage.hpp:185](../../../stage/stage.hpp#L185)*



**brief** Show the C++ screen `S` instead of the screen there is (or as the first). 

### open

*public bool open()*

*Defined at [stage/stage.hpp:207](../../../stage/stage.hpp#L207)*



**brief** Open the C++ screen `S` as a pane; its `place` and `modal`, if it has them. 

### panes

*public std::size_t panes()*



**brief** How many panes are open, once what waits has happened. 

### top

*public const gui::Layout * top()*



**brief** The table of the screen shown: a layout, or what a C++ screen wrote (valid while it is shown, until a C++ pane is written over it: they share one scratch); nullptr when there is none, or it is not built (no window yet).

### top_name

*public std::string_view top_name()*



**brief** The name of the screen shown; empty when there is none. 

### pane_name

*public std::string_view pane_name()*



**brief** The name of the top pane; empty when none is open. 

### find

*public static const gui::Layout * find(std::string_view name)*



**brief** The app's layout called `name`, from its layout files (sqgen's `layouts::all()`); nullptr when there is none.

### bindings

*public std::size_t bindings()*



**brief** How many widgets on the screen and its panes show a value of the model's. 

### showing

*public bool showing()*

*Defined at [stage/stage.hpp:255](../../../stage/stage.hpp#L255)*



**brief** Whether the C++ screen `S` is the one shown. 

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/stage.hpp:267](../../../stage/stage.hpp#L267)*



**brief** The UI end of the Telegraph: send, and read what the model said. 

### failed

*public bool failed()*

*Defined at [stage/stage.hpp:279](../../../stage/stage.hpp#L279)*



**brief** The skin could not be loaded: the Stage draws dark red. 

### ui

*public gui::Ui * ui()*



**brief** The top screen's widgets, its panes' among them; nullptr while the skin is not loaded (no window yet, or it failed).



