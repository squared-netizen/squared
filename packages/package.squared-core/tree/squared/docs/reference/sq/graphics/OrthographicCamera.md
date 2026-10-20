# class OrthographicCamera

*Defined at [graphics/orthographic_camera.hpp:13](../../../../graphics/orthographic_camera.hpp#L13)*



**brief** Two-dimensional orthographic camera with pan and zoom.



## Functions

### OrthographicCamera

*public void OrthographicCamera(float viewport_width, float viewport_height, CoordinateOrigin origin)*



**brief** Construct a camera centered in its logical viewport.

**viewport_width** Logical viewport width in the same units as rendered coordinates; must be greater than zero for a valid projection.

**viewport_height** Logical viewport height; must be greater than zero for a valid projection.

**origin** Coordinate orientation of the viewport.

### set_viewport

*public void set_viewport(float width, float height)*



**brief** Set the logical viewport dimensions.

**width** Logical viewport width; must be greater than zero for a valid projection.

**height** Logical viewport height; must be greater than zero for a valid projection.

**note** Call update() afterward to recalculate the projection matrix.

### set_position

*public void set_position(float x, float y)*



**brief** Set the camera center in logical coordinates.

**x** Camera center horizontal position.

**y** Camera center vertical position.

### set_zoom

*public void set_zoom(float zoom)*



**brief** Set the zoom factor; values below a safe minimum are clamped.

**zoom** Magnification factor; clamped at a safe positive minimum.

### update

*public void update()*



**brief** Recalculate the projection matrix after property changes. 

### combined

*public const sq::math::Matrix4 & combined()*



**brief** Read the current projection matrix.

**return** Matrix combining view and projection transforms.

### origin

*public CoordinateOrigin origin()*

*Defined at [graphics/orthographic_camera.hpp:78](../../../../graphics/orthographic_camera.hpp#L78)*



**brief** Read which way y points in this camera's logical space.

**return** TopLeft when y grows downwards, BottomLeft when it grows upwards.

 SpriteBatch reads this in begin() so an image's top edge lands at the top of its quad under either orientation.

### position

*public sq::math::Vector2 position()*



**brief** Read the camera center.

**return** Center position in logical coordinates.

### zoom

*public float zoom()*



**brief** Read the current zoom factor.

**return** Active magnification factor.



