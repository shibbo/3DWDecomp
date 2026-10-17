#include "Util/ItemUtil.hpp"
#include "MapObj/ItemBubble.hpp"
#include "MapObj/GreenStar.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

// Attachable item actors created by tryCreateAttachedItem(). Their own units are not decompiled
// yet, so only the constructors and object sizes used by this unit are declared here.



/**
 * @brief Coin attached to another object.
 */
class CoinAttach : public al::LiveActor {
public:
    CoinAttach(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x178 - 0x148];
};

/**
 * @brief 1-Up mushroom.
 */
class KinokoOneUp : public al::LiveActor {
public:
    KinokoOneUp(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x188 - 0x148];
};

/**
 * @brief Super mushroom.
 */
class KinokoSuper : public al::LiveActor {
public:
    KinokoSuper(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x188 - 0x148];
};

/**
 * @brief Super bell.
 */
class SuperBell : public al::LiveActor {
public:
    SuperBell(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x180 - 0x148];
};

/**
 * @brief Fire flower.
 */
class FireFlower : public al::LiveActor {
public:
    FireFlower(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x188 - 0x148];
};

/**
 * @brief Super leaf.
 */
class SuperLeaf : public al::LiveActor {
public:
    SuperLeaf(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x190 - 0x148];
};

/**
 * @brief Boomerang flower.
 */
class BoomerangFlower : public al::LiveActor {
public:
    BoomerangFlower(const char* pName, ItemBubble* pBubble, bool isAttached);

private:
    u8 _148[0x180 - 0x148];
};

/**
 * @brief Super star.
 */
class SuperStar : public al::LiveActor {
public:
    SuperStar(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x178 - 0x148];
};

/**
 * @brief Bomb.
 */
class Bomb : public al::LiveActor {
public:
    Bomb(const char* pName, bool isAttached);

private:
    u8 _148[0x1A0 - 0x148];
};

/**
 * @brief Collectible item (stamp).
 */
class CollectItem : public al::LiveActor {
public:
    CollectItem(const char* pName, ItemBubble* pBubble, bool isAttached);

private:
    u8 _148[0x1D8 - 0x148];
};

/**
 * @brief Double cherry.
 */
class DoubleMario : public al::LiveActor {
public:
    DoubleMario(const char* pName, ItemBubble* pBubble, bool isAttached);

private:
    u8 _148[0x168 - 0x148];
};

/**
 * @brief Illustration item.
 */
class IllustItem : public al::LiveActor {
public:
    IllustItem(const char* pName, bool isAttached);

private:
    u8 _148[0x170 - 0x148];
};

/**
 * @brief Lucky cat bell.
 */
class SuperBellSpecial : public al::LiveActor {
public:
    SuperBellSpecial(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x180 - 0x148];
};

/**
 * @brief White bell.
 */
class WhiteBell : public al::LiveActor {
public:
    WhiteBell(const char* pName, ItemBubble* pBubble);

private:
    u8 _148[0x178 - 0x148];
};

/**
 * @brief Cat shine shards.
 */
class Shards : public al::LiveActor {
public:
    Shards(const char* pName, bool isAttached);

private:
    u8 _148[0x200 - 0x148];
};

namespace {
    /// Placement argument naming the item to host.
    const char* const cItemTypeArgName = "ItemType";

    /// Default item type when the placement does not specify one.
    const char* const cItemTypeDefault = "Dummy";

    /// Appear factor for direct (player) attacks.
    const char* const cFactorDirectAttack = "直接攻撃";

    /// Appear factor for indirect attacks.
    const char* const cFactorIndirectAttack = "間接攻撃";

    /// Item appear offset for direct attacks by a mini player.
    const sead::Vector3f cAppearOffsetDirectMini(0.0f, 150.0f, 0.0f);

    /// Item appear offset for direct attacks by a player.
    const sead::Vector3f cAppearOffsetDirect(0.0f, 200.0f, 0.0f);

    /// Item appear offset for indirect attacks by a player.
    const sead::Vector3f cAppearOffsetIndirect(0.0f, 150.0f, 0.0f);

    /// Item appear offset for attacks by anything other than a player.
    const sead::Vector3f cAppearOffsetDefault(0.0f, 200.0f, 0.0f);

    /// Appear timing used by ring items.
    const char* const cTimingNormalMario = "通常マリオ用";

    /// Block suffixes indexed by [isSM][ShadowMaskSetting - 1].
    const char* const cBlockSuffixNames[2][3] = {
        {"WallSide", "ShadowMaskHide", "Collision2m"},
        {"WallSideSM", "ShadowMaskHideSM", "Collision2mSM"},
    };

    /**
     * @brief Sets the item appear factor and the matching appear offset.
     * @param pActor Actor holding the item.
     * @param pFactor Appear factor name.
     * @param pSensor Sensor of the attacker, or nullptr.
     */
    void setAppearItemFactorAndOffset(const al::LiveActor* pActor, const char* pFactor,
                                      const al::HitSensor* pSensor) {
        al::setAppearItemFactor(pActor, pFactor, pSensor);
        const sead::Vector3f* offset = &cAppearOffsetDefault;
        if (pSensor != nullptr && al::isSensorPlayer(pSensor)) {
            if (al::isEqualString(pFactor, cFactorDirectAttack)) {
                offset = rc::isPlayerMini(pSensor) ? &cAppearOffsetDirectMini
                                                   : &cAppearOffsetDirect;
            } else {
                offset = &cAppearOffsetIndirect;
            }
        }

        al::setAppearItemOffset(pActor, *offset);
    }

    /**
     * @brief Checks whether an attacker is low enough relative to the block to hit it.
     * @param pAttacker Sensor of the attacker.
     * @param pBlock Sensor of the block.
     * @param height Height of the block per unit of Y scale.
     * @return Whether the attacker is below the block's top.
     */
    bool isUnderBlockTop(const al::HitSensor* pAttacker, const al::HitSensor* pBlock, f32 height) {
        f32 diffY = al::getActorTrans(pAttacker).y - al::getActorTrans(pBlock).y;
        return diffY < al::getScaleY(al::getSensorHost(pBlock)) * height + height * -0.1f;
    }
}  // namespace

namespace rc {
    /**
     * @brief Initializes the item keeper and adds the item named by the "ItemType" argument.
     * @param pActor Host actor.
     * @param rInfo Placement info of the host.
     * @param itemNum Number of items the keeper can hold.
     */
    void initItemByHostInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo, int itemNum) {
        pActor->initItemKeeper(itemNum);
        addItemByHostInfo(pActor, rInfo, nullptr, nullptr);
    }

    /**
     * @brief Adds the item named by the host's "ItemType" placement argument.
     * @param pActor Host actor.
     * @param rInfo Placement info of the host.
     * @param pTiming Appear timing name, or nullptr.
     * @param pFactor Appear factor name, or nullptr.
     */
    void addItemByHostInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                           const char* pTiming, const char* pFactor) {
        const char* type = cItemTypeDefault;
        al::tryGetStringArg(&type, rInfo, cItemTypeArgName);

        const char* itemName;
        if (al::isEqualString(type, "Coin") || al::isEqualString(type, "Coin10")) {
            itemName = "コインx1[自動取得]";
        } else if (al::isEqualString(type, "CoinRandom10")) {
            itemName = "コインx1[飛出し出現]";
        } else if (al::isEqualString(type, "CoinInfinity")) {
            itemName = "コインx1[自動取得＆高速出現]";
        } else if (al::isEqualString(type, "Coinx3")) {
            itemName = "コインx3[自動取得]";
        } else if (al::isEqualString(type, "KinokoOneUp")) {
            itemName = "1UPキノコ";
        } else if (al::isEqualString(type, "KinokoSuper")) {
            itemName = "スーパーキノコ";
        } else if (al::isEqualString(type, "SuperBell")) {
            itemName = "スーパーベル";
        } else if (al::isEqualString(type, "FireFlower")) {
            itemName = "ファイアフラワー";
        } else if (al::isEqualString(type, "SuperLeaf")) {
            itemName = "スーパーこのは";
        } else if (al::isEqualString(type, "BoomerangFlower")) {
            itemName = "ブーメランフラワー";
        } else if (al::isEqualString(type, "SuperStar")) {
            itemName = "スーパースター";
        } else if (al::isEqualString(type, "Ball")) {
            itemName = "ボール";
        } else if (al::isEqualString(type, "Bomb")) {
            itemName = "バクダン";
        } else if (al::isEqualString(type, "CoinBlow")) {
            itemName = "コインx1";
        } else if (al::isEqualString(type, "GreenStar")) {
            itemName = "グリーンスター";
        } else if (al::isEqualString(type, "DoubleMario")) {
            itemName = "ダブルマリオ";
        } else if (al::isEqualString(type, "KinokoBig")) {
            itemName = "巨大キノコ";
        } else if (al::isEqualString(type, "AssistLeaf")) {
            itemName = "無敵このは";
        } else if (al::isEqualString(type, "SuperBellSpecial")) {
            itemName = "まねきネコベル";
        } else if (al::isEqualString(type, "CoinConcentricCircle")) {
            itemName = "同心円コイン";
        } else if (al::isEqualString(type, "CoinBlow30")) {
            itemName = "コインx30";
        } else if (al::isEqualString(type, "DoorKey")) {
            itemName = "DoorKey";
        } else if (al::isEqualString(type, "KinokoTreasure")) {
            itemName = "KinokoTreasure";
        } else if (al::isEqualString(type, "GoalItem")) {
            itemName = "GoalItem";
        } else if (al::isEqualString(type, "WhiteBell")) {
            itemName = "WhiteBell";
        } else if (al::isEqualString(type, "Shards")) {
            itemName = "Shards";
        } else {
            itemName = "コインx1[自動取得]";
        }

        al::addItem(pActor, rInfo, itemName, pTiming, pFactor, false);
    }

    /**
     * @brief Initializes the host's item unless its "ItemType" argument is "None".
     * @param pActor Host actor.
     * @param rInfo Placement info of the host.
     * @param itemNum Number of items the keeper can hold.
     * @return The item type, or -1 if the host has no item.
     */
    int tryInitItemByHostInfo(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                              int itemNum) {
        const char* type = "None";
        al::tryGetStringArg(&type, rInfo, cItemTypeArgName);
        if (al::isEqualString(type, "None")) {
            return -1;
        }

        initItemByHostInfo(pActor, rInfo, itemNum);
        return getItemType(rInfo);
    }

    /**
     * @brief Gets the item type of the host's "ItemType" placement argument.
     * @param rInfo Placement info of the host.
     * @return The item type.
     */
    int getItemType(const al::ActorInitInfo& rInfo) {
        const char* type = cItemTypeDefault;
        al::tryGetStringArg(&type, rInfo, cItemTypeArgName);
        return getItemTypeByName(type);
    }

    /**
     * @brief Converts an "ItemType" placement value to an item type.
     * @param pName Value of the "ItemType" placement argument.
     * @return The item type, defaulting to ItemType_Coin.
     */
    int getItemTypeByName(const char* pName) {
        if (al::isEqualString(pName, "Coin")) {
            return ItemType_Coin;
        }

        if (al::isEqualString(pName, "Coin10")) {
            return ItemType_Coin10;
        }

        if (al::isEqualString(pName, "CoinRandom10")) {
            return ItemType_CoinRandom10;
        }

        if (al::isEqualString(pName, "CoinInfinity")) {
            return ItemType_CoinInfinity;
        }

        if (al::isEqualString(pName, "KinokoOneUp")) {
            return ItemType_KinokoOneUp;
        }

        if (al::isEqualString(pName, "KinokoSuper")) {
            return ItemType_KinokoSuper;
        }

        if (al::isEqualString(pName, "SuperBell")) {
            return ItemType_SuperBell;
        }

        if (al::isEqualString(pName, "FireFlower")) {
            return ItemType_FireFlower;
        }

        if (al::isEqualString(pName, "SuperLeaf")) {
            return ItemType_SuperLeaf;
        }

        if (al::isEqualString(pName, "BoomerangFlower")) {
            return ItemType_BoomerangFlower;
        }

        if (al::isEqualString(pName, "SuperStar")) {
            return ItemType_SuperStar;
        }

        if (al::isEqualString(pName, "Ball")) {
            return ItemType_Ball;
        }

        if (al::isEqualString(pName, "Bomb")) {
            return ItemType_Bomb;
        }

        if (al::isEqualString(pName, "CoinBlow")) {
            return ItemType_CoinBlow;
        }

        if (al::isEqualString(pName, "GreenStar")) {
            return ItemType_GreenStar;
        }

        if (al::isEqualString(pName, "CollectItem")) {
            return ItemType_CollectItem;
        }

        if (al::isEqualString(pName, "WarpCubeLockedPiece")) {
            return ItemType_WarpCubeLockedPiece;
        }

        if (al::isEqualString(pName, "DoubleMario")) {
            return ItemType_DoubleMario;
        }

        if (al::isEqualString(pName, "KinokoBig")) {
            return ItemType_KinokoBig;
        }

        if (al::isEqualString(pName, "BoxPropeller")) {
            return ItemType_BoxPropeller;
        }

        if (al::isEqualString(pName, "BoxKiller")) {
            return ItemType_BoxKiller;
        }

        if (al::isEqualString(pName, "IllustItem")) {
            return ItemType_IllustItem;
        }

        if (al::isEqualString(pName, "AssistLeaf")) {
            return ItemType_AssistLeaf;
        }

        if (al::isEqualString(pName, "SuperBellSpecial")) {
            return ItemType_SuperBellSpecial;
        }

        if (al::isEqualString(pName, "CoinConcentricCircle")) {
            return ItemType_CoinConcentricCircle;
        }

        if (al::isEqualString(pName, "CoinBlow30")) {
            return ItemType_CoinBlow30;
        }

        if (al::isEqualString(pName, "DoorKey")) {
            return ItemType_DoorKey;
        }

        if (al::isEqualString(pName, "KinokoTreasure")) {
            return ItemType_KinokoTreasure;
        }

        if (al::isEqualString(pName, "GoalItem")) {
            return ItemType_GoalItem;
        }

        if (al::isEqualString(pName, "Shards")) {
            return ItemType_Shards;
        }

        if (al::isEqualString(pName, "WhiteBell")) {
            return ItemType_WhiteBell;
        }

        return ItemType_Coin;
    }

    /**
     * @brief Makes the item appear once the press-down reaction reaches its appear step.
     * @param pActor Actor holding the item, in its press-down nerve.
     * @param pTiming Appear timing name, or nullptr for the default timing.
     * @return Whether the item was made to appear.
     */
    bool tryAppearItemPressDown(const al::LiveActor* pActor, const char* pTiming) {
        if (!al::isStep(pActor, getStepAppearItemPressDown())) {
            return false;
        }

        if (pTiming != nullptr) {
            al::appearItemTiming(pActor, pTiming);
        } else {
            al::appearItem(pActor);
        }

        return true;
    }

    /**
     * @brief Gets the nerve step at which a pressed-down actor releases its item.
     * @return The appear step.
     */
    int getStepAppearItemPressDown() {
        return 14;
    }

    /**
     * @brief Kills a living actor by its switch and makes its item appear.
     * @param pActor Actor to kill.
     * @param pTiming Appear timing name, or nullptr for the default timing.
     */
    void killBySwitchAndAppearItem(al::LiveActor* pActor, const char* pTiming) {
        if (al::isDead(pActor)) {
            return;
        }

        pActor->kill();
        al::setAppearItemFactor(pActor, cFactorDirectAttack, nullptr);
        if (pTiming != nullptr) {
            al::appearItemTiming(pActor, pTiming);
        } else {
            al::appearItem(pActor);
        }
    }

    /**
     * @brief Sets the item appear factor and offset according to the attack message.
     * @param pActor Actor holding the item.
     * @param pMsg Attack message received.
     * @param pSensor Sensor of the attacker, or nullptr.
     */
    void setAppearItemFactorByMsg(const al::LiveActor* pActor, const al::SensorMsg* pMsg,
                                  const al::HitSensor* pSensor) {
        if (pActor->getActorItemKeeper() == nullptr) {
            return;
        }

        if (al::isMsgBlockUpperPunch(pMsg) || al::isMsgBlockLowerPunch(pMsg) ||
            al::isMsgPlayerBodyAttack(pMsg) || al::isMsgPlayerClimbAttack(pMsg) ||
            al::isMsgPlayerGiantAttack(pMsg) || al::isMsgPlayerInvincibleAttack(pMsg) ||
            al::isMsgPlayerKick(pMsg) || al::isMsgPlayerKouraAttack(pMsg) ||
            al::isMsgPlayerObjHipDropAll(pMsg) || al::isMsgPlayerObjHipDropReflectAll(pMsg) ||
            al::isMsgPlayerObjRollingAttack(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
            al::isMsgPlayerSpinAttack(pMsg) || al::isMsgPlayerTailAttack(pMsg) ||
            al::isMsgPlayerTrample(pMsg) || al::isMsgGoalKill(pMsg) || isMsgPackunEat(pMsg)) {
            setAppearItemFactorAndOffset(pActor, cFactorDirectAttack, pSensor);
        } else {
            setAppearItemFactorAndOffset(pActor, cFactorIndirectAttack, pSensor);
        }
    }

    /**
     * @brief Sets the item appear factor for an attack by an assisting screen pointer.
     * @param pActor Actor holding the item.
     * @param pMsg Attack message received.
     * @param pPointer Screen pointer that attacked.
     */
    void setAppearItemFactorByMsg(const al::LiveActor* pActor, const al::SensorMsg* pMsg,
                                  const al::ScreenPointer* pPointer) {
        setAppearItemFactorByMsg(pActor, pMsg,
                                 DrcFunction::tryFindDrcPlayerSensor(pActor, pPointer));
    }

    /**
     * @brief Sets the player assisted by a screen pointer as the item's attacker.
     * @param pActor Actor holding the item.
     * @param pPointer Screen pointer that attacked.
     */
    void setAppearItemAttackerSensorByScreenPointer(const al::LiveActor* pActor,
                                                    const al::ScreenPointer* pPointer) {
        const al::HitSensor* sensor = DrcFunction::tryFindDrcPlayerSensor(pActor, pPointer);
        if (sensor != nullptr) {
            al::setAppearItemAttackerSensor(pActor, sensor);
        }
    }

    /**
     * @brief Checks whether a message hits a block.
     * @param pMsg Message received by the block.
     * @param pAttacker Sensor of the attacker.
     * @param pBlock Sensor of the block.
     * @param height Height of the block per unit of Y scale.
     * @return Whether the message counts as a block hit.
     */
    bool isMsgForBlockAll(const al::SensorMsg* pMsg, const al::HitSensor* pAttacker,
                          const al::HitSensor* pBlock, f32 height) {
        if (al::isMsgPlayerUpperPunch(pMsg) || al::isMsgPlayerHipDropAll(pMsg) ||
            al::isMsgPlayerRollingAttack(pMsg) || al::isMsgPlayerSlidingAttack(pMsg) ||
            al::isMsgPlayerBodyAttack(pMsg)) {
            return true;
        }

        if (al::isMsgPlayerSpinAttack(pMsg) && isUnderBlockTop(pAttacker, pBlock, height)) {
            return true;
        }

        if (al::isMsgPlayerTailAttack(pMsg) && isUnderBlockTop(pAttacker, pBlock, height)) {
            return true;
        }

        if (al::isMsgPlayerClimbAttack(pMsg) && isUnderBlockTop(pAttacker, pBlock, height)) {
            return true;
        }

        if (al::isMsgKickKouraAttackCollide(pMsg) || al::isMsgBallAttackCollide(pMsg) ||
            al::isMsgExplosion(pMsg) || al::isMsgExplosionCollide(pMsg) ||
            al::isMsgPlayerBoomerangAttackCollide(pMsg) || al::isMsgPlayerGiantTouch(pMsg) ||
            al::isMsgLaserAttack(pMsg) || al::isMsgPlayerGiantHipDrop(pMsg) ||
            isMsgBullAttack(pMsg) || al::isMsgDokanBazookaAttack(pMsg)) {
            return true;
        }

        return isMsgJumpPanelAction(pMsg);
    }

    /**
     * @brief Checks whether a message hits an assist block.
     * @param pMsg Message received by the block.
     * @param pAttacker Sensor of the attacker.
     * @param pBlock Sensor of the block.
     * @return Whether the message counts as a hit by something other than an enemy.
     */
    bool isMsgForAssistBlock(const al::SensorMsg* pMsg, const al::HitSensor* pAttacker,
                             const al::HitSensor* pBlock) {
        if (!isMsgForBlockAll(pMsg, pAttacker, pBlock, 100.0f)) {
            return false;
        }

        if (al::isSensorEnemyAttack(pAttacker)) {
            return false;
        }

        return !al::isSensorEnemyBody(pAttacker);
    }

    /**
     * @brief Gets the value a block returns from receiveMsg for a hit message.
     * @param pMsg Message received by the block.
     * @return False for rolling attacks so they pass through, true otherwise.
     */
    bool getMsgReturnValueForBlock(const al::SensorMsg* pMsg) {
        return !al::isMsgPlayerRollingAttack(pMsg);
    }

    /**
     * @brief Calculates how many double cherries a block releases.
     * @param rInfo Placement info of the block.
     * @return The maximum control user number for double cherry blocks, zero otherwise.
     */
    int calcAppearDoubleMarioNumForBlock(const al::ActorInitInfo& rInfo) {
        if (getItemType(rInfo) == ItemType_DoubleMario) {
            return getControlUserNumMax();
        }

        return 0;
    }

    /**
     * @brief Gets the actor suffix of a block from its "ShadowMaskSetting" argument.
     * @param rInfo Placement info of the block.
     * @param isSM Whether to use the Super Mario (Bowser's Fury) variant.
     * @return The suffix name, or nullptr if the block has no suffix.
     */
    const char* getBlockSuffixName(const al::ActorInitInfo& rInfo, bool isSM) {
        s32 setting = 0;
        al::tryGetArg(&setting, rInfo, "ShadowMaskSetting");
        if (setting == 0) {
            return isSM ? "SM" : nullptr;
        }

        if (isSM) {
            return cBlockSuffixNames[1][setting - 1];
        }

        return cBlockSuffixNames[0][setting - 1];
    }

    /**
     * @brief Gets the number of reaction frames a block plays for an attack message.
     * @param pMsg Message received by the block, or nullptr.
     * @return The reaction frame count.
     */
    int getReactionCountByMsg(const al::SensorMsg* pMsg) {
        if (pMsg == nullptr) {
            return 6;
        }

        if (al::isMsgPlayerTailAttack(pMsg)) {
            return 14;
        }

        if (al::isMsgPlayerClimbAttack(pMsg)) {
            return 22;
        }

        if (al::isMsgPlayerBodyAttack(pMsg)) {
            return 22;
        }

        if (al::isMsgPlayerSpinAttack(pMsg)) {
            return 40;
        }

        if (al::isMsgExplosion(pMsg)) {
            return 3;
        }

        return 6;
    }

    /**
     * @brief Initializes the item released by a ring item.
     * @param pActor Ring item actor.
     * @param rInfo Placement info of the ring item.
     */
    void initItemForRingItem(al::LiveActor* pActor, const al::ActorInitInfo& rInfo) {
        pActor->initItemKeeper(1);
        const char* type = nullptr;
        if (al::tryGetStringArg(&type, rInfo, cItemTypeArgName)) {
            addItemByHostInfo(pActor, rInfo, cTimingNormalMario, nullptr);
        } else {
            al::addItem(pActor, rInfo, "スーパーこのは", cTimingNormalMario, nullptr, false);
        }
    }

    /**
     * @brief Makes the item of a ring item appear.
     * @param pActor Ring item actor.
     * @param rTrans Appear position.
     * @param rFront Appear front direction.
     */
    void appearItemForRingItem(const al::LiveActor* pActor, const sead::Vector3f& rTrans,
                               const sead::Vector3f& rFront) {
        al::appearItemTiming(pActor, cTimingNormalMario, rTrans, rFront);
    }

    /**
     * @brief Gives a super mushroom to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemKinokoSuper(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "スーパーキノコ");
    }

    /**
     * @brief Gives a super bell to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemSuperBell(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "スーパーベル");
    }

    /**
     * @brief Gives a giga bell to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemGigaBell(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "GigaBell");
    }

    /**
     * @brief Gives a fire flower to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemFireFlower(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "ファイアフラワー");
    }

    /**
     * @brief Gives a super leaf to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemSuperLeaf(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "スーパーこのは");
    }

    /**
     * @brief Gives a boomerang flower to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemBoomerangFlower(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "ブーメランフラワー");
    }

    /**
     * @brief Gives a 1-Up mushroom to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemOneUp(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "1UPキノコ");
    }

    /**
     * @brief Gives a coin to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemCoin(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "コインx1");
    }

    /**
     * @brief Gives three coins to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemCoin3(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "コインx3");
    }

    /**
     * @brief Gives fifty coins to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemCoin50(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "コインx50");
    }

    /**
     * @brief Gives a mega mushroom to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemKinokoBig(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "巨大キノコ");
    }

    /**
     * @brief Gives a giga mushroom to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemKinokoGiga(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "KinokoGiga");
    }

    /**
     * @brief Gives an invincibility leaf to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemAssistLeaf(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "無敵このは");
    }

    /**
     * @brief Gives a lucky cat bell to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemSuperBellSpecial(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "まねきネコベル");
    }

    /**
     * @brief Gives a white bell to the owner of a sensor.
     * @param pActor Item actor.
     * @param pSensor Sensor of the acquirer.
     */
    void acquirerItemWhiteBell(const al::LiveActor* pActor, al::HitSensor* pSensor) {
        al::acquirerItem(pActor, pSensor, "WhiteBell");
    }

    /**
     * @brief Gives a coin to the assisted player when an assist touch triggers on the actor.
     * @param pMsg Message received by the actor.
     * @param pActor Coin actor.
     * @param pPointer Screen pointer that touched the actor.
     * @return Whether the coin was acquired.
     */
    bool tryAcquirerCoinIfTouchAssistTrigger(const al::SensorMsg* pMsg, al::LiveActor* pActor,
                                             const al::ScreenPointer* pPointer) {
        if (!al::isMsgTouchAssistTrig(pMsg)) {
            return false;
        }

        acquirerItemCoin(pActor, DrcFunction::tryFindDrcPlayerSensor(pActor, pPointer));
        al::startHitReactionGet(pActor);
        return true;
    }

    /**
     * @brief Plays the water surface splash if the actor crossed a water surface this frame.
     * @param pActor Actor to check.
     */
    void startHitReactionIfThroughWater(const al::LiveActor* pActor) {
        sead::Vector3f trans = al::getTrans(pActor);
        sead::Vector3f prevTrans(0.0f, 0.0f, 0.0f);
        prevTrans.setSub(trans, al::getVelocity(pActor));

        const al::IUseAreaObj* areaUser = pActor;
        bool isInWater = rc::isInWaterArea(areaUser, trans);
        if (isInWater == rc::isInWaterArea(areaUser, prevTrans)) {
            return;
        }

        sead::Vector3f hitPos(0.0f, 0.0f, 0.0f);
        sead::Vector3f normal;
        const al::AreaObj* areaObj =
            rc::tryFindAreaObj(areaUser, AreaObjType::WaterArea, isInWater ? trans : prevTrans);
        if (!al::checkAreaObjCollisionByArrow(&hitPos, &normal, areaObj, prevTrans, trans)) {
            return;
        }

        if (normal.y > 0.0f) {
            al::startHitReactionHitEffect(pActor, "水面通過", hitPos);
        } else if (normal.y < 0.0f) {
            al::startHitReactionHitEffect(pActor, "水面通過[底面]", hitPos);
        } else {
            al::startHitReactionHitEffect(pActor, "水面通過[側面]", hitPos);
        }
    }

    /**
     * @brief Kills an actor that fell into water in single mode, else plays the splash only.
     * @param pActor Actor to check.
     */
    void startHitReactionDeathIfThroughWater(al::LiveActor* pActor) {
        if (!GameDataFunction::isSingleMode(GameDataHolderAccessor(pActor))) {
            startHitReactionIfThroughWater(pActor);
            return;
        }

        if (al::isSensorValid(al::getHitSensor(pActor, nullptr)) && al::isInWaterArea(pActor)) {
            al::startHitReactionDeath(pActor);
            pActor->kill();
        }
    }

    /**
     * @brief Kills an item when the goal pole demo starts.
     * @param pActor Item actor.
     * @param pMsg Message received by the item.
     * @return Whether the item was killed.
     */
    bool tryDisappearItemByStartGoalDemoPole(al::LiveActor* pActor, const al::SensorMsg* pMsg) {
        if (!isMsgStartGoalDemoPole(pMsg)) {
            return false;
        }

        al::startHitReactionDeath(pActor);
        pActor->kill();
        return true;
    }

    /**
     * @brief Removes an item without any effect when the goal house demo starts.
     * @param pActor Item actor.
     * @param pMsg Message received by the item.
     * @return Whether the item was removed.
     */
    bool tryDisappearItemByStartGoalDemoHouse(al::LiveActor* pActor, const al::SensorMsg* pMsg) {
        if (!isMsgStartGoalDemoHouse(pMsg)) {
            return false;
        }

        if (pActor->getEffectKeeper() != nullptr) {
            al::tryDeleteEmitterAndParticleAll(pActor);
        }

        if (pActor->getActorPrePassLightKeeper() != nullptr) {
            al::killPrePassLightAll(pActor, 0);
        }

        pActor->makeActorDead();
        return true;
    }

    /**
     * @brief Creates the actor of an item attached to another object.
     * @param pName Actor name.
     * @param itemType Item type.
     * @return The new item actor, or nullptr if the item type cannot be attached.
     */
    al::LiveActor* tryCreateAttachedItem(const char* pName, int itemType) {
        switch (itemType) {
        case ItemType_Coin:
            return new CoinAttach(pName, nullptr);
        case ItemType_KinokoOneUp:
            return new KinokoOneUp(pName, nullptr);
        case ItemType_KinokoSuper:
            return new KinokoSuper(pName, nullptr);
        case ItemType_SuperBell:
            return new SuperBell(pName, nullptr);
        case ItemType_FireFlower:
            return new FireFlower(pName, nullptr);
        case ItemType_SuperLeaf:
            return new SuperLeaf(pName, nullptr);
        case ItemType_BoomerangFlower:
            return new BoomerangFlower(pName, nullptr, true);
        case ItemType_SuperStar:
            return new SuperStar(pName, nullptr);
        case ItemType_Bomb:
            return new Bomb(pName, true);
        case ItemType_GreenStar:
            return new GreenStar(pName, nullptr, true);
        case ItemType_CollectItem:
            return new CollectItem(pName, nullptr, true);
        case ItemType_DoubleMario:
            return new DoubleMario(pName, nullptr, false);
        case ItemType_IllustItem:
            return new IllustItem(pName, true);
        case ItemType_SuperBellSpecial:
            return new SuperBellSpecial(pName, nullptr);
        case ItemType_WhiteBell:
            return new WhiteBell(pName, nullptr);
        case ItemType_Shards:
            return new Shards(pName, true);
        default:
            return nullptr;
        }
    }

    /**
     * @brief Creates the bubble that carries an attached item.
     * @param pName Actor name.
     * @param itemType Item type carried by the bubble.
     * @return The new bubble, or nullptr if the item type cannot be carried in a bubble.
     */
    ItemBubble* tryCreateAttachedItemBubble(const char* pName, int itemType) {
        switch (itemType) {
        case ItemType_Coin:
        case ItemType_KinokoOneUp:
        case ItemType_KinokoSuper:
        case ItemType_SuperBell:
        case ItemType_FireFlower:
        case ItemType_SuperLeaf:
        case ItemType_BoomerangFlower:
        case ItemType_SuperStar:
        case ItemType_Bomb:
        case ItemType_GreenStar:
        case ItemType_CollectItem:
        case ItemType_DoubleMario:
        case ItemType_SuperBellSpecial:
        case ItemType_WhiteBell:
            return new ItemBubble(pName, itemType);
        default:
            return nullptr;
        }
    }
};  // namespace rc
