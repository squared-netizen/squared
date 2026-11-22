export module sq.tui:test_bench;

export import :event;
export import :input_decoder;
export import :renderer;
export import :screen;
import sq.term;
import std;

export namespace sq::tui
{

/**
 * @brief A terminal program's screen, tested with no terminal: what it
 * draws goes through the Renderer into sqterm's own emulator
 * (`sq::term::Terminal`), and the test reads what a person would see.
 *
 *     sq::tui::Test t{ 80, 24 };
 *     draw_menu( t.screen(), items );
 *     t.show();
 *     CHECK( t.text( 2, 1 ) == "> Open" );
 *     auto keys = t.type( "j" );           // bytes in, through the same decoder
 */
class Test
{
    public:
        Test( std::uint16_t columns, std::uint16_t rows );

        [[nodiscard]] Screen &screen() noexcept { return screen_; }

        /** Renders the screen into the emulator; what was written stays in `output`. */
        void show();
        /** The bytes the last `show` wrote. */
        [[nodiscard]] std::string_view output() const noexcept { return output_; }

        /** What row `y` shows from column `x`, as UTF-8, without trailing blanks. */
        [[nodiscard]] std::string text( std::uint16_t x, std::uint16_t y ) const;
        /** A cell as the emulator holds it. */
        [[nodiscard]] const sq::term::Cell &cell( std::uint16_t x, std::uint16_t y ) const noexcept;
        /** Where the emulator's cursor is, and whether it is shown. */
        [[nodiscard]] std::optional<Point> cursor() const noexcept;

        /** Both the screen and the emulator a new size (a Resize, as a Terminal would see it). */
        void resize( std::uint16_t columns, std::uint16_t rows );

        /** What a terminal sends for `bytes`, decoded, as `Terminal::next` would give it. */
        [[nodiscard]] std::vector<Event> type( std::string_view bytes );

    private:
        Screen screen_;
        Renderer renderer_;
        sq::term::Terminal emulator_;
        InputDecoder decoder_;
        std::string output_;
};

} // namespace sq::tui
