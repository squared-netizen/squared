# namespace gui

## Namespaces

* [detail](detail/index.md)


## Records

* [ActorRoot](ActorRoot.md)
* [Actor](Actor.md)
* [BatchPainter](BatchPainter.md)
* [ButtonGroup](ButtonGroup.md)
* [ButtonStyle](ButtonStyle.md)
* [Button](Button.md)
* [Cell](Cell.md)
* [CheckBoxStyle](CheckBoxStyle.md)
* [CheckBox](CheckBox.md)
* [ColorDrawable](ColorDrawable.md)
* [Drawable](Drawable.md)
* [FileProblem](FileProblem.md)
* [FontResource](FontResource.md)
* [Group](Group.md)
* [Image](Image.md)
* [InputEvent](InputEvent.md)
* [Insets](Insets.md)
* [LabelStyle](LabelStyle.md)
* [Label](Label.md)
* [LayoutCheck](LayoutCheck.md)
* [LayoutHooks](LayoutHooks.md)
* [LayoutNode](LayoutNode.md)
* [LayoutTarget](LayoutTarget.md)
* [LayoutWriter](LayoutWriter.md)
* [Layout](Layout.md)
* [LinearLayout](LinearLayout.md)
* [List](List.md)
* [MarginContainer](MarginContainer.md)
* [NameEntry](NameEntry.md)
* [NameSchema](NameSchema.md)
* [NinePatchDrawable](NinePatchDrawable.md)
* [NinePatchSplits](NinePatchSplits.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeTypeOf](NodeTypeOf.md)
* [NodeWriter](NodeWriter.md)
* [Painter](Painter.md)
* [PaneFrame](PaneFrame.md)
* [PaneShape](PaneShape.md)
* [PanelStyle](PanelStyle.md)
* [Panel](Panel.md)
* [PointerEvent](PointerEvent.md)
* [ProgressBarStyle](ProgressBarStyle.md)
* [ProgressBar](ProgressBar.md)
* [RadioButton](RadioButton.md)
* [Rectangle](Rectangle.md)
* [RegionDrawable](RegionDrawable.md)
* [ScrollPane](ScrollPane.md)
* [Separator](Separator.md)
* [SizeHints](SizeHints.md)
* [Size](Size.md)
* [SkinLoadIssue](SkinLoadIssue.md)
* [SkinLoadLimits](SkinLoadLimits.md)
* [SkinLoadReport](SkinLoadReport.md)
* [Skin](Skin.md)
* [SliderStyle](SliderStyle.md)
* [Slider](Slider.md)
* [Stack](Stack.md)
* [Table](Table.md)
* [Tap](Tap.md)
* [TextFieldStyle](TextFieldStyle.md)
* [TextField](TextField.md)
* [ToggleButton](ToggleButton.md)
* [TooltipConfig](TooltipConfig.md)
* [Ui](Ui.md)
* [Widget](Widget.md)
* [WindowStyle](WindowStyle.md)


## Functions

### actor_cast

*Interface * actor_cast(Actor * actor)*

*Defined at [gui/actor.hpp:224](../../../../gui/actor.hpp#L224)*



**brief** Downcast an actor to a derived interface without RTTI.

**Interface** Type publishing a static `actor_interface_id` constant and overriding Actor::actor_interface().

**actor** Actor to inspect; null is accepted and yields null.

**return** Pointer to the interface, or null when the actor does not publish it.

**note** One virtual call. This replaces dynamic_cast on every actor and GUI traversal, so the framework builds with `-fno-rtti`.

### actor_cast

*const Interface * actor_cast(const Actor * actor)*

*Defined at [gui/actor.hpp:239](../../../../gui/actor.hpp#L239)*



**brief** Downcast a read-only actor to a derived interface without RTTI.

**Interface** Type publishing a static `actor_interface_id` constant.

**actor** Actor to inspect; null is accepted and yields null.

**return** Pointer to the interface, or null when the actor does not publish it.

### check

*LayoutCheck check(const Layout & layout)*



**brief** Whether a layout can be built: its tree, text and numbers hold together, and every node takes what it is given. `instantiate` checks first, so a bad layout builds nothing.

### edits_value

*bool edits_value(NodeType type)*

*Defined at [gui/layout.hpp:166](../../../../gui/layout.hpp#L166)*



**brief** Whether a node of this type edits the value it shows: a check box, slider or text field. 

### holds_nodes

*bool holds_nodes(NodeType type)*

*Defined at [gui/layout.hpp:137](../../../../gui/layout.hpp#L137)*



**brief** Whether nodes go inside a node of this type: a table, a scroll (one), a stack, or a list (one: its row).

### instantiate

*LayoutCheck instantiate(const Layout & layout, Table & root, const Skin & skin, const LayoutHooks & hooks)*



**brief** Build a layout's widgets into `root`: the one function that makes widgets from nodes, whichever avenue wrote them (a compiled layout file, or a C++ screen's `LayoutWriter`).

 The layout is checked first (`check`): a layout that fails builds nothing. Otherwise every node is built, and the result names the first thing that built short (a custom type no hook makes, shown as a label saying so; an image whose drawable the skin lacks). The layout need not outlive this call: a tap's line is copied into its button.

 Allocates: each widget, its words, and each tap's line, once, here; nothing while it runs. `root` should be empty.

### load_libgdx_skin

*bool load_libgdx_skin(Skin & destination, std::string_view json, const SkinDrawableResolver & resolver, SkinLoadReport & report, const SkinLoadLimits & limits)*



**brief** Load a skin while retaining fonts as descriptor-only resources.

 This compatibility overload preserves existing Painter implementations. Font-aware applications should provide SkinFontResolver through the full overload.

**destination** Skin replaced only after a successful complete import.

**json** Complete relaxed libGDX skin JSON document.

**resolver** Callback resolving drawable resource names.

**report** Diagnostics and resource counts, replaced for this call.

**limits** Explicit byte, depth, resource, and name bounds.

**return** true when the skin committed; false with destination unchanged.

### load_libgdx_skin

*bool load_libgdx_skin(Skin & destination, std::string_view json, const SkinDrawableResolver & resolver, const SkinFontResolver & font_resolver, SkinLoadReport & report, const SkinLoadLimits & limits)*



**brief** Load the supported libGDX skin subset transactionally from memory.

 The relaxed libGDX JSON dialect is normalized under explicit limits. The destination is replaced only when every supported style validates and every referenced drawable resolves.

**destination** Skin receiving the loaded colors, drawables, and styles; untouched byte-for-byte when the load fails.

**json** Complete skin JSON document in the relaxed libGDX dialect.

**resolver** Callback resolving every referenced resource name.

**font_resolver** Callback resolving declared bitmap-font resources.

**report** Diagnostics and counted resources collected during loading.

**limits** Explicit resource limits applied to the document.

**return** true when the load committed; false when any error occurred.

**note** Threading: caller must own both the Skin and the resolver.

### node_numbers

*std::size_t node_numbers(NodeType type)*

*Defined at [gui/layout.hpp:195](../../../../gui/layout.hpp#L195)*



**brief** How many numbers a node of this type takes: a slider 3, a progress bar 2. 

### node_type_name

*std::string_view node_type_name(NodeType type)*



**brief** A type's name in a layout file: `check_box`. 

### node_type_named

*std::optional<NodeType> node_type_named(std::string_view name)*



**brief** The type a layout file names; nothing for a name that is not one. 

### operator==

*bool operator==(const LayoutCheck & , const LayoutCheck & )*

*Defined at [gui/layout.hpp:344](../../../../gui/layout.hpp#L344)*

### operator==

*bool operator==(const LayoutNode & , const LayoutNode & )*

*Defined at [gui/layout.hpp:128](../../../../gui/layout.hpp#L128)*

### operator==

*bool operator==(const PaneShape & , const PaneShape & )*

*Defined at [gui/layout.hpp:73](../../../../gui/layout.hpp#L73)*

### pane_place_name

*std::string_view pane_place_name(PanePlace place)*



**brief** A place's name in a layout file: `left`. 

### pane_place_named

*std::optional<PanePlace> pane_place_named(std::string_view name)*



**brief** The place a layout file names; nothing for a name that is not one. 

### problem_text

*std::string_view problem_text(LayoutProblem problem)*



**brief** A problem in words, for a log line. 

### read_json

*bool read_json(std::string_view file, std::string_view text, data::JsonValue & out, FileProblems & problems)*



**brief** Parse `text`, the JSON file `file`, into `out`. False, with the problem added, when it is not JSON (its line and column).

### read_layout

*bool read_layout(std::string_view file, const data::JsonValue & root, const NameSchema & schema, std::span<const LayoutTarget> targets, LayoutWriter & out, FileProblems & problems)*



**brief** Read a layout file into `out`, checking every node: its keys, their values, binds against `schema` (the address exists, carries `state`, and has a type the node can show), taps (a telegram whose address, verb and value type the schema allows; a navigation to one of `targets` or a screen `schema` lists). False when anything was wrong; each problem is added, and what `out` holds is then not to be used.

### read_layout_kind

*LayoutKind read_layout_kind(std::string_view file, const data::JsonValue & root, FileProblems & problems)*



**brief** A layout file's kind (`"kind": "screen"` or `"pane"`; screen when left out), read first: every file's kind is needed to check navigation. When the kind is neither, or the file is not an object, the problem is added and the kind given is screen.

### read_names

*bool read_names(std::string_view file, const data::JsonValue & root, NameSchema & schema, FileProblems & problems)*



**brief** Read `names.json` into `schema`. A list (`{ "list": "item" }`) is its rows' key (an integer `state` taking a uid) and `<name>.count`. False when anything was wrong; each problem is added.

### read_tap

*Tap read_tap(std::string_view line)*



**brief** Read a tap's line. A telegram is checked here only for its verb, `set` or `do`; the build checks the rest against `names.json`, and `messaging::read_telegram` reads it when it is tapped.

### resolve_atlas_drawable

*DrawablePtr resolve_atlas_drawable(const graphics::TextureAtlas & atlas, std::string_view resource_name)*



**brief** Create a region or nine-patch drawable for one loaded atlas region.

**atlas** Atlas owning the requested region; must outlive the result.

**resource_name** Name and optional index of the atlas region.

**return** Drawable referencing the atlas region, or an empty pointer when the region is absent or cannot be converted.

### shows_type

*bool shows_type(NodeType type, messaging::ValueType value)*

*Defined at [gui/layout.hpp:176](../../../../gui/layout.hpp#L176)*



**brief** Whether a node of this type can show a value of this type: a label any but uids, a check box a bool, a text field text, a slider or progress bar a number or an integer.

### shows_value

*bool shows_value(NodeType type)*

*Defined at [gui/layout.hpp:151](../../../../gui/layout.hpp#L151)*



**brief** Whether a node of this type shows a value an address gives it (`bind`). 

### tap_word

*std::string_view tap_word(TapKind kind)*



**brief** The word a navigation starts with (`push`); empty for `none`, `telegram`, `invalid`. 

## Enums

| enum class Alignment |

--

| start |
| center |
| end |


*Defined at [gui/src/alignment.hpp:9](../../../../gui/src/alignment.hpp#L9)*



**brief** Per-cell alignment relative to the cell box. 

| enum class Direction |

--

| horizontal |
| vertical |


*Defined at [gui/src/direction.hpp:9](../../../../gui/src/direction.hpp#L9)*



**brief** Main stacking axis of a linear layout. 

| enum class InputPhase |

--

| capture |
| target |
| bubble |


*Defined at [gui/src/input_phase.hpp:9](../../../../gui/src/input_phase.hpp#L9)*



**brief** Dispatch phase of an event inside a Stage. 

| enum class InputType |

--

| pointer_move |
| pointer_down |
| pointer_up |
| pointer_cancel |
| key_down |
| key_up |
| navigation |


*Defined at [gui/src/input_type.hpp:9](../../../../gui/src/input_type.hpp#L9)*



**brief** Event kind carried by one InputEvent. 

| enum class LayoutKind |

--

| screen |
| pane |


*Defined at [gui/layout.hpp:46](../../../../gui/layout.hpp#L46)*



**brief** What a layout is pushed as: a whole screen, or a pane over one. 

| enum class LayoutProblem |

--

| none |
| bad_text |
| bad_tree |
| too_deep |
| not_a_container |
| scroll_holds_one |
| bad_list |
| misplaced_row |
| bad_type |
| bad_bind |
| bad_tap |
| bad_numbers |
| no_custom_name |
| unknown_custom |
| unknown_drawable |


*Defined at [gui/layout.hpp:298](../../../../gui/layout.hpp#L298)*



**brief** What is wrong with a layout, if anything. 

| enum class Navigate |

--

| push |
| replace |
| back |
| open |
| close |
| quit |


*Defined at [gui/layout_writer.hpp:144](../../../../gui/layout_writer.hpp#L144)*



**brief** Where a tap goes, when it navigates rather than sends. 

| enum class NodeType |

--

| label |
| button |
| check_box |
| text_field |
| slider |
| progress_bar |
| image |
| separator |
| table |
| scroll |
| stack |
| list |
| custom |


*Defined at [gui/layout.hpp:20](../../../../gui/layout.hpp#L20)*



**brief** What a layout node builds. A layout file names it as `type` (`"check_box"`); C++ names it through the widget (`add<CheckBox>`).

| enum class PanePlace |

--

| center |
| left |
| right |
| top |
| bottom |


*Defined at [gui/layout.hpp:53](../../../../gui/layout.hpp#L53)*



**brief** Where a pane sits on its screen. `center` at the size it asks for; `left` and `right` the screen's height, `top` and `bottom` its width. Always inside the safe area.

| enum class PointerAction |

--

| move |
| down |
| up |
| cancel |


*Defined at [gui/src/pointer_action.hpp:9](../../../../gui/src/pointer_action.hpp#L9)*



**brief** Pointer action delivered to widgets. 

| enum class SkinLoadSeverity |

--

| warning |
| error |


*Defined at [gui/src/skin_load_severity.hpp:11](../../../../gui/src/skin_load_severity.hpp#L11)*



**brief** Severity of one skin-loading diagnostics entry.

| enum class TapKind |

--

| none |
| telegram |
| push |
| replace |
| back |
| open |
| close |
| quit |
| invalid |


*Defined at [gui/layout.hpp:264](../../../../gui/layout.hpp#L264)*



**brief** What a tap's line does. 

