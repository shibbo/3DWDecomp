#include "MapObj/GoalPoleBindPuppeteer.hpp"

#include <math.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

#include "MapObj/GoalPole.hpp"
#include "Player/Normal/PlayerKeyConfig.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
NERVE_DECL(GoalPoleBindPuppeteer, Deactive)
NERVE_DECL(GoalPoleBindPuppeteer, CatchFailureHidden)
NERVE_DECL(GoalPoleBindPuppeteer, CatchFailureStart)
NERVE_DECL(GoalPoleBindPuppeteer, CatchMove)
NERVE_DECL(GoalPoleBindPuppeteer, Catch)
NERVE_DECL(GoalPoleBindPuppeteer, CatchTop)
NERVE_DECL(GoalPoleBindPuppeteer, ClimbWallRun)
NERVE_DECL(GoalPoleBindPuppeteer, ClimbWallFailure)
NERVE_DECL(GoalPoleBindPuppeteer, WaitFall)
NERVE_DECL(GoalPoleBindPuppeteer, FallEnd)
NERVE_DECL(GoalPoleBindPuppeteer, Turn)
NERVE_DECL(GoalPoleBindPuppeteer, TurnEnd)
NERVE_DECL(GoalPoleBindPuppeteer, WaitJump)
NERVE_DECL(GoalPoleBindPuppeteer, Fall)
NERVE_DECL(GoalPoleBindPuppeteer, CatchTopWait)
NERVE_DECL(GoalPoleBindPuppeteer, TurnWait)
NERVE_DECL(GoalPoleBindPuppeteer, Jump)
NERVE_DECL(GoalPoleBindPuppeteer, Land)
NERVE_DECL(GoalPoleBindPuppeteer, Pose)
NERVE_DECL(GoalPoleBindPuppeteer, CatchFailureWait)
NERVE_DECL(GoalPoleBindPuppeteer, CatchFailure)
NERVES_MAKE_STRUCT(GoalPoleBindPuppeteer, Deactive, CatchFailureHidden, CatchFailureStart,
                   CatchMove, Catch, CatchTop, ClimbWallRun, ClimbWallFailure, WaitFall, FallEnd,
                   Turn, TurnEnd, WaitJump, Fall, CatchTopWait, TurnWait)
NERVES_MAKE_NOSTRUCT(GoalPoleBindPuppeteer, Jump, Land, Pose, CatchFailureWait, CatchFailure)

/// Height of the pole between the lowest catch position and the pole top.
const f32 sCatchHeightMax = 590.0f;
/// Number of catch height levels below the top level.
const s32 sHeightLevelMax = 7;

/// Side offset of the players on a normal or last goal pole, by player count or player index.
const f32 sSideOffsetTable[] = {350.0f, 380.0f, 400.0f, 420.0f};
/// Side offset of the players on a super goal pole, by player count or player index.
const f32 sSideOffsetTableSuper[] = {350.0f, 390.0f, 415.0f, 480.0f};

/**
 * @brief Picks the side offset of a player next to the goal pole.
 * @param index Index into the offset table.
 * @param pPole The goal pole.
 * @return The side offset.
 */
f32 getSideOffset(s32 index, const GoalPole* pPole) {
    if (pPole->isLast()) {
        return sSideOffsetTable[index];
    }

    if (pPole->isSuper()) {
        return sSideOffsetTableSuper[index];
    }

    return sSideOffsetTable[index];
}

/**
 * @brief Spacing between two players posing next to the goal pole.
 * @param pPole The goal pole.
 * @return The spacing.
 */
f32 getPoseSpacing(const GoalPole* pPole) {
    if (pPole->isLast()) {
        return 160.0f;
    }

    return pPole->isSuper() ? 160.0f : 140.0f;
}

/**
 * @brief Converts a catch height into a height level (one level per seventh of the pole).
 * @param height Catch height above the lowest catch position.
 * @return The height level.
 */
s32 calcHeightLevel(f32 height) {
    return static_cast<s32>(height / (sCatchHeightMax / sHeightLevelMax));
}

/**
 * @brief Picks the goal pose of a player.
 * @param pPole The goal pole.
 * @param heightLevel Catch height level of the player.
 * @return The name of the pose action.
 */
const char* getPoseActionName(const GoalPole* pPole, s32 heightLevel) {
    if (pPole->isLast()) {
        return "GoalPoseLast";
    }

    bool isSuper = pPole->isSuper();
    const char* normalName;
    const char* superName;

    if (heightLevel >= 3) {
        normalName = "GoalPoseSuccess";
        superName = "GoalPoseSuperSuccess";
    } else {
        normalName = "GoalPoseFailure";
        superName = "GoalPoseSuperFailure";
    }

    return isSuper ? superName : normalName;
}
}  // namespace

/**
 * @brief Creates the puppeteer and the bubble that carries a player who missed the pole.
 * @param pName Name of the nerve executor.
 * @param pPole The goal pole.
 * @param rInfo Actor init info used for the bubble.
 * @param pPoleMtx Base matrix of the goal pole.
 */
GoalPoleBindPuppeteer::GoalPoleBindPuppeteer(const char* pName, GoalPole* pPole,
                                             const al::ActorInitInfo& rInfo,
                                             const sead::Matrix34f* pPoleMtx)
    : BindPuppeteer(pName), mPole(pPole), mPoleMtx(pPoleMtx),
      mBubble(new al::LiveActor("強制排出泡[ゴールポール]")) {
    initNerve(&NrvGoalPoleBindPuppeteer.Deactive, 0);
    al::initActorWithArchiveName(mBubble, rInfo, "TractorBubble", "GoalPole");
    mBubble->makeActorDead();
}

/**
 * @brief Binds a player who touched the pole and starts moving it to its catch position.
 * @param pPlayerSensor Sensor of the player.
 * @param pBinderSensor Sensor of the goal pole.
 * @param catchIndex Order in which the player caught the pole.
 */
void GoalPoleBindPuppeteer::startBind(al::HitSensor* pPlayerSensor, al::HitSensor* pBinderSensor,
                                      s32 catchIndex) {
    BindPuppeteer::startBind(pPlayerSensor, pBinderSensor);
    rc::removeAllEquipOfAllDoubleMarioExceptWithScore(pPlayerSensor);
    rc::killAllDoubleMarioExceptWithScore(pPlayerSensor);
    rc::validatePuppetDamage(getPlayerPuppet());
    rc::invalidatePuppetSensors(getPlayerPuppet());

    mPoleMtx->getTranslation(mCatchBaseTrans);
    mCatchBaseTrans.y += 80.0f;
    al::calcDirBetweenSensorsH(&mFrontDir, pPlayerSensor, pBinderSensor);

    if (al::isNearZero(mFrontDir, 0.001f)) {
        mFrontDir.set(sead::Vector3f::ez);
    }

    mPoleMtx->getBase(mPoseFront, 0);
    al::rotateVectorDegreeY(&mPoseFront, 150.0f);

    if (mIsCatchFailure) {
        mCatchHeight = -1.0f;

        if (rc::isPuppetHidden(getPlayerPuppet())) {
            al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchFailureHidden);
            return;
        }

        al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchFailureStart);
        return;
    }

    f32 height = rc::getPuppetTrans(getPlayerPuppet()).y - mCatchBaseTrans.y;

    if (height < 0.0f) {
        height = 0.0f;
    } else if (height > sCatchHeightMax) {
        height = sCatchHeightMax;
    }

    const sead::Vector3f target(mCatchBaseTrans.x, mCatchBaseTrans.y + height, mCatchBaseTrans.z);
    sead::Vector3f diff;
    diff.setSub(target, rc::getPuppetTrans(getPlayerPuppet()));
    sead::Vector3f dir;
    al::normalizeOrDirZ(&dir, diff);

    const sead::Vector3f velocity = rc::getPuppetVelocity(getPlayerPuppet());
    al::parallelizeVec(&mCatchMoveVelocity, dir, velocity);

    if (mCatchMoveVelocity.length() < 30.0f) {
        mCatchMoveVelocity.setScale(dir, 30.0f);
    }

    mCatchMoveStep = static_cast<s32>(diff.length() / mCatchMoveVelocity.length());
    mCatchMoveVelocity.y = velocity.y;

    IUsePlayerPuppet* puppet = getPlayerPuppet();
    rc::setPuppetTrans(puppet, rc::getPuppetTrans(getPlayerPuppet()) + mCatchMoveVelocity);
    mCatchIndex = catchIndex;
    al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchMove);
}

/**
 * @brief Updates the nerve of the puppeteer.
 */
void GoalPoleBindPuppeteer::update() {
    updateNerve();
}

/**
 * @brief Marks the player as having missed the pole (it gets carried away in a bubble).
 */
void GoalPoleBindPuppeteer::onCatchFailure() {
    mIsCatchFailure = true;
}

/**
 * @brief Checks whether the player caught the pole.
 * @return True if the player is bound and did not miss the pole.
 */
bool GoalPoleBindPuppeteer::isCatchSuccess() const {
    if (mIsCatchFailure) {
        return false;
    }

    return !al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive);
}

/**
 * @brief Checks whether the player grabbed the pole this frame.
 * @return True on the first frame of a catch.
 */
bool GoalPoleBindPuppeteer::isCatchJust() const {
    if (al::isNerve(this, &NrvGoalPoleBindPuppeteer.Catch) ||
        al::isNerve(this, &NrvGoalPoleBindPuppeteer.CatchTop)) {
        return al::isNewNerve(this) || al::isFirstStep(this);
    }

    return false;
}

/**
 * @brief Checks whether the player finished moving to its catch position.
 * @return True once the player holds the pole.
 */
bool GoalPoleBindPuppeteer::isEndCatchMove() const {
    if (mIsCatchFailure) {
        return false;
    }

    if (al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive)) {
        return false;
    }

    if (al::isNerve(this, &NrvGoalPoleBindPuppeteer.CatchMove)) {
        return false;
    }

    if (al::isNerve(this, &NrvGoalPoleBindPuppeteer.ClimbWallRun)) {
        return false;
    }

    return !al::isNerve(this, &NrvGoalPoleBindPuppeteer.ClimbWallFailure);
}

/**
 * @brief Checks whether the player is ready to slide down the pole.
 * @return True if the player waits for the fall or does not take part in it.
 */
bool GoalPoleBindPuppeteer::isEnableStartFall() const {
    if (mIsCatchFailure || al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive)) {
        return true;
    }

    return al::isNerve(this, &NrvGoalPoleBindPuppeteer.WaitFall);
}

/**
 * @brief Checks whether the player finished sliding down the pole.
 * @return True once the player reached the bottom of the pole.
 */
bool GoalPoleBindPuppeteer::isEndFalling() const {
    if (mIsCatchFailure || al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive) ||
        al::isNerve(this, &NrvGoalPoleBindPuppeteer.FallEnd) ||
        al::isNerve(this, &NrvGoalPoleBindPuppeteer.Turn) ||
        al::isNerve(this, &NrvGoalPoleBindPuppeteer.TurnEnd)) {
        return true;
    }

    return al::isNerve(this, &NrvGoalPoleBindPuppeteer.WaitJump);
}

/**
 * @brief Starts sliding down the pole.
 * @param heightOrder Rank of the catch height among all players (0 for the highest).
 * @param catchNum Number of players who caught the pole.
 * @param pUnder Puppeteer of the player right below, or nullptr.
 */
void GoalPoleBindPuppeteer::startFall(s32 heightOrder, s32 catchNum,
                                      const GoalPoleBindPuppeteer* pUnder) {
    s32 rank = catchNum - heightOrder - 1;
    mHeightOrder = heightOrder;
    mUnderPuppeteer = pUnder;
    mCatchBaseTrans.y = rank * 100.0f + mCatchBaseTrans.y;

    if (mCatchTrans.y < mCatchBaseTrans.y) {
        mCatchBaseTrans.y = mCatchTrans.y;
    }

    sead::Vector3f side;
    mPoleMtx->getBase(side, 0);
    mPoleMtx->getTranslation(mJumpTargetTrans);

    f32 offset = getSideOffset(catchNum - 1, mPole) +
                 getPoseSpacing(mPole) * (catchNum - 1) * 0.5f;
    offset -= getPoseSpacing(mPole) * heightOrder;
    mJumpTargetTrans += side * offset;

    mJumpStartStep = getJumpStartDelayFrame() * heightOrder + 15;
    mPoseStartStep = rank * 10 + 27;
    al::setNerve(this, &NrvGoalPoleBindPuppeteer.Fall);
}

/**
 * @brief Checks whether the player is ready to jump off the pole.
 * @return True if the player finished turning or does not take part in the jump.
 */
bool GoalPoleBindPuppeteer::isEnableStartJump() const {
    if (mIsCatchFailure || al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive)) {
        return true;
    }

    return al::isNerve(this, &NrvGoalPoleBindPuppeteer.TurnEnd);
}

/**
 * @brief Starts waiting for the jump off the pole.
 */
void GoalPoleBindPuppeteer::startJump() {
    al::setNerve(this, &NrvGoalPoleBindPuppeteer.WaitJump);
}

/**
 * @brief Checks whether the player caught the very top of the pole.
 * @return True for the highest height level.
 */
bool GoalPoleBindPuppeteer::isHeightLevelMax() const {
    return mHeightLevel == sHeightLevelMax;
}

/**
 * @brief Checks whether the flag can appear for this player.
 * @return True once the player holds the pole or does not take part in the goal.
 */
bool GoalPoleBindPuppeteer::isEnableAppearFlag() const {
    if (mIsCatchFailure || al::isNerve(this, &NrvGoalPoleBindPuppeteer.Deactive) ||
        al::isNerve(this, &NrvGoalPoleBindPuppeteer.WaitFall)) {
        return true;
    }

    if (isHeightLevelMax()) {
        return al::isNerve(this, &NrvGoalPoleBindPuppeteer.CatchTopWait);
    }

    return al::isNerve(this, &NrvGoalPoleBindPuppeteer.Catch);
}

/**
 * @brief Calculates the catch height relative to the pole height.
 * @return The catch height rate, 1 for the pole top.
 */
f32 GoalPoleBindPuppeteer::calcCatchHeightRate() const {
    return mCatchHeight / sCatchHeightMax;
}

/**
 * @brief Checks whether a player above can keep sliding down without hitting this player.
 * @param rTrans Position of the player above.
 * @return True if the player above is far enough.
 */
bool GoalPoleBindPuppeteer::isEnableFallUpperPlayer(const sead::Vector3f& rTrans) const {
    if (!al::isNerve(this, &NrvGoalPoleBindPuppeteer.Fall)) {
        return true;
    }

    return rTrans.y - rc::getPuppetTrans(getPlayerPuppet()).y > 100.0f;
}

/**
 * @brief Delay between the jumps of two players.
 * @return The delay in frames.
 */
s32 GoalPoleBindPuppeteer::getJumpStartDelayFrame() {
    return 10;
}

/**
 * @brief Longest time a player takes to slide down the pole.
 * @return The duration in frames.
 */
s32 GoalPoleBindPuppeteer::getFallFrameMax() {
    return 59;
}

/**
 * @brief Snaps the player on the pole, gives the score and plays the pole animation.
 * @param pActionName Action of the player.
 */
void GoalPoleBindPuppeteer::startCatch(const char* pActionName) {
    IUsePlayerPuppet* puppet = getPlayerPuppet();
    rc::setPuppetTrans(puppet, mCatchTrans);
    rc::setPuppetFrontVec(puppet, mFrontDir);
    rc::startPuppetAction(puppet, pActionName);
    rc::startPuppetSe(puppet, "GoalPoleCatch");
    rc::addScoreByFactor(mPole, rc::getPuppetSensor(puppet), "ゴール", mCatchTrans,
                         mHeightLevel);

    if (isHeightLevelMax()) {
        al::startAction(mPole, "GoalHigh");
    } else if (!al::isSklAnimPlaying(mPole, "GoalHigh", 0)) {
        al::startAction(mPole, "GoalLow");
    }
}

/**
 * @brief Keeps the player on the pole while the pole animation bends it.
 */
void GoalPoleBindPuppeteer::setPuppetTransSyncHostAnim() {
    f32 rate = al::normalize(mCatchTrans.y, mCatchBaseTrans.y, mPole->getPoleTopHeight());
    sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
    al::lerpVec(&trans, mCatchBaseTrans, mPole->getPoleTrans(), rate);
    rc::setPuppetTrans(getPlayerPuppet(), trans);
}

/**
 * @brief Makes the player and its bubble bob up and down.
 * @param rBaseTrans Center of the movement.
 */
void GoalPoleBindPuppeteer::updateBubbleVerticalMovement(const sead::Vector3f& rBaseTrans) {
    mBubbleStep = al::modi(mBubbleStep + 51, 50);
    f32 degree = al::normalize(static_cast<f32>(mBubbleStep), 0.0f, 50.0f) * 360.0f;
    const sead::Vector3f offset(0.0f, sinf(sead::Mathf::deg2rad(degree)) * 7.5f, 0.0f);
    rc::setPuppetTrans(getPlayerPuppet(), rBaseTrans + offset);

    al::setTrans(mBubble, rBaseTrans + offset + sead::Vector3f(0.0f, 75.0f, 0.0f));
}

/**
 * @brief Idle state: no player is bound.
 */
void GoalPoleBindPuppeteer::exeDeactive() {}

/**
 * @brief Moves the player to its catch position on the pole.
 */
void GoalPoleBindPuppeteer::exeCatchMove() {
    sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet()) + mCatchMoveVelocity;
    trans.y = sead::Mathf::max(trans.y, mCatchBaseTrans.y);

    rc::setPuppetTrans(getPlayerPuppet(), trans);

    mCatchHeight = trans.y - mCatchBaseTrans.y;
    bool isTop = false;

    if (mCatchHeight >= sCatchHeightMax) {
        isTop = true;
        mCatchHeight = sCatchHeightMax;
    } else if (mCatchHeight < 0.0f) {
        mCatchHeight = 0.0f;
    }

    mCatchTrans.set(mCatchBaseTrans);
    mCatchTrans.y += mCatchHeight;

    f32 range = isTop ? 100.0f : 50.0f;

    if ((trans - mCatchTrans).length() < range || al::isStep(this, mCatchMoveStep)) {
        if (isTop) {
            mHeightLevel = sHeightLevelMax;
            al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchTop);
            return;
        }

        if (rc::isPlayerClimbOrClimbSpecial(rc::getPuppetSensor(getPlayerPuppet()))) {
            al::setNerve(this, &NrvGoalPoleBindPuppeteer.ClimbWallRun);
            return;
        }

        mHeightLevel = sead::Mathi::clamp(calcHeightLevel(mCatchHeight), 0, sHeightLevelMax - 1);
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.Catch);
    }
}

/**
 * @brief Holds the pole below the top.
 */
void GoalPoleBindPuppeteer::exeCatch() {
    if (al::isFirstStep(this)) {
        startCatch("GoalPoleWait");
        rc::removeAllEquipFromPlayerGoalPole(rc::getPuppetSensor(getPlayerPuppet()));
    }

    setPuppetTransSyncHostAnim();
    al::setNerveAtStep(this, &NrvGoalPoleBindPuppeteer.WaitFall, 60);
}

/**
 * @brief Lets a climbing player run up the pole towards the top.
 */
void GoalPoleBindPuppeteer::exeClimbWallRun() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "ClimbWallRun");
        rc::setPuppetFrontVec(getPlayerPuppet(), mFrontDir);
    }

    const sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet());
    sead::Vector3f target = trans;
    target.y = mCatchBaseTrans.y + sCatchHeightMax;
    sead::Vector3f diff = target - trans;

    if (diff.length() < 7.5f) {
        if (mCatchIndex == 0) {
            mPole->fixCamera(target);
        }

        mCatchTrans.y = target.y;
        mCatchHeight = sCatchHeightMax;
        mHeightLevel = sHeightLevelMax;
        rc::setPuppetTrans(getPlayerPuppet(), target);
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchTop);
        return;
    }

    if (al::isGreaterEqualStep(this, 65)) {
        if (mCatchIndex == 0) {
            mPole->fixCamera(trans);
        }

        mCatchHeight = sead::Mathf::min(trans.y - mCatchBaseTrans.y, sCatchHeightMax);
        mHeightLevel = calcHeightLevel(mCatchHeight);
        mCatchTrans.y = trans.y;

        if (mHeightLevel >= sHeightLevelMax) {
            mCatchHeight = sCatchHeightMax;
            mHeightLevel = sHeightLevelMax;
            mCatchTrans.y = target.y;
            al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchTop);
            return;
        }

        al::setNerve(this, &NrvGoalPoleBindPuppeteer.ClimbWallFailure);
        return;
    }

    if (!mIsClimbStarted) {
        PlayerKeyConfig keyConfig(
            rc::getPlayerInputPort(rc::getPuppetSensor(getPlayerPuppet())));
        sead::Vector2f stick = {0.0f, 0.0f};
        keyConfig.calcLeftStick(&stick);

        if (sead::Mathf::abs(stick.length()) > 0.0f) {
            mIsClimbStarted = true;
        }
    }

    if (mIsClimbStarted) {
        if (!rc::isPuppetAction(getPlayerPuppet(), "ClimbWallRun")) {
            rc::startPuppetAction(getPlayerPuppet(), "ClimbWallRun");
        }
    } else if (!rc::isPuppetAction(getPlayerPuppet(), "ClimbClimbWallKeep")) {
        rc::startPuppetAction(getPlayerPuppet(), "ClimbClimbWallKeep");
    }

    if (mIsClimbStarted) {
        f32 length = diff.length();

        if (length > 0.0f) {
            diff *= 7.5f / length;
        }

        rc::setPuppetTrans(getPlayerPuppet(), trans + diff);
    }
}

/**
 * @brief Drops back onto the pole after the player failed to climb to the top.
 */
void GoalPoleBindPuppeteer::exeClimbWallFailure() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "ClimbClimbWallFallStart");
    }

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.Catch);
    }
}

/**
 * @brief Holds the very top of the pole.
 */
void GoalPoleBindPuppeteer::exeCatchTop() {
    if (al::isFirstStep(this)) {
        startCatch("GoalPoleTop");
        rc::startPuppetSe(getPlayerPuppet(), "GoalPoleCatchTop");
        rc::removeAllEquipFromPlayerGoalPole(rc::getPuppetSensor(getPlayerPuppet()));
    }

    setPuppetTransSyncHostAnim();

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.CatchTopWait);
    }
}

/**
 * @brief Waits at the top of the pole.
 */
void GoalPoleBindPuppeteer::exeCatchTopWait() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "GoalPoleWait");
    }

    al::setNerveAtStep(this, &NrvGoalPoleBindPuppeteer.WaitFall, 30);
}

/**
 * @brief Waits on the pole until all players can slide down.
 */
void GoalPoleBindPuppeteer::exeWaitFall() {}

/**
 * @brief Slides down the pole, keeping a distance to the player below.
 */
void GoalPoleBindPuppeteer::exeFall() {
    if (mUnderPuppeteer != nullptr &&
        !mUnderPuppeteer->isEnableFallUpperPlayer(rc::getPuppetTrans(getPlayerPuppet()))) {
        return;
    }

    if (!rc::isPuppetAction(getPlayerPuppet(), "GoalPoleFall")) {
        rc::startPuppetAction(getPlayerPuppet(), "GoalPoleFall");
    }

    const sead::Vector3f trans = rc::getPuppetTrans(getPlayerPuppet());
    sead::Vector3f diff = mCatchBaseTrans - trans;

    if (diff.length() < 10.0f) {
        rc::setPuppetTrans(getPlayerPuppet(), mCatchBaseTrans);
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.FallEnd);
        return;
    }

    f32 length = diff.length();

    if (length > 0.0f) {
        diff *= 10.0f / length;
    }

    rc::setPuppetTrans(getPlayerPuppet(), trans + diff);
}

/**
 * @brief Lands at the bottom of the pole.
 */
void GoalPoleBindPuppeteer::exeFallEnd() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "GoalPoleFallEnd");

        if (mCatchIndex == 0) {
            mPole->tryReleaseFairy();
        }
    }

    if (rc::isPuppetActionEnd(getPlayerPuppet())) {
        const al::Nerve* nerve = &NrvGoalPoleBindPuppeteer.Turn;

        if (mPole->isSuperWithFairyBottle()) {
            nerve = &NrvGoalPoleBindPuppeteer.TurnWait;
        }

        al::setNerve(this, nerve);
    }
}

/**
 * @brief Waits for the fairy before turning around.
 */
void GoalPoleBindPuppeteer::exeTurnWait() {
    al::setNerveAtStep(this, &NrvGoalPoleBindPuppeteer.Turn, 60);
}

/**
 * @brief Turns the player towards its pose direction.
 */
void GoalPoleBindPuppeteer::exeTurn() {
    sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());

    if (al::isFirstStep(this)) {
        f32 targetDegree = sead::Mathf::rad2deg(atan2f(-mPoseFront.z, mPoseFront.x));
        f32 degree = sead::Mathf::rad2deg(atan2f(-front.z, front.x));

        while (targetDegree < degree) {
            degree -= 360.0f;
        }

        f32 diff = targetDegree - degree;

        if (diff < 240.0f) {
            mTurnDegree = 12.0f;
            mTurnStep = static_cast<s32>(diff / 12.0f);
        } else {
            mTurnDegree = -12.0f;
            mTurnStep = static_cast<s32>((360.0f - diff) / 12.0f);
        }
    }

    if (al::isStep(this, mTurnStep)) {
        rc::setPuppetFrontVec(getPlayerPuppet(), mPoseFront);
        al::setNerve(this, &NrvGoalPoleBindPuppeteer.TurnEnd);
        return;
    }

    al::rotateVectorDegreeY(&front, mTurnDegree);
    rc::setPuppetFrontVec(getPlayerPuppet(), front);
}

/**
 * @brief Waits for the jump after turning around.
 */
void GoalPoleBindPuppeteer::exeTurnEnd() {}

/**
 * @brief Waits for the jump start step of this player.
 */
void GoalPoleBindPuppeteer::exeWaitJump() {
    al::setNerveAtStep(this, &NrvGoalPoleBindPuppeteerJump, mJumpStartStep);
}

/**
 * @brief Jumps off the pole to the pose position.
 */
void GoalPoleBindPuppeteer::exeJump() {
    sead::Quatf quat = sead::Quatf::unit;
    sead::Vector3f front = {0.0f, 0.0f, 0.0f};
    rc::calcPuppetQuat(&quat, getPlayerPuppet());
    mPoleMtx->getBase(front, 2);

    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "GoalPoleJump");
        rc::setPuppetFrontVec(getPlayerPuppet(), front);

        if (mPole->isEnableFur()) {
            rc::showPuppetFur(getPlayerPuppet());
        }
    }

    if (al::isStep(this, 55)) {
        rc::setPuppetQuat(getPlayerPuppet(), quat);
        rc::setPuppetTrans(getPlayerPuppet(), mJumpTargetTrans);
        al::setNerve(this, &NrvGoalPoleBindPuppeteerLand);
        return;
    }

    sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
    al::lerpVec(&trans, mCatchBaseTrans, mJumpTargetTrans, al::calcNerveRate(this, 55));
    trans.y += al::calcNerveJumpValue(this, 25, 4, 25, 250.0f);
    rc::setPuppetTrans(getPlayerPuppet(), trans);
}

/**
 * @brief Lands next to the pole.
 */
void GoalPoleBindPuppeteer::exeLand() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), "GoalPoleLand");
    }

    al::setNerveAtStep(this, &NrvGoalPoleBindPuppeteerPose, mPoseStartStep);
}

/**
 * @brief Plays the goal pose and turns the player towards the camera.
 */
void GoalPoleBindPuppeteer::exePose() {
    if (al::isFirstStep(this)) {
        rc::startPuppetAction(getPlayerPuppet(), getPoseActionName(mPole, mHeightLevel));

        if (mCatchIndex == 0) {
            mPole->startGoalPoseNpcAction();
        }
    }

    if (al::isGreaterEqualStep(this, mPole->isLast() ? 700 : 0)) {
        sead::Quatf quat = sead::Quatf::unit;
        rc::calcPuppetQuat(&quat, getPlayerPuppet());
        sead::Vector3f dir = al::getCameraPos(mPole);
        dir -= rc::getPuppetTrans(getPlayerPuppet());
        al::normalizeOrZero(&dir);

        if (!al::isNearZero(dir, 0.001f)) {
            al::turnQuatFrontToDirDegreeH(&quat, dir, 0.5f);
            rc::setPuppetQuat(getPlayerPuppet(), quat);
        }
    }
}

/**
 * @brief Catches the player who missed the pole in a bubble.
 */
void GoalPoleBindPuppeteer::exeCatchFailureStart() {
    if (al::isFirstStep(this)) {
        mBubbleBaseTrans.set(rc::getPuppetTrans(getPlayerPuppet()));
        rc::startPuppetAction(getPlayerPuppet(), "BubbleStart");
        mBubble->appear();
        al::setTrans(mBubble, rc::getPuppetTrans(getPlayerPuppet()) +
                                  sead::Vector3f(0.0f, 75.0f, 0.0f));
        al::startAction(mBubble, "Appear");
        al::calcFrontDir(&mBubbleFront, mPole);
    }

    updateBubbleVerticalMovement(mBubbleBaseTrans);

    sead::Vector3f front = rc::getPuppetFrontVec(getPlayerPuppet());
    f32 restFrame = al::getActionFrameMax(mBubble, "Appear") - al::getActionFrame(mBubble);
    f32 degree = al::calcAngleDegree(front, mBubbleFront);
    f32 turnDegree = 15.0f;

    if (restFrame * 15.0f < degree && restFrame > 0.1f) {
        turnDegree = degree / restFrame;
    }

    al::turnVecToVecDegree(&front, front, mBubbleFront, turnDegree);
    rc::setPuppetFrontVec(getPlayerPuppet(), front);

    if (al::isActionEnd(mBubble) && rc::isPuppetActionEnd(getPlayerPuppet())) {
        al::setNerve(this, &NrvGoalPoleBindPuppeteerCatchFailureWait);
    }
}

/**
 * @brief Floats in the bubble before it carries the player away.
 */
void GoalPoleBindPuppeteer::exeCatchFailureWait() {
    updateBubbleVerticalMovement(mBubbleBaseTrans);

    if (al::isGreaterEqualStep(this, 0)) {
        al::setNerve(this, &NrvGoalPoleBindPuppeteerCatchFailure);
    }
}

/**
 * @brief Carries the player in the bubble to its place next to the pole.
 */
void GoalPoleBindPuppeteer::exeCatchFailure() {
    if (al::isFirstStep(this)) {
        mBubbleBaseTrans.set(rc::getPuppetTrans(getPlayerPuppet()));
        rc::startPuppetAction(getPlayerPuppet(), "BubbleWait");
        al::startAction(mBubble, "Wait");
    }

    if (al::isLessEqualStep(this, 90)) {
        sead::Vector3f trans = {0.0f, 0.0f, 0.0f};
        sead::Vector3f target = {0.0f, 0.0f, 0.0f};
        al::calcTransLocalOffset(
            &target, mPole,
            sead::Vector3f(-250.0f - getSideOffset(getControlUserId(), mPole), 1500.0f, 0.0f));
        al::lerpVec(&trans, mBubbleBaseTrans, target, al::calcNerveEaseInRate(this, 90));
        updateBubbleVerticalMovement(trans);
    }
}

/**
 * @brief Hides a player who missed the pole while it was already hidden.
 */
void GoalPoleBindPuppeteer::exeCatchFailureHidden() {
    if (rc::isPuppetHidden(getPlayerPuppet())) {
        return;
    }

    rc::hidePuppet(getPlayerPuppet());
}
