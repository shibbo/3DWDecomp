#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include <math/seadMatrix.h>
namespace al { class CollisionObj; }
class DisasterSpikeDirt : public al::LiveActor {
public:
    explicit DisasterSpikeDirt(const char*);
    virtual void init(const al::ActorInitInfo&, bool gold, bool horizontal);
    void updateHorizontalCollisionMtx();
    void appear(sead::Vector3f, sead::Quatf);
    void kill() override;
    void setUseHorizontalCollision(bool);
    bool getUseHorizontalCollision() const;
    void tryStopGlow();
    bool tryStartGlowOffAnim();
    bool tryStartGlowLoopAnim();
    bool tryStartExplodeAnim();
    void tryResetExplodeAnim();
private:
    bool mUseHorizontalCollision = false;
    al::CollisionObj* mHorizontalCollision = nullptr;
    sead::Matrix34f mHorizontalCollisionMtx = sead::Matrix34f::ident;
    bool mGold = false;
};
static_assert(sizeof(DisasterSpikeDirt) == 0x188);
