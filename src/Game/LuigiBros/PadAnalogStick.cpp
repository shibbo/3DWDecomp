#include "LuigiBros/PadAnalogStick.hpp"

/**
 * @brief Tests an analog value using separate press and hold thresholds.
 * @param value Analog value to test.
 * @param heldThreshold Threshold used while the direction is held.
 * @param pressThreshold Threshold used for a new press.
 * @param held Whether the direction is already held.
 * @return One if the applicable threshold is reached, otherwise zero.
 */
int PadAnalogStick::PAD_ConvertAD(int value, int heldThreshold, int pressThreshold, int held) const {
    return held ? value >= heldThreshold : value >= pressThreshold;
}

/**
 * @brief Converts stick axes into direction buttons with hysteresis.
 * @param x Horizontal stick position.
 * @param y Vertical stick position.
 * @return Direction button mask.
 */
unsigned int PadAnalogStick::PAD_Analog(float x, float y) {
    unsigned int heldButtons = mButtons;
    int horizontal = static_cast<int>(x * 128.0f);
    unsigned int buttons = PAD_ConvertAD(-horizontal, 30, 70, heldButtons & 1);
    if (PAD_ConvertAD(horizontal, 30, 70, heldButtons & 2)) {
        buttons |= 2;
    }
    int vertical = static_cast<int>(y * 128.0f);
    if (PAD_ConvertAD(vertical, 30, 70, heldButtons & 8)) {
        buttons |= 8;
    }
    if (PAD_ConvertAD(-vertical, 30, 70, heldButtons & 4)) {
        buttons |= 4;
    }
    mButtons = buttons;
    return mButtons;
}

/** @brief Tracks direction changes and decays the rotation counter. @return Whether the direction changed. */
bool PadAnalogStick::IsMoveing() {
    int direction;
    switch (mButtons) {
    case 1: direction = 6; break;
    case 2: direction = 2; break;
    case 4: direction = 4; break;
    case 5: direction = 5; break;
    case 6: direction = 3; break;
    case 8: direction = 0; break;
    case 9: direction = 7; break;
    case 10: direction = 1; break;
    default: direction = 0; break;
    }
    bool changed = false;
    if (mDirection != direction) {
        if (mDirection + 1 == direction || (direction == 0 && mDirection == 7)) {
            mRotation = 20;
        } else if (mDirection - 1 == direction || (direction == 7 && mDirection == 0)) {
            mRotation = -20;
        }
        changed = true;
    }
    mDirection = direction;
    if (mRotation != 0) {
        if (mRotation > 0) {
            --mRotation;
        } else {
            ++mRotation;
        }
    }
    return changed;
}

/**
 * @brief Resolves opposing direction buttons and optionally suppresses diagonals.
 * @param buttons Direction button mask.
 * @param removeVertical Whether horizontal input suppresses vertical input.
 * @return Filtered button mask.
 */
unsigned char moveButtonProhibition(unsigned char buttons, bool removeVertical) {
    if ((buttons & 3) == 3) {
        buttons ^= 2;
    }
    if ((buttons & 12) == 12) {
        buttons ^= 4;
    }
    if (removeVertical) {
        if (buttons & 1) {
            if (buttons & 8) {
                buttons ^= 8;
            }
            if (buttons & 4) {
                buttons ^= 4;
            }
        }
        if (buttons & 2) {
            if (buttons & 8) {
                buttons ^= 8;
            }
            if (buttons & 4) {
                buttons ^= 4;
            }
        }
    }
    return buttons;
}
