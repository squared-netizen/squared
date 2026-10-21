# class ProgressBar

*Defined at [gui/progress_bar.hpp:15](../../../../gui/progress_bar.hpp#L15)*



**brief** Read-only determinate progress indicator. 



Inherits from Widget



## Functions

### ProgressBar

*public void ProgressBar(float minimum, float maximum, float value)*



**brief** Construct a progress bar.

**minimum** Value represented by an empty bar.

**maximum** Value represented by a full bar.

**value** Initial value; clamped into the ordered range.

### set_range

*public void set_range(float minimum, float maximum)*



**brief** Replace the value range.

**minimum** First range endpoint.

**maximum** Second range endpoint.

**post** Endpoints are ordered and the current value is clamped.

### set_value

*public void set_value(float value)*



**brief** Set and clamp the represented value. 

### progress

*public float progress()*



**brief** Return completion normalized to the closed range `[0, 1]`. 

### set_style

*public void set_style(std::string style)*



**brief** Select a named ProgressBarStyle from the active Skin. 

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the selected style's minimum horizontal extent. 

### preferred_size

*public Size preferred_size(Painter & , const Skin & )*



**brief** Report the selected style's preferred extent. 

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the complete track and completed portion. 

### value

*public float value()*

*Defined at [gui/progress_bar.hpp:38](../../../../gui/progress_bar.hpp#L38)*



**brief** Return the represented value. 



