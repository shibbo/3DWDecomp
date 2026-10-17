#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class CoinCourseSelect;
class ChameleonStateMic;
struct RenderMaterialIndirectParam;

class CoinChameleonCourseSelect : public al::LiveActor {
public:
    explicit CoinChameleonCourseSelect(const char* pName);
    ~CoinChameleonCourseSelect() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void control() override;
    void appear() override;
    void attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) override;
    bool tryContactAppear(al::HitSensor* pSensor);
    bool isNerveAppear();
    void exeWait();
    void exeMicReaction();
    void exeContactAppear();
    void exeInterpolate();
    void updateVelocity();
    void rotate(float delta);
    void setRotate(float angle);
private:
    sead::Quatf mBaseQuat = sead::Quatf::unit;
    sead::Vector3f mBaseTrans;
    CoinCourseSelect* mpCoin = nullptr;
    float mRotate = 0.0f;
    float mTargetRotate = 0.0f;
    float mRotateFactor = 0.0f;
    RenderMaterialIndirectParam* mpIndirect = nullptr;
    ChameleonStateMic* mpMicState = nullptr;
    float mHeight = 0.0f;
    int mObjectId;
};
static_assert(sizeof(CoinChameleonCourseSelect) == 0x190);
