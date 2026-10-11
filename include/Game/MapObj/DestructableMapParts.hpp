#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class ArrowHitResultBuffer; class CollisionParts; }
class DestructableMapParts : public al::LiveActor {
public:
    explicit DestructableMapParts(const char*);
    ~DestructableMapParts() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void appear() override;
    void kill() override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void startDestruct(const sead::Vector3f&, bool);
    void control() override;
    void exeWait();
    void exeHit();
    void exeDestruct();
    void exeConstruct();
    void exeEnd();
private:
    bool mDestructStarted = false;
    int mHitCount = 0;
    al::CollisionParts* mParts = nullptr;
    al::ArrowHitResultBuffer* mHitBuffer = nullptr;
};
static_assert(sizeof(DestructableMapParts) == 0x160);
