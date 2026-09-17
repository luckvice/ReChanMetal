// glsonyhid.h — raw-HID helper for Sony controllers (DualSense/DualShock 4)
// used by the OpenGL backend on Windows and Linux to drive the lightbar (and to
// send rumble alongside it, so the shared output report does not cancel it).
//
// SDL2 exposes no LED API, so the GL backend talks to the controller directly.
// The Metal backend has its own macOS implementation.
#pragma once

namespace glsonyhid {
    // True if a supported Sony controller is currently connected and openable.
    bool EnsureDevice();

    // Sends the DualSense/DualShock USB effects report: rumble motors plus the
    // lightbar colour (0..255 each). Returns false if no device is available.
    bool Send(unsigned char weak, unsigned char strong,
              unsigned char r, unsigned char g, unsigned char b);

    // Closes the device handle (call from the gamepad destructor/shutdown).
    void Release();
}
