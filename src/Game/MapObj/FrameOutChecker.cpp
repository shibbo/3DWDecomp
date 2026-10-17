#include "MapObj/FrameOutChecker.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
float absValue(float v) { return v > 0.0f ? v : -v; }
bool isOutsideFrame(const al::LiveActor* actor, const sead::Vector3f& trans, int direction,
                    const float& marginX, const float& marginY, const float& marginZ) {
    sead::Vector3f displayPos(0.0f, 0.0f, 0.0f);
    u32 width = al::getDisplayWidth();
    float widthMargin = marginX;
    u32 height = al::getDisplayHeight();
    float heightMargin = marginY;
    al::calcDisplayPosFromWorldPosWithClampOutRange(&displayPos, actor, trans, 100.0f, 0);
    if (direction == 4) return marginZ < displayPos.z;
    if (displayPos.z > 0.0f) {
        displayPos.x = -displayPos.x;
        displayPos.y = -displayPos.y;
    }
    if (static_cast<u32>(direction) <= 1) {
        if (widthMargin + width * 0.5f < absValue(displayPos.x) &&
            al::isSameSign(direction == 1 ? 1.0f : -1.0f, displayPos.x)) return true;
    } else {
        if (heightMargin + height * 0.5f < absValue(displayPos.y) &&
            al::isSameSign(direction == 3 ? 1.0f : -1.0f, displayPos.y)) return true;
    }
    return false;
}

}
FrameOutChecker::FrameOutChecker(const char* name) : al::LiveActor(name) {}
FrameOutChecker::~FrameOutChecker() {}
void FrameOutChecker::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    al::tryGetArg(&mScrollDirection, info, "DirScroll");
    switch (mScrollDirection) {
    case 0: mScrollDirection = 0; break;
    case 1: mScrollDirection = 1; break;
    case 2: mScrollDirection = 2; break;
    case 3: mScrollDirection = 3; break;
    case 4: mScrollDirection = 4; break;
    }
    int width = 0, height = 0, depth = 0;
    if (al::tryGetArg(&width, info, "MarginFrameOutWidth")) mWidthMargin = width;
    if (al::tryGetArg(&height, info, "MarginFrameOutHeight")) mHeightMargin = height;
    if (al::tryGetArg(&depth, info, "MarginFrameOutDepth")) mDepthMargin = depth;
    mFrameOutCounts.allocBuffer(al::getPlayerNumMax(this), nullptr);
    for (int i = 0; i < mFrameOutCounts.capacity(); ++i) mFrameOutCounts.emplaceBack(0);
    al::tryGetArg(&mIsInvalidBindKill, info, "IsInvalidBindKill");
    if (!al::trySyncStageSwitchAppearAndKill(this)) makeActorDead();
}
void FrameOutChecker::appear() {
    al::LiveActor::appear();
    rc::setDisableFrameOutBubbleForAllPlayer(this);
}
void FrameOutChecker::kill() {
    al::LiveActor::kill();
    rc::resetDisableFrameOutBubbleForAllPlayer(this);
}
void FrameOutChecker::control() {
    for (int i = 0; i < mFrameOutCounts.size(); ++i) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        if (!player || rc::isPlayerDeadOrBubble(player)) continue;
        if (mIsInvalidBindKill && rc::isPlayerBinded(player)) continue;
        const sead::Vector3f& trans = al::getTrans(player);
        bool isOutside = isOutsideFrame(this, trans, mScrollDirection, mWidthMargin, mHeightMargin, mDepthMargin);
        int* count = mFrameOutCounts.at(i);
        if (!isOutside) {
            *count = 0;
            continue;
        }
        ++*count;
        if (*mFrameOutCounts.unsafeAt(i) >= 6) {
            rc::forceKillPlayer(player);
            *mFrameOutCounts.unsafeAt(i) = 0;
        }
    }
}
