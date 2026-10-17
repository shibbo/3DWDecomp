#include "MapObj/JumpFlipPanel.hpp"
#include "MapObj/Koura.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(JumpFlipPanel, Wait);
NERVE_DECL(JumpFlipPanel, SwitchOffStart);
NERVE_DECL(JumpFlipPanel, Flip);
NERVES_MAKE_STRUCT(JumpFlipPanel, Wait, SwitchOffStart, Flip)
}
JumpFlipPanel::JumpFlipPanel(const char* name) : al::LiveActor(name) {}
JumpFlipPanel::~JumpFlipPanel() {}
void JumpFlipPanel::init(const al::ActorInitInfo& info) {
    mIsSingleMode = al::isSingleMode(info);
    al::initActorPoseTQSV(this);
    al::initMapPartsActor(this, info, mIsSingleMode ? "SM" : nullptr, 0);
    al::initNerve(this, &NrvJumpFlipPanel.Wait, 0);
    al::trySetShadowLength(this, info, nullptr);
    mBaseQuat = al::getQuat(this);
    al::initJointControllerKeeper(this, 1);
    if (al::isExistJoint(this, "Rotate")) al::initJointLocalZRotator(this, &mRotation, "Rotate");
    mCollision = al::createCollisionObjMtx(this, info, "PlayerOnly", al::getHitSensor(this, "CollisionParts"), &mCollisionMtx, "PlayerOnly");
    mCollision->makeActorDead();
    if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &JumpFlipPanel::start)))
        al::setNerve(this, &NrvJumpFlipPanel.SwitchOffStart);
    if (!al::tryGetArg(&mIsUpperTurn, info, "IsUpperTurn")) mIsUpperTurn = false;
    bool depthShadow = false;
    al::tryGetArg(&depthShadow, info, "UsingDepthShadow");
    bool expandClipping = false;
    al::tryGetArg(&expandClipping, info, "IsExpandClippingShadowLength");
    if (expandClipping && !depthShadow) al::tryExpandClippingByShadowLength(this, &mClippingCenter);
    if (!mActorSceneInfo->isSingleMode) {
        al::onDrawClipping(this);
        al::setIgnoreUpdateDrawClipping(this, true);
    }
    makeActorAppeared();
    updateLodAnim();
}
void JumpFlipPanel::start() {
    if (al::isNerve(this, &NrvJumpFlipPanel.SwitchOffStart)) al::setNerve(this, &NrvJumpFlipPanel.Wait);
}
void JumpFlipPanel::updateLodAnim() {
    if (mIsSingleMode && getFarLodActor()) {
        if (mIsFront) al::startVisAnimAndSetFrameAndStop(getFarLodActor(), "Color", 0.0f);
        else al::startVisAnimAndSetFrameAndStop(getFarLodActor(), "Color", 1.0f);
    }
}
void JumpFlipPanel::exeWait() {
    if (al::isFirstStep(this)) {
        bool upper = mIsUpperTurn;
        bool front = mIsFront;
        float backAngle = upper ? -180.0f : 180.0f;
        mRotation = front ? 0.0f : backAngle;
    }
    bool jump = rc::isAnyPlayerJumpTrigOn(this);
    if (mIsSingleMode) {
        al::LiveActor* nearest = al::tryFindNearestPlayerActor(this);
        if (nearest && rc::isReallyPlayerActor(nearest)) {
            PlayerActor* player = static_cast<PlayerActor*>(nearest);
            if (player->isInKoura()) jump |= static_cast<Koura*>(al::getSensorHost(player->getBindSensor()))->isJumpStart();
        }
    }
    if (!jump) return;
    mCollisionMtx = *getBaseMtx();
    if (!mIsFront) {
        sead::Quatf quat;
        quat.setAxisAngle(sead::Vector3f::ey, 180.0f);
        sead::Matrix34f rotate;
        rotate.makeQT(quat, sead::Vector3f(0.0f, 0.0f, 0.0f));
        mCollisionMtx = mCollisionMtx * rotate;
    }
    mCollision->appear();
    al::setNerve(this, &NrvJumpFlipPanel.Flip);
}
void JumpFlipPanel::exeFlip() {
    if (al::isFirstStep(this)) {
        al::startHitReactionStart(this);
        al::startSe(this, "FlipSt", nullptr);
    }
    al::holdSe(this, "FlipLv", nullptr);
    bool finished = false;
    if (al::isGreaterStep(this, 5)) {
        if (mIsFront) {
            if (mIsUpperTurn) {
                mRotation += -180.0f / 35.0f;
                if (mRotation < -180.0f) { mRotation = -180.0f; finished = true; }
            } else {
                mRotation += 180.0f / 35.0f;
                if (mRotation >= 180.0f) { mRotation = 180.0f; finished = true; }
            }
        } else {
            if (mIsUpperTurn) {
                mRotation += 180.0f / 35.0f;
                if (mRotation >= 0.0f) { mRotation = 0.0f; finished = true; }
            } else {
                mRotation += -180.0f / 35.0f;
                if (mRotation <= 0.0f) { mRotation = 0.0f; finished = true; }
            }
        }
    }
    if (al::isStep(this, 20)) mCollision->kill();
    if (al::isGreaterStep(this, 25) && rc::isAnyPlayerJumpTrigOn(this)) {
        mIsFront = !mIsFront;
        al::startSe(this, "FlipSt", nullptr);
        mCollision->kill();
        al::setNerve(this, &NrvJumpFlipPanel.Flip);
        return;
    }
    if (finished) {
        al::startSe(this, "FlipEd", nullptr);
        mIsFront = !mIsFront;
        updateLodAnim();
        al::setNerve(this, &NrvJumpFlipPanel.Wait);
    }
}
bool JumpFlipPanel::isFlipping() { return al::isNerve(this, &NrvJumpFlipPanel.Flip); }
void JumpFlipPanel::exeSwitchOffStart() {}
bool JumpFlipPanel::isFarLodSwitchOkay() { return !isFlipping(); }
