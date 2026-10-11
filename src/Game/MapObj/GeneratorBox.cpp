#include "MapObj/GeneratorBox.hpp"

#include <math/seadMatrix.h>
#include <nerd/nerdMath.h>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/GeneratorBoxChild.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(GeneratorBox, Wait)
NERVE_DECL(GeneratorBox, Disappear)
NERVE_DECL(GeneratorBox, AppearWait)
NERVE_DECL(GeneratorBox, Reaction)
NERVE_DECL(GeneratorBox, Empty)
NERVES_MAKE_NOSTRUCT(GeneratorBox, Wait, Disappear, AppearWait, Reaction, Empty)

/**
 * @brief Get the duration of the hit reaction for an attack message.
 * @param pMsg The attack message.
 * @return The number of reaction steps.
 */
s32 getReactionStep(const al::SensorMsg* pMsg) {
    if (al::isMsgPlayerClimbAttack(pMsg)) {
        return 26;
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerRollingAttack(pMsg)) {
        return 13;
    }

    if (al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerSlidingAttack(pMsg)) {
        return 26;
    }

    return 6;
}
}  // namespace

using GeneratorBoxFunctor = al::FunctorV0M<GeneratorBox*, void (GeneratorBox::*)()>;

/**
 * @brief Construct the generator box.
 * @param pName The actor name.
 */
GeneratorBox::GeneratorBox(const char* pName)
    : al::LiveActor(pName), mComboCounter(new al::ComboCounter()) {
    for (s32 i = 0; i < mChildNum; i++) {
        mChildren[i] = nullptr;
    }
}

/**
 * @brief Initialize the box model, create the child blocks and the empty block shown when full.
 * @param rInfo The actor init info.
 */
void GeneratorBox::init(const al::ActorInitInfo& rInfo) {
    al::initNerve(this, &NrvGeneratorBoxWait, 0);
    al::tryGetArg(&mToDisappearTime, rInfo, "ToDisappearTime");
    al::tryGetArg(&mChildNum, rInfo, "ChildNum");
    al::tryGetArg(reinterpret_cast<s32*>(&mAppearDirection), rInfo, "AppearDirection");

    if (mChildNum > 20) {
        mChildNum = 20;
    }

    const char* pSuffix = rc::getBlockSuffixName(rInfo, false);

    if (al::isSingleMode(rInfo)) {
        if (pSuffix != nullptr) {
            if (al::isEqualString(pSuffix, "WallSide")) {
                pSuffix = "WallSideSM";
            }
        } else {
            pSuffix = "SM";
        }
    }

    al::initMapPartsActor(this, rInfo, pSuffix, 0);

    al::StringTmp<256> modelName;
    al::StringTmp<256> path;
    al::makeMapPartsModelName(&modelName, &path, al::getPlacementInfo(rInfo));
    mIs2x2 = al::isEqualSubString(path.cstr(), "2x2M");

    bool isSingleMode = false;

    if (mIs2x2) {
        mChildInterval = 200.0f;
        isSingleMode = al::isSingleMode(rInfo);
    }

    const char* pChildSuffix = rc::getBlockSuffixName(rInfo, isSingleMode);

    for (s32 i = 0; i < mChildNum; i++) {
        mChildren[i] = new GeneratorBoxChild("GeneratorBoxChild");
        al::PlacementInfo placementInfo;
        al::ActorInitInfo childInfo;
        childInfo.initViewIdHost(&placementInfo, rInfo);
        mChildren[i]->initWithArchive(
            childInfo, mIs2x2 ? "GeneratorBoxChild2x2M" : "GeneratorBoxChild", pChildSuffix);
        mChildren[i]->setHost(this);
        al::setTrans(mChildren[i], al::getTrans(this));

        if (mAppearDirection == AppearDirection::Up) {
            al::invalidateShadow(mChildren[i]);
        }
    }

    for (s32 i = 0; i < mChildNum - 1; i++) {
        mChildren[i]->setChild(mChildren[i + 1]);
    }

    for (s32 i = 1; i < mChildNum; i++) {
        mChildren[i]->setParent(mChildren[i - 1]);
    }

    updateChildPos();
    al::listenStageSwitchOnOff(this, "SwitchTimerOnOff",
                               GeneratorBoxFunctor(this, &GeneratorBox::offTimer),
                               GeneratorBoxFunctor(this, &GeneratorBox::onTimer));

    mBlockEmpty = new BlockEmpty("ビックリボックス本体[空ブロック]",
                                 mIs2x2 ? "GeneratorBoxChild2x2M" : "GeneratorBoxChild");
    al::initCreateActorWithPlacementInfo(mBlockEmpty, rInfo);
    mBlockEmpty->makeActorDead();
    mBlockEmpty->mPreventGiantBreak = true;
    mPushSensorOffset = al::getSensorFollowPosOffset(this, "Push");
    calcAppearDirection(&mAppearDir);
    al::invalidateHitSensor(this, "UpperPunch");
    makeActorAppeared();
}

/**
 * @brief Get a child block.
 * @param index The index of the child, counted from the box upwards.
 * @return The child block.
 */
GeneratorBoxChild* GeneratorBox::getChild(s32 index) const {
    return mChildren[index];
}

/**
 * @brief Let every appeared child follow the block below it.
 */
void GeneratorBox::updateChildPos() {
    GeneratorBoxChild* pChild = mChildren[0];

    if (pChild == nullptr || mAppearChildNum == 0) {
        return;
    }

    const sead::Vector3f dir = mAppearDir;

    {
        sead::Vector3f basePos = al::getTrans(this);
        basePos += dir * (mChildInterval * al::getScale(this).y);
        sead::Vector3f pos = al::getTrans(mChildren[0]) * 0.8f + basePos * 0.2f;
        updateSensorFollowPosOffset(pChild, pos, basePos);
        al::updatePoseQuat(pChild, al::getQuat(this));
        al::resetPosition(mChildren[0], pos, false);
    }

    GeneratorBoxChild* pParent = mChildren[0];

    if (pParent == nullptr) {
        return;
    }

    while ((pChild = pParent->getChild()) != nullptr) {
        sead::Vector3f basePos = al::getTrans(pParent);
        basePos += dir * (mChildInterval * al::getScale(this).y);
        sead::Vector3f pos = al::getTrans(pChild) * 0.8f + basePos * 0.2f;
        updateSensorFollowPosOffset(pChild, pos, basePos);

        sead::Vector3f diff = pos;
        diff -= al::getTrans(pChild);

        if (dir.dot(diff) > 0.0f) {
            al::updatePoseQuat(pChild, al::getQuat(pParent));
            al::resetPosition(pChild, pos, false);
        }

        pParent = pChild;
    }
}

/**
 * @brief Stop the disappear timer (switch on).
 */
void GeneratorBox::offTimer() {
    mIsTimerOff = true;
}

/**
 * @brief Resume the disappear timer (switch off).
 */
void GeneratorBox::onTimer() {
    mIsTimerOff = false;
}

/**
 * @brief Calculate the direction the children are stacked in.
 * @param pDir The calculated direction.
 */
void GeneratorBox::calcAppearDirection(sead::Vector3f* pDir) {
    switch (mAppearDirection) {
    case AppearDirection::Up:
        al::calcQuatUp(pDir, this);
        return;
    case AppearDirection::Side:
        al::calcQuatSide(pDir, this);
        return;
    case AppearDirection::Front:
        al::calcQuatFront(pDir, this);
        return;
    case AppearDirection::SideReverse:
        al::calcQuatSide(pDir, this);
        pDir->negate();
        return;
    }
}

/**
 * @brief Move the box, its empty block and all children along with a linked actor.
 * @param rTrans The new translation.
 */
void GeneratorBox::updateLinkedTrans(const sead::Vector3f& rTrans) {
    al::LiveActor::updateLinkedTrans(rTrans);
    mBlockEmpty->updateLinkedTrans(rTrans);

    const sead::Vector3f dir = mAppearDir;
    sead::Vector3f pos = al::getTrans(this);
    pos += dir * (mChildInterval * al::getScale(this).y);

    for (GeneratorBoxChild* pChild = mChildren[0]; pChild != nullptr; pChild = pChild->getChild()) {
        al::resetPosition(pChild, pos, false);
        pos += dir * (mChildInterval * al::getScale(this).y);
    }
}

/**
 * @brief Kill the box, break all children and remove the empty block.
 */
void GeneratorBox::kill() {
    al::LiveActor::kill();

    for (s32 i = 0; i < mChildNum; i++) {
        mChildren[i]->requestBreak();
    }

    mBlockEmpty->kill();
}

/**
 * @brief Update the child positions and count down the disappear timer.
 */
void GeneratorBox::control() {
    updateChildPos();

    if (mToDisappearTime == 0 || mIsTimerOff) {
        return;
    }

    if (mDisappearTimer - 1 < 0) {
        return;
    }

    mDisappearTimer--;

    if (mDisappearTimer == 0) {
        al::setNerve(this, &NrvGeneratorBoxDisappear);
        return;
    }

    if (mDisappearTimer == 180) {
        disappearSignAllChild();
    }

    if (mDisappearTimer < 180 && (180 - mDisappearTimer) % 40 == 0) {
        al::startSe(this, "PgDisappearSign");
    }
}

/**
 * @brief Let all appeared children start blinking before they disappear.
 */
void GeneratorBox::disappearSignAllChild() {
    for (s32 i = 0; i < mAppearChildNum; i++) {
        mChildren[i]->requestDisappearSign();
    }
}

/**
 * @brief Let the next child appear, or revive a child that was broken.
 * @return True if a child appeared.
 */
bool GeneratorBox::appendChild() {
    s32 deadIndex = -1;
    bool isAliveAll = isAliveAllAppearChild(&deadIndex);

    if (isAliveAll && mAppearChildNum >= mChildNum) {
        return false;
    }

    if (mAppearChildNum == 0) {
        mDisappearTimer = mToDisappearTime;
    }

    if (isAliveAll) {
        mChildren[mAppearChildNum++]->appear();

        if (mAppearChildNum >= mChildNum) {
            al::copyPose(mBlockEmpty, this);
            mBlockEmpty->appear();
            al::hideModelIfShow(this);
            al::invalidateCollisionParts(this);
        }
    } else {
        mChildren[deadIndex]->appear();
    }

    return true;
}

/**
 * @brief Check whether all appeared children are still alive.
 * @param pDeadIndex The index of the first dead child (optional).
 * @return True if no appeared child is dead.
 */
bool GeneratorBox::isAliveAllAppearChild(s32* pDeadIndex) const {
    for (s32 i = 0; i < mAppearChildNum; i++) {
        if (!al::isAlive(mChildren[i])) {
            if (pDeadIndex != nullptr) {
                *pDeadIndex = i;
            }

            return false;
        }
    }

    return true;
}

/**
 * @brief Let all remaining children appear at once.
 */
void GeneratorBox::appendChildAll() {
    al::startSeWithParam(this, "PgGenerateAll", mChildNum + 1 - mAppearChildNum);

    while (appendChild()) {
    }
}

/**
 * @brief Let all children disappear and reset the box.
 */
void GeneratorBox::disappearAllChild() {
    for (s32 i = 0; i < mChildNum; i++) {
        mChildren[i]->trySetDisappear();
    }

    mAppearChildNum = 0;
    al::showModelIfHide(this);
    mBlockEmpty->kill();
    al::validateCollisionParts(this);
    al::setSensorFollowPosOffset(this, "Push", mPushSensorOffset);
    mPushLength = 0.0f;
}

/**
 * @brief Let all appeared children bounce.
 */
void GeneratorBox::boundAllChild() {
    for (s32 i = 0; i < mAppearChildNum; i++) {
        mChildren[i]->requestBound();
    }
}

/**
 * @brief Check whether the box is in a state where a hit can add a child.
 * @return True if waiting.
 */
bool GeneratorBox::isNerveEnableAppear() const {
    return al::isNerve(this, &NrvGeneratorBoxWait) || al::isNerve(this, &NrvGeneratorBoxAppearWait);
}

/**
 * @brief Pass block hits to objects above and push the camera away from a sideways column.
 * @param pSelf The sensor of the box.
 * @param pOther The touched sensor.
 */
void GeneratorBox::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    rc::trySendMsgBlockToUpperObj(pOther, pSelf, mControlUserId, mComboCounter);

    if (mAppearDirection == AppearDirection::Up || !al::isSensorName(pSelf, "Push")) {
        return;
    }

    if (!((al::isSensorPlayer(pOther) && !al::isSensorPlayerEye(pOther)) ||
          al::isSensorEnemyBody(pOther))) {
        return;
    }

    sead::BoundBox3f box(sead::Vector3f(-90.0f, -90.0f, -90.0f),
                         sead::Vector3f(90.0f, 90.0f, 90.0f));

    if (!mIs2x2) {
        box.set(sead::Vector3f(-45.0f, -45.0f, -45.0f), sead::Vector3f(45.0f, 45.0f, 45.0f));
    }

    if (!al::isHitBoxSensor(pOther, al::getSensorPos(pSelf), box)) {
        return;
    }

    sead::Vector3f diff = al::getSensorPos(pOther);
    diff -= al::getSensorPos(pSelf);

    if (al::calcAngleDegree(mAppearDir, diff) > 60.0f) {
        return;
    }

    rc::sendMsgCameraPush(pOther, pSelf, mPushLength * mAppearDir);
}

/**
 * @brief React to attacks on the box by letting a child (or all of them) appear.
 * @param pMsg The received message.
 * @param pOther The sending sensor.
 * @param pSelf The sensor of the box.
 * @return True if the message was handled.
 */
bool GeneratorBox::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                              al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlUserId)) {
        return true;
    }

    if (al::isDead(this) || al::isSensorName(pSelf, "Push")) {
        return false;
    }

    if (al::isMsgPlayerGiantAttack(pMsg) && !al::isSingleMode(this)) {
        al::startHitReactionBreak(this);
        kill();
        return false;
    }

    if (mAppearChildNum >= mChildNum || !isNerveEnableAppear()) {
        return false;
    }

    if (!(al::isMsgPlayerUpperPunch(pMsg) || al::isMsgPlayerRollingAttack(pMsg) ||
          al::isMsgKickKouraAttackCollide(pMsg) || al::isMsgPlayerHipDropAll(pMsg) ||
          al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
          al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg) ||
          al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
          al::isMsgPlayerSlidingAttack(pMsg) || al::isMsgExplosion(pMsg) ||
          al::isMsgBallAttackCollide(pMsg) || rc::isMsgBullAttack(pMsg))) {
        return false;
    }

    if (al::isMsgPlayerBoomerangAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
        al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg) ||
        al::isMsgPlayerClimbSlidingAttack(pMsg) || al::isMsgPlayerSpinAttack(pMsg) ||
        al::isMsgExplosion(pMsg) || rc::isMsgBullAttack(pMsg)) {
        sead::BoundBox3f box(sead::Vector3f(-100.0f, -100.0f, -100.0f),
                             sead::Vector3f(100.0f, 100.0f, 100.0f));

        if (!mIs2x2) {
            box.set(sead::Vector3f(-50.0f, -50.0f, -50.0f), sead::Vector3f(50.0f, 50.0f, 50.0f));
        }

        if (!al::isHitBoxSensor(pOther, al::getSensorPos(pSelf), box)) {
            return false;
        }
    }

    if (al::isMsgPlayerTailAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
        al::isMsgPlayerSpinAttack(pMsg)) {
        bool is2x2 = mIs2x2;
        f32 height = al::getSensorPos(pOther).y - al::getTrans(this).y;

        if (is2x2) {
            if (height > 220.0f) {
                return false;
            }
        } else if (height > 110.0f) {
            return false;
        }
    }

    mReactionStep = getReactionStep(pMsg);
    mControlUserId = rc::tryFindRelativeControlUserId(pOther);
    al::setNerve(this, &NrvGeneratorBoxReaction);
    boundAllChild();
    appendChild();

    if (al::isMsgPlayerHipDropAll(pMsg) || al::isMsgExplosion(pMsg)) {
        appendChildAll();
    }

    return true;
}

/**
 * @brief Let a child appear when the box is touched on the touch screen.
 * @param pMsg The received message.
 * @param pPointer The touching screen pointer.
 * @param pTarget The touched screen point target.
 * @return True if the touch was handled.
 */
bool GeneratorBox::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                         al::ScreenPointTarget* pTarget) {
    if (al::isDead(this)) {
        return false;
    }

    if (mAppearChildNum >= mChildNum || !isNerveEnableAppear()) {
        return false;
    }

    if (!al::isMsgTouchAssistTrig(pMsg)) {
        return false;
    }

    mControlUserId = rc::tryFindRelativeControlUserId(this, pPointer);
    al::setNerve(this, &NrvGeneratorBoxReaction);
    boundAllChild();
    appendChild();
    return true;
}

/**
 * @brief Move the push sensor of the box along with the top child.
 * @param pChild The child that was moved.
 * @param rPos The new position of the child.
 * @param rBasePos The resting position of the child.
 */
void GeneratorBox::updateSensorFollowPosOffset(const GeneratorBoxChild* pChild,
                                               const sead::Vector3f& rPos,
                                               const sead::Vector3f& rBasePos) {
    if (mChildren[mAppearChildNum - 1] != pChild) {
        return;
    }

    sead::Vector3f pos = rPos * 0.8f + rBasePos * 0.2f;
    const sead::Vector3f& rSensorPos = al::getSensorPos(al::getHitSensor(this, "Push"));

    if ((pos - rSensorPos).dot(mAppearDir) <= 0.0f) {
        return;
    }

    sead::Matrix34f invMtx;
    invMtx.setInverse(*getBaseMtx());
    sead::Vector3f offset;
    offset.setMul(invMtx, pos);
    offset.y += mPushSensorOffset.y;
    al::setSensorFollowPosOffset(this, "Push", offset);
    mPushLength = nerd::sqrt((pos - rBasePos).squaredLength());
}

/**
 * @brief Wait for a hit; switch to empty once all children are out.
 */
void GeneratorBox::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (al::isFirstStep(this) && mAppearChildNum == mChildNum) {
        al::setNerve(this, &NrvGeneratorBoxEmpty);
    }
}

/**
 * @brief All children are out.
 */
void GeneratorBox::exeEmpty() {
    al::isFirstStep(this);
}

/**
 * @brief Hit reaction: the upper punch sensor is active for the reaction steps.
 */
void GeneratorBox::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        al::validateHitSensor(this, "UpperPunch");

        if (mIs2x2) {
            al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0.0f, 200.0f, 0.0f));
        } else {
            al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0.0f, 100.0f, 0.0f));
        }
    }

    if (al::isGreaterStep(this, mReactionStep)) {
        al::invalidateHitSensor(this, "UpperPunch");
        al::setSensorFollowPosOffset(this, "UpperPunch", sead::Vector3f(0.0f, 0.0f, 0.0f));
        al::setNerve(this, &NrvGeneratorBoxAppearWait);
    }
}

/**
 * @brief Wait for the next hit while children are out.
 */
void GeneratorBox::exeAppearWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        al::invalidateClipping(this);
    }
}

/**
 * @brief The disappear timer ran out: remove all children and go back to waiting.
 */
void GeneratorBox::exeDisappear() {
    if (al::isFirstStep(this)) {
        disappearAllChild();
        al::validateClipping(this);
    }

    if (al::isGreaterStep(this, 30)) {
        al::setNerve(this, &NrvGeneratorBoxWait);
    }
}

/**
 * @brief Check whether the children are blinking before they disappear.
 * @return True during the last 180 frames of the disappear timer.
 */
bool GeneratorBox::isBlinkingTime() const {
    return mDisappearTimer >= 1 && mDisappearTimer <= 180;
}

/**
 * @brief Remove a child from the column and close the gap.
 * @param pChild The child to remove.
 */
void GeneratorBox::removeChild(GeneratorBoxChild* pChild) {
    bool isFound = false;

    for (s32 i = 0; i < mAppearChildNum; i++) {
        if (!isFound) {
            if (mChildren[i] != pChild) {
                continue;
            }

            if (i != 0 && i + 1 < mAppearChildNum) {
                mChildren[i - 1]->setChild(mChildren[i + 1]);
            }

            isFound = true;
        }

        if (i < mAppearChildNum - 1) {
            mChildren[i] = mChildren[i + 1];
        }
    }

    if (isFound) {
        mAppearChildNum--;
    }
}

/**
 * @brief Destroy the generator box.
 */
GeneratorBox::~GeneratorBox() {}
