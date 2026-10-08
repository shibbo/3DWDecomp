#pragma once

/**
 * @brief Action names of one idle behavior of a regular cat (sitting, sleeping...).
 */
class NekoStateWaitParam {
public:
    NekoStateWaitParam(const char* pWaitAction, const char* pStartAction, const char* pEndAction,
                       const char* pSubAction1, const char* pSubAction2);

    const char* mStartAction;  ///< Played once when the behavior starts (optional).
    const char* mEndAction;    ///< Played once when the behavior ends (optional).
    const char* mWaitAction;   ///< Loop played during the behavior.
    const char* mSubAction1;   ///< Optional variation randomly played instead of the loop.
    const char* mSubAction2;   ///< Optional variation randomly played instead of the loop.
};

static_assert(sizeof(NekoStateWaitParam) == 0x28);
