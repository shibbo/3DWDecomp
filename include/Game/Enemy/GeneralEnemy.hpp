#pragma once

#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

#include "Library/LiveActor/LiveActor.hpp"

namespace al {
class ByamlIter;
}

namespace generalEnemyElements {
namespace condition {

/** @brief Base class of the conditions that make a general enemy shift to another nerve. */
class GEConditionBase {
public:
    /**
     * @brief Reads the condition parameters.
     * @param rIter Data of the shift that owns the condition.
     */
    virtual void init(const al::ByamlIter& rIter) {}

    /**
     * @brief Resets the condition when its nerve starts.
     * @param pActor Enemy that owns the condition.
     */
    virtual void setup(al::LiveActor* pActor) {}

    virtual bool check(al::LiveActor* pActor) = 0;
};

/** @brief Condition that is never met. */
class FalseCondition : public GEConditionBase {
public:
    /**
     * @brief Never lets the shift happen.
     * @param pActor Enemy that owns the condition.
     * @return Always false.
     */
    bool check(al::LiveActor* pActor) override { return false; }
};

/** @brief Condition that is always met. */
class TrueCondition : public GEConditionBase {
public:
    /**
     * @brief Always lets the shift happen.
     * @param pActor Enemy that owns the condition.
     * @return Always true.
     */
    bool check(al::LiveActor* pActor) override { return true; }
};

/** @brief Condition met once the current action has finished. */
class ActionEnd : public GEConditionBase {
public:
    bool check(al::LiveActor* pActor) override;
};

/** @brief Condition met after the nerve has run for a number of frames. */
class Timer : public GEConditionBase {
public:
    void init(const al::ByamlIter& rIter) override;
    bool check(al::LiveActor* pActor) override;

private:
    s32 mFrame = 120;
};

/** @brief Condition met when the enemy has stopped moving for a few frames. */
class IsVelocityZero : public GEConditionBase {
public:
    void setup(al::LiveActor* pActor) override;
    bool check(al::LiveActor* pActor) override;

private:
    sead::Vector3f mPrevTrans = {0.0f, 0.0f, 0.0f};
    u32 mStopCount;
};

/** @brief Condition met randomly with a given probability. */
class Random : public GEConditionBase {
public:
    void init(const al::ByamlIter& rIter) override;
    bool check(al::LiveActor* pActor) override;

private:
    f32 mProbability = 50.0f;
};

}  // namespace condition

namespace behavior {

/** @brief Base class of the behaviors run while a general enemy nerve is active. */
class GEBehaviorBase {
public:
    /**
     * @brief Reads the behavior parameters.
     * @param rIter Data of the nerve that owns the behavior.
     */
    virtual void init(const al::ByamlIter& rIter) {}

    /**
     * @brief Prepares the behavior when its nerve starts.
     * @param pActor Enemy that runs the behavior.
     */
    virtual void setup(al::LiveActor* pActor) {}

    /**
     * @brief Cleans up the behavior when its nerve ends.
     * @param pActor Enemy that runs the behavior.
     */
    virtual void teardown(al::LiveActor* pActor) {}

    virtual void update(al::LiveActor* pActor) = 0;
};

/** @brief Stays still on the nearest rail position. */
class RailWait : public GEBehaviorBase {
public:
    void setup(al::LiveActor* pActor) override;
    void update(al::LiveActor* pActor) override;
};

/** @brief Moves along the rail at a constant speed. */
class RailMove : public GEBehaviorBase {
public:
    void init(const al::ByamlIter& rIter) override;
    void update(al::LiveActor* pActor) override;

private:
    f32 mSpeed = 5.0f;
};

/** @brief Moves along the rail while slowing down to a stop. */
class RailBrake : public GEBehaviorBase {
public:
    void init(const al::ByamlIter& rIter) override;
    void setup(al::LiveActor* pActor) override;
    void update(al::LiveActor* pActor) override;

private:
    f32 mStartSpeed = 5.0f;
    f32 mSpeed = 5.0f;
    f32 mBrakeSpeed = 0.1f;
};

/** @brief Kills the enemy. */
class Kill : public GEBehaviorBase {
public:
    void update(al::LiveActor* pActor) override;
};

}  // namespace behavior
}  // namespace generalEnemyElements

namespace GeneralEnemySub {

/** @brief A shift from one nerve to another, taken when its condition is met. */
struct Shift {
    const char* mConditionName;
    generalEnemyElements::condition::GEConditionBase* mCondition;
    const char* mNerveName;
};

/** @brief A data-driven nerve: an action, a behavior and the shifts to other nerves. */
class GeneralEnemyNerve {
public:
    /**
     * @brief Creates a nerve with no behavior and no shifts.
     * @param pName Name of the nerve.
     * @param pActionName Action started when the nerve starts, or nullptr.
     * @param pBehaviorName Name of the behavior type, or nullptr.
     */
    GeneralEnemyNerve(const char* pName, const char* pActionName, const char* pBehaviorName)
        : mName(pName), mActionName(pActionName), mBehaviorName(pBehaviorName) {}

    const char* mName;
    const char* mActionName;
    const char* mBehaviorName;
    generalEnemyElements::behavior::GEBehaviorBase* mBehavior = nullptr;
    sead::PtrArray<Shift> mShifts;
};

/** @brief Nerves of a general enemy, looked up by name. */
class NerveMap {
public:
    NerveMap();
    void allocBuffer(s32 size);
    void insert(const char* pName, GeneralEnemyNerve* pNerve);
    GeneralEnemyNerve* find(const char* pName);
    s32 findIndex(const char* pName);
    void erase(const char* pName);

    /**
     * @brief Gets the number of nerves.
     * @return Number of nerves in the map.
     */
    s32 size() const { return mNames.size(); }

private:
    sead::ObjArray<const char*> mNames;
    sead::PtrArray<GeneralEnemyNerve> mNerves;
};

/** @brief Nerve the enemy shifts to when it receives a reaction. */
struct Reaction {
    const char* mName;
    const char* mNerveName;
};

}  // namespace GeneralEnemySub

/** @brief Test enemy whose nerves, actions and behaviors are all described by a data file. */
class GeneralEnemy : public al::LiveActor {
public:
    GeneralEnemy(const char* pName);
    ~GeneralEnemy() override;

    void init(const al::ActorInitInfo& rInfo) override;
    void initNerve(const u8* pData);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                    al::HitSensor* pOther) override;
    void exeCommon();
    void setNerve(GeneralEnemySub::GeneralEnemyNerve* pNerve);
    void initReaction(const al::ByamlIter& rIter);

private:
    sead::FixedSafeString<64> mDisplayName;
    const char* mModelArcName = nullptr;
    const char* mDataFileName = nullptr;
    GeneralEnemySub::NerveMap mNerveMap;
    GeneralEnemySub::GeneralEnemyNerve* mCurrentNerve = nullptr;
    sead::ObjArray<GeneralEnemySub::Reaction> mReactions;
    sead::Vector3f mRailClippingInfo = {0.0f, 0.0f, 0.0f};
};
