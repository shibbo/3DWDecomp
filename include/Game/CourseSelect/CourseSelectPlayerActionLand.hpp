#pragma once

#include "Player/PlayerAction.hpp"
#include "Player/IUsePlayerActionEnd.hpp"
#include "Player/IUsePlayerActionCancelable.hpp"

struct PlayerActionArg;

class CourseSelectPlayerActionLand : public PlayerAction,
                                     public IUsePlayerActionEnd,
                                     public IUsePlayerActionCancelable {
    SEAD_RTTI_OVERRIDE(CourseSelectPlayerActionLand, PlayerAction)

public:
    explicit CourseSelectPlayerActionLand(const PlayerActionArg*);

    void move() override;
    void update() override;
    void setup() override;
    void teardown() override;
    bool isEnd() const override;
    bool isPossibleToCancel() const override;

private:
    const PlayerActionArg* mArg;
    bool mEnableSubAction = true;
};

static_assert(sizeof(CourseSelectPlayerActionLand) == 0x28);
