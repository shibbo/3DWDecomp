#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class SpotLightPatroller : public al::LiveActor {
public:
    SpotLightPatroller(const char*);
    void setPatrol();
    void setAlert();
    bool isPlayerWatch() const;
    bool isAlert() const;
    void setObserved(bool observed) { mIsObserved = observed; }
private:
    unsigned char _144[0x2d8 - 0x144];
    bool mIsObserved;
    unsigned char _2d9[7];
};
