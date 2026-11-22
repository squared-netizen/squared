# squared: to do

Small items decided but not yet designed or built. Each grows a design
document in `docs/design/` when its turn comes.

## A settings screen, prebuilt (added 2026-11-22)

A standard form for preferences, shipped with squared as a screen an app
adds rather than writes.

- **What it does for every app, by default**
  - Import and export preferences, stored in sq cartridges (`.sq`).
  - Load fonts (TTF, and bitmap `.fnt`) and skins, and choose among those
    loaded.
- **What an app adds**: its own, domain-specific preferences, attached to
  the screen as a tree.
  - The top of each branch is a text view that names the setting; its
    tooltip describes it.
  - Expanded, the branch shows each preference with the widget that fits
    its type (a switch for a flag, a field for text, a slider or a number
    field for a range, a list for a choice, and so on).
- Later: the same form for sqterm's settings (fonts, colours, the keyboard
  pane), as its first user.
