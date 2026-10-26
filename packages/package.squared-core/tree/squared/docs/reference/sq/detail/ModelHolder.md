# class ModelHolder

*Defined at [stage/app.hpp:21](../../../../stage/app.hpp#L21)*



**brief** A model's runner, whatever the model's type: what `App::run` keeps. 



## Functions

### ModelHolder

*public void ModelHolder()*

*Defined at [stage/app.hpp:24](../../../../stage/app.hpp#L24)*

### ~ModelHolder

*public void ~ModelHolder()*

*Defined at [stage/app.hpp:25](../../../../stage/app.hpp#L25)*

### ModelHolder

*public void ModelHolder(const ModelHolder & )*

*Defined at [stage/app.hpp:26](../../../../stage/app.hpp#L26)*

### operator=

*public ModelHolder & operator=(const ModelHolder & )*

*Defined at [stage/app.hpp:27](../../../../stage/app.hpp#L27)*

### ModelHolder

*public void ModelHolder(ModelHolder && )*

*Defined at [stage/app.hpp:28](../../../../stage/app.hpp#L28)*

### operator=

*public ModelHolder & operator=(ModelHolder && )*

*Defined at [stage/app.hpp:29](../../../../stage/app.hpp#L29)*

### turn

*public std::uint32_t turn()*



**brief** A model on the UI thread: one turn. On its own thread: nothing. 



