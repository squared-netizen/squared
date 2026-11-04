# class App

*Defined at [stage/app.cppm:97](../../../stage/app.cppm#L97)*



**brief** The app: the Telegraph, the model at its far end, and the Stage at its near one. One for the whole process.

 The platform makes it once and keeps it while the process lives, so an activity Android recreates reattaches to the same App and the model keeps running. It calls `squared_main` once, when the first window is there and the skin is loaded; the app pushes its first screen there and starts its model with `run`. Quitting (`quit()`, a `quit` tap) stops the model, after it saves, and empties the stack: the next launch in the same process starts afresh. Back at the root screen sends the app to the background instead.

 It is the platform's `sq::app::Application`: an app does not call those methods, the platform does.

 What it keeps: the screens, in what Android keeps for the activity (its saved instance state, 7.8), shown again only when Android brings the app back after ending its process; a swipe from the recent apps or a quit starts it fresh. And a model's own save (`sq::Save`), in the app's private files (`squared/model.state`), written on the model's thread after each `background` and given back to it before its next `start`, whatever ended the last run.



Inherits from Application



## Members

public Stage stage



## Functions

### App

*public void App()*

### App

*public void App(App && )*

*Defined at [stage/app.cppm:111](../../../stage/app.cppm#L111)*

### App

*public void App(const App & )*

*Defined at [stage/app.cppm:109](../../../stage/app.cppm#L109)*

### background

*public void background()*

### create

*public bool create(app::Runtime & runtime)*

### dispose

*public void dispose()*

### foreground

*public void foreground()*

### handle_event

*public void handle_event(const app::Event & event)*

### hides_bars

*public bool hides_bars()*

*Defined at [stage/app.cppm:173](../../../stage/app.cppm#L173)*

### low_memory

*public void low_memory()*

### operator=

*public App & operator=(App && )*

*Defined at [stage/app.cppm:112](../../../stage/app.cppm#L112)*

### operator=

*public App & operator=(const App & )*

*Defined at [stage/app.cppm:110](../../../stage/app.cppm#L110)*

### quit

*public void quit()*

*Defined at [stage/app.cppm:149](../../../stage/app.cppm#L149)*



**brief** Quit: the model saves and stops, and the platform closes the app. 

### quit_requested

*public bool quit_requested()*

### render

*public void render(graphics::Context & graphics)*

### resize

*public void resize(int width, int height)*

### run

*public bool run(Args &&... args)*

*Defined at [stage/app.cppm:128](../../../stage/app.cppm#L128)*



**brief** Start the model `M` at the far end of the Telegraph, made from `args` on its own thread (named "model"), or on this one with `messaging::on_ui_thread` first, one turn a frame. One model to an app: false, and reported, for a second.

 A model with `save( sq::Save & )` keeps what it writes there: it is called after each `background`, and the last save's values come back to its `receive`, as `set`s, before its `start`.

 Allocates the runner, the model inside it, once; and a model that saves, its last save, read here and freed once given back.

### running

*public bool running()*

*Defined at [stage/app.cppm:146](../../../stage/app.cppm#L146)*



**brief** Whether a model runs. 

### safe_area_changed

*public void safe_area_changed(const app::SafeArea & area)*

### save_state

*public std::size_t save_state(std::span<std::byte> out)*

### set_clipboard_service

*public void set_clipboard_service(app::ClipboardService * service)*

### set_report

*public void set_report(app::Report report)*

### set_saved_state

*public void set_saved_state(std::span<const std::byte> saved)*

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*

 sq::app::Application, for the platform.

### surface_created

*public void surface_created(graphics::Context & graphics)*

### take_background_request

*public bool take_background_request()*

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/app.cppm:152](../../../stage/app.cppm#L152)*



**brief** The UI end of the Telegraph (the Stage's). 

### update

*public void update(std::chrono::nanoseconds delta)*

### ~App

*public void ~App()*

