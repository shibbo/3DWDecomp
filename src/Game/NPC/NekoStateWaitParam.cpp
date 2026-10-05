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
    : mAction2(pAction2), mAction3(pAction3), mAction1(pAction1),
      mAction4(pAction4), mAction5(pAction5) {}
