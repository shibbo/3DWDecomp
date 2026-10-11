#pragma once

#include <basis/seadTypes.h>

namespace al {
class AudioDirector;
}  // namespace al

class StageDataHolder;
class StageTimer;

/**
 * @brief Counts the remaining time of the stage timer into the score after the goal.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ResultTimerCount {
public:
    ResultTimerCount(StageTimer* pStageTimer, StageDataHolder* pStageDataHolder,
                     al::AudioDirector* pAudioDirector);

    void appear();
    void start();
    void update();
    bool isWait() const;

private:
    u8 _0[0x38];
};

static_assert(sizeof(ResultTimerCount) == 0x38);
