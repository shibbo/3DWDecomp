#include "MapObj/RouteDokanEntranceGroup.hpp"
#include "MapObj/RouteDokanEntrance.hpp"
#include "Project/Block/BlockRailPartsGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Block/BlockRailLink.hpp"

namespace {
RouteDokanEntrance* createEntrance(const al::ActorInitInfo& rInfo, const char* pName,
                                   const char* pModelName, const char* pModelSuffix,
                                   al::BlockRailLink* pLink, bool isPrevious) {
    sead::Vector3f position(sead::Vector3f::zero);
    sead::Vector3f direction(sead::Vector3f::ez);
    pLink->calcPosAndDir(&position, &direction, isPrevious ? 0.0f : 1.0f);
    if (isPrevious)
        direction = -direction;
    auto* entrance = new RouteDokanEntrance(pName, pModelName, pModelSuffix);
    sead::Quatf rotation;
    al::makeQuatFrontNoSupport(&rotation, direction);
    entrance->setInitQT(rotation, position);
    al::initCreateActorWithPlacementInfo(entrance, rInfo);
    return entrance;
}
}
RouteDokanEntranceGroup::RouteDokanEntranceGroup(const char* pEntranceName, const char* pModelName)
    : mEntranceName(pEntranceName), mModelName(pModelName) {}
void RouteDokanEntranceGroup::init(al::BlockRailPartsGroup* pPartsGroup,
                                   const al::ActorInitInfo& rInfo, bool keepEntrancesVisible) {
    mEntranceCapacity = pPartsGroup->calcEmptyLinkCount();
    mEntranceCount = 0;
    if (mEntranceCapacity < 1)
        return;
    bool hideModel = false;
    al::tryGetArg(&hideModel, rInfo, "IsHideModel");
    mEntrances = new RouteDokanEntrance*[mEntranceCapacity];
    int partCount = pPartsGroup->getPartsNum();
    for (int i = 0; i < partCount; ++i) {
        al::BlockRailParts* parts = pPartsGroup->getParts(i);
        int linkCount = parts->getLinkNum();
        for (int j = 0; j < linkCount; ++j) {
            al::BlockRailLink* link = parts->getLink(j);
            if (link->isTerminate())
                continue;
            if (link->getPrevLinkNum() == 0) {
                auto* entrance = createEntrance(*parts->getInitInfo(), mEntranceName, mModelName,
                                               parts->mModelSuffix, link, true);
                addEntrance(parts, entrance);
            }
            if (link->getNextLinkNum() == 0) {
                auto* entrance = createEntrance(*parts->getInitInfo(), mEntranceName, mModelName,
                                               parts->mModelSuffix, link, false);
                addEntrance(parts, entrance);
            }
        }
        if (hideModel)
            parts->setIsHideModel(true);
    }
    if (hideModel && !keepEntrancesVisible) {
        for (int i = 0; i < mEntranceCount; ++i)
            mEntrances[i]->setIsHideModel(true);
    }
}
void RouteDokanEntranceGroup::addEntrance(al::BlockRailParts* pParts,
                                                       RouteDokanEntrance* pEntrance) {
    mEntrances[mEntranceCount] = pEntrance;
    al::BlockRailParts::tryConnect(pParts, mEntrances[mEntranceCount]);
    ++mEntranceCount;
}
void RouteDokanEntranceGroup::active() {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->makeActorAppeared();
}
void RouteDokanEntranceGroup::deactive() {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->makeActorDead();
}
RouteDokanEntrance* RouteDokanEntranceGroup::getEntrance(int index) const {
    return mEntrances[index];
}
void RouteDokanEntranceGroup::setHost(IUseRouteDokan* pHost) {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->setHost(pHost);
}

void RouteDokanEntranceGroup::calcOffset(const sead::Vector3f& rBaseTrans) {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->calcOffset(rBaseTrans);
}
void RouteDokanEntranceGroup::updateLinkedTrans(const sead::Vector3f& rBaseTrans) {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->updateLinkedTrans(rBaseTrans);
}
