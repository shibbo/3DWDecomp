#pragma once

#include "Player/PlayerAction.hpp"
#include "Player/IUsePlayerActionEnd.hpp"
#include "Player/IUsePlayerActionCancelable.hpp"

struct PlayerActionArg;
class CourseSelectPlayerActor;

class CourseSelectPlayerActionPush : public PlayerAction,
                                     public IUsePlayerActionEnd,
                                     public IUsePlayerActionCancelable {
    SEAD_RTTI_OVERRIDE(CourseSelectPlayerActionPush, PlayerAction)

public:
    explicit CourseSelectPlayerActionPush(const PlayerActionArg*, CourseSelectPlayerActor*);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;
    bool isEnd() const override;
    bool isPossibleToCancel() const override;

private:
    const PlayerActionArg* mArg;
    CourseSelectPlayerActor* mPlayer;
    bool mEnableSubAction = true;
};

static_assert(sizeof(CourseSelectPlayerActionPush) == 0x30);
