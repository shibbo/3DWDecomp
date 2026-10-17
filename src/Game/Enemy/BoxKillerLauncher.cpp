#include "Enemy/BoxKillerLauncher.hpp"
#include "MapObj/BoxKiller.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/BlockBrickBreakable.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"

namespace {
NERVE_DECL(BoxKillerLauncher, Shot)
NERVE_DECL(BoxKillerLauncher, DelayWait)
NERVE_DECL(BoxKillerLauncher, RotateSearchWait)
NERVE_DECL(BoxKillerLauncher, Wait)
NERVE_DECL(BoxKillerLauncher, RotateSearch)
NERVES_MAKE_NOSTRUCT(BoxKillerLauncher, Shot, DelayWait, RotateSearchWait, Wait, RotateSearch)
}
/** @brief Creates a cannon-box launcher. @param pName Actor name. */
BoxKillerLauncher::BoxKillerLauncher(const char* pName) : LiveActor(pName) {}
/** @brief Creates the cannon box and its supporting block stack. @param rInfo Placement information. */
void BoxKillerLauncher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 64);
    al::initNerve(this, &NrvBoxKillerLauncherShot, 0);
    al::initExecutorWatchObj(this, rInfo);
    al::tryGetArg(&mBulletFlyTime, rInfo, "BulletFlyTime");
    mBox = new BoxKiller("\u30ad\u30e9\u30fc\u30dc\u30c3\u30af\u30b9", mBulletFlyTime, 120);
    al::initCreateActorWithPlacementInfo(mBox, rInfo);
    s32 blockNum = 0;
    al::tryGetArg(&blockNum, rInfo, "BlockBrickNum");
    if (blockNum > 0) {
        const char* pType = "Breakable";
        al::tryGetStringArg(&pType, rInfo, "BlockBrickType");
        bool isEmpty = al::isEqualString(pType, "Empty");
        sead::Vector3f pos = al::getTrans(mBox);
        sead::Vector3f up(0.0f, 0.0f, 0.0f);
        al::calcUpDir(&up, mBox);
        mBlocks = new al::LiveActorGroup("\u30ad\u30e9\u30fc\u30dc\u30c3\u30af\u30b9\u7528\u8db3\u5143\u30d6\u30ed\u30c3\u30af\u30b0\u30eb\u30fc\u30d7", blockNum);
        for (s32 i = 0; i < blockNum; ++i) {
            al::LiveActor* pBlock;
            if (isEmpty) pBlock = new BlockEmpty("\u30ad\u30e9\u30fc\u30dc\u30c3\u30af\u30b9\u7528\u8db3\u5143\u7a7a\u30d6\u30ed\u30c3\u30af", "BlockEmpty");
            else pBlock = new BlockBrickBreakable("\u30ad\u30e9\u30fc\u30dc\u30c3\u30af\u30b9\u7528\u8db3\u5143\u30d6\u30ed\u30c3\u30af");
            al::initCreateActorNoPlacementInfo(pBlock, rInfo);
            al::copyPose(pBlock, this);
            al::resetPosition(pBlock, pos + (i * 100.0f) * up, false);
            mBlocks->registerActor(pBlock);
        }
        al::setTrans(mBox, pos + (blockNum * 100.0f) * up);
    }
    if (al::tryGetArg(&mDelayTime, rInfo, "DelayTime")) {
        if (mDelayTime < 0) mDelayTime = 0;
        al::setNerve(this, &NrvBoxKillerLauncherDelayWait);
    }
    al::tryGetArg(&mSearchType, rInfo, "SearchType");
    makeActorAppeared();
}
/** @brief Releases the box when its support disappears or it is picked up. */
void BoxKillerLauncher::control() {
    if (mBlocks) {
        bool allDead = true;
        for (s32 i = 0; i < mBlocks->getActorCount(); ++i) {
            if (al::isAlive(mBlocks->getActor(i))) { allDead = false; break; }
        }
        if (allDead) { mBox->shiftItemChange(); kill(); return; }
    }
    if (mBox->isCarrying()) kill();
}
/** @brief Waits for the next shot. */
void BoxKillerLauncher::exeWait() {
    if (al::isGreaterEqualStep(this, 120)) al::setNerve(this, &NrvBoxKillerLauncherShot);
}
/** @brief Applies the placement-defined delay before the first shot. */
void BoxKillerLauncher::exeDelayWait() {
    if (al::isGreaterEqualStep(this, mDelayTime + 120)) al::setNerve(this, &NrvBoxKillerLauncherShot);
}
/** @brief Fires and chooses whether to search for a player before firing again. */
void BoxKillerLauncher::exeShot() {
    if (al::isFirstStep(this)) { mBox->startShot(8.0f, mBulletFlyTime); return; }
    if (al::isGreaterEqualStep(this, 60)) {
        if (mSearchType) al::setNerve(this, &NrvBoxKillerLauncherRotateSearchWait);
        else al::setNerve(this, &NrvBoxKillerLauncherWait);
    }
}
/** @brief Waits before turning toward a player. */
void BoxKillerLauncher::exeRotateSearchWait() {
    if (al::isGreaterEqualStep(this, 120)) al::setNerve(this, &NrvBoxKillerLauncherRotateSearch);
}
/** @brief Chooses a turn angle using the configured aiming mode. */
void BoxKillerLauncher::exeRotateSearch() {
    if (al::isFirstStep(this)) {
        sead::Vector3f toPlayer = al::findNearestPlayerPos(mBox) - al::getTrans(mBox);
        toPlayer.y = 0.0f;
        sead::Vector3f front(0.0f, 0.0f, 0.0f);
        al::calcFrontDir(&front, mBox);
        f32 turn = 0.0f;
        switch (mSearchType) {
        case 1:
            if (!(al::calcAngleDegree(front, toPlayer) < 30.0f))
                turn = front.x * toPlayer.z - front.z * toPlayer.x > 0.0f ? -90.0f : 90.0f;
            break;
        case 2: {
            f32 angle = al::calcAngleDegree(front, toPlayer);
            if (!(angle < 15.0f)) {
                angle = angle < 60.0f ? 45.0f : 90.0f;
                turn = front.x * toPlayer.z - front.z * toPlayer.x > 0.0f ? -angle : angle;
            }
            break;
        }
        case 3: {
            f32 angle = al::calcAngleDegree(front, toPlayer);
            turn = front.x * toPlayer.z - front.z * toPlayer.x > 0.0f ? -angle : angle;
            break;
        }
        }
        if (al::isNearZero(turn, 0.001f)) {
            al::setNerve(this, &NrvBoxKillerLauncherShot);
            return;
        }
        mBox->tryShiftRotate(turn);
    }
    if (mBox->isWait()) al::setNerve(this, &NrvBoxKillerLauncherShot);
}
