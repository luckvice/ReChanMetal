// controllerlight.h — drives an RGB light bar (DualSense / DualShock 4).
//
// The base colour tracks Jackie's health (green -> yellow -> red, pulsing when
// critical) and flashes red whenever the player lands a hit on an enemy.
#pragma once

#include "core.h"

namespace ControllerLight {
    // Call when the player successfully hits an enemy.
    void FlashHit();
    // Call when the player takes damage.
    void FlashDamage();

    // Call once per frame; pushes the current colour to the gamepad light bar.
    void Update();
}
