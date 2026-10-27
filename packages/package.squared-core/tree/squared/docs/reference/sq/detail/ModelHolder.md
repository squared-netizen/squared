# class ModelHolder

*Defined at [stage/app.hpp:32](../../../../stage/app.hpp#L32)*



**brief** A model's runner, whatever the model's type: what `App::run` keeps. 



## Functions

### ModelHolder

*public void ModelHolder()*

*Defined at [stage/app.hpp:35](../../../../stage/app.hpp#L35)*

### ~ModelHolder

*public void ~ModelHolder()*

*Defined at [stage/app.hpp:36](../../../../stage/app.hpp#L36)*

### ModelHolder

*public void ModelHolder(const ModelHolder & )*

*Defined at [stage/app.hpp:37](../../../../stage/app.hpp#L37)*

### operator=

*public ModelHolder & operator=(const ModelHolder & )*

*Defined at [stage/app.hpp:38](../../../../stage/app.hpp#L38)*

### ModelHolder

*public void ModelHolder(ModelHolder && )*

*Defined at [stage/app.hpp:39](../../../../stage/app.hpp#L39)*

### operator=

*public ModelHolder & operator=(ModelHolder && )*

*Defined at [stage/app.hpp:40](../../../../stage/app.hpp#L40)*

### turn

*public std::uint32_t turn()*



**brief** A model on the UI thread: one turn. On its own thread: nothing. 



