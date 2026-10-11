#pragma once

#include <basis/seadTypes.h>

#include "Library/Nerve/NerveExecutor.hpp"

namespace al {
class LiveActor;
class AnimScaleController;
struct AnimScaleParam;
}  // namespace al

/** @brief Makes an actor wobble (scale vibration) while the microphone picks up input. */
class ActorMicRumbler : public al::NerveExecutor {
public:
    ActorMicRumbler(al::LiveActor* pActor, const al::AnimScaleParam* pParam);

    void update();
    void stopAndReset();
    void exeWait();
    void exeVibration();

private:
    al::LiveActor* mActor;
    al::AnimScaleController* mAnimScaleController = nullptr;
    const al::AnimScaleParam* mParam;
    s32 mSilentFrames = 0;
};

static_assert(sizeof(ActorMicRumbler) == 0x30);
