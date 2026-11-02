# struct TextureRecoveryOptions

*Defined at [graphics/src/texture_recovery_options.cppm:10](../../../../graphics/src/texture_recovery_options.cppm#L10)*



**brief** Recovery selection supplied when a texture is created. 



## Members

public TextureRecoveryPolicy policy

public TextureRecoveryCallback callback

public void * user_data



## Functions

### discard

*public static TextureRecoveryOptions discard()*

*Defined at [graphics/src/texture_recovery_options.cppm:69](../../../../graphics/src/texture_recovery_options.cppm#L69)*



**brief** Build options that discard the texture after context loss.

**return** Options with the Discard policy.

### regenerate

*public static TextureRecoveryOptions regenerate(TextureRecoveryCallback callback, void * user_data)*

*Defined at [graphics/src/texture_recovery_options.cppm:59](../../../../graphics/src/texture_recovery_options.cppm#L59)*



**brief** Build options that regenerate pixels through a callback.

**callback** Callback restoring pixel content; must remain valid for the life of the texture. Non-owning.

**user_data** Opaque value forwarded to callback; non-owning.

**return** Options with the Regenerate policy.

### reload_from_asset

*public static TextureRecoveryOptions reload_from_asset()*

*Defined at [graphics/src/texture_recovery_options.cppm:38](../../../../graphics/src/texture_recovery_options.cppm#L38)*



**brief** Build options that reload from the asset path.

**return** Options with the ReloadFromAsset policy.

### retain_pixels

*public static TextureRecoveryOptions retain_pixels()*

*Defined at [graphics/src/texture_recovery_options.cppm:47](../../../../graphics/src/texture_recovery_options.cppm#L47)*



**brief** Build options that retain CPU pixels for GPU re-upload.

**return** Options with the RetainPixels policy.

