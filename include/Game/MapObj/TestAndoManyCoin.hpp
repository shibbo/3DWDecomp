#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class DeferredRenderingShpUbo;
}

/**
 * @brief Interface of a TestAndoManyCoin formation: lays out and animates a large number of
 * coins, sorts them by depth for drawing and handles collecting them.
 */
class TestAndoManyCoinCalcBase {
public:
    virtual void update(u32 step) = 0;
    virtual void sort(const sead::Vector3f& rCameraPos, const sead::Vector3f& rCameraDir) = 0;
    virtual u32 getMaxNum() const = 0;
    virtual u32 getNum() const = 0;
    virtual void getPos(sead::Vector3f* pPos, u32 index) const = 0;
    virtual f32 getRadius() const = 0;
    virtual bool tryCollect(const sead::Vector3f& rPos, f32 radius, s32 step) = 0;
    virtual void drawDebug() const = 0;
};

/**
 * @brief Test object that draws hundreds of coins with a single model through per-shape uniform
 * blocks; the arrangement is chosen by the "CalcType" placement argument.
 */
class TestAndoManyCoin : public al::LiveActor {
public:
    /** @brief Placement values of "CalcType". */
    enum CalcType : s32 {
        CalcType_Ring = 0,
        CalcType_Fountain = 1,
        CalcType_Grid = 2,
    };

    explicit TestAndoManyCoin(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void draw() const override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                    al::HitSensor* pReceiver) override;
    void exeCute();

private:
    s32 mCalcType = -1;
    TestAndoManyCoinCalcBase* mCalc = nullptr;
    al::DeferredRenderingShpUbo*** mShapeUbos = nullptr;
    al::LiveActor* mLastCollector = nullptr;
    s32 mCollectCooldown = 0;
    s32 mKillStep = -1;
    f32 mRotateRadius = 0.0f;
    f32 mRotateSpeed = 0.0f;
    sead::Vector3f mCenter = {0.0f, 0.0f, 0.0f};
    sead::Vector3f mFront;
};

static_assert(sizeof(TestAndoManyCoin) == 0x188);
