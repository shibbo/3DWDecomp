#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class KoopaChaseKoopa;
class KoopaChaseBattle;
class KoopaChaseMover;
/** @brief Bowser's chase car, targeting, and battle-state coordinator. */
class KoopaChase : public al::LiveActor {
public:
    explicit KoopaChase(const char* pName);
    ~KoopaChase() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void kill() override;
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther) override;
    static f32 getRunStartDistance();
    static f32 getRunStopDistance();
    void stopEffectAll();
    bool isAllPlayerBinded() const;
    void exeBattle();
    KoopaChaseKoopa* mKoopa = nullptr;
    al::LiveActor* mTargetPlayer = nullptr;
    al::LiveActor* mLookAtTargetPlayer = nullptr;
    KoopaChaseBattle* mBattle = nullptr;
    int mValue168 = 0;
    KoopaChaseMover* mMover = nullptr;
    sead::Vector3f mCameraLookAt = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mPreviousKoopaPos = {0.0f, 0.0f, 0.0f};
    float mTireRotation = 0.0f;
    float mRootRotation = 0.0f;
    sead::Vector3f mRootOffset = sead::Vector3f::zero;
    int mSwayStep = 0;
};
static_assert(sizeof(KoopaChase) == 0x1a8);
