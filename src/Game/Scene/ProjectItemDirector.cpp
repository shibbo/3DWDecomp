#include "Scene/ProjectItemDirector.hpp"

#include <attributes.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

#include "Enemy/Bomb.hpp"
#include "Layout/SceneLayoutBase.hpp"
#include "Layout/SingleModeSceneLayout.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Item/ItemUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "MapObj/AssistLeaf.hpp"
#include "MapObj/BoomerangFlower.hpp"
#include "MapObj/CoinBlow.hpp"
#include "MapObj/CoinCountUp.hpp"
#include "MapObj/DoorKey.hpp"
#include "MapObj/DoubleMario.hpp"
#include "MapObj/FireFlower.hpp"
#include "MapObj/Fury/GigaBell.hpp"
#include "MapObj/ItemHolder.hpp"
#include "MapObj/KinokoBig.hpp"
#include "MapObj/KinokoGiga.hpp"
#include "MapObj/KinokoOneUp.hpp"
#include "MapObj/KinokoSuper.hpp"
#include "MapObj/KinokoTreasure.hpp"
#include "MapObj/SuperBell.hpp"
#include "MapObj/SuperBellSpecial.hpp"
#include "MapObj/SuperLeaf.hpp"
#include "MapObj/SuperStar.hpp"
#include "MapObj/WhiteBell.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "MapObj/Ball.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {

/// Velocity added to a blown coin so it hops up out of the spawner.
const sead::Vector3f sCoinBlowJumpVelocity(0.0f, 22.0f, 0.0f);
/// Velocity added to a blown coin under water.
const sead::Vector3f sCoinBlowJumpVelocityInWater(0.0f, 15.0f, 0.0f);

/// Items a player can win from every hundred coins in Bowser's Fury.
const char* const sHundredCoinItemNames[] = {
    "スーパーキノコ",    "スーパーベル",     "ファイアフラワー",
    "スーパーこのは",    "ブーメランフラワー", "まねきネコベル",
};

/**
 * @brief Select the jump velocity offset of a blown coin.
 * @param isInWater Whether the coin appears under water.
 * @return The velocity offset.
 */
inline const sead::Vector3f& getCoinBlowJumpVelocity(bool isInWater) {
    return isInWater ? sCoinBlowJumpVelocityInWater : sCoinBlowJumpVelocity;
}

/**
 * @brief Spawn a blown coin.
 * @param pCoin The coin to spawn.
 * @param rTrans Spawn position.
 * @param rVelocity Initial velocity without the jump offset.
 * @param isInWater Whether the coin appears under water.
 */
inline void appearCoinBlow(CoinBlow* pCoin, const sead::Vector3f& rTrans,
                           const sead::Vector3f& rVelocity, bool isInWater) {
    al::setTrans(pCoin, rTrans);
    al::resetPosition(pCoin, false);
    al::updateMaterialCodeWater(pCoin, isInWater, false);
    al::setVelocity(pCoin, rVelocity + getCoinBlowJumpVelocity(isInWater));
    pCoin->setLifeTime(600);
    pCoin->appearWithHitReaction();
}

/**
 * @brief Spawn a counting-up coin that is collected right away.
 * @param pCoin The coin to spawn.
 * @param rTrans Spawn position.
 */
inline void placeCoinCountUp(CoinCountUp* pCoin, const sead::Vector3f& rTrans) {
    al::setTrans(pCoin, rTrans);
    al::resetPosition(pCoin, false);
    rc::updateMaterialCodeWater(pCoin);
}

/**
 * @brief Blow coins out in a ring around a position.
 * @param pDirector The item director.
 * @param rTrans Spawn position.
 * @param num Number of coins.
 */
void appearCoinBlowRing(const ProjectItemDirector* pDirector, const sead::Vector3f& rTrans,
                        s32 num) {
    bool isInWater = rc::isInWaterArea(pDirector, rTrans);
    sead::Vector3f velocity = sead::Vector3f::ez;
    velocity *= isInWater ? 1.5f : 4.0f;

    for (s32 i = 0; i < num; i++) {
        CoinBlow* coin = pDirector->getItemHolder()->getCoinBlow();
        al::rotateVectorDegreeY(&velocity, 360.0f / num);
        appearCoinBlow(coin, rTrans, velocity, isInWater);

        if (i == 0) {
            al::startSe(coin, "PgAppear");
        }
    }
}

/**
 * @brief Place an item and turn it to a front direction.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @return False if the front can't be applied to the item.
 */
ALWAYS_INLINE bool tryPlaceItem(al::LiveActor* pItem, const sead::Vector3f& rTrans,
                                const sead::Vector3f& rFront) {
    al::setTrans(pItem, rTrans);

    if (al::tryGetQuatPtr(pItem) != nullptr) {
        sead::Quatf* quat = al::getQuatPtr(pItem);
        sead::Vector3f side;
        side.setCross(sead::Vector3f::ey, rFront);

        if (al::isNearZero(side, 0.001f)) {
            al::makeQuatUpFront(quat, sead::Vector3f::ey, sead::Vector3f::ez);
        } else {
            al::makeQuatUpFront(quat, sead::Vector3f::ey, rFront);
        }
    } else {
        if (al::isNearDirection(rFront, sead::Vector3f::ey, 0.01f)) {
            return false;
        }

        al::setFront(pItem, rFront);
    }

    al::resetPosition(pItem, false);
    return true;
}

/**
 * @brief Let an item appear as if pulled out of something.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @return The item, or nullptr if it didn't appear.
 */
template <typename T>
T* appearItemTakeOut(T* pItem, const sead::Vector3f& rTrans, const sead::Vector3f& rFront) {
    if (!tryPlaceItem(pItem, rTrans, rFront)) {
        return nullptr;
    }

    rc::updateMaterialCodeWater(pItem);
    pItem->appearItemTakeOut();
    return pItem;
}

/**
 * @brief Let an item pop up out of a block.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @return The item, or nullptr if it didn't appear.
 */
template <typename T>
T* appearItemPopUpAbove(T* pItem, const sead::Vector3f& rTrans, const sead::Vector3f& rFront) {
    if (!tryPlaceItem(pItem, rTrans, rFront)) {
        return nullptr;
    }

    rc::updateMaterialCodeWater(pItem);
    pItem->appearPopUpAbove();
    return pItem;
}

/**
 * @brief Let an item fly towards a sensor.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @param pSensor Target sensor.
 * @return The item, or nullptr if it didn't appear.
 */
template <typename T>
T* appearItemHoming(T* pItem, const sead::Vector3f& rTrans, const sead::Vector3f& rFront,
                    const al::HitSensor* pSensor) {
    if (!tryPlaceItem(pItem, rTrans, rFront)) {
        return nullptr;
    }

    rc::updateMaterialCodeWater(pItem);
    pItem->appearItemHoming(pSensor);
    return pItem;
}

/**
 * @brief Let an item pop up forwards.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @return The item, or nullptr if it didn't appear.
 */
template <typename T>
T* appearItemPopUpFront(T* pItem, const sead::Vector3f& rTrans, const sead::Vector3f& rFront) {
    if (!tryPlaceItem(pItem, rTrans, rFront)) {
        return nullptr;
    }

    pItem->appearPopUpFront();
    return pItem;
}

/**
 * @brief Let an item pop up forwards, optionally popping up again on collision.
 * @param pItem The item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @param isPopUpOnCollide Whether the item pops up again when it hits a wall.
 * @return The item, or nullptr if it didn't appear.
 */
template <typename T>
T* appearItemPopUpFront(T* pItem, const sead::Vector3f& rTrans, const sead::Vector3f& rFront,
                        bool isPopUpOnCollide) {
    if (!tryPlaceItem(pItem, rTrans, rFront)) {
        return nullptr;
    }

    if (isPopUpOnCollide) {
        pItem->setPopUpOnCollide();
    }

    pItem->appearPopUpFront();
    return pItem;
}

/**
 * @brief Spawn a power-up item that the player can also pull out of the item stock.
 * @param pItem The item.
 * @param pDirector The item director.
 * @param pName Placement name of the item.
 * @param pStockName Placement name of the item-stock variant.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @param pSensor Sensor of the actor that released the item.
 * @param isSingleMode Whether Bowser's Fury is played.
 * @param isTakeOut Whether the item is pulled out of something.
 * @param isPopUpOnCollide Whether the item pops up again when it hits a wall.
 * @return The item to play the appear sound for, or nullptr.
 */
inline bool isPlayerMiniSensor(const al::HitSensor* pSensor, bool isSingleMode) {
    return pSensor != nullptr && !isSingleMode && al::isSensorPlayer(pSensor) &&
           rc::isPlayerMini(pSensor);
}

}  // namespace

/**
 * @brief Construct the item director.
 * @param rInfo Layout init info of the scene (unused).
 * @param pGameDataHolder The game data.
 * @param pPlayerHolder The scene's players.
 * @param pAreaObjDirector The scene's areas.
 */
ProjectItemDirector::ProjectItemDirector(const al::LayoutInitInfo& rInfo,
                                         GameDataHolder* pGameDataHolder,
                                         al::PlayerHolder* pPlayerHolder,
                                         al::AreaObjDirector* pAreaObjDirector)
    : mGameDataHolder(pGameDataHolder), mPlayerHolder(pPlayerHolder),
      mAreaObjDirector(pAreaObjDirector) {}

/**
 * @brief Set the HUD that receives stocked items.
 * @param pSceneLayout The scene layout.
 */
void ProjectItemDirector::setSceneLayout(SceneLayoutBase* pSceneLayout) {
    mSceneLayout = pSceneLayout;
}

/**
 * @brief Create the pool of spawnable items.
 * @param rInfo Actor init info.
 * @param isSingleMode Whether Bowser's Fury is played.
 */
void ProjectItemDirector::createItemHolder(const al::ActorInitInfo& rInfo, bool isSingleMode) {
    mItemHolder = new ItemHolder(rInfo, isSingleMode);
}

/**
 * @brief Spawn an item by its placement name.
 * @param pName Placement name of the item.
 * @param rTrans Position.
 * @param rFront Front direction.
 * @param pSensor Sensor of the actor that released the item.
 * @param isTakeOut Whether the item is pulled out of something.
 * @param isPopUpOnCollide Whether the item pops up again when it hits a wall.
 */
void ProjectItemDirector::appearItem(const char* pName, const sead::Vector3f& rTrans,
                                     const sead::Vector3f& rFront, const al::HitSensor* pSensor,
                                     bool isTakeOut, bool isPopUpOnCollide) const {
    if (mItemHolder == nullptr || al::isEqualString(pName, "なし")) {
        return;
    }

    bool isSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(mGameDataHolder));
    al::HitSensor* sensor = const_cast<al::HitSensor*>(pSensor);

    if (al::isEqualString(pName, "コインx1[自動取得]") ||
        al::isEqualString(pName, "コインx1[自動取得＆高速出現]")) {
        CoinCountUp* coin = mItemHolder->getCoinCountUp();
        placeCoinCountUp(coin, rTrans);

        if (al::isEqualString(pName, "コインx1[自動取得＆高速出現]")) {
            coin->appearQuick();
        } else {
            coin->appear();
        }

        al::startSe(coin, "PgGet");
        acquirerItem(coin, sensor, "コインx1[自動取得]");
    } else if (al::isEqualString(pName, "コインx3[自動取得]")) {
        CoinCountUp* coin = mItemHolder->getCoinCountUp();
        placeCoinCountUp(coin, rTrans + sead::Vector3f(0.0f, 0.0f, 70.0f));
        coin->appearDelayQuick(0);
        acquirerItem(coin, sensor, "コインx1[自動取得]");
        al::startSe(coin, "PgGet3");

        coin = mItemHolder->getCoinCountUp();
        placeCoinCountUp(coin, rTrans + sead::Vector3f(60.9f, 0.0f, -35.0f));
        coin->appearDelayQuick(8);
        acquirerItem(coin, sensor, "コインx1[自動取得]");

        coin = mItemHolder->getCoinCountUp();
        placeCoinCountUp(coin, rTrans + sead::Vector3f(-60.9f, 0.0f, -35.0f));
        coin->appearDelayQuick(16);
        acquirerItem(coin, sensor, "コインx1[自動取得]");
    } else if (al::isEqualString(pName, "コインx10[自動取得]")) {
        for (s32 i = 0; i < 10; i++) {
            CoinCountUp* coin = mItemHolder->getCoinCountUp();
            f32 offsetX = al::getRandom(-50.0f, 50.0f);
            f32 offsetZ = al::getRandom(-50.0f, 50.0f);
            placeCoinCountUp(coin, rTrans + sead::Vector3f(offsetX, 50.0f, offsetZ));
            coin->appearDelayQuick(i * 6);
            acquirerItem(coin, sensor, "コインx1[自動取得]");

            if (i == 0) {
                al::startSe(coin, "PgGet10");
            }
        }
    } else if (al::isEqualString(pName, "コインx1[自動取得＆前方出現]")) {
        CoinCountUp* coin = mItemHolder->getCoinCountUp();
        placeCoinCountUp(coin, rTrans);
        coin->appearFront(rFront);
        al::startSe(coin, "PgGet");
        acquirerItem(coin, sensor, "コインx1[自動取得]");
    } else if (al::isEqualString(pName, "コインx1")) {
        CoinBlow* coin = mItemHolder->getCoinBlow();
        bool isInWater = rc::isInWaterArea(this, rTrans);
        appearCoinBlow(coin, rTrans, sead::Vector3f::zero, isInWater);
        al::startSe(coin, "PgAppearLight");
    } else if (al::isEqualString(pName, "コインx3")) {
        appearCoinBlowRing(this, rTrans, 3);
    } else if (al::isEqualString(pName, "コインx5")) {
        appearCoinBlowRing(this, rTrans, 5);
    } else if (al::isEqualString(pName, "コインx5[広範囲]")) {
        bool isInWater = rc::isInWaterArea(this, rTrans);
        sead::Vector3f velocity = sead::Vector3f::ez;
        velocity *= 10.0f;

        for (s32 i = 0; i < 5; i++) {
            CoinBlow* coin = mItemHolder->getCoinBlow();
            al::rotateVectorDegreeY(&velocity, 72.0f);
            appearCoinBlow(coin, rTrans, velocity, isInWater);

            if (i == 0) {
                al::startSe(coin, "PgAppear");
            }
        }
    } else if (al::isEqualString(pName, "コインx1[飛出し出現]")) {
        CoinBlow* coin = mItemHolder->getCoinBlow();
        bool isInWater = rc::isInWaterArea(this, rTrans);
        appearCoinBlow(coin, rTrans, rFront * (isInWater ? 2.5f : 6.0f), isInWater);
        al::startSe(coin, "PgAppearLight");
    } else if (al::isEqualString(pName, "コインx3[飛出し出現]")) {
        bool isInWater = rc::isInWaterArea(this, rTrans);
        sead::Vector3f velocity = rFront * (isInWater ? 2.5f : 6.0f);
        f32 angle = velocity.length() * 0.6f;

        for (s32 i = 0; i < 3; i++) {
            CoinBlow* coin = mItemHolder->getCoinBlow();
            sead::Vector3f dir = velocity;
            al::turnRandomVector(&dir, velocity, angle);
            appearCoinBlow(coin, rTrans, dir, isInWater);

            if (i == 0) {
                al::startSe(coin, "PgAppear");
            }
        }
    } else if (al::isEqualString(pName, "コインx1[飛び散り10コイン]")) {
        CoinBlow* coin = mItemHolder->getCoinBlow();
        bool isInWater = rc::isInWaterArea(this, rTrans);
        appearCoinBlow(coin, rTrans, rFront * (isInWater ? 2.5f : 6.0f), isInWater);
        coin->setOffCollide(5);
        al::startSe(coin, "PgAppearLight");
    } else if (al::isEqualString(pName, "1UPキノコ")) {
        KinokoOneUp* kinoko = appearItemPopUpFront(mItemHolder->getKinokoOneUp(), rTrans, rFront,
                                                   isPopUpOnCollide);
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "1UPキノコ[リフティング]")) {
        KinokoOneUp* kinoko = appearItemTakeOut(mItemHolder->getKinokoOneUp(), rTrans, rFront);
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "1UPキノコ[強制取得]")) {
        KinokoOneUp* kinoko = mItemHolder->getKinokoOneUp();
        sead::Quatf* quat = al::getQuatPtr(kinoko);
        sead::Vector3f side;
        side.setCross(sead::Vector3f::ey, rFront);

        if (al::isNearZero(side, 0.001f)) {
            al::makeQuatUpFront(quat, sead::Vector3f::ey, sead::Vector3f::ez);
        } else {
            al::makeQuatUpFront(quat, sead::Vector3f::ey, rFront);
        }

        kinoko->appearForceGet(pSensor);
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "1UPキノコ[ルート土管]")) {
        KinokoOneUp* kinoko = mItemHolder->getKinokoOneUp();
        al::setTrans(kinoko, rTrans);
        al::resetPosition(kinoko, false);
        kinoko->setInRouteDokan();
        kinoko->appearWait();
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "1UPキノコ[真上出現]")) {
        KinokoOneUp* kinoko = appearItemPopUpAbove(mItemHolder->getKinokoOneUp(), rTrans, rFront);
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "KinokoTreasure")) {
        appearItemPopUpFront(mItemHolder->getKinokoTreasure(), rTrans, rFront);
    } else if (al::isEqualString(pName, "スーパーキノコ") ||
               al::isEqualString(pName, "スーパーキノコ[アイテムストック]") ||
               al::isEqualString(pName, "スーパーキノコ[チョイスブロック]")) {
        if (al::isEqualString(pName, "スーパーキノコ[アイテムストック]")) {
            KinokoSuper* kinoko = mItemHolder->getKinokoSuper();

            if (isSingleMode) {
                appearItemHoming(kinoko, rTrans, rFront, pSensor);
            } else {
                appearItemTakeOut(kinoko, rTrans, rFront);
            }

            return;
        }

        bool isChoiceBlock = al::isEqualString(pName, "スーパーキノコ[チョイスブロック]");
        KinokoSuper* kinoko = mItemHolder->getKinokoSuper();

        if (isChoiceBlock) {
            if (tryPlaceItem(kinoko, rTrans, rFront)) {
                kinoko->appearPopUpFrontNoRunaway();
            }
        } else if (isTakeOut) {
            appearItemTakeOut(kinoko, rTrans, rFront);
        } else {
            appearItemPopUpFront(kinoko, rTrans, rFront, isPopUpOnCollide);
        }
    } else if (al::isEqualString(pName, "スーパーキノコ[真上出現]")) {
        appearItemPopUpAbove(mItemHolder->getKinokoSuper(), rTrans, rFront);
    } else if (al::isEqualString(pName, "スーパーベル") ||
               al::isEqualString(pName, "スーパーベル[強制出現]") ||
               al::isEqualString(pName, "スーパーベル[アイテムストック]")) {
        if (al::isEqualString(pName, "スーパーベル") && isPlayerMiniSensor(pSensor, isSingleMode)) {
            appearItem("スーパーキノコ", rTrans, rFront, pSensor, false, false);
            return;
        }

        bool isStock = al::isEqualString(pName, "スーパーベル[アイテムストック]");
        SuperBell* bell = mItemHolder->getSuperBell();

        if (isStock) {
            if (!isSingleMode) {
                appearItemTakeOut(bell, rTrans, rFront);
                return;
            }

            bell = appearItemHoming(bell, rTrans, rFront, pSensor);
        } else if (isTakeOut) {
            bell = appearItemTakeOut(bell, rTrans, rFront);
        } else {
            bell = appearItemPopUpFront(bell, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(bell, "PgAppear");
    } else if (al::isEqualString(pName, "スーパーベル[真上出現]")) {
        SuperBell* bell = appearItemPopUpAbove(mItemHolder->getSuperBell(), rTrans, rFront);
        al::startSe(bell, "PgAppear");
    } else if (al::isEqualString(pName, "ファイアフラワー") ||
               al::isEqualString(pName, "ファイアフラワー[アイテムストック]") ||
               al::isEqualString(pName, "ファイアフラワー[強制出現]")) {
        if (al::isEqualString(pName, "ファイアフラワー") &&
            isPlayerMiniSensor(pSensor, isSingleMode)) {
            appearItem("スーパーキノコ", rTrans, rFront, pSensor, false, false);
            return;
        }

        bool isStock = al::isEqualString(pName, "ファイアフラワー[アイテムストック]");
        FireFlower* flower = mItemHolder->getFireFlower();

        if (isStock) {
            if (isSingleMode) {
                appearItemHoming(flower, rTrans, rFront, pSensor);
            } else {
                appearItemTakeOut(flower, rTrans, rFront);
            }

            return;
        }

        if (isTakeOut) {
            flower = appearItemTakeOut(flower, rTrans, rFront);
        } else {
            flower = appearItemPopUpFront(flower, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(flower, "PgAppear");
    } else if (al::isEqualString(pName, "ファイアフラワー[真上出現]")) {
        FireFlower* flower = appearItemPopUpAbove(mItemHolder->getFireFlower(), rTrans, rFront);
        al::startSeOld(flower, "WsdSyItemAppear", nullptr);
    } else if (al::isEqualString(pName, "スーパーこのは") ||
               al::isEqualString(pName, "スーパーこのは[アイテムストック]")) {
        if (isPlayerMiniSensor(pSensor, isSingleMode)) {
            appearItem("スーパーキノコ", rTrans, rFront, pSensor, false, false);
            return;
        }

        bool isStock = al::isEqualString(pName, "スーパーこのは[アイテムストック]");
        SuperLeaf* leaf = mItemHolder->getSuperLeaf();

        if (isStock) {
            if (isSingleMode) {
                appearItemHoming(leaf, rTrans, rFront, pSensor);
            } else {
                appearItemTakeOut(leaf, rTrans, rFront);
            }

            return;
        }

        if (isTakeOut) {
            leaf = appearItemTakeOut(leaf, rTrans, rFront);
        } else {
            leaf = appearItemPopUpFront(leaf, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(leaf, "PgAppear");
    } else if (al::isEqualString(pName, "スーパーこのは[真上出現]")) {
        SuperLeaf* leaf = appearItemPopUpAbove(mItemHolder->getSuperLeaf(), rTrans, rFront);
        al::startSe(leaf, "PgAppear");
    } else if (al::isEqualString(pName, "ブーメランフラワー") ||
               al::isEqualString(pName, "ブーメランフラワー[アイテムストック]")) {
        if (isPlayerMiniSensor(pSensor, isSingleMode)) {
            appearItem("スーパーキノコ", rTrans, rFront, pSensor, false, false);
            return;
        }

        bool isStock = al::isEqualString(pName, "ブーメランフラワー[アイテムストック]");
        BoomerangFlower* flower = mItemHolder->getBoomerangFlower();

        if (isStock) {
            if (isSingleMode) {
                appearItemHoming(flower, rTrans, rFront, pSensor);
            } else {
                appearItemTakeOut(flower, rTrans, rFront);
            }

            return;
        }

        if (isTakeOut) {
            flower = appearItemTakeOut(flower, rTrans, rFront);
        } else {
            flower = appearItemPopUpFront(flower, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(flower, "PgAppear");
    } else if (al::isEqualString(pName, "ブーメランフラワー[真上出現]")) {
        BoomerangFlower* flower =
            appearItemPopUpAbove(mItemHolder->getBoomerangFlower(), rTrans, rFront);
        al::startSe(flower, "PgAppear");
    } else if (al::isEqualString(pName, "スーパースター")) {
        SuperStar* star = mItemHolder->getSuperStar();

        if (isTakeOut) {
            if (!tryPlaceItem(star, rTrans, rFront)) {
                star = nullptr;
            } else {
                if (isPopUpOnCollide) {
                    star->setPopUpOnCollide();
                }

                rc::updateMaterialCodeWater(star);
                star->appearItemTakeOut();
            }
        } else {
            star = appearItemPopUpFront(star, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(star, "PgAppear");
    } else if (al::isEqualString(pName, "SuperStar[Homing]")) {
        SuperStar* star = appearItemHoming(mItemHolder->getSuperStar(), rTrans, rFront, pSensor);
        al::startSe(star, "PgAppear");
    } else if (al::isEqualString(pName, "ボール")) {
        Ball* ball = mItemHolder->getBall();
        al::setTrans(ball, rTrans);
        sead::Quatf quat = sead::Quatf::unit;
        al::makeQuatFrontNoSupport(&quat, rFront);
        al::setQuat(ball, quat);
        al::resetPosition(ball, false);
        ball->appearPopUpFront();
        al::startSe(ball, "PgAppear");
    } else if (al::isEqualString(pName, "バクダン")) {
        Bomb* bomb = appearItemPopUpFront(mItemHolder->getBomb(), rTrans, rFront);
        al::startSe(bomb, "PgAppear");
    } else if (al::isEqualString(pName, "ダブルマリオ")) {
        DoubleMario* mario = appearItemPopUpFront(mItemHolder->getDoubleMario(), rTrans, rFront);
        al::startSe(mario, "PgAppear");
    } else if (al::isEqualString(pName, "巨大キノコ")) {
        KinokoBig* kinoko = appearItemPopUpFront(mItemHolder->getKinokoBig(), rTrans, rFront);
        al::startSe(kinoko, "PgAppear");
    } else if (al::isEqualString(pName, "無敵このは") ||
               al::isEqualString(pName, "無敵このは[アイテムストック]")) {
        bool isStock = al::isEqualString(pName, "無敵このは[アイテムストック]");
        AssistLeaf* leaf = mItemHolder->getAssistLeaf();

        if (isStock) {
            appearItemTakeOut(leaf, rTrans, rFront);
            return;
        }

        leaf = appearItemPopUpFront(leaf, rTrans, rFront);
        al::startSe(leaf, "PgAppear");
    } else if (al::isEqualString(pName, "まねきネコベル") ||
               al::isEqualString(pName, "まねきネコベル[アイテムストック]")) {
        if (isPlayerMiniSensor(pSensor, isSingleMode)) {
            appearItem("スーパーキノコ", rTrans, rFront, pSensor, false, false);
            return;
        }

        bool isStock = al::isEqualString(pName, "まねきネコベル[アイテムストック]");
        SuperBellSpecial* bell = mItemHolder->getSuperBellSpecial();

        if (isStock) {
            if (isSingleMode) {
                appearItemHoming(bell, rTrans, rFront, pSensor);
            } else {
                appearItemTakeOut(bell, rTrans, rFront);
            }

            return;
        }

        if (isTakeOut) {
            bell = appearItemTakeOut(bell, rTrans, rFront);
        } else {
            bell = appearItemPopUpFront(bell, rTrans, rFront, isPopUpOnCollide);
        }

        al::startSe(bell, "PgAppear");
    } else if (al::isEqualString(pName, "まねきネコベル[リフティング]")) {
        SuperBellSpecial* bell =
            appearItemPopUpAbove(mItemHolder->getSuperBellSpecial(), rTrans, rFront);
        al::startSe(bell, "PgAppear");
    } else if (al::isEqualString(pName, "DoorKey")) {
        DoorKey* key = appearItemPopUpFront(mItemHolder->getDoorKey(), rTrans, rFront);
        al::startSe(key, "PgAppear");
    } else if (al::isEqualString(pName, "GigaBell")) {
        appearItemPopUpFront(mItemHolder->getGigaBell(), rTrans, rFront);
    } else if (al::isEqualString(pName, "KinokoGiga")) {
        appearItemPopUpFront(mItemHolder->getKinokoGiga(), rTrans, rFront);
    } else if (al::isEqualString(pName, "WhiteBell")) {
        WhiteBell* bell = mItemHolder->getWhiteBell();

        if (isTakeOut) {
            appearItemTakeOut(bell, rTrans, rFront);
        } else {
            appearItemPopUpFront(bell, rTrans, rFront, isPopUpOnCollide);
        }
    } else if (al::isEqualString(pName, "WhiteBell[Homing]")) {
        appearItemHoming(mItemHolder->getWhiteBell(), rTrans, rFront, pSensor);
    }
}

namespace {

/**
 * @brief Put the power-up a player loses into the item stock.
 * @param pPlayer The player that collected the item.
 * @param pItem The collected item.
 * @param pSceneLayout HUD that shows the stocked item.
 * @param itemType Stock item type of the collected item.
 * @param isPlayer Whether the item was collected by a player.
 * @return True if the stock is full of this item in Bowser's Fury.
 */
bool tryStockItem(const al::LiveActor* pPlayer, const al::LiveActor* pItem,
                  SceneLayoutBase* pSceneLayout, s32 itemType, bool isPlayer) {
    al::startHitReactionGet(pItem);
    s32 stockType;

    if (isPlayer) {
        s32 figure = rc::getPlayerFigureType(pPlayer);

        if (rc::isPlayerNextFigureRequested(pPlayer)) {
            figure = rc::getPlayerNextFigureType(pPlayer);
        }

        bool isSingleMode = GameDataFunction::isSingleMode(pPlayer);

        if (rc::isPlayerGiant(al::getHitSensor(pPlayer, "Body")) ||
            (figure == 8 && !isSingleMode)) {
            stockType = itemType;
        } else {
            stockType = 0;

            switch (figure) {
            case 0:
                stockType = itemType == 1 ? 1 : 0;
                break;
            case 2:
                stockType = itemType == 1 ? 1 : 3;
                break;
            case 3:
            case 8:
            case 9:
                stockType = itemType == 1 ? 1 : 2;
                break;
            case 4:
                stockType = itemType == 1 ? 1 : 4;
                break;
            case 5:
                stockType = itemType == 1 ? 1 : 5;
                break;
            case 6:
                if (GameDataFunction::isSingleMode(pPlayer)) {
                    stockType = itemType == 1 ? 1 : 4;
                } else {
                    stockType = itemType == 8 && !GameDataFunction::isSingleMode(pPlayer) ?
                                    6 :
                                    itemType;
                }
                break;
            case 7:
                stockType = itemType == 1 ? 1 : 7;
                break;
            default:
                break;
            }
        }
    } else {
        stockType = itemType >= 1 && itemType <= 8 ? itemType : 0;
    }

    if (stockType == 0) {
        return false;
    }

    if (GameDataFunction::stockItem(pPlayer, stockType)) {
        if (pSceneLayout != nullptr) {
            pSceneLayout->stockItem(pItem, pPlayer, stockType);
        }

        return false;
    }

    return GameDataFunction::isSingleMode(pPlayer) &&
           SingleModeDataFunction::getStockItemCount(pPlayer, itemType) == 5;
}

/**
 * @brief Add collected coins, rewarding an item or a life for every hundred coins.
 * @param pItem The collected coin.
 * @param pSensor Sensor that collected the coin.
 * @param pName Score name of the coin.
 * @param pGameDataHolder The game data.
 * @param pPlayerHolder The scene's players.
 * @param num Number of coins.
 * @param pSceneLayout The scene's HUD.
 */
void addCoin(const al::LiveActor* pItem, al::HitSensor* pSensor, const char* pName,
             GameDataHolder* pGameDataHolder, const al::PlayerHolder* pPlayerHolder, s32 num,
             SceneLayoutBase* pSceneLayout) {
    rc::addScoreBySystem(pItem, pSensor, pName, 0);

    if (!GameDataFunction::addCoin(GameDataHolderWriter(pGameDataHolder), num)) {
        return;
    }

    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(pGameDataHolder))) {
        al::LiveActor* player =
            rc::tryFindPlayerFromInputPort(pPlayerHolder, al::getMainControllerPort(), false);

        if (player == nullptr) {
            return;
        }

        al::LiveActor* koopaJr = static_cast<PlayerActor*>(player)->getKoopaJr();
        al::LiveActor* target = koopaJr != nullptr ? koopaJr : player;

        if (SingleModeDataFunction::hasSeenCutscene(player, 1) && al::isPercentProbability(10.0f)) {
            al::ItemDirectorBase* director = player->getSceneInfo()->itemDirectorBase;

            if (director != nullptr) {
                director->appearItem("スーパースター", al::getTrans(player), rc::getPlayerFront(player),
                                     nullptr, false, false);
            }
        } else {
            const char* itemName = sHundredCoinItemNames[al::getRandom(0, 5)];

            if (SingleModeDataFunction::getUnlockedPhase(player) < 2 &&
                !SingleModeDataFunction::hasSeenCutscene(player, 1)) {
                target = player;
                itemName = "スーパーキノコ";
            }

            al::setAppearItemAttackerSensor(player, al::getHitSensor(target, "Body"));
            al::appearItemTiming(player, itemName, al::getTrans(target), rc::getPlayerFront(player));
        }

        al::startSe(player, "100CoinItemGet");

        if (pSensor != nullptr) {
            al::startHitReaction(al::getSensorHost(pSensor), "100CoinItemGet");
        } else {
            al::startHitReaction(player, "100CoinItemGet");
        }

        if (pSceneLayout != nullptr) {
            al::emitEffect(static_cast<SingleModeSceneLayout*>(pSceneLayout), "100CoinItemGet",
                           al::getTransPtr(target));
        }
    } else {
        const al::LiveActor* actor = pItem;

        if (pSensor != nullptr && alPlayerFunction::isPlayerActor(pSensor)) {
            actor = al::getSensorHost(pSensor);
        }

        ScoreFunction::popUpPlayerOneUp(actor, 100.0f);
        GameDataFunction::addPlayerLife(pItem, 1);
    }
}

/**
 * @brief Put the power-up of a player into the item stock when it rings the Giga Bell.
 * @param pItem The Giga Bell.
 * @param pPlayer The player.
 * @param pSceneLayout HUD that shows the stocked item.
 */
inline void stockPlayerFigureItem(const al::LiveActor* pItem, const al::LiveActor* pPlayer,
                                  SceneLayoutBase* pSceneLayout) {
    s32 itemType;

    switch (rc::getPlayerFigureType(pPlayer)) {
    case 2:
        itemType = 3;
        break;
    case 3:
        itemType = 2;
        break;
    case 4:
        itemType = 4;
        break;
    case 5:
        itemType = 5;
        break;
    case 7:
        itemType = 7;
        break;
    case 8:
        itemType = 2;
        break;
    default:
        return;
    }

    if (GameDataFunction::stockItem(pPlayer, itemType)) {
        pSceneLayout->stockItemSilent(pItem, pPlayer, itemType);
    }
}

}  // namespace

/**
 * @brief Give the player the reward of a collected item.
 * @param pItem The collected item.
 * @param pSensor Sensor that collected the item.
 * @param pName Placement name of the item.
 */
void ProjectItemDirector::acquirerItem(const al::LiveActor* pItem, al::HitSensor* pSensor,
                                       const char* pName) const {
    bool isPlayer = pSensor != nullptr && al::isSensorPlayer(pSensor);

    if (al::isEqualString(pName, "スーパーキノコ")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 1, isPlayer) &&
            isPlayer && !rc::isPlayerMini(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToSuperMario(pSensor);
        }
    } else if (al::isEqualString(pName, "スーパーベル")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 2, isPlayer) &&
            isPlayer && rc::isPlayerClimb(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToClimbMario(pSensor);
        }
    } else if (al::isEqualString(pName, "ファイアフラワー")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 3, isPlayer) &&
            isPlayer && rc::isPlayerFire(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToFireMario(pSensor);
        }
    } else if (al::isEqualString(pName, "スーパーこのは")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 4, isPlayer) &&
            isPlayer && rc::isPlayerRaccoonDog(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToRaccoonDogMario(pSensor);
        }
    } else if (al::isEqualString(pName, "ブーメランフラワー")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 5, isPlayer) &&
            isPlayer && rc::isPlayerBoomerang(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToBoomerangMario(pSensor);
        }
    } else if (al::isEqualString(pName, "1UPキノコ")) {
        const al::LiveActor* actor = pItem;

        if (pSensor != nullptr && alPlayerFunction::isPlayerActor(pSensor)) {
            actor = al::getSensorHost(pSensor);
        }

        ScoreFunction::popUpPlayerOneUp(actor, 100.0f);
        GameDataFunction::addPlayerLife(pItem, 1);
    } else if (al::isEqualString(pName, "コインx1")) {
        addCoin(pItem, pSensor, "コインx1", mGameDataHolder, mPlayerHolder, 1, mSceneLayout);
    } else if (al::isEqualString(pName, "コインx3")) {
        addCoin(pItem, pSensor, "コインx3", mGameDataHolder, mPlayerHolder, 3, mSceneLayout);
    } else if (al::isEqualString(pName, "コインx1[自動取得]")) {
        addCoin(pItem, pSensor, "コインx1", mGameDataHolder, mPlayerHolder, 1, mSceneLayout);
    } else if (al::isEqualString(pName, "コインx5[自動取得]")) {
        addCoin(pItem, pSensor, "コインx5", mGameDataHolder, mPlayerHolder, 5, mSceneLayout);
    } else if (al::isEqualString(pName, "コインx10[自動取得]")) {
        addCoin(pItem, pSensor, "コインx10", mGameDataHolder, mPlayerHolder, 10, mSceneLayout);
    } else if (al::isEqualString(pName, "コインx50")) {
        addCoin(pItem, pSensor, "コインx50", mGameDataHolder, mPlayerHolder, 50, mSceneLayout);
    } else if (al::isEqualString(pName, "巨大キノコ")) {
        if (isPlayer && !rc::isPlayerRaccoonDogWhite(pSensor) && !rc::isPlayerClimbWhite(pSensor)) {
            rc::tryChangeToGiantMario(pSensor);
        }
    } else if (al::isEqualString(pName, "無敵このは")) {
        tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 6, isPlayer);

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToRaccoonDogWhiteMario(pSensor);
        }
    } else if (al::isEqualString(pName, "まねきネコベル")) {
        if (tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 7, isPlayer) &&
            isPlayer && rc::isPlayerClimbSpecial(pSensor)) {
            static_cast<SingleModeSceneLayout*>(mSceneLayout)->spawnItemStockCoins(10, pSensor);
        }

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToClimbMarioSpecial(pSensor);
        }
    } else if (al::isEqualString(pName, "KinokoGiga")) {
        rc::tryChangeToGigaMario(pItem, pSensor);
    } else if (al::isEqualString(pName, "GigaBell")) {
        stockPlayerFigureItem(pItem, al::getSensorHost(pSensor), mSceneLayout);
    } else if (al::isEqualString(pName, "WhiteBell")) {
        tryStockItem(al::getSensorHost(pSensor), pItem, mSceneLayout, 8, isPlayer);

        if (isPlayer && !rc::isPlayerGiant(pSensor)) {
            rc::tryChangeToClimbWhiteMario(pSensor);
        }
    }
}

/**
 * @brief Declare an item that the scene's placement can spawn.
 * @param pName Placement name of the item.
 * @param rInfo Actor init info.
 */
void ProjectItemDirector::declareItem(const char* pName, const al::ActorInitInfo& rInfo) {
    mItemHolder->declareItem(pName, rInfo);
}

/**
 * @brief Finish the initialization (nothing to do).
 */
void ProjectItemDirector::endInit() {}

/**
 * @brief Spawn the top item of the item stock for a player.
 * @param port Controller port of the player.
 * @param itemType Stock item type to use in Bowser's Fury.
 * @return True if the item was used.
 */
bool ProjectItemDirector::useStockItem(s32 port, s32 itemType) {
    bool isSingleMode = GameDataFunction::isSingleMode(GameDataHolderAccessor(mGameDataHolder));
    bool isKoopaJrPort = isSingleMode && al::getMainControllerPort() != port;

    if (isKoopaJrPort) {
        port = al::getMainControllerPort();
    }

    al::LiveActor* player = rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);

    if (player == nullptr || rc::isPlayerDeadOrBubble(player)) {
        return false;
    }

    s32 type = itemType;

    if (rc::isPlayerBinded(player) || rc::isPlayerGiant(player)) {
        if (!isSingleMode) {
            return false;
        }

        if (rc::isPlayerBinded(player) && !rc::isPlayerOnRaidon(player)) {
            return false;
        }
    } else if (!isSingleMode) {
        type = GameDataFunction::getTopItem(GameDataHolderAccessor(mGameDataHolder));
    }

    const char* itemName;

    switch (type) {
    case 1:
        itemName = "スーパーキノコ";
        break;
    case 2:
        itemName = "スーパーベル";
        break;
    case 3:
        itemName = "ファイアフラワー";
        break;
    case 4:
        itemName = "スーパーこのは";
        break;
    case 5:
        itemName = "ブーメランフラワー";
        break;
    case 6:
        itemName = "無敵このは";
        break;
    case 7:
        itemName = "まねきネコベル";
        break;
    case 8:
        itemName = "スーパーベル";
        break;
    default:
        return false;
    }

    if (isSingleMode) {
        auto* koopaJr =
            static_cast<PlayerKoopaJr*>(static_cast<PlayerActor*>(player)->getKoopaJr());

        if (koopaJr == nullptr) {
            return true;
        }

        if ((isKoopaJrPort || !koopaJr->isAIMovement()) &&
            koopaJr->tryThrowStockItem(itemType, itemName, player)) {
            return true;
        }

        if (koopaJr->isThrowingItem()) {
            return false;
        }

        al::setAppearItemAttackerSensor(player, al::getHitSensor(player, "Body"));
    }

    al::appearItemTiming(player, itemName, al::getTrans(player), rc::getPlayerFront(player));

    if (!isSingleMode) {
        GameDataFunction::useStockItem(GameDataHolderWriter(mGameDataHolder));
    }

    return true;
}

/**
 * @brief Enter demo mode (nothing to do).
 */
void ProjectItemDirector::setDemoMode() {}

/**
 * @brief Leave demo mode (nothing to do).
 */
void ProjectItemDirector::resetDemoMode() {}
