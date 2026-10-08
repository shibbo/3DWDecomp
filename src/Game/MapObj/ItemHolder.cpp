#include "MapObj/ItemHolder.hpp"

#include <attributes.h>

#include "Enemy/Bomb.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "MapObj/AssistLeaf.hpp"
#include "MapObj/Ball.hpp"
#include "MapObj/BoomerangFlower.hpp"
#include "MapObj/CoinBlow.hpp"
#include "MapObj/CoinCountUp.hpp"
#include "MapObj/DoorKey.hpp"
#include "MapObj/DoubleMario.hpp"
#include "MapObj/FireFlower.hpp"
#include "MapObj/Fury/GigaBell.hpp"
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
#include "Project/Base/StringUtil.hpp"

namespace {
/**
 * @brief Creates a stock group of item actors and leaves every created actor dead.
 * @param pGroupName The name of the actor group.
 * @param maxNum The number of actors to create.
 * @param rInfo The actor init info.
 * @param args The arguments forwarded to the item constructor.
 * @return The created group.
 */
template <typename T, typename... Args>
ALWAYS_INLINE al::DeriveActorGroup<T>* createItemGroup(const char* pGroupName, s32 maxNum,
                                                       const al::ActorInitInfo& rInfo,
                                                       Args... args) {
    auto* group = new al::DeriveActorGroup<T>(pGroupName, maxNum);

    for (s32 i = 0; i < group->getMaxActorCount(); i++) {
        auto* actor = new T(args...);
        al::initCreateActorNoPlacementInfo(actor, rInfo);
        group->registerActor(actor);
    }

    for (s32 i = 0; i < group->getActorCount(); i++) {
        group->getActor(i)->makeActorDead();
    }

    return group;
}

/**
 * @brief Creates a group of coin actors that are not bound to a view.
 * @param pGroup Receives the created group before its coins are created.
 * @param pGroupName The name of the actor group.
 * @param maxNum The number of coins to create.
 * @param pCoinName The name of each coin.
 * @param rInfo The actor init info.
 */
template <typename T>
ALWAYS_INLINE void createCoinGroup(al::DeriveActorGroup<T>** pGroup, const char* pGroupName,
                                   s32 maxNum, const char* pCoinName,
                                   const al::ActorInitInfo& rInfo) {
    auto* group = new al::DeriveActorGroup<T>(pGroupName, maxNum);
    *pGroup = group;

    for (s32 i = 0; i < group->getMaxActorCount(); i++) {
        auto* coin = new T(pCoinName);
        al::initCreateActorNoPlacementInfoNoViewId(coin, rInfo);
        group->registerActor(coin);
    }
}

/**
 * @brief Gets a dead actor of a group, killing the first actor if every actor is alive.
 * @param pGroup The actor group.
 * @return The actor to use.
 */
template <typename T>
ALWAYS_INLINE T* getOrRecycleActor(const al::DeriveActorGroup<T>* pGroup) {
    al::LiveActor* actor = pGroup->tryFindDeadActor();
    if (actor == nullptr) {
        actor = pGroup->getActor(0);
        actor->makeActorDead();
    }

    return static_cast<T*>(actor);
}
}  // namespace

/**
 * @brief Constructs the item holder and creates the coin and stock item pools.
 * @param rInfo The actor init info.
 * @param isSingleMode Whether the holder belongs to the single mode (Bowser's Fury) scene.
 */
ItemHolder::ItemHolder(const al::ActorInitInfo& rInfo, bool isSingleMode)
    : mIsSingleMode(isSingleMode) {
    initGroupCoinCountUp(rInfo);

    if (!isSingleMode) {
        initGroupCoinBlow(rInfo);
        declareStockItem(rInfo);
    }
}

/**
 * @brief Creates the pool of counting coins.
 * @param rInfo The actor init info.
 */
void ItemHolder::initGroupCoinCountUp(const al::ActorInitInfo& rInfo) {
    createCoinGroup(&mCoinCountUpGroup, "カウントアップコインリスト", 30, "カウントアップコイン",
                    rInfo);
}

/**
 * @brief Creates the pool of blown out coins.
 * @param rInfo The actor init info.
 */
void ItemHolder::initGroupCoinBlow(const al::ActorInitInfo& rInfo) {
    createCoinGroup(&mCoinBlowGroup, "吹き出しコインリスト", 40, "吹き出しコイン", rInfo);
}

/**
 * @brief Declares the items the player can keep in the item stock.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareStockItem(const al::ActorInitInfo& rInfo) {
    declareKinokoSuper(rInfo);
    declareSuperBell(rInfo);
    declareFireFlower(rInfo);
    declareSuperLeaf(rInfo);
    declareBoomerangFlower(rInfo);
}

/**
 * @brief Declares the item pool matching an item name.
 * @param pName The item name.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareItem(const char* pName, const al::ActorInitInfo& rInfo) {
    if (mIsSingleMode) {
        return;
    }

    if (al::isEqualSubString(pName, "コイン")) {
        return;
    }

    if (al::isEqualSubString(pName, "1UPキノコ")) {
        declareKinokoOneUp(rInfo);
    } else if (al::isEqualSubString(pName, "スーパーキノコ")) {
        declareKinokoSuper(rInfo);
    } else if (al::isEqualSubString(pName, "KinokoTreasure")) {
        declareKinokoTreasure(rInfo);
    } else if (al::isEqualSubString(pName, "まねきネコベル")) {
        declareSuperBellSpecial(rInfo);
    } else if (al::isEqualSubString(pName, "スーパーベル")) {
        declareSuperBell(rInfo);
    } else if (al::isEqualSubString(pName, "ファイアフラワー")) {
        declareFireFlower(rInfo);
    } else if (al::isEqualSubString(pName, "スーパーこのは")) {
        declareSuperLeaf(rInfo);
    } else if (al::isEqualSubString(pName, "ブーメランフラワー")) {
        declareBoomerangFlower(rInfo);
    } else if (al::isEqualString(pName, "スーパースター")) {
        declareSuperStar(rInfo);
    } else if (al::isEqualString(pName, "ボール")) {
        declareBall(rInfo);
    } else if (al::isEqualString(pName, "バクダン")) {
        declareBomb(rInfo);
    } else if (al::isEqualString(pName, "ダブルマリオ")) {
        declareDoubleMario(rInfo);
    } else if (al::isEqualString(pName, "巨大キノコ")) {
        declareKinokoBig(rInfo);
    } else if (al::isEqualSubString(pName, "無敵このは")) {
        declareAssistLeaf(rInfo);
    } else if (al::isEqualSubString(pName, "DoorKey")) {
        declareDoorKey(rInfo);
    } else if (al::isEqualSubString(pName, "GigaBell")) {
        declareGigaBell(rInfo);
    } else if (al::isEqualSubString(pName, "KinokoGiga")) {
        declareKinokoGiga(rInfo);
    } else if (al::isEqualSubString(pName, "WhiteBell")) {
        declareWhiteBell(rInfo);
    }
}

/**
 * @brief Creates the 1-Up Mushroom pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareKinokoOneUp(const al::ActorInitInfo& rInfo) {
    if (mKinokoOneUpGroup != nullptr) {
        return;
    }

    mKinokoOneUpGroup =
        createItemGroup<KinokoOneUp>("1UPキノコリスト", 16, rInfo, "1UPキノコ", nullptr);
}

/**
 * @brief Creates the Super Mushroom pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareKinokoSuper(const al::ActorInitInfo& rInfo) {
    if (mKinokoSuperGroup != nullptr) {
        return;
    }

    mKinokoSuperGroup =
        createItemGroup<KinokoSuper>("スーパーキノコリスト", 16, rInfo, "スーパーキノコ", nullptr);
}

/**
 * @brief Creates the treasure mushroom pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareKinokoTreasure(const al::ActorInitInfo& rInfo) {
    if (mKinokoTreasureGroup != nullptr) {
        return;
    }

    mKinokoTreasureGroup =
        createItemGroup<KinokoTreasure>("KinokoTreasure", 16, rInfo, "KinokoTreasure", nullptr);
}

/**
 * @brief Creates the Lucky Bell pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareSuperBellSpecial(const al::ActorInitInfo& rInfo) {
    if (mSuperBellSpecialGroup != nullptr) {
        return;
    }

    mSuperBellSpecialGroup = createItemGroup<SuperBellSpecial>(
        "まねきネコベルリスト", 16, rInfo, "まねきネコベル", nullptr);
}

/**
 * @brief Creates the Super Bell pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareSuperBell(const al::ActorInitInfo& rInfo) {
    if (mSuperBellGroup != nullptr) {
        return;
    }

    mSuperBellGroup =
        createItemGroup<SuperBell>("スーパーベルリスト", 16, rInfo, "スーパーベル", nullptr);
}

/**
 * @brief Creates the Fire Flower pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareFireFlower(const al::ActorInitInfo& rInfo) {
    if (mFireFlowerGroup != nullptr) {
        return;
    }

    mFireFlowerGroup = createItemGroup<FireFlower>("ファイアフラワーリスト", 16, rInfo,
                                                   "ファイアフラワー", nullptr);
}

/**
 * @brief Creates the Super Leaf pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareSuperLeaf(const al::ActorInitInfo& rInfo) {
    if (mSuperLeafGroup != nullptr) {
        return;
    }

    mSuperLeafGroup =
        createItemGroup<SuperLeaf>("スーパーこのはリスト", 16, rInfo, "スーパーこのは", nullptr);
}

/**
 * @brief Creates the Boomerang Flower pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareBoomerangFlower(const al::ActorInitInfo& rInfo) {
    if (mBoomerangFlowerGroup != nullptr) {
        return;
    }

    mBoomerangFlowerGroup = createItemGroup<BoomerangFlower>(
        "ブーメランフラワーリスト", 16, rInfo, "ブーメランフラワー", nullptr, false);
}

/**
 * @brief Creates the Super Star pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareSuperStar(const al::ActorInitInfo& rInfo) {
    if (mSuperStarGroup != nullptr) {
        return;
    }

    mSuperStarGroup =
        createItemGroup<SuperStar>("スーパースターリスト", 16, rInfo, "スーパースター", nullptr);
}

/**
 * @brief Creates the ball pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareBall(const al::ActorInitInfo& rInfo) {
    if (mBallGroup != nullptr) {
        return;
    }

    mBallGroup = createItemGroup<Ball>("ボールリスト", 16, rInfo, "ボール");
}

/**
 * @brief Creates the bomb pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareBomb(const al::ActorInitInfo& rInfo) {
    if (mBombGroup != nullptr) {
        return;
    }

    mBombGroup = createItemGroup<Bomb>("バクダンリスト", 30, rInfo, "バクダン", false);
}

/**
 * @brief Creates the Double Cherry pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareDoubleMario(const al::ActorInitInfo& rInfo) {
    if (mDoubleMarioGroup != nullptr) {
        return;
    }

    mDoubleMarioGroup = createItemGroup<DoubleMario>(
        "ダブルマリオアイテムリスト", 10, rInfo, "ダブルマリオアイテム", nullptr, false);
}

/**
 * @brief Creates the Mega Mushroom pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareKinokoBig(const al::ActorInitInfo& rInfo) {
    if (mKinokoBigGroup != nullptr) {
        return;
    }

    mKinokoBigGroup = createItemGroup<KinokoBig>("巨大キノコリスト", 16, rInfo, "巨大キノコ");
}

/**
 * @brief Creates the Invincibility Leaf pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareAssistLeaf(const al::ActorInitInfo& rInfo) {
    if (mAssistLeafGroup != nullptr) {
        return;
    }

    mAssistLeafGroup = createItemGroup<AssistLeaf>("無敵このはリスト", 16, rInfo, "無敵このは");
}

/**
 * @brief Creates the door key pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareDoorKey(const al::ActorInitInfo& rInfo) {
    if (mDoorKeyGroup != nullptr) {
        return;
    }

    mDoorKeyGroup = createItemGroup<DoorKey>("DoorKey", 16, rInfo, "DoorKey");
}

/**
 * @brief Creates the Giga Bell pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareGigaBell(const al::ActorInitInfo& rInfo) {
    if (mGigaBellGroup != nullptr) {
        return;
    }

    mGigaBellGroup = createItemGroup<GigaBell>("GigaBell", 5, rInfo, "GigaBell");
}

/**
 * @brief Creates the giant mushroom pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareKinokoGiga(const al::ActorInitInfo& rInfo) {
    if (mKinokoGigaGroup != nullptr) {
        return;
    }

    mKinokoGigaGroup = createItemGroup<KinokoGiga>("KinokoGiga", 5, rInfo, "KinokoGiga");
}

/**
 * @brief Creates the white bell pool if it does not exist yet.
 * @param rInfo The actor init info.
 */
void ItemHolder::declareWhiteBell(const al::ActorInitInfo& rInfo) {
    if (mWhiteBellGroup != nullptr) {
        return;
    }

    mWhiteBellGroup = createItemGroup<WhiteBell>("WhiteBell", 5, rInfo, "WhiteBell", nullptr);
}

/**
 * @brief Gets a blown out coin to spawn.
 * @return The coin.
 */
CoinBlow* ItemHolder::getCoinBlow() const {
    return getOrRecycleActor(mCoinBlowGroup);
}

/**
 * @brief Gets a counting coin to spawn.
 * @return The coin.
 */
CoinCountUp* ItemHolder::getCoinCountUp() const {
    return getOrRecycleActor(mCoinCountUpGroup);
}

/**
 * @brief Gets a 1-Up Mushroom to spawn.
 * @return The mushroom.
 */
KinokoOneUp* ItemHolder::getKinokoOneUp() const {
    return getOrRecycleActor(mKinokoOneUpGroup);
}

/**
 * @brief Gets a Super Mushroom to spawn.
 * @return The mushroom.
 */
KinokoSuper* ItemHolder::getKinokoSuper() const {
    return getOrRecycleActor(mKinokoSuperGroup);
}

/**
 * @brief Gets a treasure mushroom to spawn.
 * @return The mushroom.
 */
KinokoTreasure* ItemHolder::getKinokoTreasure() const {
    return getOrRecycleActor(mKinokoTreasureGroup);
}

/**
 * @brief Gets a Super Bell to spawn.
 * @return The bell.
 */
SuperBell* ItemHolder::getSuperBell() const {
    return getOrRecycleActor(mSuperBellGroup);
}

/**
 * @brief Gets a Fire Flower to spawn.
 * @return The flower.
 */
FireFlower* ItemHolder::getFireFlower() const {
    return getOrRecycleActor(mFireFlowerGroup);
}

/**
 * @brief Gets a Boomerang Flower to spawn.
 * @return The flower.
 */
BoomerangFlower* ItemHolder::getBoomerangFlower() const {
    return getOrRecycleActor(mBoomerangFlowerGroup);
}

/**
 * @brief Gets a Super Leaf to spawn.
 * @return The leaf.
 */
SuperLeaf* ItemHolder::getSuperLeaf() const {
    return getOrRecycleActor(mSuperLeafGroup);
}

/**
 * @brief Gets a Super Star to spawn.
 * @return The star.
 */
SuperStar* ItemHolder::getSuperStar() const {
    return getOrRecycleActor(mSuperStarGroup);
}

/**
 * @brief Gets a ball to spawn.
 * @return The ball.
 */
Ball* ItemHolder::getBall() const {
    return getOrRecycleActor(mBallGroup);
}

/**
 * @brief Gets a bomb to spawn.
 * @return The bomb.
 */
Bomb* ItemHolder::getBomb() const {
    return getOrRecycleActor(mBombGroup);
}

/**
 * @brief Gets a Double Cherry to spawn.
 * @return The cherry.
 */
DoubleMario* ItemHolder::getDoubleMario() const {
    return getOrRecycleActor(mDoubleMarioGroup);
}

/**
 * @brief Gets a Mega Mushroom to spawn.
 * @return The mushroom.
 */
KinokoBig* ItemHolder::getKinokoBig() const {
    return getOrRecycleActor(mKinokoBigGroup);
}

/**
 * @brief Gets an Invincibility Leaf to spawn.
 * @return The leaf.
 */
AssistLeaf* ItemHolder::getAssistLeaf() const {
    return getOrRecycleActor(mAssistLeafGroup);
}

/**
 * @brief Gets a Lucky Bell to spawn.
 * @return The bell.
 */
SuperBellSpecial* ItemHolder::getSuperBellSpecial() const {
    return getOrRecycleActor(mSuperBellSpecialGroup);
}

/**
 * @brief Gets a door key to spawn.
 * @return The key.
 */
DoorKey* ItemHolder::getDoorKey() const {
    return getOrRecycleActor(mDoorKeyGroup);
}

/**
 * @brief Gets a Giga Bell to spawn.
 * @return The bell.
 */
GigaBell* ItemHolder::getGigaBell() const {
    return getOrRecycleActor(mGigaBellGroup);
}

/**
 * @brief Gets a giant mushroom to spawn.
 * @return The mushroom.
 */
KinokoGiga* ItemHolder::getKinokoGiga() const {
    return getOrRecycleActor(mKinokoGigaGroup);
}

/**
 * @brief Gets a white bell to spawn.
 * @return The bell.
 */
WhiteBell* ItemHolder::getWhiteBell() const {
    return getOrRecycleActor(mWhiteBellGroup);
}
