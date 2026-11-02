# class ProgressBar

*Defined at [gui/progress_bar.cppm:14](../../../../gui/progress_bar.cppm#L14)*



**brief** Read-only determinate progress indicator. 



Inherits from Widget



## Functions

### ProgressBar

*public void ProgressBar(float minimum, float maximum, float value)*



**brief** Construct a progress bar.

**minimum** Value represented by an empty bar.

**maximum** Value represented by a full bar.

**value** Initial value; clamped into the ordered range.

### minimum_size

*public Size minimum_size(Painter & , const Skin & )*



**brief** Report the selected style's minimum horizontal extent. 

### paint

*public void paint(Painter & , const Skin & , float , float )*



**brief** Paint the complete track and completed portion. 

### preferred_size

*public Size preferred_size(Painter & , const Skin & )*



**brief** Report the selected style's preferred extent. 

### progress

*public float progress()*



**brief** Return completion normalized to the closed range `[0, 1]`. 

### set_range

*public void set_range(float minimum, float maximum)*



**brief** Replace the value range.

**minimum** First range endpoint.

**maximum** Second range endpoint.

**post** Endpoints are ordered and the current value is clamped.

### set_style

*public void set_style(std::string style)*



**brief** Select a named ProgressBarStyle from the active Skin. 

### set_value

*public void set_value(float value)*



**brief** Set and clamp the represented value. 

### value

*public float value()*

*Defined at [gui/progress_bar.cppm:37](../../../../gui/progress_bar.cppm#L37)*



**brief** Return the represented value. 

