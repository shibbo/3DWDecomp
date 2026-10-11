#pragma once

#include <basis/seadTypes.h>

class IUsePlayerEventReceiver;
class IUsePlayerLandingObserver;
class PlayerActionGraph;
class PlayerLandingChecker;
class PlayerTrigger;

/// Tells the registered observers when the player lands.
class PlayerLandingInformer {
public:
    PlayerLandingInformer();
    void appendObserver(IUsePlayerLandingObserver* pObserver);
    void update();

    void setLandingChecker(const PlayerLandingChecker* pChecker) { mLandingChecker = pChecker; }

    void setTrigger(const PlayerTrigger* pTrigger) { mTrigger = pTrigger; }

    void setActionGraph(const PlayerActionGraph* pActionGraph) { mActionGraph = pActionGraph; }

    void setEventReceiver(IUsePlayerEventReceiver* pReceiver) { mEventReceiver = pReceiver; }

private:
    void* mObservers;  // 0x0
    const PlayerLandingChecker* mLandingChecker;  // 0x8
    const PlayerTrigger* mTrigger;  // 0x10
    const PlayerActionGraph* mActionGraph;  // 0x18
    IUsePlayerEventReceiver* mEventReceiver;  // 0x20
};
static_assert(sizeof(PlayerLandingInformer) == 0x28);
