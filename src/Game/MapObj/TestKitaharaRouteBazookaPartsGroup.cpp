#include "MapObj/TestKitaharaRouteBazookaPartsGroup.hpp"
#include "Library/ActorUtil.hpp"
#include "Project/Block/BlockRailParts.hpp"
#include "Project/Block/BlockRailLink.hpp"

TestKitaharaRouteBazookaPartsGroup::TestKitaharaRouteBazookaPartsGroup() = default;
void TestKitaharaRouteBazookaPartsGroup::init(const al::ActorInitInfo& rInfo) {
    mPartCount = al::calcLinkChildNum(rInfo, "Parts");
    mParts = new al::BlockRailParts*[mPartCount];
    for (int i = 0; i < mPartCount; ++i) {
        mParts[i] = new al::BlockRailParts("ルート土管部品");
        al::initLinksActor(mParts[i], rInfo, "Parts", i);
    }
    for (int i = 0; i < mPartCount; ++i) {
        for (int j = i + 1; j < mPartCount; ++j) {
            al::BlockRailParts* a = mParts[i];
            al::BlockRailParts* b = mParts[j];
            int aCount = a->getLinkNum();
            int bCount = b->getLinkNum();
            for (int aIndex = 0; aIndex < aCount; ++aIndex) {
                for (int bIndex = 0; bIndex < bCount; ++bIndex)
                    al::BlockRailLink::tryConnect(a->getLink(aIndex), b->getLink(bIndex), 50.0f);
            }
        }
    }
}
void TestKitaharaRouteBazookaPartsGroup::active() {
    for (int i = 0; i < mPartCount; ++i)
        mParts[i]->makeActorAppeared();
}
void TestKitaharaRouteBazookaPartsGroup::deactive() {
    for (int i = 0; i < mPartCount; ++i)
        mParts[i]->makeActorDead();
}
al::BlockRailParts* TestKitaharaRouteBazookaPartsGroup::getParts(int index) const {
    return mParts[index];
}
