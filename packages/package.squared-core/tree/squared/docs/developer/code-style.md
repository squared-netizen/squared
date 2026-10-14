# Code style &mdash; internals

How squared's C++ is formatted, where the rules come from, and where they
deliberately differ from their source.

Programmer counterpart: none. Formatting is a maintainer's concern; code that
uses squared can be formatted however its author likes.

## Source

The rules are Cataclysm: Dark Days Ahead's, from its `.astylerc`. CDDA is a
large, long-lived, modern C++ code base, and its style is built for code read
far more often than written: braces always, operators at the end of a broken
line, and padded parentheses that keep a call's arguments visually separate
from its name.

CDDA formats with Artistic Style. squared uses clang-format, because the
Sprint 2 plan already relies on the LLVM toolchain for clang-tidy, and one tool
family is one less thing to install on a phone. `squared/.clang-format` says
the same things in clang-format's terms, one astyle option per comment.

## What it looks like

```cpp
namespace sq::gui
{

class Ui final : public Widget
{
    public:
        [[nodiscard]] Insets safe_area() const noexcept { return safe_area_; }
        void set_safe_area( Insets insets ) noexcept;

    private:
        Widget *content_{ nullptr };
};

void Ui::set_safe_area( Insets insets ) noexcept
{
    if( content_ != nullptr ) {
        content_->invalidate_layout();
    }
    for( const auto &[key, value] : classes ) {
        switch( value.kind() ) {
            case 1:
                break;
            default:
                return;
        }
    }
}

} // namespace sq::gui
```

## The rules, by CDDA option

| CDDA (astyle) | Effect | clang-format |
|---|---|---|
| `--style=1tbs` | braces on their own line for namespaces, classes and functions; attached for control statements and enums | `BraceWrapping` |
| `--add-brackets` | every `if`, `for` and `while` body has braces | `InsertBraces` |
| `--indent=spaces=4`, `--convert-tabs` | four spaces, no tabs | `IndentWidth`, `UseTab` |
| `--max-code-length=100` | 100 columns | `ColumnLimit` |
| `--align-pointer=name` | `char *foo`, `const T &x` | `PointerAlignment`, `ReferenceAlignment` |
| `--indent-classes` | `public:` one level in, members two | `IndentAccessModifiers` |
| `--indent-switches` | `case` one level inside `switch` | `IndentCaseLabels` |
| `--break-after-logical`, `--pad-oper` | a broken condition ends its line with `&&`; spaces around binary operators | `BreakBeforeBinaryOperators: None` |
| `--unpad-paren`, `--pad-paren-in` | `if( x )`, `f( a, b )`, `f()` | `SpaceBeforeParens: Never`, `SpacesInParens` |
| `--min-conditional-indent=0` | a wrapped condition lines up with its open paren | `AlignAfterOpenBracket` |

## Where squared differs

**`--attach-inlines` has no clang-format equivalent.** CDDA keeps the brace of
a function defined inside a class on the same line; clang-format has one
setting for every function definition. A body short enough for one line stays
on one line inside a class (`AllowShortFunctionsOnASingleLine: InlineOnly`),
which covers accessors; a longer one gets its brace on its own line. That is
also how squared's code already read before the format.

**`--indent-preprocessor` is not reproduced.** clang-format leaves the
continuation lines of a multi-line `#define` alone. squared has few, and none
in a header a user reads.

**Includes are sorted and grouped.** astyle never reorders includes, so CDDA
says nothing about it. squared regroups them: the file's local `"detail/..."`
headers, then `<squared/...>`, then platform and third-party C headers, then
the standard library, alphabetical within each group. This is safe only because
every squared header is self-contained (`tools/check_headers.sh`): no header
depends on another having been included first, so order cannot change meaning.
A file whose includes really must stay in order &mdash; a third-party
implementation file with `#define`s between its includes &mdash; keeps it,
because sorting stops at anything that is not an `#include` line.

**Comments are not reflowed.** astyle does not reflow them either; clang-format
would, and would break the tables and lists in doxygen comments. A comment over
100 columns stays over.

**Additions with no CDDA counterpart**, chosen to fit it: closing namespace
comments, braced lists padded like parentheses (`{ a, b }`), constructor
initialisers one per line with the comma leading, `template<` with no space.

## Running it

```sh
squared/tools/format.sh --check   # list files that would change; exit 1 if any
squared/tools/format.sh           # format in place
```

It formats the framework only. A generated project's own code (`sq_app/`,
`sq_android/`) is the project's: it has the template's `.clang-format`, and the
project decides its own style. The tool refuses to run on a tree with
uncommitted changes unless given `--force`, so a formatting commit holds
formatting and nothing else.

`third_party/` is never touched. Neither is any file between
`// clang-format off` and `// clang-format on`.

## Cost

None at run time: formatting changes no generated code. It is the one change
in Sprint 2 that the priority order has nothing to say about.
