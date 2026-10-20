# class Sprite

*Defined at [graphics/sprite.hpp:12](../../../../graphics/sprite.hpp#L12)*



**brief** Lightweight mutable state for drawing one TextureRegion.



## Functions

### Sprite

*public void Sprite(const TextureRegion & region)*

*Defined at [graphics/sprite.hpp:20](../../../../graphics/sprite.hpp#L20)*



**brief** Construct a sprite whose size matches the region.

**region** Region referenced non-owningly; must outlive the sprite. Initial width and height are set from the region.

### set_position

*public void set_position(float x, float y)*

*Defined at [graphics/sprite.hpp:31](../../../../graphics/sprite.hpp#L31)*



**brief** Set the top-left position in logical coordinates.

**x** Horizontal position.

**y** Vertical position.

### set_size

*public void set_size(float width, float height)*

*Defined at [graphics/sprite.hpp:42](../../../../graphics/sprite.hpp#L42)*



**brief** Set the unscaled logical dimensions.

**width** Unscaled width in logical pixels.

**height** Unscaled height in logical pixels.

### set_origin

*public void set_origin(float x, float y)*

*Defined at [graphics/sprite.hpp:53](../../../../graphics/sprite.hpp#L53)*



**brief** Set the transform origin relative to the top-left corner.

**x** Origin offset in logical pixels.

**y** Origin offset in logical pixels.

### set_scale

*public void set_scale(float x, float y)*

*Defined at [graphics/sprite.hpp:64](../../../../graphics/sprite.hpp#L64)*



**brief** Set independent horizontal and vertical scale factors.

**x** Horizontal multiplier.

**y** Vertical multiplier.

### set_rotation

*public void set_rotation(float degrees)*

*Defined at [graphics/sprite.hpp:74](../../../../graphics/sprite.hpp#L74)*



**brief** Set clockwise rotation in top-left coordinates.

**degrees** Clockwise rotation in degrees.

### set_color

*public void set_color(sq::graphics::Color color)*

*Defined at [graphics/sprite.hpp:80](../../../../graphics/sprite.hpp#L80)*



**brief** Set the color multiplied with the sampled texture.

**color** Tint color; white preserves the texture unchanged.

### region

*public const TextureRegion & region()*

*Defined at [graphics/sprite.hpp:86](../../../../graphics/sprite.hpp#L86)*



**brief** Access the referenced texture region.

**return** Reference to the region provided at construction.

### x

*public float x()*

*Defined at [graphics/sprite.hpp:92](../../../../graphics/sprite.hpp#L92)*



**brief** Read the horizontal position.

**return** Current x position in logical pixels.

### y

*public float y()*

*Defined at [graphics/sprite.hpp:98](../../../../graphics/sprite.hpp#L98)*



**brief** Read the vertical position.

**return** Current y position in logical pixels.

### width

*public float width()*

*Defined at [graphics/sprite.hpp:104](../../../../graphics/sprite.hpp#L104)*



**brief** Read the unscaled width.

**return** Unscaled width in logical pixels.

### height

*public float height()*

*Defined at [graphics/sprite.hpp:110](../../../../graphics/sprite.hpp#L110)*



**brief** Read the unscaled height.

**return** Unscaled height in logical pixels.

### origin_x

*public float origin_x()*

*Defined at [graphics/sprite.hpp:116](../../../../graphics/sprite.hpp#L116)*



**brief** Read the horizontal transform origin.

**return** Origin offset in logical pixels.

### origin_y

*public float origin_y()*

*Defined at [graphics/sprite.hpp:122](../../../../graphics/sprite.hpp#L122)*



**brief** Read the vertical transform origin.

**return** Origin offset in logical pixels.

### scale_x

*public float scale_x()*

*Defined at [graphics/sprite.hpp:128](../../../../graphics/sprite.hpp#L128)*



**brief** Read the horizontal scale factor.

**return** Currently applied horizontal multiplier.

### scale_y

*public float scale_y()*

*Defined at [graphics/sprite.hpp:134](../../../../graphics/sprite.hpp#L134)*



**brief** Read the vertical scale factor.

**return** Currently applied vertical multiplier.

### rotation

*public float rotation()*

*Defined at [graphics/sprite.hpp:140](../../../../graphics/sprite.hpp#L140)*



**brief** Read rotation in degrees.

**return** Clockwise rotation in degrees.

### color

*public sq::graphics::Color color()*

*Defined at [graphics/sprite.hpp:146](../../../../graphics/sprite.hpp#L146)*



**brief** Read the current tint color.

**return** Color used for tinting the sampled texture.



