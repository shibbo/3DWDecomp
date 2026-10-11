#pragma once

#include "Library/Nerve/NerveStateBase.hpp"

namespace al {
class LiveActor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
}  // namespace al

/// Shared state that lets players stroke (pet) an actor with the touch screen.
class ActorStateSupportStroke : public al::NerveStateBase {
public:
    ActorStateSupportStroke(al::LiveActor* pActor);

    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    bool isTouch() const;

    /** @return Whether the actor was stroked this frame. */
    bool isTrigStroke() const { return mIsTrigStroke; }

    /** @brief Clears both stroke option flags. */
    void resetFlags() {
        _3e = false;
        _3f = false;
    }

private:
    u8 _11[0x3d - 0x11];
    bool mIsTrigStroke;  // 0x3d
    bool _3e;
    bool _3f;
};

static_assert(sizeof(ActorStateSupportStroke) == 0x40);
