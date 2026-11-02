# class ModelRunner

*Defined at [messaging/model_runner.cppm:96](../../../../messaging/model_runner.cppm#L96)*



**brief** Runs a model at the far end of a `TelegraphLine`: on its own thread (named "model"), or on the UI thread a turn at a time.

 On its own thread the model is made there, from the arguments (copied, as `std::thread` copies them: `std::ref` for a reference), and it sleeps between turns until the UI sends, Android's lifecycle reaches it, a `tick` is due, or its `wake_at()` time comes. A turn is `deliver`, then `tick` if due, then `due` if its time has come; the clock is read once a turn. The runner's destructor asks the model to quit (it saves, in `background`), wakes it, and waits for its thread: the model is destroyed on the thread it lived on.

 One runner to a line. The model lives inside the runner, not on a stack.



## Functions

### ModelRunner<M>

*public void ModelRunner<M>(ModelRunner<M> && )*

*Defined at [messaging/model_runner.cppm:137](../../../../messaging/model_runner.cppm#L137)*

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, Args &&... args)*

*Defined at [messaging/model_runner.cppm:108](../../../../messaging/model_runner.cppm#L108)*



**brief** The model on its own thread, made there from `args`. 

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, OnUiThread , Args &&... args)*

*Defined at [messaging/model_runner.cppm:127](../../../../messaging/model_runner.cppm#L127)*



**brief** The model on the UI thread, made now: call `turn()` once a frame (ticks and timers run there). 

### ModelRunner<M>

*public void ModelRunner<M>(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.cppm:135](../../../../messaging/model_runner.cppm#L135)*

### operator=

*public ModelRunner<M> & operator=(ModelRunner<M> && )*

*Defined at [messaging/model_runner.cppm:138](../../../../messaging/model_runner.cppm#L138)*

### operator=

*public ModelRunner<M> & operator=(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.cppm:136](../../../../messaging/model_runner.cppm#L136)*

### threaded

*public bool threaded()*

*Defined at [messaging/model_runner.cppm:172](../../../../messaging/model_runner.cppm#L172)*



**brief** Whether the model runs on its own thread. 

### turn

*public std::uint32_t turn()*

*Defined at [messaging/model_runner.cppm:157](../../../../messaging/model_runner.cppm#L157)*



**brief** Inline only: one turn of the model, on the UI thread. The telegrams it handled; 0 once it has quit.

### ~ModelRunner<M>

*public void ~ModelRunner<M>()*

*Defined at [messaging/model_runner.cppm:140](../../../../messaging/model_runner.cppm#L140)*

