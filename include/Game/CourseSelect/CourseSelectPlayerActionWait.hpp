#pragma once

#include "Player/PlayerAction.hpp"

struct PlayerActionArg;
class IUsePlayerHorizontalSpeedAverage;
class PlayerFigureDirector;
class IUsePlayerPanicControl;

class CourseSelectPlayerActionWait : public PlayerAction {
    SEAD_RTTI_OVERRIDE(CourseSelectPlayerActionWait, PlayerAction)

public:
    CourseSelectPlayerActionWait(const PlayerActionArg*, IUsePlayerHorizontalSpeedAverage*,
                                const PlayerFigureDirector*, bool, IUsePlayerPanicControl*);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;

private:
    const PlayerActionArg* mArg;
    IUsePlayerHorizontalSpeedAverage* mSpeedAverage;
    const PlayerFigureDirector* mFigureDirector;
    bool mSolveAir;
    IUsePlayerPanicControl* mPanicControl;
};

static_assert(sizeof(CourseSelectPlayerActionWait) == 0x30);
