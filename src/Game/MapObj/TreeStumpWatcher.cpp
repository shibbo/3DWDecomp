#include "MapObj/TreeStumpWatcher.hpp"
#include "MapObj/TreeStump.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"

namespace {
    NERVE_DECL(TreeStumpWatcher, Watch);
    NERVES_MAKE_NOSTRUCT(TreeStumpWatcher, Watch)
}

TreeStumpWatcher::TreeStumpWatcher(const char* pName) : al::LiveActor(pName) {
}

void TreeStumpWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    bool isAppearSwitch = al::trySyncStageSwitchAppear(this);
    al::initNerve(this, &NrvTreeStumpWatcherWatch, 0);
    int count = al::calcLinkChildNum(rInfo, "TreeStump");
    if (count > 0) {
        mStumps.allocBuffer(count, nullptr, 8);
        for (int i = 0; i < count; ++i) {
            auto* stump = new TreeStump("TreeStump");
            al::initLinksActor(stump, rInfo, "TreeStump", i);
            stump->mWatcher = this;
            if (isAppearSwitch) {
                stump->makeActorDead();
            }
            mStumps.pushBack(stump);
        }
    }
}

void TreeStumpWatcher::appear() {
    al::LiveActor::appear();
    for (int i = 0; i < mStumps.size(); ++i) {
        mStumps[i]->appear();
    }
    al::setNerve(this, &NrvTreeStumpWatcherWatch);
}

void TreeStumpWatcher::kill() {
    al::LiveActor::kill();
}

void TreeStumpWatcher::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
}

bool TreeStumpWatcher::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    return false;
}

bool TreeStumpWatcher::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer, al::ScreenPointTarget* pTarget) {
    return false;
}

void TreeStumpWatcher::exeWatch() {
    if (mStompedCount == mStumps.size() && !mIsSwitchOn) {
        mIsSwitchOn = al::tryOnStageSwitch(this, "SwitchAllStompedOn");
    }
}

void TreeStumpWatcher::addStomped() {
    ++mStompedCount;
}

TreeStumpWatcher::~TreeStumpWatcher() {
}
