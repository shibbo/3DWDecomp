#include "Enemy/SamboSnowHead.hpp"
#include "Enemy/ActorStateSupportFreeze.hpp"
#include "Enemy/EnemyStateUtil.hpp"
#include "Enemy/SamboSnowBody.hpp"
#include "Enemy/SamboSnowHat.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/BallSnow.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/InkUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

// Nerve whose host also has an end callback.
#define NERVE_END_DECL(Class, Action)                                                              \
    class Class##Nrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<Class>())->exe##Action();                                          \
        }                                                                                          \
                                                                                                   \
        void executeOnEnd(al::NerveKeeper* pKeeper) const override {                               \
            (pKeeper->getParent<Class>())->end##Action();                                          \
        }                                                                                          \
    };

// Non-const nerve object: the nerves are merged into one data block, so neighbouring nerves are
// addressed relative to each other (e.g. SupportFreeze as Wait + 8).
#define NERVE_MAKE_MUTABLE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(SamboSnowHead, Wait)
NERVE_END_DECL(SamboSnowHead, SupportFreeze)
NERVE_DECL(SamboSnowHead, BlowDown)
NERVE_DECL(SamboSnowHead, FireDown)
NERVE_DECL(SamboSnowHead, Trample)
NERVE_DECL(SamboSnowHead, BodyAttacked)
NERVE_DECL(SamboSnowHead, Search)
NERVE_DECL(SamboSnowHead, Find)
NERVE_DECL(SamboSnowHead, Attack)
NERVE_DECL(SamboSnowHead, HipDrop)
NERVE_DECL(SamboSnowHead, TouchDown)
NERVE_END_DECL(SamboSnowHead, Chase)
NERVE_DECL(SamboSnowHead, GiveUpChase)
NERVE_DECL(SamboSnowHead, Land)
FOR_EACH(NERVE_MAKE_MUTABLE, SamboSnowHead, Wait, SupportFreeze, BlowDown, FireDown, Trample,
         BodyAttacked, Search, Find, Attack, HipDrop, TouchDown, Chase, GiveUpChase, Land)

ActorStateSupportFreezeParam cSupportFreezeParam(true, 15, false, true, 120,
                                                 sead::Vector3f(0.0f, 150.0f, 0.0f));

const f32 cBodyHeight = 97.0f;
const f32 cBodyTwistDegree = 45.0f;
const f32 cSearchRadius = 1700.0f;
const f32 cGiveUpDistance = 2200.0f;
}  // namespace

/** @brief Constructs a snow Pokey head.
 * @param pName Actor name.
 */
SamboSnowHead::SamboSnowHead(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the snow ball, body stack and hat, and stacks the head on top of the bodies.
 * @param rInfo Actor placement and scene information.
 */
void SamboSnowHead::init(const al::ActorInitInfo& rInfo) {
    al::initActorChangeModel(this, rInfo);
    mGroundY = al::getTrans(this).y;
    mStateSupportFreeze = new ActorStateSupportFreeze(this, &cSupportFreezeParam);
    al::initNerve(this, &NrvSamboSnowHeadWait, 1);
    al::initNerveState(this, mStateSupportFreeze, &NrvSamboSnowHeadSupportFreeze,
                       "[state]フリーズ");

    s32 groundType = 0;
    al::tryGetArg(&groundType, rInfo, "GroundType");
    mIsGroundSnow = groundType == 0;
    mBallSnow = new BallSnow("雪玉");
    al::initCreateActorNoPlacementInfo(mBallSnow, rInfo);
    mBallSnow->makeActorDead();
    al::tryGetArg(&mBodyNum, rInfo, "BodyNum");
    al::initSubActorKeeperNoFile(this, rInfo, mBodyNum);
    mBodies = new SamboSnowBody*[mBodyNum];
    sead::Quatf quat = al::getQuat(this);
    for (s32 i = 0; i < mBodyNum; i++) {
        mBodies[i] = new SamboSnowBody("雪サンボ胴体");
        mBodies[i]->init(rInfo);
        al::setTrans(mBodies[i], al::getTrans(this) + sead::Vector3f::ey * cBodyHeight * i);
        sead::Quatf bodyQuat;
        al::rotateQuatYDirDegree(&bodyQuat, quat, i * cBodyTwistDegree);
        al::setQuat(mBodies[i], bodyQuat);
        mBodies[i]->mHeadIsStacked = &mIsStacked;
        mBodies[i]->mHeadIsRequestAttack = &mIsRequestAttack;
        mBodies[i]->mHeadIsRequestBlowDown = &mIsRequestBlowDown;
        mBodies[i]->mHeadSupportFreezeActor = &mSupportFreezeActor;
        al::registerSubActorSyncClipping(this, mBodies[i], false);
    }

    al::setTrans(this, al::getTrans(this) + sead::Vector3f::ey * cBodyHeight * mBodyNum);
    mHat = new SamboSnowHat("帽子");
    mHat->init(rInfo);
    al::setClippingInfo(this, mBodyNum * cBodyHeight + 100.0f, nullptr);
    mPushSensorPos = al::getTrans(this);
    f32 height = mBodyNum * cBodyHeight;
    mPushSensorPos.y = mGroundY + height * 0.5f;
    al::setSensorRadius(this, "Push", height);
    al::setHitSensorPosPtr(this, "Push", &mPushSensorPos);
    if (mIsGroundSnow) {
        al::setEffectFollowMtxPtr(this, "TraceSnow", &mEffectMtx);
    } else {
        al::setEffectFollowMtxPtr(this, "TraceIce", &mEffectMtx);
    }

    mChaseArea = al::createLinkAreaGroup(this, rInfo, "ChaseArea", "追いかけ有効エリアグループ",
                                         "子供エリア");
    al::createAndSetColliderSpecialPurpose(this, "MoveLimit");
    makeActorAppeared();
    mInitQuat = al::getQuat(this);
    mIsSingleMode = rInfo.getActorSceneInfo().isSingleMode;
}

/** @brief Keeps the body stack aligned under the head and reacts to requests from the bodies. */
void SamboSnowHead::control() {
    if (al::isNerve(this, &NrvSamboSnowHeadBlowDown) ||
        al::isNerve(this, &NrvSamboSnowHeadFireDown) ||
        al::isNerve(this, &NrvSamboSnowHeadTrample) ||
        al::isNerve(this, &NrvSamboSnowHeadBodyAttacked)) {
        return;
    }

    if (mIsSingleMode && InkUtil::isInInkLimitSphere(this)) {
        if (!al::isNerve(this, &NrvSamboSnowHeadBlowDown)) {
            al::setNerve(this, &NrvSamboSnowHeadBlowDown);
            requestBodyBlowDown();
        }
        return;
    }

    if (mIsRequestBlowDown) {
        sead::Vector3f dir = al::getTrans(this) - al::findNearestPlayerPos(this);
        al::setVelocitySeparateHV(this, dir, 8.0f, 30.0f);
        al::startAction(this, "BlowDown");
        if (!mIsHatBlown) {
            al::setTrans(mHat, al::getTrans(this));
            al::rotateVectorDegreeY(&dir, 60.0f);
            al::setVelocitySeparateHV(mHat, dir, 8.0f, 30.0f);
            al::faceToDirection(mHat, dir);
        }

        mBlowDownVelocity = al::getVelocity(this);
        al::setNerve(this, &NrvSamboSnowHeadBlowDown);
        requestBodyBlowDown();
        return;
    }

    s32 aliveNum = calcAliveBodyNum();
    mPushSensorPos = al::getTrans(this);
    f32 height = aliveNum * cBodyHeight;
    mPushSensorPos.y = mGroundY + height * 0.5f;
    al::setSensorRadius(this, "Push", height);

    if (!mIsStacked) {
        for (s32 i = 0; i < mBodyNum; i++) {
            if (!al::isDead(mBodies[i]) && mBodies[i]->isAttacked()) {
                mAttackedBodyIndex = i;
                break;
            }
        }

        mLandY = mGroundY;
        s32 lowerNum = 0;
        for (s32 i = 0; i < mAttackedBodyIndex; i++) {
            lowerNum += al::isAlive(mBodies[i]);
        }

        mLandY = lowerNum * cBodyHeight + mGroundY;
        al::setNerve(this, &NrvSamboSnowHeadBodyAttacked);
        return;
    }

    sead::Vector3f base = al::getTrans(this);
    base.y = mGroundY;
    s32 index = 0;
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            al::setTrans(mBodies[i], base + sead::Vector3f::ey * cBodyHeight * index);
            index++;
        }
    }

    al::setTrans(this, base + sead::Vector3f::ey * cBodyHeight * index);

    if (!al::isNerve(this, &NrvSamboSnowHeadSearch) &&
        !al::isNerve(this, &NrvSamboSnowHeadFind)) {
        sead::Quatf quat = al::getQuat(this);
        s32 twistIndex = 0;
        for (s32 i = mBodyNum - 1; i >= 0; i--) {
            if (!isBodyDeadOrAttacked(i)) {
                twistIndex++;
                sead::Quatf bodyQuat;
                al::rotateQuatYDirDegree(&bodyQuat, quat, twistIndex * cBodyTwistDegree);
                al::setQuat(mBodies[i], bodyQuat);
            }
        }
    }

    if (al::isNerve(this, &NrvSamboSnowHeadAttack)) {
        return;
    }

    if (mIsRequestAttack) {
        al::setNerve(this, &NrvSamboSnowHeadAttack);
        return;
    }

    if (isEnableSupportFreeze() && mSupportFreezeActor != nullptr) {
        mStateSupportFreeze->forceSetTouchActor(mSupportFreezeActor);
        al::setNerve(this, &NrvSamboSnowHeadSupportFreeze);
        mSupportFreezeActor = nullptr;
    }
}

/** @brief Blows every remaining body away from the nearest player, fanning out their directions. */
void SamboSnowHead::requestBodyBlowDown() {
    sead::Vector3f dir = al::getTrans(this) - al::findNearestPlayerPos(this);
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            al::rotateVectorDegreeY(&dir, 144.0f);
            al::setVelocitySeparateHV(mBodies[i], dir, 8.0f, 30.0f);
            mBodies[i]->requestBlowDown();
        }
    }
}

/** @brief Counts the bodies that are alive and not knocked out of the stack.
 * @return Number of bodies still in the stack.
 */
s32 SamboSnowHead::calcAliveBodyNum() {
    s32 num = 0;
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            num++;
        }
    }

    return num;
}

/** @brief Checks whether a body has left the stack.
 * @param index Body index, counted from the bottom.
 * @return True if the body is dead or has been attacked.
 */
bool SamboSnowHead::isBodyDeadOrAttacked(s32 index) {
    if (al::isDead(mBodies[index])) {
        return true;
    }

    return mBodies[index]->isAttacked();
}

/** @brief Checks whether the head may currently be frozen by a touch.
 * @return True while waiting, searching or chasing.
 */
bool SamboSnowHead::isEnableSupportFreeze() const {
    return al::isNerve(this, &NrvSamboSnowHeadWait) ||
           al::isNerve(this, &NrvSamboSnowHeadSearch) ||
           al::isNerve(this, &NrvSamboSnowHeadChase);
}

/** @brief Appears, also making the bodies appear in single-player mode. */
void SamboSnowHead::appear() {
    al::LiveActor::appear();
    if (mIsSingleMode) {
        for (s32 i = 0; i < mBodyNum; i++) {
            mBodies[i]->appear();
        }
    }
}

/** @brief Rebuilds the whole stack at the placement position with the hat on. */
void SamboSnowHead::reappear() {
    al::setNerve(this, &NrvSamboSnowHeadWait);
    al::setNerve(mHat, &NrvSamboSnowHeadWait);
    al::startAction(this, "Wait");
    mBallSnow->makeActorDead();
    al::setQuat(this, mInitQuat);
    al::setTransY(this, mGroundY);
    sead::Vector3f base = al::getTrans(this);
    for (s32 i = 0; i < mBodyNum; i++) {
        al::setTrans(mBodies[i], base + sead::Vector3f::ey * cBodyHeight * i);
        sead::Quatf bodyQuat;
        al::rotateQuatYDirDegree(&bodyQuat, mInitQuat, i * cBodyTwistDegree);
        al::setQuat(mBodies[i], bodyQuat);
        mBodies[i]->reappear();
    }

    al::setTrans(this, base + sead::Vector3f::ey * cBodyHeight * mBodyNum);
    al::tryStartVisAnimIfExist(this, "AppearHat");
    mIsStacked = true;
    mIsRequestAttack = false;
    mIsRequestBlowDown = false;
    al::LiveActor::appear();
}

/** @brief Kills the bodies and the snow ball, releasing the ball from a player first.
 * @param isForce Unused.
 */
void SamboSnowHead::killComplete(bool isForce) {
    for (s32 i = 0; i < mBodyNum; i++) {
        mBodies[i]->killComplete(false);
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this)) &&
        mBallSnow->isPlayerHold()) {
        mBallSnow->requestPlayerRelease();
    }

    mBallSnow->makeActorDead();
    kill();
}

/** @brief Pushes enemies and objects and attacks players with the attack sensor.
 * @param pSelf Own sensor.
 * @param pOther Touched sensor.
 */
void SamboSnowHead::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (isDown()) {
        return;
    }

    if (al::isSensorEnemyBody(pSelf) && al::isSensorName(pOther, "Push")) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSelf, pOther);
        return;
    }

    if (al::isSensorEnemyBody(pOther) || al::isSensorNpc(pOther)) {
        al::sendMsgPush(pOther, pSelf);
        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) {
        if (al::isSensorHostName(pOther, "コウラ") || al::isSensorHostName(pOther, "BallNeko")) {
            al::sendMsgPushStrong(pOther, pSelf);
        } else if ((al::isSensorName(pSelf, "Attack") &&
                    (al::isSensorHostName(pOther, "雪玉") ||
                     al::isSensorHostName(pOther, "Spinner"))) ||
                   (al::isSensorMapObj(pOther) && al::isSensorEnemyBody(pSelf))) {
            al::sendMsgPush(pOther, pSelf);
        }
    }

    if ((al::isSensorPlayer(pOther) || al::isSensorKoopaJr(pOther)) &&
        al::isSensorName(pSelf, "Attack")) {
        al::sendMsgPush(pOther, pSelf);
        if (al::sendMsgEnemyAttackForCrossoverSensor(pOther, pSelf)) {
            mIsRequestAttack = true;
        }
    }
}

/** @brief Checks whether the head is being knocked down.
 * @return True while blown down, trampled, hip-dropped, burnt or touched down.
 */
bool SamboSnowHead::isDown() const {
    return al::isNerve(this, &NrvSamboSnowHeadBlowDown) ||
           al::isNerve(this, &NrvSamboSnowHeadTrample) ||
           al::isNerve(this, &NrvSamboSnowHeadHipDrop) ||
           al::isNerve(this, &NrvSamboSnowHeadFireDown) ||
           al::isNerve(this, &NrvSamboSnowHeadTouchDown);
}

/** @brief Handles pushes, tramples, hip drops, fireballs and blow-down attacks.
 * @param pMsg Received message.
 * @param pOther Sending sensor.
 * @param pSelf Receiving sensor.
 * @return True if the message was handled.
 */
bool SamboSnowHead::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                               al::HitSensor* pSelf) {
    if (isDown() || al::isNerve(this, &NrvSamboSnowHeadBodyAttacked)) {
        return false;
    }

    if (!mIsStacked) {
        return false;
    }

    if (al::isSensorName(pSelf, "Push") &&
        al::isHitCylinderSensor(pOther, pSelf, sead::Vector3f::ey, 70.0f) &&
        al::tryReceiveMsgPushAndAddVelocity(this, pMsg, pOther, pSelf, 1.0f)) {
        return true;
    }

    if (al::isSensorName(pSelf, "Push")) {
        return false;
    }

    if (al::isMsgTrampleAll(pMsg)) {
        sead::Vector3f dir = al::getTrans(this) - al::getSensorPos(pOther);
        al::setVelocitySeparateHV(this, dir, 8.0f, 30.0f);
        al::startAction(this, "BlowDown");
        tryBlowHat(dir, true);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvSamboSnowHeadTrample);
        return true;
    }

    if (al::isMsgPlayerObjHipDropAll(pMsg)) {
        sead::Vector3f dir = al::getTrans(this) - al::getSensorPos(pOther);
        tryBlowHat(dir, false);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvSamboSnowHeadHipDrop);
        return true;
    }

    if (al::isMsgPlayerFireBallAttack(pMsg)) {
        sead::Vector3f dir = al::getTrans(this) - al::getSensorPos(pOther);
        tryBlowHat(dir, false);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::setAppearItemFactorByMsg(this, pMsg, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        al::setNerve(this, &NrvSamboSnowHeadFireDown);
        return true;
    }

    if (EnemyStateUtil::isMsgBlowDown(pMsg)) {
        sead::Vector3f dir = al::getTrans(this) - al::getSensorPos(pOther);
        al::setVelocitySeparateHV(this, dir, 8.0f, 30.0f);
        al::startAction(this, "BlowDown");
        tryBlowHat(dir, true);
        rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        rc::addScoreCombo(this, pOther, pMsg, 100.0f);
        mBlowDownVelocity = al::getVelocity(this);
        al::setNerve(this, &NrvSamboSnowHeadBlowDown);
        return true;
    }

    return false;
}

/** @brief Knocks the hat off in the given direction unless it is already gone.
 * @param rDir Horizontal direction to blow the hat in.
 * @param isRotate Whether to turn the direction by 60 degrees first.
 */
void SamboSnowHead::tryBlowHat(const sead::Vector3f& rDir, bool isRotate) {
    if (mIsHatBlown) {
        return;
    }

    al::setTrans(mHat, al::getTrans(this));
    sead::Vector3f dir = rDir;
    if (isRotate) {
        al::rotateVectorDegreeY(&dir, 60.0f);
    }

    al::setVelocitySeparateHV(mHat, dir, 8.0f, 30.0f);
    al::faceToDirection(mHat, dir);
}

/** @brief Handles touches: first knocks the hat off, then blows the head down; also freezes.
 * @param pMsg Received message.
 * @param pPointer Touching screen pointer.
 * @param pTarget Touched screen point target.
 * @return True if the message was handled.
 */
bool SamboSnowHead::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                          al::ScreenPointTarget* pTarget) {
    if (isDown() || al::isNerve(this, &NrvSamboSnowHeadBodyAttacked)) {
        return false;
    }

    if (!mIsStacked) {
        return false;
    }

    if (al::isMsgTouchAssistTrig(pMsg)) {
        if (!mIsHatBlown) {
            sead::Vector3f dir = al::getTrans(this) - al::getHitScreenPointTargetPos(pPointer);
            tryBlowHat(dir, false);
            al::setNerve(this, &NrvSamboSnowHeadTouchDown);
            return true;
        }

        if (!al::isNerve(this, &NrvSamboSnowHeadTouchDown)) {
            sead::Vector3f dir = al::getTrans(this) - al::getHitScreenPointTargetPos(pPointer);
            al::setVelocitySeparateHV(this, dir, 8.0f, 30.0f);
            al::startAction(this, "BlowDown");
            rc::addScoreCombo(this, pPointer, pMsg, 100.0f);
            mBlowDownVelocity = al::getVelocity(this);
            al::setNerve(this, &NrvSamboSnowHeadBlowDown);
            requestBodyBlowDown();
            return true;
        }
    }

    if (mStateSupportFreeze->receiveMsgScreenPoint(pMsg, pPointer, pTarget)) {
        if (isEnableSupportFreeze()) {
            al::setNerve(this, &NrvSamboSnowHeadSupportFreeze);
        }

        return true;
    }

    return false;
}

/** @brief Idles until a player comes near inside the chase area. */
void SamboSnowHead::exeWait() {
    if (al::isFirstStep(this)) {
        setVelocityZeroHeadAndBody();
        al::startAction(this, "Wait");
        startBodyAction("Wait", 50);
    }

    mTarget = rc::tryFindNearestActivePlayerActorInSphere(this, cSearchRadius);
    if (mTarget != nullptr && isTargetInChaseArea()) {
        al::setNerve(this, &NrvSamboSnowHeadSearch);
    }
}

/** @brief Stops the head and every body still in the stack. */
void SamboSnowHead::setVelocityZeroHeadAndBody() {
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            al::setVelocityZero(mBodies[i]);
        }
    }

    al::setVelocityZero(this);
}

/** @brief Starts an action on the stacked bodies, offsetting the frame of each one.
 * @param pActionName Action name.
 * @param frameInterval Frame offset between neighbouring bodies, from the top.
 */
void SamboSnowHead::startBodyAction(const char* pActionName, s32 frameInterval) {
    s32 index = 0;
    for (s32 i = mBodyNum - 1; i >= 0; i--) {
        if (!isBodyDeadOrAttacked(i)) {
            index++;
            al::startAction(mBodies[i], pActionName);
            al::setSklAnimFrame(mBodies[i], index * frameInterval, 0);
        }
    }
}

/** @brief Checks whether the target is inside the linked chase area.
 * @return True if so, or if there is no chase area.
 */
bool SamboSnowHead::isTargetInChaseArea() const {
    if (mChaseArea == nullptr) {
        return true;
    }

    return al::tryIsInAreaObj(mChaseArea, al::getTrans(mTarget));
}

/** @brief Turns towards the target, with the bodies following one after another. */
void SamboSnowHead::exeSearch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Move");
        startBodyAction("Move", 50);
    }

    f32 turnDegree = 5.0f;
    if (al::getNerveStep(this) <= 5) {
        f32 rate = al::easeIn(al::getNerveStep(this) / 5.0f);
        turnDegree = sead::Mathf::clamp(rate, 0.0f, 1.0f) * 5.0f;
    }

    al::turnToTarget(this, mTarget, turnDegree);
    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    s32 index = 0;
    for (s32 i = mBodyNum - 1; i >= 0; i--) {
        if (!isBodyDeadOrAttacked(i)) {
            index++;
            if (al::getNerveStep(this) > index * 5) {
                sead::Vector3f dir = front;
                al::rotateVectorDegreeY(&dir, index * cBodyTwistDegree);
                al::turnToDirection(mBodies[i], dir, 5.0f);
            }
        }
    }

    f32 speed = al::calcSpeedH(this);
    if (mIsGroundSnow) {
        al::holdSeWithParam(this, "PgMove", speed);
    } else {
        al::holdSeWithParam(this, "PgMoveIce", speed);
    }

    if (al::isFaceToTargetDegreeH(this, al::getTrans(mTarget), front, 10.0f) &&
        isTargetInChaseArea()) {
        al::setNerve(this, &NrvSamboSnowHeadFind);
        return;
    }

    if (al::isFar(this, mTarget, cGiveUpDistance) || al::isGreaterEqualStep(this, 450) ||
        !isTargetInChaseArea()) {
        al::setNerve(this, &NrvSamboSnowHeadWait);
    }
}

/** @brief Plays the reaction on finding the target, then starts chasing. */
void SamboSnowHead::exeFind() {
    if (al::isFirstStep(this)) {
        setVelocityZeroHeadAndBody();
        al::startAction(this, "Find");
        startBodyAction("Find", 0);
    }

    if (al::isStep(this, 38) && !mIsHatBlown) {
        al::startSe(this, "PgBucketLand");
    }

    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    s32 index = 0;
    for (s32 i = mBodyNum - 1; i >= 0; i--) {
        if (!isBodyDeadOrAttacked(i)) {
            index++;
            sead::Vector3f dir = front;
            al::rotateVectorDegreeY(&dir, index * cBodyTwistDegree);
            al::turnToDirection(mBodies[i], dir, 5.0f);
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvSamboSnowHeadChase);
    }
}

/** @brief Slides after the target until it gets away, stopping in front of obstacles. */
void SamboSnowHead::exeChase() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Move");
        startBodyAction("Move", 50);
        if (mIsGroundSnow) {
            al::tryEmitEffect(this, "TraceSnow", nullptr);
        } else {
            al::tryEmitEffect(this, "TraceIce", nullptr);
        }
    }

    f32 speed = al::calcSpeedH(this);
    if (mIsGroundSnow) {
        al::holdSeWithParam(this, "PgMove", speed);
    } else {
        al::holdSeWithParam(this, "PgMoveIce", speed);
    }

    f32 turnDegree = 5.0f;
    if (al::getNerveStep(this) <= 5) {
        f32 rate = al::easeIn(al::getNerveStep(this) / 5.0f);
        turnDegree = sead::Mathf::clamp(rate, 0.0f, 1.0f) * 5.0f;
    }

    al::turnToTarget(this, mTarget, turnDegree);
    if (al::isDead(mTarget)) {
        al::setNerve(this, &NrvSamboSnowHeadGiveUpChase);
        return;
    }

    if (checkForwardObstacle()) {
        if (!mIsStopSePlayed) {
            al::startSeWithParam(this, "Stop", al::calcSpeedH(this));
            mIsStopSePlayed = true;
        }

        al::setVelocityZero(this);
    } else {
        sead::Vector3f front;
        al::calcQuatFront(&front, this);
        al::addVelocity(this, front * 0.2f);
        al::scaleVelocity(this, 0.97f);
        mIsStopSePlayed = false;
    }

    calcEffectMtx();
    if (al::isFar(this, mTarget, cGiveUpDistance) || al::isGreaterEqualStep(this, 450) ||
        !isTargetInChaseArea()) {
        al::setNerve(this, &NrvSamboSnowHeadGiveUpChase);
    }
}

/** @brief Checks for a wall in front of the head or a missing floor in front of the stack.
 * @return True if the head should not move forward.
 */
bool SamboSnowHead::checkForwardObstacle() {
    sead::Vector3f front;
    al::calcQuatFront(&front, this);
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f spherePos = front * 30.0f + trans;
    spherePos.y = mGroundY + 100.0f;
    if (alCollisionUtil::checkStrikeSphere(this, spherePos, 70.0f, nullptr, nullptr) != 0) {
        return true;
    }

    sead::Vector3f hitPos = sead::Vector3f::zero;
    al::Triangle triangle;
    sead::Vector3f side;
    al::calcQuatSide(&side, this);
    const sead::Vector3f& floorTrans = al::getTrans(this);
    sead::Vector3f center = front * 100.0f + floorTrans;
    center.y = mGroundY;
    sead::Vector3f right = center + side * 60.0f;
    sead::Vector3f left = center - side * 60.0f;
    if (!alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle,
                                              right + sead::Vector3f::ey * 100.0f,
                                              sead::Vector3f::ey * -150.0f, nullptr, nullptr)) {
        return true;
    }

    if (!alCollisionUtil::getFirstPolyOnArrow(this, &hitPos, &triangle,
                                              sead::Vector3f::ey * 100.0f + left,
                                              sead::Vector3f::ey * -150.0f, nullptr, nullptr)) {
        return true;
    }

    const sead::Vector3f& wallTrans = al::getTrans(this);
    sead::Vector3f wallCenter = front * 100.0f + wallTrans;
    sead::Vector3f sideOffset = side * 60.0f;
    if (alCollisionUtil::getFirstPolyOnArrow(
            this, &hitPos, &triangle, right,
            wallCenter + sideOffset + sead::Vector3f::ey * 150.0f - right, nullptr, nullptr)) {
        return true;
    }

    return alCollisionUtil::getFirstPolyOnArrow(
        this, &hitPos, &triangle, left,
        wallCenter - sideOffset + sead::Vector3f::ey * 150.0f - left, nullptr, nullptr);
}

/** @brief Places the trace effect matrix at the base of the stack. */
void SamboSnowHead::calcEffectMtx() {
    mEffectMtx = *getBaseMtx();
    const sead::Vector3f& trans = al::getTrans(this);
    sead::Vector3f pos(trans.x, mGroundY, trans.z);
    mEffectMtx.setTranslation(pos);
}

/** @brief Stops the trace effect when the chase ends. */
void SamboSnowHead::endChase() {
    if (mIsGroundSnow) {
        al::tryDeleteEffect(this, "TraceSnow");
    } else {
        al::tryDeleteEffect(this, "TraceIce");
    }
}

/** @brief Slides to a halt, then searches again or goes back to waiting. */
void SamboSnowHead::exeGiveUpChase() {
    if (checkForwardObstacle()) {
        al::setVelocityZero(this);
    } else {
        al::scaleVelocity(this, 0.97f);
    }

    calcEffectMtx();
    if (al::isGreaterEqualStep(this, 10)) {
        mTarget = rc::tryFindNearestActivePlayerActorInSphere(this, cSearchRadius);
        if (mTarget != nullptr) {
            al::setNerve(this, &NrvSamboSnowHeadSearch);
        } else {
            al::setNerve(this, &NrvSamboSnowHeadWait);
        }
    }
}

/** @brief Plays the successful attack reaction facing the victim. */
void SamboSnowHead::exeAttack() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "AttackSuccess");
        startBodyAction("AttackSuccess", 0);
        setVelocityZeroHeadAndBody();
        if (mIsSingleMode) {
            const al::LiveActor* pTarget =
                rc::tryFindNearestActivePlayerOrKoopaJrActorInSphere(this, cSearchRadius);
            if (pTarget != nullptr) {
                al::faceToTarget(this, pTarget);
            }
        } else if (mTarget != nullptr) {
            al::faceToTarget(this, mTarget);
        }
    }

    if (al::isActionEnd(this)) {
        mIsRequestAttack = false;
        mTarget = rc::tryFindNearestActivePlayerActorInSphere(this, cSearchRadius);
        if (mTarget != nullptr) {
            al::setNerve(this, &NrvSamboSnowHeadChase);
        } else {
            al::setNerve(this, &NrvSamboSnowHeadWait);
        }
    }
}

/** @brief Drops the part of the stack above the attacked body until it lands on the rest. */
void SamboSnowHead::exeBodyAttacked() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "BodyAttack");
        al::setVelocityZero(this);
        for (s32 i = mAttackedBodyIndex + 1; i < mBodyNum; i++) {
            al::setVelocityY(mBodies[i], 15.0f);
        }

        al::setVelocityY(this, 15.0f);
    }

    al::LiveActor* pLowest = this;
    s32 lowestIndex = mBodyNum;
    for (s32 i = mAttackedBodyIndex + 1; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            if (i < lowestIndex) {
                lowestIndex = i;
                pLowest = mBodies[i];
            }

            al::addVelocityToGravity(mBodies[i], 1.5f);
        }
    }

    al::addVelocityToGravity(this, 1.5f);
    if (al::getTrans(pLowest).y + al::getVelocity(pLowest).y < mLandY) {
        mIsStacked = true;
        al::setNerve(this, &NrvSamboSnowHeadLand);
    }
}

/** @brief Lands back on the stack, then resumes the chase or waits. */
void SamboSnowHead::exeLand() {
    if (al::isFirstStep(this)) {
        setVelocityZeroHeadAndBody();
        al::startAction(this, "Land");
        startBodyAction("Land", 0);
    }

    al::setVelocityZero(this);
    if (al::isActionEnd(this)) {
        if (mTarget != nullptr) {
            al::setNerve(this, &NrvSamboSnowHeadChase);
        } else {
            al::setNerve(this, &NrvSamboSnowHeadWait);
        }
    }
}

/** @brief Falls after being trampled and turns into a snow ball on landing. */
void SamboSnowHead::exeTrample() {
    if (al::isFirstStep(this) && !mIsHatBlown) {
        mHat->startBlow();
    }

    if (al::isStep(this, 2)) {
        requestBodyBlowDown();
    }

    al::addVelocityToGravity(this, 1.5f);
    if (al::isOnGround(this, 0, 0.0f)) {
        mBallSnow->makeActorAppeared();
        al::emitEffect(this, "Change", nullptr);
        al::setTrans(mBallSnow, al::getTrans(this) + sead::Vector3f::ey * 50.0f);
        al::resetPosition(mBallSnow, false);
        kill();
    }

    if (al::isGreaterEqualStep(this, 120)) {
        al::emitEffect(this, "Die", nullptr);
        kill();
    }
}

/** @brief Plays the defeat after a hip drop and drops an item. */
void SamboSnowHead::exeHipDrop() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FireDown");
    }

    if (!mIsHatBlown) {
        mHat->startBlow();
    }

    if (al::isStep(this, 2)) {
        requestBodyBlowDown();
    }

    if (al::isActionEnd(this)) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Flies off after a blow-down attack and turns into a snow ball on landing. */
void SamboSnowHead::exeBlowDown() {
    if (al::isFirstStep(this)) {
        if (!mIsHatBlown) {
            mHat->startBlow();
        }

        al::setVelocity(this, mBlowDownVelocity);
    }

    if (al::isStep(this, 2)) {
        requestBodyBlowDown();
    }

    al::addVelocityToGravity(this, 1.5f);
    if (al::isOnGround(this, 0, 0.0f)) {
        mBallSnow->makeActorAppeared();
        al::emitEffect(this, "Change", nullptr);
        al::startSe(this, "PgLand");
        al::setTrans(mBallSnow, al::getTrans(this) + sead::Vector3f::ey * 50.0f);
        al::resetPosition(mBallSnow, false);
        kill();
    }

    if (al::isGreaterEqualStep(this, 120)) {
        al::emitEffect(this, "Die", nullptr);
        kill();
    }
}

/** @brief Plays the defeat after a fireball hit and drops an item. */
void SamboSnowHead::exeFireDown() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "FireDown");
    }

    if (!mIsHatBlown) {
        mHat->startBlow();
    }

    if (al::isStep(this, 2)) {
        requestBodyBlowDown();
    }

    if (al::isActionEnd(this)) {
        al::appearItem(this);
        kill();
    }
}

/** @brief Loses the hat after a touch, then resumes the chase or waits. */
void SamboSnowHead::exeTouchDown() {
    if (al::isFirstStep(this)) {
        setVelocityZeroHeadAndBody();
        al::startAction(this, "TouchDamage");
        startBodyAction("TouchDamage", 0);
        mHat->startBlow();
        mIsHatBlown = true;
    }

    if (al::isActionEnd(this)) {
        mTarget = rc::tryFindNearestActivePlayerActorInSphere(this, cSearchRadius);
        if (mTarget != nullptr) {
            al::setNerve(this, &NrvSamboSnowHeadChase);
        } else {
            al::setNerve(this, &NrvSamboSnowHeadWait);
        }
    }
}

/** @brief Stays frozen together with the bodies while touched. */
void SamboSnowHead::exeSupportFreeze() {
    if (al::isFirstStep(this)) {
        setVelocityZeroHeadAndBody();
    }

    requestBodySupportFreeze(mStateSupportFreeze->getTouchActor());

    if (al::updateNerveState(this)) {
        al::setNerve(this, &NrvSamboSnowHeadWait);
    }
}

/** @brief Freezes every body still in the stack.
 * @param pActor Actor causing the freeze.
 */
void SamboSnowHead::requestBodySupportFreeze(const al::LiveActor* pActor) {
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            mBodies[i]->requestSupportFreeze(pActor);
        }
    }
}

/** @brief Unfreezes the bodies when the freeze ends. */
void SamboSnowHead::endSupportFreeze() {
    requestBodyEndSupportFreeze();
}

/** @brief Unfreezes every body still in the stack. */
void SamboSnowHead::requestBodyEndSupportFreeze() {
    for (s32 i = 0; i < mBodyNum; i++) {
        if (!isBodyDeadOrAttacked(i)) {
            mBodies[i]->requestEndSupportFreeze();
        }
    }
}
