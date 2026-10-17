#pragma once

class PlayerActor;

/** @brief Controls one player's participation in a demo. */
class DemoPlayerController {
public:
    DemoPlayerController();
    void setPlayerActor(PlayerActor* pActor);
    bool tryStartDemo();
    void endDemo();

    /** @brief Gets the assigned player. @return Player actor, or nullptr when unassigned. */
    PlayerActor* getPlayerActor() const { return mPlayerActor; }

private:
    PlayerActor* mPlayerActor;
    bool mIsStarted;
};

static_assert(sizeof(DemoPlayerController) == 0x10);
