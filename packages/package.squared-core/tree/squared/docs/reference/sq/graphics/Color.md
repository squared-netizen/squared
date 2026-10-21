# struct Color

*Defined at [graphics/color.hpp:12](../../../../graphics/color.hpp#L12)*



**brief** Normalized four-component color used by Squared graphics APIs.



## Members

public float red

public float green

public float blue

public float alpha



## Functions

### white

*public static Color white()*

*Defined at [graphics/color.hpp:27](../../../../graphics/color.hpp#L27)*



**brief** Construct opaque white.

**return** A color with every component at one.

### transparent

*public static Color transparent()*

*Defined at [graphics/color.hpp:33](../../../../graphics/color.hpp#L33)*



**brief** Construct fully transparent black.

**return** A color with every component at zero.

### from_rgba8

*public static Color from_rgba8(std::uint8_t red_value, std::uint8_t green_value, std::uint8_t blue_value, std::uint8_t alpha_value)*

*Defined at [graphics/color.hpp:43](../../../../graphics/color.hpp#L43)*



**brief** Convert byte components to normalized floating-point values.

**red_value** Red byte value in the inclusive range [0, 255].

**green_value** Green byte value in the inclusive range [0, 255].

**blue_value** Blue byte value in the inclusive range [0, 255].

**alpha_value** Alpha byte value in the inclusive range [0, 255].

**return** A color with each component divided by 255.

### clamped

*public Color clamped()*

*Defined at [graphics/color.hpp:56](../../../../graphics/color.hpp#L56)*



**brief** Return a copy with every component clamped to `[0, 1]`.

**return** A color whose components never exceed the normalized range.



