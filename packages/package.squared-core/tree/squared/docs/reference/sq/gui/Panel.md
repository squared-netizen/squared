# class Panel

*Defined at [gui/panel.hpp:14](../../../../gui/panel.hpp#L14)*



**brief** Container painting a single style background. 



Inherits from Widget



## Functions

### Panel

*public void Panel(int style)*



**brief** Construct a panel.

**style** Named panel style to apply.

### set_style

*public void set_style(int style)*



**brief** Change the applied panel style.

**style** Named panel style to apply.

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the panel background.

**note** Parameters match the Widget::paint signature: the active painter, the skin providing the panel style, and the stage-space origin.



