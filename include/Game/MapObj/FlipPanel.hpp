#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <container/seadBuffer.h>
namespace al {
class KeyMoveMovement;
class CollisionObj;
class MtxConnector;
} // namespace al
class FlipPanelParts;
class FlipPanel : public al::LiveActor {
  public:
    explicit FlipPanel(const char* name) : LiveActor(name) {}
    void init(const al::ActorInitInfo&) override;
    void exeWatch();
    void exeFix();
    void setLastSensor(al::HitSensor* sensor) { mLastSensor = sensor; }
    void setLastPanel(FlipPanelParts* panel) { mLastPanel = panel; }

  private:
    sead::Buffer<FlipPanelParts*> mPanels;
    al::HitSensor* mLastSensor = nullptr;
    FlipPanelParts* mLastPanel = nullptr;
};
class FlipPanelParts : public al::LiveActor {
  public:
    FlipPanelParts(const char* name, FlipPanel* host) : LiveActor(name), mHost(host) {}
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void makeActorAppeared() override;
    void makeActorDead() override;
    void control() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isExclamation() const;
    void setPrevEndNerve();
    void setEndNerve();
    void exeQuestion();
    void exeQuestionOn();
    void exeExclamation();
    void exeExclamationOn();
    void exePrevEnd();
    void exeEnd();

  private:
    al::KeyMoveMovement* mMovement = nullptr;
    al::CollisionObj* mCollision = nullptr;
    FlipPanel* mHost;
    al::MtxConnector* mConnector = nullptr;
    int mTouchTimer = 0;
    int mPreviousTouchTimer = 0;
    int mLightType = 0;
};
static_assert(sizeof(FlipPanel) == 0x168);
static_assert(sizeof(FlipPanelParts) == 0x178);
