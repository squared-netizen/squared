# class Sprite

*Defined at [graphics/sprite.cppm:12](../../../../graphics/sprite.cppm#L12)*



**brief** Lightweight mutable state for drawing one TextureRegion.



## Functions

### Sprite

*public void Sprite(const TextureRegion & region)*

*Defined at [graphics/sprite.cppm:20](../../../../graphics/sprite.cppm#L20)*



**brief** Construct a sprite whose size matches the region.

**region** Region referenced non-owningly; must outlive the sprite. Initial width and height are set from the region.

### color

*public sq::graphics::Color color()*

*Defined at [graphics/sprite.cppm:146](../../../../graphics/sprite.cppm#L146)*



**brief** Read the current tint color.

**return** Color used for tinting the sampled texture.

### height

*public float height()*

*Defined at [graphics/sprite.cppm:110](../../../../graphics/sprite.cppm#L110)*



**brief** Read the unscaled height.

**return** Unscaled height in logical pixels.

### origin_x

*public float origin_x()*

*Defined at [graphics/sprite.cppm:116](../../../../graphics/sprite.cppm#L116)*



**brief** Read the horizontal transform origin.

**return** Origin offset in logical pixels.

### origin_y

*public float origin_y()*

*Defined at [graphics/sprite.cppm:122](../../../../graphics/sprite.cppm#L122)*



**brief** Read the vertical transform origin.

**return** Origin offset in logical pixels.

### region

*public const TextureRegion & region()*

*Defined at [graphics/sprite.cppm:86](../../../../graphics/sprite.cppm#L86)*



**brief** Access the referenced texture region.

**return** Reference to the region provided at construction.

### rotation

*public float rotation()*

*Defined at [graphics/sprite.cppm:140](../../../../graphics/sprite.cppm#L140)*



**brief** Read rotation in degrees.

**return** Clockwise rotation in degrees.

### scale_x

*public float scale_x()*

*Defined at [graphics/sprite.cppm:128](../../../../graphics/sprite.cppm#L128)*



**brief** Read the horizontal scale factor.

**return** Currently applied horizontal multiplier.

### scale_y

*public float scale_y()*

*Defined at [graphics/sprite.cppm:134](../../../../graphics/sprite.cppm#L134)*



**brief** Read the vertical scale factor.

**return** Currently applied vertical multiplier.

### set_color

*public void set_color(sq::graphics::Color color)*

*Defined at [graphics/sprite.cppm:80](../../../../graphics/sprite.cppm#L80)*



**brief** Set the color multiplied with the sampled texture.

**color** Tint color; white preserves the texture unchanged.

### set_origin

*public void set_origin(float x, float y)*

*Defined at [graphics/sprite.cppm:53](../../../../graphics/sprite.cppm#L53)*



**brief** Set the transform origin relative to the top-left corner.

**x** Origin offset in logical pixels.

**y** Origin offset in logical pixels.

### set_position

*public void set_position(float x, float y)*

*Defined at [graphics/sprite.cppm:31](../../../../graphics/sprite.cppm#L31)*



**brief** Set the top-left position in logical coordinates.

**x** Horizontal position.

**y** Vertical position.

### set_rotation

*public void set_rotation(float degrees)*

*Defined at [graphics/sprite.cppm:74](../../../../graphics/sprite.cppm#L74)*



**brief** Set clockwise rotation in top-left coordinates.

**degrees** Clockwise rotation in degrees.

### set_scale

*public void set_scale(float x, float y)*

*Defined at [graphics/sprite.cppm:64](../../../../graphics/sprite.cppm#L64)*



**brief** Set independent horizontal and vertical scale factors.

**x** Horizontal multiplier.

**y** Vertical multiplier.

### set_size

*public void set_size(float width, float height)*

*Defined at [graphics/sprite.cppm:42](../../../../graphics/sprite.cppm#L42)*



**brief** Set the unscaled logical dimensions.

**width** Unscaled width in logical pixels.

**height** Unscaled height in logical pixels.

### width

*public float width()*

*Defined at [graphics/sprite.cppm:104](../../../../graphics/sprite.cppm#L104)*



**brief** Read the unscaled width.

**return** Unscaled width in logical pixels.

### x

*public float x()*

*Defined at [graphics/sprite.cppm:92](../../../../graphics/sprite.cppm#L92)*



**brief** Read the horizontal position.

**return** Current x position in logical pixels.

### y

*public float y()*

*Defined at [graphics/sprite.cppm:98](../../../../graphics/sprite.cppm#L98)*



**brief** Read the vertical position.

**return** Current y position in logical pixels.

