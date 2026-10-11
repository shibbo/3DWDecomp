#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class AssistSlideMapPartsGroup;
class AssistSlideMapParts : public al::LiveActor {
public:
    AssistSlideMapParts(const char*);
    void setGroupHost(AssistSlideMapPartsGroup*);
    void touchAssist();
private:
    unsigned char _144[0x94];
};
static_assert(sizeof(AssistSlideMapParts) == 0x1d8);
