#include "Boss/BossWackun.hpp"

#include <cmath>
#include <math/seadMathCalcCommon.h>

#include "Boss/BossStateDemoStart.hpp"
#include "Boss/BossWackunBody.hpp"
#include "Boss/BossWackunFrame.hpp"
#include "Boss/BossWackunHand.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/MapObj/EffectMtxSetter.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"

namespace {
NERVE_DECL(BossWackun, Wait);
NERVE_DECL(BossWackun, DemoStart);
NERVE_DECL(BossWackun, WaitStart);
NERVE_DECL(BossWackun, WaitDemoStart);
NERVE_DECL(BossWackun, BattleStart);
NERVE_DECL(BossWackun, Damage);
NERVE_DECL(BossWackun, RotateSign);
NERVE_DECL(BossWackun, DamageRotate);
NERVE_DECL(BossWackun, Rotate);
NERVE_DECL(BossWackun, Land);
NERVE_DECL(BossWackun, Down);
NERVE_DECL(BossWackun, RecoverStandUp);
NERVE_DECL(BossWackun, RecoverJump);
NERVE_DECL(BossWackun, RecoverFrame);
NERVE_DECL(BossWackun, RecoverRotate);
NERVE_DECL(BossWackun, RecoverFallSign);
NERVE_DECL(BossWackun, RecoverFall);
NERVE_DECL(BossWackun, RecoverLand);
NERVES_MAKE_NOSTRUCT(BossWackun, WaitDemoStart, BattleStart, RecoverJump, RecoverFrame,
                     RecoverRotate, RecoverFallSign, RecoverFall, RecoverLand)

// Nerves shared by several states are mutable globals (they get merged into one block).
BossWackunNrvWait NrvBossWackunWait;
BossWackunNrvDemoStart NrvBossWackunDemoStart;
BossWackunNrvWaitStart NrvBossWackunWaitStart;
BossWackunNrvDamage NrvBossWackunDamage;
BossWackunNrvRotateSign NrvBossWackunRotateSign;
BossWackunNrvDamageRotate NrvBossWackunDamageRotate;
BossWackunNrvRotate NrvBossWackunRotate;
BossWackunNrvLand NrvBossWackunLand;
BossWackunNrvDown NrvBossWackunDown;
BossWackunNrvRecoverStandUp NrvBossWackunRecoverStandUp;

/** @brief One edge of the bottom face the body can tumble over. */
struct RotateCandidate {
    sead::Vector3f axis;
    sead::Vector3f center;
    sead::Vector3f trans;
    sead::Vector3f normal;
};

constexpr const char* cSignLightName = "攻撃予兆影";
constexpr const char* cSignLightSideName = "攻撃予兆影[サイド]";
constexpr f32 cRotateDegree = 90.0f;
constexpr f32 cTiltOffsetY = 50.0f;
constexpr f32 cSignHeight = 100.0f;
constexpr f32 cSignAngleOffset = 2.3561945f;
constexpr f32 cRecoverJumpHeight = 1200.0f;

/**
 * @brief Returns how many steps a tumble (sign or rotation) lasts.
 * @param pBody Body whose damage count speeds the boss up.
 * @return 30 steps before the first hit, 25 afterwards.
 */
s32 getRotateStep(const BossWackunBody* pBody) {
    return pBody->getDamageCount() < 1 ? 30 : 25;
}


/**
 * @brief Returns the frame hit reaction played when the body lands.
 * @param type How the body tumbled.
 * @return Hit reaction name.
 */
const char* getLandHitReactionName(BossWackun::RotateType type) {
    switch (type) {
    case BossWackun::RotateType::FallFront:
    case BossWackun::RotateType::FallBack:
        return "倒れこみ着地";
    case BossWackun::RotateType::Side:
        return "横回転着地";
    case BossWackun::RotateType::Rise:
        return "起き上がり着地";
    default:
        return "起き上がり着地";
    }
}
/**
 * @brief Copies a vector as a plain copy of its storage.
 * @param pDst Destination vector.
 * @param rSrc Source vector.
 */
void copyVec(sead::Vector3f* pDst, const sead::Vector3f& rSrc) {
    pDst->e = rSrc.e;
}

/**
 * @brief Copies a quaternion as a plain copy of its storage.
 * @param pDst Destination quaternion.
 * @param rSrc Source quaternion.
 */
void copyQuat(sead::Quatf* pDst, const sead::Quatf& rSrc) {
    *static_cast<sead::BaseQuat<f32>*>(pDst) = rSrc;
}
}  // namespace

/**
 * @brief Creates the boss with all poses reset.
 * @param pName Actor name.
 */
BossWackun::BossWackun(const char* pName) : al::LiveActor(pName) {}

/** @brief Turns the dead switch on and removes the boss. */
void BossWackun::kill() {
    al::tryOnSwitchDeadOn(this);
    al::LiveActor::kill();
}

/**
 * @brief Creates the hand, body and frame sub actors, the demo state and the attack sign lights.
 * @param rInfo Actor placement and scene initialization information.
 */
void BossWackun::init(const al::ActorInitInfo& rInfo) {
    al::initSubActorKeeperNoFile(this, rInfo, 3);
    al::initActor(this, rInfo);
    copyQuat(&mInitQuat, al::getQuat(this));
    copyVec(&mInitTrans, al::getTrans(this));
    copyVec(&mPoseTrans, al::getTrans(this));
    copyQuat(&mRotateBaseQuat, al::getQuat(this));
    copyVec(&mRotateBaseTrans, mPoseTrans);
    copyVec(&mRotateCenter, al::getTrans(this));

    al::ByamlIter boxInfoIter(al::getBymlFromObjectResource("BossWackun", "BoxInfo"));
    al::tryGetByamlBox3f(&mBoxInfo, boxInfoIter);

    mHand = new BossWackunHand(this);
    al::initCreateActorWithPlacementInfo(mHand, rInfo);
    al::registerSubActorSyncClipping(this, mHand, false);
    mBody = new BossWackunBody(this, mHand);
    al::initCreateActorWithPlacementInfo(mBody, rInfo);
    al::registerSubActorSyncClipping(this, mBody, false);
    mFrame = new BossWackunFrame(this);
    al::initCreateActorWithPlacementInfo(mFrame, rInfo);
    al::registerSubActorSyncClipping(this, mFrame, false);

    mEffectMtxSetter = al::tryCreateEffectMtxSetter(mFrame, "EffectMtxSetter");
    if (mEffectMtxSetter != nullptr) {
        mEffectMtxSetter->setMtxPtr(&mLandEffectMtx, "LandEffectMtx");
    }

    al::initPrePassLightMtxConnector(mFrame, cSignLightName, &mLandEffectMtx);
    al::killPrePassLight(mFrame, cSignLightName, -1);
    al::initPrePassLightMtxConnector(mFrame, cSignLightSideName, &mLandEffectMtx);
    al::killPrePassLight(mFrame, cSignLightSideName, -1);
    al::initNerve(this, &NrvBossWackunWait, 1);
    mCameraInfo = al::initAnimCamera(mBody, rInfo);
    mDemoStartInfo = new BossDemoStartInfo(mCameraInfo, mBody, "DemoBattleStart", 60, nullptr);
    mStateDemoStart = new BossStateDemoStart(this, rInfo, mDemoStartInfo);
    al::initNerveState(this, mStateDemoStart, &NrvBossWackunDemoStart, "開始デモ");

    if (al::listenStageSwitchOnStart(this, al::Functor(this, &BossWackun::start))) {
        mBody->makeActorDead();
        mFrame->makeActorDead();
        al::setNerve(this, &NrvBossWackunWaitStart);
    }

    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    setRotatePose(0.0f);
    makeActorAppeared();
}

/** @brief Starts the opening demo once the start switch turns on. */
void BossWackun::start() {
    al::setNerve(this, &NrvBossWackunWaitDemoStart);
}

/**
 * @brief Rotates the pose around the current rotation axis and lifts it by the tilt.
 * @param degree Rotation angle from the rotation base pose, in degrees.
 */
void BossWackun::setRotatePose(f32 degree) {
    al::rotateQuatAndTransDegree(al::getQuatPtr(this), &mPoseTrans, mRotateBaseQuat,
                                 mRotateBaseTrans, mRotateAxis, mRotateCenter, degree);
    sead::Vector3f front;
    al::calcQuatFront(&front, al::getQuat(this));
    al::setTrans(this, mPoseTrans + sead::Vector3f(0.0f, sead::Mathf::abs(front.y) * cTiltOffsetY,
                                                   0.0f));
}

/** @brief Keeps the camera look-at position below the frame. */
void BossWackun::control() {
    mCameraLookAtPos = al::getTrans(mFrame);
    mCameraLookAtPos.y = mInitTrans.y - 400.0f;
}

/** @brief Waits for the start switch. */
void BossWackun::exeWaitStart() {}

/** @brief Waits a moment before the opening demo. */
void BossWackun::exeWaitDemoStart() {
    if (al::isGreaterEqualStep(this, 120)) {
        al::setNerve(this, &NrvBossWackunDemoStart);
    }
}

/** @brief Plays the opening demo and starts the boss music. */
void BossWackun::exeDemoStart() {
    if (al::isFirstStep(this)) {
        mBody->startDemo();
        mFrame->makeActorAppeared();
        al::startAction(mFrame, "DemoBattleStart");
    }

    if (al::isStep(this, 520)) {
        al::startBgm(this, "Boss", -1, 0, -1, -1);
    }

    s32 step = al::getNerveStep(this);
    if (al::updateNerveStateAndNextNerve(this, &NrvBossWackunBattleStart)) {
        if (step < 520) {
            al::startBgm(this, "Boss", -1, 0, -1, -1);
        }

        al::setAdditionalCameraLookAtPosPtr(this, this, &mCameraLookAtPos);
    }
}

/** @brief Starts the battle. */
void BossWackun::exeBattleStart() {
    if (al::isFirstStep(this)) {
        mBody->startBattle();
        al::startAction(mFrame, "Wait");
    }

    if (al::isGreaterEqualStep(this, 60)) {
        al::setNerve(this, &NrvBossWackunWait);
    }
}

/** @brief Chooses the next tumble towards the player and lets the body move to it. */
void BossWackun::exeWait() {
    if (al::isFirstStep(this)) {
        sead::Vector3f playerPos = al::findNearestPlayerPos(this);
        setRotateAxisAndPos(playerPos, mIsReverseNextRotate);
        mIsReverseNextRotate = false;
        s32 damageCount = mBody->getDamageCount();
        s32 moveNum = damageCount + 1;
        al::startAction(mFrame, "Wait");
        mBody->startMove(mRotateAxis, static_cast<s32>(mRotateType), moveNum,
                         damageCount < 1 ? 20 : 15,
                         damageCount < 1 ? 20 : (damageCount == 1 ? 15 : 10));
        mRotateDegree = 0.0f;
    }

    if (mBody->isDamage()) {
        mFrame->setDamage();
        al::setNerve(this, &NrvBossWackunDamage);
    } else if (mBody->isTurnEnd()) {
        al::setNerve(this, &NrvBossWackunRotateSign);
    }
}

/**
 * @brief Picks the bottom edge to tumble over, preferring the one facing the target.
 * @param rTargetPos Position to tumble towards.
 * @param isReverse Whether to tumble away from the target instead.
 * @return Whether a rotation axis was set.
 */
bool BossWackun::setRotateAxisAndPos(const sead::Vector3f& rTargetPos, bool isReverse) {
    sead::Vector3f toTarget = rTargetPos - mPoseTrans;
    if (isReverse) {
        toTarget = -toTarget;
    }

    sead::Vector3f toTargetDir;
    al::normalizeOrZero(&toTargetDir, toTarget);
    sead::Vector3f nearVec;
    al::Axis axis = al::calcNearVecFromAxis3(&nearVec, -sead::Vector3f::ey, al::getQuat(this));
    sead::Vector3f facePoints[4];
    al::calcBoxFacePoint(facePoints, mBoxInfo, static_cast<s32>(axis), al::getQuat(this),
                         mPoseTrans);

    sead::Vector3f edgeA = facePoints[1] - facePoints[0];
    sead::Vector3f edgeB = facePoints[2] - facePoints[1];
    al::normalizeOrZero(&edgeA);
    al::normalizeOrZero(&edgeB);

    if (al::isNearZero(edgeA, 0.001f)) {
        edgeA.setCross(edgeB, sead::Vector3f::ey);
        al::normalizeOrZero(&edgeA);
    }

    if (al::isNearZero(edgeB, 0.001f)) {
        edgeB.setCross(edgeA, sead::Vector3f::ey);
        al::normalizeOrZero(&edgeB);
    }

    RotateCandidate candidates[4];
    candidates[0].axis = edgeB;
    candidates[0].center = facePoints[1];
    candidates[0].normal = edgeA;
    candidates[1].axis = edgeB;
    candidates[1].center = facePoints[0];
    candidates[1].normal = -edgeA;
    candidates[2].axis = edgeA;
    candidates[2].center = facePoints[2];
    candidates[2].normal = edgeB;
    candidates[3].axis = edgeA;
    candidates[3].center = facePoints[1];
    candidates[3].normal = -edgeB;

    for (s32 i = 0; i < 4; i++) {
        RotateCandidate& candidate = candidates[i];
        if (candidate.axis.cross(candidate.normal).dot(sead::Vector3f::ey) > 0.0f) {
            candidate.axis = -candidate.axis;
        }

        al::rotateQuatAndTransDegree(nullptr, &candidate.trans, al::getQuat(this), mPoseTrans,
                                     candidate.axis, candidate.center, cRotateDegree);
    }

    sead::Vector3f prevNormal = mRotateNormal;
    sead::Vector3f front;
    al::calcFrontDir(&front, this);
    al::CollisionPartsFilterSpecialPurpose filter("BossWackun");
    f32 absFrontY = sead::Mathf::abs(front.y);
    bool isFound = false;
    s32 backIndex = -1;
    f32 bestScore = -2.0f;

    for (s32 i = 0; i < 4; i++) {
        const RotateCandidate& candidate = candidates[i];
        if (mIsRotateAxisValid && prevNormal.dot(candidate.normal) < -0.9f) {
            backIndex = i;
        } else {
            sead::Vector3f checkPos = sead::Vector3f::ey * cTiltOffsetY + candidate.trans;
            sead::Vector3f checkDir = sead::Vector3f::ey * -600.0f;
            if (alCollisionUtil::checkStrikeArrow(this, checkPos, checkDir, &filter, nullptr) != 0) {
                f32 score = toTargetDir.dot(candidate.normal);
                if (absFrontY < 0.1f && score > 0.0f &&
                    sead::Mathf::abs(front.dot(candidate.normal)) > 0.707f &&
                    sead::Mathf::abs(candidate.axis.dot(toTarget)) < mBoxInfo.getSizeX() * 0.5f) {
                    score = 2.0f;
                }

                if (score >= bestScore) {
                    mRotateAxis = candidate.axis;
                    mRotateCenter = candidate.center;
                    mRotateNormal = candidate.normal;
                    isFound = true;
                    mIsRotateAxisValid = true;
                    bestScore = score;
                }
            }
        }
    }

    if (backIndex >= 0) {
        if (!isFound) {
            const RotateCandidate& candidate = candidates[backIndex];
            mRotateAxis = candidate.axis;
            mRotateCenter = candidate.center;
            mRotateNormal = candidate.normal;
            isFound = true;
            mIsRotateAxisValid = true;
        }
    }

    if (absFrontY < 0.1f) {
        if (sead::Mathf::abs(front.dot(mRotateAxis)) > 0.9f) {
            mRotateType = RotateType::Side;
        } else {
            sead::Vector3f bodyFront;
            al::calcFrontDir(&bodyFront, mBody);
            mRotateType = bodyFront.z * mRotateAxis.x - mRotateAxis.z * bodyFront.x >= 0.0f ?
                              RotateType::FallFront :
                              RotateType::FallBack;
        }
    } else {
        mRotateType = RotateType::Rise;
    }

    return isFound;
}

/** @brief Shows where the body will land and wobbles before tumbling. */
void BossWackun::exeRotateSign() {
    if (al::isFirstStep(this)) {
        copyQuat(&mRotateBaseQuat, al::getQuat(this));
        copyVec(&mRotateBaseTrans, mPoseTrans);

        if (mRotateType == RotateType::FallFront || mRotateType == RotateType::FallBack) {
            sead::Quatf landQuat;
            sead::Vector3f landTrans;
            al::rotateQuatAndTransDegree(&landQuat, &landTrans, mRotateBaseQuat, mRotateBaseTrans,
                                         mRotateAxis, mRotateCenter, cRotateDegree);
            sead::Vector3f signDir;
            signDir.setRotated(landQuat, mBody->getSignLocalDir());
            al::normalizeOrZero(&signDir);
            f32 angle = std::atan2(signDir.x, signDir.z) + cSignAngleOffset;
            mLandEffectMtx.makeRT(sead::Vector3f(0.0f, angle, 0.0f),
                                  sead::Vector3f::ey * cSignHeight + landTrans);
            al::appearPrePassLight(mFrame, cSignLightName, 10);
            al::startAction(mFrame, "RotateSignFall");
        }

        if (mRotateType == RotateType::Side) {
            sead::Vector3f landTrans;
            al::rotateQuatAndTransDegree(nullptr, &landTrans, mRotateBaseQuat, mRotateBaseTrans,
                                         mRotateAxis, mRotateCenter, cRotateDegree);
            f32 offset = mBoxInfo.getSizeX() * 0.5f - cSignHeight;
            al::makeMtxFrontUpPos(&mLandEffectMtx, mRotateAxis, sead::Vector3f::ey,
                                  landTrans - sead::Vector3f::ey * offset);
            al::appearPrePassLight(mFrame, cSignLightSideName, 10);
            al::startAction(mFrame, "RotateSignSide");
        }

        if (mRotateType == RotateType::Rise) {
            sead::Vector3f landTrans;
            al::rotateQuatAndTransDegree(nullptr, &landTrans, mRotateBaseQuat, mRotateBaseTrans,
                                         mRotateAxis, mRotateCenter, cRotateDegree);
            f32 offset = mBoxInfo.getSizeX() * 0.5f - cSignHeight;
            al::makeMtxSideUpPos(&mLandEffectMtx, mRotateAxis, sead::Vector3f::ey,
                                 landTrans - sead::Vector3f::ey * offset);
            al::appearPrePassLight(mFrame, cSignLightSideName, 10);
            al::startAction(mFrame, "RotateSignRise");
        }
    }

    if (mBody->isDamage()) {
        mFrame->setDamage();
        al::setNerve(this, &NrvBossWackunDamageRotate);
        return;
    }

    f32 rate = al::calcNerveRate(this, getRotateStep(mBody));
    setRotatePose((0.5f - std::cos(rate * sead::Mathf::pi2()) * 0.5f) * 5.0f);

    if (al::isGreaterEqualStep(this, getRotateStep(mBody))) {
        mBody->startRotate(static_cast<s32>(mRotateType));
        al::setNerve(this, &NrvBossWackunRotate);
    }
}

/** @brief Tumbles the body over the chosen edge. */
void BossWackun::exeRotate() {
    if (al::isFirstStep(this)) {
        if (mRotateType == RotateType::FallFront || mRotateType == RotateType::FallBack) {
            al::startAction(mFrame, "RotateFall");
        }

        if (mRotateType == RotateType::Side) {
            al::startAction(mFrame, "RotateSide");
        }

        if (mRotateType == RotateType::Rise) {
            al::startAction(mFrame, "RotateRise");
        }

        copyQuat(&mRotateBaseQuat, al::getQuat(this));
        copyVec(&mRotateBaseTrans, mPoseTrans);
    }

    if (mBody->isDamage()) {
        mFrame->setDamage();
        al::setNerve(this, &NrvBossWackunDamageRotate);
        return;
    }

    f32 rate = al::calcNerveRate(this, getRotateStep(mBody));
    mRotateDegree = rate * rate * cRotateDegree;
    setRotatePose(mRotateDegree);

    if (al::isGreaterEqualStep(this, getRotateStep(mBody))) {
        mBody->startLand(static_cast<s32>(mRotateType));
        al::setNerve(this, &NrvBossWackunLand);
    }
}

/** @brief Lands on the new face, snapping the pose back onto the grid. */
void BossWackun::exeLand() {
    if (al::isFirstStep(this)) {
        al::startAction(mFrame, "Land");
        al::killPrePassLight(mFrame, cSignLightName, 20);
        al::killPrePassLight(mFrame, cSignLightSideName, 20);

        al::startHitReaction(mFrame, getLandHitReactionName(mRotateType));
        resetPoseError();
    }

    if (mBody->isDamage()) {
        mFrame->setDamage();
        al::setNerve(this, &NrvBossWackunDamage);
        return;
    }

    s32 landStep;
    switch (mRotateType) {
    case RotateType::Side:
    case RotateType::Rise:
        landStep = 20;
        break;
    default:
        landStep = 60;
        break;
    }

    if (al::isGreaterEqualStep(this, landStep)) {
        al::setNerve(this, &NrvBossWackunWait);
    }
}

/** @brief Snaps the pose to the box grid to remove accumulated rotation error. */
void BossWackun::resetPoseError() {
    al::calcFittingBoxPoseEqualAxisAll(al::getQuatPtr(this), al::getQuat(this), mInitQuat);
    al::snapVecToGrid(&mPoseTrans, mPoseTrans, cTiltOffsetY, mInitTrans);
}

/** @brief Rolls back (or finishes) the tumble after being hit mid-rotation. */
void BossWackun::exeDamageRotate() {
    if (al::isFirstStep(this)) {
        al::startAction(mFrame, "DamageRotate");
        sead::Vector3f bodyFront;
        al::calcFrontDir(&bodyFront, mBody);
        sead::Vector3f toPose = mPoseTrans - mRotateCenter;
        al::verticalizeVec(&toPose, mRotateAxis, toPose);
        mDamageDegree = bodyFront.cross(toPose).dot(mRotateAxis) > 0.0f ? 0.0f : cRotateDegree;
        al::killPrePassLight(mFrame, cSignLightName, 20);
        al::killPrePassLight(mFrame, cSignLightSideName, 20);
    }

    s32 step = static_cast<s32>(sead::Mathf::abs(mRotateDegree - mDamageDegree) / cRotateDegree *
                                30.0f);
    setRotatePose(al::calcNerveEaseInValue(this, step, mRotateDegree, mDamageDegree));

    if (al::isGreaterEqualStep(this, step)) {
        resetPoseError();
        al::startHitReaction(mFrame, "ダメージ着地");
        al::setNerve(this, &NrvBossWackunDamage);
    }
}

/** @brief Breaks the frame and either goes down or recovers. */
void BossWackun::exeDamage() {
    if (al::isFirstStep(this)) {
        al::startAction(mFrame, "Break");
        mHand->kill();
        al::startHitReactionBreak(mFrame);

        if (mBody->getDamageCount() >= 3) {
            al::stopBgm(this, "Boss", 20, -1);
        }
    }

    if (al::isGreaterEqualStep(this, 30)) {
        if (mBody->getDamageCount() >= 3) {
            al::setNerve(this, &NrvBossWackunDown);
        } else {
            al::setNerve(this, &NrvBossWackunRecoverStandUp);
        }
    }
}

/** @brief Lets the body stand back up. */
void BossWackun::exeRecoverStandUp() {
    if (al::isFirstStep(this)) {
        mBody->startStandUp();
    }

    if (al::isStep(this, 102)) {
        al::invalidateHitSensors(mBody);
        al::invalidateCollisionParts(mBody);
    }

    if (al::isActionEnd(mBody)) {
        al::setNerve(this, &NrvBossWackunRecoverJump);
    }
}

/** @brief Jumps the body up out of view. */
void BossWackun::exeRecoverJump() {
    if (al::isFirstStep(this)) {
        al::startAction(mBody, "Jump");
        copyQuat(&mRotateBaseQuat, al::getQuat(this));
        copyVec(&mRotateBaseTrans, al::getTrans(this));
    }

    f32 height = al::calcNerveEaseOutValue(this, 40, 0.0f, cRecoverJumpHeight);
    al::setTrans(this, mRotateBaseTrans + sead::Vector3f(0.0f, height, 0.0f));

    if (al::isGreaterEqualStep(this, 40)) {
        al::setNerve(this, &NrvBossWackunRecoverFrame);
    }
}

/** @brief Restores the frame while the body is up in the air. */
void BossWackun::exeRecoverFrame() {
    if (al::isFirstStep(this)) {
        al::startAction(mBody, "Recover");
        mFrame->revival();
    }

    if (al::isActionEnd(mBody) && al::isActionEnd(mFrame)) {
        al::setNerve(this, &NrvBossWackunRecoverRotate);
    }
}

/** @brief Spins the body around while in the air. */
void BossWackun::exeRecoverRotate() {
    f32 degree = al::calcNerveEaseInOutValue(this, 45, 0.0f, 540.0f);
    al::rotateQuatXDirDegree(this, mRotateBaseQuat, degree);

    if (al::isGreaterEqualStep(this, 45)) {
        al::setNerve(this, &NrvBossWackunRecoverFallSign);
    }
}

/** @brief Shows where the body will fall back down. */
void BossWackun::exeRecoverFallSign() {
    if (al::isFirstStep(this)) {
        mBody->startRevival();
        al::startAction(mFrame, "RecoverFallSign");
        sead::Vector3f signDir;
        signDir.setRotated(al::getQuat(this), mBody->getSignLocalDir());
        al::normalizeOrZero(&signDir);
        f32 angle = std::atan2(signDir.x, signDir.z) + cSignAngleOffset;
        mLandEffectMtx.makeRT(sead::Vector3f(0.0f, angle, 0.0f),
                              sead::Vector3f::ey * cSignHeight + mRotateBaseTrans);
        al::appearPrePassLight(mFrame, cSignLightName, 10);
    }

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvBossWackunRecoverFall);
    }
}

/** @brief Drops the body back onto the frame and turns on the damage switches. */
void BossWackun::exeRecoverFall() {
    if (al::isFirstStep(this)) {
        al::startAction(mBody, "RecoverAttack");
        al::startAction(mFrame, "RecoverFall");
    }

    f32 height = al::calcNerveEaseInValue(this, 35, cRecoverJumpHeight, 0.0f);
    al::setTrans(this, mRotateBaseTrans + sead::Vector3f(0.0f, height, 0.0f));

    if (al::isGreaterEqualStep(this, 35)) {
        if (mBody->getDamageCount() == 1) {
            al::tryOnStageSwitch(this, "SwitchDamageFirstOn");
        }

        if (mBody->getDamageCount() == 2) {
            al::tryOnStageSwitch(this, "SwitchDamageSecondOn");
        }

        al::startHitReaction(mFrame, "復活着地");
        al::killPrePassLight(mFrame, cSignLightName, 20);
        al::setNerve(this, &NrvBossWackunRecoverLand);
    }
}

/** @brief Lands after recovering and resumes the battle. */
void BossWackun::exeRecoverLand() {
    if (al::isFirstStep(this)) {
        al::startAction(mBody, "LandBack");
        al::startAction(mFrame, "RecoverLand");
    }

    if (al::isGreaterEqualStep(this, 45)) {
        mIsRotateAxisValid = false;
        al::setNerve(this, &NrvBossWackunWait);
    }
}

/** @brief Defeats the boss: plays the down action, then removes every part. */
void BossWackun::exeDown() {
    if (al::isFirstStep(this)) {
        al::tryOnStageSwitch(this, "SwitchDownStartOn");
        mBody->startDown(sead::Vector3f(0.70710677f, 0.0f, 0.70710677f));
        al::invalidateCollisionParts(mBody);
    }

    if (al::isActionEnd(mBody)) {
        al::setAdditionalCameraLookAtPosPtr(this, this, nullptr);
        al::startBgm(this, "AfterBattle", -1, 0, -1, -1);
        mFrame->kill();
        mBody->kill();
        kill();
    }
}

/** @brief Destroys the boss actor's base resources. */
BossWackun::~BossWackun() = default;
