# class ModelRunner

*Defined at [messaging/model_runner.hpp:100](../../../../messaging/model_runner.hpp#L100)*



**brief** Runs a model at the far end of a `TelegraphLine`: on its own thread (named "model"), or on the UI thread a turn at a time.

 On its own thread the model is made there, from the arguments (copied, as `std::thread` copies them: `std::ref` for a reference), and it sleeps between turns until the UI sends, Android's lifecycle reaches it, a `tick` is due, or its `wake_at()` time comes. A turn is `deliver`, then `tick` if due, then `due` if its time has come; the clock is read once a turn. The runner's destructor asks the model to quit (it saves, in `background`), wakes it, and waits for its thread: the model is destroyed on the thread it lived on.

 One runner to a line. The model lives inside the runner, not on a stack.



## Functions

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, Args &&... args)*

*Defined at [messaging/model_runner.hpp:108](../../../../messaging/model_runner.hpp#L108)*



**brief** The model on its own thread, made there from `args`. 

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, OnUiThread , Args &&... args)*

*Defined at [messaging/model_runner.hpp:126](../../../../messaging/model_runner.hpp#L126)*



**brief** The model on the UI thread, made now: call `turn()` once a frame (ticks and timers run there). 

### ModelRunner<M>

*public void ModelRunner<M>(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.hpp:134](../../../../messaging/model_runner.hpp#L134)*

### operator=

*public ModelRunner<M> & operator=(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.hpp:135](../../../../messaging/model_runner.hpp#L135)*

### ModelRunner<M>

*public void ModelRunner<M>(ModelRunner<M> && )*

*Defined at [messaging/model_runner.hpp:136](../../../../messaging/model_runner.hpp#L136)*

### operator=

*public ModelRunner<M> & operator=(ModelRunner<M> && )*

*Defined at [messaging/model_runner.hpp:137](../../../../messaging/model_runner.hpp#L137)*

### ~ModelRunner<M>

*public void ~ModelRunner<M>()*

*Defined at [messaging/model_runner.hpp:139](../../../../messaging/model_runner.hpp#L139)*

### turn

*public std::uint32_t turn()*

*Defined at [messaging/model_runner.hpp:156](../../../../messaging/model_runner.hpp#L156)*



**brief** Inline only: one turn of the model, on the UI thread. The telegrams it handled; 0 once it has quit.

### threaded

*public bool threaded()*

*Defined at [messaging/model_runner.hpp:171](../../../../messaging/model_runner.hpp#L171)*



**brief** Whether the model runs on its own thread. 



