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
    const sead::Vector3f& getTrans() const;
    const sead::Vector3f& getFrontVec() const;
    const sead::Vector3f& getUpVec() const;
    void setScale(float scale);
    void setTrans(const sead::Vector3f& rTrans);
    void resetPosition(const sead::Vector3f& rTrans);
    void setFrontUpVec(const sead::Vector3f& rFront, const sead::Vector3f& rUp);
    void setRotate(const sead::Vector3f& rRotate);
    void setVelocity(const sead::Vector3f& rVelocity);
    bool isActionEnd() const;

    /** @brief Gets the assigned player. @return Player actor, or nullptr when unassigned. */
    PlayerActor* getPlayerActor() const { return mPlayerActor; }

private:
    PlayerActor* mPlayerActor;
    bool mIsStarted;
};

static_assert(sizeof(DemoPlayerController) == 0x10);
