#pragma once

#include "Library/Nerve/IUseNerve.hpp"
#include "Project/Audio/IUseAudioKeeper.hpp"

class StageTimer;
class StageDataHolder;
namespace al {
class AudioDirector;
}  // namespace al

/** @brief Converts the remaining stage time into team score after the goal. */
class ResultTimerCount : public al::IUseNerve, public al::IUseAudioKeeper {
public:
    ResultTimerCount(StageTimer* pTimer, StageDataHolder* pStageData, al::AudioDirector* pAudio);
    void update();
    void appear();
    void start();
    bool isWait() const;
    void exeWait();
    void exeTimerCount();
    void exeEndWait();
    /** @brief Returns the state controller.
     * @return Owned nerve keeper. */
    al::NerveKeeper* getNerveKeeper() const override { return mNerveKeeper; }
    /** @brief Returns the countdown audio controller.
     * @return Owned audio keeper. */
    al::AudioKeeper* getAudioKeeper() const override { return mAudioKeeper; }

private:
    al::NerveKeeper* mNerveKeeper = nullptr;
    StageTimer* mTimer;
    StageDataHolder* mStageData;
    int mInitialCount = 0;
    al::AudioKeeper* mAudioKeeper = nullptr;
};

static_assert(sizeof(ResultTimerCount) == 0x38);
