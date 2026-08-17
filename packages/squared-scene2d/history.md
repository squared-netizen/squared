# Squared Scene2D History

## 0.6.0-dev.7

- Dependency alignment only: advanced the exact Graphics2D dependency
  coordinate to `0.6.0-dev.7`. No ABI or behavior change.

## 0.6.0-dev.6

- Expanded Doxygen coverage on actor, group, stage, and input headers.
- Documentation-only release: no ABI or behavior change.

## 0.6.0-dev.5

- Added portable actor input listeners with stable actor-local identifiers.
- Added deterministic capture, target, and bubble dispatch with local pointer
  coordinates, handling, stopping, and cancellation.
- Allowed listener registration and removal during notification through
  callback snapshots.

## 0.6.0-dev.4

- Advanced the exact Graphics2D dependency for selectable texture recovery;
  the portable scene hierarchy API remains unchanged.

## 0.6.0-dev.3

- Advanced the package coordinate to keep exact GUI and Graphics2D dependency versions consistent; the portable hierarchy API remained stable.

## 0.6.0-dev.2

- Separated portable scene contracts from graphics-backend implementation details.

## 0.6.0-dev.1

- Added actor/group ownership, hierarchy, stage updates, and hit testing.
