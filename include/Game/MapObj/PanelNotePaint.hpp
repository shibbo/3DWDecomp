#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class MtxConnector;
}
class PanelNotePaint : public al::LiveActor {
  public:
    explicit PanelNotePaint(const char* name);
    ~PanelNotePaint() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void makeActorAppeared() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*,
                               al::ScreenPointTarget*) override;
    bool tryContinuePushing();
    bool isEnablePush() const;
    void pushFirstTime(al::HitSensor*, bool, bool);
    void pushFirstTimeScreenPoint(al::ScreenPointer*, bool, bool);
    void exeOffWait();
    void exeReaction();
    void exeOnWaitBefore();
    void setNerveToOnWait();
    void exeOnWait();
    void exeOnWaitAfterHipDropped();
    void exeReactionReturn();
    void exeHipDropped();
    void setNerveToOnWaitAfterHipDropped();
    void exeInvincibleTouched();
    void settingForOnWait();
    void settingForInvincibleTouched();
    void setNerveToInvincibleTouched();
    bool isOn() const;
    al::HitSensor* getScoreSensor();
    void setCompleteEffect();
    bool isInitiallyOn() const { return mInitiallyOn; }
    void setPitch(int pitch) { mPitch = pitch; }

  private:
    int mColor = 0;
    al::MtxConnector* mConnector = nullptr;
    float mExplosionRadius = 0.0f;
    bool mHitReactionStarted = false;
    int mHitReactionTimer = 0;
    al::HitSensor* mScoreSensor = nullptr;
    bool mInitiallyOn = false;
    bool mOn = false;
    int mPitch = -1;
};
static_assert(sizeof(PanelNotePaint) == 0x170);
