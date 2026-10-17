#include "MapObj/ItemBubbleSingleMode.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Project/Base/StringUtil.hpp"
ItemBubbleSingleMode::ItemBubbleSingleMode(const char*) : ItemBubble("ItemBubbleSingleModeItemActor", 0) {}
ItemBubbleSingleMode::~ItemBubbleSingleMode() {}
void ItemBubbleSingleMode::init(const al::ActorInitInfo& rInfo) {
    int type = 0;
    al::tryGetArg(&type, rInfo, "ItemType");
    switch (type) {
    case 0: setItemType(0); break;
    case 1: setItemType(4); break;
    case 2: setItemType(5); break;
    case 3: setItemType(6); break;
    case 4: setItemType(7); break;
    case 5: setItemType(8); break;
    case 6: setItemType(9); break;
    case 7: setItemType(10); break;
    case 8: setItemType(12); break;
    case 9: setItemType(14); break;
    case 10: setItemType(15); break;
    }
    initActor(rInfo, "ItemBubbleSingleMode");
    if (mItemActor) {
        updatePosture();
        float scale = 1.0f;
        al::tryGetArg(&scale, rInfo, "BubbleScale");
        al::setScaleAll(this, scale * 2.0f);
    }
}
bool ItemBubbleSingleMode::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (isDisappear() || al::isMsgDisasterSpikeAttack(pMsg) || al::isMsgLaserAttack(pMsg)) return false;
    if (al::isMsgEnemyAttackFire(pMsg)) {
        const char* name = al::getSensorHost(pSender)->getName();
        if (al::isEqualString(name, "KoopaFireBall") || al::isEqualString(name, "KoopaFireBallGiant")) return false;
    }
    if (al::isSensorRide(pSender)) {
        al::HitSensor* playerSensor = al::getHitSensor(al::tryFindNearestPlayerActor(this), "Body");
        if (al::sendMsgPlayerItemGet(al::getHitSensor(this, "Body"), playerSensor)) return true;
    }
    if (al::isMsgBallItemGet(pMsg)) {
        startDisappear();
        return true;
    }
    return ItemBubble::receiveMsg(pMsg, pSender, pReceiver);
}
void ItemBubbleSingleMode::control() { if (mItemActor) updatePosture(); }
sead::Vector3f ItemBubbleSingleMode::getItemActorOffset(int type) const {
    if (type == 6) return sead::Vector3f(0.0f, -72.5f, 0.0f);
    return ItemBubble::getItemActorOffset(type);
}
