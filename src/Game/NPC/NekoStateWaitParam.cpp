#include "NPC/NekoStateWaitParam.hpp"

/**
 * @brief Stores the five action names used by the cat's waiting state.
 * @param pAction1 First action name; stored without copying.
 * @param pAction2 Second action name; stored without copying.
 * @param pAction3 Third action name; stored without copying.
 * @param pAction4 Fourth action name; stored without copying.
 * @param pAction5 Fifth action name; stored without copying.
 */
NekoStateWaitParam::NekoStateWaitParam(const char* pAction1, const char* pAction2,
                                     const char* pAction3, const char* pAction4,
                                     const char* pAction5)
    : mStartAction(pAction2), mEndAction(pAction3), mWaitAction(pAction1),
      mSubAction1(pAction4), mSubAction2(pAction5) {}
