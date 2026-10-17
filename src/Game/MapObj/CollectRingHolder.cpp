#include "MapObj/CollectRingHolder.hpp"
#include "MapObj/CollectRing.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ItemUtil.hpp"
namespace {
    NERVE_DECL(CollectRingHolder, CountDown);
    NERVE_DECL(CollectRingHolder, End);
    NERVES_MAKE_NOSTRUCT(CollectRingHolder, CountDown, End)
}
CollectRingHolder::CollectRingHolder(const char* pName) : al::LiveActor(pName) {}
CollectRingHolder::~CollectRingHolder() {}
void CollectRingHolder::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvCollectRingHolderCountDown, 0);
    int count = al::calcLinkNestNum(rInfo, "Ring");
    if (count > 0) {
        mRings = new al::DeriveActorGroup<CollectRing>("リングリスト", count);
        al::PlacementInfo current = al::getPlacementInfo(rInfo);
        al::PlacementInfo next;
        for (int i = 0; i < count; ++i) {
            al::getLinksInfo(&next, current, "Ring");
            auto* ring = new CollectRing("リング集めリング");
            ring->setHost(this);
            al::ActorInitInfo info;
            info.initViewIdHost(&next, rInfo);
            al::initCreateActorWithPlacementInfo(ring, info);
            mRings->registerActor(ring);
            current = next;
        }
    }
    rc::initItemForRingItem(this, rInfo);
    al::trySyncStageSwitchAppear(this);
}
void CollectRingHolder::noticeGet(CollectRing* pRing) {
    ++mCollectedCount;
    if (mCollectedCount == mRings->mNumActors) {
        pRing->appearNumberComplete(mCollectedCount);
        appearItem(pRing);
        al::startSeSetSeqLoacalVariable(this, "SeSyRedCoinStarterComplete", 0, mCollectedCount);
        al::setNerve(this, &NrvCollectRingHolderEnd);
        kill();
    } else {
        pRing->appearNumber(mCollectedCount);
        al::startSeSetSeqLoacalVariable(this, "SeSyRedCoin", 0, mCollectedCount);
    }
}
void CollectRingHolder::appearItem(CollectRing* pRing) {
    sead::Vector3f direction = sead::Vector3f::ez;
    al::calcFrontDir(&direction, pRing);
    if (al::isParallelDirection(direction, sead::Vector3f::ey, 0.01f))
        al::calcUpDir(&direction, pRing);
    rc::appearItemForRingItem(this, al::getTrans(pRing), direction);
}
void CollectRingHolder::exeCountDown() {
    if (al::isFirstStep(this) && mRings) {
        mRings->appearAll();
        al::startSe(this, "Start");
    }
    al::holdSe(this, "TimerNormal");
    if (al::isGreaterEqualStep(this, 900)) {
        al::setNerve(this, &NrvCollectRingHolderEnd);
        if (mRings)
            mRings->killAll();
        al::startSe(this, "TimeUp");
        kill();
    }
}
void CollectRingHolder::exeEnd() {}
