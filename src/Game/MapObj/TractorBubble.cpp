#include "MapObj/TractorBubble.hpp"

#include <math.h>
#include <math/seadMathCalcCommon.h>

#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"

#include "MapObj/TractorBubblePuppeteer.hpp"
#include "Player/IUsePlayerKeyConfig.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
NERVE_DECL(TractorBubble, Wait)
NERVE_DECL(TractorBubble, Bubble)

/**
 * @brief Nerve for a bubble burst by a punch; shares exeBurstBubble.
 */
class TractorBubbleNrvBurstBubblePunch : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TractorBubble>()->exeBurstBubble();
    }
};

/**
 * @brief Nerve that waits for the bind of a player put in by activation; shares exeBindWait.
 */
class TractorBubbleNrvActivateBindWait : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<TractorBubble>()->exeBindWait();
    }
};

NERVE_DECL(TractorBubble, AppearBubble)
NERVE_DECL(TractorBubble, BindWait)
NERVE_DECL(TractorBubble, BurstBubble)

// Non-const nerve objects: the game merges them into one block.
TractorBubbleNrvWait NrvTractorBubbleWait;
TractorBubbleNrvBubble NrvTractorBubbleBubble;
TractorBubbleNrvBurstBubblePunch NrvTractorBubbleBurstBubblePunch;
TractorBubbleNrvActivateBindWait NrvTractorBubbleActivateBindWait;
TractorBubbleNrvAppearBubble NrvTractorBubbleAppearBubble;
TractorBubbleNrvBindWait NrvTractorBubbleBindWait;
TractorBubbleNrvBurstBubble NrvTractorBubbleBurstBubble;

/// Layout position the bubble appears at when it pops in on the target player's screen.
sead::Vector2f sAppearLayoutPos(-700.0f, -150.0f);
/// Shadow drop direction used while the bubble is inside a FrameOutCtrlArea (side view).
sead::Vector3f sShadowDropDirFrameOut(0.0f, 0.0f, -1.0f);
/// Appear offset from the target player inside a FrameOutCtrlArea.
sead::Vector3f sAppearOffsetFrameOut(-2000.0f, 500.0f, 0.0f);

/// What the bubble ran into while following the target player.
enum class BubbleHitType : s32 {
    None = 0,
    Arrow = 1,
    Ceiling = 2,
    Sphere = 3,
};

/// Collision found between the carried player and the target player.
struct BubbleHitResult {
    BubbleHitType type;
    sead::Vector3f pos;
    sead::Vector3f normal;
    al::Triangle triangle;
    f32 depth;
};

/**
 * @brief Finds the alive player the bubble should fly to.
 * @param pPlayer The player carried by the bubble.
 * @return The camera's top player if usable, otherwise the nearest usable player, or nullptr.
 */
inline const al::LiveActor* findTargetPlayer(const al::LiveActor* pPlayer) {
    const al::LiveActor* top = al::getTopPlayerActor(pPlayer);

    if (top != nullptr && top != pPlayer && !rc::isPlayerDeadOrBubble(top)) {
        return top;
    }

    const al::LiveActor* playerList[5];
    u32 playerNum = al::calcPlayerListOrderByDistance(pPlayer, playerList, 5);

    for (u32 i = 0; i < playerNum; i++) {
        const al::LiveActor* player = playerList[i];

        if (player == pPlayer || player == nullptr) {
            continue;
        }

        if (!rc::isPlayerDeadOrBubble(player)) {
            return playerList[i];
        }
    }

    return nullptr;
}

/**
 * @brief Computes where the bubble floats next to the target player.
 * @param pTarget The player the bubble flies to.
 * @param pPlayer The player carried by the bubble (picks the side by control user).
 * @return The goal position.
 */
inline sead::Vector3f calcGoalPos(const al::LiveActor* pTarget, const al::LiveActor* pPlayer) {
    sead::Vector3f pos = al::getTrans(pTarget);
    sead::Vector3f front;
    al::calcFrontDir(&front, pTarget);
    pos.y += 300.0f;

    s32 userId = rc::findControlUserId(pPlayer);
    const sead::Vector3f offsets[4] = {
        {150.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 150.0f},
        {-150.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, -150.0f},
    };
    pos += offsets[userId];
    return pos;
}

/**
 * @brief Looks for walls between the carried player and the target player.
 * @param pResult Receives the wall that was hit, if any.
 * @param pPlayer The player carried by the bubble.
 * @param pTarget The player the bubble flies to.
 * @return Whether the bubble is close to the target with nothing in between (it may be cancelled).
 */
inline bool checkNearTarget(BubbleHitResult* pResult, const al::LiveActor* pPlayer,
                            const al::LiveActor* pTarget) {
    if (rc::isPlayerInRouteDokan(pTarget)) {
        return false;
    }

    const sead::Vector3f& playerTrans = al::getTrans(pPlayer);
    sead::Vector3f goalPos = calcGoalPos(pTarget, pPlayer);

    if ((playerTrans - goalPos).squaredLength() > 360000.0f) {
        return false;
    }

    const sead::Vector3f& playerTransNow = al::getTrans(pPlayer);
    sead::Vector3f targetPos = al::getTrans(pTarget);
    sead::Vector3f playerPos = playerTransNow;
    playerPos.y += 50.0f;
    targetPos.y += 50.0f;
    sead::Vector3f diff = playerPos - targetPos;

    sead::Vector3f hitPos;
    al::Triangle triangle;
    f32 len = diff.length();
    BubbleHitType hitType = BubbleHitType::None;

    if (!al::isNearZero(len, 0.001f)) {
        if (!al::isNearZero(diff, 0.001f) &&
            alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, &triangle, targetPos,
                                                 diff, nullptr, nullptr)) {
            hitType = BubbleHitType::Arrow;
        } else if (len > 50.0f && !al::isNearZero(diff, 0.001f)) {
            sead::Vector3f back = diff * (-(len - 50.0f) / len);

            if (alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, &triangle,
                                                     playerPos, back, nullptr, nullptr)) {
                hitType = BubbleHitType::Arrow;
            }
        }
    }

    if (hitType == BubbleHitType::None &&
        alCollisionUtil::getFirstPolyOnArrow(pPlayer, &hitPos, &triangle, playerPos,
                                             sead::Vector3f(0.0f, 120.0f, 0.0f), nullptr,
                                             nullptr)) {
        hitType = BubbleHitType::Ceiling;
    }

    if (hitType != BubbleHitType::None) {
        pResult->type = hitType;
        pResult->pos = hitPos;
        pResult->triangle = triangle;
        pResult->normal = *triangle.getFaceNormal();
    } else {
        sead::Vector3f center = al::getTrans(pPlayer);
        center.y += 50.0f;
        s32 hitNum =
            alCollisionUtil::checkStrikeSphere(pPlayer, center, 35.0f, nullptr, nullptr);

        if (hitNum == 0) {
            al::HitSensor* targetSensor =
                static_cast<const PlayerActor*>(pTarget)->getBindSensor();
            al::HitSensor* playerSensor =
                static_cast<const PlayerActor*>(pPlayer)->getBindSensor();

            if (targetSensor == nullptr || playerSensor == nullptr ||
                !rc::sendMsgIsDisableCancelBubble(targetSensor, targetSensor)) {
                return true;
            }
        } else {
            s32 deepest = -1;
            f32 maxDepth = -1.0f;

            for (s32 i = 0; i < hitNum; i++) {
                const al::SphereHitInfo* info =
                    alCollisionUtil::getStrikeSphereInfo(pPlayer, i);
                f32 depth = info->_70;
                deepest = depth < maxDepth ? deepest : i;
                maxDepth = depth < maxDepth ? maxDepth : depth;
            }

            if (deepest != -1) {
                const al::SphereHitInfo* info =
                    alCollisionUtil::getStrikeSphereInfo(pPlayer, deepest);
                pResult->type = BubbleHitType::Sphere;
                pResult->depth = info->_70;
                pResult->pos = info->mPos;
                pResult->triangle = info->mTriangle;
                pResult->normal = *info->mTriangle.getFaceNormal();
            }
        }
    }

    return false;
}
}  // namespace

/**
 * @brief Constructs the bubble.
 * @param isSingleMode Whether the game runs in single mode (uses the Toad Brigade model).
 */
TractorBubble::TractorBubble(bool isSingleMode)
    : al::LiveActor("引き戻し泡"), mIsSingleMode(isSingleMode) {
    mPrevGoalPos = {0.0f, 0.0f, 0.0f};
    mGoalPos = {0.0f, 0.0f, 0.0f};
    mSmoothGoalPos = {0.0f, 0.0f, 0.0f};
}

/**
 * @brief Creates the puppeteer and the model, then hides the bubble until it is used.
 * @param rInfo Actor init info.
 */
void TractorBubble::init(const al::ActorInitInfo& rInfo) {
    mPuppeteer = new TractorBubblePuppeteer(this);
    al::initActorWithArchiveName(
        this, rInfo, mIsSingleMode ? "TractorBubbleKinopioBrigade" : "TractorBubble", nullptr);
    al::initNerve(this, &NrvTractorBubbleWait, 0);
    al::invalidateClipping(this);
    al::offCollide(this);
    al::hideModel(this);
    makeActorDead();
}

/**
 * @brief Appears and waits for a player.
 */
void TractorBubble::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTractorBubbleWait);
}

/**
 * @brief Updates the carried player.
 */
void TractorBubble::control() {
    mPuppeteer->update();
}

/**
 * @brief Handles messages; a punch or giant attack bursts a cancellable bubble.
 * @param pMsg The message.
 * @param pSelf Receiving sensor.
 * @param pOther Sending sensor.
 * @return Whether the puppeteer consumed the message.
 */
bool TractorBubble::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                               al::HitSensor* pOther) {
    if (mPuppeteer->receiveMsg(pMsg, pSelf, pOther)) {
        return true;
    }

    if (mTargetPlayer != nullptr && al::isNerve(this, &NrvTractorBubbleBubble) &&
        (al::isMsgPlayerUpperPunchForCrossoverSensor(pMsg, pSelf, pOther, 9.0f) ||
         al::isMsgPlayerGiantAttack(pMsg)) &&
        mIsEnableCancel) {
        al::setNerve(this, &NrvTractorBubbleBurstBubblePunch);
    }

    return false;
}

/**
 * @brief Starts the bubble once the player got bound.
 * @param pSelf The bubble's sensor.
 * @param pOther The player's sensor.
 */
void TractorBubble::receiveBindInit(al::HitSensor* pSelf, al::HitSensor* pOther) {
    al::sendMsgHoldCancel(pSelf, pOther);
    mIsActivate = al::isNerve(this, &NrvTractorBubbleActivateBindWait);
    al::setNerve(this, &NrvTractorBubbleAppearBubble);
}

/**
 * @brief Removes the bubble when the bind gets cancelled.
 */
void TractorBubble::receiveBindCancel() {
    al::hideModelIfShow(this);
    kill();
}

/**
 * @brief Puts a player who gets (re)activated into the bubble.
 * @param pPlayer The player to carry.
 */
void TractorBubble::activatePlayerWithBubble(al::LiveActor* pPlayer) {
    mPuppeteer->startBubbleWithActivate(pPlayer);
    appear();
    al::setNerve(this, &NrvTractorBubbleActivateBindWait);
}

/**
 * @brief Puts a player who left the screen into the bubble.
 * @param pPlayer The player to carry.
 */
void TractorBubble::startBubbleWithScreenOut(al::LiveActor* pPlayer) {
    mPuppeteer->startBubbleWithScreenOut(pPlayer);
    appear();
    al::setNerve(this, &NrvTractorBubbleBindWait);
}

/**
 * @brief Puts a player who pressed the bubble button into the bubble.
 * @param pPlayer The player to carry.
 */
void TractorBubble::startBubbleWithInput(al::LiveActor* pPlayer) {
    mPuppeteer->startBubbleWithInput(pPlayer);
    appear();
    al::setNerve(this, &NrvTractorBubbleBindWait);
}

/**
 * @brief Checks whether a player that left the screen can be put into the bubble.
 * @param pPlayer The player.
 * @return Whether the player can enter the bubble.
 */
bool TractorBubble::isEnableBubbleOutFrame(const al::LiveActor* pPlayer) {
    return rc::isPlayerEnableBubble(pPlayer, al::getHitSensor(this, "BindOutScreen"));
}

/**
 * @brief Checks whether a player can enter the bubble by input.
 * @param pPlayer The player.
 * @return Whether the player can enter the bubble.
 */
bool TractorBubble::isEnableBubbleInput(const al::LiveActor* pPlayer) {
    return rc::isPlayerEnableBubble(pPlayer, al::getHitSensor(this, "BindInput"));
}

/**
 * @brief Checks whether a player is carried right now.
 * @return Whether the bubble is alive and appearing or floating.
 */
bool TractorBubble::isPlayerInBubble() {
    if (al::isNerve(this, &NrvTractorBubbleAppearBubble) ||
        al::isNerve(this, &NrvTractorBubbleBubble)) {
        return al::isAlive(this);
    }

    return false;
}

/**
 * @brief Checks whether the bubble is in use.
 * @return Whether the bubble is alive and appearing, floating or bursting.
 */
bool TractorBubble::isBubble() {
    if (al::isNerve(this, &NrvTractorBubbleAppearBubble) ||
        al::isNerve(this, &NrvTractorBubbleBubble) ||
        al::isNerve(this, &NrvTractorBubbleBurstBubble) ||
        al::isNerve(this, &NrvTractorBubbleBurstBubblePunch)) {
        return al::isAlive(this);
    }

    return false;
}

/**
 * @brief Checks whether the bubble waits for the player to get bound.
 * @return Whether the bubble is in one of the bind wait states.
 */
bool TractorBubble::isBindWait() {
    return al::isNerve(this, &NrvTractorBubbleBindWait) ||
           al::isNerve(this, &NrvTractorBubbleActivateBindWait);
}

/**
 * @brief Points the shadow at the back wall inside side-view areas and restores it outside.
 */
void TractorBubble::updateShadow() {
    al::LiveActor* player = mPuppeteer->getPlayerActor();

    if (player == nullptr) {
        return;
    }

    if (rc::tryFindAreaObj(this, rc::AreaObjType::FrameOutCtrlArea, al::getTrans(player)) !=
        nullptr) {
        if (!mIsShadowInArea) {
            mIsShadowInArea = true;
            mShadowDropLength = al::getShadowDropLength(this, "引き戻し泡影");
            al::setShadowDropLength(this, 10000.0f, "引き戻し泡影");
            al::setShadowDropDir(this, sShadowDropDirFrameOut);
        }
    } else if (mIsShadowInArea) {
        mIsShadowInArea = false;
        al::setShadowDropLength(this, mShadowDropLength, "引き戻し泡影");
        al::setShadowDropDir(this, -sead::Vector3f::ey);
    }
}

/**
 * @brief Nerve: waits until a player gets put into the bubble.
 */
void TractorBubble::exeWait() {}

/**
 * @brief Nerve: waits for the bind to start.
 */
void TractorBubble::exeBindWait() {}

/**
 * @brief Nerve: places the bubble around the player and plays the appear animation.
 */
void TractorBubble::exeAppearBubble() {
    updateShadow();

    if (al::isFirstStep(this)) {
        mBubblePos = al::getTrans(mPuppeteer->getPlayerActor());

        if (mIsActivate) {
            const al::LiveActor* target = findTargetPlayer(mPuppeteer->getPlayerActor());

            if (target != nullptr) {
                sead::Vector3f pos = mBubblePos;
                al::AreaObj* area = rc::tryFindAreaObj(this, rc::AreaObjType::FrameOutCtrlArea,
                                                       al::getTrans(target));
                const sead::Vector3f& targetTrans = al::getTrans(target);

                if (area != nullptr) {
                    pos = targetTrans + sAppearOffsetFrameOut;
                } else {
                    al::calcWorldPosFromLayoutPos(&pos, target, sAppearLayoutPos, targetTrans);
                }

                mBubblePos = pos;
            }
        }

        mIsActivate = false;
        al::showModelIfHide(this);

        sead::Vector3f trans = mBubblePos;
        trans.y += 70.0f;
        al::setTrans(this, trans);
        mIsEnableCancel = true;
        al::startAction(this, "Appear");
    }

    if (al::isActionEnd(this)) {
        al::startAction(this, "Wait");
        al::setNerve(this, &NrvTractorBubbleBubble);
    }
}

/**
 * @brief Nerve: flies the carried player towards another player, avoiding walls on the way.
 */
void TractorBubble::exeBubble() {
    updateShadow();

    bool isTrigCancel =
        rc::getPlayerKeyConfig(mPuppeteer->getPlayerActor())->isPadTriggerPlayerJump();
    const al::LiveActor* target = findTargetPlayer(mPuppeteer->getPlayerActor());

    if (al::isFirstStep(this)) {
        al::setVelocity(this, sead::Vector3f::zero);
        mSpeed = 0.0f;
        mWaveFrame = 0.0f;

        sead::Vector3f goalPos;
        if (target != nullptr) {
            goalPos = calcGoalPos(target, mPuppeteer->getPlayerActor());
        } else {
            goalPos = mBubblePos;
        }

        mPrevGoalPos = goalPos;
        mGoalPos = goalPos;
        mSmoothGoalPos = goalPos;
        mTargetPlayer = nullptr;
        mIsEnableCancel = false;
        mHitWallRate = 0.0f;
        mPushOffset = {0.0f, 0.0f, 0.0f};

        al::StringTmp<64> animName(
            "Disable%s", rc::getPlayerCharacterBubbleMatAnimName(mPuppeteer->getPlayerActor()));
        al::startMclAnim(this, animName.cstr());
    }

    mTargetPlayer = target;

    if (target != nullptr) {
        const al::LiveActor* player = mPuppeteer->getPlayerActor();
        sead::Vector3f goalPos = calcGoalPos(target, player);

        mGoalPos += goalPos + mPushOffset - mPrevGoalPos;
        mSmoothGoalPos += mGoalPos - mSmoothGoalPos;
        mPrevGoalPos = mSmoothGoalPos;

        sead::Vector3f goal = mSmoothGoalPos;
        sead::Vector3f dir = goal - mBubblePos;
        f32 distSqH = dir.x * dir.x + dir.z * dir.z;
        f32 dist;

        if (distSqH < 2500.0f) {
            goal.x = mBubblePos.x;
            goal.z = mBubblePos.z;
            f32 sign = dir.y < 0.0f ? -1.0f : 1.0f;
            dist = dir.y * sign;
            dir = {0.0f, sign, 0.0f};
        } else {
            f32 distH = sead::Mathf::sqrt(distSqH) - 50.0f;
            dist = sead::Mathf::sqrt(dir.y * dir.y + distH * distH);
            al::normalizeOrDirZ(&dir);
        }

        f32 maxSpeed;
        f32 accel;
        f32 turnDegree;

        if (dist < 300.0f) {
            f32 nearRate = dist / 300.0f;
            maxSpeed = nearRate * 5.0f;
            accel = nearRate * 0.2f;
            turnDegree = 0.5f;
        } else if (dist < 500.0f) {
            f32 farRate = (dist - 500.0f) / 1000.0f;
            f32 nearRate = dist / 300.0f;
            f32 rate = (dist - 300.0f) / 200.0f;
            f32 nearMaxSpeed = nearRate * 5.0f;
            f32 nearAccel = nearRate * 0.2f;
            f32 farMaxSpeed = farRate * 35.0f + 25.0f;
            f32 farAccel = farRate * 0.5f + 0.5f;
            maxSpeed = nearMaxSpeed * (1.0f - rate) + rate * farMaxSpeed;
            accel = nearAccel * (1.0f - rate) + rate * farAccel;
            turnDegree = rate * 0.5f * 4.5f + 0.5f;
        } else if (dist < 1500.0f) {
            f32 farRate = (dist - 500.0f) / 1000.0f;
            maxSpeed = farRate * 35.0f + 25.0f;
            accel = farRate * 0.5f + 0.5f;
            turnDegree = accel * 4.5f + 0.5f;
        } else {
            maxSpeed = 60.0f;
            accel = 1.0f;
            turnDegree = 0.5f;
        }

        if (maxSpeed < mSpeed) {
            f32 speed = mSpeed * 0.8f;
            mSpeed = speed > maxSpeed ? speed : maxSpeed;
        } else {
            f32 speed = accel + mSpeed;
            mSpeed = speed < maxSpeed ? speed : maxSpeed;
        }

        if (dist > 10000.0f) {
            al::normalizeOrDirZ(&dir);
            mBubblePos = goal - dir * 10000.0f;
        } else {
            mBubblePos += dir * mSpeed;
        }

        sead::Vector3f front = rc::getPlayerFront(target);
        front.y = 0.0f;
        al::normalizeOrDirZ(&front);

        sead::Vector3f puppetFront = rc::getPuppetFrontVec(mPuppeteer->getPlayerPuppet());
        if (al::isNearZero(puppetFront.dot(front) + 1.0f, 0.001f)) {
            sead::Vector3f side;
            side.setCross(puppetFront, sead::Vector3f::ey);
            al::normalizeOrDirZ(&side);
            al::turnVecToVecDegree(&puppetFront, side, turnDegree);
        } else {
            al::turnVecToVecDegree(&puppetFront, front, turnDegree);
        }

        rc::setPuppetFrontVec(mPuppeteer->getPlayerPuppet(), puppetFront);
    }

    mWaveFrame += 1.0f;
    f32 wave = sinf(mWaveFrame / 210.0f * 2.0f * sead::Mathf::pi());
    sead::Vector3f trans = mBubblePos;
    trans.y += wave * 60.0f + 70.0f;
    al::setTrans(this, trans);

    BubbleHitResult result;
    result.type = BubbleHitType::None;
    bool isNearTarget =
        target != nullptr && checkNearTarget(&result, mPuppeteer->getPlayerActor(), target);

    f32 pushLen = mPushOffset.length();

    if (result.type != BubbleHitType::None) {
        if (!al::isNearZero(pushLen, 0.001f)) {
            f32 dot = result.normal.dot(mPushOffset);
            sead::Vector3f tangent = mPushOffset - result.normal * dot;
            f32 tangentLen = tangent.length();

            if (!al::isNearZero(tangentLen, 0.001f)) {
                f32 newLen = fmaxf(tangentLen - 5.0f, 0.0f);
                mPushOffset = tangent * (newLen / tangentLen) + result.normal * dot;
                pushLen = sead::Mathf::sqrt(dot * dot + newLen * newLen);
            }
        }

        if (pushLen > 300.0f) {
            mPushOffset *= 300.0f / pushLen;
        }

        mPushOffset += result.normal * 5.0f;
        mHitWallRate = fminf(mHitWallRate + 1.0f, 120.0f);
    } else {
        if (!al::isNearZero(pushLen, 0.001f)) {
            f32 newLen = fmaxf(pushLen - 1.0f, 0.0f);
            mPushOffset *= newLen / pushLen;
        }

        mHitWallRate *= 0.95f;
    }

    if (!isNearTarget) {
        if (mIsEnableCancel) {
            mIsEnableCancel = false;
            al::StringTmp<64> animName(
                "Disable%s", rc::getPlayerCharacterBubbleMatAnimName(mPuppeteer->getPlayerActor()));
            al::startMclAnim(this, animName.cstr());
        }

        if (target != nullptr && isTrigCancel) {
            al::startSe(this, "Invalid", nullptr);
        }
    } else if (!mIsEnableCancel) {
        mIsEnableCancel = true;
        al::StringTmp<64> animName(
            "Enable%s", rc::getPlayerCharacterBubbleMatAnimName(mPuppeteer->getPlayerActor()));
        al::startMclAnim(this, animName.cstr());
    }

    if (isNearTarget && (isTrigCancel || al::isGreaterEqualStep(this, 2400))) {
        al::setNerve(this, &NrvTractorBubbleBurstBubble);
    }
}

/**
 * @brief Bursts the bubble and releases the player.
 */
void TractorBubble::cancelBubble() {
    al::setNerve(this, &NrvTractorBubbleBurstBubble);
}

/**
 * @brief Nerve: releases the player, plays the burst animation and kills the bubble.
 */
void TractorBubble::exeBurstBubble() {
    updateShadow();

    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvTractorBubbleBurstBubblePunch)) {
            mPuppeteer->endBubblePunch();
        } else {
            mPuppeteer->endBubble();
        }

        al::startAction(this, "Disappear");
    }

    if (al::isActionEnd(this)) {
        al::hideModelIfShow(this);
        kill();
    }
}
