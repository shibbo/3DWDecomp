#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al { class KeyPoseKeeper; }

class KoopaChaseWarpDummy : public al::LiveActor {
public:
    explicit KoopaChaseWarpDummy(const char* pName);
    ~KoopaChaseWarpDummy() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void appear() override;
    void appear(int damageCount);
    al::LiveActor* getKoopa() const;
    void exeMove();
    void exeLand();
    void exeWait();
    void exeProvocation();

private:
    al::KeyPoseKeeper* mKeyPoseKeeper = nullptr;
    int mMoveTime = 0;
    int mDamageCount = 0;
};
static_assert(sizeof(KoopaChaseWarpDummy) == 0x158);
