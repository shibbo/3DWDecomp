#include "MapObj/TestKinokoHead.hpp"
#include "MapObj/TestKinokoKuribo.hpp"
#include "MapObj/BindPuppeteer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
namespace {
    NERVE_DECL(TestKinokoHead, Item);
    NERVE_DECL(TestKinokoHead, Worn);
    NERVE_DECL(TestKinokoHead, WornAnim);
    NERVES_MAKE_NOSTRUCT(TestKinokoHead, Item, Worn, WornAnim)
}
TestKinokoHead::TestKinokoHead(const char* name) : al::LiveActor(name), mPuppeteer(new BindPuppeteer(name)) {}
TestKinokoHead::~TestKinokoHead() {}
void TestKinokoHead::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "TestKinokoHead", nullptr);
    al::initNerve(this, &NrvTestKinokoHeadItem, 0);
    int count = al::calcLinkChildNum(info, "GenerateEnemy");
    mKuribos.allocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i) {
        al::PlacementInfo placement;
        al::getLinksInfoByIndex(&placement, al::getPlacementInfo(info), "GenerateEnemy", i);
        auto* kuribo = new TestKinokoKuribo("キノコ好きなクリボー");
        al::initCreateActorWithPlacementInfo(kuribo, info, placement);
        mKuribos.pushBack(kuribo);
    }
    makeActorAppeared();
}
void TestKinokoHead::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::setNerve(this, &NrvTestKinokoHeadItem);
}
void TestKinokoHead::makeActorDead() {
    al::LiveActor::makeActorDead();
    al::hideModelIfShow(this);
    al::hideModelIfShow(al::getSubActor(this, "体の着ぐるみ"));
    al::hideModelIfShow(al::getSubActor(this, "ゲットアイテム"));
}
void TestKinokoHead::exeItem() {
    if (al::isFirstStep(this)) {
        al::hideModelIfShow(this);
        al::hideModelIfShow(al::getSubActor(this, "体の着ぐるみ"));
        al::showModelIfHide(al::getSubActor(this, "ゲットアイテム"));
        mHeadMtx = nullptr;
        mBodyMtx = nullptr;
    }
    for (int i = 0; i < mKuribos.size(); ++i) mKuribos.unsafeAt(i)->setHeadWorn(false);
}
void TestKinokoHead::exeWornAnim() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::showModelIfHide(al::getSubActor(this, "体の着ぐるみ"));
        al::hideModelIfShow(al::getSubActor(this, "ゲットアイテム"));
    }
    al::updatePoseMtx(this, mHeadMtx);
    al::updatePoseMtx(al::getSubActor(this, "体の着ぐるみ"), mBodyMtx);
    if (rc::isPuppetActionEnd(mPuppeteer->getPlayerPuppet())) {
        if (mPuppeteer->isBind()) mPuppeteer->endBind(nullptr);
        al::setNerve(this, &NrvTestKinokoHeadWorn);
        return;
    }
    for (int i = 0; i < mKuribos.size(); ++i) mKuribos.unsafeAt(i)->setHeadWorn(true);
}
void TestKinokoHead::exeWorn() {
    al::updatePoseMtx(this, mHeadMtx);
    al::updatePoseMtx(al::getSubActor(this, "体の着ぐるみ"), mBodyMtx);
    for (int i = 0; i < mKuribos.size(); ++i) mKuribos.unsafeAt(i)->setHeadWorn(true);
}
bool TestKinokoHead::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (al::isMsgPlayerItemGet(msg)) {
        if (al::isNerve(this, &NrvTestKinokoHeadItem)) {
            mWearer = sender;
            mHeadMtx = rc::getPlayerModelJointMtxPtr(sender, "Head");
            mBodyMtx = rc::getPlayerModelJointMtxPtr(sender, "Hip");
            al::setNerve(this, &NrvTestKinokoHeadWornAnim);
        }
        return false;
    }
    if (al::isMsgBindStart(msg)) {
        if (al::isNerve(this, &NrvTestKinokoHeadWornAnim)) {
            mPuppeteer->startBind(mWearer, al::getHitSensor(this, 0));
            return true;
        }
        return false;
    }
    if (al::isMsgBindInit(msg)) {
        if (al::isNerve(this, &NrvTestKinokoHeadWornAnim)) {
            rc::startPuppetAction(mPuppeteer->getPlayerPuppet(), "LandStiffen");
            return true;
        }
        return false;
    }
    if (al::isMsgBindCancel(msg)) {
        mPuppeteer->endBind(nullptr);
        if (al::isNerve(this, &NrvTestKinokoHeadWornAnim)) al::setNerve(this, &NrvTestKinokoHeadWorn);
        return true;
    }
    return false;
}
