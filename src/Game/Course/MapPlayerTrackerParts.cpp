#include "Course/MapPlayerTrackerParts.hpp"

#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * @brief Creates a map trail with one stored position per tracker pane.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPaneName Parent pane for these layout parts.
 * @param pParent Parent layout actor.
 */
MapPlayerTrackerParts::MapPlayerTrackerParts(const al::LayoutInitInfo& rInfo,
                                           const char* pName, const char* pPaneName,
                                           al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    mLastKnownPos = sead::Vector2f::zero;
    al::initLayoutPartsActor(this, pParent, rInfo, pPaneName, nullptr);
    if (al::getPaneChildNum(this, "tracker") > 0) {
        mPositions.allocBuffer(al::getPaneChildNum(this, "tracker"), nullptr);
    }
}

/** @brief Positions stored trail points and fades the older points. */
void MapPlayerTrackerParts::setPanePositions() {
    for (s32 i = 0; i < mPositions.capacity(); ++i) {
        al::StringTmp<32> paneName("PicTracker_%02d", i);
        if (i < mPositions.size()) {
            al::setPaneLocalTrans(this, paneName.cstr(), mPositions[i]);
            f32 alpha = al::cosInterpolation(static_cast<f32>(i) / mPositions.capacity(),
                                             255.0f, 0.0f);
            al::setPaneLocalAlpha(this, paneName.cstr(), alpha);
            al::showPane(this, paneName.cstr());
        } else {
            al::hidePane(this, paneName.cstr());
        }
    }
}

/**
 * @brief Applies a scale to every trail pane.
 * @param rScale Pane scale.
 */
void MapPlayerTrackerParts::updateScale(const sead::Vector2f& rScale) {
    for (s32 i = 0; i < mPositions.capacity(); ++i) {
        al::StringTmp<32> paneName("PicTracker_%02d", i);
        al::setPaneLocalScale(this, paneName.cstr(), rScale);
    }
}

/** @brief Counts down the timer and returns true once it has expired. */
bool MapPlayerTrackerParts::updateTimer() {
    if (mTimer - 1 >= 0) {
        --mTimer;
        return false;
    }
    return true;
}

/**
 * @brief Adds a sufficiently distant point, discarding the oldest if full.
 * @param position New map position.
 */
void MapPlayerTrackerParts::addPlayerPos(sead::Vector2f position) {
    if (!mPositions.empty() && al::isNear(position, mPositions.front(), 4.0f)) {
        return;
    }
    mHasLastKnownPos = true;
    mLastKnownPos = position;
    // Keep overflow recovery together so both insertion paths share the same copy.
    auto insertPosition = [this, &position]() {
        if (!mPositions.pushBackwards(position)) {
            mPositions.popBack();
            mPositions.pushBackwards(position);
        }
    };
    insertPosition();
    mTimer = 100;
}

/**
 * @brief Copies the latest position and reports whether one has been recorded.
 * @param pOut Receives the last recorded position.
 * @return True when a position has been recorded.
 */
bool MapPlayerTrackerParts::getLastKnownPos(sead::Vector2f* pOut) {
    *pOut = mLastKnownPos;
    return mHasLastKnownPos;
}
