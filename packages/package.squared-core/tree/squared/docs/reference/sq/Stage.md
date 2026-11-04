# class Stage

*Defined at [stage/stage.cppm:109](../../../stage/stage.cppm#L109)*



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

public static const int least_zoom

public static const int most_zoom

public static const int zoom_step



## Functions

### Stage

*public void Stage(Stage && )*

*Defined at [stage/stage.cppm:151](../../../stage/stage.cppm#L151)*

### Stage

*public void Stage(const Stage & )*

*Defined at [stage/stage.cppm:149](../../../stage/stage.cppm#L149)*

### Stage

*public void Stage(messaging::TelegraphUi & telegraph)*

### add

*public bool add()*

*Defined at [stage/stage.cppm:214](../../../stage/stage.cppm#L214)*



**brief** Let taps reach the C++ screen `S` by its name (`push about`), as names.json's `screens` lists it. In `squared_main`. False, and reported, when `max_added` are already added or another has the name.

### back

*public bool back()*



**brief** What the back button does: close the top pane if one is open, or else go back to the screen beneath. False at the root with no pane open: nothing to go back to.

### bindings

*public std::size_t bindings()*



**brief** How many widgets on the screen and its panes show a value of the model's. 

### clipboard

*public app::ClipboardService * clipboard()*

*Defined at [stage/stage.cppm:277](../../../stage/stage.cppm#L277)*



**brief** The platform's clipboard: what a paste reads, a copy writes; nullptr where the platform gives none (a host test, until it does).



 if( sq::app::ClipboardService *clip = stage.clipboard() ) { clip->set_text( "copied" ); }

### close

*public bool close()*



**brief** Close the top pane. False when none is open. 

### custom

*public bool custom(std::string_view name, CustomMaker make)*



**brief** Make a layout's custom node `name` with `make`. `name` must outlive the Stage: a literal does. False, and reported, when `max_added` are already there or `name` is.

### depth

*public std::size_t depth()*



**brief** How many screens there are, once what waits has happened. 

### describe

*public void describe(app::Report out)*



**brief** The stack, a line a screen from the root, then the panes over the top, each with what it keeps: what the Console's `screens` shows.

### failed

*public bool failed()*

*Defined at [stage/stage.cppm:302](../../../stage/stage.cppm#L302)*



**brief** The skin could not be loaded: the Stage draws dark red. 

### files

*public files::FileSystem * files()*

*Defined at [stage/stage.cppm:267](../../../stage/stage.cppm#L267)*



**brief** The app's files (the platform's: `Internal` is what the APK bundles), for a custom widget that reads its own (a font to `map`); nullptr before the first window.

### find

*public static const gui::Layout * find(std::string_view name)*



**brief** The app's layout called `name`, from its layout files (sqgen's `layouts::all()`); nullptr when there is none.

### hide_bars

*public void hide_bars(bool hide)*

*Defined at [stage/stage.cppm:320](../../../stage/stage.cppm#L320)*



**brief** Whether the system bars (status and navigation) are hidden: on by default. A swipe from the edge shows them for a moment. A tool that wants them, such as an editor, turns this off; it takes effect at the next frame.

### hides_bars

*public bool hides_bars()*

*Defined at [stage/stage.cppm:321](../../../stage/stage.cppm#L321)*

### keep_screens

*public void keep_screens(bool keep)*

*Defined at [stage/stage.cppm:311](../../../stage/stage.cppm#L311)*



**brief** Whether the stack is kept for Android, to be shown again, panes, scrolls and an unfinished edit with it, when Android brings the app back after ending its process: on, unless an app says otherwise in `squared_main`. A swipe from the recent apps or a quit starts the app fresh either way.

### keeps_screens

*public bool keeps_screens()*

*Defined at [stage/stage.cppm:312](../../../stage/stage.cppm#L312)*

### lists

*public std::size_t lists()*



**brief** How many lists the screen and its panes show. 

### on_surface

*public void on_surface(SurfaceHook hook, void * context)*



**brief** Call `hook( context, graphics )` each time the drawing surface is made (the first, and each one after Android took it away), before the screen is built again: what an app's own textures need to come back. One; a later call replaces it, and null removes it. `context` must outlive the Stage or be removed.

### open

*public bool open()*

*Defined at [stage/stage.cppm:200](../../../stage/stage.cppm#L200)*



**brief** Open the C++ screen `S` as a pane; its `place` and `modal`, if it has them. 

### open

*public bool open(const gui::Layout & layout)*



**brief** Open the pane `layout` over the screen shown: its file's `place` and `modal` say where and how. False, and reported, when `max_panes` are open or `layout` is a screen. The pane is built when it opens and freed when it closes; it outlives nothing.

### operator=

*public Stage & operator=(Stage && )*

*Defined at [stage/stage.cppm:152](../../../stage/stage.cppm#L152)*

### operator=

*public Stage & operator=(const Stage & )*

*Defined at [stage/stage.cppm:150](../../../stage/stage.cppm#L150)*

### pane_name

*public std::string_view pane_name()*



**brief** The name of the top pane; empty when none is open. 

### panes

*public std::size_t panes()*



**brief** How many panes are open, once what waits has happened. 

### pinch_zoom

*public void pinch_zoom(bool on)*

*Defined at [stage/stage.cppm:349](../../../stage/stage.cppm#L349)*



**brief** Whether a pinch zooms the UI: on by default. Off, both fingers reach the widgets as touches, as a game's canvas wants, and no widget is offered a pinch. In `squared_main`.

### pinch_zooms

*public bool pinch_zooms()*

*Defined at [stage/stage.cppm:350](../../../stage/stage.cppm#L350)*

### pixels_per_unit

*public float pixels_per_unit()*



**brief** Pixels to one logical unit (the drawable's shorter side is 720 units), for a custom widget that draws pixel-exact, as a terminal's glyphs are; 0 before the first window.

### push

*public bool push()*

*Defined at [stage/stage.cppm:171](../../../stage/stage.cppm#L171)*



**brief** Show the C++ screen `S` over the screen there is; as `push( layout )`. 

### push

*public bool push(const gui::Layout & layout)*



**brief** Show `layout` over the screen there is, which keeps nothing but its layout, and closes its panes. False when the stack is full (`max_depth`) or `layout` is a pane; the problem is reported. `layout` must outlive its place on the stack: a compiled one (`layouts::settings`) does.

### replace

*public bool replace(const gui::Layout & layout)*



**brief** Show `layout` instead of the screen there is (or as the first). False, and reported, when `layout` is a pane.

### replace

*public void replace()*

*Defined at [stage/stage.cppm:178](../../../stage/stage.cppm#L178)*



**brief** Show the C++ screen `S` instead of the screen there is (or as the first). 

### set_zoom

*public void set_zoom(int percent)*



**brief** Set the zoom (a settings screen's): brought into the range, to the nearest step, laid out at the next frame, and kept as a pinch's is.

 app.stage.set_zoom( 150 );

### showing

*public bool showing()*

*Defined at [stage/stage.cppm:248](../../../stage/stage.cppm#L248)*



**brief** Whether the C++ screen `S` is the one shown. 

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/stage.cppm:260](../../../stage/stage.cppm#L260)*



**brief** The UI end of the Telegraph: send, and read what the model said. 

### top

*public const gui::Layout * top()*



**brief** The table of the screen shown: a layout, or what a C++ screen wrote (valid while it is shown, until a C++ pane is written over it: they share one scratch); nullptr when there is none, or it is not built (no window yet).

### top_name

*public std::string_view top_name()*



**brief** The name of the screen shown; empty when there is none. 

### zoom

*public int zoom()*



**brief** How big the UI is, in percent: at 100 the drawable's shorter side is 720 units; at 200, 360 (everything twice as big). A two-finger pinch changes it in every squared app, laid out once a step; it is kept in the app's private files (`squared/zoom`) for the next launch. A widget may take the pinch instead (`gui::Widget::pinch`): a terminal's font.

### ~Stage

*public void ~Stage()*

