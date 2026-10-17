#include "NPC/NpcTargetFinder.hpp"
#include "NPC/IUseTargetFinderFilter.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Library/Screen/ScreenPointer.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "NPC/NekoNormal.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include <cmath>

/**
 * @brief Initializes default sight, chase, and scan settings.
 */
NpcTargetFinderParam::NpcTargetFinderParam()
    : mSightDistance(500.0f), mHorizontalAngle(80.0f), mVerticalAngle(80.0f),
      mTargetGraceFrames(60), mSenseDistance(-1.0f), mMaxHeight(-1.0f), mMaxDepth(-1.0f),
      mChaseDistance(-1.0f), mParam20(false), mCandidateCapacity(5), mScanInterval(1) {}
/**
 * @brief Stores target search and retention settings.
 * @param sightDistance Maximum distance for sight checks.
 * @param horizontalAngle Horizontal sight angle in degrees.
 * @param verticalAngle Vertical sight angle in degrees.
 * @param targetGraceFrames Frames to retain a target before searching again.
 * @param senseDistance Radius of the sense area.
 * @param maxHeight Maximum positive height difference; negative disables the limit.
 * @param maxDepth Maximum negative height difference; negative disables the limit.
 * @param chaseDistance Maximum chase distance.
 * @param param20 Whether to reject players reported as disregarded by their equipment.
 * @param scanInterval Frame interval between sensor candidate collections.
 */
NpcTargetFinderParam::NpcTargetFinderParam(float sightDistance, float horizontalAngle,
                                           float verticalAngle, unsigned int targetGraceFrames,
                                           float senseDistance, float maxHeight, float maxDepth,
                                           float chaseDistance, bool param20,
                                           unsigned int scanInterval)
    : mSightDistance(sightDistance), mHorizontalAngle(horizontalAngle),
      mVerticalAngle(verticalAngle), mTargetGraceFrames(targetGraceFrames),
      mSenseDistance(senseDistance), mMaxHeight(maxHeight), mMaxDepth(maxDepth),
      mChaseDistance(chaseDistance), mParam20(param20), mCandidateCapacity(5),
      mScanInterval(scanInterval) {}
/**
 * @brief Creates candidate storage and target-priority tracking.
 * @param pHost Actor searching for targets.
 * @param pParam Search settings; must not be null.
 */
NpcTargetFinder::NpcTargetFinder(al::LiveActor* pHost, const NpcTargetFinderParam* pParam)
    : mHostActor(pHost), mFilter(nullptr), mTarget(nullptr), mLastTarget(nullptr),
      mType(npc::NpcFindTargetType::None), mLastType(npc::NpcFindTargetType::None),
      mAllowedTypes(-1), mInSight(false), mIsInChaseRange(false), mHasTargetChanged(false),
      mFront(nullptr), mGravity(nullptr), mGraceFrames(0), mScanFrames(pParam->mScanInterval),
      mParam(pParam), mMetrics(new npc::TargetMetrics()), mSensor(nullptr) {
    mMetrics->reset();
}
/**
 * @brief Changes the host, sensor, and target filter.
 * @param pHost New searching actor.
 * @param pSensor Name of its search sensor.
 * @param pFilter Filter that approves candidate actors.
 */
void NpcTargetFinder::changeHost(al::LiveActor* pHost, const char* pSensor,
                                 const IUseTargetFinderFilter* pFilter) {
    mHostActor = pHost;
    mFilter = pFilter;
    mSensor = al::getHitSensor(pHost, pSensor);
    al::setSensorRadius(pHost, pSensor,
                        mParam->mSightDistance > mParam->mSenseDistance ? mParam->mSightDistance
                                                                        : mParam->mSenseDistance);
}
/**
 * @brief Leaves ordinary sensor messages unhandled.
 * @param pMessage Message; unused.
 * @param pSelf Sender sensor; unused.
 * @param pOther Other sensor; unused.
 * @return Always false.
 */
bool NpcTargetFinder::receiveMsg(const al::SensorMsg* pMessage, al::HitSensor* pSelf,
                                 al::HitSensor* pOther) {
    return false;
}
/**
 * @brief Looks up a target category priority.
 * @param rType Target category to find.
 * @return Configured priority, or zero if absent.
 */
unsigned char NpcTargetFinder::getTargetPriority(const npc::NpcFindTargetType& rType) const {
    auto* pNode = mPriorities.find(rType);
    return pNode ? pNode->value() : 0;
}
/**
 * @brief Updates the current target and periodically clears collected candidates.
 */
void NpcTargetFinder::update() {
    mHasTargetChanged = false;
    updateTarget();
    if (static_cast<unsigned int>(mType) & 3) {
        mType = rc::isPlayerClimbOrClimbSpecial(mTarget) ? npc::NpcFindTargetType::PlayerClimb
                                                         : npc::NpcFindTargetType::PlayerRegular;
    }
    if (--mScanFrames <= 0) {
        mCandidates.clear();
        mScanFrames = mParam->mScanInterval;
    }
}
/**
 * @brief Searches immediately, refreshes the target, and clears candidates.
 */
void NpcTargetFinder::forceUpdate() {
    findTarget();
    mGraceFrames = mGraceFrames > 1 ? mGraceFrames : 1;
    update();
    mCandidates.clear();
    mScanFrames = mParam->mScanInterval;
}
/**
 * @brief Discards current and previous targets before searching again.
 */
void NpcTargetFinder::refindTarget() {
    mTarget = nullptr;
    mLastTarget = nullptr;
    mType = npc::NpcFindTargetType::None;
    mLastType = npc::NpcFindTargetType::None;
    findTarget();
}
/**
 * @brief Assigns a priority to a target category.
 * @param rType Target category.
 * @param rPriority Priority value; higher values are preferred.
 */
void NpcTargetFinder::setTargetTypePriority(const npc::NpcFindTargetType& rType,
                                            const unsigned char& rPriority) {
    mPriorities.find(rType);
    mPriorities.insert(rType, rPriority);
}
/**
 * @brief Removes all target-category priorities.
 */
void NpcTargetFinder::clearPriorityMap() { mPriorities.clear(); }
/**
 * @brief Retains the previous target and resets the current search.
 */
void NpcTargetFinder::clearTarget() {
    if (mTarget) {
        mLastTarget = mTarget;
        mLastType = mType;
    }
    mTarget = nullptr;
    mType = npc::NpcFindTargetType::None;
    mCandidates.clear();
    mScanFrames = mParam->mScanInterval;
}
/**
 * @brief Sets the optional facing-direction override.
 * @param pDirection Direction override, or nullptr to use the host front.
 */
void NpcTargetFinder::setFrontDir(sead::Vector3f* pDirection) { mFront = pDirection; }
/**
 * @brief Sets the optional support-direction override.
 * @param pDirection Direction override, or nullptr to use host gravity.
 */
void NpcTargetFinder::setSupportUpDir(sead::Vector3f* pDirection) { mGravity = pDirection; }
/**
 * @brief Gets the current target position.
 * @return Position of the current target; a current target is required.
 */
const sead::Vector3f& NpcTargetFinder::getTargetPos() const { return al::getTrans(mTarget); }
/**
 * @brief Gets the current or most recent target position.
 * @return Current target position, falling back to the previous target; one must exist.
 */
const sead::Vector3f& NpcTargetFinder::getLastTargetPos() const {
    if (mTarget) {
        return al::getTrans(mTarget);
    }
    return al::getTrans(mLastTarget);
}
/**
 * @brief Checks sight visibility and horizontal distance.
 * @param distance Maximum horizontal distance.
 * @return Whether the current target satisfies both checks.
 */
bool NpcTargetFinder::isInSightTarget(float distance) const {
    if (!mTarget) {
        return false;
    }
    if (!mInSight) {
        return false;
    }
    return al::calcDistanceH(mHostActor, mTarget) <= distance;
}
/**
 * @brief Checks chase validity and horizontal distance.
 * @param distance Maximum horizontal distance.
 * @return Whether the current target satisfies both checks.
 */
bool NpcTargetFinder::isInChaseRangeTarget(float distance) const {
    if (!mTarget) {
        return false;
    }
    if (!mIsInChaseRange) {
        return false;
    }
    return al::calcDistanceH(mHostActor, mTarget) <= distance;
}
/**
 * @brief Checks the current target against the configured sense radius.
 * @return Whether a valid chase target is inside the sense area.
 */
bool NpcTargetFinder::isInSenseAreaTarget() const {
    if (!mTarget) {
        return false;
    }
    if (!mIsInChaseRange) {
        return false;
    }
    const float distance = mParam->mSenseDistance;
    return al::calcDistanceH(mHostActor, mTarget) <= distance;
}

/**
 * @brief Collects eligible actors seen by the search sensor.
 * @param pSelf The searching actor sensor.
 * @param pOther Potential target sensor.
 */
void NpcTargetFinder::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (mScanFrames != mParam->mScanInterval || mCandidates.isFull() || !al::isSensorEye(pSelf)) {
        return;
    }
    if ((mAllowedTypes & 3) && al::isSensorPlayer(pOther) && al::isSensorName(pOther, "Body")) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::Player)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Player);
        }
    } else if ((mAllowedTypes & 4) && al::isSensorKoopaJr(pOther)) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::KoopaJr)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::KoopaJr);
        }
    } else if ((mAllowedTypes & 32) && al::isSensorKickKoura(pOther)) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::Koura)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Koura);
        }
    } else if ((mAllowedTypes & 16) && al::isSensorMapObj(pOther) &&
               al::isSensorName(pOther, "Body") &&
               (al::isSensorHostName(pOther, "BallNeko") ||
                al::isSensorHostName(pOther, "\u30dc\u30fc\u30eb") ||
                al::isSensorHostName(pOther, "\u96ea\u7389"))) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::Ball)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Ball);
        }
    } else if ((mAllowedTypes & 64) && al::isSensorMapObj(pOther) &&
               al::isSensorHostName(pOther, "CatGull")) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::Bird)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Bird);
        }
    } else if ((mAllowedTypes & 128) && al::isSensorNpc(pOther) &&
               al::isSensorHostName(pOther, "NekoNormal")) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(al::getSensorHost(pOther), npc::NpcFindTargetType::Neko)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Neko);
        }
    }
}
/**
 * @brief Collects an eligible touch pointer as a cursor target.
 * @param pMessage Touch-assist message to inspect.
 * @param pPointer Pointer whose host may become a candidate.
 * @param pTarget Screen target; must be named Eye to collect a candidate.
 * @return Always false, allowing other receivers to process the message.
 */
bool NpcTargetFinder::receiveMsgScreenPoint(const al::SensorMsg* pMessage,
                                            al::ScreenPointer* pPointer,
                                            al::ScreenPointTarget* pTarget) {
    if ((mAllowedTypes & 8) && mScanFrames == mParam->mScanInterval && !mCandidates.isFull() &&
        al::isMsgTouchAssist(pMessage) && al::isScreenPointTargetName(pTarget, "Eye")) {
        if (const al::LiveActor* pCandidate =
                tryFilterTarget(pPointer->getHost(), npc::NpcFindTargetType::Cursor)) {
            mCandidates.emplaceBack(pCandidate, npc::NpcFindTargetType::Cursor);
        }
    }
    return false;
}

/**
 * @brief Applies target masks, actor state checks, and the host filter.
 * @param pActor Candidate actor, or nullptr.
 * @param rType Candidate category.
 * @return The candidate actor if accepted, otherwise nullptr.
 */
const al::LiveActor* NpcTargetFinder::tryFilterTarget(const al::LiveActor* pActor,
                                                      const npc::NpcFindTargetType& rType) const {
    if (!pActor) {
        goto done;
    }
    if (al::isDead(pActor)) {
        goto rejected;
    }
    if (!(mAllowedTypes & static_cast<unsigned int>(rType))) {
        goto rejected;
    }
    switch (rType) {
    case npc::NpcFindTargetType::None:
        goto rejected;
    case npc::NpcFindTargetType::PlayerRegular:
    case npc::NpcFindTargetType::PlayerClimb:
    case npc::NpcFindTargetType::Player:
        if (rc::isPlayerDeadOrBubble(pActor)) {
            goto rejected;
        }
        if (mParam->mParam20 && rc::isPlayerEquipDisregard(pActor)) {
            goto rejected;
        }
        break;
    case npc::NpcFindTargetType::Cursor:
        if (al::isHideModel(pActor)) {
            goto rejected;
        }
        break;
    case npc::NpcFindTargetType::Neko: {
        const NekoNormal* pNeko = static_cast<const NekoNormal*>(pActor);
        if (al::isHideModel(pActor) || pNeko->isHold() || pNeko->isEnableGoal()) {
            goto rejected;
        }
        break;
    }
    default:
        break;
    }
    if (!mFilter || !mFilter->acceptTarget(pActor, rType)) {
        goto rejected;
    }
    goto done;
rejected:
    pActor = nullptr;
done:
    return pActor;
}
namespace {
/**
 * @brief Tests the permitted vertical separation.
 * @param rMetrics Calculated target geometry.
 * @param rParam Height and depth limits.
 * @return Whether the target is inside the vertical limits.
 */
bool isWithinHeight(const npc::TargetMetrics& rMetrics, const NpcTargetFinderParam& rParam) {
    if (rMetrics.delta.y >= 0.0f) {
        return rParam.mMaxHeight < 0.0f || rMetrics.delta.y <= rParam.mMaxHeight;
    }
    return rParam.mMaxDepth < 0.0f || rParam.mMaxDepth >= -rMetrics.delta.y;
}
/**
 * @brief Tests sight distance, facing, and vertical limits.
 * @param rMetrics Calculated target geometry.
 * @param rParam Sight settings.
 * @return Whether the target passes the sight checks.
 */
bool isWithinSight(const npc::TargetMetrics& rMetrics, const NpcTargetFinderParam& rParam) {
    if (rMetrics.distance > rParam.mSightDistance) {
        return false;
    }
    if (rMetrics.horizontalCos < std::cos(rParam.mHorizontalAngle * 0.017453292f)) {
        return false;
    }
    if (rMetrics.verticalCos < std::cos(rParam.mVerticalAngle * 0.017453292f)) {
        return false;
    }
    return isWithinHeight(rMetrics, rParam);
}
/**
 * @brief Tests chase distance and vertical limits.
 * @param rMetrics Calculated target geometry.
 * @param rParam Chase settings.
 * @return Whether the target passes the chase checks.
 */
bool isWithinChase(const npc::TargetMetrics& rMetrics, const NpcTargetFinderParam& rParam) {
    if (rMetrics.distance > rParam.mChaseDistance) {
        return false;
    }
    return isWithinHeight(rMetrics, rParam);
}
} // namespace
/**
 * @brief Chooses the best valid candidate by priority and distance.
 */
void NpcTargetFinder::findTarget() {
    if (!mTarget) {
        mTarget = nullptr;
        mType = npc::NpcFindTargetType::None;
        mMetrics->reset();
    }
    for (int i = 0; i < mCandidates.size(); ++i) {
        Candidate* pCandidate = mCandidates[i];
        if (!pCandidate || !pCandidate->mActor) {
            continue;
        }
        npc::TargetMetrics* pMetrics = mMetrics;
        unsigned int priority = getTargetPriority(pCandidate->mType);
        const al::LiveActor* pTarget = tryFilterTarget(pCandidate->mActor, pCandidate->mType);
        if (!pTarget) {
            continue;
        }
        const al::LiveActor* pHost = mHostActor;
        const sead::Vector3f* pFront = mFront;
        if (!pFront) {
            pFront = &al::getFront(pHost);
        }
        const sead::Vector3f* pGravity = mGravity;
        if (!pGravity) {
            pGravity = &al::getGravity(mHostActor);
        }
        if (!npc::calcTargetMetrics(pMetrics, pHost, *pFront, *pGravity, pTarget)) {
            continue;
        }
        const bool closer = pMetrics->distance < pMetrics->mBestDistance;
        const bool acceptablePriority = priority >= pMetrics->mBestPriority;
        if (!((closer & acceptablePriority) || priority > pMetrics->mBestPriority)) {
            continue;
        }
        bool inSight = isWithinSight(*pMetrics, *mParam);
        if (!inSight && !isWithinChase(*pMetrics, *mParam)) {
            continue;
        }
        pMetrics->mBestDistance = pMetrics->distance;
        pMetrics->mBestTarget = pTarget;
        pMetrics->mBestInSight = inSight;
        pMetrics->mHasCandidate = true;
        pMetrics->mBestPriority = priority;
        pMetrics->mBestType = pCandidate->mType;
    }
    if (mMetrics->mBestTarget && mMetrics->mHasCandidate) {
        if (mTarget) {
            mLastTarget = mTarget;
            mLastType = mType;
        }
        mTarget = mMetrics->mBestTarget;
        mType = mMetrics->mBestType;
        mIsInChaseRange = true;
        mInSight = mMetrics->mBestInSight;
        mGraceFrames = mParam->mTargetGraceFrames;
    }
    if (mTarget) {
        mHasTargetChanged = mLastTarget != mTarget;
    }
}
/**
 * @brief Retains a valid target or searches again when necessary.
 */
void NpcTargetFinder::updateTarget() {
    if (mTarget) {
        bool keep = false;
        if (tryFilterTarget(mTarget, mType)) {
            const al::LiveActor* pTarget = mTarget;
            npc::TargetMetrics metrics;
            const al::LiveActor* pHost = mHostActor;
            const sead::Vector3f* pFront = mFront;
            if (!pFront) {
                pFront = &al::getFront(pHost);
            }
            const sead::Vector3f* pGravity = mGravity;
            if (!pGravity) {
                pGravity = &al::getGravity(mHostActor);
            }
            keep = npc::calcTargetMetrics(&metrics, pHost, *pFront, *pGravity, pTarget) &&
                   isWithinChase(metrics, *mParam);
        }
        if (!keep) {
            mLastTarget = mTarget;
            mTarget = nullptr;
            mLastType = mType;
        }
    }
    if (!mTarget) {
        findTarget();
        return;
    }
    const al::LiveActor* pHost = mHostActor;
    npc::TargetMetrics* pMetrics = mMetrics;
    const sead::Vector3f* pFront = mFront;
    if (!pFront) {
        pFront = &al::getFront(pHost);
    }
    const sead::Vector3f& rGravity = al::getGravity(mHostActor);
    if (!npc::calcTargetMetrics(pMetrics, pHost, *pFront, rGravity, mTarget)) {
        findTarget();
        return;
    }
    bool inSight = isWithinSight(*mMetrics, *mParam);
    if (!inSight && !isWithinChase(*mMetrics, *mParam)) {
        findTarget();
        return;
    }
    mInSight = inSight;
    mIsInChaseRange = true;
    if (mTarget && mGraceFrames-- <= 0) {
        findTarget();
    }
}

namespace npc {

/**
 * @brief Checks for a supported ball body sensor.
 * @param pSensor Sensor to inspect.
 * @return Whether the sensor belongs to a supported ball.
 */
bool isSensorBall(const al::HitSensor* pSensor) {
    if (!al::isSensorMapObj(pSensor)) {
        return false;
    }
    if (!al::isSensorName(pSensor, "Body")) {
        return false;
    }
    return al::isSensorHostName(pSensor, "BallNeko") ||
           al::isSensorHostName(pSensor, "\u30dc\u30fc\u30eb") ||
           al::isSensorHostName(pSensor, "\u96ea\u7389");
}
/**
 * @brief Checks for a cat gull sensor.
 * @param pSensor Sensor to inspect.
 * @return Whether the sensor belongs to a cat gull.
 */
bool isSensorBird(const al::HitSensor* pSensor) {
    return al::isSensorMapObj(pSensor) && al::isSensorHostName(pSensor, "CatGull");
}
/**
 * @brief Checks for a normal cat sensor.
 * @param pSensor Sensor to inspect.
 * @return Whether the sensor belongs to a normal cat.
 */
bool isSensorNeko(const al::HitSensor* pSensor) {
    return al::isSensorNpc(pSensor) && al::isSensorHostName(pSensor, "NekoNormal");
}
/**
 * @brief Checks for a disaster cat sensor.
 * @param pSensor Sensor to inspect.
 * @return Whether the sensor belongs to a disaster cat.
 */
bool isSensorNekoDisaster(const al::HitSensor* pSensor) {
    return al::isSensorEnemy(pSensor) && al::isSensorHostName(pSensor, "NekoDisaster");
}
/**
 * @brief Gets the first applicable target-type label.
 * @param rType Target-type flags to describe.
 * @return Target label, or Unknown for unrecognized flags.
 */
const char* NpcFindTargetTypeToString(const NpcFindTargetType& rType) {
    unsigned int bits = static_cast<unsigned int>(rType);
    if (!bits) {
        return "None";
    }
    if (bits & 1) {
        return "PlayerRegular";
    }
    if (bits & 2) {
        return "PlayerClimb";
    }
    if (bits & 4) {
        return "KoopaJr";
    }
    if (bits & 8) {
        return "Cursor";
    }
    if (bits & 16) {
        return "Ball";
    }
    if (bits & 32) {
        return "Koura";
    }
    if (bits & 64) {
        return "Bird";
    }
    if (bits & 128) {
        return "Neko";
    }
    if (bits & 3) {
        return "Player";
    }
    if (bits & 0x70) {
        return "ChaseObj";
    }
    return "Unknown";
}

/**
 * @brief Calculates target displacement, distance, and facing measures.
 * @param pOut Receives the calculated target measures.
 * @param pHost Actor searching for a target.
 * @param rFront Host facing direction.
 * @param rGravity Host gravity direction.
 * @param pTarget Candidate target actor.
 * @return Whether the displacement and projected direction are usable.
 */
bool calcTargetMetrics(TargetMetrics* pOut, const al::LiveActor* pHost,
                       const sead::Vector3f& rFront, const sead::Vector3f& rGravity,
                       const al::LiveActor* pTarget) {
    sead::Vector3f delta = al::getTrans(pTarget) - al::getTrans(pHost);
    float lengthSquared = delta.squaredLength();
    pOut->delta = delta;
    if (al::isNearZero(lengthSquared)) {
        pOut->distance = 0.0f;
        return false;
    }
    pOut->distance = sead::Mathf::sqrt(lengthSquared);
    sead::Vector3f up;
    al::verticalizeVec(&up, rFront, rGravity);
    if (al::normalizeOrZero(&up)) {
        return false;
    }
    delta *= 1.0f / pOut->distance;
    float dot = up.dot(delta);
    float cosV = 0.0f;
    if (!al::isNearZero((dot > 0.0f ? dot : -dot) - 1.0f)) {
        cosV = sead::Mathf::sqrt(1.0f - dot * dot);
    }
    pOut->verticalCos = cosV;
    sead::Vector3f flat = delta - up * cosV;
    float cosH = 0.0f;
    if (!al::normalizeOrZero(&flat)) {
        cosH = rFront.dot(flat);
    }
    pOut->horizontalCos = cosH;
    return true;
}
/**
 * @brief Checks a candidate against sight and height limits.
 * @param pFinder Finder supplying the host and direction overrides.
 * @param pTarget Candidate target actor.
 * @param pParam Optional settings override; nullptr uses the finder settings.
 * @return Whether the target is within the sight limits.
 */
bool calcIsTargetInSight(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                         const NpcTargetFinderParam* pParam) {
    TargetMetrics metrics;
    const al::LiveActor* pHost = pFinder->mHostActor;
    const sead::Vector3f* pFront = pFinder->mFront;
    if (!pFront) {
        pFront = &al::getFront(pHost);
    }
    const sead::Vector3f* pGravity = pFinder->mGravity;
    if (!pGravity) {
        pGravity = &al::getGravity(pFinder->mHostActor);
    }
    if (!calcTargetMetrics(&metrics, pHost, *pFront, *pGravity, pTarget)) {
        return false;
    }
    if (!pParam) {
        pParam = pFinder->mParam;
    }
    if (metrics.distance > pParam->mSightDistance) {
        return false;
    }
    if (metrics.horizontalCos < std::cos(pParam->mHorizontalAngle * 0.017453292f)) {
        return false;
    }
    if (metrics.verticalCos < std::cos(pParam->mVerticalAngle * 0.017453292f)) {
        return false;
    }
    if (metrics.delta.y >= 0.0f) {
        return pParam->mMaxHeight < 0.0f || metrics.delta.y <= pParam->mMaxHeight;
    }
    return pParam->mMaxDepth < 0.0f || pParam->mMaxDepth >= -metrics.delta.y;
}
/**
 * @brief Checks a candidate against chase distance and height limits.
 * @param pFinder Finder supplying the host and direction overrides.
 * @param pTarget Candidate target actor.
 * @param pParam Optional settings override; nullptr uses the finder settings.
 * @return Whether the target is within chase range.
 */
bool calcIsTargetInChaseRange(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                              const NpcTargetFinderParam* pParam) {
    TargetMetrics metrics;
    const al::LiveActor* pHost = pFinder->mHostActor;
    const sead::Vector3f* pFront = pFinder->mFront;
    if (!pFront) {
        pFront = &al::getFront(pHost);
    }
    const sead::Vector3f* pGravity = pFinder->mGravity;
    if (!pGravity) {
        pGravity = &al::getGravity(pFinder->mHostActor);
    }
    if (!calcTargetMetrics(&metrics, pHost, *pFront, *pGravity, pTarget)) {
        return false;
    }
    if (!pParam) {
        pParam = pFinder->mParam;
    }
    if (metrics.distance > pParam->mChaseDistance) {
        return false;
    }
    if (metrics.delta.y >= 0.0f) {
        return pParam->mMaxHeight < 0.0f || metrics.delta.y <= pParam->mMaxHeight;
    }
    return pParam->mMaxDepth < 0.0f || pParam->mMaxDepth >= -metrics.delta.y;
}
} // namespace npc
