#pragma once
#include "MapObj/DisasterModeController.hpp"
namespace al { class MtxConnector; class PrePassPointLight; }
class TouchReactionMapParts : public al::LiveActor, public DisasterModeController::IUseEventReceiver {
public:
    explicit TouchReactionMapParts(const char*);
    ~TouchReactionMapParts() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void kill() override;
    void updateLinkedTrans(const sead::Vector3f&) override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPointSM(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    bool hideActor() override;
    bool showActor() override;
    void startClipped() override;
    void endClipped() override;
    void respawn() override;
    void updateSparkleLight();
    void onDisasterModeStateChange(DisasterModeController::State) override;
    void exeWait();
    void exeTouched();
    void exeReaction();
    void exeFinish();
private:
    s32 mAnimType = 0;
    s32 mItemType = 0;
    s32 mTouchFrames = 0;
    al::MtxConnector* mConnector = nullptr;
    al::PrePassPointLight* mLight = nullptr;
    sead::Vector3f mLightOffset;
    bool mIsInvisible = true;
    bool mHasSparkle = false;
    bool mIsDisaster = false;
    bool mLinkedTransUpdated = false;
};
static_assert(sizeof(TouchReactionMapParts) == 0x180);
