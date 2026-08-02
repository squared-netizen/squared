---
title: Squared Application
tags:
  - application
  - architecture
  - cpp
---

# Squared Application

Squared Application is a header-only module containing the platform-neutral
application lifecycle and event contracts.

`squared::application::Application` is implemented by developer application
code. A platform adapter owns operating-system and SDL integration, translates
native events into `squared::application::Event`, and calls the application
lifecycle without exposing SDL types through the public contract.

The interface covers creation, event handling, frame updates, rendering,
pause and resume, resize, graphics-surface availability, disposal, and an
application-requested shutdown query.

The module has no implementation library or third-party dependency. Its CMake
target is `squared_application`, implemented as an `INTERFACE` target that
exports the headers and the C++20 requirement.
