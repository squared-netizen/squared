# class Stage

*Defined at [stage/stage.hpp:50](../../../stage/stage.hpp#L50)*



**brief** The view: a stack of screens, each a layout, with the skin, the drawing, input, and the UI end of the Telegraph.

 Only the top screen has widgets: a screen that comes back (`back`) is built again from its layout. A button's tap is sent as it reads: a telegram to the model, or a navigation to this stack. The App owns the Stage and calls the rest; an app calls `push`, `replace` and `back`.

 Navigation asked for while an event is being delivered (a tap) waits until delivery has finished: building the next screen replaces the widgets, and the tapped button is one of them.



## Members

public static const std::size_t max_depth



## Functions

### Stage

*public void Stage(messaging::TelegraphUi & telegraph)*

### Stage

*public void Stage(const Stage & )*

*Defined at [stage/stage.hpp:58](../../../stage/stage.hpp#L58)*

### operator=

*public Stage & operator=(const Stage & )*

*Defined at [stage/stage.hpp:59](../../../stage/stage.hpp#L59)*

### Stage

*public void Stage(Stage && )*

*Defined at [stage/stage.hpp:60](../../../stage/stage.hpp#L60)*

### operator=

*public Stage & operator=(Stage && )*

*Defined at [stage/stage.hpp:61](../../../stage/stage.hpp#L61)*

### ~Stage

*public void ~Stage()*

### push

*public bool push(const gui::Layout & layout)*



**brief** Show `layout` over the screen there is, which keeps nothing but its layout. False when the stack is full (`max_depth`); the problem is reported. `layout` must outlive its place on the stack: a compiled one (`layouts::settings`) does.

### replace

*public void replace(const gui::Layout & layout)*



**brief** Show `layout` instead of the screen there is (or as the first). 

### back

*public bool back()*



**brief** Back to the screen beneath. False at the root: nothing to go back to. 

### depth

*public std::size_t depth()*



**brief** How many screens there are, once what waits has happened. 

### top

*public const gui::Layout * top()*



**brief** The screen shown; nullptr when there is none. 

### find

*public static const gui::Layout * find(std::string_view name)*



**brief** The app's layout called `name`, from its layout files (sqgen's `layouts::all()`); nullptr when there is none.

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/stage.hpp:90](../../../stage/stage.hpp#L90)*



**brief** The UI end of the Telegraph: send, and read what the model said. 

### failed

*public bool failed()*

*Defined at [stage/stage.hpp:99](../../../stage/stage.hpp#L99)*



**brief** The skin could not be loaded: the Stage draws dark red. 

### ui

*public gui::Ui * ui()*



**brief** The top screen's widgets; nullptr while the skin is not loaded (no window yet, or it failed).



