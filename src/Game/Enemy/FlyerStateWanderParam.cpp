#include "Enemy/FlyerStateWanderParam.hpp"

/** @brief Creates movement and timing settings for a flying enemy's wandering state.
 * @param stepRandomRange Random duration increment, multiplied by a value from zero to two.
 * @param stepWander Minimum wandering duration.
 * @param stepWait Minimum waiting duration.
 * @param pAction Wandering animation name.
 * @param pMoveParam Movement acceleration, gravity, friction, and turning settings.
 */
FlyerStateWanderParam::FlyerStateWanderParam(int stepRandomRange, int stepWander, int stepWait,
        const char* pAction, const al::ActorParamMove* pMoveParam)
    : mStepRandomRange(stepRandomRange), mStepWander(stepWander), mStepWait(stepWait),
      mAction(pAction), mMoveParam(pMoveParam) {}
