# class App

*Defined at [stage/app.hpp:107](../../../stage/app.hpp#L107)*



**brief** The app: the Telegraph, the model at its far end, and the Stage at its near one. One for the whole process.

 The platform makes it once and keeps it while the process lives, so an activity Android recreates reattaches to the same App and the model keeps running. It calls `squared_main` once, when the first window is there and the skin is loaded; the app pushes its first screen there and starts its model with `run`. Quitting (`quit()`, a `quit` tap, back at the root) stops the model, after it saves, and empties the stack: the next launch in the same process starts afresh.

 It is the platform's `sq::app::Application`: an app does not call those methods, the platform does.

 What it keeps (7.6), in the app's private files, `squared/`: the screens, written when the app goes to the background and shown again at the next start (once: a quit forgets them, and so does showing them), unless `stage.keep_screens( false )`; and a model's own save (`sq::Save`), written on the model's thread after each `background` and given back to it before its next `start`.



Inherits from Application



## Members

public Stage stage



## Functions

### App

*public void App(const App & )*

*Defined at [stage/app.hpp:119](../../../stage/app.hpp#L119)*

### App

*public void App()*

### App

*public void App(App && )*

*Defined at [stage/app.hpp:121](../../../stage/app.hpp#L121)*

### ~App

*public void ~App()*

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*

 sq::app::Application, for the platform.

### set_report

*public void set_report(app::Report report)*

### create

*public bool create(app::Runtime & runtime)*

### handle_event

*public void handle_event(const app::Event & event)*

### operator=

*public App & operator=(const App & )*

*Defined at [stage/app.hpp:120](../../../stage/app.hpp#L120)*

### operator=

*public App & operator=(App && )*

*Defined at [stage/app.hpp:122](../../../stage/app.hpp#L122)*

### run

*public bool run(Args &&... args)*

*Defined at [stage/app.hpp:138](../../../stage/app.hpp#L138)*



**brief** Start the model `M` at the far end of the Telegraph, made from `args` on its own thread (named "model"), or on this one with `messaging::on_ui_thread` first, one turn a frame. One model to an app: false, and reported, for a second.

 A model with `save( sq::Save & )` keeps what it writes there: it is called after each `background`, and the last save's values come back to its `receive`, as `set`s, before its `start`.

 Allocates the runner, the model inside it, once; and a model that saves, its last save, read here and freed once given back.

### running

*public bool running()*

*Defined at [stage/app.hpp:156](../../../stage/app.hpp#L156)*



**brief** Whether a model runs. 

### quit

*public void quit()*

*Defined at [stage/app.hpp:159](../../../stage/app.hpp#L159)*



**brief** Quit: the model saves and stops, and the platform closes the app. 

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/app.hpp:162](../../../stage/app.hpp#L162)*



**brief** The UI end of the Telegraph (the Stage's). 

### update

*public void update(std::chrono::nanoseconds delta)*

### render

*public void render(graphics::Context & graphics)*

### foreground

*public void foreground()*

### background

*public void background()*

### low_memory

*public void low_memory()*

### resize

*public void resize(int width, int height)*

### safe_area_changed

*public void safe_area_changed(const app::SafeArea & area)*

### surface_created

*public void surface_created(graphics::Context & graphics)*

### dispose

*public void dispose()*

### quit_requested

*public bool quit_requested()*



