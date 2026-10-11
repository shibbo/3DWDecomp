#pragma once

#include <basis/seadTypes.h>

namespace al {
class IUseAudioKeeper;
class LiveActor;

class FlashingCtrl {
public:
    FlashingCtrl(LiveActor* pActor, bool isHideModel, bool isPlaySe);

    virtual void movement();

    void end();
    bool isNowFlashing() const;
    void updateFlashing();
    void start(s32 time);
    s32 getCurrentInterval() const;
    f32 getFlashingAnimRate() const;
    bool isNowJustFlashed() const;
    bool isFastFlashing() const;
    bool isNowOn() const;
    bool isEnded() const { return mIsEnded; }

private:
    LiveActor* mActor;
    IUseAudioKeeper* mAudioKeeper;
    bool mIsHideModel;
    bool mIsEnded = true;
    bool mIsSlowInterval = false;
    s32 mTimer = 0;
    s32 mFlashingStartTime = 0;
    bool mIsHidden = false;
    bool mIsPlaySe;
};

static_assert(sizeof(FlashingCtrl) == 0x28);

}  // namespace al
