#pragma once

#include "Player/PlayerAction.hpp"

struct PlayerActionAirMoveArg;
struct PlayerActionArg;
class PlayerContinuousJump;
class IUsePlayerHorizontalSpeedAverage;
class IUsePlayerActualMove;
class IUsePlayerFlag;

class CourseSelectPlayerActionJump : public PlayerAction {
    SEAD_RTTI_OVERRIDE(CourseSelectPlayerActionJump, PlayerAction)

public:
    CourseSelectPlayerActionJump(PlayerActionAirMoveArg*, PlayerContinuousJump*,
                                const IUsePlayerHorizontalSpeedAverage*,
                                const IUsePlayerActualMove*, const IUsePlayerFlag*);
    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;
    virtual int getJumpPowCountMax() const;
    virtual float getDashJumpAddition() const;
    virtual float getJumpPow() const;
    virtual float getJumpPowLow() const;
    void updateSound();

private:
    const PlayerActionArg* mArg;
    bool mJumpButtonHeld = false;
    int mJumpButtonFrames = 0;
    int mSoundTimer = 0;
    PlayerContinuousJump* mContinuousJump;
    const IUsePlayerHorizontalSpeedAverage* mSpeedAverage;
    const IUsePlayerActualMove* mActualMove;
    const IUsePlayerFlag* mFlag;
};

static_assert(sizeof(CourseSelectPlayerActionJump) == 0x40);
