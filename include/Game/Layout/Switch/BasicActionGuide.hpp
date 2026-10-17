#pragma once

#include "Library/Layout/LayoutActor.hpp"

namespace al {
class IUseAudioKeeper;
class LayoutInitInfo;
}  // namespace al

class BasicActionGuide : public al::LayoutActor {
public:
    BasicActionGuide(const al::LayoutInitInfo& rInfo, const char* pName, const char* pPartsName,
                     al::LayoutActor* pParent);

    void setControllerPort(s32 port);
    void startIn(bool isImmediate);
    void startOut(bool isImmediate);
    void end();
    bool isWait();
    void resetScrollLocation();
    void exeAppear();
    void exeWait();
    void exeEnd();

private:
    s32 mControllerPort;
    f32 mScrollRate;
    f32 mScrollHeight;
    al::IUseAudioKeeper* mAudioKeeper;
};
