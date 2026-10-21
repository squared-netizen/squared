# class TextureRecoveryTarget

*Defined at [graphics/src/texture_recovery_target.hpp:15](../../../../graphics/src/texture_recovery_target.hpp#L15)*



**brief** Synchronous destination supplied to a regeneration callback.

 Pixel memory only needs to remain valid for the duration of upload_rgba().



## Functions

### ~TextureRecoveryTarget

*public void ~TextureRecoveryTarget()*

*Defined at [graphics/src/texture_recovery_target.hpp:18](../../../../graphics/src/texture_recovery_target.hpp#L18)*

### TextureRecoveryTarget

*public void TextureRecoveryTarget(const TextureRecoveryTarget & )*

*Defined at [graphics/src/texture_recovery_target.hpp:19](../../../../graphics/src/texture_recovery_target.hpp#L19)*

### operator=

*public TextureRecoveryTarget & operator=(const TextureRecoveryTarget & )*

*Defined at [graphics/src/texture_recovery_target.hpp:20](../../../../graphics/src/texture_recovery_target.hpp#L20)*

### upload_rgba

*public bool upload_rgba(int width, int height, const std::uint8_t * pixels)*



**brief** Upload one tightly packed RGBA8888 image synchronously.

**width** Image width in pixels; must be greater than zero.

**height** Image height in pixels; must be greater than zero.

**pixels** Tightly packed RGBA8888 source; must be non-null and hold width * height * 4 bytes, valid for the duration of the call.

**return** true when the upload replaced the recovered texture.



