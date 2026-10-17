#include "MapObj/TestKitaharaRouteBazookaEntranceGroup.hpp"
#include "MapObj/TestKitaharaRouteBazookaEntrance.hpp"
#include "MapObj/TestKitaharaRouteBazookaPartsGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Project/Block/BlockRailLink.hpp"

TestKitaharaRouteBazookaEntranceGroup::TestKitaharaRouteBazookaEntranceGroup(const char* pModelName)
    : mModelName(pModelName) {}
void TestKitaharaRouteBazookaEntranceGroup::init(TestKitaharaRouteBazookaPartsGroup* pPartsGroup,
                                                const al::ActorInitInfo& rInfo) {
    mEntranceCount = 0;
    mEntranceCapacity = 0;
    int partCount = pPartsGroup->getPartCount();
    for (int i = 0; i < partCount; ++i) {
        al::BlockRailParts* parts = pPartsGroup->getParts(i);
        int linkCount = parts->getLinkNum();
        for (int j = 0; j < linkCount; ++j) {
            al::BlockRailLink* link = parts->getLink(j);
            if (link->isTerminate())
                continue;
            if (link->getPrevLinkNum() == 0)
                ++mEntranceCapacity;
            if (link->getNextLinkNum() == 0)
                ++mEntranceCapacity;
        }
    }
    if (mEntranceCapacity < 1)
        return;
    mEntrances = new TestKitaharaRouteBazookaEntrance*[mEntranceCapacity];
    for (int i = 0; i < partCount; ++i) {
        al::BlockRailParts* parts = pPartsGroup->getParts(i);
        int linkCount = parts->getLinkNum();
        for (int j = 0; j < linkCount; ++j) {
            al::BlockRailLink* link = parts->getLink(j);
            if (link->isTerminate())
                continue;
            if (link->getPrevLinkNum() == 0) {
                const char* modelName = mModelName;
                sead::Vector3f position(sead::Vector3f::zero);
                sead::Vector3f direction(sead::Vector3f::ez);
                link->calcPosAndDir(&position, &direction, 0.0f);
                direction = -direction;
                auto* entrance = new TestKitaharaRouteBazookaEntrance("ルート土管出入口", modelName);
                sead::Quatf rotation;
                al::makeQuatFrontNoSupport(&rotation, direction);
                entrance->setInitQT(rotation, position);
                al::initCreateActorNoPlacementInfo(entrance, rInfo);
                addEntrance(parts, entrance);
            }
            if (link->getNextLinkNum() == 0) {
                const char* modelName = mModelName;
                sead::Vector3f position(sead::Vector3f::zero);
                sead::Vector3f direction(sead::Vector3f::ez);
                link->calcPosAndDir(&position, &direction, 1.0f);
                auto* entrance = new TestKitaharaRouteBazookaEntrance("ルート土管出入口", modelName);
                sead::Quatf rotation;
                al::makeQuatFrontNoSupport(&rotation, direction);
                entrance->setInitQT(rotation, position);
                al::initCreateActorNoPlacementInfo(entrance, rInfo);
                addEntrance(parts, entrance);
            }
        }
    }
}
void TestKitaharaRouteBazookaEntranceGroup::addEntrance(al::BlockRailParts* pParts,
                                                       TestKitaharaRouteBazookaEntrance* pEntrance) {
    mEntrances[mEntranceCount] = pEntrance;
    al::BlockRailParts::tryConnect(pParts, mEntrances[mEntranceCount]);
    ++mEntranceCount;
}
void TestKitaharaRouteBazookaEntranceGroup::active() {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->makeActorAppeared();
}
void TestKitaharaRouteBazookaEntranceGroup::deactive() {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->makeActorDead();
}
TestKitaharaRouteBazookaEntrance* TestKitaharaRouteBazookaEntranceGroup::getEntrance(int index) const {
    return mEntrances[index];
}
void TestKitaharaRouteBazookaEntranceGroup::setHost(TestKitaharaRouteBazooka* pHost) {
    for (int i = 0; i < mEntranceCount; ++i)
        mEntrances[i]->setHost(pHost);
}
