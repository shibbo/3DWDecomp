#include "MapObj/BlockStateItem.hpp"

#include <attributes.h>

#include "Library/Actor/ComboCounter.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Item/AcquireItemFunc.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/BlockStateCoinTen.hpp"
#include "MapObj/BlockStateHeadgear.hpp"
#include "MapObj/SnowCover.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/PlayerStocker.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
/// A nerve whose execute function is shared with another nerve.
#define BLOCK_STATE_ITEM_NERVE_DECL_(Action, ActionFunc)                                           \
    class BlockStateItemNrv##Action : public al::Nerve {                                           \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<BlockStateItem>()->exe##ActionFunc();                               \
        }                                                                                          \
    };

NERVE_DECL(BlockStateItem, Headgear)
NERVE_DECL(BlockStateItem, Coin10)
NERVE_DECL(BlockStateItem, Wait)
NERVE_DECL(BlockStateItem, AppearItem)
BLOCK_STATE_ITEM_NERVE_DECL_(AppearItemHipDrop, AppearItem)
NERVE_DECL(BlockStateItem, End)
NERVES_MAKE_NOSTRUCT(BlockStateItem, Headgear, Coin10, Wait, AppearItem, AppearItemHipDrop, End)

typedef al::FunctorV0M<BlockStateItem*, void (BlockStateItem::*)()> BlockStateItemFunctor;

/// Spread angle of the per-player items, indexed by the number of players minus one.
const f32 sAppearItemSpreadDegree[] = {0.0f, 60.0f, 80.0f, 100.0f};

/// Spread angle of the per-player items of item type 18, indexed by the number of players minus one.
const f32 sAppearItemSpreadDegreeWide[] = {0.0f, 60.0f, 120.0f, 180.0f};

void validateHitSensorsLong(al::LiveActor* pActor, f32 offsetY, f32 radius);
}  // namespace

/**
 * @brief Construct the item state of a block.
 * @param pHost The block owning this state.
 * @param rInfo The block's init info.
 * @param isLong Whether the block is a long block (three punch sensors, coins on both sides).
 * @param isTransparent Whether the block is a transparent block (single item for all players).
 * @param isCreateBoxCoin Whether a coin box may appear once all ten coins are out.
 * @param isAssist Whether the block is an assist block (no empty block left behind).
 * @param isForceAppearBoxCoin Whether the coin box always appears once all ten coins are out.
 */
BlockStateItem::BlockStateItem(al::LiveActor* pHost, const al::ActorInitInfo& rInfo, bool isLong,
                               bool isTransparent, bool isCreateBoxCoin, bool isAssist,
                               bool isForceAppearBoxCoin)
    : al::ActorStateBase("アイテム出現ステート", pHost), mIsLong(isLong),
      mIsTransparent(isTransparent), mIsAssist(isAssist) {
    mComboCounter = new al::ComboCounter();
    mItemType = rc::getItemType(rInfo);
    bool isSingleMode = al::isSingleMode(rInfo);
    const char* snowCoverSuffix = isSingleMode && !isLong ? "SM" : nullptr;
    const char* snowCoverName = isLong ? "BlockSnowCoverLong" : "BlockSnowCover";
    mSnowCover = SnowCoverFunction::tryCreateSnowCover(mHostActor, rInfo, snowCoverName, true,
                                                       snowCoverSuffix);

    if (isUseStateHeadgear()) {
        initNerve(&NrvBlockStateItemHeadgear, 1);
        mStateHeadgear = new BlockStateHeadgear(mHostActor, rInfo, mItemType);
        al::initNerveState(this, mStateHeadgear, &NrvBlockStateItemHeadgear, "被り物");

        if (!mIsTransparent) {
            al::invalidateHitSensor(mHostActor, "UpperPunch");
        }

        return;
    }

    al::calcFrontDir(&mAppearDir, mHostActor);
    const char* appearAction = "Dummy";

    if (al::tryGetStringArg(&appearAction, rInfo, "ItemAppearAction")) {
        f32 degree = 0.0f;

        if (al::isEqualString(appearAction, "Above")) {
            mIsPopUp = false;
        } else if (al::isEqualString(appearAction, "PopUpFront")) {
            mIsPopUp = true;
        } else if (al::isEqualString(appearAction, "PopUpRight")) {
            degree = 90.0f;
            mIsPopUp = true;
        } else if (al::isEqualString(appearAction, "PopUpLeft")) {
            degree = 270.0f;
            mIsPopUp = true;
        } else if (al::isEqualString(appearAction, "PopUpBack")) {
            degree = 180.0f;
            mIsPopUp = true;
        }

        sead::Vector3f up = -al::getGravity(mHostActor);
        al::rotateVectorDegree(&mAppearDir, up, degree);
    }

    s32 userNum;

    switch (mItemType) {
    case 0:
    case 1:
    case 2:
    case 3:
        mIsPopUp = false;
        mIsItemPerPlayer = false;
        userNum = 1;
        break;
    case 4:
    case 10:
        mIsItemPerPlayer = false;
        userNum = 1;
        break;
    default:
        mIsItemPerPlayer = true;

        if (mItemType == 12) {
            mPlayerNum = 4;
            userNum = 4;
        } else {
            userNum = rc::getControlUserNumMax();

            if (mIsTransparent) {
                userNum = 1;
            }
        }

        break;
    }

    mIsUserItemAppeared.tryAllocBuffer(userNum, nullptr);

    for (s32 i = 0; i < mIsUserItemAppeared.size(); i++) {
        mIsUserItemAppeared[i] = false;
    }

    mHostActor->initItemKeeper(3);
    rc::addItemByHostInfo(mHostActor, rInfo, "通常アイテム", nullptr);

    if (mIsItemPerPlayer && !mIsAssist) {
        if (mItemType == 17) {
            al::addItem(mHostActor, rInfo, "コインx1[自動取得]", "ダブルマリオ用コイン", nullptr,
                        false);
        } else {
            al::addItem(mHostActor, rInfo, "スーパーキノコ", "スーパーキノコ", nullptr, false);
        }
    }

    if (mIsLong) {
        if (isUseStateCoinTen()) {
            al::addItem(mHostActor, rInfo, "コインx1[自動取得＆高速出現]", "ロングブロック用コイン",
                        nullptr, false);
        } else {
            al::addItem(mHostActor, rInfo, "コインx1[自動取得]", "ロングブロック用コイン", nullptr,
                        false);
        }
    }

    if (mItemType == 2) {
        al::addItem(mHostActor, rInfo, "コインx1[飛び散り10コイン]", "コインx1[飛び散り10コイン]",
                    nullptr, false);
    }

    const char* blockEmptyName = isLong ? "BlockEmptyLong" : "BlockEmpty";
    mBlockEmpty = new BlockEmpty("空ブロック", blockEmptyName);
    al::initCreateActorWithPlacementInfo(mBlockEmpty, rInfo);
    mBlockEmpty->makeActorDead();
    al::syncSensorScaleY(mHostActor);

    if (mBlockEmpty != nullptr) {
        al::setScale(mBlockEmpty, al::getScale(mHostActor));
        al::syncSensorScaleY(mBlockEmpty);
    }

    al::tryGetArg(&mIsForceChangeItem, rInfo, "IsForceChangeItem");
    const char* suffix = rc::getBlockSuffixName(rInfo, false);

    if (suffix != nullptr) {
        mIsWallSide = al::isEqualString("WallSide", suffix);
    }

    if (isUseStateCoinTen()) {
        initNerve(&NrvBlockStateItemCoin10, 1);
        bool isAppearBoxCoin = true;

        if (!isForceAppearBoxCoin) {
            isAppearBoxCoin = mItemType != 3 && !mIsLong && isCreateBoxCoin;
        }

        mStateCoinTen = new BlockStateCoinTen(mHostActor, rInfo, isAppearBoxCoin);
        al::initNerveState(this, mStateCoinTen, &NrvBlockStateItemCoin10, "10コイン");

        if (mItemType == 2) {
            mStateCoinTen->setAppearCoinCallBack(
                BlockStateItemFunctor(this, &BlockStateItem::appearCoinRandom10));
        } else {
            mStateCoinTen->setAppearCoinCallBack(
                BlockStateItemFunctor(this, &BlockStateItem::appearCoin10));
        }

        if (mItemType == 3) {
            mStateCoinTen->setCoinMax(-1);
        }
    } else {
        initNerve(&NrvBlockStateItemWait, 0);
    }

    if (!mIsTransparent) {
        al::LiveActor* host = mHostActor;

        if (mIsLong) {
            al::invalidateHitSensor(host, "UpperPunchCenter");
            al::invalidateHitSensor(host, "UpperPunchLeft");
            al::invalidateHitSensor(host, "UpperPunchRight");
        } else {
            al::invalidateHitSensor(host, "UpperPunch");
        }
    }
}

/**
 * @brief Check whether the block holds a headgear box (propeller or cannon box).
 * @return True if the item type is a headgear box.
 */
bool BlockStateItem::isUseStateHeadgear() const {
    return mItemType == 19 || mItemType == 20;
}

/**
 * @brief Check the item type of the block.
 * @param itemType The item type to compare with.
 * @return True if the block holds this item type.
 */
bool BlockStateItem::isItemType(int itemType) const {
    return mItemType == itemType;
}

/**
 * @brief Check whether the block gives out coins over several hits.
 * @return True if the item type is one of the ten coin types.
 */
bool BlockStateItem::isUseStateCoinTen() const {
    return mItemType >= 1 && mItemType <= 3;
}

/**
 * @brief Make one coin of a ten coin block fly out in a random direction.
 */
void BlockStateItem::appearCoinRandom10() {
    sead::Vector3f dir = sead::Vector3f::ey;
    f32 randomX = al::getRandom(-3.5f, 3.5f);
    f32 randomZ = al::getRandom(mIsWallSide ? 0.0f : -3.5f, 3.5f);
    dir += sead::Vector3f(randomX, 0.0f, randomZ);

    if (al::normalizeOrZero(&dir)) {
        dir = sead::Vector3f::ey;
    }

    al::LiveActor* host = mHostActor;
    sead::Vector3f pos = al::getTrans(host) + sead::Vector3f(0.0f, 150.0f, 0.0f);
    sead::Vector3f velocity = dir * 1.5f;
    al::appearItemTiming(host, "コインx1[飛び散り10コイン]", pos, velocity, mAppearSensor, false);
}

/**
 * @brief Make one coin of a ten coin block appear above the block.
 */
void BlockStateItem::appearCoin10() {
    al::LiveActor* host = mHostActor;
    sead::Vector3f pos = al::getTrans(host) + sead::Vector3f(0.0f, 150.0f, 0.0f);
    al::appearItemTiming(host, "通常アイテム", pos, sead::Vector3f::ez, mAppearSensor, false);
    tryExecAppearItemLong(mAppearSensor);
}

/**
 * @brief Reset the state so the block can give out its item again.
 * @return False if the headgear box could not be reset.
 */
bool BlockStateItem::reset() {
    appear();

    if (isUseStateCoinTen()) {
        al::setNerve(this, &NrvBlockStateItemCoin10);
        mStateCoinTen->appear();
    } else if (mStateHeadgear != nullptr) {
        if (!mStateHeadgear->reset()) {
            return false;
        }

        al::setNerve(this, &NrvBlockStateItemHeadgear);
    } else {
        al::setNerve(this, &NrvBlockStateItemWait);
    }

    if (mBlockEmpty != nullptr && al::isAlive(mBlockEmpty)) {
        mBlockEmpty->kill();
    }

    if (mSnowCover != nullptr) {
        mSnowCover->respawn();
    }

    return true;
}

/**
 * @brief Forward the host block's attack to the objects above or below it.
 * @param pSelf The host block's sensor.
 * @param pOther The touched sensor.
 */
void BlockStateItem::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    trySendMsgToUpperLowerObj(pOther, pSelf);
}

/**
 * @brief Send the block punch message to the object on top of or below the block while it reacts.
 * @param pReceiver The touched sensor.
 * @param pSender The host block's sensor.
 * @return True if the message was received.
 */
bool BlockStateItem::trySendMsgToUpperLowerObj(al::HitSensor* pReceiver, al::HitSensor* pSender) {
    if (al::isNerve(this, &NrvBlockStateItemCoin10)) {
        if (!mStateCoinTen->isValidUpperPunch(5)) {
            return false;
        }
    } else if (al::isNerve(this, &NrvBlockStateItemHeadgear)) {
        if (!mStateHeadgear->isAppearHeadgear()) {
            return false;
        }
    } else if (al::isNerve(this, &NrvBlockStateItemAppearItem) ||
               al::isNerve(this, &NrvBlockStateItemAppearItemHipDrop)) {
        if (!al::isLessStep(this, 5)) {
            return false;
        }

        if (mIsDrcPunch) {
            return rc::trySendMsgBlockToUpperObj(pReceiver, pSender, 0, mComboCounter);
        }
    } else {
        return false;
    }

    const sead::Vector3f& offset =
        al::getSensorFollowPosOffset(mHostActor, mIsLong ? "UpperPunchCenter" : "UpperPunch");

    if (offset.y == 100.0f) {
        return rc::trySendMsgBlockToUpperObj(pReceiver, pSender, mControlUserId, mComboCounter);
    }

    return rc::trySendMsgBlockToLowerObj(pReceiver, pSender, mComboCounter);
}

/**
 * @brief Handle a message sent to the host block.
 * @param pMsg The message.
 * @param pOther The sender's sensor.
 * @param pSelf The host block's sensor.
 * @return True if the message was handled.
 */
bool BlockStateItem::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther,
                                al::HitSensor* pSelf) {
    if (rc::isMsgAskControlUserId(pMsg, mControlUserId) ||
        rc::isMsgRaidonBreakLightReaction(pMsg)) {
        return true;
    }

    bool isMsgForBlock = mIsAssist ? rc::isMsgForAssistBlock(pMsg, pOther, pSelf) :
                                     rc::isMsgForBlockAll(pMsg, pOther, pSelf, 100.0f);

    if (!isMsgForBlock) {
        return false;
    }

    if (!mIsAssist && (al::isMsgPlayerGiantTouch(pMsg) || al::isMsgLaserAttack(pMsg))) {
        if (mSnowCover != nullptr) {
            mSnowCover->tryBreak();
        }

        rc::addScoreByFactor(al::getSensorHost(pSelf), pOther, "壊れ", 0.0f, 0);
        al::startHitReactionBreak(mHostActor);
        kill();
        return true;
    }

    if (!receiveMsgAndAppearItem(pMsg, pOther)) {
        return false;
    }

    mControlUserId = rc::tryFindRelativeControlUserId(pOther);

    if (al::isNerve(this, &NrvBlockStateItemCoin10) ||
        al::isNerve(this, &NrvBlockStateItemHeadgear) ||
        al::isNerve(this, &NrvBlockStateItemAppearItem)) {
        if (al::isMsgPlayerHipDropAll(pMsg) || al::isMsgPlayerStatueDrop(pMsg)) {
            validateHitSensorsForBlockLowerPunch();

            if (al::isNerve(this, &NrvBlockStateItemAppearItem)) {
                al::setNerve(this, &NrvBlockStateItemAppearItemHipDrop);
            }
        } else {
            validateHitSensorsForBlockUpperPunch();
        }
    }

    return rc::getMsgReturnValueForBlock(pMsg);
}

/**
 * @brief Let the current state handle a punch and give out the item.
 * @param pMsg The punch message.
 * @param pSensor The punching sensor.
 * @return True if the punch was handled.
 */
bool BlockStateItem::receiveMsgAndAppearItem(const al::SensorMsg* pMsg, al::HitSensor* pSensor) {
    if (al::isNerve(this, &NrvBlockStateItemCoin10)) {
        mAppearSensor = pSensor;
        bool isReceived = mStateCoinTen->receiveMsg(pMsg);
        mAppearSensor = nullptr;

        if (!isReceived) {
            return false;
        }
    } else if (al::isNerve(this, &NrvBlockStateItemHeadgear)) {
        if (!mStateHeadgear->receiveMsg(pMsg, pSensor)) {
            return false;
        }
    } else if (!tryAppearItem(pMsg, pSensor, al::isMsgPlayerHipDropAll(pMsg))) {
        return false;
    }

    if (mSnowCover != nullptr) {
        mSnowCover->tryBreak();
    }

    return true;
}

namespace {
/**
 * @brief Enable the punch sensor of a block.
 * @param pActor The block.
 * @param offsetY The height of the sensor.
 * @param radius The radius of the sensor.
 */
ALWAYS_INLINE void validateHitSensor(al::LiveActor* pActor, f32 offsetY, f32 radius) {
    al::setSensorRadius(pActor, "UpperPunch", radius);
    al::validateHitSensor(pActor, "UpperPunch");
    al::setSensorFollowPosOffset(pActor, "UpperPunch", sead::Vector3f(0.0f, offsetY, 0.0f));
}
/**
 * @brief Get how long the block reacts to a hit before it can be hit again.
 * @param pMsg The punch message, may be nullptr.
 * @return The reaction length in steps.
 */
inline s32 getReactionStep(const al::SensorMsg* pMsg) {
    if (pMsg != nullptr) {
        if (al::isMsgPlayerTailAttack(pMsg)) {
            return 14;
        }

        if (al::isMsgPlayerClimbAttack(pMsg) || al::isMsgPlayerBodyAttack(pMsg)) {
            return 20;
        }

        if (al::isMsgExplosion(pMsg)) {
            return 3;
        }
    }

    return 6;
}
}  // namespace

/**
 * @brief Enable the punch sensors to hit objects below the block (after a ground pound).
 */
void BlockStateItem::validateHitSensorsForBlockLowerPunch() {
    al::LiveActor* host = mHostActor;

    if (mIsLong) {
        validateHitSensorsLong(host, 0.0f, 50.0f);
    } else {
        validateHitSensor(host, 0.0f, 50.0f);
    }
}

/**
 * @brief Enable the punch sensors to hit objects on top of the block.
 */
void BlockStateItem::validateHitSensorsForBlockUpperPunch() {
    al::LiveActor* host = mHostActor;

    if (mIsLong) {
        validateHitSensorsLong(host, 100.0f, 50.0f);
    } else {
        validateHitSensor(host, 100.0f, 50.0f);
    }
}

/**
 * @brief Handle a touch on the block from the GamePad screen.
 * @param pMsg The message.
 * @param pPointer The screen pointer touching the block.
 * @return True if the touch was handled.
 */
bool BlockStateItem::receiveMsgScreenPoint(const al::SensorMsg* pMsg,
                                           al::ScreenPointer* pPointer) {
    if (!al::isMsgTouchAssistTrig(pMsg)) {
        return false;
    }

    al::HitSensor* sensor = DrcFunction::tryFindDrcPlayerSensor(mHostActor, pPointer);

    if (!receiveMsgAndAppearItem(pMsg, sensor)) {
        return false;
    }

    s32 userId = rc::tryFindRelativeControlUserId(mHostActor, pPointer);
    mIsDrcPunch = true;
    mControlUserId = userId;
    validateHitSensorsForBlockDrcPunch();
    return rc::getMsgReturnValueForBlock(pMsg);
}

/**
 * @brief Enable the punch sensors after a touch from the GamePad screen.
 */
void BlockStateItem::validateHitSensorsForBlockDrcPunch() {
    al::LiveActor* host = mHostActor;

    if (mIsLong) {
        validateHitSensorsLong(host, 50.0f, 60.0f);
    } else {
        validateHitSensor(host, 50.0f, 60.0f);
    }
}

/**
 * @brief Give out the item of the block.
 * @param pMsg The punch message, may be nullptr.
 * @param pSensor The punching sensor, may be nullptr.
 * @param isHipDrop Whether the block was hit by a ground pound.
 * @return True if an item appeared.
 */
bool BlockStateItem::tryAppearItem(const al::SensorMsg* pMsg, al::HitSensor* pSensor,
                                   bool isHipDrop) {
    if (!isValidAppearItem()) {
        return false;
    }

    al::startAction(mHostActor, isHipDrop ? "ReactionHipDrop" : "Reaction");
    f32 appearHeight = mItemType == 0 ? 150.0f : 100.0f;
    f32 scaleY = al::getScaleY(mHostActor);
    mAppearCount++;
    sead::Vector3f offset = sead::Vector3f(0.0f, appearHeight, 0.0f) * scaleY;
    bool isPlayer = pSensor != nullptr && alPlayerFunction::isPlayerActor(pSensor);

    if (mIsPopUp && mIsItemPerPlayer) {
        const char* itemName = collectAppearItemTiming(pSensor);
        sead::Vector3f dir = mAppearDir;

        if (mPlayerNum >= 2 && !mIsTransparent) {
            s32 lastIndex = mPlayerNum - 1;
            const f32* spreadTable =
                mItemType == 18 ? sAppearItemSpreadDegreeWide : sAppearItemSpreadDegree;
            f32 spread = spreadTable[lastIndex];
            sead::Vector3f up = -al::getGravity(mHostActor);
            al::rotateVectorDegree(&dir, dir, up,
                                   spread * 0.5f - (spread / lastIndex) * (mAppearCount - 1));
        }

        al::LiveActor* host = mHostActor;
        sead::Vector3f pos = offset + al::getTrans(host);
        al::appearItemTiming(host, itemName, pos, dir);
    } else {
        al::LiveActor* host = mHostActor;
        sead::Vector3f pos = offset + al::getTrans(host);
        al::appearItemTiming(host, "通常アイテム", pos, mAppearDir, pSensor, false);
        mIsAppearEnd = true;
    }

    if (isPlayer && (mItemType == 11 || mItemType == 12)) {
        rc::addScoreByFactor(mHostActor, pSensor, "アイテム出現", 0.0f, 0);
    }

    mReactionStep = getReactionStep(pMsg);
    tryExecAppearItemLong(pSensor);
    al::setNerve(this, &NrvBlockStateItemAppearItem);
    return true;
}

/**
 * @brief Check whether the block can give out an item now.
 * @return True if the block is not used up and is not still reacting to the previous hit.
 */
bool BlockStateItem::isValidAppearItem() const {
    if (al::isNerve(this, &NrvBlockStateItemEnd)) {
        return false;
    }

    if (al::isNerve(this, &NrvBlockStateItemAppearItem) ||
        al::isNerve(this, &NrvBlockStateItemAppearItemHipDrop)) {
        if (!al::isGreaterStep(this, mReactionStep)) {
            return false;
        }

        return !mIsAppearEnd;
    }

    return true;
}

/**
 * @brief Pick the item given to the player who hit the block and record that they got one.
 * @param pSensor The punching sensor, may be nullptr.
 * @return The name of the item to make appear.
 */
const char* BlockStateItem::collectAppearItemTiming(al::HitSensor* pSensor) {
    if (mPlayerNum == 0) {
        s32 userNum = rc::getActiveControlUserNum(GameDataHolderAccessor(mHostActor));
        mPlayerNum = mIsTransparent ? 1 : userNum;
    }

    const char* itemName = "通常アイテム";
    bool isPowerUpItem = false;

    switch (mItemType) {
    case 6:
    case 7:
    case 8:
    case 9:
    case 23:
        isPowerUpItem = true;
        break;
    }

    s32 userId = -1;

    if (pSensor != nullptr) {
        userId = al::isSensorPlayer(pSensor) ? rc::findControlUserId(pSensor) :
                                               rc::tryFindRelativeControlUserId(pSensor);
    }

    if (pSensor != nullptr && isPowerUpItem && userId >= 0 && !mIsForceChangeItem) {
        al::LiveActor* player = al::isSensorPlayer(pSensor) ?
                                    al::getSensorHost(pSensor) :
                                    rc::findPlayerActorFirstByUserId(mHostActor, userId);
        s32 index = mIsTransparent ? 0 : userId;

        if (mIsUserItemAppeared[index]) {
            for (s32 i = 0; i < mIsUserItemAppeared.size(); i++) {
                if (mIsUserItemAppeared[i]) {
                    continue;
                }

                al::LiveActor* otherPlayer = rc::findPlayerActorFirstByUserId(mHostActor, i);

                if (otherPlayer != nullptr) {
                    index = i;
                    player = otherPlayer;
                    break;
                }
            }
        }

        mIsUserItemAppeared[index] = true;
        itemName = rc::isPlayerMini(player) ? "スーパーキノコ" : "通常アイテム";
    } else {
        for (s32 i = 0; i < mIsUserItemAppeared.size(); i++) {
            if (mIsUserItemAppeared[i]) {
                mIsUserItemAppeared[i] = true;
                itemName = "通常アイテム";
                break;
            }
        }
    }

    if (mItemType == 17) {
        itemName = PlayerStockerFunction::isDoubleItemAppearedMax(mHostActor) ?
                       "ダブルマリオ用コイン" :
                       itemName;
    }

    mIsAppearEnd = true;

    if (mAppearCount < mPlayerNum) {
        for (s32 i = 0; i < mIsUserItemAppeared.size(); i++) {
            if (!mIsUserItemAppeared[i] &&
                rc::findPlayerActorFirstByUserId(mHostActor, i) != nullptr) {
                mIsAppearEnd = false;
                break;
            }
        }
    }

    return itemName;
}

/**
 * @brief Make the coins on both sides of a long block appear.
 * @param pSensor The punching sensor, may be nullptr.
 */
void BlockStateItem::tryExecAppearItemLong(const al::HitSensor* pSensor) {
    if (!mIsLong) {
        return;
    }

    const sead::Vector3f localOffsets[] = {{100.0f, 150.0f, 0.0f}, {-100.0f, 150.0f, 0.0f}};

    for (s32 i = 0; i < 2; i++) {
        sead::Vector3f pos;
        pos.setMul(*mHostActor->getBaseMtx(), localOffsets[i]);
        al::appearItemTiming(mHostActor, "ロングブロック用コイン", pos, sead::Vector3f::ez, pSensor,
                             false);
    }
}

/**
 * @brief Get the empty block left behind once the item is out.
 * @return The empty block.
 */
BlockEmpty* BlockStateItem::getBlockEmpty() const {
    return mBlockEmpty;
}

/**
 * @brief Get the empty block left behind once the item is out.
 * @return The empty block, or nullptr if there is none.
 */
BlockEmpty* BlockStateItem::tryGetBlockEmpty() const {
    return mBlockEmpty;
}

/**
 * @brief Get the ten coin state of the block.
 * @return The ten coin state, or nullptr if the block holds no ten coins.
 */
BlockStateCoinTen* BlockStateItem::tryGetBlockStateCoinTen() const {
    return mStateCoinTen;
}

/**
 * @brief Replace the block by its empty block and end the state.
 */
void BlockStateItem::setNerveEnd() {
    if (!mIsAssist) {
        al::copyPose(mBlockEmpty, mHostActor);
        mBlockEmpty->appear();

        if (mIsConnectedRailBlock) {
            mBlockEmpty->onConnectRailBlock();
        }
    }

    al::setNerve(this, &NrvBlockStateItemEnd);
}

namespace {
/**
 * @brief Enable the three punch sensors of a long block.
 * @param pActor The long block.
 * @param offsetY The height of the sensors.
 * @param radius The radius of the sensors.
 */
void validateHitSensorsLong(al::LiveActor* pActor, f32 offsetY, f32 radius) {
    al::setSensorRadius(pActor, "UpperPunchCenter", radius);
    al::setSensorRadius(pActor, "UpperPunchLeft", radius);
    al::setSensorRadius(pActor, "UpperPunchRight", radius);
    al::validateHitSensor(pActor, "UpperPunchCenter");
    al::validateHitSensor(pActor, "UpperPunchLeft");
    al::validateHitSensor(pActor, "UpperPunchRight");
    al::setSensorFollowPosOffset(pActor, "UpperPunchCenter", sead::Vector3f(0.0f, offsetY, 0.0f));
    al::setSensorFollowPosOffset(pActor, "UpperPunchLeft", sead::Vector3f(-100.0f, offsetY, 0.0f));
    al::setSensorFollowPosOffset(pActor, "UpperPunchRight", sead::Vector3f(100.0f, offsetY, 0.0f));
}
}  // namespace

/**
 * @brief Wait for the block to be hit.
 */
void BlockStateItem::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(mHostActor, "Wait");
    }
}

/**
 * @brief Wait for the hit reaction to end after an item appeared.
 */
void BlockStateItem::exeAppearItem() {
    if (!al::isActionEnd(mHostActor)) {
        return;
    }

    if (mIsAppearEnd) {
        setNerveEnd();
        return;
    }

    if (!al::isGreaterStep(this, mReactionStep)) {
        return;
    }

    if (!mIsTransparent) {
        al::LiveActor* host = mHostActor;

        if (mIsLong) {
            al::invalidateHitSensor(host, "UpperPunchCenter");
            al::invalidateHitSensor(host, "UpperPunchLeft");
            al::invalidateHitSensor(host, "UpperPunchRight");
        } else {
            al::invalidateHitSensor(host, "UpperPunch");
        }
    }

    al::setNerve(this, &NrvBlockStateItemWait);
}

/**
 * @brief Give out ten coins, then end the state.
 */
void BlockStateItem::exeCoin10() {
    if (al::updateNerveState(this)) {
        if (mStateCoinTen->isAppearBoxCoin()) {
            al::setNerve(this, &NrvBlockStateItemEnd);
        } else {
            setNerveEnd();
        }
    }
}

/**
 * @brief Give out the headgear box, then end the state.
 */
void BlockStateItem::exeHeadgear() {
    if (al::updateNerveState(this)) {
        kill();
    }
}

/**
 * @brief End the state.
 */
void BlockStateItem::exeEnd() {
    kill();
}

BlockStateItem::~BlockStateItem() = default;
