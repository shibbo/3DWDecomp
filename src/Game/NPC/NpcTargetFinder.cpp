#include "NPC/NpcTargetFinder.hpp"

#include <math/seadMathCalcCommon.h>
#include <nerd/nerdMath.h>

#include "NPC/IUseTargetFinderFilter.hpp"
#include "NPC/NekoNormal.hpp"
#include "NPC/NpcFunction.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Screen/ScreenPointer.hpp"

namespace {

al::LiveActor* tryGetValidTarget(const NpcTargetFinder* pFinder, al::LiveActor* pActor,
                                 const npc::NpcFindTargetType& rType);

/**
 * @brief Check whether the height difference to a target is within the allowed range.
 * @param rInfo Relation to the target.
 * @param pParam Search parameters.
 * @return Whether the height difference is acceptable.
 */
inline bool isInHeightRange(const NpcTargetCheckInfo& rInfo, const NpcTargetFinderParam* pParam) {
    f32 dy = rInfo.mDirToTarget.y;
    if (dy >= 0.0f) {
        return pParam->mUpperRange < 0.0f || dy <= pParam->mUpperRange;
    }

    return pParam->mLowerRange < 0.0f || pParam->mLowerRange >= -dy;
}

/**
 * @brief Check whether a target is in the sight cone.
 * @param rInfo Relation to the target.
 * @param pParam Search parameters.
 * @return Whether the target is in sight.
 */
inline bool isInSight(const NpcTargetCheckInfo& rInfo, const NpcTargetFinderParam* pParam) {
    if (rInfo.mDistance > pParam->mSightRange) {
        return false;
    }

    if (rInfo.mFrontDot < cosf(pParam->mSightAngleH * (sead::Mathf::pi() / 180.0f))) {
        return false;
    }

    if (rInfo.mUpDot < cosf(pParam->mSightAngleV * (sead::Mathf::pi() / 180.0f))) {
        return false;
    }

    return isInHeightRange(rInfo, pParam);
}

/**
 * @brief Check whether a target is within chase range.
 * @param rInfo Relation to the target.
 * @param pParam Search parameters.
 * @return Whether the target can be chased.
 */
inline bool isInChaseRange(const NpcTargetCheckInfo& rInfo, const NpcTargetFinderParam* pParam) {
    if (rInfo.mDistance > pParam->mChaseRange) {
        return false;
    }

    return isInHeightRange(rInfo, pParam);
}

}  // namespace

/**
 * @return Front direction used for the sight cone.
 */
inline const sead::Vector3f& NpcTargetFinder::getFrontDir() const {
    return mFrontDir != nullptr ? *mFrontDir : al::getFront(mHost);
}

/**
 * @return Up direction used for the sight cone.
 */
inline const sead::Vector3f& NpcTargetFinder::getUpDir() const {
    return mSupportUpDir != nullptr ? *mSupportUpDir : al::getGravity(mHost);
}

namespace npc {

/**
 * @brief Check whether a sensor is the body of a ball (cat ball, ball or snowball).
 * @param pSensor Sensor to check.
 * @return Whether the sensor belongs to a ball.
 */
bool isSensorBall(const al::HitSensor* pSensor) {
    if (!al::isSensorMapObj(pSensor) || !al::isSensorName(pSensor, "Body")) {
        return false;
    }

    return al::isSensorHostName(pSensor, "BallNeko") || al::isSensorHostName(pSensor, "ボール") ||
           al::isSensorHostName(pSensor, "雪玉");
}

/**
 * @brief Check whether a sensor belongs to a cat gull.
 * @param pSensor Sensor to check.
 * @return Whether the sensor belongs to a bird.
 */
bool isSensorBird(const al::HitSensor* pSensor) {
    return al::isSensorMapObj(pSensor) && al::isSensorHostName(pSensor, "CatGull");
}

/**
 * @brief Check whether a sensor belongs to a regular cat.
 * @param pSensor Sensor to check.
 * @return Whether the sensor belongs to a cat.
 */
bool isSensorNeko(const al::HitSensor* pSensor) {
    return al::isSensorNpc(pSensor) && al::isSensorHostName(pSensor, "NekoNormal");
}

/**
 * @brief Check whether a sensor belongs to a disaster cat.
 * @param pSensor Sensor to check.
 * @return Whether the sensor belongs to a disaster cat.
 */
bool isSensorNekoDisaster(const al::HitSensor* pSensor) {
    return al::isSensorEnemy(pSensor) && al::isSensorHostName(pSensor, "NekoDisaster");
}

/**
 * @brief Get a readable name for a kind of target.
 * @param rType Kind of target (bit flags).
 * @return Name of the first kind set in the flags.
 */
const char* NpcFindTargetTypeToString(const NpcFindTargetType& rType) {
    if (rType == NpcFindTargetType_None) {
        return "None";
    }

    if ((rType & NpcFindTargetType_PlayerRegular) != 0) {
        return "PlayerRegular";
    }

    if ((rType & NpcFindTargetType_PlayerClimb) != 0) {
        return "PlayerClimb";
    }

    if ((rType & NpcFindTargetType_KoopaJr) != 0) {
        return "KoopaJr";
    }

    if ((rType & NpcFindTargetType_Cursor) != 0) {
        return "Cursor";
    }

    if ((rType & NpcFindTargetType_Ball) != 0) {
        return "Ball";
    }

    if ((rType & NpcFindTargetType_Koura) != 0) {
        return "Koura";
    }

    if ((rType & NpcFindTargetType_Bird) != 0) {
        return "Bird";
    }

    if ((rType & NpcFindTargetType_Neko) != 0) {
        return "Neko";
    }

    if ((rType & NpcFindTargetType_Player) != 0) {
        return "Player";
    }

    if ((rType & (NpcFindTargetType_Ball | NpcFindTargetType_Koura | NpcFindTargetType_Bird)) !=
        0) {
        return "ChaseObj";
    }

    return "Unknown";
}

/**
 * @brief Check whether a target is in the sight of a finder.
 * @param pFinder Finder searching for targets.
 * @param pTarget Target to check.
 * @param pParam Search parameters, or nullptr to use the ones of the finder.
 * @return Whether the target is in sight.
 */
bool calcIsTargetInSight(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                         const NpcTargetFinderParam* pParam) {
    NpcTargetCheckInfo info;
    if (!calcTargetCheckInfo(&info, pFinder->getHost(), pFinder->getFrontDir(),
                             pFinder->getUpDir(), pTarget)) {
        return false;
    }

    if (pParam == nullptr) {
        pParam = pFinder->getParam();
    }

    return isInSight(info, pParam);
}

/**
 * @brief Compute the relation between a host and a target.
 * @param pInfo Receives the direction, distance and sight cone dots.
 * @param pHost Searching actor.
 * @param rFront Front direction of the host.
 * @param rUp Up direction of the host.
 * @param pTarget Target actor.
 * @return Whether the relation could be computed (target not at the host position).
 */
bool calcTargetCheckInfo(NpcTargetCheckInfo* pInfo, const al::LiveActor* pHost,
                         const sead::Vector3f& rFront, const sead::Vector3f& rUp,
                         const al::LiveActor* pTarget) {
    sead::Vector3f dir = al::getTrans(pTarget) - al::getTrans(pHost);
    pInfo->mDirToTarget = dir;

    f32 squaredDistance = dir.squaredLength();
    if (al::isNearZero(squaredDistance)) {
        pInfo->mDistance = 0.0f;
        return false;
    }

    pInfo->mDistance = nerd::sqrt(squaredDistance);

    sead::Vector3f side;
    al::verticalizeVec(&side, rFront, rUp);
    if (al::normalizeOrZero(&side)) {
        return false;
    }

    dir *= 1.0f / pInfo->mDistance;
    f32 sideDot = side.dot(dir);

    f32 upDot = 0.0f;
    if (!al::isNearZero(sead::Mathf::abs(sideDot) - 1.0f)) {
        upDot = nerd::sqrt(1.0f - sideDot * sideDot);
    }

    pInfo->mUpDot = upDot;

    sead::Vector3f frontDir = dir - side * upDot;
    f32 frontDot = 0.0f;
    if (!al::normalizeOrZero(&frontDir)) {
        frontDot = rFront.dot(frontDir);
    }

    pInfo->mFrontDot = frontDot;
    return true;
}

/**
 * @brief Check whether a target is within the chase range of a finder.
 * @param pFinder Finder searching for targets.
 * @param pTarget Target to check.
 * @param pParam Search parameters, or nullptr to use the ones of the finder.
 * @return Whether the target can be chased.
 */
bool calcIsTargetInChaseRange(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                              const NpcTargetFinderParam* pParam) {
    NpcTargetCheckInfo info;
    if (!calcTargetCheckInfo(&info, pFinder->getHost(), pFinder->getFrontDir(),
                             pFinder->getUpDir(), pTarget)) {
        return false;
    }

    if (pParam == nullptr) {
        pParam = pFinder->getParam();
    }

    return isInChaseRange(info, pParam);
}

}  // namespace npc

namespace {

/**
 * @brief Check whether a target is within the chase range of a finder.
 * @note Same as npc::calcIsTargetInChaseRange with the parameters of the finder, but inlined.
 * @param pFinder Finder searching for targets.
 * @param pTarget Target to check.
 * @return Whether the target can be chased.
 */
inline bool canChaseTarget(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget) {
    NpcTargetCheckInfo info;
    if (!npc::calcTargetCheckInfo(&info, pFinder->getHost(), pFinder->getFrontDir(),
                                  pFinder->getUpDir(), pTarget)) {
        return false;
    }

    return isInChaseRange(info, pFinder->getParam());
}

}  // namespace

/**
 * @brief Construct the default parameters.
 */
NpcTargetFinderParam::NpcTargetFinderParam()
    : mSightRange(500.0f), mSightAngleH(80.0f), mSightAngleV(80.0f), mKeepTargetFrame(60),
      mSenseRange(-1.0f), mUpperRange(-1.0f), mLowerRange(-1.0f), mChaseRange(-1.0f),
      mIsIgnoreDisregardPlayer(false), _24(5), mCandidateClearFrame(1) {}

/**
 * @brief Construct the parameters.
 * @param sightRange Distance within which targets can be seen.
 * @param sightAngleH Horizontal half angle of the sight cone, in degrees.
 * @param sightAngleV Vertical half angle of the sight cone, in degrees.
 * @param keepTargetFrame Frames a found target is kept before searching again.
 * @param senseRange Distance within which targets are sensed without sight.
 * @param upperRange Height above the host targets can be at (negative: unlimited).
 * @param lowerRange Depth below the host targets can be at (negative: unlimited).
 * @param chaseRange Distance within which a target keeps being chased.
 * @param isIgnoreDisregardPlayer Whether players the NPCs should disregard are ignored.
 * @param candidateClearFrame Frames between two clears of the seen candidates.
 */
NpcTargetFinderParam::NpcTargetFinderParam(f32 sightRange, f32 sightAngleH, f32 sightAngleV,
                                           u32 keepTargetFrame, f32 senseRange, f32 upperRange,
                                           f32 lowerRange, f32 chaseRange,
                                           bool isIgnoreDisregardPlayer, u32 candidateClearFrame)
    : mSightRange(sightRange), mSightAngleH(sightAngleH), mSightAngleV(sightAngleV),
      mKeepTargetFrame(keepTargetFrame), mSenseRange(senseRange), mUpperRange(upperRange),
      mLowerRange(lowerRange), mChaseRange(chaseRange),
      mIsIgnoreDisregardPlayer(isIgnoreDisregardPlayer), _24(5),
      mCandidateClearFrame(candidateClearFrame) {}

/**
 * @brief Construct a target finder.
 * @param pHost Actor that searches for targets.
 * @param pParam Search parameters; must outlive the finder.
 */
NpcTargetFinder::NpcTargetFinder(al::LiveActor* pHost, const NpcTargetFinderParam* pParam)
    : mHost(pHost), mCandidateClearTime(pParam->mCandidateClearFrame), mParam(pParam),
      mCheckInfo(new NpcTargetCheckInfo()) {
    mCheckInfo->reset();
}

/**
 * @brief Change the actor that searches for targets.
 * @param pHost New host actor.
 * @param pSensorName Name of the eye sensor of the host.
 * @param pFilter Filter that can reject targets, or nullptr.
 */
void NpcTargetFinder::changeHost(al::LiveActor* pHost, const char* pSensorName,
                                 const IUseTargetFinderFilter* pFilter) {
    mHost = pHost;
    mFilter = pFilter;
    mEyeSensor = al::getHitSensor(pHost, pSensorName);

    f32 sightRange = mParam->mSightRange;
    f32 senseRange = mParam->mSenseRange;
    al::setSensorRadius(pHost, pSensorName, sightRange > senseRange ? sightRange : senseRange);
}

/**
 * @brief Add an actor to the candidates if it can be a target.
 * @param pActor Actor seen by the eye sensor.
 * @param type Kind of the actor.
 */
inline void NpcTargetFinder::addCandidate(al::LiveActor* pActor, npc::NpcFindTargetType type) {
    // The check gets its own copy of the type, the candidate is built from the parameter.
    al::LiveActor* target = tryGetValidTarget(this, pActor, npc::NpcFindTargetType(type));
    if (target != nullptr) {
        mCandidates.emplaceBack(target, type);
    }
}

/**
 * @brief Collect candidates seen by the eye sensor.
 * @param pSelf Sensor of the host.
 * @param pOther Sensor that was hit.
 */
void NpcTargetFinder::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mCandidateClearTime != mParam->mCandidateClearFrame || mCandidates.isFull() ||
        !al::isSensorEye(pSelf)) {
        return;
    }

    if ((mSearchTypes & npc::NpcFindTargetType_Player) != 0 && al::isSensorPlayer(pOther) &&
        al::isSensorName(pOther, "Body")) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_Player);
    } else if ((mSearchTypes & npc::NpcFindTargetType_KoopaJr) != 0 &&
               al::isSensorKoopaJr(pOther)) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_KoopaJr);
    } else if ((mSearchTypes & npc::NpcFindTargetType_Koura) != 0 &&
               al::isSensorKickKoura(pOther)) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_Koura);
    } else if ((mSearchTypes & npc::NpcFindTargetType_Ball) != 0 && npc::isSensorBall(pOther)) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_Ball);
    } else if ((mSearchTypes & npc::NpcFindTargetType_Bird) != 0 && npc::isSensorBird(pOther)) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_Bird);
    } else if ((mSearchTypes & npc::NpcFindTargetType_Neko) != 0 && npc::isSensorNeko(pOther)) {
        addCandidate(al::getSensorHost(pOther), npc::NpcFindTargetType_Neko);
    }
}

namespace {

/**
 * @brief Check whether an actor can be a target of a finder.
 * @param pFinder Finder searching for targets.
 * @param pActor Actor to check.
 * @param rType Kind of the actor.
 * @return The actor if it can be a target, nullptr otherwise.
 */
al::LiveActor* tryGetValidTarget(const NpcTargetFinder* pFinder, al::LiveActor* pActor,
                                 const npc::NpcFindTargetType& rType) {
    if (pActor == nullptr) {
        return nullptr;
    }

    if (al::isDead(pActor)) {
        return nullptr;
    }

    if ((pFinder->getSearchTypes() & rType) == 0) {
        return nullptr;
    }

    // the switch compares signed values while the priority map compares unsigned ones
    switch (static_cast<s32>(rType)) {
    case npc::NpcFindTargetType_None:
        return nullptr;
    case npc::NpcFindTargetType_PlayerRegular:
    case npc::NpcFindTargetType_PlayerClimb:
    case npc::NpcFindTargetType_Player:
        if (rc::isPlayerDeadOrBubble(pActor)) {
            return nullptr;
        }

        if (pFinder->getParam()->mIsIgnoreDisregardPlayer && rc::isPlayerEquipDisregard(pActor)) {
            return nullptr;
        }

        break;
    case npc::NpcFindTargetType_Cursor:
        if (al::isHideModel(pActor)) {
            return nullptr;
        }

        break;
    case npc::NpcFindTargetType_Neko: {
        if (al::isHideModel(pActor)) {
            return nullptr;
        }

        auto* neko = static_cast<NekoNormal*>(pActor);
        if (neko->isHold() || neko->isEnableGoal()) {
            return nullptr;
        }

        break;
    }
    default:
        break;
    }

    const IUseTargetFinderFilter* filter = pFinder->getFilter();
    if (filter != nullptr && filter->acceptTarget(pActor, rType)) {
        return pActor;
    }

    return nullptr;
}

}  // namespace

/**
 * @brief Does nothing.
 * @param pMsg Received message.
 * @param pSelf Sensor of the host.
 * @param pOther Sensor that sent the message.
 * @return Always false.
 */
bool NpcTargetFinder::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf,
                                 al::HitSensor* pOther) {
    return false;
}

/**
 * @brief Collect the touch cursor as a candidate when it points at the eye of the host.
 * @param pMsg Received message.
 * @param pPointer Pointer that sent the message.
 * @param pTarget Screen point target that was hit.
 * @return Always false.
 */
bool NpcTargetFinder::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if ((mSearchTypes & npc::NpcFindTargetType_Cursor) != 0 &&
        mCandidateClearTime == mParam->mCandidateClearFrame && !mCandidates.isFull() &&
        al::isMsgTouchAssist(pMsg) && al::isScreenPointTargetName(pTarget, "Eye")) {
        addCandidate(const_cast<al::LiveActor*>(pPointer->getHost()),
                     npc::NpcFindTargetType_Cursor);
    }

    return false;
}

/**
 * @brief Forget the current target and the best target found so far.
 */
inline void NpcTargetFinder::resetTarget() {
    mTarget = nullptr;
    mTargetType = npc::NpcFindTargetType_None;
    mCheckInfo->reset();
}

/**
 * @brief Pick the best target among the candidates.
 */
void NpcTargetFinder::findTarget() {
    if (mTarget == nullptr) {
        resetTarget();
    }

    for (s32 i = 0; i < mCandidates.size(); i++) {
        NpcTargetCandidate* candidate = mCandidates.at(i);
        if (candidate == nullptr || candidate->mActor == nullptr) {
            continue;
        }

        NpcTargetCheckInfo* info = mCheckInfo;
        u8 priority = getTargetPriority(candidate->mType);
        al::LiveActor* target = tryGetValidTarget(this, candidate->mActor, candidate->mType);
        if (target == nullptr) {
            continue;
        }

        if (!npc::calcTargetCheckInfo(info, mHost, getFrontDir(), getUpDir(), target)) {
            continue;
        }

        bool isNearer = info->mDistance < info->mNearestDistance;
        if (!(priority > info->mPriority || (isNearer && priority >= info->mPriority))) {
            continue;
        }

        bool isInSightTarget;
        if (isInSight(*info, mParam)) {
            isInSightTarget = true;
        } else if (isInChaseRange(*info, mParam)) {
            isInSightTarget = false;
        } else {
            continue;
        }

        info->mNearestDistance = info->mDistance;
        info->mTarget = target;
        info->mIsInSight = isInSightTarget;
        info->mIsFound = true;
        info->mPriority = priority;
        info->mTargetType = candidate->mType;
    }

    NpcTargetCheckInfo* info = mCheckInfo;
    if (info->mTarget != nullptr && info->mIsFound) {
        if (mTarget != nullptr) {
            mLastTarget = mTarget;
            mLastTargetType = mTargetType;
        }

        mTarget = info->mTarget;
        mTargetType = info->mTargetType;
        mIsTargetInChaseRange = true;
        mIsTargetValid = info->mIsInSight;
        mKeepTargetTime = mParam->mKeepTargetFrame;
    }

    if (mTarget != nullptr) {
        mIsTargetChanged = mLastTarget != mTarget;
    }
}

/**
 * @brief Get the priority of a kind of target.
 * @param rType Kind of target.
 * @return The priority set with setTargetTypePriority, or 0.
 */
u8 NpcTargetFinder::getTargetPriority(const npc::NpcFindTargetType& rType) const {
    PriorityMap::Node* node = mPriorityMap.find(rType);
    if (node == nullptr) {
        return 0;
    }

    return node->value();
}

/**
 * @brief Clear the candidates and restart the candidate clear timer.
 */
inline void NpcTargetFinder::clearCandidates() {
    mCandidates.clear();
    mCandidateClearTime = mParam->mCandidateClearFrame;
}

/**
 * @brief Update the target and the candidates, once per frame.
 */
void NpcTargetFinder::update() {
    mIsTargetChanged = false;
    updateTarget();

    if ((mTargetType & npc::NpcFindTargetType_Player) != 0) {
        mTargetType = rc::isPlayerClimbOrClimbSpecial(mTarget) ?
                          npc::NpcFindTargetType_PlayerClimb :
                          npc::NpcFindTargetType_PlayerRegular;
    }

    mCandidateClearTime--;
    if (mCandidateClearTime <= 0) {
        clearCandidates();
    }
}

/**
 * @brief Remember the current target as the last one and drop it.
 */
inline void NpcTargetFinder::loseTarget() {
    mLastTarget = mTarget;
    mTarget = nullptr;
    mLastTargetType = mTargetType;
}

/**
 * @brief Check whether the current target is still valid and search for a new one if needed.
 */
void NpcTargetFinder::updateTarget() {
    if (mTarget != nullptr && (tryGetValidTarget(this, mTarget, mTargetType) == nullptr ||
                               !canChaseTarget(this, mTarget))) {
        loseTarget();
    }

    if (mTarget == nullptr) {
        findTarget();
        return;
    }

    if (!npc::calcTargetCheckInfo(mCheckInfo, mHost, getFrontDir(), al::getGravity(mHost),
                                  mTarget)) {
        findTarget();
        return;
    }

    bool isInSightTarget;
    if (isInSight(*mCheckInfo, mParam)) {
        isInSightTarget = true;
    } else if (isInChaseRange(*mCheckInfo, mParam)) {
        isInSightTarget = false;
    } else {
        findTarget();
        return;
    }

    mIsTargetValid = isInSightTarget;
    mIsTargetInChaseRange = true;

    if (mTarget != nullptr && mKeepTargetTime-- <= 0) {
        findTarget();
    }
}

/**
 * @brief Search for a target right away and update the finder.
 */
void NpcTargetFinder::forceUpdate() {
    findTarget();

    mKeepTargetTime = mKeepTargetTime > 1 ? mKeepTargetTime : 1;

    update();
    clearCandidates();
}

/**
 * @brief Forget the current and last targets and search again.
 */
void NpcTargetFinder::refindTarget() {
    mTarget = nullptr;
    mLastTarget = nullptr;
    mTargetType = npc::NpcFindTargetType_None;
    mLastTargetType = npc::NpcFindTargetType_None;
    findTarget();
}

/**
 * @brief Set the priority of a kind of target; higher priorities win over nearer targets.
 * @param rType Kind of target.
 * @param rPriority Priority.
 */
void NpcTargetFinder::setTargetTypePriority(const npc::NpcFindTargetType& rType,
                                            const u8& rPriority) {
    // The result of this lookup is unused, insert() overwrites the priority of an existing type.
    mPriorityMap.find(rType);
    mPriorityMap.insert(rType, rPriority);
}

/**
 * @brief Remove every target priority.
 */
void NpcTargetFinder::clearPriorityMap() {
    mPriorityMap.clear();
}

/**
 * @brief Drop the current target and the candidates.
 */
void NpcTargetFinder::clearTarget() {
    if (mTarget != nullptr) {
        mLastTarget = mTarget;
        mLastTargetType = mTargetType;
    }

    mTarget = nullptr;
    mTargetType = npc::NpcFindTargetType_None;
    clearCandidates();
}

/**
 * @brief Set the front direction used for the sight cone.
 * @param pFrontDir Front direction, or nullptr to use the front of the host.
 */
void NpcTargetFinder::setFrontDir(sead::Vector3f* pFrontDir) {
    mFrontDir = pFrontDir;
}

/**
 * @brief Set the up direction used for the sight cone.
 * @param pUpDir Up direction, or nullptr to use the gravity of the host.
 */
void NpcTargetFinder::setSupportUpDir(sead::Vector3f* pUpDir) {
    mSupportUpDir = pUpDir;
}

/**
 * @return Position of the current target.
 */
const sead::Vector3f& NpcTargetFinder::getTargetPos() const {
    return al::getTrans(mTarget);
}

/**
 * @return Position of the current target, or of the last one if there is none.
 */
const sead::Vector3f& NpcTargetFinder::getLastTargetPos() const {
    if (mTarget != nullptr) {
        return al::getTrans(mTarget);
    }

    return al::getTrans(mLastTarget);
}

/**
 * @brief Check whether the target is in sight and horizontally near the host.
 * @param distance Maximum horizontal distance.
 * @return Whether the target is in sight and near.
 */
bool NpcTargetFinder::isInSightTarget(f32 distance) const {
    if (mTarget == nullptr || !mIsTargetValid) {
        return false;
    }

    return al::calcDistanceH(mHost, mTarget) <= distance;
}

/**
 * @brief Check whether the target is chased and horizontally near the host.
 * @param distance Maximum horizontal distance.
 * @return Whether the target is chased and near.
 */
bool NpcTargetFinder::isInChaseRangeTarget(f32 distance) const {
    if (mTarget == nullptr || !mIsTargetInChaseRange) {
        return false;
    }

    return al::calcDistanceH(mHost, mTarget) <= distance;
}

/**
 * @return Whether the target is chased and horizontally within the sense range.
 */
bool NpcTargetFinder::isInSenseAreaTarget() const {
    return isInChaseRangeTarget(mParam->mSenseRange);
}
