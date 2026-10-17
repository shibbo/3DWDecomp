#include "Layout/ButtonGroup.hpp"

#include "Layout/ButtonCursorParts.hpp"
#include "Layout/ButtonSquareIconParts.hpp"
#include "Layout/CursorTarget.hpp"
#include "Util/InputUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Project/Base/StringUtil.hpp"

/**
 * @brief Creates the buttons and the cursor listed in the layout's "InitButtonGroup" BYAML.
 * @param rInfo Layout initialization context.
 * @param pParent Layout owning the buttons; also used to play the button sounds.
 * @param pLayoutName Layout archive holding the button group BYAML.
 * @param pCursorName Suffix of the BYAML name, or nullptr for none.
 * @param isUseIcon Whether the square buttons show their icon.
 */
ButtonGroup::ButtonGroup(const al::LayoutInitInfo& rInfo, al::LayoutActor* pParent,
                         const char* pLayoutName, const char* pCursorName, bool isUseIcon)
    : mAudioKeeper(pParent) {
    al::StringTmp<256> archivePath("LayoutData/%s", pLayoutName);
    al::Resource* resource = al::findOrCreateResource(archivePath.cstr(), nullptr);
    al::StringTmp<128> bymlName("InitButtonGroup%s", pCursorName != nullptr ? pCursorName : "");
    al::ByamlIter rootIter(resource->getByml(bymlName.cstr()));

    s32 size = rootIter.getSize();
    for (s32 i = 0; i < size; i++) {
        al::ByamlIter partsIter;
        if (!rootIter.tryGetIterByIndex(&partsIter, i)) {
            continue;
        }

        const char* partsName = nullptr;
        const char* name = nullptr;
        partsIter.tryGetStringByKey(&partsName, "PartsName");
        partsIter.tryGetStringByKey(&name, "Name");

        if (al::isEqualSubString(partsName, "Cursor")) {
            mCursor = new ButtonCursorParts(rInfo, name, partsName, pParent);
        } else if (al::isEqualSubString(partsName, "ScrollButton")) {
            continue;
        } else if (al::isEqualSubString(partsName, "Button")) {
            registerButtonLocal(
                new ButtonSquareIconParts(rInfo, name, partsName, pParent, isUseIcon));
        }
    }

    setCursorDestination(&rootIter, false);
}

/**
 * @brief Appends a button to the group.
 * @param pButton Button to register.
 */
void ButtonGroup::registerButtonLocal(CursorTarget* pButton) {
    mButtons.pushBack(pButton);
}

/**
 * @brief Reads the directional links of every button entry of a button group BYAML.
 * @param pIter Iterator over the BYAML's part entries.
 * @param isReset Whether existing links are overwritten instead of new ones appended.
 */
void ButtonGroup::setCursorDestination(al::ByamlIter* pIter, bool isReset) {
    s32 size = pIter->getSize();
    for (s32 i = 0; i < size; i++) {
        al::ByamlIter partsIter;
        if (!pIter->tryGetIterByIndex(&partsIter, i)) {
            continue;
        }

        const char* partsName = nullptr;
        const char* name = nullptr;
        partsIter.tryGetStringByKey(&partsName, "PartsName");
        partsIter.tryGetStringByKey(&name, "Name");

        if (!al::isEqualSubString(partsName, "Button")) {
            continue;
        }

        const char* upName = nullptr;
        const char* downName = nullptr;
        const char* leftName = nullptr;
        const char* rightName = nullptr;
        partsIter.tryGetStringByKey(&upName, "Up");
        partsIter.tryGetStringByKey(&downName, "Down");
        partsIter.tryGetStringByKey(&leftName, "Left");
        partsIter.tryGetStringByKey(&rightName, "Right");

        if (isReset) {
            resetDestination(name, upName, downName, leftName, rightName, true);
        } else {
            setDestination(name, upName, downName, leftName, rightName);
        }
    }
}

/** @brief Moves the selection off disabled buttons and tracks touch input on the buttons. */
void ButtonGroup::update() {
    if (mSelectedButton != nullptr && mSelectedButton->isDisable()) {
        mSelectedButton = findButtonSelectableFromCurrent();
        if (mSelectedButton != nullptr) {
            mSelectedButton->select();
            mCursor->set(mSelectedButton);
        } else {
            mCursor->hide();
        }
    }

    if (getButtonTouched() != nullptr) {
        if (!mIsTouching) {
            mCursor->set(getButtonTouched());
            mIsTouching = true;
            mTouchedButton = getButtonTouched();
            if (mSelectedButton != mTouchedButton) {
                mSelectedButton->wait();
            }

            if (mAudioKeeper != nullptr) {
                al::tryStartSe(mAudioKeeper, "ButtonTouched");
            }
        }
    } else if (mIsTouching) {
        select(mTouchedButton);
        mIsTouching = false;
        mTouchedButton = nullptr;
    }

    bool isTouchHold = getButtonTouched() != nullptr;
    if (!isTouchHold && mIsTouchHold && mAudioKeeper != nullptr) {
        al::tryStartSe(mAudioKeeper, "ButtonReleased");
    }

    mIsTouchHold = isTouchHold;
}

/**
 * @brief Finds the first enabled button, starting at the selected one and wrapping around.
 * @return The enabled button, or nullptr when every button is disabled.
 */
CursorTarget* ButtonGroup::findButtonSelectableFromCurrent() const {
    s32 size = mButtons.size();
    s32 current = mButtons.indexOf(mSelectedButton);
    for (s32 i = 0; i < size; i++) {
        s32 index = (current + i) % size;
        if (!mButtons.unsafeAt(index)->isDisable()) {
            return mButtons.at(index);
        }
    }

    return nullptr;
}

/**
 * @brief Finds the button currently touched.
 * @return The touched button, or nullptr.
 */
CursorTarget* ButtonGroup::getButtonTouched() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isTouch()) {
            return mButtons.at(i);
        }
    }

    return nullptr;
}

/**
 * @brief Moves the selection to a button of this group.
 * @param pButton Button to select; ignored when not registered.
 */
void ButtonGroup::select(CursorTarget* pButton) {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.at(i) == pButton) {
            mSelectedButton = pButton;
            pButton->select();
            return;
        }
    }
}

/**
 * @brief Updates the group and applies the default pad handling (move and decide).
 * @param port Controller port driving the cursor.
 */
void ButtonGroup::updateAndCursorDefault(s32 port) {
    setPort(port);
    update();

    if (mIsTouching || isDecideAny()) {
        return;
    }

    if (rc::isPadTriggerUiUpByPort(port)) {
        tryMove(Direction_Up);
    }

    if (rc::isPadTriggerUiDownByPort(port)) {
        tryMove(Direction_Down);
    }

    if (rc::isPadTriggerUiLeftByPort(port)) {
        tryMove(Direction_Left);
    }

    if (rc::isPadTriggerUiRightByPort(port)) {
        tryMove(Direction_Right);
    }

    if (rc::isPadTriggerUiDecideByPort(port)) {
        CursorTarget* button = mSelectedButton;
        if (button != nullptr && button->isValid()) {
            button->decide();
            if (mAudioKeeper != nullptr &&
                al::isExistSeActionNameInUserInfo(mAudioKeeper, "ButtonDecided")) {
                al::tryStartSe(mAudioKeeper, "ButtonDecided");
            }
        }
    }
}

/**
 * @brief Assigns the controller port of every button.
 * @param port Controller port.
 */
void ButtonGroup::setPort(s32 port) {
    for (s32 i = 0; i < mButtons.size(); i++) {
        mButtons.at(i)->setPort(port);
    }
}

/**
 * @brief Checks whether any button was decided.
 * @return True when a button is in its decide state.
 */
bool ButtonGroup::isDecideAny() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isDecide()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Appends a button created by the owning layout.
 * @param pParent Layout owning the button (unused).
 * @param pButton Button to register.
 */
void ButtonGroup::registerButton(al::LayoutActor* pParent, CursorTarget* pButton) {
    mButtons.pushBack(pButton);
}

/**
 * @brief Replaces the links of a button, or adds them when the button has none yet.
 * @param pButtonName Button whose links change.
 * @param pUpName Button reached with up, or nullptr.
 * @param pDownName Button reached with down, or nullptr.
 * @param pLeftName Button reached with left, or nullptr.
 * @param pRightName Button reached with right, or nullptr.
 * @param isReset Whether missing names clear the link instead of keeping the current target.
 */
void ButtonGroup::resetDestination(const char* pButtonName, const char* pUpName,
                                   const char* pDownName, const char* pLeftName,
                                   const char* pRightName, bool isReset) {
    if (isReset) {
        for (s32 i = 0; i < mDestinationInfos.size(); i++) {
            DestinationInfo* info = mDestinationInfos.at(i);
            if (info->mButton != getButton(pButtonName)) {
                continue;
            }

            info->set(getButton(pButtonName), getButton(pUpName), getButton(pDownName),
                      getButton(pLeftName), getButton(pRightName));
            return;
        }

        setDestination(pButtonName, pUpName, pDownName, pLeftName, pRightName);
    } else {
        CursorTarget* button = getButton(pButtonName);
        for (s32 i = 0; i < mDestinationInfos.size(); i++) {
            DestinationInfo* info = mDestinationInfos.at(i);
            if (info->mButton == getButton(pButtonName)) {
                info->set(getButton(pButtonName),
                          pUpName != nullptr ? getButton(pUpName) :
                                               findMoveTargetButton(button, Direction_Up),
                          pDownName != nullptr ? getButton(pDownName) :
                                                 findMoveTargetButton(button, Direction_Down),
                          pLeftName != nullptr ? getButton(pLeftName) :
                                                 findMoveTargetButton(button, Direction_Left),
                          pRightName != nullptr ? getButton(pRightName) :
                                                  findMoveTargetButton(button, Direction_Right));
                return;
            }
        }
    }

    // NOTE: a reset of a button without links reaches this a second time and adds them twice.
    setDestination(pButtonName, pUpName, pDownName, pLeftName, pRightName);
}

/**
 * @brief Adds the links of a button by name.
 * @param pButtonName Button whose links are added.
 * @param pUpName Button reached with up, or nullptr.
 * @param pDownName Button reached with down, or nullptr.
 * @param pLeftName Button reached with left, or nullptr.
 * @param pRightName Button reached with right, or nullptr.
 */
void ButtonGroup::setDestination(const char* pButtonName, const char* pUpName,
                                 const char* pDownName, const char* pLeftName,
                                 const char* pRightName) {
    auto* info = new DestinationInfo(getButton(pButtonName), getButton(pUpName),
                                     getButton(pDownName), getButton(pLeftName),
                                     getButton(pRightName));
    mDestinationInfos.pushBack(info);
}

/**
 * @brief Finds a button by name.
 * @param pButtonName Button name, or nullptr.
 * @return The button, or nullptr when none matches.
 */
CursorTarget* ButtonGroup::getButton(const char* pButtonName) const {
    if (pButtonName == nullptr) {
        return nullptr;
    }

    for (s32 i = 0; i < mButtons.size(); i++) {
        if (al::isEqualString(mButtons.unsafeAt(i)->getName(), pButtonName)) {
            return mButtons.at(i);
        }
    }

    return nullptr;
}

/**
 * @brief Adds the links of a button.
 * @param pButton Button whose links are added.
 * @param pUp Button reached with up, or nullptr.
 * @param pDown Button reached with down, or nullptr.
 * @param pLeft Button reached with left, or nullptr.
 * @param pRight Button reached with right, or nullptr.
 */
void ButtonGroup::setDestination(CursorTarget* pButton, CursorTarget* pUp, CursorTarget* pDown,
                                 CursorTarget* pLeft, CursorTarget* pRight) {
    auto* info = new DestinationInfo(pButton, pUp, pDown, pLeft, pRight);
    mDestinationInfos.pushBack(info);
}

/**
 * @brief Follows the links from a button in one direction, skipping disabled buttons.
 * @param pButton Starting button.
 * @param direction Direction to follow.
 * @return The first enabled button reached, or nullptr when the chain ends.
 */
CursorTarget* ButtonGroup::findMoveTargetButton(CursorTarget* pButton, s32 direction) {
    CursorTarget* target = pButton;
    do {
        DestinationInfo* info = findDestinationInfo(target);
        switch (direction) {
        case Direction_Up:
            target = info->mUp;
            break;
        case Direction_Down:
            target = info->mDown;
            break;
        case Direction_Left:
            target = info->mLeft;
            break;
        case Direction_Right:
            target = info->mRight;
            break;
        default:
            return nullptr;
        }

        if (target == nullptr) {
            return nullptr;
        }
    } while (target->isDisable());

    return target;
}

/** @brief Ends any touch in progress and puts every button back to its wait state. */
void ButtonGroup::reset() {
    mIsTouching = false;
    mTouchedButton = nullptr;
    for (s32 i = 0; i < mButtons.size(); i++) {
        mButtons.unsafeAt(i)->wait();
    }
}

/**
 * @brief Selects a button by name.
 * @param pButtonName Button name.
 */
void ButtonGroup::select(const char* pButtonName) {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (al::isEqualString(mButtons.unsafeAt(i)->getName(), pButtonName)) {
            mSelectedButton = mButtons.at(i);
            mSelectedButton->select();
            return;
        }
    }
}

/**
 * @brief Plays the select state of a button by index (the selection itself is unchanged).
 * @param index Registration index of the button.
 */
void ButtonGroup::select(s32 index) {
    mButtons.unsafeAt(index)->select();
}

/** @brief Puts the cursor on the selected button. */
void ButtonGroup::showCursor() {
    mCursor->set(mSelectedButton);
}

/** @brief Puts the cursor on the selected button with its appear animation. */
void ButtonGroup::showCursorAppear() {
    mCursor->setAppear(mSelectedButton);
    mSelectedButton->select();
}

/** @brief Hides the cursor. */
void ButtonGroup::hideCursor() {
    mCursor->hide();
}

/** @brief Invalidates every button. */
void ButtonGroup::invalidate() {
    for (s32 i = 0; i < mButtons.size(); i++) {
        mButtons.unsafeAt(i)->invalidate();
    }
}

/** @brief Validates every button. */
void ButtonGroup::validate() {
    for (s32 i = 0; i < mButtons.size(); i++) {
        mButtons.unsafeAt(i)->validate();
    }
}

/**
 * @brief Reloads the button links from another button group BYAML.
 * @param pLayoutName Layout archive holding the button group BYAML.
 * @param pCursorName Suffix of the BYAML name, or nullptr for none.
 */
void ButtonGroup::reloadCursorDestination(const char* pLayoutName, const char* pCursorName) {
    al::StringTmp<256> archivePath("LayoutData/%s", pLayoutName);
    al::Resource* resource = al::findOrCreateResource(archivePath.cstr(), nullptr);
    al::StringTmp<128> bymlName("InitButtonGroup%s", pCursorName != nullptr ? pCursorName : "");
    al::ByamlIter rootIter(resource->getByml(bymlName.cstr()));
    setCursorDestination(&rootIter, true);
}

/**
 * @brief Selects and decides a button by name, unless a touch is in progress.
 * @param pButtonName Button name.
 */
void ButtonGroup::decide(const char* pButtonName) {
    if (mIsTouching) {
        return;
    }

    CursorTarget* button = getButton(pButtonName);
    if (mSelectedButton != button) {
        mSelectedButton->wait();
        mSelectedButton = button;
        select(button);
        mCursor->set(mSelectedButton);
    }

    button->decide();
    if (mAudioKeeper != nullptr &&
        al::isExistSeActionNameInUserInfo(mAudioKeeper, "ButtonDecided")) {
        al::tryStartSe(mAudioKeeper, "ButtonDecided");
    }
}

/**
 * @brief Checks whether a button is selected.
 * @param pButtonName Button name.
 * @return True when the selected button has this name.
 */
bool ButtonGroup::isSelect(const char* pButtonName) const {
    return al::isEqualString(mSelectedButton->getName(), pButtonName);
}

/**
 * @brief Checks whether a button is selected.
 * @param pButton Button.
 * @return True when the button is the selected one.
 */
bool ButtonGroup::isSelect(CursorTarget* pButton) const {
    return mSelectedButton == pButton;
}

/**
 * @brief Checks whether a button was decided.
 * @param pButtonName Button name.
 * @return True when the button exists and is in its decide state.
 */
bool ButtonGroup::isDecide(const char* pButtonName) const {
    if (getButton(pButtonName) == nullptr) {
        return false;
    }

    return getButton(pButtonName)->isDecide();
}

/**
 * @brief Checks whether a button finished its decide animation.
 * @param pButtonName Button name.
 * @return True when the button exists and its decide has ended.
 */
bool ButtonGroup::isDecideEnd(const char* pButtonName) const {
    if (getButton(pButtonName) == nullptr) {
        return false;
    }

    return getButton(pButtonName)->isDecideEnd();
}

/**
 * @brief Checks whether any button finished its decide animation.
 * @return True when a button's decide has ended.
 */
bool ButtonGroup::isDecideEndAny() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isDecideEnd()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Finds the decided button.
 * @return The decided button's name, or nullptr.
 */
const char* ButtonGroup::getDecideButton() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isDecide()) {
            return mButtons.unsafeAt(i)->getName();
        }
    }

    return nullptr;
}

/**
 * @brief Finds the button whose decide animation ended.
 * @return The button's name, or nullptr.
 */
const char* ButtonGroup::getDecideEndButton() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isDecideEnd()) {
            return mButtons.unsafeAt(i)->getName();
        }
    }

    return nullptr;
}

/**
 * @brief Finds the index of the button whose decide animation ended.
 * @return The registration index, or -1.
 */
s32 ButtonGroup::getDecideEndButtonIndex() const {
    for (s32 i = 0; i < mButtons.size(); i++) {
        if (mButtons.unsafeAt(i)->isDecideEnd()) {
            return i;
        }
    }

    return -1;
}

/**
 * @brief Checks whether the cursor's appear animation finished.
 * @return True while the cursor waits after appearing.
 */
bool ButtonGroup::isWaitCursorAppear() const {
    return mCursor->isWaitAppear();
}

/**
 * @brief Reads the selected button's name.
 * @return The name.
 */
const char* ButtonGroup::getSelectedButtonName() const {
    return mSelectedButton->getName();
}

/**
 * @brief Moves the selection along a link, unless the selected button consumes the input.
 * @param direction Direction to move in.
 * @return True when the selection moved.
 */
bool ButtonGroup::tryMove(s32 direction) {
    if (mSelectedButton == nullptr) {
        return false;
    }

    switch (direction) {
    case Direction_Up:
        if (mSelectedButton->up()) {
            return false;
        }
        break;
    case Direction_Down:
        if (mSelectedButton->down()) {
            return false;
        }
        break;
    case Direction_Left:
        if (mSelectedButton->left()) {
            return false;
        }
        break;
    case Direction_Right:
        if (mSelectedButton->right()) {
            return false;
        }
        break;
    default:
        return false;
    }

    CursorTarget* target = findMoveTargetButton(mSelectedButton, direction);
    if (target == nullptr) {
        return false;
    }

    mSelectedButton->wait();
    mSelectedButton = target;
    target->select();
    mCursor->set(mSelectedButton);
    if (mAudioKeeper != nullptr) {
        al::tryStartSe(mAudioKeeper, "ButtonMoved");
    }

    return true;
}

/**
 * @brief Finds the links of a button.
 * @param pButton Button.
 * @return The button's links, or nullptr when it has none.
 */
ButtonGroup::DestinationInfo* ButtonGroup::findDestinationInfo(CursorTarget* pButton) const {
    for (s32 i = 0; i < mDestinationInfos.size(); i++) {
        if (mDestinationInfos.unsafeAt(i)->mButton == pButton) {
            return mDestinationInfos.unsafeAt(i);
        }
    }

    return nullptr;
}
