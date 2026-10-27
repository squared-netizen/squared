#pragma once

#include <squared/gui/src/size.hpp>
#include <squared/gui/widget.hpp>

#include <string_view>

namespace sq::gui
{

class Painter;
class Skin;
class Table;
struct PointerEvent;

/**
 * @brief A pane's frame: the skin's window background around a table, over
 * the screen. What the Stage opens a pane in; the Ui keeps it above the
 * content, and a modal one takes every tap and dims what is beneath.
 *
 * Internal (7.7): it took the place of `gui::Window`, of which a pane used
 * only the frame. No title, no close button, no dragging or resizing: the
 * Stage places a pane.
 */
class PaneFrame final : public Widget
{
    public:
        static constexpr ActorInterfaceId actor_interface_id = 0x50'41'4E'45U;

        [[nodiscard]] void *actor_interface( ActorInterfaceId id ) noexcept override
        {
            return id == actor_interface_id ? this : Widget::actor_interface( id );
        }

        [[nodiscard]] const void *actor_interface( ActorInterfaceId id ) const noexcept override
        {
            return id == actor_interface_id ? this : Widget::actor_interface( id );
        }

        /**
         * @brief A frame drawn with the skin's window style `style`, which
         * must outlive it (a literal does); a style the skin lacks is its
         * `default`.
         */
        explicit PaneFrame( std::string_view style = "pane" );

        /** @brief The table inside the frame: the pane's content. */
        [[nodiscard]] Table &content_table() noexcept { return *content_; }
        [[nodiscard]] const Table &content_table() const noexcept { return *content_; }

        /** @brief The skin's window style it is drawn with. */
        [[nodiscard]] std::string_view style() const noexcept { return style_; }

        /** @brief Modal: it takes every tap, and the Ui dims what is beneath it. */
        void set_modal( bool modal ) noexcept { modal_ = modal; }
        [[nodiscard]] bool modal() const noexcept { return modal_; }

        /** @brief The content's, and the style's insets around it. */
        [[nodiscard]] Size minimum_size( Painter &painter, const Skin &skin ) const override;
        [[nodiscard]] Size preferred_size( Painter &painter, const Skin &skin ) const override;

        /** @brief The content inside the style's insets. */
        void layout( Painter &painter, const Skin &skin ) override;

        /** @brief The style's background, behind the content. */
        void paint( Painter &painter, const Skin &skin, float x, float y ) const override;

        /** @brief A tap on the frame stays there; a modal frame takes any. */
        bool pointer_event( const PointerEvent &event ) override;

    private:
        std::string_view style_;
        Table *content_ = nullptr;
        bool modal_ = false;
};

} // namespace sq::gui
