# class App

*Defined at [stage/app.hpp:111](../../../stage/app.hpp#L111)*



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

*public void App(const App & )*

*Defined at [stage/app.hpp:123](../../../stage/app.hpp#L123)*

### dispose

*public void dispose()*

### operator=

*public App & operator=(const App & )*

*Defined at [stage/app.hpp:124](../../../stage/app.hpp#L124)*

### App

*public void App(App && )*

*Defined at [stage/app.hpp:125](../../../stage/app.hpp#L125)*

### create

*public bool create(app::Runtime & runtime)*

### handle_event

*public void handle_event(const app::Event & event)*

### foreground

*public void foreground()*

### background

*public void background()*

### operator=

*public App & operator=(App && )*

*Defined at [stage/app.hpp:126](../../../stage/app.hpp#L126)*

### run

*public bool run(Args &&... args)*

*Defined at [stage/app.hpp:142](../../../stage/app.hpp#L142)*



**brief** Start the model `M` at the far end of the Telegraph, made from `args` on its own thread (named "model"), or on this one with `messaging::on_ui_thread` first, one turn a frame. One model to an app: false, and reported, for a second.

 A model with `save( sq::Save & )` keeps what it writes there: it is called after each `background`, and the last save's values come back to its `receive`, as `set`s, before its `start`.

 Allocates the runner, the model inside it, once; and a model that saves, its last save, read here and freed once given back.

### running

*public bool running()*

*Defined at [stage/app.hpp:160](../../../stage/app.hpp#L160)*



**brief** Whether a model runs. 

### quit

*public void quit()*

*Defined at [stage/app.hpp:163](../../../stage/app.hpp#L163)*



**brief** Quit: the model saves and stops, and the platform closes the app. 

### telegraph

*public messaging::TelegraphUi & telegraph()*

*Defined at [stage/app.hpp:166](../../../stage/app.hpp#L166)*



**brief** The UI end of the Telegraph (the Stage's). 

### hides_bars

*public bool hides_bars()*

*Defined at [stage/app.hpp:186](../../../stage/app.hpp#L186)*

### ~App

*public void ~App()*

### set_text_input_service

*public void set_text_input_service(app::TextInputService * service)*

 sq::app::Application, for the platform.

### set_report

*public void set_report(app::Report report)*

### update

*public void update(std::chrono::nanoseconds delta)*

### render

*public void render(graphics::Context & graphics)*

### low_memory

*public void low_memory()*

### resize

*public void resize(int width, int height)*

### safe_area_changed

*public void safe_area_changed(const app::SafeArea & area)*

### surface_created

*public void surface_created(graphics::Context & graphics)*

### quit_requested

*public bool quit_requested()*

### set_saved_state

*public void set_saved_state(std::span<const std::byte> saved)*

### save_state

*public std::size_t save_state(std::span<std::byte> out)*

### take_background_request

*public bool take_background_request()*



