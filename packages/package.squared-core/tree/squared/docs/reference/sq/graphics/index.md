# namespace graphics

## Records

* [AtlasRegion](AtlasRegion.md)
* [BitmapFontError](BitmapFontError.md)
* [BitmapFontInfo](BitmapFontInfo.md)
* [BitmapFontPage](BitmapFontPage.md)
* [BitmapFontParseLimits](BitmapFontParseLimits.md)
* [BitmapFont](BitmapFont.md)
* [BitmapGlyph](BitmapGlyph.md)
* [BitmapKerning](BitmapKerning.md)
* [Color](Color.md)
* [ContextConfig](ContextConfig.md)
* [Context](Context.md)
* [GlyphLayoutOptions](GlyphLayoutOptions.md)
* [GlyphLayout](GlyphLayout.md)
* [GlyphLine](GlyphLine.md)
* [GlyphPlacement](GlyphPlacement.md)
* [OrthographicCamera](OrthographicCamera.md)
* [SpriteBatch](SpriteBatch.md)
* [Sprite](Sprite.md)
* [TextureAtlas](TextureAtlas.md)
* [TextureRecoveryOptions](TextureRecoveryOptions.md)
* [TextureRecoveryTarget](TextureRecoveryTarget.md)
* [TextureRegion](TextureRegion.md)
* [Texture](Texture.md)


## Enums

| enum class BitmapFontErrorCode |

--

| none |
| invalid_argument |
| limit_exceeded |
| malformed |
| unsupported |
| duplicate |
| missing_required |
| invalid_utf8 |


*Defined at [graphics/src/bitmap_font_error_code.hpp:9](../../../../graphics/src/bitmap_font_error_code.hpp#L9)*



**brief** Stable failure categories for BMFont parsing and glyph layout. 

| enum class CoordinateOrigin |

--

| BottomLeft |
| TopLeft |


*Defined at [graphics/src/coordinate_origin.hpp:11](../../../../graphics/src/coordinate_origin.hpp#L11)*



**brief** Coordinate orientation used by an orthographic camera.

| enum class GlyphAlignment |

--

| start |
| center |
| end |


*Defined at [graphics/src/glyph_alignment.hpp:9](../../../../graphics/src/glyph_alignment.hpp#L9)*



**brief** Horizontal placement within an optional target width. 

| enum class TextureFilter |

--

| Nearest |
| Linear |


*Defined at [graphics/src/texture_filter.hpp:14](../../../../graphics/src/texture_filter.hpp#L14)*



**brief** How a texture is sampled between texels.

 The one filter vocabulary in squared: `graphics::Texture` and the lower-level `gles::Texture2D` both take it.

| enum class TextureRecoveryPolicy |

--

| ReloadFromAsset |
| RetainPixels |
| Regenerate |
| Discard |


*Defined at [graphics/src/texture_recovery_policy.hpp:15](../../../../graphics/src/texture_recovery_policy.hpp#L15)*



**brief** CPU-side recipe used to recover a texture after context loss.

 Policies are backend-neutral. They describe what source data Squared keeps or asks the application to regenerate; they do not expose the selected windowing or graphics API.

| enum class TextureWrap |

--

| ClampToEdge |
| Repeat |
| MirroredRepeat |


*Defined at [graphics/src/texture_wrap.hpp:14](../../../../graphics/src/texture_wrap.hpp#L14)*



**brief** How sampling outside the zero-to-one range behaves.

 The one wrap vocabulary in squared: `graphics::Texture` and the lower-level `gles::Texture2D` both take it.

