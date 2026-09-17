# Changelog

All notable changes to this project are documented in this file.

The format is based on [Keep a Changelog](https://keepachangelog.com/en/1.1.0/).

## [Unreleased]

### Added

- macOS (arm64) platform support with a native Metal rendering backend under
  `vendor/libp3d/pddi/metal/`, selected via `RC_PLATFORM_MACOS`.
- Metal implementation of the full `pddi` interface: vertex/index buffers,
  RGBA8 textures and R16UI PSX VRAM textures, a reversed-Z depth buffer,
  MSL shaders compiled at runtime, off-screen render targets
  (RGBA8/RGBA16F/Depth32F/R32Uint), cascaded shadow maps, an ImGui/Metal
  overlay and GLFW gamepad support.
- Runtime MSL ports of the 3D PSX texture shader (CLUT/palette decoding,
  magenta/zero-texel keying) and the title/movie effect shaders (`tilt`,
  `glow`, `godrays`, `dot`, `moviedenoise`, `movieupscale`, `moviesharp`).
- GLFW Cocoa backend sources wired into the macOS build.
- `scripts/build_macos.sh` to generate makefiles and build on macOS.
- Controller button-prompt style for **PlayStation/DualSense** (`ps`): a
  PlayStation glyph sheet (`controller_sheet_ps.png`) and controller overlay
  (`controller_overlay_ps.png`), selectable in Controller Settings alongside the
  existing Xbox (default) and Nintendo Switch styles. This is additive: the
  default style and existing behavior are unchanged.
- DualSense/DualShock 4 **light bar** support (macOS, via the GameController
  framework): the colour tracks Jackie's health (green → yellow → red, pulsing
  when critical) and flashes red whenever the player lands a hit on an enemy.
  Exposed through a new optional `pddiGamepad::SetLight`, which is a no-op on
  backends without support (OpenGL/Windows/Linux keep working unchanged).
- Controller settings for the above: a **Vibration** strength slider (0–100), a
  **Lightbar** on/off toggle and the existing **Shock** toggle. They are
  disabled unless a rumble-capable DualSense/DualShock is connected.
- Light bar on **Windows/Linux** (OpenGL backend): raw-HID implementation in
  `pddi/gl/glsonyhid.{h,cpp}` (SetupAPI/hid.dll on Windows, `/sys/class/hidraw`
  on Linux), mirroring rumble into the same effects report. USB only.
- **Brazilian Portuguese** localization (`pt` language, `res/pc/text/portuguese.txt`).

### Changed

- Vibration strength is now scaled by the new vibration setting, and the light
  bar flashes on damage in addition to landing hits.

### Changed

- `premake5.lua` and `vendor/libp3d/premake5.lua`: added a `system:macosx`
  build configuration (arm64, required frameworks, Cocoa GLFW sources). The
  OpenGL backend is excluded on macOS and the Metal backend is excluded on
  every other platform.
- `src/pc/crashreporter.cpp`: the POSIX crash handler now reports the correct
  platform label on macOS, and `/proc/self/maps` and `/etc/os-release` are
  only read on Linux.

### Notes

- Windows (x86_64) and Linux (x86_64) builds are unchanged and keep using the
  OpenGL backend.
