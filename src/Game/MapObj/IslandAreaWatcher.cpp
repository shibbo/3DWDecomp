#include "MapObj/IslandAreaWatcher.hpp"
#include "AreaObj/IslandArea.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"

IslandAreaWatcher::IslandAreaWatcher(al::PlayerHolder* pPlayerHolder) {
    mAreas.setBuffer(99, mAreaBuffer);
    mCurrentIsland = -1;
    mPlayerHolder = pPlayerHolder;
}

void IslandAreaWatcher::registerIslandArea(IslandArea* pArea) {
    mAreas.pushBack(pArea);
}

IslandArea* IslandAreaWatcher::getActiveIsland() {
    al::LiveActor* player = al::findAlivePlayerActorFirst(mPlayerHolder);
    if (!player)
        return nullptr;
    const sead::Vector3f& trans = al::getTrans(player);
    for (int i = 0; i < mAreas.size(); ++i) {
        if (mAreas.unsafeAt(i)->isInVolume(trans)) {
            if (mCurrentIsland == i)
                return nullptr;
            mCurrentIsland = i;
            return mAreas.at(i);
        }
    }
    return nullptr;
}

int IslandAreaWatcher::getActiveIslandIndex() {
    al::LiveActor* player = al::findAlivePlayerActorFirst(mPlayerHolder);
    if (!player)
        return -1;
    const sead::Vector3f& trans = al::getTrans(player);
    for (int i = 0; i < mAreas.size(); ++i) {
        if (mAreas.unsafeAt(i)->isInVolume(trans)) {
            mCurrentIsland = mAreas.unsafeAt(i)->mZoneID - 1;
            return mCurrentIsland < 0 ? -1 : mCurrentIsland;
        }
    }
    return -1;
}

int IslandAreaWatcher::getCurrentIslandID() {
    if (mCurrentIsland < 0)
        return -1;
    return mAreas.unsafeAt(mCurrentIsland)->mZoneID;
}

bool IslandAreaWatcher::isEmpty() const {
    return mAreas.isEmpty();
}
