#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"

namespace al {
class GraphicsObjShadowMaskSphere : public ShadowMaskBase, public LiveActor {
public:
    GraphicsObjShadowMaskSphere(const char* pName);
    ~GraphicsObjShadowMaskSphere() override;
    void init(const ActorInitInfo& rInfo) override;
    void declare(ShadowMaskDrawCategory category) override;
    void update() override;
    void movementPaused(bool isPaused) override;
    void updateMulti() override {}
    void addMulti() override {}
    ShadowMaskType getShadowMaskType() const override { return ShadowMaskType::Sphere; }
    void control() override { update(); }

private:
    float mRadius = 300.0f;
    float mPower = 100.0f;
    bool mIsPausedMovement = false;
};
}
