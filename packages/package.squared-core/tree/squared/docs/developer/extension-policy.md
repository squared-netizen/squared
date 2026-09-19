# Extension policy

## The rule

Do not add a virtual, a plugin point, a registry, a template parameter, or a
callback hook until a second concrete use exists.

That is priority 4 from the project's priority order, and priority 4 is last
for a reason: every speculative extension point spends priority 1's budget. A
vtable pointer is 8 bytes on every instance. A `std::function` hook is 32 bytes
plus a possible allocation plus an indirect call. A registry is a container
that has to live somewhere and be accounted for. All three are paid by every
user of the type, including the ones who never extend it.

## Why squared can hold that line harder than most libraries

In a normal linked library, refusing extension points is a gamble. If you guess
wrong, the user is stuck: they cannot reach the code, so they fork the project,
wrap it in something awkward, or give up. That fear is why most libraries end
up with more abstraction than they need.

squared vendors. `squared-core` is installed into the project tree, and the
user owns the source. When they need behaviour the API does not expose, they
open the file and change it.

**So the failure mode that makes other libraries over-engineer does not exist
here.** The escape hatch is the source itself, and it is always open. That is
not a fallback for when the design is wrong; it is the design.

The practical consequence: when you are weighing whether to add an extension
point "just in case", the answer here is no more often than it would be
elsewhere, and you can say no with more confidence.

## The test

Add the extension point when one of these is true:

- **A second concrete implementation exists in the tree.** Not is planned, not
  is likely. Exists.
- **The porting layer needs it.** Platform boundaries are the one place where
  a second use is guaranteed by the architecture, so `Painter`,
  `application::Application` and `TextInputService` are interfaces on purpose.
- **It has to vary per instance at runtime.** Editing the source changes the
  behaviour of every instance. If two live objects in the same program need to
  behave differently, source editing cannot express that and a real extension
  point is required. `Drawable` qualifies: one skin holds a colour fill and a
  nine-patch at the same time.

Reasons that are not sufficient:

- "Someone might want to." They can edit the file.
- "For testing." Vendored source is testable by editing it, and the test build
  can compile a different file.
- "For symmetry with the other subsystem." Symmetry is not a use.
- "It is only one virtual." It is one virtual per type, forever, on every
  instance, and it has to be documented and kept working.

## What to do instead

| Instead of | Prefer |
|---|---|
| an interface with one implementation | the concrete type |
| a strategy hierarchy over a closed set | a tagged union, or an enum plus a switch |
| a subclass to vary one behaviour | a parameter |
| a factory | a constructor, or a free function |
| a registry of handlers | an explicit call at the one site that needs it |
| a hook "for later" | a line in this document saying where to edit |

That last row is the one people forget. If a subsystem is expected to be
modified, **say so in its developer page and say which file** — a documented
edit point costs zero bytes and is more discoverable than a hook nobody knew
about.

## What this policy obliges in return

Making source editing the extension mechanism is only honest if the source can
actually be edited. Three obligations follow, and they are not optional:

**The source has to be readable.** This is why one type per file and
self-contained headers matter more here than they would in a linked library. A
user who needs to change how `ScrollPane` clamps its scroll offset should open
`scroll_pane.cpp`, find 53 lines, and understand all of them. Before the split
that same change meant reading a 3,281-line file to find out whether anything
else depended on what they were touching. See [file-layout.md](file-layout.md).

**Internals have to be documented, not just the public API.** Someone editing a
file needs the invariants it maintains, not a description of what its functions
return. That is what this developer tree is for, and it is why every subsystem
page states what the subsystem allocates and what it assumes.

**Files a user may edit must be marked `shared` in the package manifest**, so
that updating `squared-core` reports a conflict instead of silently reverting
their work. A vendored file that is marked `generated` is not editable in
practice, whatever this document says.

## Where the line has held

Worth recording, because these are the decisions this policy is meant to
protect:

- **No Service Locator.** `Ui` receives its `TextInputService` explicitly; every
  subsystem takes its dependencies as parameters. See
  [design-patterns.md](design-patterns.md).
- **ECS is not the mandatory public model.** It is available as a storage
  strategy for simulation work and nothing is built on top of it.
- **No Abstract Factory anywhere.** `Widget::TooltipFactory` is a plain
  callback, not a hierarchy.
- **`actor_cast` publishes one identifier per layer** rather than a type
  registry. It was added when the second concrete use existed — `Group` and
  `gui::Widget` — and not before.

## Where it did not, and what that cost

`Drawable` is a three-kind hierarchy behind a `shared_ptr`. It passes the
per-instance test above, so an extension point is genuinely warranted — but the
set is closed and known, and a tagged union would have carried the same three
kinds in 400 bytes with no vtable, no control block, and no atomic refcount per
copy. The interface was the reflex rather than the decision.

The lesson is narrower than "do not use interfaces": when the test says you
need runtime variation, ask next whether the set of kinds is open or closed. A
closed set does not need virtual dispatch.
