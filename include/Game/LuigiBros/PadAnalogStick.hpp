#pragma once

class PadAnalogStick {
public:
    /** @brief Initializes direction history and held buttons. */
    PadAnalogStick() { mButtons = 0; mDirection = 0; }
    int PAD_ConvertAD(int value, int heldThreshold, int pressThreshold, int held) const;
    unsigned int PAD_Analog(float x, float y);
    bool IsMoveing();

private:
    int mDirection;
    short mRotation;
    unsigned int mButtons;
};

unsigned char moveButtonProhibition(unsigned char buttons, bool removeVertical);
