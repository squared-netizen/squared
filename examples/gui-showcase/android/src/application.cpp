#include <SDL.h>
#include <SDL_opengles2_khrplatform.h>
#include <SDL_opengles2_gl2platform.h>
#include <SDL_opengles2_gl2.h>
#include <SDL_ttf.h>

#include <squared_gui_showcase/application.hpp>
#include <squared_gui_showcase/showcase.hpp>
#include <squared/application/application.hpp>
#include <squared/graphics/context.hpp>
#include <squared/graphics2d/orthographic_camera.hpp>
#include <squared/graphics2d/sprite_batch.hpp>
#include <squared/graphics2d/texture.hpp>
#include <squared/graphics2d/texture_atlas.hpp>
#include <squared/graphics2d/texture_region.hpp>
#include <squared/gui/skin_loader.hpp>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <unordered_map>
#include <utility>
#include <vector>

namespace squared_gui_showcase {
namespace {

constexpr float logical_width = 960.0F;
constexpr float logical_height = 540.0F;
constexpr const char* font_asset = "fonts/DejaVuSansMono.ttf";
constexpr const char* skin_atlas_asset =
    "gui/gdx-skins/selected/gdx-holo/uiskin.atlas";
constexpr const char* skin_json_asset =
    "gui/gdx-skins/selected/gdx-holo/uiskin.json";

bool read_asset_text(const char* path, std::string& text)
{
    SDL_RWops* stream = SDL_RWFromFile(path, "rb");
    if (!stream) return false;
    const Sint64 size = SDL_RWsize(stream);
    if (size < 0 || size > 1024 * 1024) {
        SDL_RWclose(stream);
        return false;
    }
    text.resize(static_cast<std::size_t>(size));
    const std::size_t read = size == 0 ? 0 : SDL_RWread(
        stream, text.data(), 1, static_cast<std::size_t>(size)
    );
    SDL_RWclose(stream);
    return read == static_cast<std::size_t>(size);
}

SDL_Color to_sdl_color(squared::graphics::Color color) noexcept
{
    const auto safe = color.clamped();
    const auto component = [](float value) {
        return static_cast<Uint8>(value * 255.0F + 0.5F);
    };
    return {
        component(safe.red), component(safe.green),
        component(safe.blue), component(safe.alpha)
    };
}

squared::gui::Rectangle intersection(
    const squared::gui::Rectangle& first,
    const squared::gui::Rectangle& second
) noexcept
{
    const float left = std::max(first.x, second.x);
    const float top = std::max(first.y, second.y);
    const float right = std::min(
        first.x + first.width, second.x + second.width
    );
    const float bottom = std::min(
        first.y + first.height, second.y + second.height
    );
    return {
        left, top, std::max(0.0F, right - left),
        std::max(0.0F, bottom - top)
    };
}

class SpriteGuiPainter final : public squared::gui::Painter {
public:
    SpriteGuiPainter(
        squared::graphics2d::SpriteBatch& batch,
        const squared::graphics2d::TextureRegion& white,
        int pixel_width,
        int pixel_height
    ) : batch_(batch), white_(white),
        pixel_width_(pixel_width), pixel_height_(pixel_height)
    {
        font_ = TTF_OpenFont(font_asset, 18);
    }

    ~SpriteGuiPainter() override
    {
        cache_.clear();
        if (font_) TTF_CloseFont(font_);
    }

    [[nodiscard]] bool valid() const noexcept { return font_ != nullptr; }

    void set_pixel_size(int width, int height) noexcept
    {
        pixel_width_ = std::max(1, width);
        pixel_height_ = std::max(1, height);
    }

    void begin_frame()
    {
        clips_.clear();
        glDisable(GL_SCISSOR_TEST);
    }

    void end_frame()
    {
        batch_.flush();
        glDisable(GL_SCISSOR_TEST);
        clips_.clear();
    }

    void invalidate_graphics() noexcept
    {
        clips_.clear();
        for (auto& [text, cached] : cache_) {
            static_cast<void>(text);
            cached->texture.invalidate();
        }
    }

    [[nodiscard]] bool restore_graphics(bool context_preserved) noexcept
    {
        for (auto& [text, cached] : cache_) {
            static_cast<void>(text);
            if (!cached->texture.restore(context_preserved)) return false;
        }
        return true;
    }

    squared::gui::Size measure_text(std::string_view text) override
    {
        if (!font_) return {};
        int width = 0;
        int height = TTF_FontHeight(font_);
        const std::string owned(text);
        if (!owned.empty() && TTF_SizeUTF8(
                font_, owned.c_str(), &width, &height
            ) != 0) {
            return {};
        }
        return {static_cast<float>(width), static_cast<float>(height)};
    }

    void fill_rectangle(
        const squared::gui::Rectangle& rectangle,
        squared::graphics::Color color
    ) override
    {
        batch_.draw(
            white_, rectangle.x, rectangle.y,
            rectangle.width, rectangle.height, color
        );
    }

    void stroke_rectangle(
        const squared::gui::Rectangle& rectangle,
        squared::graphics::Color color,
        float thickness
    ) override
    {
        const float safe = std::max(0.0F, thickness);
        fill_rectangle({rectangle.x, rectangle.y, rectangle.width, safe}, color);
        fill_rectangle({rectangle.x, rectangle.y + rectangle.height - safe,
                        rectangle.width, safe}, color);
        fill_rectangle({rectangle.x, rectangle.y, safe, rectangle.height}, color);
        fill_rectangle({rectangle.x + rectangle.width - safe, rectangle.y,
                        safe, rectangle.height}, color);
    }

    void draw_region(
        const squared::graphics2d::TextureRegion& region,
        const squared::gui::Rectangle& rectangle,
        squared::graphics::Color color
    ) override
    {
        if (!region.valid()) return;
        batch_.draw(
            region, rectangle.x, rectangle.y,
            rectangle.width, rectangle.height, color
        );
    }

    void draw_text(
        std::string_view text,
        float x,
        float y,
        squared::graphics::Color color
    ) override
    {
        CachedText* cached = text_texture(text);
        if (!cached) return;
        batch_.draw(
            cached->region, x, y,
            static_cast<float>(cached->width),
            static_cast<float>(cached->height), color
        );
    }

    void push_clip(const squared::gui::Rectangle& rectangle) override
    {
        batch_.flush();
        clips_.push_back(
            clips_.empty() ? rectangle : intersection(clips_.back(), rectangle)
        );
        apply_clip();
    }

    void pop_clip() override
    {
        batch_.flush();
        if (!clips_.empty()) clips_.pop_back();
        apply_clip();
    }

private:
    struct CachedText {
        squared::graphics2d::Texture texture;
        squared::graphics2d::TextureRegion region;
        std::string text;
        TTF_Font* font{nullptr};
        int width{0};
        int height{0};
    };

    static bool regenerate_text(
        void* user_data,
        squared::graphics2d::TextureRecoveryTarget& target
    ) noexcept
    {
        auto* cached = static_cast<CachedText*>(user_data);
        if (!cached || !cached->font || cached->text.empty()) return false;
        SDL_Surface* rendered = TTF_RenderUTF8_Blended(
            cached->font,
            cached->text.c_str(),
            to_sdl_color(squared::graphics::Color::white())
        );
        if (!rendered) return false;
        SDL_Surface* rgba = SDL_ConvertSurfaceFormat(
            rendered, SDL_PIXELFORMAT_ABGR8888, 0
        );
        SDL_FreeSurface(rendered);
        if (!rgba) return false;
        const bool uploaded = target.upload_rgba(
            rgba->w,
            rgba->h,
            static_cast<const std::uint8_t*>(rgba->pixels)
        );
        SDL_FreeSurface(rgba);
        return uploaded;
    }

    CachedText* text_texture(std::string_view text)
    {
        const std::string key(text);
        if (key.empty() || !font_) return nullptr;
        const auto found = cache_.find(key);
        if (found != cache_.end()) return found->second.get();

        SDL_Surface* rendered = TTF_RenderUTF8_Blended(
            font_, key.c_str(), to_sdl_color(squared::graphics::Color::white())
        );
        if (!rendered) return nullptr;
        SDL_Surface* rgba = SDL_ConvertSurfaceFormat(
            rendered, SDL_PIXELFORMAT_ABGR8888, 0
        );
        SDL_FreeSurface(rendered);
        if (!rgba) return nullptr;

        auto cached = std::make_unique<CachedText>();
        cached->text = key;
        cached->font = font_;
        cached->width = rgba->w;
        cached->height = rgba->h;
        const bool created = cached->texture.create_rgba(
            rgba->w, rgba->h,
            static_cast<const std::uint8_t*>(rgba->pixels),
            squared::graphics2d::TextureRecoveryOptions::regenerate(
                &regenerate_text,
                cached.get()
            )
        );
        SDL_FreeSurface(rgba);
        if (!created) return nullptr;
        cached->region = squared::graphics2d::TextureRegion(cached->texture);
        CachedText* result = cached.get();
        cache_.emplace(key, std::move(cached));
        return result;
    }

    void apply_clip()
    {
        if (clips_.empty()) {
            glDisable(GL_SCISSOR_TEST);
            return;
        }
        const auto& clip = clips_.back();
        const float scale_x = static_cast<float>(pixel_width_) / logical_width;
        const float scale_y = static_cast<float>(pixel_height_) / logical_height;
        const int x = static_cast<int>(clip.x * scale_x);
        const int y = static_cast<int>(
            (logical_height - clip.y - clip.height) * scale_y
        );
        const int width = static_cast<int>(clip.width * scale_x + 0.5F);
        const int height = static_cast<int>(clip.height * scale_y + 0.5F);
        glEnable(GL_SCISSOR_TEST);
        glScissor(x, y, std::max(0, width), std::max(0, height));
    }

    squared::graphics2d::SpriteBatch& batch_;
    const squared::graphics2d::TextureRegion& white_;
    TTF_Font* font_{nullptr};
    int pixel_width_{1};
    int pixel_height_{1};
    std::vector<squared::gui::Rectangle> clips_;
    std::unordered_map<std::string, std::unique_ptr<CachedText>> cache_;
};

squared::gui::Skin make_skin(
    squared::graphics2d::TextureAtlas& atlas,
    std::string_view json,
    squared::gui::DrawablePtr& showcase_image
)
{
    squared::gui::Skin skin;
    squared::gui::SkinLoadReport report;
    if (!squared::gui::load_libgdx_skin(
            skin,
            json,
            [&atlas](std::string_view name) {
                return squared::gui::resolve_atlas_drawable(atlas, name);
            },
            report
        )) {
        SDL_Log("gdx-holo skin rejected; using the programmatic fallback");
    } else {
        skin.add_slider_style(
            "default", skin.slider_style("default-horizontal")
        );
        showcase_image = squared::gui::resolve_atlas_drawable(
            atlas, "apptheme_btn_check_off_holo_light"
        );
    }
    for (const auto& issue : report.issues) {
        SDL_Log(
            "gdx-holo %s at %s: %s",
            issue.severity == squared::gui::SkinLoadSeverity::error
                ? "error" : "warning",
            issue.path.c_str(), issue.message.c_str()
        );
    }
    return skin;
}

class GuiShowcaseApplication final : public squared::application::Application {
public:
    GuiShowcaseApplication()
        : camera_(
            logical_width, logical_height,
            squared::graphics2d::CoordinateOrigin::TopLeft
        )
    {
    }

    void set_text_input_service(
        squared::application::TextInputService* service
    ) noexcept override
    {
        text_input_service_ = service;
        if (showcase_) showcase_->ui().set_text_input_service(service);
    }

    [[nodiscard]] bool create(squared::graphics::Context&) override
    {
        return true;
    }

    void surface_destroyed() override
    {
        if (!surface_ready_) return;
        if (painter_) painter_->invalidate_graphics();
        atlas_.invalidate();
        white_texture_.invalidate();
        batch_.invalidate();
        surface_ready_ = false;
    }

    void surface_created(squared::graphics::Context& graphics) override
    {
        if (surface_ready_) return;
        if (!showcase_) {
            if (!initialize_graphics(graphics)) quit_requested_ = true;
            return;
        }
        const bool preserved = graphics.resources_preserved();
        const bool restored = batch_.restore(preserved) &&
            white_texture_.restore(preserved) && atlas_.restore(preserved) &&
            painter_ && painter_->restore_graphics(preserved);
        if (!restored) {
            SDL_Log("GUI showcase graphics restoration failed");
            atlas_.release();
            white_texture_.release();
            batch_.release();
            quit_requested_ = true;
            return;
        }
        if (showcase_) {
            showcase_->report_graphics_recovery(
                graphics.generation(), preserved
            );
        }
        surface_ready_ = true;
    }

    void handle_event(const squared::application::Event& event) override
    {
        if (event.type == squared::application::Event::Type::QuitRequested) {
            quit_requested_ = true;
        } else if (event.type == squared::application::Event::Type::BackRequested) {
            if (!showcase_ || !showcase_->ui().key_down(squared::gui::Key::escape)) {
                quit_requested_ = true;
            }
        }
        if (showcase_) showcase_->ui().event(event);
    }

    void update(std::chrono::nanoseconds delta) override
    {
        if (showcase_) {
            showcase_->ui().update(std::chrono::duration<double>(delta).count());
        }
    }

    void render(squared::graphics::Context& graphics) override
    {
        if (!surface_ready_) return;
        graphics.clear(squared::graphics::Color::from_rgba8(13, 18, 27));
        if (!showcase_ || !painter_ || !batch_.begin(camera_)) return;
        painter_->set_pixel_size(graphics.pixel_width(), graphics.pixel_height());
        painter_->begin_frame();
        showcase_->ui().layout(*painter_);
        showcase_->ui().paint(*painter_);
        painter_->end_frame();
        batch_.end();
    }

    void resize(int width, int height) override
    {
        if (painter_) painter_->set_pixel_size(width, height);
    }

    void dispose() override
    {
        showcase_.reset();
        painter_.reset();
        atlas_.destroy();
        white_texture_.destroy();
        batch_.destroy();
        surface_ready_ = false;
    }

    [[nodiscard]] bool quit_requested() const noexcept override
    {
        return quit_requested_;
    }

private:
    [[nodiscard]] bool initialize_graphics(
        squared::graphics::Context& graphics
    )
    {
        if (!batch_.initialize() ||
            !white_texture_.create_solid(squared::graphics::Color::white())) {
            SDL_Log("GUI showcase graphics initialization failed");
            dispose();
            return false;
        }
        white_region_ = squared::graphics2d::TextureRegion(white_texture_);
        if (!atlas_.load(skin_atlas_asset)) {
            SDL_Log("GUI showcase skin atlas failed to load");
            dispose();
            return false;
        }
        std::string skin_json;
        if (!read_asset_text(skin_json_asset, skin_json)) {
            SDL_Log("GUI showcase skin JSON failed to load");
            dispose();
            return false;
        }
        painter_ = std::make_unique<SpriteGuiPainter>(
            batch_, white_region_, graphics.pixel_width(), graphics.pixel_height()
        );
        if (!painter_->valid()) {
            SDL_Log("GUI showcase font failed to load: %s", TTF_GetError());
            dispose();
            return false;
        }
        squared::gui::DrawablePtr image;
        auto skin = make_skin(atlas_, skin_json, image);
        showcase_ = std::make_unique<Showcase>(
            logical_width, logical_height, std::move(skin), std::move(image)
        );
        showcase_->ui().set_text_input_service(text_input_service_);
        surface_ready_ = true;
        return true;
    }

    squared::graphics2d::OrthographicCamera camera_;
    squared::graphics2d::SpriteBatch batch_;
    squared::graphics2d::Texture white_texture_;
    squared::graphics2d::TextureRegion white_region_;
    squared::graphics2d::TextureAtlas atlas_;
    std::unique_ptr<SpriteGuiPainter> painter_;
    std::unique_ptr<Showcase> showcase_;
    squared::application::TextInputService* text_input_service_{nullptr};
    bool quit_requested_{false};
    bool surface_ready_{false};
};

} // namespace

std::unique_ptr<squared::application::Application> create_application()
{
    return std::make_unique<GuiShowcaseApplication>();
}

} // namespace squared_gui_showcase
