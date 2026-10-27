# class Stage

*Defined at [stage/stage.hpp:136](../../../stage/stage.hpp#L136)*



**brief** The view: a stack of screens, each a layout or a C++ screen, with the skin, the drawing, input, and the UI end of the Telegraph.

 Only the top screen has widgets: a screen that comes back (`back`) is built again (a layout from its table, a C++ screen by its `build`). Panes open over the top screen and belong to it: leaving the screen closes them. A button's tap is sent as it reads: a telegram to the model, or a navigation to this stack. The App owns the Stage and calls the rest; an app calls `push`, `replace`, `back`, `open`, `close`, and `add` and `custom` to name what its layout files reach.

 Navigation asked for while an event is being delivered (a tap) waits until delivery has finished: building the next screen replaces the widgets, and the tapped button is one of them.



## Members

public static const std::size_t max_depth

public static const std::size_t max_added

public static const std::size_t max_panes

public static const std::size_t max_bindings

public static const std::size_t max_lists

public static const std::size_t max_row_values

public static const std::size_t kept_scrolls

public static const std::size_t kept_text

public static const std::size_t saved_bytes

public static const std::size_t scratch_nodes

public static const std::size_t scratch_text

public static const std::size_t scratch_numbers



## Functions

### Stage

*public void Stage(const Stage & )*

*Defined at [stage/stage.hpp:176](../../../stage/stage.hpp#L176)*

### Stage

*public void Stage(messaging::TelegraphUi & telegraph)*

### Stage

*public void Stage(Stage && )*

*Defined at [stage/stage.hpp:178](../../../stage/stage.hpp#L178)*

### add

*public bool add()*

*Defined at [stage/stage.hpp:241](../../../stage/stage.hpp#L241)*



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

*Defined at [stage/stage.hpp:177](../../../stage/stage.hpp#L177)*

### operator=

*public Stage & operator=(Stage && )*

*Defined at [stage/stage.hpp:179](../../../stage/stage.hpp#L179)*

### push

*public bool push()*

*Defined at [stage/stage.hpp:198](../../../stage/stage.hpp#L198)*



**brief** Show the C++ screen `S` over the screen there is; as `push( layout )`. 

### replace

*public void replace()*

*Defined at [stage/stage.hpp:205](../../../stage/stage.hpp#L205)*



**brief** Show the C++ screen `S` instead of the screen there is (or as the first). 

### open

*public bool open()*

*Defined at [stage/stage.hpp:227](../../../stage/stage.hpp#L227)*



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

### lists

*public std::size_t lists()*



**brief** How many lists the screen and its panes show. 

### describe

*public void describe(app::Report out)*



**brief** The stack, a line a screen from the root, then the panes over the top, each with what it keeps: what the Console's `screens` shows.

### showing

*public bool showing()*

*Defined at [stage/stage.hpp:275](../../../stage/stage.hpp#L275)*



**brief** Whether the C++ screen `S` is the one shown. 

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/stage.hpp:287](../../../stage/stage.hpp#L287)*



**brief** The UI end of the Telegraph: send, and read what the model said. 

### failed

*public bool failed()*

*Defined at [stage/stage.hpp:296](../../../stage/stage.hpp#L296)*



**brief** The skin could not be loaded: the Stage draws dark red. 

### keep_screens

*public void keep_screens(bool keep)*

*Defined at [stage/stage.hpp:304](../../../stage/stage.hpp#L304)*



**brief** Whether the stack is kept when the app goes to the background, to be shown again, panes, scrolls and an unfinished edit with it, at the next start after Android ended the process: on, unless an app says otherwise in `squared_main`. A quit forgets it.

### keeps_screens

*public bool keeps_screens()*

*Defined at [stage/stage.hpp:305](../../../stage/stage.hpp#L305)*



