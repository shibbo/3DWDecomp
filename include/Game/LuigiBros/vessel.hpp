#pragma once

class Emu;

class vessel {
public:
    unsigned int mFlags;
    unsigned int _04;
    unsigned int mState;
    unsigned char _0c[0x10];
    bool mDrcDisplay;
    bool mDrcSensorBar;
    unsigned char _1e[0x12];
    Emu* mpEmulator;
    unsigned char _38[8];
};
static_assert(sizeof(vessel) == 0x40, "Vessel layout");
extern vessel m_vessel;
