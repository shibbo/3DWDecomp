#include "CourseSelect/CourseSelectObjKeeper.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * @brief Allocate object lists for the world map and each numbered world.
 * @param worldNum Number of numbered worlds.
 * @param maxObjectsPerWorld Capacity of each world's object list.
 */
CourseSelectObjKeeper::CourseSelectObjKeeper(int worldNum, int maxObjectsPerWorld) {
    if (worldNum < 0)
        return;
    mWorldObjects.tryAllocBuffer(worldNum + 1, nullptr);
    for (int i = 0; i < worldNum + 1; i++)
        mWorldObjects[i].allocBuffer(maxObjectsPerWorld, nullptr);
}

/**
 * @brief Register an actor unless it is part of the always-visible sea terrain.
 * @param pActor Actor to register.
 * @param worldId World whose visibility controls the actor.
 */
void CourseSelectObjKeeper::registerObject(al::LiveActor* pActor, int worldId) {
    if (al::isEqualString(pActor->getName(), "コース選択海底"))
        return;
    if (al::isEqualString(pActor->getName(), "コース選択地形") &&
        al::isEqualString(al::getModelName(pActor), "CourseSelectSea"))
        return;
    mWorldObjects[worldId].pushBack(pActor);
}

/** @brief Record all worlds as initially active after actors are placed. */
void CourseSelectObjKeeper::initAfterPlacement() {
    for (int i = 0; i < mWorldObjects.size(); i++)
        mActiveWorlds.setBit(i);
}

/**
 * @brief Apply changes in active worlds to their registered actors.
 * @param rActiveWorlds New world-visibility flags.
 */
void CourseSelectObjKeeper::updateActiveWorld(const sead::BitFlag<u32>& rActiveWorlds) {
    int worldNum = mWorldObjects.size();
    for (int i = 1; i < worldNum; i++) {
        if (rActiveWorlds.isOnBit(i)) {
            if (!mActiveWorlds.isOnBit(i))
                appearWorldObj(i);
        } else {
            if (mActiveWorlds.isOnBit(i))
                killWorldObj(i);
        }
    }
    mActiveWorlds = rActiveWorlds;
}

/**
 * @brief Show all objects registered to one world.
 * @param worldId World whose actors should appear.
 */
void CourseSelectObjKeeper::appearWorldObj(int worldId) {
    for (al::LiveActor& rActor : mWorldObjects[worldId])
        rActor.appear();
}

/**
 * @brief Hide all objects registered to one world.
 * @param worldId World whose actors should be removed.
 */
void CourseSelectObjKeeper::killWorldObj(int worldId) {
    for (al::LiveActor& rActor : mWorldObjects[worldId])
        rActor.kill();
}
