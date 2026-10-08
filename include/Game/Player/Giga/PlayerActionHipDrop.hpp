#pragma once

#include <basis/seadTypes.h>

#include "Player/IUsePlayerActionCancelable.hpp"
#include "Player/IUsePlayerActionEnd.hpp"
#include "Player/IUsePlayerHipDropObserver.hpp"
#include "Player/PlayerAction.hpp"

class IUsePlayerCheckArea;
class IUsePlayerFlag;
class IUsePlayerFlagControl;
struct PlayerActionArg;

/// Hip drop observer that ignores every notification (the action's default).
class PlayerHipDropObserverNull : public IUsePlayerHipDropObserver {
public:
    void notifyOnFloorTrig() override {}
};

/// The player's hip drop action: the spin in the air, the fall and the landing.
class PlayerActionHipDrop : public PlayerAction,
                            public IUsePlayerActionEnd,
                            public IUsePlayerActionCancelable {
    SEAD_RTTI_OVERRIDE(PlayerActionHipDrop, PlayerAction)

public:
    /// The phase of the hip drop.
    enum class EState : u32 {
        None = 0,
        Start = 1,
        Loop = 2,
        LandReady = 3,
        Land = 4,
    };

    PlayerActionHipDrop(const PlayerActionArg* pArg, IUsePlayerFlagControl* pFlagControl,
                        const IUsePlayerFlag* pLandReactionFlag,
                        const IUsePlayerCheckArea* pCheckArea,
                        const IUsePlayerFlag* pFlingPoleDashFlag,
                        const IUsePlayerFlag* pInkLimitFlag);
    ~PlayerActionHipDrop() override;

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;
    bool isEnd() const override;
    bool isPossibleToCancel() const override;

    virtual const char* getStartAnimName() const;
    virtual const char* getLoopAnimName() const;
    virtual const char* getLandAnimName() const;
    virtual const char* getReactionAnimName() const;
    virtual f32 getHipDropSpeed() const;
    virtual f32 getAnimRate() const;
    virtual bool isLandVelocityOffset() const;
    virtual void startLand();

    void exeStart();
    void exeLoop();
    void exeLandReady();
    void exeLand();
    void setState(u32 state);
    bool isFlingPoleDashFlagIsOn() const;
    void shiftHipDropLoop();
    bool isInkLimitFlagIsOn() const;

private:
    bool isLanding() const;
    void pushToFloor();

    const PlayerActionArg* mArg;                 // 0x18
    bool mIsEnd = false;                         // 0x20
    IUsePlayerHipDropObserver* mObserver;        // 0x28
    IUsePlayerFlagControl* mFlagControl;         // 0x30
    const IUsePlayerFlag* mLandReactionFlag;     // 0x38
    const IUsePlayerCheckArea* mCheckArea;       // 0x40
    s32 mLandReadyTimer = 0;                     // 0x48
    s32 mLandReactionCount = 0;                  // 0x4c
    EState mState = EState::None;                // 0x50
    s32 mStep = 0;                               // 0x54
    const IUsePlayerFlag* mFlingPoleDashFlag;    // 0x58
    const IUsePlayerFlag* mInkLimitFlag;         // 0x60
};
