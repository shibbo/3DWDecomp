#include "MapObj/LuckyIslandController.hpp"
#include "MapObj/LuckyIsland.hpp"
#include "MapObj/LuckyIslandHolder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include <cfloat>
LuckyIslandController::LuckyIslandController(const char* name) : al::LiveActor(name) {}
LuckyIslandController::~LuckyIslandController() {}
void LuckyIslandController::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    int count = al::calcLinkChildNum(info, "DemoCamera");
    if (count > 0) mCameraPositions.allocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i) {
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, info, "DemoCamera", i);
        mCameraPositions.emplaceBack();
        al::getTrans(mCameraPositions[i], placement);
    }
}
void LuckyIslandController::initAfterPlacement() {
    mHolder = static_cast<LuckyIslandHolder*>(al::getSceneObj(this, 38));
    if (auto* controller = DisasterModeController::tryGetController(this)) controller->registerStateListener(this);
    auto* holder = mHolder;
    for (int i = 0; i < holder->getIslandCount(); ++i) holder->getIsland(i)->setController(this);
    makeActorDead();
}
void LuckyIslandController::onDisasterModeStateChange(DisasterModeController::State state) {
    if (SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(this)) < 8) return;
    switch (static_cast<int>(state)) {
    case 1:
    case 3: {
        auto* holder = mHolder;
        for (int i = 0; i < holder->getIslandCount(); ++i) holder->getIsland(i)->DisasterDisappear();
        break;
    }
    case 5:
        mSelectIsland = true;
        setActiveIsland();
        break;
    case 9:
        if (!mSelectIsland) break;
        [[fallthrough]];
    case 7:
        mSelectIsland = false;
        if (!mHolder->getCurrentIsland() || al::isDead(mHolder->getCurrentIsland())) setActiveIsland();
        if (mHolder->getCurrentIsland() && al::isDead(mHolder->getCurrentIsland())) mHolder->getCurrentIsland()->DisasterAppear();
        break;
    }
}
void LuckyIslandController::setActiveIsland() {
    auto* holder = mHolder;
    sead::Vector3f player = al::getPlayerPos(al::findNearestPlayerActor(this), 0);
    int remaining = SingleModeDataFunction::getNumRemainingLuckyShines(GameDataHolderAccessor(this));
    LuckyIsland* closest = nullptr;
    float distance = FLT_MAX;
    if (holder->getIslandCount() <= 0) return;
    if (remaining != 0) {
        for (int i = 0; i < holder->getIslandCount(); ++i) {
            if (SingleModeDataFunction::wasLuckyIslandPosCompleted(GameDataHolderAccessor(this), i)) continue;
            sead::Vector3f islandPos = al::getTrans(holder->getIsland(i));
            sead::Vector3f offset = islandPos - player;
            float d = offset.squaredLength();
            if (d < distance) { closest = holder->getIsland(i); distance = d; }
        }
    } else {
        for (int i = 0; i < holder->getIslandCount(); ++i) {
            sead::Vector3f islandPos = al::getTrans(holder->getIsland(i));
            sead::Vector3f offset = islandPos - player;
            float d = offset.squaredLength();
            if (d < distance) { closest = holder->getIsland(i); distance = d; }
        }
    }
    if (closest) mHolder->setCurrentIsland(closest);
}
const sead::Vector3f* LuckyIslandController::getClosestCamera(const sead::Vector3f& position) {
    // The original requires at least one camera position within a finite distance.
    const sead::Vector3f* closest;
    float distance = FLT_MAX;
    for (int i = 0; i < mCameraPositions.size(); ++i) {
        float d = (position - *mCameraPositions.unsafeAt(i)).squaredLength();
        if (d < distance) { distance = d; closest = mCameraPositions[i]; }
    }
    return closest;
}
