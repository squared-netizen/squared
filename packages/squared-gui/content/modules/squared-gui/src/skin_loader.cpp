#include <squared/gui/skin_loader.hpp>

#include <squared/data/json.hpp>

#include <algorithm>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <utility>

namespace squared::gui {
namespace {

using data::JsonValue;

constexpr std::string_view color_class =
    "com.badlogic.gdx.graphics.Color";
constexpr std::string_view font_class =
    "com.badlogic.gdx.graphics.g2d.BitmapFont";
constexpr std::string_view tinted_class =
    "com.badlogic.gdx.scenes.scene2d.ui.Skin$TintedDrawable";
constexpr std::string_view button_class =
    "com.badlogic.gdx.scenes.scene2d.ui.Button$ButtonStyle";
constexpr std::string_view text_button_class =
    "com.badlogic.gdx.scenes.scene2d.ui.TextButton$TextButtonStyle";
constexpr std::string_view text_field_class =
    "com.badlogic.gdx.scenes.scene2d.ui.TextField$TextFieldStyle";
constexpr std::string_view label_class =
    "com.badlogic.gdx.scenes.scene2d.ui.Label$LabelStyle";
constexpr std::string_view check_box_class =
    "com.badlogic.gdx.scenes.scene2d.ui.CheckBox$CheckBoxStyle";
constexpr std::string_view slider_class =
    "com.badlogic.gdx.scenes.scene2d.ui.Slider$SliderStyle";
constexpr std::string_view progress_bar_class =
    "com.badlogic.gdx.scenes.scene2d.ui.ProgressBar$ProgressBarStyle";
constexpr std::string_view window_class =
    "com.badlogic.gdx.scenes.scene2d.ui.Window$WindowStyle";

bool bare_delimiter(char value) noexcept
{
    return value == '{' || value == '}' || value == '[' || value == ']' ||
           value == ':' || value == ',' ||
           value == ' ' || value == '\t' || value == '\r' || value == '\n';
}

bool strict_number(std::string_view token)
{
    if (token.empty()) return false;
    std::string owned(token);
    char* end = nullptr;
    errno = 0;
    const double value = std::strtod(owned.c_str(), &end);
    return errno != ERANGE && end == owned.c_str() + owned.size() &&
           std::isfinite(value);
}

void append_quoted(std::string& output, std::string_view token)
{
    output.push_back('"');
    for (char value : token) {
        if (value == '"' || value == '\\') output.push_back('\\');
        output.push_back(value);
    }
    output.push_back('"');
}

bool normalize_libgdx_json(
    std::string_view input,
    std::string& output,
    std::string& error
)
{
    output.clear();
    output.reserve(input.size() + input.size() / 8U);
    for (std::size_t index = 0; index < input.size();) {
        const char value = input[index];
        if (value == '"') {
            const std::size_t start = index++;
            bool escaped = false;
            bool closed = false;
            while (index < input.size()) {
                const char current = input[index++];
                if (escaped) escaped = false;
                else if (current == '\\') escaped = true;
                else if (current == '"') { closed = true; break; }
            }
            if (!closed) {
                error = "unterminated string";
                return false;
            }
            output.append(input.substr(start, index - start));
            continue;
        }
        if (value == '/' && index + 1U < input.size() &&
            input[index + 1U] == '/') {
            index += 2U;
            while (index < input.size() && input[index] != '\n') ++index;
            continue;
        }
        if (value == '/' && index + 1U < input.size() &&
            input[index + 1U] == '*') {
            index += 2U;
            const std::size_t close = input.find("*/", index);
            if (close == std::string_view::npos) {
                error = "unterminated block comment";
                return false;
            }
            index = close + 2U;
            continue;
        }
        if (bare_delimiter(value)) {
            output.push_back(value);
            ++index;
            continue;
        }
        const std::size_t start = index;
        while (index < input.size() && !bare_delimiter(input[index])) {
            if (input[index] == '"') break;
            ++index;
        }
        if (index == start) {
            error = "unexpected quote in bare token";
            return false;
        }
        const std::string_view token = input.substr(start, index - start);
        std::size_t lookahead = index;
        while (lookahead < input.size() &&
               (input[lookahead] == ' ' || input[lookahead] == '\t' ||
                input[lookahead] == '\r' || input[lookahead] == '\n')) {
            ++lookahead;
        }
        const bool key = lookahead < input.size() && input[lookahead] == ':';
        if (!key && (token == "true" || token == "false" || token == "null" ||
                     strict_number(token))) {
            output.append(token);
        } else {
            append_quoted(output, token);
        }
    }
    return true;
}

const std::string* string_member(
    const JsonValue::Object& object,
    std::string_view name
) noexcept
{
    const auto found = object.find(name);
    return found == object.end() ? nullptr : found->second.string_if();
}

bool contained_asset_path(std::string_view path) noexcept
{
    if (path.empty() || path.size() > 240U || path.front() == '/' ||
        path.find('\\') != std::string_view::npos) return false;
    std::size_t start = 0;
    while (start <= path.size()) {
        const std::size_t slash = path.find('/', start);
        const std::size_t end = slash == std::string_view::npos
            ? path.size() : slash;
        const std::string_view part = path.substr(start, end - start);
        if (part.empty() || part == "." || part == "..") return false;
        if (slash == std::string_view::npos) break;
        start = slash + 1U;
    }
    return true;
}

class Importer final {
public:
    Importer(
        Skin& destination,
        const SkinDrawableResolver& resolver,
        const SkinFontResolver& font_resolver,
        SkinLoadReport& report,
        const SkinLoadLimits& limits
    ) : destination_(destination), resolver_(resolver), report_(report),
        limits_(limits), font_resolver_(font_resolver)
    {
    }

    bool import(const JsonValue& root)
    {
        const auto* classes = root.object_if();
        if (!classes) {
            error("$", "skin root must be an object");
            return false;
        }
        if (classes->size() > limits_.maximum_resources) {
            error("$", "skin contains too many resource classes");
            return false;
        }
        std::size_t declared_resources = 0;
        for (const auto& [class_name, class_value] : *classes) {
            if (class_name.empty() || class_name.size() > 240U) {
                error(class_name, "resource class name is empty or too long");
                continue;
            }
            const auto* values = class_value.object_if();
            if (!values) {
                error(class_name, "resource class must be an object");
                continue;
            }
            if (values->size() > limits_.maximum_resources -
                    std::min(declared_resources, limits_.maximum_resources)) {
                error(class_name, "skin exceeds the resource limit");
                declared_resources = limits_.maximum_resources;
                break;
            }
            declared_resources += values->size();
            for (const auto& [resource_name, resource_value] : *values) {
                static_cast<void>(resource_value);
                if (resource_name.empty() ||
                    resource_name.size() > limits_.maximum_name_bytes) {
                    error(class_name + "." + resource_name,
                          "resource name is empty or too long");
                }
            }
        }
        if (!report_.success()) return false;
        load_colors(*classes);
        load_fonts(*classes);
        load_tinted_colors(*classes);
        load_buttons(*classes, button_class, false);
        load_buttons(*classes, text_button_class, true);
        load_labels(*classes);
        load_text_fields(*classes);
        load_check_boxes(*classes);
        load_sliders(*classes);
        load_progress_bars(*classes);
        load_windows(*classes);
        report_unsupported(*classes);
        if (!report_.success()) return false;
        destination_ = std::move(candidate_);
        return true;
    }

private:
    void issue(SkinLoadSeverity severity, std::string path, std::string message)
    {
        report_.issues.push_back({severity, std::move(path), std::move(message)});
    }

    void error(std::string path, std::string message)
    {
        issue(SkinLoadSeverity::error, std::move(path), std::move(message));
    }

    void warning(std::string path, std::string message)
    {
        issue(SkinLoadSeverity::warning, std::move(path), std::move(message));
    }

    const JsonValue::Object* section(
        const JsonValue::Object& classes,
        std::string_view class_name
    )
    {
        const auto found = classes.find(class_name);
        if (found == classes.end()) return nullptr;
        const auto* result = found->second.object_if();
        if (!result) error(std::string(class_name), "resource class must be an object");
        return result;
    }

    bool count_resource(std::string_view path, std::string_view name)
    {
        ++resources_;
        if (resources_ > limits_.maximum_resources) {
            error(std::string(path), "skin exceeds the resource limit");
            return false;
        }
        if (name.empty() || name.size() > limits_.maximum_name_bytes) {
            error(std::string(path), "resource name is empty or too long");
            return false;
        }
        return true;
    }

    static bool number(const JsonValue& value, float& output) noexcept
    {
        double numeric = 0.0;
        if (const auto* real = value.real_if()) numeric = *real;
        else if (const auto* signed_value = value.signed_integer_if()) {
            numeric = static_cast<double>(*signed_value);
        } else if (const auto* unsigned_value = value.unsigned_integer_if()) {
            numeric = static_cast<double>(*unsigned_value);
        } else return false;
        if (!std::isfinite(numeric)) return false;
        output = static_cast<float>(numeric);
        return true;
    }

    bool parse_color(
        const JsonValue& value,
        std::string_view path,
        graphics::Color& color
    )
    {
        const auto* object = value.object_if();
        if (!object) {
            error(std::string(path), "color must be an object");
            return false;
        }
        graphics::Color parsed{0.0F, 0.0F, 0.0F, 1.0F};
        for (const auto& [name, component] : {
                 std::pair{"r", &parsed.red}, std::pair{"g", &parsed.green},
                 std::pair{"b", &parsed.blue}, std::pair{"a", &parsed.alpha}
             }) {
            const auto found = object->find(name);
            if (found == object->end()) continue;
            if (!number(found->second, *component) || *component < 0.0F ||
                *component > 1.0F) {
                error(std::string(path) + "." + name,
                      "color component must be between zero and one");
                return false;
            }
        }
        color = parsed;
        return true;
    }

    graphics::Color color_value(
        const JsonValue::Object& style,
        std::string_view field,
        graphics::Color fallback,
        std::string_view path
    )
    {
        const auto found = style.find(field);
        if (found == style.end()) return fallback;
        if (const auto* name = found->second.string_if()) {
            const auto color = colors_.find(*name);
            if (color == colors_.end()) {
                error(std::string(path) + "." + std::string(field),
                      "unknown color resource: " + *name);
                return fallback;
            }
            return color->second;
        }
        graphics::Color parsed;
        if (!parse_color(found->second,
                         std::string(path) + "." + std::string(field), parsed)) {
            return fallback;
        }
        return parsed;
    }

    DrawablePtr drawable_value(
        const JsonValue::Object& style,
        std::string_view field,
        DrawablePtr fallback,
        std::string_view path
    )
    {
        const auto found = style.find(field);
        if (found == style.end()) return fallback;
        const auto* name = found->second.string_if();
        if (!name || name->empty() || name->size() > limits_.maximum_name_bytes) {
            error(std::string(path) + "." + std::string(field),
                  "drawable reference must be a bounded name");
            return fallback;
        }
        const auto cached = drawables_.find(*name);
        if (cached != drawables_.end()) return cached->second;
        DrawablePtr resolved = resolver_ ? resolver_(*name) : DrawablePtr{};
        if (!resolved) {
            error(std::string(path) + "." + std::string(field),
                  "unresolved drawable: " + *name);
            return fallback;
        }
        candidate_.add_drawable(*name, resolved);
        drawables_.emplace(*name, resolved);
        ++report_.drawables_loaded;
        return resolved;
    }

    FontPtr font_value(
        const JsonValue::Object& style,
        std::string_view field,
        FontPtr fallback,
        std::string_view path
    )
    {
        const auto found = style.find(field);
        if (found == style.end()) return fallback;
        const auto* name = found->second.string_if();
        if (!name || name->empty() || name->size() > limits_.maximum_name_bytes) {
            error(std::string(path) + "." + std::string(field),
                  "font reference must be a bounded name");
            return fallback;
        }
        const auto font = fonts_.find(*name);
        if (font == fonts_.end()) {
            error(std::string(path) + "." + std::string(field),
                  "unknown font resource: " + *name);
            return fallback;
        }
        return font->second;
    }

    void load_colors(const JsonValue::Object& classes)
    {
        const auto* values = section(classes, color_class);
        if (!values) return;
        for (const auto& [name, value] : *values) {
            const std::string path = std::string(color_class) + "." + name;
            if (!count_resource(path, name)) continue;
            graphics::Color color;
            if (parse_color(value, path, color)) {
                colors_.emplace(name, color);
                ++report_.colors_loaded;
            }
        }
        if (const auto found = colors_.find("default"); found != colors_.end()) {
            candidate_.text = found->second;
        }
        if (const auto found = colors_.find("grey"); found != colors_.end()) {
            candidate_.muted_text = found->second;
        }
        if (const auto found = colors_.find("ui"); found != colors_.end()) {
            candidate_.accent = found->second;
        }
        if (const auto found = colors_.find("white"); found != colors_.end()) {
            candidate_.surface = found->second;
            candidate_.add_panel_style("default", {
                std::make_shared<ColorDrawable>(found->second)
            });
        }
    }

    void load_fonts(const JsonValue::Object& classes)
    {
        const auto* values = section(classes, font_class);
        if (!values) return;
        for (const auto& [name, value] : *values) {
            const std::string path = std::string(font_class) + "." + name;
            if (!count_resource(path, name)) continue;
            const auto* object = value.object_if();
            const auto* file = object ? string_member(*object, "file") : nullptr;
            if (!file || !contained_asset_path(*file)) {
                error(path + ".file", "font path must stay within the skin directory");
                continue;
            }
            FontPtr font = font_resolver_
                ? font_resolver_(name, *file)
                : std::make_shared<FontResource>(*file);
            if (!font) {
                error(path + ".file", "font resolver rejected: " + *file);
                continue;
            }
            candidate_.add_font(name, font);
            fonts_.emplace(name, std::move(font));
            ++report_.fonts_loaded;
        }
    }

    enum class StyleVisit { visiting, complete };

    template <typename Style, typename Apply, typename Commit>
    void load_typed_styles(
        const JsonValue::Object& classes,
        std::string_view class_name,
        std::string_view description,
        Style fallback,
        Apply apply,
        Commit commit
    )
    {
        const auto* values = section(classes, class_name);
        if (!values) return;

        std::unordered_set<std::string> accepted;
        for (const auto& [name, value] : *values) {
            static_cast<void>(value);
            const std::string path = std::string(class_name) + "." + name;
            if (count_resource(path, name)) accepted.insert(name);
        }

        std::unordered_map<std::string, Style> resolved;
        std::unordered_map<std::string, StyleVisit> visits;
        std::function<const Style*(const std::string&)> resolve;
        resolve = [&](const std::string& name) -> const Style* {
            if (!accepted.contains(name)) return nullptr;
            if (const auto ready = resolved.find(name); ready != resolved.end()) {
                return &ready->second;
            }
            if (const auto visit = visits.find(name);
                visit != visits.end() && visit->second == StyleVisit::visiting) {
                error(std::string(class_name) + "." + name,
                      "cyclic typed style inheritance");
                return nullptr;
            }
            const auto source = values->find(name);
            if (source == values->end()) {
                error(std::string(class_name) + "." + name,
                      "unknown parent " + std::string(description) + " style");
                return nullptr;
            }
            const auto* object = source->second.object_if();
            const std::string path = std::string(class_name) + "." + name;
            if (!object) {
                error(path, std::string(description) + " style must be an object");
                return nullptr;
            }

            visits[name] = StyleVisit::visiting;
            Style style = fallback;
            const std::string* parent = string_member(*object, "parent");
            const std::string* extends = string_member(*object, "extends");
            if (parent && extends) {
                error(path, "style cannot declare both parent and extends");
                return nullptr;
            }
            const std::string* base = parent ? parent : extends;
            if (base) {
                if (base->empty() || base->size() > limits_.maximum_name_bytes) {
                    error(path, "parent style name is empty or too long");
                    return nullptr;
                }
                const Style* inherited = resolve(*base);
                if (!inherited) return nullptr;
                style = *inherited;
            }
            apply(style, *object, path);
            visits[name] = StyleVisit::complete;
            const auto [stored, inserted] = resolved.emplace(name, std::move(style));
            static_cast<void>(inserted);
            return &stored->second;
        };

        for (const auto& [name, value] : *values) {
            static_cast<void>(value);
            if (!accepted.contains(name)) continue;
            if (const Style* style = resolve(name)) {
                commit(name, *style);
                ++report_.styles_loaded;
            }
        }
    }

    void load_tinted_colors(const JsonValue::Object& classes)
    {
        const auto* values = section(classes, tinted_class);
        if (!values) return;
        for (const auto& [name, value] : *values) {
            const std::string path = std::string(tinted_class) + "." + name;
            if (!count_resource(path, name)) continue;
            const auto* object = value.object_if();
            if (!object) {
                error(path, "tinted drawable must be an object");
                continue;
            }
            const auto color = object->find("color");
            graphics::Color parsed;
            if (!object || color == object->end() ||
                !parse_color(color->second, path + ".color", parsed)) continue;
            tinted_colors_.emplace(name, parsed);
        }
    }

    void load_buttons(
        const JsonValue::Object& classes,
        std::string_view class_name,
        bool text_style
    )
    {
        load_typed_styles(
            classes, class_name, "button",
            candidate_.button_style("default"),
            [this, text_style](
                ButtonStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.normal = drawable_value(object, "up", style.normal, path);
                style.hovered = drawable_value(
                    object, "over", style.normal, path
                );
                style.pressed = drawable_value(
                    object, "down", style.pressed, path
                );
                style.disabled = drawable_value(
                    object, "disabled", style.disabled, path
                );
                if (text_style) {
                    style.font = font_value(object, "font", style.font, path);
                    style.text = color_value(
                        object, "fontColor", style.text, path
                    );
                    style.disabled_text = color_value(
                        object, "disabledFontColor", style.disabled_text, path
                    );
                }
            },
            [this](const std::string& name, const ButtonStyle& style) {
                candidate_.add_button_style(name, style);
            }
        );
    }

    void load_labels(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, label_class, "label", candidate_.label_style("default"),
            [this](
                LabelStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.font = font_value(object, "font", style.font, path);
                if (object.contains("fontColor")) {
                    style.text = color_value(
                        object, "fontColor",
                        style.text.value_or(candidate_.text), path
                    );
                }
                if (object.contains("disabledFontColor")) {
                    style.muted_text = color_value(
                        object, "disabledFontColor",
                        style.muted_text.value_or(candidate_.muted_text), path
                    );
                }
            },
            [this](const std::string& name, const LabelStyle& style) {
                candidate_.add_label_style(name, style);
            }
        );
    }

    void load_text_fields(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, text_field_class, "text-field",
            candidate_.text_field_style("default"),
            [this](
                TextFieldStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.normal = drawable_value(
                    object, "background", style.normal, path
                );
                style.focused = drawable_value(
                    object, "focusedBackground", style.normal, path
                );
                style.font = font_value(object, "font", style.font, path);
                style.text = color_value(object, "fontColor", style.text, path);
            },
            [this](const std::string& name, const TextFieldStyle& style) {
                candidate_.add_text_field_style(name, style);
            }
        );
    }

    void load_check_boxes(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, check_box_class, "check-box",
            candidate_.check_box_style("default"),
            [this](
                CheckBoxStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.unchecked = drawable_value(
                    object, "checkboxOff", style.unchecked, path
                );
                style.checked = drawable_value(
                    object, "checkboxOn", style.checked, path
                );
                style.disabled = drawable_value(
                    object, "checkboxOffDisabled", style.disabled, path
                );
                style.font = font_value(object, "font", style.font, path);
                style.text = color_value(object, "fontColor", style.text, path);
            },
            [this](const std::string& name, const CheckBoxStyle& style) {
                candidate_.add_check_box_style(name, style);
            }
        );
    }

    void load_sliders(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, slider_class, "slider", candidate_.slider_style("default"),
            [this](
                SliderStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.track = drawable_value(
                    object, "background", style.track, path
                );
                style.filled_track = drawable_value(
                    object, "knobBefore", style.track, path
                );
                style.knob = drawable_value(object, "knob", style.knob, path);
            },
            [this](const std::string& name, const SliderStyle& style) {
                candidate_.add_slider_style(name, style);
            }
        );
    }

    void load_progress_bars(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, progress_bar_class, "progress-bar",
            candidate_.progress_bar_style("default"),
            [this](
                ProgressBarStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.track = drawable_value(
                    object, "background", style.track, path
                );
                style.fill = drawable_value(
                    object, "knobBefore", style.fill, path
                );
            },
            [this](const std::string& name, const ProgressBarStyle& style) {
                candidate_.add_progress_bar_style(name, style);
            }
        );
    }

    void load_windows(const JsonValue::Object& classes)
    {
        load_typed_styles(
            classes, window_class, "window", candidate_.window_style("default"),
            [this](
                WindowStyle& style,
                const JsonValue::Object& object,
                const std::string& path
            ) {
                style.background = drawable_value(
                    object, "background", style.background, path
                );
                style.title_font = font_value(
                    object, "titleFont", style.title_font, path
                );
                style.title_text = color_value(
                    object, "titleFontColor", style.title_text, path
                );
                if (const auto* stage = string_member(object, "stageBackground")) {
                    const auto color = tinted_colors_.find(*stage);
                    if (color == tinted_colors_.end()) {
                        error(path + ".stageBackground",
                              "unknown tinted drawable: " + *stage);
                    } else {
                        style.modal_overlay = color->second;
                    }
                }
            },
            [this](const std::string& name, const WindowStyle& style) {
                candidate_.add_window_style(name, style);
            }
        );
    }

    void report_unsupported(const JsonValue::Object& classes)
    {
        static const std::unordered_set<std::string_view> supported{
            color_class, font_class, tinted_class, button_class,
            text_button_class, label_class, text_field_class, check_box_class,
            slider_class, progress_bar_class, window_class
        };
        for (const auto& [name, value] : classes) {
            static_cast<void>(value);
            if (!supported.contains(name)) {
                warning(name, "libGDX resource class is not used by this GUI slice");
            }
        }
    }

    Skin& destination_;
    const SkinDrawableResolver& resolver_;
    SkinLoadReport& report_;
    const SkinLoadLimits& limits_;
    const SkinFontResolver& font_resolver_;
    Skin candidate_;
    std::size_t resources_{0};
    std::unordered_map<std::string, graphics::Color> colors_;
    std::unordered_map<std::string, graphics::Color> tinted_colors_;
    std::unordered_map<std::string, DrawablePtr> drawables_;
    std::unordered_map<std::string, FontPtr> fonts_;
};

} // namespace

bool SkinLoadReport::success() const noexcept
{
    return std::none_of(
        issues.begin(), issues.end(),
        [](const SkinLoadIssue& issue) {
            return issue.severity == SkinLoadSeverity::error;
        }
    );
}

bool load_libgdx_skin(
    Skin& destination,
    std::string_view json,
    const SkinDrawableResolver& resolver,
    const SkinFontResolver& font_resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits
) noexcept
{
    report = {};
    try {
        if (json.size() > limits.maximum_json_bytes) {
            report.issues.push_back({
                SkinLoadSeverity::error, "$", "skin JSON exceeds the byte limit"
            });
            return false;
        }
        std::string strict;
        std::string normalization_error;
        if (!normalize_libgdx_json(json, strict, normalization_error)) {
            report.issues.push_back({
                SkinLoadSeverity::error, "$", std::move(normalization_error)
            });
            return false;
        }
        data::JsonParseOptions options;
        options.maximum_bytes = strict.size();
        options.maximum_depth = limits.maximum_depth;
        options.reject_duplicate_keys = true;
        auto parsed = data::parse_json(strict, options);
        if (!parsed) {
            report.issues.push_back({
                SkinLoadSeverity::error,
                "$",
                "invalid libGDX skin JSON at line " +
                    std::to_string(parsed.error.line) + ":" +
                    std::to_string(parsed.error.column) + ": " +
                    parsed.error.message
            });
            return false;
        }
        Importer importer(destination, resolver, font_resolver, report, limits);
        return importer.import(parsed.value);
    } catch (const std::exception& exception) {
        report.issues.push_back({
            SkinLoadSeverity::error, "$",
            std::string("skin loading failed: ") + exception.what()
        });
        return false;
    } catch (...) {
        report.issues.push_back({
            SkinLoadSeverity::error, "$", "skin loading failed unexpectedly"
        });
        return false;
    }
}

bool load_libgdx_skin(
    Skin& destination,
    std::string_view json,
    const SkinDrawableResolver& resolver,
    SkinLoadReport& report,
    const SkinLoadLimits& limits
) noexcept
{
    const SkinFontResolver descriptor_only;
    return load_libgdx_skin(
        destination, json, resolver, descriptor_only, report, limits
    );
}

} // namespace squared::gui
