#pragma once
#include "Library/Obj/EffectObjFollowCamera.hpp"

namespace rc {
class EffectObjFollowCameraGame : public al::EffectObjFollowCamera {
public:
    EffectObjFollowCameraGame(const char* pName);
    ~EffectObjFollowCameraGame() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void startAppear() override;
    void startDisappear() override;
    void exeDisasterMode();

private:
    bool mStopInDisasterMode = false;
    bool mIsRaining = false;
    bool mIsSingleMode = false;
    bool mShouldAppear = false;
};
}
