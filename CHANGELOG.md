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
