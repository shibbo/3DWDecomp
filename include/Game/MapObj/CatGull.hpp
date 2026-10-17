#pragma once

#include "MapObj/DisasterModeController.hpp"
namespace al { class MtxConnector; }
class Lighthouse;

/**
 * @brief The seagull flock perched around an island in Bowser's Fury.
 */
class CatGull : public al::LiveActor, public DisasterModeController::IUseEventReceiver {
public:
    explicit CatGull(const char* pName);
    ~CatGull() override;
    void init(const al::ActorInitInfo&) override;
    void startFlySwitch();
    void initAfterPlacement() override;
    void pausedModelUpdate() override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startFly(const sead::Vector3f&);
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void exeWait();
    void updateConnector();
    void exeTurn();
    void exeFly();
    void exeWaitRespawn();
    void exeFlyReturn();
    void onDisasterModeStateChange(DisasterModeController::State) override;
    void endClipped() override;

    void startFlyReturn();

    /**
     * @brief Get the island the gulls belong to.
     * @return The island (zone) number.
     */
    s32 getIslandId() const { return mIslandId; }

private:
    float mTurnSpeed = 0.0f;
    float mClippingRadius;
    float mFlightDistance = 0.0f;
    sead::Vector3f mFlightDir = sead::Vector3f::ez;
    sead::Vector3f mHomeTrans = sead::Vector3f::zero;
    Lighthouse* mLighthouse = nullptr;
    s32 mIslandId = 0;
    bool mOnLighthouse;
    al::MtxConnector* mConnector;
};
static_assert(sizeof(CatGull) == 0x190);
