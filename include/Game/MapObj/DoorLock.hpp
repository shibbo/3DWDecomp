#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class MtxConnector; class CameraTicket; }
class DoorKey;
class DoorLock : public al::LiveActor {
public:
    explicit DoorLock(const char* name);
    ~DoorLock() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void control() override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool showActor() override;
    bool hideActor() override;
    void exeClosedWait();
    void exeClosedReaction();
    void exeWaitKeyDisappear();
    void exeOpen();
    void exeOpenWait();

    /** @return The placement zone ID of the door. */
    int getZoneId() const { return mZoneId; }
private:
    al::LiveActor* mCollision = nullptr;
    al::MtxConnector* mConnector = nullptr;
    DoorKey* mOpeningKey = nullptr;
    DoorKey* mLinkedKey = nullptr;
    bool mCutScene = false;
    int mCutSceneDuration = 50;
    int mZoneId;
    int mExplosionCooldown = 0;
    al::CameraTicket* mCamera = nullptr;
    al::LiveActor* mBreakModel;
};
static_assert(sizeof(DoorLock) == 0x188);
