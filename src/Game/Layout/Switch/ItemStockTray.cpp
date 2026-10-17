#include "Layout/Switch/ItemStockTray.hpp"

#include <math.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <prim/seadSafeString.h>
#include <stdlib.h>

#include <attributes.h>

#include "Layout/StockBlurEffect.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerUtil.hpp"

// Non-const nerve objects: these nerves are merged into one data block, so neighbouring nerves are
// addressed relative to each other.
#define ITEM_STOCK_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(ItemStockTray, WaitIdle);
NERVE_DECL(ItemStockTray, Appear);
NERVE_DECL(ItemStockTray, WaitStockEffect);
NERVE_DECL(ItemStockTray, StockEffect);

/** Closes the tray before playing the stock effect of a newly stocked item. */
class ItemStockTrayNrvEndAndStock : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<ItemStockTray>()->exeEnd();
    }
};

NERVE_DECL(ItemStockTray, TriggerMenu);
NERVE_DECL(ItemStockTray, End);
NERVE_DECL(ItemStockTray, WaitActive);
NERVE_DECL(ItemStockTray, TrySpawnItem);
NERVE_DECL(ItemStockTray, WaitUseItem);
NERVE_DECL(ItemStockTray, Stock);
NERVE_DECL(ItemStockTray, WaitReleaseButton);
NERVE_DECL(ItemStockCoinSpawner, Wait);
NERVE_DECL(ItemStockCoinSpawner, AddCoins);
FOR_EACH(ITEM_STOCK_NERVE_MAKE, ItemStockTray, WaitIdle, WaitStockEffect, StockEffect, EndAndStock,
         TriggerMenu, End, WaitActive, TrySpawnItem, WaitReleaseButton)
FOR_EACH(ITEM_STOCK_NERVE_MAKE, ItemStockCoinSpawner, Wait, AddCoins)
NERVES_MAKE_NOSTRUCT(ItemStockTray, Appear, WaitUseItem, Stock)

/** Number of item slots in the tray. */
constexpr s32 cSlotNum = 6;
/** Number of blur effects that can fly towards the tray at once. */
constexpr s32 cBlurEffectNum = 4;
/** Slot in the middle of the tray, used as the origin of the slot positions. */
constexpr s32 cCenterSlot = 2;
/** Horizontal distance between two slots in the layout. */
constexpr f32 cSlotWidth = 58.0f;
/** Angle step between two coins spawned by the coin spawner. */
constexpr f32 cCoinAngleStep = 108.0f;

/** Modes of ItemStockTray::updateCountAndSelection(). */
enum UpdateMode {
    UpdateMode_None = 0,
    UpdateMode_Freeze = 1,
    UpdateMode_Animate = 2,
};

/**
 * @brief Creates the parts actor keeper of a layout actor with room for all of its parts panes.
 * @param pActor Layout actor whose layout was just initialized.
 */
inline void initPartsActorKeeper(al::LayoutActor* pActor) {
    const nn::util::IntrusiveListNode& partsList =
        pActor->getLayoutKeeper()->getLayout()->_48;

    if (partsList.GetNext() == &partsList) {
        return;
    }

    s32 partsNum = 0;

    for (const nn::util::IntrusiveListNode* node = partsList.GetNext(); node != &partsList;
         node = node->GetNext()) {
        partsNum++;
    }

    pActor->initLayoutPartsActorKeeper(partsNum);
}

/**
 * @brief Converts a stocked item type to the tray slot that shows it.
 * @param itemType Item type.
 * @return The slot of the item.
 */
inline s32 convertItemTypeToSlot(s32 itemType) {
    switch (itemType) {
    case 1:
        return 1;
    case 2:
        return 0;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return 2;
    case 6:
        return 0;
    case 7:
        return 5;
    default:
        return 0;
    }
}

/**
 * @brief Converts a tray slot to the item type it shows.
 * @param slot Tray slot.
 * @return The item type of the slot.
 */
inline s32 convertSlotToItemType(s32 slot) {
    switch (slot) {
    case 0:
        return 2;
    case 1:
        return 1;
    case 2:
        return 5;
    case 3:
        return 3;
    case 4:
        return 4;
    case 5:
        return 7;
    default:
        return 0;
    }
}

/**
 * @brief Checks if the item button of a controller is held.
 * @param port Controller port.
 * @return Whether the button is held.
 */
inline bool isHoldItemButtonByPort(s32 port) {
    if (al::isPadTypeJoySingle(port)) {
        return al::isPadHoldX(port);
    }

    return al::isPadHoldUp(port);
}

/**
 * @brief Starts a cursor animation of a layout unless it is already playing.
 * @param pLayout Layout with the cursor pane.
 * @param pActionName Cursor animation.
 */
inline void startCursorAction(al::IUseLayoutAction* pLayout, const char* pActionName) {
    if (!al::isActionPlaying(pLayout, pActionName, "PicCursor")) {
        al::startAction(pLayout, pActionName, "PicCursor");
    }
}
}  // namespace

/**
 * @brief Creates the tray, its slot parts, the blur effects and the coin spawner.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pArchiveName Name of the parts pane in the parent layout.
 * @param pParent Parent layout actor.
 * @param pHolder Game data holder used to read the stocked items.
 * @param pDirector Item director that spawns the used items.
 */
ItemStockTray::ItemStockTray(const al::LayoutInitInfo& rInfo, const char* pName,
                             const char* pArchiveName, al::LayoutActor* pParent,
                             const GameDataHolder* pHolder, ProjectItemDirector* pDirector)
    : al::LayoutActor(pName), ItemStockLayoutBase(pParent), mGameDataHolder(pHolder),
      mItemDirector(pDirector) {
    al::initLayoutPartsActor(this, pParent, rInfo, pArchiveName, nullptr);
    initPartsActorKeeper(this);

    mBlurEffects = new StockBlurEffect*[cBlurEffectNum];

    for (s32 i = 0; i < cBlurEffectNum; i++) {
        mBlurEffects[i] = new StockBlurEffect(this, i + 1);
    }

    initNerve(&NrvItemStockTrayWaitIdle, 0);

    mSlotParts = new al::LayoutActor*[cSlotNum];

    for (s32 i = 0; i < cSlotNum; i++) {
        mSlotParts[i] = new al::LayoutActor("ItemStockTrayParts");
        al::initLayoutPartsActor(mSlotParts[i], this, rInfo,
                                 al::StringTmp<32>("ParItem_%d", i).cstr(), nullptr);
        setCount(i, getStockItemCount(i));
    }

    mCoinSpawner = new ItemStockCoinSpawner(this, mItemDirector);
}

/**
 * @brief Shows the stock count of a slot and greys the slot out when it is empty.
 * @param slot Tray slot.
 * @param count Number of stocked items of the slot.
 */
void ItemStockTray::setCount(s32 slot, s32 count) {
    al::setPaneString(mSlotParts[slot], "TxtItemCount0",
                      sead::WFormatFixedSafeString<3>(u"%d", count).cstr());

    if (count != 0) {
        al::startAction(mSlotParts[slot], "Activate", "ItemPic");

        if (slot == cSlotNum - 1) {
            al::showPaneNoRecursive(this, "ParItem_5");
        }
    } else {
        al::startAction(mSlotParts[slot], "Deactivate", "ItemPic");
        al::startAction(mSlotParts[slot], "Idle", "Main");

        if (slot == cSlotNum - 1) {
            al::hidePaneNoRecursive(this, "ParItem_5");
        }
    }
}

/** @brief Updates the blur effects and the coin spawner. */
void ItemStockTray::control() {
    for (s32 i = 0; i < cBlurEffectNum; i++) {
        mBlurEffects[i]->update();
    }

    mCoinSpawner->getNerveKeeper()->update();
}

/** @brief Shows the tray with the current stock. */
void ItemStockTray::appear() {
    al::LayoutActor::appear();
    updateCountAndSelection(UpdateMode_Freeze);
}

/**
 * @brief Refreshes the stock counts of all slots and moves the selection to a stocked item.
 * @param mode How the tray appear/end animations are played (see UpdateMode).
 */
void ItemStockTray::updateCountAndSelection(s32 mode) {
    if (getStockItemCount(cSlotNum - 1) != 0) {
        al::showPaneNoRecursive(this, "ParItem_5");
    } else {
        al::hidePaneNoRecursive(this, "ParItem_5");
    }

    s32 stockedSlot = -1;

    for (s32 i = 0; i < cSlotNum; i++) {
        s32 slot = static_cast<u32>(mSelectedSlot + i) % cSlotNum;
        s32 count = getStockItemCount(slot);
        setCount(slot, count);

        if (count > 0 && stockedSlot == -1) {
            stockedSlot = slot;
        }
    }

    if (stockedSlot != -1) {
        mSelectedSlot = stockedSlot;

        if (mIsEmpty) {
            if (mode == UpdateMode_Animate) {
                if (!al::isActionPlaying(this, "Tray_Appear", "Main")) {
                    al::startAction(this, "Tray_Appear", nullptr);
                }
            } else if (mode == UpdateMode_Freeze) {
                al::startFreezeActionEnd(this, "Tray_Appear", nullptr);
            }

            mIsEmpty = false;
        }

        if (mIsIdle) {
            startSlotAction("Slot%d_IDLE");
            al::startAction(mSlotParts[mSelectedSlot], "Idle", "Main");
        } else {
            startSlotAction("Slot%d_SELECTED");
            al::startAction(mSlotParts[mSelectedSlot], "Selected", "Main");
        }
    } else {
        if (mode == UpdateMode_Animate) {
            al::startAction(this, "End", nullptr);
        } else if (mode == UpdateMode_Freeze) {
            al::startFreezeActionEnd(this, "End", nullptr);
        }

        mIsEmpty = true;
        al::startAction(this, "SelectNone", "ItemSlots");
    }
}

/**
 * @brief Opens the tray for a controller if it is idle.
 * @param port Controller port that opened the tray.
 */
void ItemStockTray::startAppear(s32 port) {
    if (!isAlive()) {
        appear();
    }

    if (mIsIdle) {
        mPort = port;
        mIsIdle = false;
        al::setNerve(this, &NrvItemStockTrayAppear);
    }
}

/**
 * @brief Spawns coins instead of an item that does not fit in the tray.
 * @param count Number of coins.
 * @param pSensor Sensor of the actor that took the item.
 */
void ItemStockTray::spawnCoins(u8 count, al::HitSensor* pSensor) {
    mCoinSpawner->spawnCoins(count, pSensor);
}

/** @brief Stops the blur effects for a demo. */
void ItemStockTray::startDemo() {
    mIsDemo = true;

    for (s32 i = 0; i < cBlurEffectNum; i++) {
        mBlurEffects[i]->stop(true);
    }
}

/** @brief Allows the tray to be used again after a demo. */
void ItemStockTray::endDemo() {
    mIsDemo = false;
}

/**
 * @brief Adds a newly stocked item to the tray, optionally with a blur effect flying to its slot.
 * @param pItem Stocked item (unused).
 * @param pActor Actor that took the item, the blur effect starts at its position.
 * @param itemType Item type.
 * @param isPlayEffect Whether to play the stock effect.
 */
void ItemStockTray::stockItem(const al::LiveActor* pItem, const al::LiveActor* pActor,
                              s32 itemType, bool isPlayEffect) {
    s32 stockSlot = convertItemTypeToSlot(itemType);
    bool isFirstItem = true;

    for (s32 i = 0; i < cSlotNum; i++) {
        u32 count = getStockItemCount(i);

        if (i == stockSlot ? count > 1 : count != 0) {
            isFirstItem = false;
            break;
        }
    }

    mStockActor = pActor;

    if (pActor != nullptr && isPlayEffect) {
        sead::Vector2f layoutPos = sead::Vector2f::zero;
        sead::Vector3f trans = al::getTrans(pActor);
        trans.y += StockBlurEffect::getEffectStartOffsetY();
        al::calcLayoutPosFromWorldPos(&layoutPos, pActor, trans, 0);
        f32 posZ = StockBlurEffect::getEffectStartPosZ();

        if (mBlurEffects != nullptr) {
            sead::Vector3f startPos(layoutPos.x, layoutPos.y, posZ);

            if (isFirstItem) {
                mStockEffectStartPos = startPos;
            } else {
                sead::Vector3f endPos;
                al::calcTrans(&endPos, this);
                endPos.x = (stockSlot - cCenterSlot) * cSlotWidth + endPos.x;
                takeBlurEffect()->start(startPos, endPos);
            }
        }
    }

    if (isFirstItem) {
        mSelectedSlot = stockSlot;
        al::startAction(this, "Tray_Appear", nullptr);
        mIsEmpty = false;
    }

    if (isPlayEffect) {
        if (mIsIdle) {
            if (isFirstItem) {
                al::setNerve(this, &NrvItemStockTrayWaitStockEffect);
            } else {
                al::setNerve(this, &NrvItemStockTrayStockEffect);
            }
        } else {
            al::setNerve(this, &NrvItemStockTrayEndAndStock);
        }

        return;
    }

    for (s32 i = 0; i < cSlotNum; i++) {
        setCount(i, getStockItemCount(i));
    }

    al::setNerve(this, &NrvItemStockTrayWaitIdle);
}

/**
 * @brief Checks if the item button was just tapped, which opens the tray.
 * @return Whether the tray menu was triggered.
 */
bool ItemStockTray::canSpawnItem() const {
    return al::isNerve(this, &NrvItemStockTrayTriggerMenu);
}

/**
 * @brief Checks if the tray just started closing.
 * @return Whether this is the first step of closing.
 */
bool ItemStockTray::isStartClose() {
    return al::isNerve(this, &NrvItemStockTrayEnd) && al::isFirstStep(this);
}

/** @brief Plays the activate animation and the cursor of the controller type. */
void ItemStockTray::exeAppear() {
    if (al::isFirstStep(this)) {
        updateSelection(mSelectedSlot);
        al::startAction(this, "TrayActivate", "Main");
        al::startSe(this, "Appear");
        if (al::isPadTypeJoySingle(mPort)) {
            al::startAction(this, "PicCursor_RIGHT_Generic", "PicCursor");
        } else {
            al::startAction(this, "PicCursor_A", "PicCursor");
        }
    }

    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvItemStockTrayWaitActive);
    }
}

/**
 * @brief Moves the selection to the nearest stocked slot in the direction of a slot.
 * @param slot Requested slot, can be one past either end of the tray.
 */
void ItemStockTray::updateSelection(s32 slot) {
    s32 prevSlot = mSelectedSlot;

    if (prevSlot == slot) {
        if (getStockItemCount(slot) == 0) {
            return;
        }

        mSelectedSlot = slot;

        if (mIsIdle) {
            startSlotAction("Slot%d_IDLE");
        } else {
            startSlotAction("Slot%d_SELECTED");
            al::startAction(mSlotParts[mSelectedSlot], "Selected", "Main");
        }

        return;
    }

    if (prevSlot < slot) {
        for (s32 i = static_cast<u32>(slot) % cSlotNum; i != mSelectedSlot;
             i = static_cast<u32>(i + 1) % cSlotNum) {
            if (getStockItemCount(i) == 0) {
                continue;
            }

            if (i != mSelectedSlot) {
                al::startSe(this, "Select");
            }

            mSelectedSlot = i;

            if (mIsIdle) {
                startSlotAction("Slot%d_IDLE");
                return;
            }

            if (i != prevSlot) {
                al::startAction(mSlotParts[prevSlot], "Idle", "Main");
            }

            startSlotAction("Slot%d_SELECTED");
            al::startAction(mSlotParts[mSelectedSlot], "Selected", "Main");
            return;
        }
    } else if (prevSlot > slot) {
        for (s32 i = slot < 0 ? cSlotNum - 1 : slot; i != mSelectedSlot;
             i = i - 1 < 0 ? cSlotNum - 1 : i - 1) {
            if (getStockItemCount(i) == 0) {
                continue;
            }

            mSelectedSlot = i;
            al::startSe(this, "Select");

            if (mIsIdle) {
                startSlotAction("Slot%d_IDLE");
                return;
            }

            startSlotAction("Slot%d_SELECTED");

            if (mSelectedSlot != prevSlot) {
                al::startAction(mSlotParts[prevSlot], "Idle", "Main");
            }

            al::startAction(mSlotParts[mSelectedSlot], "Selected", "Main");
            return;
        }
    }
}

/** @brief Lets the player move the selection, use the selected item or close the tray. */
void ItemStockTray::exeWaitActive() {
    if (al::isFirstStep(this)) {
        updateSelection(mSelectedSlot);
    }

    if (al::isPadTypeJoySingle(mPort)) {
        startCursorAction(this, "PicCursor_RIGHT_Generic");
    } else {
        startCursorAction(this, "PicCursor_A");
    }

    if (rc::isPadTriggerUiInventoryByPort(mPort) || rc::isPadTriggerUiCancelByPort(mPort)) {
        al::setNerve(this, &NrvItemStockTrayEnd);
        return;
    }

    if (rc::isPadTriggerUiLeftByPort(mPort)) {
        updateSelection(mSelectedSlot - 1);
    } else if (rc::isPadTriggerUiRightByPort(mPort)) {
        updateSelection(mSelectedSlot + 1);
    }

    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        al::setNerve(this, &NrvItemStockTrayTrySpawnItem);
    }
}

/** @brief Waits for a player (or the assist player) to hold the item button. */
void ItemStockTray::exeWaitIdle() {
    if (al::isFirstStep(this)) {
        mIsIdle = true;
    }

    if (al::isLessStep(this, 15) || mIsDemo) {
        return;
    }

    s32 port = al::getMainControllerPort();
    s32 assistPort = SingleModeDataFunction::getIs2PAssistMode(this) ? rc::getPadPortByUserId(1) :
                                                                         -1;
    if (al::isPadTypeJoySingle(port)) {
        startCursorAction(this, "PicCursor_UP_Generic");
    } else {
        startCursorAction(this, "PicCursor_UP");
    }

    if (port < 0 || !isHoldItemButtonByPort(port)) {
        port = assistPort;

        if (port < 0 || !isHoldItemButtonByPort(port)) {
            return;
        }
    }

    mPort = port;
    al::setNerve(this, &NrvItemStockTrayWaitUseItem);
}

/** @brief One step nerve telling the scene that the item button was tapped. */
void ItemStockTray::exeTriggerMenu() {
    al::setNerve(this, &NrvItemStockTrayWaitIdle);
}

/** @brief Uses the selected item when the item button is held long enough. */
void ItemStockTray::exeWaitUseItem() {
    if (al::isFirstStep(this) && mPort < 0) {
        mPort = al::getMainControllerPort();
    }

    if (mIsDemo) {
        al::setNerve(this, &NrvItemStockTrayWaitIdle);
        return;
    }

    if (al::isGreaterEqualStep(this, 30)) {
        al::setNerve(this, &NrvItemStockTrayTrySpawnItem);
        return;
    }

    if (isHoldItemButton()) {
        return;
    }

    if (al::isLessEqualStep(this, 15)) {
        al::setNerve(this, &NrvItemStockTrayTriggerMenu);
    } else {
        al::setNerve(this, &NrvItemStockTrayWaitIdle);
    }
}

/** @brief Waits for the tray to appear, then starts the blur effect of the first item. */
void ItemStockTray::exeWaitStockEffect() {
    if (al::isActionEnd(this, "Main")) {
        startStockEffect(mStockEffectStartPos);
        al::setNerve(this, &NrvItemStockTrayStockEffect);
    }
}

/** @brief Waits for the blur effect to reach the tray. */
void ItemStockTray::exeStockEffect() {
    if (al::isGreaterEqualStep(this, StockBlurEffect::getEffectFrameNum() +
                                          StockBlurEffect::getEffectDelayFrames())) {
        al::setNerve(this, &NrvItemStockTrayStock);
    }
}

/** @brief Plays the stock sound and reaction and refreshes the slot counts. */
void ItemStockTray::exeStock() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "Stock");

        if (mStockActor != nullptr) {
            al::startHitReaction(mStockActor, "ItemGet");
        }

        for (s32 i = 0; i < cSlotNum; i++) {
            setCount(i, getStockItemCount(i));
        }
    }

    updateSelection(mSelectedSlot);
    al::setNerve(this, &NrvItemStockTrayWaitIdle);
}

/** @brief Spawns the selected item, or plays the invalid sound when the slot is empty. */
void ItemStockTray::exeTrySpawnItem() {
    if (al::isFirstStep(this) && mSelectedSlot >= 0) {
        if (getStockItemCount(mSelectedSlot) == 0) {
            al::startSe(this, "Invalid");

            if (mIsIdle) {
                al::setNerve(this, &NrvItemStockTrayWaitIdle);
            } else {
                al::setNerve(this, &NrvItemStockTrayWaitActive);
            }

            return;
        }

        if (mItemDirector == nullptr) {
            al::setNerve(this, &NrvItemStockTrayEnd);
            return;
        }

        if (mItemDirector->useStockItem(mPort, convertSlotToItemType(mSelectedSlot))) {
            al::startSe(mParentLayout, "ItemConfirm");
            SingleModeDataFunction::useStockItem(GameDataHolderWriter(this),
                                                 convertSlotToItemType(mSelectedSlot));
            updateCountAndSelection(UpdateMode_None);
        }
    }

    if (!al::isActionEnd(mSlotParts[mSelectedSlot], "Main")) {
        return;
    }

    if (!isHoldItemButton()) {
        al::setNerve(this, &NrvItemStockTrayEnd);
        return;
    }

    al::setNerve(this, &NrvItemStockTrayWaitReleaseButton);
    updateCountAndSelection(UpdateMode_Animate);
}

/** @brief Waits until the item button is released after using an item. */
void ItemStockTray::exeWaitReleaseButton() {
    if (!isHoldItemButton()) {
        al::setNerve(this, &NrvItemStockTrayWaitIdle);
    }
}

/** @brief Closes the tray, then waits idle or plays the pending stock effect. */
void ItemStockTray::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "TrayDeactivate", "Main");
        al::startSe(this, "End");
    }

    if (!al::isActionEnd(this, "Main")) {
        return;
    }

    mIsIdle = true;

    if (mSelectedSlot >= 0) {
        updateSelection(mSelectedSlot);
    }

    if (al::isNerve(this, &NrvItemStockTrayEndAndStock)) {
        al::setNerve(this, &NrvItemStockTrayStockEffect);
        return;
    }

    if (al::isPadTypeJoySingle(mPort)) {
        al::startAction(this, "PicCursor_UP_Generic", "PicCursor");
    } else {
        al::startAction(this, "PicCursor_UP", "PicCursor");
    }

    updateCountAndSelection(UpdateMode_Animate);
    al::setNerve(this, &NrvItemStockTrayWaitIdle);
}

/**
 * @brief Checks if the item button of the controller using the tray is held.
 * @return Whether the button is held.
 */
inline bool ItemStockTray::isHoldItemButton() const {
    return al::isPadTypeJoySingle(mPort) ? al::isPadHoldX(mPort) : al::isPadHoldUp(mPort);
}

/**
 * @brief Gets the number of stocked items shown in a slot.
 * @param slot Tray slot.
 * @return The number of stocked items.
 */
inline u32 ItemStockTray::getStockItemCount(s32 slot) const {
    return SingleModeDataFunction::getStockItemCountByIndex(
        GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)), slot);
}

/**
 * @brief Gets an idle blur effect, or stops and reuses the one closest to its end.
 * @return The blur effect to start.
 */
ALWAYS_INLINE inline StockBlurEffect* ItemStockTray::takeBlurEffect() {
    StockBlurEffect* oldest = nullptr;

    for (s32 i = 0; i < cBlurEffectNum; i++) {
        StockBlurEffect* effect = mBlurEffects[i];

        if (effect->getStep() < 0) {
            return effect;
        }

        if (oldest == nullptr || effect->getStep() < oldest->getStep()) {
            oldest = effect;
        }
    }

    oldest->stop(false);
    return oldest;
}

/**
 * @brief Starts a blur effect flying to the selected slot.
 * @param rStartPos Start position of the effect in the layout.
 */
ALWAYS_INLINE inline void ItemStockTray::startStockEffect(const sead::Vector3f& rStartPos) {
    sead::Vector3f endPos;
    al::calcTrans(&endPos, this);
    endPos.x += (mSelectedSlot - cCenterSlot) * cSlotWidth;
    takeBlurEffect()->start(rStartPos, endPos);
}

/**
 * @brief Starts the slot animation of the tray for the selected slot.
 * @param pFormat Action name format, takes the slot.
 */
inline void ItemStockTray::startSlotAction(const char* pFormat) {
    al::startAction(this, al::StringTmp<32>(pFormat, mSelectedSlot).cstr(), "ItemSlots");
}

/**
 * @brief Creates the coin spawner.
 * @param pTray Tray that owns the spawner (unused).
 * @param pDirector Item director that spawns the coins.
 */
ItemStockCoinSpawner::ItemStockCoinSpawner(ItemStockTray* pTray, ProjectItemDirector* pDirector)
    : mItemDirector(pDirector) {
    mNerveKeeper = new al::NerveKeeper(this, &NrvItemStockCoinSpawnerWait, 0);
}

/**
 * @brief Queues coins to spawn and starts spawning them if no coins are being spawned.
 * @param count Number of coins.
 * @param pSensor Sensor of the actor that took the item.
 */
void ItemStockCoinSpawner::spawnCoins(u8 count, al::HitSensor* pSensor) {
    mCoinNum += count;
    mSensor = pSensor;

    if (al::isNerve(this, &NrvItemStockCoinSpawnerWait)) {
        al::setNerve(this, &NrvItemStockCoinSpawnerAddCoins);
    }
}

/** @brief Waits for coins to spawn. */
void ItemStockCoinSpawner::exeWait() {}

/** @brief Spawns one coin in front of the actor that took the item, rotating around it. */
void ItemStockCoinSpawner::exeAddCoins() {
    if (al::isFirstStep(this) && mCoinNum <= 0) {
        al::setNerve(this, &NrvItemStockCoinSpawnerWait);
    }

    if (!al::isStep(this, 1)) {
        return;
    }

    s32 random = rand();
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    sead::Vector3f front;
    sead::Vector3f velocity(0.0f, 0.0f, 0.0f);
    al::LiveActor* host = al::getSensorHost(mSensor);

    if (rc::isReallyPlayerActor(host)) {
        front = rc::getPlayerFront(host);
    } else {
        al::calcFrontDir(&front, host);
    }

    al::calcUpDir(&up, host);
    velocity = front;
    al::rotateVectorDegree(&velocity, velocity, up, mAngle);
    al::normalize(&velocity);

    f32 speed = random % 30 + 120;
    velocity *= speed;
    velocity.y += random % 100 + 100;

    mItemDirector->appearItem("コインx1[自動取得]", al::getTrans(host) + velocity,
                              sead::Vector3f::ez, mSensor, false, false);
    mAngle = fmodf(mAngle + cCoinAngleStep, 360.0f);
    mCoinNum--;
    al::setNerve(this, &NrvItemStockCoinSpawnerAddCoins);
}
