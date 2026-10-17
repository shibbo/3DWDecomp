#include "MapObj/Fury/DisasterSpikeDirt.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Project/Collision/CollisionParts.hpp"
DisasterSpikeDirt::DisasterSpikeDirt(const char* name) : al::LiveActor(name) {}
void DisasterSpikeDirt::init(const al::ActorInitInfo& info, bool gold, bool horizontal) {
    mGold = gold;
    if (gold) al::initActorWithArchiveName(this, info, "DisasterSpikeDirtGold", nullptr);
    else al::initActorWithArchiveName(this, info, "DisasterSpikeDirt", nullptr);
    mUseHorizontalCollision = horizontal;
    if (gold || horizontal) {
        al::initSubActorKeeperNoFile(this, info, 1);
        mHorizontalCollision = al::createCollisionObj(this, info, "DisasterSpikeDirtHorizontal", al::getHitSensor(this, "Collision"), nullptr, nullptr);
        al::registerSubActorSyncClipping(this, mHorizontalCollision, false);
        al::setSubActorOnSyncAppear(this);
        updateHorizontalCollisionMtx();
        mHorizontalCollision->getCollisionParts()->setSyncCollisionMtx(&mHorizontalCollisionMtx);
        mHorizontalCollision->getCollisionParts()->syncMtx();
        mHorizontalCollision->getCollisionParts()->updateMtx();
    }
    makeActorDead();
}
void DisasterSpikeDirt::updateHorizontalCollisionMtx() {
    mHorizontalCollisionMtx.fromQuat(al::getQuat(this));
    mHorizontalCollisionMtx.setTranslation(al::getTrans(this));
    sead::Vector3f scale = al::getScaleX(this) * sead::Vector3f::ones;
    mHorizontalCollisionMtx.scaleBases(scale.x, scale.y, scale.z);
}
void DisasterSpikeDirt::appear(sead::Vector3f trans, sead::Quatf quat) {
    al::setTrans(this, trans);
    al::setQuat(this, quat);
    updateHorizontalCollisionMtx();
    if (mUseHorizontalCollision) {
        mHorizontalCollision->appear();
        al::invalidateCollisionParts(this);
    }
    al::LiveActor::appear();
}
void DisasterSpikeDirt::kill() {
    if (mUseHorizontalCollision) mHorizontalCollision->kill();
    al::LiveActor::kill();
}
void DisasterSpikeDirt::setUseHorizontalCollision(bool value) { mUseHorizontalCollision = value; }
bool DisasterSpikeDirt::getUseHorizontalCollision() const { return mUseHorizontalCollision; }
void DisasterSpikeDirt::tryStopGlow() {
    if (al::tryStartMclAnimIfExist(this, mGold ? "DisasterSpikeDirtGoldOff" : "DisasterSpikeDirtOff")) al::setMclAnimFrameAndStopEnd(this);
}
bool DisasterSpikeDirt::tryStartGlowOffAnim() { return al::tryStartMclAnimIfExist(this, mGold ? "DisasterSpikeDirtGoldOff" : "DisasterSpikeDirtOff"); }
bool DisasterSpikeDirt::tryStartGlowLoopAnim() { return al::tryStartMclAnimIfExist(this, mGold ? "DisasterSpikeDirtGold" : "DisasterSpikeDirt"); }
bool DisasterSpikeDirt::tryStartExplodeAnim() {
    const char* anim = mGold ? "DisasterSpikeDirtGoldExplode" : "DisasterSpikeDirtExplode";
    al::tryStartMclAnimIfExist(this, anim);
    return al::tryStartMtpAnimIfExist(this, anim);
}
void DisasterSpikeDirt::tryResetExplodeAnim() {
    const char* anim = mGold ? "DisasterSpikeDirtGoldExplode" : "DisasterSpikeDirtExplode";
    if (al::isMclAnimExist(this, anim) && al::isMtpAnimExist(this, anim)) {
        al::startMclAnimAndSetFrameAndStop(this, anim, 0.0f);
        al::startMtpAnimAndSetFrameAndStop(this, anim, 0.0f);
    }
}
