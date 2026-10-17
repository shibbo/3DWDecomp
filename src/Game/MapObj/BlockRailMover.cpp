#include "MapObj/BlockRailMover.hpp"
#include "MapObj/BlockQuestion.hpp"
#include "MapObj/BlockBrick.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/BlockBrickBreakable.hpp"
#include "MapObj/BlockPow.hpp"
#include "MapObj/BlockTransparent.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
    NERVE_DECL(BlockRailMover, Stop);
    NERVE_DECL(BlockRailMover, StandBy);
    NERVE_DECL(BlockRailMover, Move);
    NERVE_DECL(BlockRailMover, Wait);
    NERVES_MAKE_STRUCT(BlockRailMover, Stop, StandBy, Move, Wait)
}
BlockRailMover::BlockRailMover(const char* name) : al::LiveActor(name) {}
BlockRailMover::~BlockRailMover() = default;
void BlockRailMover::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, info);
    al::initActorClipping(this, info);
    al::initGroupClipping(this, info, 64);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvBlockRailMover.Stop, 0);
    al::tryGetArg(&mFixedDirection, info, "IsFixedDirection");
    al::tryGetArg(&mMoveTime, info, "BlockSpeedByTime");
    al::tryGetArg(&mWaitTime, info, "WaitTime");
    al::tryGetArg(&mDelayTime, info, "DelayTime");
    al::tryGetArg(&mSpeed, info, "BlockSpeed");
    al::tryGetArg(&mMoveType, info, "RailBlockMoveType");
    if (!al::isExistRail(info)) { makeActorDead(); return; }
    initRailKeeper(info);
    al::setRailPosToStart(this);
    if (al::getRailPointNum(this) == 2) al::isLoopRail(this);
    mBlockCount = al::calcLinkChildNum(info, "BlockRailMove");
    if (mBlockCount == 0) { makeActorDead(); return; }
    mBlocks = new Block*[mBlockCount];
    for (int i = 0; i < mBlockCount; ++i) {
        const char* type = al::getLinksActorClassName(info, "BlockRailMove", i);
        al::LiveActor* actor;
        bool isLong = false;
        if (al::isEqualString(type, "BlockQuestion")) {
            auto* block = new BlockQuestion("ハテナブロック");
            al::initLinksActor(block, info, "BlockRailMove", i);
            block->onConnectRailBlock();
            isLong = block->isLong();
            actor = block;
        } else if (al::isEqualString(type, "BlockBrick")) {
            auto* block = new BlockBrick("レンガブロック");
            al::initLinksActor(block, info, "BlockRailMove", i);
            block->onConnectRailBlock();
            actor = block;
        } else if (al::isEqualString(type, "BlockEmpty")) {
            auto* block = new BlockEmpty("空ブロック", "BlockEmpty");
            al::initLinksActor(block, info, "BlockRailMove", i);
            block->onConnectRailBlock();
            actor = block;
        } else if (al::isEqualString(type, "BlockBrickBreakable")) {
            auto* block = new BlockBrickBreakable("壊れブロック");
            al::initLinksActor(block, info, "BlockRailMove", i);
            al::setShadowFixed(block, false);
            actor = block;
        } else if (al::isEqualString(type, "BlockPow")) {
            auto* block = new BlockPow("POWブロック");
            al::initLinksActor(block, info, "BlockRailMove", i);
            block->onConnectRailBlock();
            actor = block;
        } else if (al::isEqualString(type, "BlockTransparent")) {
            auto* block = new BlockTransparent("透明ブロック");
            block->onConnectRailBlock();
            al::initLinksActor(block, info, "BlockRailMove", i);
            isLong = block->isLong();
            actor = block;
        } else continue;
        auto* block = new Block;
        block->actor = actor;
        block->coord = al::calcNearestRailCoord(getRailKeeper(), al::getTrans(actor));
        block->isLong = (al::isEqualString(type, "BlockTransparent") || al::isEqualString(type, "BlockQuestion")) && isLong;
        mBlocks[i] = block;
        for (int j = i; j > 0; --j) {
            auto* current = mBlocks[j];
            auto* previous = mBlocks[j - 1];
            if (!(current->coord < previous->coord)) break;
            mBlocks[j] = previous;
            mBlocks[j - 1] = current;
        }
    }
    if (mBlockCount > 0) {
        for (unsigned i = 0; i < unsigned(mBlockCount); ++i) {
            bool isShort = !mBlocks[i]->isLong;
            float width;
            if (i == 0 || i == mBlockCount - 1) width = isShort ? 50.0f : 150.0f;
            else width = isShort ? 100.0f : 300.0f;
            mBlockLength = mBlockLength + width;
        }
    }
    al::setSyncRailToCoord(this, mBlocks[0]->coord);
    mBlockLength = mBlockCount == 1 ? 0.0f : mBlockLength;
    float travel = al::getRailTotalLength(this) - mBlockLength;
    if (travel <= 0.0f) return;
    if (mMoveTime > 0) mSpeed = travel / float(mMoveTime);
    if (!al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &BlockRailMover::start))) start();
    sead::Vector3f center(0.0f, 0.0f, 0.0f);
    calcBlockClippingCenter(&center);
    float extent = (center - al::getRailPos(this)).length();
    float radius = 0.0f;
    al::calcRailClippingInfo(&mClippingCenter, &radius, this, 100.0f, extent);
    al::setClippingInfo(this, radius, &mClippingCenter);
    for (int i = 0; i < mBlockCount; ++i) al::setClippingInfo(mBlocks[i]->actor, radius, &mClippingCenter);
    makeActorAppeared();
    accompanyToRail();
}
void BlockRailMover::start() {
    if (al::isNerve(this, &NrvBlockRailMover.Stop)) {
        if (mDelayTime > 0) al::setNerve(this, &NrvBlockRailMover.StandBy);
        else al::setNerve(this, &NrvBlockRailMover.Move);
    }
}
void BlockRailMover::calcBlockClippingCenter(sead::Vector3f* center) {
    float shadow = al::getShadowDropLengthMax(mBlocks[0]->actor);
    float extent = mBlockLength > shadow ? mBlockLength : shadow;
    al::calcRailPosAtCoord(center, this, mBlockLength * 0.5f);
    center->y -= extent * 0.5f;
}
void BlockRailMover::accompanyToRail() {
    float coord = al::getRailCoord(this);
    float offset = 0.0f;
    for (int i = 0; i < mBlockCount; ++i) {
        auto* actor = mBlocks[i]->actor;
        al::setSyncRailToCoord(this, coord + offset);
        if (i != mBlockCount - 1) {
            bool longCurrent = mBlocks[i]->isLong;
            bool longNext = mBlocks[i + 1]->isLong;
            if (!longCurrent) {
                if (longNext) offset += 200.0f;
                else offset += 100.0f;
            } else {
                if (longNext) offset += 300.0f;
                else offset += 200.0f;
            }
        }
        al::setTrans(actor, al::getTrans(this));
        if (!al::isNearDirection(al::getRailDir(this), al::getGravity(actor), 0.01f)) {
            const sead::Vector3f& direction = al::getRailDir(this);
            if (!mFixedDirection && direction.y == 0.0f) {
                al::setFront(actor, direction);
                sead::Vector3f side;
                al::calcSideDir(&side, actor);
                al::setFront(actor, side);
            }
        }
    }
    al::setSyncRailToCoord(this, coord);
}
void BlockRailMover::exeStandBy() { if (al::isGreaterEqualStep(this, mDelayTime)) al::setNerve(this, &NrvBlockRailMover.Move); }
void BlockRailMover::exeWait() { if (al::isGreaterEqualStep(this, mWaitTime)) al::setNerve(this, &NrvBlockRailMover.Move); }
void BlockRailMover::exeStop() {}
void BlockRailMover::exeMove() {
    al::moveSyncRail(this, mSpeed);
    if (al::isRailReachedNearGoal(this, 0.0f, mBlockLength)) {
        if (mMoveType != 0) { al::setNerve(this, &NrvBlockRailMover.Stop); return; }
        al::reverseRail(this);
        if (mWaitTime > 0) { al::setNerve(this, &NrvBlockRailMover.Wait); return; }
    }
    accompanyToRail();
}
