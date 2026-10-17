#include "Layout/ButtonItemStockParts.hpp"

#include <attributes.h>
#include <math/seadVector.h>

#include "Layout/ButtonTouch.hpp"
#include "Layout/StockBlurEffect.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Scene/ProjectItemDirector.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"

// Non-const nerve objects: these nerves are merged into one data block, so neighbouring nerves are
// addressed relative to each other.
#define ITEM_STOCK_PARTS_NERVE_MAKE(Class, Action) Class##Nrv##Action Nrv##Class##Action;

namespace {
NERVE_DECL(ButtonItemStockParts, WaitCourseSelect);
NERVE_DECL(ButtonItemStockParts, Wait);
NERVE_DECL(ButtonItemStockParts, NoItemWait);
NERVE_DECL(ButtonItemStockParts, Disable);
NERVE_DECL(ButtonItemStockParts, NoItemHide);

/** Plays the stock effect while the box is hidden because no item was stocked. */
class ButtonItemStockPartsNrvStockEffectFromNoItem : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<ButtonItemStockParts>()->exeStockEffect();
    }
};

NERVE_DECL(ButtonItemStockParts, StockEffect);
NERVE_DECL(ButtonItemStockParts, Use);
NERVE_DECL(ButtonItemStockParts, Stock);
NERVES_MAKE_NOSTRUCT(ButtonItemStockParts, Use)

/**
 * @brief Gets the action of the item picture slots for a stocked item.
 * @param itemType Stocked item type.
 * @return The action name, "NoItem" for no item.
 */
const char* getItemActionName(s32 itemType) {
    switch (itemType) {
    case 1:
        return "SuperKinoko";
    case 2:
        return "Animal";
    case 3:
        return "FireFlower";
    case 4:
        return "Konoha";
    case 5:
        return "Boomerang";
    case 6:
        return "KonohaAssist";
    case 7:
        return "SpBell";
    case 8:
        return "Animal";
    default:
        return "NoItem";
    }
}

FOR_EACH(ITEM_STOCK_PARTS_NERVE_MAKE, ButtonItemStockParts, WaitCourseSelect, Wait, NoItemWait,
         Disable, NoItemHide, StockEffectFromNoItem, StockEffect, Stock)

/** Number of item picture slots of the box. */
constexpr s32 cItemSlotNum = 4;
/** Number of blur effects that can fly towards the box at once. */
constexpr s32 cBlurEffectNum = 4;
/** Number of controller ports checked for the use-item button (the handheld port excluded). */
constexpr s32 cPadPortNum = 4;
/** Controller port of the handheld mode. */
constexpr s32 cHandheldPort = 5;
}  // namespace

/**
 * @brief Creates the item stock box and the blur effects of the items flying into it.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPaneName Parts pane name in the parent layout.
 * @param pParent Parent layout.
 * @param pItemDirector Item director used to spawn the stocked item (can be null).
 * @param isCourseSelect Whether the box is shown on the course select screen.
 */
ButtonItemStockParts::ButtonItemStockParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                           const char* pPaneName, al::LayoutActor* pParent,
                                           ProjectItemDirector* pItemDirector,
                                           bool isCourseSelect)
    : al::LayoutActor(pName), ItemStockLayoutBase(pParent), mItemDirector(pItemDirector),
      mIsCourseSelect(isCourseSelect) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPaneName, nullptr);
    initNerve(getWaitNerve(), 0);

    if (!mIsCourseSelect) {
        mTouch = new ButtonTouch("アイテムストックボタン", this, nullptr);
        mBlurEffects = new StockBlurEffect*[cBlurEffectNum];

        for (s32 i = 0; i < cBlurEffectNum; i++) {
            mBlurEffects[i] = new StockBlurEffect(this, i + 1);
        }
    }

    al::initLayoutPartsAudioKeeper(this, rInfo, "ButtonItemStockParts");
    al::startAction(this, mIsCourseSelect ? "CourseSelectSceneBox" : "StageSceneBox", "BoxType");
    al::startAction(this, mIsCourseSelect ? "WaitCourseSelect" : "Wait", "Main");
}

/** @brief Updates the controller icon after a controller change and moves the blur effects. */
void ButtonItemStockParts::control() {
    if (rc::isControllerAssignmentChanged()) {
        updateIcon();
    }

    StockBlurEffect** blurEffects = mBlurEffects;
    if (blurEffects != nullptr) {
        for (s32 i = 0; i < cBlurEffectNum; i++) {
            blurEffects[i]->update();
        }
    }
}

/** @brief Shows the button icon matching the play style of the active controllers. */
void ButtonItemStockParts::updateIcon() {
    u64 portList = rc::getActiveInputPortList(this);
    bool isExistJoyDouble = false;
    bool isExistJoySingle = false;

    for (s32 i = 0; i < al::getMaxControllerPorts(); i++) {
        s32 port = i + 1;
        bool isConnected = false;

        u16 portMask = 1 << port;
        if ((portList & portMask) != 0) {
            isConnected = al::isPadConnected(port);
        }

        s32 userId = rc::tryCalcControlUserIdFromPortNum(this, port);
        if (userId < 0) {
            continue;
        }

        bool isAlive = !rc::isDeadControlUserInStage(this, userId);
        if (isConnected & isAlive) {
            bool isJoySingle = al::isPadTypeJoySingle(port);
            isExistJoyDouble |= !isJoySingle;
            isExistJoySingle |= isJoySingle;
        }
    }

    if (!isExistJoySingle) {
        al::startAction(this, "IconUp", "IconPlayStyle");
    } else if (isExistJoyDouble) {
        al::startAction(this, "IconPlayStyle_Mixed", "IconPlayStyle");
    } else {
        al::startAction(this, "IconPlayStyle_Sideways", "IconPlayStyle");
    }
}

/**
 * @brief Checks whether the box is disabled.
 * @return Whether the Disable state is active.
 */
bool ButtonItemStockParts::isDisable() const {
    return al::isNerve(this, &NrvButtonItemStockPartsDisable);
}

/** @brief Disables (hides) the box. */
void ButtonItemStockParts::setDisable() {
    al::setNerve(this, &NrvButtonItemStockPartsDisable);
}

/**
 * @brief Starts the blur effect of a newly stocked item flying from the actor into the box.
 * @param pItem Stocked item actor (unused).
 * @param pActor Actor the effect starts from (can be null).
 * @param itemType Stocked item type (unused).
 */
void ButtonItemStockParts::stockItem(const al::LiveActor* pItem, const al::LiveActor* pActor,
                                     s32 itemType) {
    if (al::isNerve(this, &NrvButtonItemStockPartsDisable)) {
        return;
    }

    if (!isExistStockItem()) {
        return;
    }

    if (pActor != nullptr) {
        sead::Vector2f layoutPos = sead::Vector2f::zero;
        sead::Vector3f trans = al::getTrans(pActor);
        trans.y += StockBlurEffect::getEffectStartOffsetY();
        al::calcLayoutPosFromWorldPos(&layoutPos, pActor, trans, 0);
        f32 posZ = StockBlurEffect::getEffectStartPosZ();

        if (mBlurEffects != nullptr) {
            sead::Vector3f startPos(layoutPos.x, layoutPos.y, posZ);
            takeBlurEffect()->start(startPos, sead::Vector3f::zero);
        }
    }

    if (al::isNerve(this, &NrvButtonItemStockPartsNoItemHide) ||
        al::isNerve(this, &NrvButtonItemStockPartsNoItemWait)) {
        al::setNerve(this, &NrvButtonItemStockPartsStockEffectFromNoItem);
    } else {
        al::setNerve(this, &NrvButtonItemStockPartsStockEffect);
    }
}

/** @brief Starts a demo: the box stops reacting to input. */
void ButtonItemStockParts::startDemo() {
    mIsDemo = true;

    if (!al::isNerve(this, &NrvButtonItemStockPartsDisable)) {
        al::setNerve(this, getWaitNerve());
    }
}

/** @brief Ends a demo: the box reacts to input again. */
void ButtonItemStockParts::endDemo() {
    mIsDemo = false;

    if (!al::isNerve(this, &NrvButtonItemStockPartsDisable)) {
        al::setNerve(this, getWaitNerve());
    }
}

/**
 * @brief Uses the stocked item, or plays the invalid sound if it cannot be used.
 * @param port Controller port that uses the item.
 * @return Whether the item is used.
 */
bool ButtonItemStockParts::useItem(s32 port) {
    if (mItemDirector != nullptr && !mItemDirector->useStockItem(port, 0)) {
        al::startSe(this, "Invalid");
        return false;
    }

    al::setNerve(this, &NrvButtonItemStockPartsUse);
    return true;
}

/** @brief Hides the box. */
void ButtonItemStockParts::exeDisable() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
    }
}

/** @brief Waits during a demo. */
void ButtonItemStockParts::exeDemoWait() {
    if (al::isFirstStep(this)) {
    }
}

/** @brief Waits in a stage for the touch or button input that uses the stocked item. */
void ButtonItemStockParts::exeWait() {
    if (al::isFirstStep(this)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRootNoRecursive(this);
        }

        updateItemAction();
        mTouch->reset();
        al::startAction(this, "StageSceneBox", "BoxType");
        al::startAction(this, "Wait", "Main");
    }

    if (!mIsDemo) {
        mTouch->update();

        if (mTouch->isTrigerDecide()) {
            if (useItem(al::getMainControllerPort())) {
                al::startSe(this, "Release");
            } else {
                mTouch->reset();
            }

            return;
        }

        for (s32 port = 1; port <= cPadPortNum; port++) {
            bool isTrigger;
            if (al::isPadTypeJoySingle(port)) {
                isTrigger = al::isPadTriggerX(port);
            } else {
                isTrigger = al::isPadTriggerUp(port);
            }

            if (isTrigger && tryUseItemByPad(port)) {
                return;
            }
        }

        if (al::isPadTriggerUp(cHandheldPort) && tryUseItemByPad(cHandheldPort)) {
            return;
        }
    }

    updateBox();
}

/** @brief Updates the item pictures of the box slots from the stocked items. */
void ButtonItemStockParts::updateItemAction() {
    for (s32 i = 0; i < cItemSlotNum; i++) {
        s32 itemType;
        if (mIsCourseSelect) {
            itemType = GameDataFunction::getStockItemInCourseSelect(mParentLayout, i);
        } else {
            itemType = GameDataFunction::getStockItem(mParentLayout, i);
        }

        al::startAction(this, getItemActionName(itemType), al::StringTmp<32>("Item%d", i + 1).cstr());
    }
}

/** @brief Waits on the course select screen. */
void ButtonItemStockParts::exeWaitCourseSelect() {
    if (al::isFirstStep(this)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRootNoRecursive(this);
        }

        updateItemAction();
        al::startAction(this, "CourseSelectSceneBox", "BoxType");
        al::startAction(this, "WaitCourseSelect", "Main");
    }

    updateBox();
}

/** @brief Hides the box after the last stocked item was used. */
void ButtonItemStockParts::exeNoItemHide() {
    if (al::isFirstStep(this)) {
        updateItemAction();
        al::startAction(this, "End", "Main");
    } else if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, &NrvButtonItemStockPartsNoItemWait);
    }
}

/** @brief Waits hidden while no item is stocked. */
void ButtonItemStockParts::exeNoItemWait() {
    if (al::isFirstStep(this)) {
        updateItemAction();
        al::startAction(this, "NoItemWait", "Main");
    }
}

/** @brief Waits for the blur effect to reach the box, showing the box first if it was hidden. */
void ButtonItemStockParts::exeStockEffect() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvButtonItemStockPartsStockEffectFromNoItem)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRootNoRecursive(this);
        }

        updateIcon();
        updateBox();
        al::startAction(this, mIsCourseSelect ? "CourseSelectSceneBox" : "StageSceneBox",
                        "BoxType");
        al::startAction(this, "NoItem", "Item1");
        al::startAction(this, "Appear", "Main");
    }

    if (al::isGreaterEqualStep(this, StockBlurEffect::getEffectFrameNum() +
                                         StockBlurEffect::getEffectDelayFrames())) {
        al::setNerve(this, &NrvButtonItemStockPartsStock);
    }
}

/** @brief Plays the stock animation, effect and sound, then waits again. */
void ButtonItemStockParts::exeStock() {
    if (al::isFirstStep(this)) {
        updateItemAction();
        al::emitEffect(mParentLayout, "StockIn", nullptr);
        al::startAction(this, mIsCourseSelect ? "CourseSelectSceneBox" : "StageSceneBox",
                        "BoxType");
        al::startAction(this, "Stock", "Main");
        al::startSe(this, "Stock");
    }

    if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, getStockWaitNerve());
    }
}

/** @brief Plays the touch decide animation, then waits again or hides the box if empty. */
void ButtonItemStockParts::exeUse() {
    if (al::isFirstStep(this)) {
        updateItemAction();
    }

    mTouch->update();

    if (mTouch->isDecideEnd()) {
        if (isExistStockItem()) {
            al::setNerve(this, getStockWaitNerve());
        } else {
            al::setNerve(this, &NrvButtonItemStockPartsNoItemHide);
        }
    }
}

/**
 * @brief Checks whether an item is stocked.
 * @return Whether the first stocked item exists.
 */
inline bool ButtonItemStockParts::isExistStockItem() const {
    if (mIsCourseSelect) {
        return GameDataFunction::getStockItemInCourseSelect(mParentLayout, 0) != 0;
    }

    return GameDataFunction::getTopItem(mParentLayout) != 0;
}

/**
 * @brief Gets the waiting nerve used while an item is stocked.
 * @return The course select or stage waiting nerve.
 */
inline const al::Nerve* ButtonItemStockParts::getStockWaitNerve() const {
    if (mIsCourseSelect) {
        return &NrvButtonItemStockPartsWaitCourseSelect;
    }

    return &NrvButtonItemStockPartsWait;
}

/**
 * @brief Gets the waiting nerve for the current stock.
 * @return The waiting nerve, or the hidden no-item nerve if nothing is stocked.
 */
inline const al::Nerve* ButtonItemStockParts::getWaitNerve() const {
    if (isExistStockItem()) {
        return getStockWaitNerve();
    }

    return &NrvButtonItemStockPartsNoItemWait;
}

/** @brief Shows the box for the current number of players. */
inline void ButtonItemStockParts::updateBox() {
    s32 userNum = rc::getActiveControlUserNum(mParentLayout);
    if (mActiveUserNum == userNum) {
        return;
    }

    updateIcon();
    mActiveUserNum = userNum;
    al::startAction(this, al::StringTmp<32>("Box%d", userNum).cstr(), "Box");
}

/**
 * @brief Takes a free blur effect, or stops and takes the one closest to its end.
 * @return The blur effect to start.
 */
ALWAYS_INLINE inline StockBlurEffect* ButtonItemStockParts::takeBlurEffect() {
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
 * @brief Uses the stocked item with a button of a controller.
 * @param port Controller port.
 * @return Whether the item is used.
 */
inline bool ButtonItemStockParts::tryUseItemByPad(s32 port) {
    if (!useItem(port)) {
        return false;
    }

    mTouch->startDecide();
    al::startSe(this, "Release");
    return true;
}
