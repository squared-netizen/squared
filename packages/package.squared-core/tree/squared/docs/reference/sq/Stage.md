# class Stage

*Defined at [stage/stage.hpp:94](../../../stage/stage.hpp#L94)*



**brief** The view: a stack of screens, each a layout or a C++ screen, with the skin, the drawing, input, and the UI end of the Telegraph.

 Only the top screen has widgets: a screen that comes back (`back`) is built again (a layout from its table, a C++ screen by its `build`). A button's tap is sent as it reads: a telegram to the model, or a navigation to this stack. The App owns the Stage and calls the rest; an app calls `push`, `replace`, `back`, and `add` and `custom` to name what its layout files reach.

 Navigation asked for while an event is being delivered (a tap) waits until delivery has finished: building the next screen replaces the widgets, and the tapped button is one of them.



## Members

public static const std::size_t max_depth

public static const std::size_t max_added

public static const std::size_t scratch_nodes

public static const std::size_t scratch_text

public static const std::size_t scratch_numbers



## Functions

### Stage

*public void Stage(messaging::TelegraphUi & telegraph)*

### Stage

*public void Stage(const Stage & )*

*Defined at [stage/stage.hpp:110](../../../stage/stage.hpp#L110)*

### operator=

*public Stage & operator=(const Stage & )*

*Defined at [stage/stage.hpp:111](../../../stage/stage.hpp#L111)*

### Stage

*public void Stage(Stage && )*

*Defined at [stage/stage.hpp:112](../../../stage/stage.hpp#L112)*

### operator=

*public Stage & operator=(Stage && )*

*Defined at [stage/stage.hpp:113](../../../stage/stage.hpp#L113)*

### ~Stage

*public void ~Stage()*

### push

*public bool push(const gui::Layout & layout)*



**brief** Show `layout` over the screen there is, which keeps nothing but its layout. False when the stack is full (`max_depth`); the problem is reported. `layout` must outlive its place on the stack: a compiled one (`layouts::settings`) does.

### push

*public bool push()*

*Defined at [stage/stage.hpp:128](../../../stage/stage.hpp#L128)*



**brief** Show the C++ screen `S` over the screen there is; as `push( layout )`. 

### replace

*public void replace(const gui::Layout & layout)*



**brief** Show `layout` instead of the screen there is (or as the first). 

### back

*public bool back()*



**brief** Back to the screen beneath. False at the root: nothing to go back to. 

### replace

*public void replace()*

*Defined at [stage/stage.hpp:135](../../../stage/stage.hpp#L135)*



**brief** Show the C++ screen `S` instead of the screen there is (or as the first). 

### add

*public bool add()*

*Defined at [stage/stage.hpp:149](../../../stage/stage.hpp#L149)*



**brief** Let taps reach the C++ screen `S` by its name (`push about`), as names.json's `screens` lists it. In `squared_main`. False, and reported, when `max_added` are already added or another has the name.

### custom

*public bool custom(std::string_view name, CustomMaker make)*



**brief** Make a layout's custom node `name` with `make`. `name` must outlive the Stage: a literal does. False, and reported, when `max_added` are already there or `name` is.

### depth

*public std::size_t depth()*



**brief** How many screens there are, once what waits has happened. 

### showing

*public bool showing()*

*Defined at [stage/stage.hpp:176](../../../stage/stage.hpp#L176)*



**brief** Whether the C++ screen `S` is the one shown. 

### top

*public const gui::Layout * top()*



**brief** The table of the screen shown: a layout, or what a C++ screen wrote (valid while it is shown); nullptr when there is none, or it is not built (no window yet).

### top_name

*public std::string_view top_name()*



**brief** The name of the screen shown; empty when there is none. 

### find

*public static const gui::Layout * find(std::string_view name)*



**brief** The app's layout called `name`, from its layout files (sqgen's `layouts::all()`); nullptr when there is none.

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/stage.hpp:188](../../../stage/stage.hpp#L188)*



**brief** The UI end of the Telegraph: send, and read what the model said. 

### failed

*public bool failed()*

*Defined at [stage/stage.hpp:197](../../../stage/stage.hpp#L197)*



**brief** The skin could not be loaded: the Stage draws dark red. 

### ui

*public gui::Ui * ui()*



**brief** The top screen's widgets; nullptr while the skin is not loaded (no window yet, or it failed).



