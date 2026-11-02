# namespace detail

## Records

* [FileWriting](FileWriting.md)


## Functions

### bounds_of

*Rectangle bounds_of(const Widget & widget, float x, float y)*

*Defined at [gui/src/detail/gui_detail.cppm:26](../../../../../gui/src/detail/gui_detail.cppm#L26)*

### build_row

*LayoutCheck build_row(const Layout & row, Table & into, const Skin & skin, const LayoutHooks & hooks)*



**brief** Build `row`, a list's row as a layout, into `into`, with `hooks`: `instantiate` without its check, which the layout the row came from has passed already (a row on its own would fail it: its uids need a list).

### clamp_dimension

*float clamp_dimension(float value, float minimum, float maximum)*

*Defined at [gui/src/detail/gui_detail.cppm:31](../../../../../gui/src/detail/gui_detail.cppm#L31)*

### multiply

*graphics::Color multiply(graphics::Color left, graphics::Color right)*

*Defined at [gui/src/detail/gui_detail.cppm:20](../../../../../gui/src/detail/gui_detail.cppm#L20)*

### next_codepoint

*std::size_t next_codepoint(const std::string & text, std::size_t cursor)*

*Defined at [gui/src/detail/gui_detail.cppm:69](../../../../../gui/src/detail/gui_detail.cppm#L69)*

### previous_codepoint

*std::size_t previous_codepoint(const std::string & text, std::size_t cursor)*

*Defined at [gui/src/detail/gui_detail.cppm:57](../../../../../gui/src/detail/gui_detail.cppm#L57)*

### style_or_default

*const typename Map::mapped_type & style_or_default(const Map & styles, std::string_view name)*

*Defined at [gui/src/detail/gui_detail.cppm:83](../../../../../gui/src/detail/gui_detail.cppm#L83)*

### tap_verb

*Verb tap_verb()*

*Defined at [gui/layout_writer.cppm:125](../../../../../gui/layout_writer.cppm#L125)*

### valid_font_path

*bool valid_font_path(std::string_view path)*

*Defined at [gui/src/detail/gui_detail.cppm:36](../../../../../gui/src/detail/gui_detail.cppm#L36)*

