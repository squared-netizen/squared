# Squared SDL2/OpenGL Backend History

## 0.6.0-dev.5

- Dependency alignment only: advanced the exact Graphics2D dependency
  coordinate to `0.6.0-dev.7`. No ABI or behavior change.

## 0.6.0-dev.4

- Implemented the portable selectable texture recovery policies without
  exposing SDL2 or OpenGL through public contracts.
- Added synchronous callback uploads and policy-aware atlas page loading.

## 0.6.0-dev.3

- Added SDL GL-context reactivation with replacement fallback and generation tracking.
- Added OpenGL handle validation and reconstruction for restorable Graphics2D resources.
- Added Android background behavior that performs no GL calls after surface suspension.

## 0.6.0-dev.2

- Added libGDX atlas-region loading, rotation normalization, and nine-patch metadata support.
- Kept SDL, OpenGL ES, and platform asset loading behind the link-time backend boundary.

## 0.6.0-dev.1

- Established SDL2/OpenGL implementations for portable Graphics and Graphics2D contracts.
