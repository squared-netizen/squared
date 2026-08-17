# Squared Application History

## 0.6.0-dev.5

- Expanded Doxygen coverage on the application lifecycle and event boundary headers, including modifier, key, text-input, and navigation event contracts.
- Documentation-only release: no ABI or behavior change.

## 0.6.0-dev.4

- Added backend-neutral Shift, Control, Alt, and Meta modifier state.
- Added semantic navigation events for directional movement, traversal,
  activation, and cancellation from keyboards or controllers.
- Expanded the portable key set with Up, Down, and Space.

## 0.6.0-dev.3

- Defined ordered pause, graphics-surface loss, restoration, resize, resume, and disposal responsibilities.
- Kept context recovery behind existing portable lifecycle callbacks without backend coupling.

## 0.6.0-dev.2

- Added portable key-down, key-up, committed-text, and IME-composition events.
- Added the platform-neutral `TextInputService` soft-keyboard boundary.
- Added optional service injection through `Application` without SDL or Android coupling.

## 0.6.0-dev.1

- Established the portable application lifecycle and event boundary.
