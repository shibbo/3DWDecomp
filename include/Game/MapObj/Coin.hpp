#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; }
class ItemStateAssistRotate;
class Coin : public al::LiveActor {
public:
    explicit Coin(const char*);
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void reappear() override;
    void kill() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*) override;
    void killBySwitch();
    void appearBySwitch();
    void show();
    void hide();
    bool isEnableMsgItemGet(const al::SensorMsg*) const;
    void resetBaseQuat();
    void appearJump();
    void rotate();
    void setEnableAppear(bool enable) { mEnableAppear = enable; }
    void exeAppearJumpRise();
    void exeAppearJumpFall();
    void exeWait();
    void exeSpin();
private:
    al::MtxConnector* mConnector = nullptr;
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mJumpStart = {0.0f, 0.0f, 0.0f};
    ItemStateAssistRotate* mAssistRotate = nullptr;
    float mCollisionCheckDist = 300.0f;
    bool mInRouteDokan = false;
    bool mDropShadowActorDown = false;
    bool mSingleMode = false;
    bool mEnableAppear = true;
    int mFileID = -1;
};
static_assert(sizeof(Coin) == 0x188);
