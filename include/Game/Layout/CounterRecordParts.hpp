#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class LayoutInitInfo;
}

class CounterRecordParts : public al::LayoutActor {
public:
    CounterRecordParts(const al::LayoutInitInfo& rInfo, const char* pName,
                       const char* pPartsName, al::LayoutActor* pParent);

    void exeWait();
    void show();
    void hide();
    bool isHide() const;
    void setPaneStringScore(s32 score);
    void setPaneStringTime(s32 time);
};
