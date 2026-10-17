#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }
/** @brief Bowser's key-pose-driven descent from a wall. */
class KoopaLastWallClimbDown : public al::LiveActor {
public:
    explicit KoopaLastWallClimbDown(const char* pName);
    ~KoopaLastWallClimbDown() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    void exeAppear();
    void exeDownStart();
    void exeDown();
    void exeDownEnd();
    void exeFall();
    void exeHide();
private:
    bool mIsUseAppear = false;
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    sead::Vector3f mClippingCenter = {0.0f,0.0f,0.0f};
    sead::Vector3f mAppearPos = {0.0f,0.0f,0.0f};
    sead::Vector3f mUnused168 = {0.0f,0.0f,0.0f};
    float mMoveSpeed = 0.0f;
    float mMoveDistance = 0.0f;
    float mKeyDistance = 0.0f;
};
static_assert(sizeof(KoopaLastWallClimbDown) == 0x180);
