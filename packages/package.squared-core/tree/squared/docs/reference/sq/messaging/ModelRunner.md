# class ModelRunner

*Defined at [messaging/model_runner.hpp:79](../../../../messaging/model_runner.hpp#L79)*



**brief** Runs a model at the far end of a `TelegraphLine`: on its own thread (named "model"), or on the UI thread a turn at a time.

 On its own thread the model is made there, from the arguments (copied, as `std::thread` copies them: `std::ref` for a reference), and it sleeps between turns until the UI sends, Android's lifecycle reaches it, or a `tick` is due. A turn is `deliver`, then `tick` if due. The runner's destructor asks the model to quit (it saves, in `background`), wakes it, and waits for its thread: the model is destroyed on the thread it lived on.

 One runner to a line. The model lives inside the runner, not on a stack.



## Functions

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, Args &&... args)*

*Defined at [messaging/model_runner.hpp:87](../../../../messaging/model_runner.hpp#L87)*



**brief** The model on its own thread, made there from `args`. 

### ModelRunner<M>

*public void ModelRunner<M>(TelegraphLine & line, OnUiThread , Args &&... args)*

*Defined at [messaging/model_runner.hpp:105](../../../../messaging/model_runner.hpp#L105)*



**brief** The model on the UI thread, made now: call `turn()` once a frame. 

### ModelRunner<M>

*public void ModelRunner<M>(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.hpp:113](../../../../messaging/model_runner.hpp#L113)*

### operator=

*public ModelRunner<M> & operator=(const ModelRunner<M> & )*

*Defined at [messaging/model_runner.hpp:114](../../../../messaging/model_runner.hpp#L114)*

### ModelRunner<M>

*public void ModelRunner<M>(ModelRunner<M> && )*

*Defined at [messaging/model_runner.hpp:115](../../../../messaging/model_runner.hpp#L115)*

### operator=

*public ModelRunner<M> & operator=(ModelRunner<M> && )*

*Defined at [messaging/model_runner.hpp:116](../../../../messaging/model_runner.hpp#L116)*

### ~ModelRunner<M>

*public void ~ModelRunner<M>()*

*Defined at [messaging/model_runner.hpp:118](../../../../messaging/model_runner.hpp#L118)*

### turn

*public std::uint32_t turn()*

*Defined at [messaging/model_runner.hpp:135](../../../../messaging/model_runner.hpp#L135)*



**brief** Inline only: one turn of the model, on the UI thread. The telegrams it handled; 0 once it has quit.

### threaded

*public bool threaded()*

*Defined at [messaging/model_runner.hpp:150](../../../../messaging/model_runner.hpp#L150)*



**brief** Whether the model runs on its own thread. 



