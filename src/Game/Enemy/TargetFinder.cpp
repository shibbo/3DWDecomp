#include "Enemy/TargetFinder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <math/seadMathCalcCommon.h>
#include <cfloat>

namespace {
struct TargetInfo {
    sead::Vector3f offset;
    f32 distance;
    f32 horizontalCos;
    f32 verticalCos;
};

/** @brief Calculates target distance and angular components.
 * @param pInfo Receives relative position and visibility measurements.
 * @param pHost Searching actor.
 * @param rFront Search direction.
 * @param rUp Axis perpendicular to the search plane.
 * @param pTarget Candidate target.
 * @return Whether the target has a usable direction and distance.
 */
bool calcTargetInfo(TargetInfo* pInfo, const al::LiveActor* pHost,
        const sead::Vector3f& rFront, const sead::Vector3f& rUp, const al::LiveActor* pTarget) {
    sead::Vector3f offset = al::getTrans(pTarget) - al::getTrans(pHost);
    const f32 lengthSquared = offset.squaredLength();
    pInfo->offset = offset;
    if (al::isNearZero(lengthSquared, 0.001f)) {
        pInfo->distance = 0.0f;
        return false;
    }
    pInfo->distance = sead::Mathf::sqrt(lengthSquared);
    sead::Vector3f front;
    al::verticalizeVec(&front, rFront, rUp);
    if (al::normalizeOrZero(&front)) { return false; }
    offset *= 1.0f / pInfo->distance;
    const f32 dot = front.dot(offset);
    f32 verticalCos = 0.0f;
    if (!al::isNearZero(sead::Mathf::abs(dot) - 1.0f, 0.001f)) {
        verticalCos = sead::Mathf::sqrt(1.0f - dot * dot);
    }
    pInfo->verticalCos = verticalCos;
    sead::Vector3f projected = offset - verticalCos * front;
    f32 horizontalCos = 0.0f;
    if (!al::normalizeOrZero(&projected)) { horizontalCos = rFront.dot(projected); }
    pInfo->horizontalCos = horizontalCos;
    return true;
}

/** @brief Tests distance, angles, and vertical bounds for a search region.
 * @param rInfo Target measurements.
 * @param pParam Search constraints.
 * @param distance Target distance.
 * @param range Maximum distance.
 * @return Whether all search constraints pass.
 */
inline bool isInSearchRange(const TargetInfo& rInfo, const TargetFinderParam* pParam, f32 distance, f32 range) {
    if (distance > range ||
        rInfo.horizontalCos < sead::Mathf::cos(sead::Mathf::deg2rad(pParam->_4)) ||
        rInfo.verticalCos < sead::Mathf::cos(sead::Mathf::deg2rad(pParam->_8))) { return false; }
    if (rInfo.offset.y >= 0.0f) {
        return pParam->_14 < 0.0f || rInfo.offset.y <= pParam->_14;
    }
    return pParam->_18 < 0.0f || -rInfo.offset.y <= pParam->_18;
}

}

/** @brief Initializes default range, angles, and target retention settings. */
TargetFinderParam::TargetFinderParam()
    : _0(500.0f), _4(80.0f), _8(80.0f), _C(60), _10(-1.0f), _14(-1.0f),
      _18(-1.0f), _1C(-1.0f), _20(false) {}

/** @brief Initializes custom target search settings.
 * @param range Maximum discovery distance.
 * @param angleH Horizontal search angle in degrees.
 * @param angleV Vertical search angle in degrees.
 * @param keepTime Initial target retention counter.
 * @param nearbyRange Distance for nearby targets outside the viewing cone.
 * @param heightUp Maximum positive vertical offset, or negative to disable.
 * @param heightDown Maximum negative vertical offset, or negative to disable.
 * @param keepRange Maximum distance for retaining a target outside discovery range.
 * @param disregardEquipment Whether to skip players with disregarded equipment.
 */
TargetFinderParam::TargetFinderParam(f32 range, f32 angleH, f32 angleV, u32 keepTime,
        f32 nearbyRange, f32 heightUp, f32 heightDown, f32 keepRange, bool disregardEquipment)
    : _0(range), _4(angleH), _8(angleV), _C(keepTime), _10(nearbyRange), _14(heightUp),
      _18(heightDown), _1C(keepRange), _20(disregardEquipment) {}

/** @brief Initializes an enemy's target search state.
 * @param pHost Searching actor.
 * @param pParam Search settings.
 */
TargetFinder::TargetFinder(al::LiveActor* pHost, const TargetFinderParam* pParam)
    : mActor(pHost), _8(nullptr), mIsFound(false), _11(false), mIsNewTarget(false), mFrontDir(nullptr),
      mSupportUpDir(nullptr), _28(0), mParams(pParam), _38(nullptr) {}

/** @brief Finds the nearest eligible player using the search cone and proximity range. */
void TargetFinder::findTarget() {
    const s32 playerNum = al::getPlayerNumMax(mActor);
    al::LiveActor* pNearest = nullptr;
    f32 nearestDistance = FLT_MAX;
    s32 i = 0;
    bool found = false;
    for (; i < playerNum; ++i) {
        al::LiveActor* pPlayer = al::getPlayerActor(mActor, i);
        if (rc::isPlayerDeadOrBubble(pPlayer)) { continue; }
        if (mParams->_20 && rc::isPlayerEquipDisregard(pPlayer)) { continue; }
        TargetInfo info;
        if (!calcTargetInfo(&info, mActor, mFrontDir ? *mFrontDir : al::getFront(mActor),
                mSupportUpDir ? *mSupportUpDir : al::getGravity(mActor), pPlayer)) { continue; }
        const TargetFinderParam* pParam = mParams;
        const bool inCone = isInSearchRange(info, pParam, info.distance, pParam->_0);
        if (!inCone && !(mParams->_10 > 0.0f && info.distance < mParams->_10)) { continue; }
        if (info.distance < nearestDistance) {
            nearestDistance = info.distance;
            found = inCone;
            pNearest = pPlayer;
        }
    }
    _8 = pNearest;
    if (_8) { _12[0] = true; }
    if (found) {
        mIsFound = true;
        _11 = true;
    } else {
        mIsFound = false;
        _11 = false;
    }
    if (_8) {
        _28 = mParams->_C;
        _38 = _8;
    }
}

/** @brief Keeps a valid target or searches again when it leaves the allowed region. */
void TargetFinder::update() {
    _12[0] = false;
    if (!_8 || rc::isPlayerDeadOrBubble(_8) ||
        (mParams->_20 && rc::isPlayerEquipDisregard(_8))) {
        findTarget();
        return;
    }
    TargetInfo info;
    if (!calcTargetInfo(&info, mActor, mFrontDir ? *mFrontDir : al::getFront(mActor),
            al::getGravity(mActor), _8)) {
        findTarget();
        return;
    }
    const TargetFinderParam* pParam = mParams;
    if (isInSearchRange(info, pParam, info.distance, pParam->_0)) {
        mIsFound = true;
        _11 = true;
    } else if (isInSearchRange(info, mParams, info.distance, mParams->_1C)) {
        mIsFound = false;
        _11 = true;
    } else {
        findTarget();
    }
}

/** @brief Clears both target references and performs a new search. */
void TargetFinder::refindTarget() { _8 = nullptr; _38 = nullptr; findTarget(); }

/** @brief Overrides the host's search direction.
 * @param pFrontDir Search direction, or nullptr to use the host's front.
 */
void TargetFinder::setFrontDir(sead::Vector3f* pFrontDir) { mFrontDir = pFrontDir; }

/** @brief Overrides the search plane's perpendicular axis.
 * @param pSupportUpDir Axis, or nullptr to use host gravity.
 */
void TargetFinder::setSupportUpDir(sead::Vector3f* pSupportUpDir) { mSupportUpDir = pSupportUpDir; }

/** @brief Gets the selected target position.
 * @return Position of the current target.
 */
const sead::Vector3f& TargetFinder::getTargetPos() const { return al::getTrans(_8); }

/** @brief Gets the current or most recently selected target position.
 * @return Position of the current target, falling back to the previous target.
 */
const sead::Vector3f& TargetFinder::getLastTargetPos() const {
    if (_8) { return al::getTrans(_8); }
    return al::getTrans(_38);
}
