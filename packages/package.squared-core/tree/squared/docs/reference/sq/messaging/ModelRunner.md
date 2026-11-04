# class ModelRunner

*Defined at [messaging/model_runner.cppm:98](../../../../messaging/model_runner.cppm#L98)*



**brief** Runs a model at the far end of a `TelegraphLine`: on its own thread (named "model"), or on the UI thread a turn at a time.

 On its own thread the model is made there, from the arguments (copied, as `std::thread` copies them: `std::ref` for a reference), and it sleeps between turns until the UI sends, Android's lifecycle reaches it, a `tick` is due, or its `wake_at()` time comes. A turn is `deliver`, then `tick` if due, then `due` if its time has come; the clock is read once a turn. The runner's destructor asks the model to quit (it saves, in `background`), wakes it, and waits for its thread: the model is destroyed on the thread it lived on.

 One runner to a line. The model lives inside the runner, not on a stack.



## Functions

### ModelRunner<M>

*public void ModelRunner<M>(ModelRunner<M> && )*

*Defined at [messaging/model_runner.cppm:139](../../../../messaging/model_runner.cppm#L139)*

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, Args &&... args)*

*Defined at [messaging/model_runner.cppm:110](../../../../messaging/model_runner.cppm#L110)*



**brief** The model on its own thread, made there from `args`. 

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, OnUiThread , Args &&... args)*

*Defined at [messaging/model_runner.cppm:129](../../../../messaging/model_runner.cppm#L129)*



**brief** The model on the UI thread, made now: call `turn()` once a frame (ticks and timers run there). 

### ModelRunner<M>

*public void ModelRunner<M>(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.cppm:137](../../../../messaging/model_runner.cppm#L137)*

### operator=

*public ModelRunner<M> & operator=(ModelRunner<M> && )*

*Defined at [messaging/model_runner.cppm:140](../../../../messaging/model_runner.cppm#L140)*

### operator=

*public ModelRunner<M> & operator=(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.cppm:138](../../../../messaging/model_runner.cppm#L138)*

### threaded

*public bool threaded()*

*Defined at [messaging/model_runner.cppm:178](../../../../messaging/model_runner.cppm#L178)*



**brief** Whether the model runs on its own thread. 

### turn

*public std::uint32_t turn()*

*Defined at [messaging/model_runner.cppm:161](../../../../messaging/model_runner.cppm#L161)*



**brief** Inline only: one turn of the model, on the UI thread. The telegrams it handled; 0 once it has quit.

### ~ModelRunner<M>

*public void ~ModelRunner<M>()*

*Defined at [messaging/model_runner.cppm:142](../../../../messaging/model_runner.cppm#L142)*

