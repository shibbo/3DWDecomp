#include "MapObj/ShardsWatcher.hpp"
#include "MapObj/Shards.hpp"
#include "MapObj/ShardsWatcherHolder.hpp"
#include "MapObj/GoalItem.hpp"
#include "MapObj/SinkedItem.hpp"
#include "MapObj/IslandKeeper.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/DemoUtil.hpp"
namespace {
    NERVE_DECL(ShardsWatcher, Watch);
    NERVE_DECL(ShardsWatcher, WatchLast);
    NERVES_MAKE_NOSTRUCT(ShardsWatcher, Watch, WatchLast)
}
bool ShardsWatcher::sIsFinalShardGetPending;
ShardsWatcher::ShardsWatcher(const char* name) : al::LiveActor(name) {}
ShardsWatcher::~ShardsWatcher() = default;
void ShardsWatcher::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initStageSwitch(this, info);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvShardsWatcherWatch, 0);
    ProjectActorFactory factory;
    if (al::calcLinkChildNum(info, "GoalItem"))
        mGoalItem = static_cast<GoalItem*>(al::createLinksActorFromFactory(factory, info, "GoalItem", 0));
    bool complete = false;
    if (mGoalItem) {
        mId = mGoalItem->getIslandId();
        mScenarioId = mGoalItem->getShineId();
        mGoalItem->setFromShards();
        complete = SingleModeDataFunction::isScenarioComplete(GameDataHolderAccessor(this), mId - 1, mScenarioId - 1);
    }
    if (auto* holder = al::getSceneObj<ShardsWatcherHolder>(this, 47)) holder->registerShardWatcher(this);
    if (complete) {
        al::LiveActor::kill();
        al::tryOnSwitchDeadOn(this);
        return;
    }
    mGoalItem->makeActorDead();
    int shardCount = al::calcLinkChildNum(info, "ShardPiece");
    int buriedCount = al::calcLinkChildNum(info, "BuriedShardPiece");
    int enemyCount = al::calcLinkChildNum(info, "EnemyShardPiece");
    mPieceCount = shardCount + buriedCount + enemyCount;
    sead::Vector3f origin;
    al::getTrans(&origin, info.getPlacementInfo());
    mPieces = new Piece[mPieceCount];
    int index = 0;
    for (int i = 0; i < shardCount; ++i, ++index) {
        auto* shard = static_cast<Shards*>(al::createLinksActorFromFactory(factory, info, "ShardPiece", i));
        mPieces[index].actor = shard;
        mPieces[index].offset = al::getTrans(shard) - origin;
    }
    for (int i = 0; i < buriedCount; ++i, ++index) {
        auto* item = static_cast<SinkedItem*>(al::createLinksActorFromFactory(factory, info, "BuriedShardPiece", i));
        auto* shard = item->getShard();
        mPieces[index].actor = shard;
        mPieces[index].offset = al::getTrans(shard) - origin;
    }
    for (int i = 0; i < enemyCount; ++i, ++index) {
        auto* actor = al::createLinksActorFromFactory(factory, info, "EnemyShardPiece", i);
        auto* shard = static_cast<Shards*>(actor->getLinkedActor());
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, info, "EnemyShardPiece", i);
        shard->setFromWatcher();
        if (shard->getIslandId() < 0) {
            shard->setIslandId(mId);
            shard->setShardId(placement);
            shard->sharedInit(info);
        }
        mPieces[index].actor = shard;
        mPieces[index].offset = al::getTrans(shard) - origin;
    }
    al::tryGetArg(&mDelay, info, "SwitchOnDelayStep");
    for (int i = 0; i < mPieceCount; ++i) mPieces[i].actor->setWatcher(this);
    if (al::trySyncStageSwitchAppear(this)) {
        for (int i = 0; i < mPieceCount; ++i) mPieces[i].actor->makeActorDead();
    }
    sIsFinalShardGetPending = false;
}
void ShardsWatcher::killObject(bool skipGoal) {
    al::LiveActor::kill();
    if (!skipGoal) {
        sead::Vector3f pos = al::getTrans(mGoalItem);
        if (auto* player = al::tryFindAlivePlayerActorFirst(mGoalItem)) pos = al::getTrans(player);
        pos.y += 50.0f;
        al::setTrans(mGoalItem, pos);
        rc::addDemoActor(mGoalItem);
        mGoalItem->setCollectListener(this);
        mGoalItem->appearCollect(true);
        mGoalItem->setFront(mGoalFront, mUseFrontAngle, mFrontAngle);
        if (mCollectedBySensor) mGoalItem->setCollectedBySensor();
        if (mPieces && mPieces[mLastPiece].actor) mGoalItem->disableCollectionFlag();
    }
    al::tryOnSwitchDeadOn(this);
}
void ShardsWatcher::control() {
    mPlayerInIsland = false;
    if (auto* keeper = al::getSceneObj<IslandKeeper>(this, 44)) {
        if (keeper->getActiveIslandIndex() + 1 == mId) mPlayerInIsland = true;
    } else if (mAreaGroup && al::tryIsInAreaObjPlayer(mAreaGroup)) mPlayerInIsland = true;
}
void ShardsWatcher::appear() {
    al::LiveActor::appear();
    for (int i = 0; i < mPieceCount; ++i) mPieces[i].actor->appear();
}
void ShardsWatcher::hide() {
    al::LiveActor::appear();
    for (int i = 0; i < mPieceCount; ++i) mPieces[i].actor->hideActor();
}
void ShardsWatcher::kill() { killObject(false); }
void ShardsWatcher::updateLinkedTrans(const sead::Vector3f& pos) {
    for (int i = 0; i < mPieceCount; ++i) mPieces[i].actor->updateLinkedTrans(pos + mPieces[i].offset);
}
void ShardsWatcher::goalItemCollectCallback() {
    if (mShardId >= 0) {
        SingleModeDataFunction::collectShard(GameDataHolderWriter(GameDataHolderAccessor(this)), mId - 1, mShardId - 1);
        mShardId = -1;
    }
}
void ShardsWatcher::exeWatch() {
    bool pending = false;
    for (int i = 0; i < mPieceCount; ++i) {
        if (al::isAlive(mPieces[i].actor) || al::isDeadAlive(mPieces[i].actor)) {
            auto* shard = mPieces[i].actor;
            if (shard) {
                pending |= shard->isGoalPending();
                if (shard->isCollectionFinished() || shard->isCollected()) continue;
            }
            mLastPiece = i;
            return;
        }
    }
    if (pending) al::setNerve(this, &NrvShardsWatcherWatchLast);
}
void ShardsWatcher::setGoalitemPos() {
    if (mCollectedBySensor && mCollectSensor && al::getSensorHost(mCollectSensor)) {
        al::setTrans(mGoalItem, al::getTrans(al::getSensorHost(mCollectSensor)));
        return;
    }
    mGoalPosition = al::findNearestPlayerPos(mPieces[mLastPiece].actor);
    mGoalPosition.y += 50.0f;
    al::setTrans(mGoalItem, mGoalPosition);
}
void ShardsWatcher::exeWatchLast() {
    al::isFirstStep(this);
    for (int i = 0; i < mPieceCount; ++i) {
        if (al::isAlive(mPieces[i].actor) || al::isDeadAlive(mPieces[i].actor)) {
            auto* shard = mPieces[i].actor;
            if (shard && shard->isCollecting()) {
                mCollectedBySensor = shard->isCollectedBySensor();
                if (mCollectedBySensor) mCollectSensor = shard->getCollectSensor();
                mLastPiece = i;
                mGoalFront = shard->getGoalFront();
                mUseFrontAngle = shard->isUseFrontAngle();
                if (mUseFrontAngle) mFrontAngle = shard->getFrontAngle();
                return;
            }
        }
    }
    setGoalitemPos();
    kill();
}
void ShardsWatcher::exeWait() { if (al::isGreaterEqualStep(this, mDelay)) kill(); }
bool ShardsWatcher::isLastShard() const {
    for (int i = 0; i < mPieceCount; ++i) {
        if (mId > 0 && !SingleModeDataFunction::isShardCollected(GameDataHolderAccessor(this), mId - 1, i)) return false;
    }
    return true;
}
bool ShardsWatcher::isFinalShard() const {
    int count = 0;
    for (int i = 0; i < mPieceCount; ++i) {
        if (mId > 0) count += SingleModeDataFunction::isShardCollected(GameDataHolderAccessor(this), mId - 1, i);
    }
    return count == mPieceCount - 1;
}
