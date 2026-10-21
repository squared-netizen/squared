# class Skin

*Defined at [gui/skin.hpp:37](../../../../gui/skin.hpp#L37)*



**brief** Named, reusable skin resources and widget styles.

 The skin owns named drawables and style values. Styles are plain value types; drawables are shared via DrawablePtr. A skin is not thread-safe.



## Members

public Color surface

public Color control

public Color control_hover

public Color accent

public Color text

public Color muted_text

public Color border

public float padding

public float spacing

public float border_width

public float minimum_touch_size



## Functions

### Skin

*public void Skin()*



**brief** Construct a skin with default palette values and default styles. 

### add_drawable

*public void add_drawable(std::string name, DrawablePtr drawable)*



**brief** Register a named drawable.

**name** Lookup name; replaces any existing drawable with the name.

**drawable** Drawable to store; shared ownership.

### add_region_drawable

*public void add_region_drawable(std::string name, const graphics::TextureRegion & region, Insets insets)*



**brief** Register a named region drawable.

**name** Lookup name.

**region** Region referenced by the new drawable; must outlive the skin.

**insets** Content insets in logical units.

### add_nine_patch_drawable

*public void add_nine_patch_drawable(std::string name, const graphics::TextureRegion & region, NinePatchSplits splits, std::optional<Insets> content_insets)*



**brief** Register a named nine-patch drawable.

**name** Lookup name.

**region** Region referenced by the new drawable; must outlive the skin.

**splits** Corner scissor amounts in source pixels.

**content_insets** Content insets; derived from splits when empty.

### add_nine_patch_drawable

*public void add_nine_patch_drawable(std::string name, const graphics::AtlasRegion & region)*



**brief** Register a named nine-patch drawable from an atlas region.

**name** Lookup name.

**region** Atlas region supplying texture and splits; must outlive the skin.

### drawable

*public DrawablePtr drawable(std::string_view name)*



**brief** Look up a named drawable.

**name** Drawable name to look up.

**return** Shared drawable, or an empty pointer when the name is absent.

### add_font

*public void add_font(std::string name, FontPtr font)*



**brief** Register or replace a named immutable font resource.

**name** Lookup name; must be non-empty.

**font** Shared font resource; must be non-empty.

**throws**when name or font is empty.

### font

*public FontPtr font(std::string_view name)*



**brief** Look up a named font resource.

**name** Font name to look up.

**return** Shared font, or an empty pointer when absent.

### add_panel_style

*public void add_panel_style(std::string name, PanelStyle style)*



**brief** Register or replace a named panel style.

**name** Style name.

**style** Style value to store.

### add_label_style

*public void add_label_style(std::string name, LabelStyle style)*



**brief** Register or replace a named label style.

**name** Style name.

**style** Style value to store.

### add_button_style

*public void add_button_style(std::string name, ButtonStyle style)*



**brief** Register or replace a named button style.

**name** Style name.

**style** Style value to store.

### add_text_field_style

*public void add_text_field_style(std::string name, TextFieldStyle style)*



**brief** Register or replace a named text-field style.

**name** Style name.

**style** Style value to store.

### add_check_box_style

*public void add_check_box_style(std::string name, CheckBoxStyle style)*



**brief** Register or replace a named check-box style.

**name** Style name.

**style** Style value to store.

### add_slider_style

*public void add_slider_style(std::string name, SliderStyle style)*



**brief** Register or replace a named slider style.

**name** Style name.

**style** Style value to store.

### add_progress_bar_style

*public void add_progress_bar_style(std::string name, ProgressBarStyle style)*



**brief** Register or replace a named progress-bar style.

**name** Style name.

**style** Style value to store.

### add_window_style

*public void add_window_style(std::string name, WindowStyle style)*



**brief** Register or replace a named window style.

**name** Style name.

**style** Style value to store.

### panel_style

*public const PanelStyle & panel_style(std::string_view name)*



**brief** Look up a panel style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### label_style

*public const LabelStyle & label_style(std::string_view name)*



**brief** Look up a label style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### button_style

*public const ButtonStyle & button_style(std::string_view name)*



**brief** Look up a button style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### text_field_style

*public const TextFieldStyle & text_field_style(std::string_view name)*



**brief** Look up a text-field style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### check_box_style

*public const CheckBoxStyle & check_box_style(std::string_view name)*



**brief** Look up a check-box style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### slider_style

*public const SliderStyle & slider_style(std::string_view name)*



**brief** Look up a slider style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### progress_bar_style

*public const ProgressBarStyle & progress_bar_style(std::string_view name)*



**brief** Look up a progress-bar style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.

### window_style

*public const WindowStyle & window_style(std::string_view name)*



**brief** Look up a window style.

**name** Style name to look up.

**return** Named style, or `default` when the name is absent.

**throws**only when `default` is also absent.



