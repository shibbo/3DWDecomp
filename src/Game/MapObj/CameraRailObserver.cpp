#include "MapObj/CameraRailObserver.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Obj/CameraRailHolder.hpp"
#include "Library/Play/Camera/CameraRail.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"

CameraRailObserver::CameraRailObserver(const char* pName) : al::LiveActor(pName) {}
CameraRailObserver::~CameraRailObserver() {}

void CameraRailObserver::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::PlacementInfo links;
    if (!al::tryGetPlacementInfoByKey(&links, al::getPlacementInfo(rInfo), "Links")) {
        makeActorDead();
        return;
    }
    al::PlacementInfo rails;
    int count = 1;
    if (!al::tryGetPlacementInfoByKey(&rails, links, "Rail")) {
        if (!al::tryGetPlacementInfoByKey(&rails, links, "Rails")) {
            makeActorDead();
            return;
        }
        count = al::getCountPlacementInfo(rails);
    }
    mRailCount = count;
    bool valid = true;
    al::tryGetArg(&valid, rInfo, "IsValid");
    mRailHolder = al::getCameraRailHolder(this);
    mFirstRailIndex = mRailHolder->mCameraRailNum;
    al::PlacementInfo rail;
    mRails = new al::CameraRail*[mRailCount];
    for (int i = 0; i < mRailCount; ++i) {
        al::tryGetPlacementInfoByIndex(&rail, rails, i);
        mRails[i] = new al::CameraRail(rail);
        mRailHolder->setCameraRail(mRails[i], valid);
    }
    al::listenStageSwitchOnStart(this, al::Functor(this, &CameraRailObserver::start));
    al::listenStageSwitchOnStop(this, al::Functor(this, &CameraRailObserver::stop));
    if (mRailCount == mRailHolder->mCameraRailNum)
        makeActorAppeared();
    else
        makeActorDead();
}

void CameraRailObserver::start() {
    for (int i = 0; i < mRailCount; ++i)
        mRailHolder->setCameraRailValidFlag(i + mFirstRailIndex, true);
}

void CameraRailObserver::stop() {
    for (int i = 0; i < mRailCount; ++i)
        mRailHolder->setCameraRailValidFlag(i + mFirstRailIndex, false);
}

void CameraRailObserver::control() {
    int playerIndex = -1;
    int railIndex = -1;
    float closest;
    for (int i = 0; i < mRailHolder->mCameraRailNum; ++i) {
        if (mRailHolder->isCameraRailValid(i)) {
            int player = 0;
            float distance = mRailHolder->getCameraRail(i)->calcTopPlayer(this, &player);
            if (playerIndex == -1 || distance < closest) {
                closest = distance;
                railIndex = i;
                playerIndex = player;
            }
        }
    }
    al::setTopPlayerActorFromRail(this, al::getPlayerActor(this, playerIndex));
    mRailHolder->getCameraRail(railIndex)->setPlayerRailPos(this);
    mRailHolder->getCameraRail(railIndex)->setPlayerRailDir(al::getPlayerActor(this, playerIndex));
}
