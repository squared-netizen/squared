#pragma once

#include <squared/application/event.hpp>
#include <squared/application/text_input.hpp>
#include <squared/graphics/color.hpp>
#include <squared/graphics2d/bitmap_font.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/scene2d/group.hpp>
#include <squared/scene2d/stage.hpp>

#include <cstddef>
#include <cstdint>
#include <array>
#include <deque>
#include <functional>
#include <limits>
#include <memory>
#include <optional>
#include <span>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace squared::gui {

/** @brief Two-dimensional extent in logical units. */
struct Size {
    /** @brief Horizontal extent in logical units. */
    float width{0.0F};
    /** @brief Vertical extent in logical units. */
    float height{0.0F};
};

/** @brief Axis-aligned rectangle in logical units. */
struct Rectangle {
    /** @brief Left edge in logical units. */
    float x{0.0F};
    /** @brief Top edge in logical units. */
    float y{0.0F};
    /** @brief Width in logical units; non-negative. */
    float width{0.0F};
    /** @brief Height in logical units; non-negative. */
    float height{0.0F};
};

/** @brief Insets from the edges of a rectangle, in logical units. */
struct Insets {
    /** @brief Left inset from the left edge. */
    float left{0.0F};
    /** @brief Top inset from the top edge. */
    float top{0.0F};
    /** @brief Right inset from the right edge. */
    float right{0.0F};
    /** @brief Bottom inset from the bottom edge. */
    float bottom{0.0F};
};

/** @brief Proposed minimum, preferred, and maximum sizes for one widget. */
struct SizeHints {
    /** @brief Smallest acceptable extent in logical units. */
    Size minimum{};
    /** @brief Extent the widget occupies when unconstrained. */
    Size preferred{};
    /** @brief Largest acceptable extent; infinity means unconstrained. */
    Size maximum{
        std::numeric_limits<float>::infinity(),
        std::numeric_limits<float>::infinity()
    };
};

/**
 * @brief Immutable portable bitmap-font resource selected by GUI styles.
 *
 * A descriptor-only resource records the safe asset path imported from a
 * skin. A resolved resource additionally owns parsed BMFont metrics and
 * non-owning texture-page regions. The textures referenced by those regions
 * must outlive the resource. FontResource performs no I/O and has no HoloDisk
 * or rendering-backend dependency.
 */
class FontResource final {
public:
    /**
     * @brief Construct a descriptor-only resource.
     * @param descriptor_path Contained relative BMFont descriptor path.
     * @throws std::invalid_argument when the path is empty, absolute, contains
     * a backslash, or contains an empty, `.` or `..` component.
     */
    explicit FontResource(std::string descriptor_path);

    /**
     * @brief Construct a resolved bitmap-font resource.
     * @param descriptor_path Contained relative descriptor identity.
     * @param font Valid parsed BMFont value transferred into the resource.
     * @param pages Page regions in BMFont page-id order. Each region must be
     * at least as large as the descriptor page dimensions and its texture
     * must outlive this resource.
     * @param scale Uniform logical-unit scale; finite and greater than zero.
     * @throws std::invalid_argument when any invariant is not satisfied.
     */
    FontResource(
        std::string descriptor_path,
        graphics2d::BitmapFont font,
        std::vector<graphics2d::TextureRegion> pages,
        float scale = 1.0F
    );

    /** @brief Return the descriptor asset path owned by this resource. */
    [[nodiscard]] const std::string& descriptor_path() const noexcept
    {
        return descriptor_path_;
    }

    /** @brief Return true when parsed metrics and matching pages are present. */
    [[nodiscard]] bool resolved() const noexcept { return font_.has_value(); }

    /** @brief Return parsed metrics, or null for a descriptor-only resource. */
    [[nodiscard]] const graphics2d::BitmapFont* bitmap_font() const noexcept
    {
        return font_ ? &*font_ : nullptr;
    }

    /** @brief Return page regions in BMFont page-id order. */
    [[nodiscard]] std::span<const graphics2d::TextureRegion> pages() const noexcept
    {
        return pages_;
    }

    /**
     * @brief Derive the texture view for one placement from this font.
     * @param glyph Placement produced from this resource's BitmapFont.
     * @return Non-owning page subregion, or an empty region when unresolved
     * or when the placement does not fit its declared page.
     */
    [[nodiscard]] graphics2d::TextureRegion glyph_region(
        const graphics2d::GlyphPlacement& glyph
    ) const noexcept;

    /** @brief Return the uniform conversion from source pixels to GUI units. */
    [[nodiscard]] float scale() const noexcept { return scale_; }

private:
    std::string descriptor_path_;
    std::optional<graphics2d::BitmapFont> font_;
    std::vector<graphics2d::TextureRegion> pages_;
    float scale_{1.0F};
};

/** @brief Immutable shared ownership of one GUI font resource. */
using FontPtr = std::shared_ptr<const FontResource>;

/**
 * @brief Portable drawing boundary implemented over Squared Graphics2D.
 *
 * All coordinate systems are logical pixels in the same space used for widget
 * placement. The painter is owned by the UI framework and is only valid within
 * the measure/layout/paint calls that pass it.
 */
class Painter {
public:
    virtual ~Painter() = default;

    /**
     * @brief Measure rendered text.
     * @param text UTF-8 text to measure.
     * @return Extent the text occupies when drawn with the current font.
     */
    [[nodiscard]] virtual Size measure_text(std::string_view text) = 0;

    /**
     * @brief Measure text using a style-selected font when it is resolved.
     * @param text UTF-8 text to measure.
     * @param font Borrowed font resource, or null for the painter default.
     * @return Extent in logical units. The base implementation uses
     * GlyphLayout for resolved resources and otherwise calls measure_text(text).
     */
    [[nodiscard]] virtual Size measure_text(
        std::string_view text,
        const FontResource* font
    );

    /**
     * @brief Fill a rectangle with a solid color.
     * @param rectangle Area to fill in logical units.
     * @param color Normalized fill color.
     */
    virtual void fill_rectangle(
        const Rectangle& rectangle,
        graphics::Color color
    ) = 0;

    /**
     * @brief Stroke a rectangle border.
     * @param rectangle Centerline of the border in logical units.
     * @param color Normalized stroke color.
     * @param thickness Border width in logical units; non-negative.
     */
    virtual void stroke_rectangle(
        const Rectangle& rectangle,
        graphics::Color color,
        float thickness
    ) = 0;

    /**
     * @brief Draw a texture region scaled to a rectangle.
     * @param region Region to sample; must outlive the call.
     * @param rectangle Target rectangle in logical units.
     * @param color Tint multiplied with the sampled texture.
     */
    virtual void draw_region(
        const graphics2d::TextureRegion& region,
        const Rectangle& rectangle,
        graphics::Color color = graphics::Color::white()
    ) = 0;

    /**
     * @brief Draw single-line text.
     * @param text UTF-8 text to render.
     * @param x Left baseline position in logical units.
     * @param y Top baseline position in logical units.
     * @param color Normalized text color.
     */
    virtual void draw_text(
        std::string_view text,
        float x,
        float y,
        graphics::Color color
    ) = 0;

    /**
     * @brief Draw single-line text with a style-selected font.
     * @param text UTF-8 text to render.
     * @param x Left position in logical units.
     * @param y Top position in logical units.
     * @param font Borrowed font resource, or null for the painter default.
     * @param color Normalized text color.
     * @note The base implementation delegates to the font-agnostic overload;
     * renderers supporting bitmap pages override this method.
     */
    virtual void draw_text(
        std::string_view text,
        float x,
        float y,
        const FontResource* font,
        graphics::Color color
    );

    /**
     * @brief Push a clipping rectangle.
     * @param rectangle Clip region in logical units; nested clips intersect.
     */
    virtual void push_clip(const Rectangle& rectangle) = 0;

    /** @brief Pop the most recently pushed clipping rectangle. */
    virtual void pop_clip() = 0;
};

/**
 * @brief Skin image abstraction; it never exposes SDL or another backend type.
 *
 * Drawables are immutable and shared through DrawablePtr. Any referenced
 * Texture or TextureAtlas must outlive every use of the drawable.
 */
class Drawable {
public:
    virtual ~Drawable() = default;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] virtual Size minimum_size() const noexcept = 0;

    /**
     * @brief Report the region reserved for content.
     * @return Insets between the outer box and drawable content.
     */
    [[nodiscard]] virtual Insets content_insets() const noexcept = 0;

    /**
     * @brief Draw the image into a rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the drawable's own coloring.
     */
    virtual void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const = 0;
};

/**
 * @brief Shared ownership of an immutable drawable.
 */
using DrawablePtr = std::shared_ptr<const Drawable>;

/**
 * @brief Drawable that paints a flat, optionally inset color.
 */
class ColorDrawable final : public Drawable {
public:
    /**
     * @brief Construct a flat color drawable.
     * @param color Normalized fill color.
     * @param minimum Smallest acceptable size in logical units.
     * @param insets Content insets in logical units.
     */
    explicit ColorDrawable(
        graphics::Color color,
        Size minimum = {},
        Insets insets = {}
    ) noexcept;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Insets between the outer box and the flat color.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Paint the flat color into the rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the stored fill color.
     */
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    graphics::Color color_;
    Size minimum_;
    Insets insets_;
};

/**
 * @brief Drawable that samples one texture region.
 *
 * The referenced region must outlive the drawable.
 */
class RegionDrawable final : public Drawable {
public:
    /**
     * @brief Construct a texture-region drawable.
     * @param region Region to sample; must outlive the drawable.
     * @param insets Content insets in logical units.
     */
    explicit RegionDrawable(
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    ) noexcept;

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent set from the region's pixel size.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Insets between the outer box and the region content.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Draw the region stretched to the rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the sampled texture.
     */
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    const graphics2d::TextureRegion* region_;
    Insets insets_;
};

/** @brief Scissor amounts used by nine-patch scaling. */
struct NinePatchSplits {
    /** @brief Left corner width in source pixels. */
    int left{0};
    /** @brief Top corner height in source pixels. */
    int top{0};
    /** @brief Right corner width in source pixels. */
    int right{0};
    /** @brief Bottom corner height in source pixels. */
    int bottom{0};
};

/**
 * @brief Scalable drawable that preserves corners and stretches edges and center.
 */
class NinePatchDrawable final : public Drawable {
public:
    /**
     * @brief Construct a nine-patch drawable.
     * @param region Region to sample; must outlive the drawable.
     * @param splits Corner scissor amounts in source pixels.
     * @param content_insets Content insets; defaults to a nine-patch inset
     * derived from the splits.
     */
    NinePatchDrawable(
        const graphics2d::TextureRegion& region,
        NinePatchSplits splits,
        std::optional<Insets> content_insets = std::nullopt
    );

    /**
     * @brief Report the smallest acceptable size.
     * @return Minimum extent that preserves the corner pieces.
     */
    [[nodiscard]] Size minimum_size() const noexcept override;

    /**
     * @brief Report the content insets.
     * @return Effective insets bounding the stretchable content region.
     */
    [[nodiscard]] Insets content_insets() const noexcept override;

    /**
     * @brief Draw the nine scalable pieces into the rectangle.
     * @param painter Destination painter; valid for the call.
     * @param rectangle Target rectangle in logical units.
     * @param tint Color multiplied with the sampled texture.
     */
    void draw(
        Painter& painter,
        const Rectangle& rectangle,
        graphics::Color tint = graphics::Color::white()
    ) const override;

private:
    std::array<graphics2d::TextureRegion, 9> regions_{};
    NinePatchSplits splits_{};
    Insets insets_{};
};

/** @brief Style data for a Panel widget. */
struct PanelStyle {
    /** @brief Background drawable; may be empty for no background. */
    DrawablePtr background;
};

/** @brief Style data for a Label widget. */
struct LabelStyle {
    /** @brief Font resource, or empty to use the painter default. */
    FontPtr font;
    /** @brief Primary label color. */
    std::optional<graphics::Color> text;
    /** @brief Muted label color. */
    std::optional<graphics::Color> muted_text;
};

/** @brief Style data for a Button widget. */
struct ButtonStyle {
    /** @brief Drawable shown in the normal state. */
    DrawablePtr normal;
    /** @brief Drawable shown while the pointer hovers. */
    DrawablePtr hovered;
    /** @brief Drawable shown while the button is pressed. */
    DrawablePtr pressed;
    /** @brief Drawable shown when the button is disabled. */
    DrawablePtr disabled;
    /** @brief Normal label color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Label color when the button is disabled. */
    graphics::Color disabled_text{graphics::Color::from_rgba8(174, 181, 194)};
    /** @brief Minimum button height in logical units. */
    float minimum_height{44.0F};
    /** @brief Horizontal label padding in logical units. */
    float horizontal_padding{12.0F};
    /** @brief Label font, or empty to use the painter default. */
    FontPtr font;
    /** @brief Square drawable/glyph slot size in logical units. */
    float icon_size{20.0F};
    /** @brief Gap between an icon and non-empty label in logical units. */
    float icon_spacing{8.0F};
};

/** @brief Style data for a TextField widget. */
struct TextFieldStyle {
    /** @brief Drawable shown when unfocused. */
    DrawablePtr normal;
    /** @brief Drawable shown while focused. */
    DrawablePtr focused;
    /** @brief Text color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Cursor color. */
    graphics::Color cursor{graphics::Color::from_rgba8(79, 137, 255)};
    /** @brief Minimum field height in logical units. */
    float minimum_height{44.0F};
    /** @brief Horizontal text padding in logical units. */
    float horizontal_padding{10.0F};
    /** @brief Text font, or empty to use the painter default. */
    FontPtr font;
};

/** @brief Style data for a CheckBox widget. */
struct CheckBoxStyle {
    /** @brief Drawable for the unchecked box. */
    DrawablePtr unchecked;
    /** @brief Drawable for the checked box. */
    DrawablePtr checked;
    /** @brief Drawable shown when the box is disabled. */
    DrawablePtr disabled;
    /** @brief Label color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Gap between the box and its label in logical units. */
    float spacing{8.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};
    /** @brief Label font, or empty to use the painter default. */
    FontPtr font;
};

/** @brief Style data for a Slider widget. */
struct SliderStyle {
    /** @brief Drawable for the unfilled track portion. */
    DrawablePtr track;
    /** @brief Drawable for the filled track portion. */
    DrawablePtr filled_track;
    /** @brief Drawable for the slider knob. */
    DrawablePtr knob;
    /** @brief Minimum track length in logical units. */
    float minimum_length{120.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};
};

/** @brief Style data for a non-interactive ProgressBar widget. */
struct ProgressBarStyle {
    /** @brief Drawable for the complete progress track. */
    DrawablePtr track;
    /** @brief Drawable for the completed portion of the track. */
    DrawablePtr fill;
    /** @brief Preferred horizontal length in logical units. */
    float minimum_length{120.0F};
    /** @brief Preferred track thickness in logical units. */
    float thickness{12.0F};
};

/** @brief Style data for a one-axis ScrollBar widget. */
struct ScrollBarStyle {
    /** @brief Drawable for the complete track. */
    DrawablePtr track;
    /** @brief Drawable for the draggable thumb. */
    DrawablePtr knob;
    /** @brief Minimum cross-axis touch target in logical units. */
    float minimum_touch_size{44.0F};
    /** @brief Minimum thumb length in logical units. */
    float minimum_knob_length{24.0F};
};

/** @brief Style data for a text ListView widget. */
struct ListViewStyle {
    /** @brief Drawable behind the complete list. */
    DrawablePtr background;
    /** @brief Drawable behind selected rows. */
    DrawablePtr selection;
    /** @brief Text color for unselected rows. */
    graphics::Color unselected_text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Text color for selected rows. */
    graphics::Color selected_text{graphics::Color::white()};
    /** @brief Row font, or empty to use the painter default. */
    FontPtr font;
    /** @brief Minimum row height in logical units. */
    float row_height{44.0F};
    /** @brief Horizontal text padding in logical units. */
    float horizontal_padding{10.0F};
};

/** @brief Style data for a Window widget. */
struct WindowStyle {
    /** @brief Drawable for the window body. */
    DrawablePtr background;
    /** @brief Drawable for the title bar. */
    DrawablePtr title_background;
    /** @brief Drawable for the close button in the normal state. */
    DrawablePtr close_normal;
    /** @brief Drawable for the close button while hovered. */
    DrawablePtr close_hovered;
    /** @brief Drawable for the close button while pressed. */
    DrawablePtr close_pressed;
    /** @brief Title text color. */
    graphics::Color title_text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Close-button glyph color. */
    graphics::Color close_text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Overlay color used for modal windows. */
    graphics::Color modal_overlay{graphics::Color::from_rgba8(0, 0, 0, 140)};
    /** @brief Insets between the window border and its content. */
    Insets content_insets{8.0F, 8.0F, 8.0F, 8.0F};
    /** @brief Title bar height in logical units. */
    float title_height{36.0F};
    /** @brief Close-button size in logical units. */
    float close_size{28.0F};
    /** @brief Resize grab border width in logical units. */
    float resize_border{8.0F};
    /** @brief Title and close-glyph font, or empty for the painter default. */
    FontPtr title_font;
};

/**
 * @brief Named, reusable skin resources and widget styles.
 *
 * The skin owns named drawables and style values. Styles are plain value
 * types; drawables are shared via DrawablePtr. A skin is not thread-safe.
 */
class Skin {
public:
    /** @brief Construct a skin with default palette values and default styles. */
    Skin();

    /** @brief Base surface background color. */
    graphics::Color surface{graphics::Color::from_rgba8(28, 31, 38)};
    /** @brief Control background color. */
    graphics::Color control{graphics::Color::from_rgba8(52, 57, 68)};
    /** @brief Control color while hovered. */
    graphics::Color control_hover{graphics::Color::from_rgba8(66, 73, 87)};
    /** @brief Interactive accent/focus color. */
    graphics::Color accent{graphics::Color::from_rgba8(79, 137, 255)};
    /** @brief Primary text color. */
    graphics::Color text{graphics::Color::from_rgba8(238, 241, 247)};
    /** @brief Secondary/muted text color. */
    graphics::Color muted_text{graphics::Color::from_rgba8(174, 181, 194)};
    /** @brief Border color. */
    graphics::Color border{graphics::Color::from_rgba8(91, 99, 116)};
    /** @brief Default padding inside containers, in logical units. */
    float padding{8.0F};
    /** @brief Default spacing between children, in logical units. */
    float spacing{6.0F};
    /** @brief Default border thickness in logical units. */
    float border_width{1.0F};
    /** @brief Minimum touch target size in logical units. */
    float minimum_touch_size{44.0F};

    /**
     * @brief Register a named drawable.
     * @param name Lookup name; replaces any existing drawable with the name.
     * @param drawable Drawable to store; shared ownership.
     */
    void add_drawable(std::string name, DrawablePtr drawable);

    /**
     * @brief Register a named region drawable.
     * @param name Lookup name.
     * @param region Region referenced by the new drawable; must outlive the
     * skin.
     * @param insets Content insets in logical units.
     */
    void add_region_drawable(
        std::string name,
        const graphics2d::TextureRegion& region,
        Insets insets = {}
    );

    /**
     * @brief Register a named nine-patch drawable.
     * @param name Lookup name.
     * @param region Region referenced by the new drawable; must outlive the
     * skin.
     * @param splits Corner scissor amounts in source pixels.
     * @param content_insets Content insets; derived from splits when empty.
     */
    void add_nine_patch_drawable(
        std::string name,
        const graphics2d::TextureRegion& region,
        NinePatchSplits splits,
        std::optional<Insets> content_insets = std::nullopt
    );

    /**
     * @brief Register a named nine-patch drawable from an atlas region.
     * @param name Lookup name.
     * @param region Atlas region supplying texture and splits; must outlive
     * the skin.
     */
    void add_nine_patch_drawable(
        std::string name,
        const graphics2d::AtlasRegion& region
    );

    /**
     * @brief Look up a named drawable.
     * @param name Drawable name to look up.
     * @return Shared drawable, or an empty pointer when the name is absent.
     */
    [[nodiscard]] DrawablePtr drawable(std::string_view name) const noexcept;

    /**
     * @brief Register or replace a named immutable font resource.
     * @param name Lookup name; must be non-empty.
     * @param font Shared font resource; must be non-empty.
     * @throws std::invalid_argument when name or font is empty.
     */
    void add_font(std::string name, FontPtr font);

    /**
     * @brief Look up a named font resource.
     * @param name Font name to look up.
     * @return Shared font, or an empty pointer when absent.
     */
    [[nodiscard]] FontPtr font(std::string_view name) const noexcept;

    /**
     * @brief Register or replace a named panel style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_panel_style(std::string name, PanelStyle style);

    /**
     * @brief Register or replace a named label style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_label_style(std::string name, LabelStyle style);

    /**
     * @brief Register or replace a named button style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_button_style(std::string name, ButtonStyle style);

    /**
     * @brief Register or replace a named text-field style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_text_field_style(std::string name, TextFieldStyle style);

    /**
     * @brief Register or replace a named check-box style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_check_box_style(std::string name, CheckBoxStyle style);

    /**
     * @brief Register or replace a named slider style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_slider_style(std::string name, SliderStyle style);

    /**
     * @brief Register or replace a named progress-bar style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_progress_bar_style(std::string name, ProgressBarStyle style);

    /** @brief Register or replace a named scroll-bar style. */
    void add_scroll_bar_style(std::string name, ScrollBarStyle style);

    /** @brief Register or replace a named list-view style. */
    void add_list_view_style(std::string name, ListViewStyle style);

    /**
     * @brief Register or replace a named window style.
     * @param name Style name.
     * @param style Style value to store.
     */
    void add_window_style(std::string name, WindowStyle style);

    /**
     * @brief Look up a panel style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const PanelStyle& panel_style(std::string_view name) const;

    /**
     * @brief Look up a label style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const LabelStyle& label_style(std::string_view name) const;

    /**
     * @brief Look up a button style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const ButtonStyle& button_style(std::string_view name) const;

    /**
     * @brief Look up a text-field style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const TextFieldStyle& text_field_style(std::string_view name) const;

    /**
     * @brief Look up a check-box style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const CheckBoxStyle& check_box_style(std::string_view name) const;

    /**
     * @brief Look up a slider style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const SliderStyle& slider_style(std::string_view name) const;

    /**
     * @brief Look up a progress-bar style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const ProgressBarStyle& progress_bar_style(
        std::string_view name
    ) const;

    /** @brief Look up a scroll-bar style, falling back to `default`. */
    [[nodiscard]] const ScrollBarStyle& scroll_bar_style(
        std::string_view name
    ) const;

    /** @brief Look up a list-view style, falling back to `default`. */
    [[nodiscard]] const ListViewStyle& list_view_style(
        std::string_view name
    ) const;

    /**
     * @brief Look up a window style.
     * @param name Style name to look up.
     * @return Named style, or `default` when the name is absent.
     * @throws std::out_of_range only when `default` is also absent.
     */
    [[nodiscard]] const WindowStyle& window_style(std::string_view name) const;

private:
    std::unordered_map<std::string, DrawablePtr> drawables_;
    std::unordered_map<std::string, FontPtr> fonts_;
    std::unordered_map<std::string, PanelStyle> panel_styles_;
    std::unordered_map<std::string, LabelStyle> label_styles_;
    std::unordered_map<std::string, ButtonStyle> button_styles_;
    std::unordered_map<std::string, TextFieldStyle> text_field_styles_;
    std::unordered_map<std::string, CheckBoxStyle> check_box_styles_;
    std::unordered_map<std::string, SliderStyle> slider_styles_;
    std::unordered_map<std::string, ProgressBarStyle> progress_bar_styles_;
    std::unordered_map<std::string, ScrollBarStyle> scroll_bar_styles_;
    std::unordered_map<std::string, ListViewStyle> list_view_styles_;
    std::unordered_map<std::string, WindowStyle> window_styles_;
};

/** @brief Pointer action delivered to widgets. */
enum class PointerAction { move, down, up, cancel };

/** @brief Portable pointer event payload in widget-local logical units. */
struct PointerEvent {
    /** @brief Action being reported. */
    PointerAction action{PointerAction::move};

    /** @brief Stable identifier for the pointer/contact. */
    std::int64_t pointer_id{0};

    /** @brief Horizontal position in logical pixels. */
    float x{0.0F};

    /** @brief Vertical position in logical pixels. */
    float y{0.0F};

    /** @brief Button index; zero means no button or the primary button. */
    int button{0};
};

/** @brief Portable key name used by the GUI input boundary. */
enum class Key {
    left,
    right,
    up,
    down,
    home,
    end,
    backspace,
    delete_key,
    enter,
    space,
    tab,
    escape
};

/** @brief Modifier state reused from the Scene2D input contract. */
using KeyModifiers = scene2d::InputModifiers;

/**
 * @brief Base class for every GUI node. Widgets may own other widgets.
 *
 * Widget inherits the Scene2D composite ownership model: children owned via
 * Group. Widgets carry layout, painting, and input behavior.
 */
class Widget : public scene2d::Group {
public:
    /** @brief Factory creating one fresh tooltip widget subtree on demand. */
    using TooltipFactory = std::function<std::unique_ptr<Widget>()>;

    ~Widget() override = default;

    /**
     * @brief Report the smallest acceptable size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] virtual Size minimum_size(Painter& painter, const Skin& skin) const;

    /**
     * @brief Report the preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style, font, and spacing values.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] virtual Size preferred_size(
        Painter& painter,
        const Skin& skin
    ) const;

    /**
     * @brief Report the largest acceptable size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Maximum extent in logical units.
     */
    [[nodiscard]] virtual Size maximum_size(Painter& painter, const Skin& skin) const;

    /**
     * @brief Compute the full size-hint set.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     * @return Minimum, preferred, and maximum sizes.
     */
    [[nodiscard]] SizeHints size_hints(Painter& painter, const Skin& skin) const;

    /**
     * @brief Invalidate cached layout so it is recomputed next pass.
     */
    void invalidate_layout() noexcept;

    /**
     * @brief Recompute layout if currently invalid.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     */
    void validate_layout(Painter& painter, const Skin& skin);

    /**
     * @brief Check whether cached layout is current.
     * @return true while the layout is valid.
     */
    [[nodiscard]] bool layout_valid() const noexcept { return layout_valid_; }

    /**
     * @brief Position and size this widget, then lay out children.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing style and spacing values.
     */
    virtual void layout(Painter& painter, const Skin& skin);

    /**
     * @brief Paint this widget and, typically, its children.
     * @param painter Active painter; valid for the call.
     * @param skin Skin providing style resources.
     * @param stage_x Stage-space origin associated with this widget.
     * @param stage_y Stage-space origin associated with this widget.
     */
    virtual void paint(
        Painter& painter,
        const Skin& skin,
        float stage_x,
        float stage_y
    ) const;

    /**
     * @brief Handle a pointer event.
     * @param event Pointer payload in widget-local coordinates.
     * @return true when the event was handled and should stop propagating.
     */
    virtual bool pointer_event(const PointerEvent& event);

    /**
     * @brief Handle a key-down event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the key was handled.
     */
    virtual bool key_down(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Handle committed UTF-8 text input.
     * @param text Committed text to insert.
     * @return true when the text was handled.
     */
    virtual bool text_input(std::string_view text);

    /**
     * @brief Handle composition-edit updates.
     * @note Arguments are the current composition text, its start offset, and
     * its length, matching the virtual signature.
     * @return true when the composition was handled.
     */
    virtual bool text_editing(std::string_view, int, int);

    /**
     * @brief Notify the widget of a focus change.
     * @param focused true when the widget gained focus.
     */
    virtual void focus_changed(bool focused);

    /**
     * @brief Report whether this widget can take keyboard focus.
     * @return true when the widget is focusable.
     */
    [[nodiscard]] virtual bool focusable() const noexcept;

    /**
     * @brief Control the enabled state.
     * @param enabled false disables interaction and dims rendering.
     */
    void set_enabled(bool enabled) noexcept { enabled_ = enabled; }

    /**
     * @brief Read the enabled state.
     * @return true while the widget is enabled.
     */
    [[nodiscard]] bool enabled() const noexcept { return enabled_; }

    /**
     * @brief Attach a plain-text tooltip composed from standard GUI widgets.
     * @param text UTF-8 tooltip text. An empty string clears the tooltip.
     * @post A fresh Stack/Panel/MarginContainer/Label subtree is created each
     * time the tooltip is shown.
     */
    void set_tooltip(std::string text);

    /**
     * @brief Attach a custom tooltip widget factory.
     * @param factory Copyable callback returning a fresh unparented widget
     * subtree. An empty callback clears the tooltip.
     * @note The Ui temporarily owns each returned subtree while it is shown.
     */
    void set_tooltip_factory(TooltipFactory factory);

    /** @brief Remove the tooltip declaration from this widget. */
    void clear_tooltip() noexcept;

    /** @brief Return true when this widget declares tooltip content. */
    [[nodiscard]] bool has_tooltip() const noexcept
    {
        return static_cast<bool>(tooltip_factory_);
    }

private:
    friend class Ui;
    bool layout_valid_{false};
    bool enabled_{true};
    TooltipFactory tooltip_factory_;

protected:
    /**
     * @brief Bridge Scene2D dispatch into widget input handlers.
     * @param event Scene2D input event to adapt.
     */
    void input_event(scene2d::InputEvent& event) override;
};

/** @brief Timing and placement policy for transient Ui tooltips. */
struct TooltipConfig final {
    /** @brief Stationary pointer delay in seconds. */
    double hover_delay{0.5};
    /** @brief Primary-contact hold delay in seconds. */
    double long_press_delay{0.6};
    /** @brief Keyboard/controller focus delay in seconds. */
    double focus_delay{0.5};
    /** @brief Pointer travel cancelling a long press, in logical units. */
    float movement_tolerance{8.0F};
    /** @brief Minimum tooltip distance from viewport edges, in logical units. */
    float viewport_margin{8.0F};
    /** @brief Gap between focused owner and tooltip, in logical units. */
    float owner_gap{8.0F};
    /** @brief Offset from a hover/press pointer, in logical units. */
    float pointer_offset{14.0F};
};

/** @brief Read-only text widget. */
class Label final : public Widget {
public:
    /**
     * @brief Construct a label.
     * @param text Initial text content.
     */
    explicit Label(std::string text = {});

    /**
     * @brief Replace the label text.
     * @param text New text content.
     */
    void set_text(std::string text);

    /**
     * @brief Read the current text.
     * @return Reference to the stored text.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Select a named LabelStyle from the active Skin.
     * @param style Style name; an unknown name falls back to `default`.
     */
    void set_style(std::string style);

    /** @brief Return the selected label-style name. */
    [[nodiscard]] const std::string& style() const noexcept { return style_; }

    /**
     * @brief Control the muted visual style.
     * @param muted true renders with the muted text color.
     */
    void set_muted(bool muted) noexcept { muted_ = muted; }

    /**
     * @brief Report the measured text size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected label style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the label text.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing text colors, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string text_;
    std::string style_{"default"};
    bool muted_{false};
};

/** @brief Widget rendering one shared drawable. */
class Image final : public Widget {
public:
    /**
     * @brief Construct an image widget.
     * @param drawable Drawable to display, or empty for a blank image.
     */
    explicit Image(DrawablePtr drawable = {});

    /**
     * @brief Replace the displayed drawable.
     * @param drawable Drawable to display; shared ownership.
     */
    void set_drawable(DrawablePtr drawable);

    /**
     * @brief Report the drawable size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by image measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the drawable.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing style resources, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    DrawablePtr drawable_;
};

/** @brief Container painting a single style background. */
class Panel final : public Widget {
public:
    /**
     * @brief Construct a panel.
     * @param style Named panel style to apply.
     */
    explicit Panel(std::string style = "default");

    /**
     * @brief Change the applied panel style.
     * @param style Named panel style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Paint the panel background.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the panel style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string style_;
};

/** @brief Main stacking axis of a linear layout. */
enum class Direction { horizontal, vertical };

/** @brief Per-cell alignment relative to the cell box. */
enum class Alignment { start, center, end };

class Table;

/** @brief Per-child constraints returned by Table::add for fluent configuration. */
class Cell {
public:
    /**
     * @brief Span multiple grid columns.
     * @param columns Number of grid columns covered by this cell; must be at
     * least one.
     * @return This cell for chaining.
     */
    Cell& column_span(std::size_t columns) noexcept;

    /**
     * @brief Stretch equally with other growing cells on both axes.
     * @return This cell for chaining.
     */
    Cell& grow() noexcept;

    /**
     * @brief Stretch horizontally with other growing cells.
     * @return This cell for chaining.
     */
    Cell& grow_x() noexcept;

    /**
     * @brief Stretch vertically with other growing cells.
     * @return This cell for chaining.
     */
    Cell& grow_y() noexcept;

    /**
     * @brief Expand to fill available cell space.
     * @return This cell for chaining.
     */
    Cell& fill() noexcept;

    /**
     * @brief Expand horizontally to fill available cell width.
     * @return This cell for chaining.
     */
    Cell& fill_x() noexcept;

    /**
     * @brief Expand vertically to fill available cell height.
     * @return This cell for chaining.
     */
    Cell& fill_y() noexcept;

    /**
     * @brief Set uniform padding on all four sides.
     * @param value Padding in logical units; non-negative.
     * @return This cell for chaining.
     */
    Cell& pad(float value) noexcept;

    /**
     * @brief Set per-side padding.
     * @param value Insets in logical units; non-negative.
     * @return This cell for chaining.
     */
    Cell& pad(Insets value) noexcept;

    /**
     * @brief Set horizontal and vertical alignment within the cell.
     * @param horizontal start, center, or end alignment.
     * @param vertical start, center, or end alignment.
     * @return This cell for chaining.
     */
    Cell& align(Alignment horizontal, Alignment vertical = Alignment::center) noexcept;

    /**
     * @brief Access the child widget placed in this cell.
     * @return Reference to the cell's child widget.
     */
    [[nodiscard]] Widget& widget() noexcept { return *widget_; }

private:
    friend class Table;
    void changed() noexcept;
    Table* owner_{nullptr};
    Widget* widget_{nullptr};
    std::size_t row_{0};
    std::size_t column_{0};
    std::size_t column_span_{1};
    float grow_x_{0.0F};
    float grow_y_{0.0F};
    bool fill_x_{false};
    bool fill_y_{false};
    Insets padding_{};
    Alignment horizontal_{Alignment::center};
    Alignment vertical_{Alignment::center};
};

/**
 * @brief Grid layout with libGDX-style rows and chainable cell constraints.
 */
class Table final : public Widget {
public:
    /**
     * @brief Add a child widget to the current cell.
     * @param child Widget to adopt; ownership transfers to the table.
     * @return Configurable Cell describing the child's placement.
     */
    Cell& add(std::unique_ptr<Widget> child);

    /**
     * @brief Advance to the next grid row.
     * @return Reference to this table for chaining.
     */
    Table& row() noexcept;

    /**
     * @brief Set table padding.
     * @param padding Padding in logical units; non-negative.
     */
    void set_padding(float padding) noexcept;

    /**
     * @brief Set spacing between rows and columns.
     * @param spacing Gap in logical units; non-negative.
     */
    void set_spacing(float spacing) noexcept;

    /**
     * @brief Report the smallest size fitting every cell.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing style values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the preferred size from cell content.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every cell widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Compute cell rectangles and position every child.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    struct GridMetrics;
    [[nodiscard]] GridMetrics measure(Painter&, const Skin*, bool minimum) const;
    float padding_{-1.0F};
    float spacing_{-1.0F};
    std::size_t current_row_{0};
    std::size_t current_column_{0};
    std::deque<Cell> cells_;
};

/**
 * @brief Container stacking children along one axis with optional growth.
 */
class LinearLayout final : public Widget {
public:
    /**
     * @brief Construct a linear layout.
     * @param direction Stacking axis.
     */
    explicit LinearLayout(Direction direction = Direction::vertical) noexcept;

    /**
     * @brief Add a child widget.
     * @param child Widget to adopt; ownership transfers to the layout.
     * @param grow Extra stretch share for the child, in logical units.
     * @return Reference to the added child.
     */
    Widget& add(std::unique_ptr<Widget> child, float grow = 0.0F);

    /**
     * @brief Set layout padding.
     * @param padding Padding in logical units; non-negative.
     */
    void set_padding(float padding) noexcept;

    /**
     * @brief Set spacing between children.
     * @param spacing Gap in logical units; non-negative.
     */
    void set_spacing(float spacing) noexcept;

    /**
     * @brief Report the smallest size fitting all children.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing style values.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the preferred size from stacked content.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every child widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position children along the layout axis.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    struct Slot { Widget* widget{nullptr}; float grow{0.0F}; };
    [[nodiscard]] Size measured_size(Painter&, const Skin*, bool minimum) const;
    Direction direction_;
    float padding_{-1.0F};
    float spacing_{-1.0F};
    std::vector<Slot> slots_;
};

/** @brief Container overlaying children into the same box, later on top. */
class Stack final : public Widget {
public:
    /**
     * @brief Add a child widget to the stack.
     * @param child Widget to adopt; ownership transfers to the stack.
     * @return Reference to the added child.
     */
    Widget& add(std::unique_ptr<Widget> child);

    /**
     * @brief Report the largest child as preferred.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to every child widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Expand every child to the stack box.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;
};

/** @brief Container adding fixed insets around one child. */
class MarginContainer final : public Widget {
public:
    /**
     * @brief Construct a margin container.
     * @param margin Insets in logical units; non-negative.
     */
    explicit MarginContainer(Insets margin = {}) noexcept;

    /**
     * @brief Set the single wrapped child.
     * @param content Widget to adopt; ownership transfers to the container.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Report the content size plus margins.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to the content widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position the content inside the margin insets.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

private:
    Insets margin_;
    Widget* content_{nullptr};
};

/** @brief Container clipping and vertically scrolling a larger child. */
class ScrollPane final : public Widget {
public:
    /**
     * @brief Set the single scrollable child.
     * @param content Widget to adopt; ownership transfers to the pane.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Set the vertical scroll offset.
     * @param scroll_y Offset in logical units; clamped to valid range.
     */
    void set_scroll_y(float scroll_y) noexcept;

    /**
     * @brief Read the vertical scroll offset.
     * @return Current offset in logical units.
     */
    [[nodiscard]] float scroll_y() const noexcept { return scroll_y_; }

    /**
     * @brief Report the content size.
     * @param painter Active painter used for measurement.
     * @param skin Skin forwarded to the content widget.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Position the content and clamp the scroll offset.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing style values.
     */
    void layout(Painter&, const Skin&) override;

    /**
     * @brief Handle drag-to-scroll pointer events.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

private:
    void clamp_scroll() noexcept;
    Widget* content_{nullptr};
    float scroll_y_{0.0F};
    float last_pointer_y_{0.0F};
    std::optional<std::int64_t> drag_pointer_;
};

class ToggleButton;

/**
 * @brief Non-owning coordinator for toggle buttons and radio-style choices.
 *
 * A button may belong to at most one group. The group and every registered
 * button detach from each other during destruction, so either may be owned by
 * an ordinary Widget subtree without imposing a second ownership hierarchy.
 */
class ButtonGroup final {
public:
    /**
     * @brief Construct a checked-count policy.
     * @param minimum_checked Minimum checked buttons while members exist.
     * @param maximum_checked Maximum checked buttons; must not be smaller
     * than `minimum_checked`.
     * @throws std::invalid_argument when the limits are reversed.
     */
    explicit ButtonGroup(
        std::size_t minimum_checked = 0,
        std::size_t maximum_checked =
            std::numeric_limits<std::size_t>::max()
    );
    ~ButtonGroup();

    ButtonGroup(const ButtonGroup&) = delete;
    ButtonGroup& operator=(const ButtonGroup&) = delete;
    ButtonGroup(ButtonGroup&&) = delete;
    ButtonGroup& operator=(ButtonGroup&&) = delete;

    /**
     * @brief Register a non-owned toggle button.
     * @param button Button that must outlive the call; duplicate adds are no-ops.
     * @throws std::invalid_argument when the button already belongs to another
     * group.
     */
    void add(ToggleButton& button);

    /**
     * @brief Detach one registered button.
     * @param button Candidate member.
     * @return true when the button was a member.
     */
    bool remove(ToggleButton& button);

    /** @brief Detach every member without destroying any button. */
    void clear() noexcept;

    /**
     * @brief Replace the checked-count policy and rebalance current members.
     * @param minimum_checked Minimum checked buttons while members exist.
     * @param maximum_checked Maximum checked buttons.
     * @throws std::invalid_argument when the limits are reversed.
     */
    void set_limits(std::size_t minimum_checked, std::size_t maximum_checked);

    /** @brief Return the number of registered buttons. */
    [[nodiscard]] std::size_t size() const noexcept { return buttons_.size(); }

    /** @brief Return the number of currently checked members. */
    [[nodiscard]] std::size_t checked_count() const noexcept;

    /** @brief Return the first checked member, or null when none is checked. */
    [[nodiscard]] ToggleButton* checked_button() const noexcept;

private:
    friend class ToggleButton;
    bool request_state(ToggleButton& button, bool checked);
    void rebalance();

    std::vector<ToggleButton*> buttons_;
    std::size_t minimum_checked_{0};
    std::size_t maximum_checked_{std::numeric_limits<std::size_t>::max()};
};

/** @brief Clickable, focusable control with an optional label and icon. */
class Button : public Widget {
public:
    /** @brief Click action invoked when the button is activated. */
    using Callback = std::function<void()>;

    /**
     * @brief Construct a button.
     * @param text Optional label text.
     * @param callback Click action; copied into the button.
     */
    explicit Button(std::string text = {}, Callback callback = {});

    /**
     * @brief Replace the label text.
     * @param text New label text.
     */
    void set_text(std::string text);

    /**
     * @brief Read the label text.
     * @return Reference to the stored label.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Set the click action.
     * @param callback Action copied into the button; may be empty.
     */
    void set_on_click(Callback callback);

    /**
     * @brief Change the applied button style.
     * @param style Named button style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Display a drawable before the label.
     * @param drawable Shared immutable drawable; empty clears the icon.
     * @post Any previously configured glyph is cleared.
     */
    void set_icon(DrawablePtr drawable);

    /**
     * @brief Display one UTF-8 glyph before the label.
     * @param glyph UTF-8 glyph text; empty clears the icon.
     * @param font Optional immutable font, or empty for the button style font.
     * @post Any previously configured drawable icon is cleared.
     */
    void set_glyph(std::string glyph, FontPtr font = {});

    /** @brief Remove either kind of icon while preserving the label. */
    void clear_icon();

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the button style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the label-plus-padding size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected button style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the styled button background and label.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the button style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Track press, hover, and release onto the button.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Activate on Enter or Space.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key activated the button.
     */
    bool key_down(Key, KeyModifiers = {}) override;

    /**
     * @brief Track the focused state.
     * @note The parameter matches Widget::focus_changed: true when the widget
     * gained focus.
     */
    void focus_changed(bool) override;

    /**
     * @brief Report focusability.
     * @return true; buttons take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

protected:
    /**
     * @brief Perform the button's activation action.
     *
     * Base invokes the click callback. Subclasses override to toggle state.
     */
    virtual void activate();

    /**
     * @brief Report the selected state used for styling.
     * @return true when the button is rendered as selected.
     */
    [[nodiscard]] virtual bool selected() const noexcept;

    /**
     * @brief Read the pressed state.
     * @return true while the pointer presses the button.
     */
    [[nodiscard]] bool pressed() const noexcept { return pressed_; }

    /**
     * @brief Read the hovered state.
     * @return true while the pointer hovers the button.
     */
    [[nodiscard]] bool hovered() const noexcept { return hovered_; }

    /**
     * @brief Read the focused state.
     * @return true while the button owns keyboard focus.
     */
    [[nodiscard]] bool focused() const noexcept { return focused_; }

private:
    std::string text_;
    std::string style_{"default"};
    Callback callback_;
    DrawablePtr icon_drawable_;
    std::string glyph_;
    FontPtr glyph_font_;
    bool pressed_{false};
    bool hovered_{false};
    bool focused_{false};
};

/** @brief Two-state button that reports changes through a callback. */
class ToggleButton : public Button {
public:
    /** @brief Action invoked with the new checked state. */
    using ChangeCallback = std::function<void(bool)>;

    /**
     * @brief Construct a toggle button.
     * @param text Optional label text.
     * @param checked Initial checked state.
     */
    explicit ToggleButton(std::string text = {}, bool checked = false);
    ~ToggleButton() override;

    /**
     * @brief Set the checked state.
     * @param checked New boolean state; invokes the change callback.
     */
    void set_checked(bool checked);

    /**
     * @brief Read the checked state.
     * @return Current boolean state.
     */
    [[nodiscard]] bool checked() const noexcept { return checked_; }

    /**
     * @brief Set the change callback.
     * @param callback Action copied into the button; invoked with the new
     * state whenever it changes.
     */
    void set_on_change(ChangeCallback callback);

protected:
    /** @brief Toggle the checked state and invoke the change callback. */
    void activate() override;

    /**
     * @brief Report the checked state for selected styling.
     * @return `true` when the toggle button is checked.
     */
    [[nodiscard]] bool selected() const noexcept override;

private:
    friend class ButtonGroup;
    void apply_checked(bool checked);

    bool checked_{false};
    ChangeCallback change_callback_;
    ButtonGroup* group_{nullptr};
};

/** @brief Toggle button rendering a checkbox glyph and label. */
class CheckBox : public ToggleButton {
public:
    /**
     * @brief Construct a checkbox.
     * @param text Optional label text.
     * @param checked Initial checked state.
     */
    explicit CheckBox(std::string text = {}, bool checked = false);

    /**
     * @brief Change the applied check-box style.
     * @param style Named check-box style to apply.
     */
    void set_check_style(std::string style);

    /**
     * @brief Report the touch-target-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the check-box style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the box-plus-label size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected check-box style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the checkbox glyph and label.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the check-box style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    std::string check_style_{"default"};
};

/** @brief CheckBox-shaped choice intended for a one-of-many ButtonGroup. */
class RadioButton final : public CheckBox {
public:
    /**
     * @brief Construct a radio choice using the named `radio` check style.
     * @param text Optional label text.
     * @param checked Initial checked state before group registration.
     */
    explicit RadioButton(std::string text = {}, bool checked = false);
};

/**
 * @brief Single-line editable text input with cursor and composition support.
 */
class TextField final : public Widget {
public:
    /**
     * @brief Construct a text field.
     * @param text Initial text content.
     */
    explicit TextField(std::string text = {});

    /**
     * @brief Replace the field text and reset the cursor.
     * @param text New text content.
     */
    void set_text(std::string text);

    /**
     * @brief Read the current text.
     * @return Reference to the stored text.
     */
    [[nodiscard]] const std::string& text() const noexcept { return text_; }

    /**
     * @brief Read the cursor position.
     * @return Byte offset into the UTF-8 text, or text length at the end.
     */
    [[nodiscard]] std::size_t cursor() const noexcept { return cursor_; }

    /**
     * @brief Change the applied text-field style.
     * @param style Named text-field style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the text-field style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the text-plus-padding size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing the selected text-field style and font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the field background, text, cursor, and composition.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the text-field style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Place the cursor from a pointer event.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the field handled the event.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Move the cursor and edit the text with keys.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key was handled.
     */
    bool key_down(Key, KeyModifiers = {}) override;

    /**
     * @brief Insert committed UTF-8 text at the cursor.
     * @note The parameter matches Widget::text_input: committed text to insert.
     * @return true when the text was inserted.
     */
    bool text_input(std::string_view) override;

    /**
     * @brief Update the in-progress composition span.
     * @note Parameters match the Widget::text_editing signature: the current
     * composition text, its start offset, and its length.
     * @return true when the composition was handled.
     */
    bool text_editing(std::string_view, int, int) override;

    /**
     * @brief Track focus to drive the focused style and underline state.
     * @note The parameter matches Widget::focus_changed: true when the widget
     * gained focus.
     */
    void focus_changed(bool) override;

    /**
     * @brief Report focusability.
     * @return true; text fields take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

private:
    std::string text_;
    std::string style_{"default"};
    std::size_t cursor_{0};
    bool focused_{false};
    std::string composition_;
};

/** @brief Draggable one-axis value selector with an optional gradient fill. */
class Slider final : public Widget {
public:
    /** @brief Action invoked with each new slider value. */
    using ChangeCallback = std::function<void(float)>;

    /**
     * @brief Construct a slider.
     * @param minimum Smallest selectable value.
     * @param maximum Largest selectable value.
     * @param value Initial value; clamped into the range.
     */
    Slider(float minimum = 0.0F, float maximum = 1.0F, float value = 0.0F);

    /**
     * @brief Set the selectable range.
     * @param minimum Smallest selectable value.
     * @param maximum Largest selectable value; with minimum must be ordered.
     */
    void set_range(float minimum, float maximum) noexcept;

    /**
     * @brief Set the current value.
     * @param value New value; clamped into the range and snapped to the step.
     */
    void set_value(float value);

    /**
     * @brief Read the current value.
     * @return Current value in the configured range.
     */
    [[nodiscard]] float value() const noexcept { return value_; }

    /**
     * @brief Set the value granularity.
     * @param step Snap increment in value units; zero disables snapping.
     */
    void set_step(float step) noexcept;

    /**
     * @brief Set the change callback.
     * @param callback Action copied into the slider; invoked with each new
     * value.
     */
    void set_on_change(ChangeCallback callback);

    /**
     * @brief Change the applied slider style.
     * @param style Named slider style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Report the style-driven minimum size.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the slider style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the track-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by slider measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the track, gradient fill, and knob.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the slider style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Drag the knob to set the value.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

    /**
     * @brief Adjust the value with arrow keys.
     * @note Parameters match the Widget::key_down signature: a portable key
     * name and the modifier state at the time of the event.
     * @return true when the key adjusted the value.
     */
    bool key_down(Key, KeyModifiers = {}) override;

    /**
     * @brief Track the focused state.
     * @note The parameter matches Widget::focus_changed: true when the widget
     * gained focus.
     */
    void focus_changed(bool) override;

    /**
     * @brief Report focusability.
     * @return true; sliders take keyboard focus.
     */
    [[nodiscard]] bool focusable() const noexcept override;

private:
    void update_from_pointer(float x);
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    float step_{0.0F};
    std::string style_{"default"};
    ChangeCallback callback_;
    std::optional<std::int64_t> drag_pointer_;
    bool focused_{false};
};

/** @brief Draggable one-axis viewport-position control. */
class ScrollBar final : public Widget {
public:
    /** @brief Action invoked when the scroll value changes. */
    using ChangeCallback = std::function<void(float)>;

    /** @brief Construct a scroll bar for the requested axis. */
    explicit ScrollBar(Direction direction = Direction::vertical) noexcept;

    /** @brief Set ordered content-space limits and clamp the current value. */
    void set_range(float minimum, float maximum) noexcept;

    /** @brief Set the visible content extent used to size the thumb. */
    void set_page_size(float page_size) noexcept;

    /** @brief Set and clamp the current scroll value. */
    void set_value(float value);

    /** @brief Return the current scroll value. */
    [[nodiscard]] float value() const noexcept { return value_; }

    /** @brief Return the configured visible content extent. */
    [[nodiscard]] float page_size() const noexcept { return page_size_; }

    /** @brief Set arrow-key granularity; zero selects an automatic amount. */
    void set_step(float step) noexcept;

    /** @brief Install the change callback. */
    void set_on_change(ChangeCallback callback);

    /** @brief Select a named ScrollBarStyle. */
    void set_style(std::string style);

    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&, const Skin&) const override;
    void paint(Painter&, const Skin&, float, float) const override;
    bool pointer_event(const PointerEvent&) override;
    bool key_down(Key, KeyModifiers = {}) override;
    void focus_changed(bool) override;
    [[nodiscard]] bool focusable() const noexcept override;

private:
    [[nodiscard]] float axis_length() const noexcept;
    [[nodiscard]] float knob_length(const ScrollBarStyle&) const noexcept;
    void update_from_pointer(float coordinate, float grab_offset);

    Direction direction_{Direction::vertical};
    float minimum_{0.0F};
    float maximum_{0.0F};
    float value_{0.0F};
    float page_size_{0.0F};
    float step_{0.0F};
    float grab_offset_{0.0F};
    std::string style_{"default"};
    ChangeCallback callback_;
    std::optional<std::int64_t> drag_pointer_;
    bool focused_{false};
};

/** @brief Injectable selection state used by ListView. */
class ListSelectionModel {
public:
    virtual ~ListSelectionModel() = default;
    [[nodiscard]] virtual bool selected(std::size_t index) const noexcept = 0;
    [[nodiscard]] virtual std::optional<std::size_t> primary() const noexcept = 0;
    virtual void select(std::size_t index, KeyModifiers modifiers = {}) = 0;
    virtual void clear() noexcept = 0;
    virtual void trim(std::size_t item_count) noexcept = 0;
};

/** @brief Single-choice ListView selection model. */
class SingleListSelectionModel final : public ListSelectionModel {
public:
    [[nodiscard]] bool selected(std::size_t index) const noexcept override;
    [[nodiscard]] std::optional<std::size_t> primary() const noexcept override;
    void select(std::size_t index, KeyModifiers modifiers = {}) override;
    void clear() noexcept override;
    void trim(std::size_t item_count) noexcept override;

private:
    std::optional<std::size_t> selected_;
};

/** @brief Scrollable text list with externally supplied selection state. */
class ListView final : public Widget {
public:
    using SelectionCallback = std::function<void(std::optional<std::size_t>)>;

    /** @brief Construct a list with a default single-selection model. */
    explicit ListView(std::vector<std::string> items = {});

    /** @brief Replace all rows and trim selection to the new size. */
    void set_items(std::vector<std::string> items);

    /** @brief Return the immutable row labels. */
    [[nodiscard]] std::span<const std::string> items() const noexcept;

    /** @brief Inject shared selection state; null restores a private model. */
    void set_selection_model(std::shared_ptr<ListSelectionModel> model);

    /** @brief Return the active selection model. */
    [[nodiscard]] const std::shared_ptr<ListSelectionModel>& selection_model() const noexcept;

    /** @brief Set the first visible row, clamped during layout and painting. */
    void set_scroll_index(std::size_t index) noexcept;

    /** @brief Return the first visible row. */
    [[nodiscard]] std::size_t scroll_index() const noexcept { return scroll_index_; }

    /** @brief Install a callback fired after user-driven selection changes. */
    void set_on_selection_changed(SelectionCallback callback);

    /** @brief Select a named ListViewStyle. */
    void set_style(std::string style);

    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;
    [[nodiscard]] Size preferred_size(Painter&, const Skin&) const override;
    void paint(Painter&, const Skin&, float, float) const override;
    bool pointer_event(const PointerEvent&) override;
    bool key_down(Key, KeyModifiers = {}) override;
    void focus_changed(bool) override;
    [[nodiscard]] bool focusable() const noexcept override;

private:
    [[nodiscard]] std::size_t visible_rows(float row_height) const noexcept;
    void select_index(std::size_t index, KeyModifiers modifiers);
    void reveal(std::size_t index, float row_height) noexcept;

    std::vector<std::string> items_;
    std::shared_ptr<ListSelectionModel> selection_model_;
    std::size_t scroll_index_{0};
    std::string style_{"default"};
    SelectionCallback callback_;
    bool focused_{false};
    mutable float resolved_row_height_{44.0F};
};

/** @brief Read-only determinate progress indicator. */
class ProgressBar final : public Widget {
public:
    /**
     * @brief Construct a progress bar.
     * @param minimum Value represented by an empty bar.
     * @param maximum Value represented by a full bar.
     * @param value Initial value; clamped into the ordered range.
     */
    ProgressBar(float minimum = 0.0F, float maximum = 1.0F, float value = 0.0F);

    /**
     * @brief Replace the value range.
     * @param minimum First range endpoint.
     * @param maximum Second range endpoint.
     * @post Endpoints are ordered and the current value is clamped.
     */
    void set_range(float minimum, float maximum) noexcept;

    /** @brief Set and clamp the represented value. */
    void set_value(float value) noexcept;

    /** @brief Return the represented value. */
    [[nodiscard]] float value() const noexcept { return value_; }

    /** @brief Return completion normalized to the closed range `[0, 1]`. */
    [[nodiscard]] float progress() const noexcept;

    /** @brief Select a named ProgressBarStyle from the active Skin. */
    void set_style(std::string style);

    /** @brief Report the selected style's minimum horizontal extent. */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /** @brief Report the selected style's preferred extent. */
    [[nodiscard]] Size preferred_size(Painter&, const Skin&) const override;

    /** @brief Paint the complete track and completed portion. */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    float minimum_{0.0F};
    float maximum_{1.0F};
    float value_{0.0F};
    std::string style_{"default"};
};

/** @brief Thin vertical or horizontal divider line. */
class Separator final : public Widget {
public:
    /**
     * @brief Construct a separator.
     * @param direction Orientation of the divider.
     */
    explicit Separator(Direction direction = Direction::horizontal) noexcept;

    /**
     * @brief Report the line-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Active skin; unused by separator measurement.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Paint the divider line.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the border color, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

private:
    Direction direction_;
};

/**
 * @brief Floating table-backed panel with a draggable title bar.
 *
 * Windows are hosted by Ui; ownership transfers to the Ui when shown.
 */
class Window : public Widget {
public:
    /**
     * @brief Construct a window.
     * @param title Title-bar text.
     * @param style Named window style to apply.
     */
    explicit Window(std::string title = {}, std::string style = "default");

    /**
     * @brief Access the window's content table.
     * @return Reference to the content table for adding children.
     */
    [[nodiscard]] Table& content_table() noexcept { return *content_; }

    /**
     * @brief Access the window's content table, read-only.
     * @return Reference to the content table.
     */
    [[nodiscard]] const Table& content_table() const noexcept { return *content_; }

    /**
     * @brief Replace the title-bar text.
     * @param title New title text.
     */
    void set_title(std::string title);

    /**
     * @brief Read the title text.
     * @return Reference to the stored title.
     */
    [[nodiscard]] const std::string& title() const noexcept { return title_; }

    /**
     * @brief Change the applied window style.
     * @param style Named window style to apply.
     */
    void set_style(std::string style);

    /**
     * @brief Control modal blocking.
     * @param modal true dims the background outside the window.
     */
    void set_modal(bool modal) noexcept { modal_ = modal; }

    /**
     * @brief Read the modal state.
     * @return true while the window blocks background interaction.
     */
    [[nodiscard]] bool modal() const noexcept { return modal_; }

    /**
     * @brief Control title-bar dragging.
     * @param movable false keeps the window at its placed position.
     */
    void set_movable(bool movable) noexcept { movable_ = movable; }

    /**
     * @brief Read the movable state.
     * @return true while the window can be dragged by its title bar.
     */
    [[nodiscard]] bool movable() const noexcept { return movable_; }

    /**
     * @brief Control the close button.
     * @param closable true shows the close button.
     */
    void set_closable(bool closable) noexcept;

    /**
     * @brief Read the closable state.
     * @return true while the window shows a close button.
     */
    [[nodiscard]] bool closable() const noexcept { return closable_; }

    /**
     * @brief Control edge resizing.
     * @param resizable true enables resize grabs on the window border.
     */
    void set_resizable(bool resizable) noexcept { resizable_ = resizable; }

    /**
     * @brief Read the resizable state.
     * @return true while the window can be resized.
     */
    [[nodiscard]] bool resizable() const noexcept { return resizable_; }

    /**
     * @brief Set the smallest allowed window size.
     * @param size Minimum extent in logical units.
     */
    void set_minimum_window_size(Size size) noexcept;

    /**
     * @brief Read the minimum window size.
     * @return Requested minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_window_size() const noexcept
    {
        return requested_minimum_;
    }

    /**
     * @brief Request the window to close.
     *
     * The host Ui prunes windows marked close-requested on the next update.
     */
    void request_close() noexcept { close_requested_ = true; }

    /**
     * @brief Read the close-requested flag.
     * @return true after request_close() was called.
     */
    [[nodiscard]] bool close_requested() const noexcept { return close_requested_; }

    /**
     * @brief Report the minimum size constrained by layout.
     * @note Parameters match Widget::minimum_size: an active painter and the
     * skin providing the window style.
     * @return Minimum extent in logical units.
     */
    [[nodiscard]] Size minimum_size(Painter&, const Skin&) const override;

    /**
     * @brief Report the content-driven preferred size.
     * @param painter Active painter used for measurement.
     * @param skin Skin providing window metrics and the title font.
     * @return Preferred extent in logical units.
     */
    [[nodiscard]] Size preferred_size(Painter& painter, const Skin& skin) const override;

    /**
     * @brief Lay out the title bar and content table.
     * @note Parameters match Widget::layout: an active painter and the skin
     * providing the window style.
     */
    void layout(Painter&, const Skin&) override;

    /**
     * @brief Paint the title bar, background, and content.
     * @note Parameters match the Widget::paint signature: the active painter,
     * the skin providing the window style, and the stage-space origin.
     */
    void paint(Painter&, const Skin&, float, float) const override;

    /**
     * @brief Handle drag, resize, and close-button events.
     * @note The parameter matches Widget::pointer_event: a pointer payload in
     * widget-local coordinates.
     * @return true when the event was handled.
     */
    bool pointer_event(const PointerEvent&) override;

protected:
    /**
     * @brief Decide whether Escape closes the window.
     * @return false by default; overridden by Dialog.
     */
    [[nodiscard]] virtual bool escape_closes() const noexcept { return false; }

private:
    friend class Ui;
    enum ResizeEdge : unsigned int {
        resize_none = 0,
        resize_left = 1,
        resize_top = 2,
        resize_right = 4,
        resize_bottom = 8
    };
    [[nodiscard]] Rectangle close_bounds() const noexcept;
    [[nodiscard]] unsigned int resize_edges(float x, float y) const noexcept;
    void constrain_to_parent() noexcept;
    void resize_from_pointer(float stage_x, float stage_y) noexcept;
    std::string title_;
    std::string style_;
    Table* content_{nullptr};
    bool modal_{false};
    bool movable_{true};
    bool closable_{false};
    bool resizable_{false};
    bool close_requested_{false};
    bool close_hovered_{false};
    bool close_pressed_{false};
    float title_height_{36.0F};
    float close_size_{28.0F};
    float resize_border_{8.0F};
    Size requested_minimum_{};
    Size measured_minimum_{};
    std::optional<std::int64_t> interaction_pointer_;
    unsigned int resize_edges_{resize_none};
    float drag_offset_x_{0.0F};
    float drag_offset_y_{0.0F};
    Rectangle resize_start_{};
    float pointer_start_x_{0.0F};
    float pointer_start_y_{0.0F};
};

/**
 * @brief Modal Window with separate content and action-button tables.
 */
class Dialog final : public Window {
public:
    /** @brief Action invoked with the chosen result name. */
    using ResultCallback = std::function<void(std::string_view)>;

    /**
     * @brief Construct a dialog.
     * @param title Title-bar text.
     * @param result Result action; invoked when any button is chosen.
     */
    explicit Dialog(std::string title = {}, ResultCallback result = {});

    /**
     * @brief Access the dialog content table.
     * @return Reference to the content table for the message body.
     */
    [[nodiscard]] Table& dialog_content() noexcept { return *dialog_content_; }

    /**
     * @brief Access the action-button table.
     * @return Reference to the table receiving configured buttons.
     */
    [[nodiscard]] Table& button_table() noexcept { return *buttons_; }

    /**
     * @brief Append a message paragraph.
     * @param text Message text to display.
     * @return This dialog for chaining.
     */
    Dialog& text(std::string text);

    /**
     * @brief Append an action button.
     * @param text Button label.
     * @param result Result name reported when the button is chosen.
     * @return This dialog for chaining.
     */
    Dialog& button(std::string text, std::string result);

    /**
     * @brief Set the result action.
     * @param result Action copied into the dialog; invoked with the chosen
     * result name.
     */
    void set_on_result(ResultCallback result);

    /**
     * @brief Control Escape-to-cancel behavior.
     * @param enabled true reports a cancel result when Escape is pressed.
     */
    void set_cancel_on_escape(bool enabled) noexcept { cancel_on_escape_ = enabled; }

protected:
    /**
     * @brief Report Escape closing for dialogs.
     * @return true, unless cancel-on-escape was disabled.
     */
    [[nodiscard]] bool escape_closes() const noexcept override;

private:
    void choose(std::string result);
    Table* dialog_content_{nullptr};
    Table* buttons_{nullptr};
    ResultCallback result_;
    bool cancel_on_escape_{true};
};

/**
 * @brief Owns one widget tree and consumes the framework application event type.
 *
 * The Ui owns the stage, one content widget, and the set of open windows. All
 * input enters through event(), pointer(), and the key/text entry points. Not
 * thread-safe; one thread must drive it.
 */
class Ui {
public:
    /**
     * @brief Construct a user interface.
     * @param width Initial virtual width in logical pixels.
     * @param height Initial virtual height in logical pixels.
     * @param skin Skin styling every widget; copied into the Ui.
     */
    Ui(float width, float height, Skin skin = {});

    /**
     * @brief Replace the full-screen content widget.
     * @param content Widget to adopt; ownership transfers to the Ui.
     * @return Reference to the adopted content widget.
     */
    Widget& set_content(std::unique_ptr<Widget> content);

    /**
     * @brief Show a floating window.
     * @param window Window to adopt; ownership transfers to the Ui.
     * @param center true centers the window over the current viewport.
     * @return Reference to the shown window.
     */
    Window& show_window(std::unique_ptr<Window> window, bool center = true);

    /**
     * @brief Show a modal dialog.
     * @param dialog Dialog to adopt; ownership transfers to the Ui.
     * @param center true centers the dialog over the current viewport.
     * @return Reference to the shown dialog.
     */
    Dialog& show_dialog(std::unique_ptr<Dialog> dialog, bool center = true);

    /**
     * @brief Mark a window closed and remove it on the next update.
     * @param window Window currently shown by this Ui.
     */
    void close_window(Window& window);

    /**
     * @brief Count currently shown windows and dialogs.
     * @return Number of open overlays.
     */
    [[nodiscard]] std::size_t window_count() const noexcept
    {
        return overlays_.size();
    }

    /**
     * @brief Access the content widget.
     * @return Pointer to the content widget, or null when unset.
     */
    [[nodiscard]] Widget* content() noexcept { return content_; }

    /**
     * @brief Access the content widget, read-only.
     * @return Pointer to the content widget, or null when unset.
     */
    [[nodiscard]] const Widget* content() const noexcept { return content_; }

    /**
     * @brief Resize the virtual viewport.
     * @param width New width in logical pixels.
     * @param height New height in logical pixels.
     */
    void resize(float width, float height);

    /**
     * @brief Advance time-based state and prune close-requested windows.
     * @param delta_seconds Elapsed time in seconds; non-negative.
     * @throws Any exception raised by a tooltip factory, or
     * std::invalid_argument when a factory returns a parented widget.
     */
    void update(double delta_seconds);

    /**
     * @brief Validate layout of every visible widget.
     * @param painter Painter used for measurement.
     */
    void layout(Painter& painter);

    /**
     * @brief Paint the content and every open overlay.
     * @param painter Painter receiving all drawing.
     */
    void paint(Painter& painter) const;

    /**
     * @brief Consume one framework application event.
     * @param event Event to translate and dispatch.
     * @return true when the Ui handled the event.
     */
    bool event(const application::Event& event);

    /**
     * @brief Inject a pointer input.
     * @param action Pointer action being reported.
     * @param x Horizontal position in logical pixels.
     * @param y Vertical position in logical pixels.
     * @param button Button index; zero for the primary button.
     * @param pointer_id Stable identifier for the pointer contact.
     * @return true when some widget handled the event.
     */
    bool pointer(
        PointerAction action,
        float x,
        float y,
        int button = 0,
        std::int64_t pointer_id = 0
    );

    /**
     * @brief Dispatch a key-down event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the focused widget handled the key.
     */
    bool key_down(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Dispatch a key-up event.
     * @param key Portable key name.
     * @param modifiers Modifier state at the time of the event.
     * @return true when the focused widget handled the key.
     */
    bool key_up(Key key, KeyModifiers modifiers = {});

    /**
     * @brief Dispatch a navigation action, including focus moves.
     * @param navigation Semantic navigation action.
     * @param input_device_id Source device identifier.
     * @return true when the navigation was handled.
     */
    bool navigation(
        application::Event::Navigation navigation,
        std::int32_t input_device_id = 0
    );

    /**
     * @brief Forward committed text to the focused widget.
     * @param text Committed UTF-8 text.
     * @return true when the focused widget handled the text.
     */
    bool text_input(std::string_view text);

    /**
     * @brief Forward composition updates to the focused widget.
     * @param text Current composition text.
     * @param start Start offset of the composition span.
     * @param length Length of the composition span.
     * @return true when the focused widget handled the composition.
     */
    bool text_editing(std::string_view text, int start, int length);

    /**
     * @brief Install the platform text-input boundary.
     * @param service Non-owning service; may be null to disable. Must outlive
     * the Ui while set.
     */
    void set_text_input_service(application::TextInputService* service) noexcept;

    /**
     * @brief Clear keyboard focus to no widget.
     */
    void clear_focus();

    /**
     * @brief Read the focused widget.
     * @return Focused widget, or null when none is focused.
     */
    [[nodiscard]] Widget* focused() noexcept { return focused_; }

    /**
     * @brief Replace tooltip timing and placement policy.
     * @param config Finite, non-negative delays and distances.
     * @throws std::invalid_argument when any value is negative or non-finite.
     */
    void set_tooltip_config(TooltipConfig config);

    /** @brief Return the active tooltip policy.
     * @return Reference valid for the lifetime of this Ui.
     */
    [[nodiscard]] const TooltipConfig& tooltip_config() const noexcept
    {
        return tooltip_config_;
    }

    /** @brief Return true while a tooltip subtree is attached to the Stage.
     * @return true between materialization and dismissal.
     */
    [[nodiscard]] bool tooltip_visible() const noexcept
    {
        return tooltip_widget_ != nullptr;
    }

    /** @brief Return the widget whose tooltip is visible, or null.
     * @return Non-owning pointer invalidated when its owning tree is removed.
     */
    [[nodiscard]] const Widget* tooltip_owner() const noexcept
    {
        return tooltip_owner_;
    }

    /**
     * @brief Return the visible tooltip rectangle in Stage coordinates.
     * @return Bounds after layout, or no value when no tooltip is attached.
     */
    [[nodiscard]] std::optional<Rectangle> tooltip_bounds() const noexcept
    {
        if (!tooltip_widget_) return std::nullopt;
        return Rectangle{
            tooltip_widget_->x(), tooltip_widget_->y(),
            tooltip_widget_->width(), tooltip_widget_->height()
        };
    }

    /**
     * @brief Access the skin, read-only.
     * @return Reference to the Ui's skin.
     */
    [[nodiscard]] const Skin& skin() const noexcept { return skin_; }

    /**
     * @brief Access the skin for mutation.
     * @return Reference to the Ui's skin.
     */
    [[nodiscard]] Skin& skin() noexcept { return skin_; }

private:
    [[nodiscard]] Widget* widget_at(float x, float y) noexcept;
    static void paint_tree(
        const scene2d::Actor&, Painter&, const Skin&, float, float
    );
    void set_focus(Widget* widget);
    bool focus_next(bool reverse);
    bool focus_direction(Key direction);
    [[nodiscard]] std::vector<Widget*> focusable_widgets();
    static void collect_focusable(
        scene2d::Actor& actor,
        std::vector<Widget*>& result
    );
    static void stage_position(
        const scene2d::Actor& actor,
        float& x,
        float& y
    ) noexcept;
    [[nodiscard]] Widget* tooltip_owner_for(Widget* target) const noexcept;
    [[nodiscard]] bool tooltip_owner_allowed(const Widget* owner) const noexcept;
    void arm_focus_tooltip(Widget* owner) noexcept;
    void show_tooltip(Widget& owner, bool pointer_anchor);
    void hide_tooltip() noexcept;
    void reset_tooltip_candidates() noexcept;
    void layout_tooltip(Painter& painter);
    static void make_subtree_untouchable(scene2d::Actor& actor) noexcept;
    void prune_closed_windows();
    [[nodiscard]] Window* top_modal() noexcept;
    [[nodiscard]] const Window* top_modal() const noexcept;
    [[nodiscard]] static bool is_descendant_of(
        const scene2d::Actor* actor, const scene2d::Actor* ancestor
    ) noexcept;

    scene2d::Stage stage_;
    Skin skin_;
    Widget* content_{nullptr};
    Widget* focused_{nullptr};
    std::unordered_map<std::int64_t, Widget*> captures_;
    application::TextInputService* text_input_service_{nullptr};
    struct Overlay { Window* window; Widget* previous_focus; bool center_pending; };
    std::vector<Overlay> overlays_;
    TooltipConfig tooltip_config_;
    Widget* hover_tooltip_owner_{nullptr};
    double hover_tooltip_elapsed_{0.0};
    Widget* press_tooltip_owner_{nullptr};
    double press_tooltip_elapsed_{0.0};
    std::int64_t press_tooltip_pointer_id_{0};
    float press_start_x_{0.0F};
    float press_start_y_{0.0F};
    bool press_tooltip_active_{false};
    Widget* focus_tooltip_owner_{nullptr};
    double focus_tooltip_elapsed_{0.0};
    float tooltip_anchor_x_{0.0F};
    float tooltip_anchor_y_{0.0F};
    bool tooltip_pointer_anchor_{false};
    Widget* tooltip_owner_{nullptr};
    Widget* tooltip_widget_{nullptr};
};

} // namespace squared::gui
