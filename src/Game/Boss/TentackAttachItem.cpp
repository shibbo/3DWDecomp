#include "Boss/TentackAttachItem.hpp"
#include "Boss/TentackAttachItemHolder.hpp"
#include "MapObj/Coin.hpp"
#include "MapObj/SuperBell.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
namespace {
const sead::Vector3f sOffsets[] = {{0.0f, 100.0f, 0.0f}, {0.0f, 100.0f, 0.0f}, {0.0f, 50.0f, 0.0f}};
const int sItemTypes[] = {2, 2, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1};
}
/** @brief Creates a group of scaffold items. @param pName Actor name. @param type Item group type. */
TentackAttachItem::TentackAttachItem(const char* pName, ETentackAttachItemType type) : LiveActor(pName), mType(type) {}
/** @brief Destroys the item group. */
TentackAttachItem::~TentackAttachItem() = default;
/** @brief Creates the carried items and their offsets. @param rInfo Initialization data. */
void TentackAttachItem::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    int count = int(mType) == ETentackAttachItemType::CoinThree ? 3 : 1;
    mItems.allocBuffer(count, nullptr);
    mOffsets.allocBuffer(count, nullptr);
    if (int(mType) == ETentackAttachItemType::Coin || int(mType) == ETentackAttachItemType::CoinThree) {
        for (int i = 0; i < count; i++) mItems.pushBack(new Coin("コイン[テンタック付属アイテム"));
    } else if (int(mType) == ETentackAttachItemType::SuperBell) mItems.pushBack(new SuperBell("スーパーベル[テンタック付属アイテム]", nullptr));
    for (int i = 0; i < mItems.size(); i++) {
        al::initCreateActorNoPlacementInfo(mItems.at(i), rInfo);
        al::offCollide(mItems.at(i));
        al::invalidateClipping(mItems.at(i));
    }
    if (int(mType) == ETentackAttachItemType::CoinThree) {
        sead::Vector3f offset = sead::Vector3f::ez * 60.0f;
        float angle = 360.0f / count;
        for (int i = 0; i < count; i++) {
            mOffsets.emplaceBack(sOffsets[mType] + offset);
            al::rotateVectorDegreeY(&offset, angle);
        }
    } else mOffsets.emplaceBack(sOffsets[mType]);
    for (int i = 0; i < mItems.size(); i++) mItems[i]->makeActorDead();
    makeActorDead();
}
/** @brief Shows the items and starts their pickup delay. */
void TentackAttachItem::appear() {
    al::LiveActor::appear();
    mFrame = 0;
    for (int i = 0; i < mItems.size(); i++) {
        if (al::isDead(mItems.at(i))) {
            mItems[i]->appear();
            al::invalidateHitSensors(mItems.at(i));
        }
    }
    updateItemTrans();
}
/** @brief Moves active items with their host scaffold. */
void TentackAttachItem::updateItemTrans() {
    for (int i = 0; i < mItems.size(); i++)
        if (al::isAlive(mItems.at(i))) al::resetPosition(mItems.at(i), *mHostTrans + *mOffsets.unsafeAt(i), false);
}
/** @brief Reserves the group for a scaffold. @param pTrans Host position. */
void TentackAttachItem::attach(const sead::Vector3f* pTrans) { mHostTrans = pTrans; mIsAttached = true; }
/** @brief Releases the reservation and removes active items. */
void TentackAttachItem::releaseAndTryKill() {
    mIsAttached = false;
    mHostTrans = nullptr;
    if (al::isAlive(this)) kill();
}
/** @brief Removes the group and all active items. */
void TentackAttachItem::kill() {
    al::LiveActor::kill();
    for (int i = 0; i < mItems.size(); i++) if (al::isAlive(mItems.at(i))) mItems[i]->kill();
}
/** @brief Plays the consumption reaction and removes the items. @param isForce Selects the burned reaction. */
void TentackAttachItem::eat(bool isForce) {
    const char* reaction = isForce ? "焼かれる" : "食べられる";
    for (int i = 0; i < mItems.size(); i++)
        if (al::isAlive(mItems.at(i))) al::startHitReaction(mItems.at(i), reaction);
    kill();
}
/** @brief Updates positions, pickup sensors, and group lifetime. */
void TentackAttachItem::control() {
    ++mFrame;
    updateItemTrans();
    if (mFrame == 17) for (int i = 0; i < mItems.size(); i++) al::validateHitSensors(mItems.at(i));
    bool allDead = true;
    for (int i = 0; i < mItems.size(); i++) allDead &= al::isDead(mItems.at(i));
    if (allDead) kill();
}
/** @brief Allocates the pool of scaffold item groups. */
TentackAttachItemHolder::TentackAttachItemHolder() { mItems.allocBuffer(18, nullptr); }
/** @brief Creates the configured item groups. @param rInfo Initialization data. */
void TentackAttachItemHolder::init(const al::ActorInitInfo& rInfo) {
    for (int i = 0; i < mItems.capacity(); i++) {
        auto* item = new TentackAttachItem("テンタック付属アイテム", sItemTypes[i]);
        item->init(rInfo);
        mItems.pushBack(item);
    }
}
/** @brief Reserves an available item group. @param pHost Scaffold actor. @param type Item group type. @return Reserved group, or null. */
TentackAttachItem* TentackAttachItemHolder::attachItem(al::LiveActor* pHost, int type) {
    return tryAttachItem(pHost, type);
}
/** @brief Reserves an available item group. @param pHost Scaffold actor. @param type Item group type. @return Reserved group, or null. */
TentackAttachItem* TentackAttachItemHolder::tryAttachItem(al::LiveActor* pHost, int type) {
    for (int i = 0; i < mItems.size(); i++) {
        auto* item = mItems.at(i);
        if (int(item->getType()) == type && al::isDead(item) && !item->isAttached()) {
            item->attach(al::getTransPtr(pHost));
            return item;
        }
    }
    return nullptr;
}
