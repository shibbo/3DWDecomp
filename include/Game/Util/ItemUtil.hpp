#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
    class LiveActor;
    class ActorInitInfo;
    class SensorMsg;
    class HitSensor;
    class ScreenPointer;
};  // namespace al

class ItemBubble;

namespace rc {
    /**
     * @brief Item kinds selectable through the "ItemType" placement argument.
     */
    enum ItemType : s32 {
        ItemType_Coin = 0,
        ItemType_Coin10 = 1,
        ItemType_CoinRandom10 = 2,
        ItemType_CoinInfinity = 3,
        ItemType_KinokoOneUp = 4,
        ItemType_KinokoSuper = 5,
        ItemType_SuperBell = 6,
        ItemType_FireFlower = 7,
        ItemType_SuperLeaf = 8,
        ItemType_BoomerangFlower = 9,
        ItemType_SuperStar = 10,
        ItemType_Ball = 11,
        ItemType_Bomb = 12,
        ItemType_CoinBlow = 13,
        ItemType_GreenStar = 14,
        ItemType_CollectItem = 15,
        ItemType_WarpCubeLockedPiece = 16,
        ItemType_DoubleMario = 17,
        ItemType_KinokoBig = 18,
        ItemType_BoxPropeller = 19,
        ItemType_BoxKiller = 20,
        ItemType_IllustItem = 21,
        ItemType_AssistLeaf = 22,
        ItemType_SuperBellSpecial = 23,
        ItemType_CoinConcentricCircle = 24,
        ItemType_CoinBlow30 = 25,
        ItemType_DoorKey = 26,
        ItemType_KinokoTreasure = 27,
        ItemType_GoalItem = 28,
        ItemType_WhiteBell = 29,
        ItemType_Shards = 30,
    };

    void initItemByHostInfo(al::LiveActor*, const al::ActorInitInfo&, int);
    void addItemByHostInfo(al::LiveActor*, const al::ActorInitInfo&, const char*, const char*);
    int tryInitItemByHostInfo(al::LiveActor*, const al::ActorInitInfo&, int);
    int getItemType(const al::ActorInitInfo&);
    int getItemTypeByName(const char*);
    bool tryAppearItemPressDown(const al::LiveActor*, const char*);
    int getStepAppearItemPressDown();
    void killBySwitchAndAppearItem(al::LiveActor*, const char*);
    void setAppearItemFactorByMsg(const al::LiveActor*, const al::SensorMsg*, const al::HitSensor*);
    void setAppearItemFactorByMsg(const al::LiveActor*, const al::SensorMsg*,
                                  const al::ScreenPointer*);
    void setAppearItemAttackerSensorByScreenPointer(const al::LiveActor*, const al::ScreenPointer*);

    bool isMsgForBlockAll(const al::SensorMsg*, const al::HitSensor*, const al::HitSensor*, f32);
    bool isMsgForAssistBlock(const al::SensorMsg*, const al::HitSensor*, const al::HitSensor*);
    bool getMsgReturnValueForBlock(const al::SensorMsg*);
    int calcAppearDoubleMarioNumForBlock(const al::ActorInitInfo&);
    const char* getBlockSuffixName(const al::ActorInitInfo&, bool);
    int getReactionCountByMsg(const al::SensorMsg*);

    void initItemForRingItem(al::LiveActor*, const al::ActorInitInfo&);
    void appearItemForRingItem(const al::LiveActor*, const sead::Vector3f&, const sead::Vector3f&);

    void acquirerItemKinokoSuper(const al::LiveActor*, al::HitSensor*);
    void acquirerItemSuperBell(const al::LiveActor*, al::HitSensor*);
    void acquirerItemGigaBell(const al::LiveActor*, al::HitSensor*);
    void acquirerItemFireFlower(const al::LiveActor*, al::HitSensor*);
    void acquirerItemSuperLeaf(const al::LiveActor*, al::HitSensor*);
    void acquirerItemBoomerangFlower(const al::LiveActor*, al::HitSensor*);
    void acquirerItemOneUp(const al::LiveActor*, al::HitSensor*);
    float getCoinRotateYByFrame(const al::LiveActor* pActor);
    float getCoinRotateY(const al::LiveActor* pActor);
    void acquirerItemCoin(const al::LiveActor*, al::HitSensor*);
    void acquirerItemCoin3(const al::LiveActor*, al::HitSensor*);
    void acquirerItemCoin50(const al::LiveActor*, al::HitSensor*);
    void acquirerItemKinokoBig(const al::LiveActor*, al::HitSensor*);
    void acquirerItemKinokoGiga(const al::LiveActor*, al::HitSensor*);
    void acquirerItemAssistLeaf(const al::LiveActor*, al::HitSensor*);
    void acquirerItemSuperBellSpecial(const al::LiveActor*, al::HitSensor*);
    void acquirerItemWhiteBell(const al::LiveActor*, al::HitSensor*);
    bool tryAcquirerCoinIfTouchAssistTrigger(const al::SensorMsg*, al::LiveActor*,
                                             const al::ScreenPointer*);

    void startHitReactionIfThroughWater(const al::LiveActor*);
    void startHitReactionDeathIfThroughWater(al::LiveActor*);
    bool tryDisappearItemByStartGoalDemoPole(al::LiveActor*, const al::SensorMsg*);
    bool tryDisappearItemByStartGoalDemoHouse(al::LiveActor*, const al::SensorMsg*);

    al::LiveActor* tryCreateAttachedItem(const char*, int);
    ItemBubble* tryCreateAttachedItemBubble(const char*, int);
};  // namespace rc
