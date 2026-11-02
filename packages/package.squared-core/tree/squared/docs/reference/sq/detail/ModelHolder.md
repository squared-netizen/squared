# class ModelHolder

*Defined at [stage/app.cppm:18](../../../../stage/app.cppm#L18)*



**brief** A model's runner, whatever the model's type: what `App::run` keeps. 



## Functions

### ModelHolder

*public void ModelHolder()*

*Defined at [stage/app.cppm:21](../../../../stage/app.cppm#L21)*

### ModelHolder

*public void ModelHolder(ModelHolder && )*

*Defined at [stage/app.cppm:25](../../../../stage/app.cppm#L25)*

### ModelHolder

*public void ModelHolder(const ModelHolder & )*

*Defined at [stage/app.cppm:23](../../../../stage/app.cppm#L23)*

### operator=

*public ModelHolder & operator=(ModelHolder && )*

*Defined at [stage/app.cppm:26](../../../../stage/app.cppm#L26)*

### operator=

*public ModelHolder & operator=(const ModelHolder & )*

*Defined at [stage/app.cppm:24](../../../../stage/app.cppm#L24)*

### turn

*public std::uint32_t turn()*



**brief** A model on the UI thread: one turn. On its own thread: nothing. 

### ~ModelHolder

*public void ~ModelHolder()*

*Defined at [stage/app.cppm:22](../../../../stage/app.cppm#L22)*

