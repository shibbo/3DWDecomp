#include "MapObj/SwitchBlockWatcher.hpp"
#include "MapObj/SwitchBlock.hpp"
#include "MapObj/TrampleSwitch.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(SwitchBlockWatcher, Wait);
    NERVE_DECL(SwitchBlockWatcher, Move);
    NERVES_MAKE_NOSTRUCT(SwitchBlockWatcher, Wait, Move)
}

SwitchBlockWatcher::SwitchBlockWatcher(const char* pName) : al::LiveActor(pName) {
}

void SwitchBlockWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initNerve(this, &NrvSwitchBlockWatcherWait, 0);

    int blockCount = al::calcLinkChildNum(rInfo, "SwitchBlock");
    int mapPartsCount = al::calcLinkChildNum(rInfo, "SwitchBlockMapParts");
    int totalCount = blockCount + mapPartsCount;
    mBlocks = new al::DeriveActorGroup<SwitchBlock>("スイッチブロックホルダー", totalCount);
    for (int i = 0; i < blockCount; ++i) {
        auto* block = new SwitchBlock("スイッチブロック");
        al::initLinksActor(block, rInfo, "SwitchBlock", i);
        mBlocks->registerActor(block);
    }
    for (int i = 0; i < mapPartsCount; ++i) {
        auto* block = new SwitchBlock("スイッチブロック");
        al::initLinksActor(block, rInfo, "SwitchBlockMapParts", blockCount + i);
        mBlocks->registerActor(block);
    }

    int switchCount = al::calcLinkChildNum(rInfo, "SwitchBlockSwitch");
    do {
        if (switchCount < 1) {
            break;
        }
        mSwitches = new al::DeriveActorGroup<TrampleSwitch>("スイッチブロック用スイッチホルダー", switchCount);
        for (int i = 0; i < switchCount; ++i) {
            auto* sw = new TrampleSwitch("踏みスイッチ");
            sw->mIsReusable = true;
            al::initLinksActor(sw, rInfo, "SwitchBlockSwitch", i);
            mSwitches->registerActor(sw);
        }
        if (mSwitches != nullptr) {
            makeActorAppeared();
            return;
        }
    } while (false);
    makeActorDead();
}

void SwitchBlockWatcher::exeWait() {
    for (int i = 0; i < mSwitches->mNumActors; ++i) {
        if (mSwitches->getDeriveActor(i)->isTrigSwitchOn()) {
            al::setNerve(this, &NrvSwitchBlockWatcherMove);
            for (int j = 0; j < mSwitches->mNumActors; ++j) {
                if (i != j) {
                    mSwitches->getDeriveActor(j)->mIsHeldOn = false;
                }
            }
            return;
        }
        if (mSwitches->getDeriveActor(i)->isEarlyTrigOn()) {
            mMoveRequested = false;
        }
    }
    if (mMoveRequested) {
        al::setNerve(this, &NrvSwitchBlockWatcherMove);
    }
}

void SwitchBlockWatcher::exeMove() {
    if (al::isFirstStep(this)) {
        ++mMoveCount;
    }
    for (int i = 0; i < mBlocks->mNumActors; ++i) {
        mBlocks->getDeriveActor(i)->startMove();
    }
    mMoveRequested = false;
    al::setNerve(this, &NrvSwitchBlockWatcherWait);
}

bool SwitchBlockWatcher::isMoving() const {
    for (int i = 0; i < mSwitches->mNumActors; ++i) {
        if (mSwitches->getDeriveActor(i)->isEarlyTrigOn()) {
            return true;
        }
    }
    for (int i = 0; i < mBlocks->mNumActors; ++i) {
        if (mBlocks->getDeriveActor(i)->isMove()) {
            return true;
        }
    }
    return al::isNerve(this, &NrvSwitchBlockWatcherMove);
}

SwitchBlockWatcher::~SwitchBlockWatcher() {
}
