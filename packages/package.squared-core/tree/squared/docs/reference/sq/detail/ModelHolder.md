# class ModelHolder

*Defined at [stage/app.hpp:29](../../../../stage/app.hpp#L29)*



**brief** A model's runner, whatever the model's type: what `App::run` keeps. 



## Functions

### ModelHolder

*public void ModelHolder()*

*Defined at [stage/app.hpp:32](../../../../stage/app.hpp#L32)*

### ~ModelHolder

*public void ~ModelHolder()*

*Defined at [stage/app.hpp:33](../../../../stage/app.hpp#L33)*

### ModelHolder

*public void ModelHolder(const ModelHolder & )*

*Defined at [stage/app.hpp:34](../../../../stage/app.hpp#L34)*

### operator=

*public ModelHolder & operator=(const ModelHolder & )*

*Defined at [stage/app.hpp:35](../../../../stage/app.hpp#L35)*

### ModelHolder

*public void ModelHolder(ModelHolder && )*

*Defined at [stage/app.hpp:36](../../../../stage/app.hpp#L36)*

### operator=

*public ModelHolder & operator=(ModelHolder && )*

*Defined at [stage/app.hpp:37](../../../../stage/app.hpp#L37)*

### turn

*public std::uint32_t turn()*



**brief** A model on the UI thread: one turn. On its own thread: nothing. 



