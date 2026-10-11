#include "MapObj/LuckyIslandHolder.hpp"
#include "MapObj/LuckyIsland.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/DemoUtil.hpp"

LuckyIslandHolder::LuckyIslandHolder() { mIslands.allocBuffer(25, nullptr); }
void LuckyIslandHolder::registerIsland(LuckyIsland* pIsland) {
    mIslands.pushBack(pIsland);
    if (!mCurrentIsland)
        mCurrentIsland = pIsland;
}
bool LuckyIslandHolder::canStartLuckyIslandDemo() const {
    if (mCurrentIsland && SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(mCurrentIsland)) > 7)
        return true;
    return false;
}
LuckyIsland* LuckyIslandHolder::startLuckyIslandDemo() {
    if (canStartLuckyIslandDemo()) {
        if (al::isDead(mCurrentIsland)) mCurrentIsland->DisasterAppear();
        mCurrentIsland->StartDemo();
        return mCurrentIsland;
    }
    return nullptr;
}
void LuckyIslandHolder::queueLuckyIslandDemo() {
    al::requestCaptureScreenCover(mCurrentIsland, 2);
    rc::requestEndDemoInGameCutscene(mCurrentIsland);
    rc::setImmediateSwitchFlag(mCurrentIsland);
    rc::requestStartDemoInGameCutscene(mCurrentIsland);
    rc::setDemoAudioType(mCurrentIsland, static_cast<alSeFunction::DemoType>(2));
    al::addDemoActor(startLuckyIslandDemo());
}
void LuckyIslandHolder::setIslandFlagActive() {
    for (int i = 0; i < mIslands.size(); ++i) {
        if (mIslands[i] == mCurrentIsland) {
            SingleModeDataFunction::setLuckyIslandPosCompleted(GameDataHolderWriter(mIslands[i]), i);
            return;
        }
    }
}
void LuckyIslandHolder::disableLuckyIslandCollision() {
    if (mCurrentIsland && al::isAlive(mCurrentIsland))
        mCurrentIsland->disableCollision();
}
