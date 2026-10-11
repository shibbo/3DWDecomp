#pragma once

#include <math/seadQuat.h>
#include <math/seadVector.h>

class PlayerActor;

/** @brief Controls one player's participation in a demo. */
class DemoPlayerController {
public:
    DemoPlayerController();
    void setPlayerActor(PlayerActor* pActor);
    bool tryStartDemo();
    void endDemo();
    void stopSklAnimAndDeleteEffect();
    void hide();
    void show();
    void startAction(const char* pActionName);
    void setActionFrame(int frame);
    void setQuat(const sead::Quatf& rQuat);
    void setTransAndResetDynamics(const sead::Vector3f& rTrans);

    /** @brief Gets the assigned player. @return Player actor, or nullptr when unassigned. */
    PlayerActor* getPlayerActor() const { return mPlayerActor; }

private:
    PlayerActor* mPlayerActor;
    bool mIsStarted;
};

static_assert(sizeof(DemoPlayerController) == 0x10);
