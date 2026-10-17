#include "MapObj/GraphicsObjShadowMaskSphere.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Shadow/ShadowMaskDrawer.hpp"

namespace al {
GraphicsObjShadowMaskSphere::GraphicsObjShadowMaskSphere(const char* pName)
    : ShadowMaskBase(pName), LiveActor(pName) {}
GraphicsObjShadowMaskSphere::~GraphicsObjShadowMaskSphere() {}

void GraphicsObjShadowMaskSphere::init(const ActorInitInfo& rInfo) {
    initActorSceneInfo(this, rInfo);
    initActorPoseTRSV(this);
    setHost(this);
    initExecutorUpdate(this, rInfo, "シャドウマスク");
    initActorSRT(this, rInfo);
    initStageSwitch(this, rInfo);
    trySyncStageSwitchAppear(this);
    tryGetArg(&mColor.r, rInfo, "ColorRed");
    tryGetArg(&mColor.g, rInfo, "ColorGreen");
    tryGetArg(&mColor.b, rInfo, "ColorBlue");
    tryGetArg(&mColor.a, rInfo, "ColorAlpha");
    tryGetArg(&mOffset.x, rInfo, "OffsetX");
    tryGetArg(&mOffset.y, rInfo, "OffsetY");
    tryGetArg(&mOffset.z, rInfo, "OffsetZ");
    tryGetArg(reinterpret_cast<int*>(&mDrawCategory), rInfo, "ShadowMaskDrawCategory");
    declare(mDrawCategory);
    tryGetArg(&mRadius, rInfo, "Radius");
    tryGetArg(&mPower, rInfo, "ShadowMaskPow");
}

void GraphicsObjShadowMaskSphere::declare(ShadowMaskDrawCategory category) {
    ShadowMaskFunction::getShadowMaskKeeper(this)->declare(ShadowMaskType::Sphere, category);
}

void GraphicsObjShadowMaskSphere::update() {
    sead::Vector3f position = getTrans(this) + mOffset;
    mShadowMtx.makeST(sead::Vector3f(mRadius, mRadius, mRadius), position);
    ShadowMaskFunction::getShadowMaskKeeper(this)->addSphere(mShadowMtx, mColor, mPower,
        getShadowIntensity(), mDrawCategory);
}

void GraphicsObjShadowMaskSphere::movementPaused(bool isPaused) {
    mIsPausedMovement = true;
    movement();
    mIsPausedMovement = false;
}

void ShadowMaskBase::calcShadowMatrix(sead::Matrix34f* pMtx) {}
}
