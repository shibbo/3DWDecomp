#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "math/seadMatrix.h"
class Bottle;
class ActorStateSupportStroke;
class ActorJointLookController;
namespace al {
class RumbleCalculatorCosMultLinear;
}
class FairyPrincess : public al::LiveActor {
public:
    explicit FairyPrincess(const char* pName);
    ~FairyPrincess() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void initFairyWithArchiveName(const al::ActorInitInfo& rInfo, const char* pArchive,
                                  const char* pAction);
    void control() override;
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) override;
    bool receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pOther,
                    al::HitSensor* pSelf) override;
    bool receiveMsgScreenPoint(const al::SensorMsg* pMessage, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget) override;
    bool isEnableReaction() const;
    void calcAnim() override;
    void startClipped() override;
    void endClipped() override;
    void startDemoAction(const char* pAction, const al::LiveActor* pDemo);
    void createBottle(const al::ActorInitInfo& rInfo);
    bool tryBreakBottle();
    al::LiveActor* getWingActor() const;
    al::LiveActor* getBottleActor() const;
    al::LiveActor* getBottleInnerActor() const;
    al::LiveActor* getBottleCapActor() const;
    /** @brief Checks whether a bottle was created. @return Whether the bottle exists. */
    bool isExistBottle() const { return mBottle != nullptr; }
    /** @brief Enables bottled reactions. @param isEnable Whether reactions are enabled. */
    void setEnableBottleReaction(bool isEnable) { mLookEnabled = isEnable; }
    void exeWaitGoalPole();
    void exeStroke();
    void exeDemoAction();

private:
    Bottle* mBottle;
    ActorStateSupportStroke* mStroke;
    ActorJointLookController* mLook;
    sead::Matrix34f mHeadBase;
    al::RumbleCalculatorCosMultLinear* mRumble;
    bool mHasAppearedItem;
    bool mLookEnabled;
};
static_assert(sizeof(FairyPrincess) == 0x1a0);
