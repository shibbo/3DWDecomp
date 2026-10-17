#include "CourseSelect/CourseSelectRouteDokanEntranceGroup.hpp"
#include "CourseSelect/CourseSelectRouteDokanEntrance.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Block/BlockRailLink.hpp"
#include "Project/Block/BlockRailPartsGroup.hpp"

/** @brief Create an empty group of pipe-route entrances. */
CourseSelectRouteDokanEntranceGroup::CourseSelectRouteDokanEntranceGroup() = default;

/**
 * @brief Build entrances for every unconnected end of the route's rail links.
 * @param pParts Route parts whose open links need entrances.
 * @param rInfo Scene initialization data; each entrance uses its part's placement.
 */
void CourseSelectRouteDokanEntranceGroup::init(al::BlockRailPartsGroup* pParts,
                                            const al::ActorInitInfo& rInfo) {
    mEntranceCapacity = pParts->calcEmptyLinkCount();
    mEntranceCount = 0;
    if (mEntranceCapacity < 1)
        return;
    mpEntrances = new CourseSelectRouteDokanEntrance*[mEntranceCapacity];
    int partsNum = pParts->getPartsNum();
    for (int i = 0; i < partsNum; i++) {
        al::BlockRailParts* pPart = pParts->getParts(i);
        int linkNum = pPart->getLinkNum();
        for (int j = 0; j < linkNum; j++) {
            al::BlockRailLink* pLink = pPart->getLink(j);
            if (pLink->isTerminate())
                continue;
            if (pLink->getPrevLinkNum() == 0) {
                const al::ActorInitInfo* pInfo = pPart->getInitInfo();
                const char* pSuffix = pPart->mModelSuffix;
                sead::Vector3f pos = sead::Vector3f::zero;
                sead::Vector3f dir = sead::Vector3f::ez;
                pLink->calcPosAndDir(&pos, &dir, 0.0f);
                dir = -dir;
                auto* pEntrance = new CourseSelectRouteDokanEntrance("コース選択ルート土管出入口", pSuffix);
                sead::Quatf quat;
                al::makeQuatFrontNoSupport(&quat, dir);
                pEntrance->setInitQT(quat, pos);
                al::initCreateActorWithPlacementInfo(pEntrance, *pInfo);
                addEntrance(pPart, pEntrance);
            }
            if (pLink->getNextLinkNum() == 0) {
                const al::ActorInitInfo* pInfo = pPart->getInitInfo();
                const char* pSuffix = pPart->mModelSuffix;
                sead::Vector3f pos = sead::Vector3f::zero;
                sead::Vector3f dir = sead::Vector3f::ez;
                pLink->calcPosAndDir(&pos, &dir, 1.0f);
                auto* pEntrance = new CourseSelectRouteDokanEntrance("コース選択ルート土管出入口", pSuffix);
                sead::Quatf quat;
                al::makeQuatFrontNoSupport(&quat, dir);
                pEntrance->setInitQT(quat, pos);
                al::initCreateActorWithPlacementInfo(pEntrance, *pInfo);
                addEntrance(pPart, pEntrance);
            }
        }
    }
}

/**
 * @brief Register an entrance and connect it to its route part.
 * @param pParts Route part to connect.
 * @param pEntrance New entrance actor.
 */
void CourseSelectRouteDokanEntranceGroup::addEntrance(al::BlockRailParts* pParts,
                                                   CourseSelectRouteDokanEntrance* pEntrance) {
    mpEntrances[mEntranceCount] = pEntrance;
    mpEntrances[mEntranceCount]->getName();
    al::BlockRailParts::tryConnect(pParts, mpEntrances[mEntranceCount]);
    mEntranceCount++;
}

/**
 * @brief Create a stopper at each entrance.
 * @param rInfo Scene initialization data for the stopper actors.
 */
void CourseSelectRouteDokanEntranceGroup::createStopper(const al::ActorInitInfo& rInfo) {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->createStopper(rInfo);
}

/**
 * @brief Show every entrance stopper.
 * @param isAppear Whether to play the appearance reaction.
 */
void CourseSelectRouteDokanEntranceGroup::appearStopper(bool isAppear) {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->appearStopper(isAppear);
}

/** @brief Activate all entrance actors. */
void CourseSelectRouteDokanEntranceGroup::active() {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->makeActorAppeared();
}

/** @brief Deactivate all entrance actors. */
void CourseSelectRouteDokanEntranceGroup::deactive() {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->makeActorDead();
}

/** @brief Enable input at every entrance. */
void CourseSelectRouteDokanEntranceGroup::setEnableInput() {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->setEnableInput();
}

/** @brief Disable input at every entrance. */
void CourseSelectRouteDokanEntranceGroup::setDisableInput() {
    for (int i = 0; i < mEntranceCount; i++)
        mpEntrances[i]->setDisableInput();
}
