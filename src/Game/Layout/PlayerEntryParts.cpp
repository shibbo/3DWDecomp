#include "Layout/PlayerEntryParts.hpp"

#include <controller/seadControllerMgr.h>
#include <prim/seadSafeString.h>

#include "Layout/LayoutFontUtil.hpp"
#include "Layout/PlayerEntry.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/PlayerEntryPlayer.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/NpadController.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/PlayerDef.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"

/// Declares a nerve named Action that runs PlayerEntryParts::exe##Exe (several nerves share an exe).
#define PLAYER_ENTRY_PARTS_NERVE(Action, Exe)                                                     \
    class PlayerEntryPartsNrv##Action : public al::Nerve {                                         \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<PlayerEntryParts>()->exe##Exe();                                    \
        }                                                                                          \
    };

namespace {
PLAYER_ENTRY_PARTS_NERVE(Entry, Entry)
PLAYER_ENTRY_PARTS_NERVE(EntryNoPad, Entry)
NERVE_DECL(PlayerEntryParts, DecideEnd)
NERVE_DECL(PlayerEntryParts, Disable)
NERVE_DECL(PlayerEntryParts, EntryEnd)
NERVE_DECL(PlayerEntryParts, DecideWait)
NERVE_DECL(PlayerEntryParts, Decide)
NERVE_DECL(PlayerEntryParts, Appear)
NERVE_DECL(PlayerEntryParts, Select)
PLAYER_ENTRY_PARTS_NERVE(MoveOutLeft, MoveOut)
PLAYER_ENTRY_PARTS_NERVE(MoveOutRight, MoveOut)
PLAYER_ENTRY_PARTS_NERVE(MoveInLeft, MoveIn)
PLAYER_ENTRY_PARTS_NERVE(MoveInRight, MoveIn)
NERVE_DECL(PlayerEntryParts, Retire)
NERVE_DECL(PlayerEntryParts, BackEntry)

NERVES_MAKE_NOSTRUCT(PlayerEntryParts, Entry, EntryNoPad, DecideEnd, Disable, EntryEnd, DecideWait,
                     Decide, Appear, Select, MoveOutLeft, MoveOutRight, MoveInLeft, MoveInRight,
                     Retire, BackEntry)

typedef sead::WFormatFixedSafeString<16> StartTextString;

/// Button icon shown in front of the start text.
enum class StartButton : s32 {
    Decide = 0,
    Cancel = 1,
};

/**
 * @brief Gets the Npad controller of a port.
 * @param port Controller port.
 * @return The Npad controller, or nullptr if the port has none.
 */
al::NpadController* getNpadController(s32 port) {
    return sead::DynamicCast<al::NpadController>(
        sead::ControllerMgr::instance()->getController(port));
}

/**
 * @brief Checks whether the Npad controller of a port is connected.
 * @param port Controller port (negative when none).
 * @return Whether the port is valid and its controller is connected.
 */
bool isNpadConnected(s32 port) {
    if (port < 0) {
        return false;
    }

    return getNpadController(port)->isConnected();
}

void setStartText(PlayerEntryParts* pParts, StartButton button, const char* pLabel,
                  PlayerEntry* pEntry);
void setRemoconNum(PlayerEntryParts* pParts, s32 port);
}  // namespace

/**
 * @brief Creates the panel of one player.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param userId Control user index of this panel.
 * @param pEntry Owning character select window.
 */
PlayerEntryParts::PlayerEntryParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                   const char* pPartsName, s32 userId, PlayerEntry* pEntry)
    : al::LayoutActor(pName), mEntry(pEntry), mUserId(userId) {
    al::initLayoutPartsActor(this, pEntry, rInfo, pPartsName, nullptr);
    initNerve(&NrvPlayerEntryPartsEntry, 0);
}

/** @brief Shows the panel and waits for the user to join. */
void PlayerEntryParts::appear() {
    al::LayoutActor::appear();
    mIsShowArrow = true;
    mIsMoving = false;

    if (isUserPadConnected()) {
        al::setNerve(this, &NrvPlayerEntryPartsEntry);
    } else {
        al::setNerve(this, &NrvPlayerEntryPartsEntryNoPad);
    }
}

/**
 * @brief Checks whether the user's controller is connected.
 * @return Whether the user has a port and its controller is connected.
 */
bool PlayerEntryParts::isUserPadConnected() const {
    return isNpadConnected(getPadPortCurrentUser());
}

/** @brief Removes the player model and closes the panel. */
void PlayerEntryParts::cancel() {
    if (mPlayer != nullptr) {
        mPlayer->kill();
    }

    al::setNerve(this, &NrvPlayerEntryPartsEntryEnd);
}

/** @brief Ends the entry: decided players start their decision, the others are disabled. */
void PlayerEntryParts::closeEntry() {
    if (isDecided()) {
        if (PlayerEntry::isUse3dPlayer()) {
            mPlayer->startDecision();
        }

        al::setNerve(this, &NrvPlayerEntryPartsDecideEnd);
    } else {
        al::setNerve(this, &NrvPlayerEntryPartsDisable);
    }
}

/**
 * @brief Checks whether the player has decided a character.
 * @return Whether the panel is in a decide state (and has its model, if 3D models are used).
 */
bool PlayerEntryParts::isDecided() const {
    if (PlayerEntry::isUse3dPlayer() && mPlayer == nullptr) {
        return false;
    }

    return al::isNerve(this, &NrvPlayerEntryPartsDecide) ||
           al::isNerve(this, &NrvPlayerEntryPartsDecideWait) ||
           al::isNerve(this, &NrvPlayerEntryPartsDecideEnd);
}

/** @brief Refreshes the button icon of the start text for the current controller. */
void PlayerEntryParts::resetButtonIcons() {
    if (al::isNerve(this, &NrvPlayerEntryPartsDecideWait)) {
        setStartText(this, StartButton::Cancel, "PlayerEntryParts_Cancel", mEntry);
    } else {
        setStartText(this, StartButton::Decide, "PlayerEntryParts_Decide", mEntry);
    }
}

namespace {
/**
 * @brief Shows a start message, prefixed by a button icon when the user's controller is connected.
 * @param pParts Panel to write to.
 * @param button Button icon to show.
 * @param pLabel Message label in "PlayerEntryMessage".
 * @param pEntry Character select window (message system user).
 */
void setStartText(PlayerEntryParts* pParts, StartButton button, const char* pLabel,
                  PlayerEntry* pEntry) {
    const char16_t* message = al::getSystemMessageString(pEntry, "PlayerEntryMessage", pLabel);

    if (!pParts->isUserPadConnected()) {
        al::setPaneString(pParts, "TxtStart", message);
        return;
    }

    s32 port = pParts->getPadPortCurrentUser();
    const char16_t* buttonFont = nullptr;

    switch (button) {
    case StartButton::Decide:
        buttonFont = LayoutFontUtil::getSystemFontBtnDecide(port);
        break;
    case StartButton::Cancel:
        buttonFont = LayoutFontUtil::getSystemFontBtnCancel(port);
        break;
    }

    al::setPaneString(pParts, "TxtStart", StartTextString(u"%s %s", buttonFont, message).cstr());
}
}  // namespace

/**
 * @brief Finds the controller port the panel's user plays with.
 * @return The port whose controller is assigned to the user, else the user's default port.
 */
s32 PlayerEntryParts::getPadPortCurrentUser() const {
    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();

    for (s32 port = 0; port < controllerMgr->getControllerNum(); port++) {
        if (sead::DynamicCast<al::NpadController>(controllerMgr->getController(port)) == nullptr) {
            continue;
        }

        s32 modeIndex = sead::DynamicCast<al::NpadController>(controllerMgr->getController(port))
                            ->getControllerModeIndex();
        if (modeIndex < 0) {
            continue;
        }

        if (modeIndex == mUserId || (modeIndex == 8 && mUserId == 0)) {
            return port;
        }
    }

    for (s32 port = 1; port <= 4; port++) {
        s32 npadId = getNpadController(port)->getNpadId();
        if (npadId >= 0 && npadId == mUserId) {
            return port;
        }
    }

    return rc::getPadPortByUserId(mUserId);
}

/**
 * @brief Checks whether the panel waits for its user to join.
 * @return Whether the panel is in an entry state.
 */
bool PlayerEntryParts::isEntry() const {
    return al::isNerve(this, &NrvPlayerEntryPartsEntry) ||
           al::isNerve(this, &NrvPlayerEntryPartsEntryNoPad);
}

/**
 * @brief Checks whether the user is choosing a character.
 * @return Whether the panel has neither decided nor waits for its user to join.
 */
bool PlayerEntryParts::isActive() const {
    return !isDecided() && !isEntry();
}

/** @brief Lets the decided player model start its decision animation. */
void PlayerEntryParts::requestStartDecision() {
    if (isDecided() && PlayerEntry::isUse3dPlayer()) {
        mPlayer->requestStartDecision();
    }
}

/**
 * @brief Checks whether the decision can start.
 * @return Whether the decided player model has landed (always true without a model).
 */
bool PlayerEntryParts::isEnableStartDecision() const {
    if (isDecided() && PlayerEntry::isUse3dPlayer()) {
        return mPlayer->isLand();
    }

    return true;
}

/**
 * @brief Checks whether the player model finished its decision animation.
 * @return Whether the decision ended (always true without 3D models).
 */
bool PlayerEntryParts::isEndPlayerDecision() const {
    if (PlayerEntry::isUse3dPlayer()) {
        return mPlayer->isEndDecision();
    }

    return true;
}

/** @brief Waits for the user to connect a controller and join. */
void PlayerEntryParts::exeEntry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Entry", nullptr);
        s32 port = getPadPortCurrentUser();

        if (al::isNerve(this, &NrvPlayerEntryPartsEntryNoPad)) {
            al::startAction(this, "RemoconNum0", "Controller");
        } else {
            setRemoconNum(this, port);
        }
    }

    if (mIsDisableInput) {
        return;
    }

    if (al::isNerve(this, &NrvPlayerEntryPartsEntryNoPad) && isUserPadConnected()) {
        al::setNerve(this, &NrvPlayerEntryPartsAppear);
        return;
    }

    if (!al::isNerve(this, &NrvPlayerEntryPartsEntry)) {
        return;
    }

    if (isUserPadTrigJoin()) {
        al::setNerve(this, &NrvPlayerEntryPartsAppear);
        return;
    }

    if (!isUserPadConnected()) {
        al::setNerve(this, &NrvPlayerEntryPartsEntryNoPad);
    }
}

namespace {
/**
 * @brief Shows the number of the controller bound to a port.
 * @param pParts Panel to animate.
 * @param port Controller port.
 */
void setRemoconNum(PlayerEntryParts* pParts, s32 port) {
    s32 npadId = getNpadController(port)->getNpadId();
    al::startAction(pParts, al::StringTmp<32>("RemoconNum%d", npadId == 8 ? 1 : npadId + 1).cstr(),
                    "Controller");
}
}  // namespace

/**
 * @brief Checks whether the user pressed the join button combination.
 * @return Whether one shoulder button is held while the opposite one is pressed.
 */
bool PlayerEntryParts::isUserPadTrigJoin() const {
    if (!mEntry->isEnablePartsControl()) {
        return false;
    }

    s32 port = getPadPortCurrentUser();

    if (al::isPadHoldL(port) && al::isPadTriggerR(port)) {
        return true;
    }

    if (al::isPadHoldR(port) && al::isPadTriggerL(port)) {
        return true;
    }

    if (al::isPadHoldZL(port) && al::isPadTriggerZR(port)) {
        return true;
    }

    return al::isPadHoldZR(port) && al::isPadTriggerZL(port);
}

/** @brief Slides the panel in with the user's last (or next free) character. */
void PlayerEntryParts::exeAppear() {
    if (al::isFirstStep(this)) {
        mCharacterType = rc::getControlUserCharacterType(
            GameDataHolderAccessor(mEntry->getGameDataHolder()), mUserId);

        if (!PlayerEntryFunction::isEnableUsePlayerCharacterType(mCharacterType,
                                                                 mEntry->getGameDataHolder())) {
            mCharacterType = PlayerEntryFunction::calcNextPlayerCharacterType(
                mCharacterType, mEntry->getGameDataHolder());
        }

        al::setPaneString(this, "TxtCharacter",
                          rc::getPlayerCharacterMessageName(mEntry, mCharacterType));
        al::startAction(this, GameDataConst::getPlayerCharacterName(mCharacterType),
                        "CharacterSelect");
        setStartText(this, StartButton::Decide, "PlayerEntryParts_Decide", mEntry);
        al::startFreezeActionEnd(this, "RightIn", "Main");
        setRemoconNum(this, getPadPortCurrentUser());
        return;
    }

    if (al::isStep(this, 1)) {
        al::startAction(this, "Appear", nullptr);
        al::startSe(mEntry, "PgEntryPartsAppear");
        return;
    }

    updateArrow();

    if (al::isActionEnd(this, nullptr)) {
        al::setNerve(this, &NrvPlayerEntryPartsSelect);
    }
}

/**
 * @brief Gets the character the user played with last.
 * @return Character type.
 */
s32 PlayerEntryParts::getInitialCharacter() const {
    return rc::getControlUserCharacterType(GameDataHolderAccessor(mEntry->getGameDataHolder()),
                                           mUserId);
}

/**
 * @brief Sets the selected character.
 * @param characterType Character type.
 */
void PlayerEntryParts::setCharacter(s32 characterType) {
    mCharacterType = characterType;
}

/** @brief Shows the selection arrows only while more than one character is free. */
void PlayerEntryParts::updateArrow() {
    if (PlayerEntryFunction::calcNotUsePlayerCharacterTypeList(nullptr,
                                                               mEntry->getGameDataHolder()) >= 2) {
        if (!mIsShowArrow) {
            mIsShowArrow = true;
            al::startAction(this, "Wait", "Arrow");
        }
    } else if (mIsShowArrow) {
        mIsShowArrow = false;
        al::startAction(this, "Hide", "Arrow");
    }
}

/** @brief Lets the user browse the free characters and decide one. */
void PlayerEntryParts::exeSelect() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait", "Arrow");
        setStartText(this, StartButton::Decide, "PlayerEntryParts_Decide", mEntry);
    }

    if (mIsDisableInput || processDisconnects()) {
        return;
    }

    if (isUserPadTrigDecide()) {
        al::setNerve(this, &NrvPlayerEntryPartsDecide);
        return;
    }

    if (!PlayerEntryFunction::isEnableUsePlayerCharacterType(mCharacterType,
                                                             mEntry->getGameDataHolder())) {
        s32 characterType = PlayerEntryFunction::calcNextPlayerCharacterType(
            mCharacterType, mEntry->getGameDataHolder());
        PlayerEntry* entry = mEntry;
        mCharacterType = characterType;
        al::startAction(this, GameDataConst::getPlayerCharacterName(characterType),
                        "CharacterSelect");
        al::setPaneString(this, "TxtCharacter",
                          rc::getPlayerCharacterMessageName(entry, characterType));
        updateArrow();
        return;
    }

    updateArrow();

    if (!mIsShowArrow) {
        return;
    }

    if (rc::isPadTriggerLeftOrStick(getPadPortCurrentUser())) {
        mIsMoving = true;
        al::setNerve(this, &NrvPlayerEntryPartsMoveOutLeft);
        s32 characterType = PlayerEntryFunction::calcPrevPlayerCharacterType(
            mCharacterType, mEntry->getGameDataHolder());
        if (characterType >= 0) {
            mCharacterType = characterType;
        }
        return;
    }

    if (rc::isPadTriggerRightOrStick(getPadPortCurrentUser())) {
        mIsMoving = true;
        al::setNerve(this, &NrvPlayerEntryPartsMoveOutRight);
        s32 characterType = PlayerEntryFunction::calcNextPlayerCharacterType(
            mCharacterType, mEntry->getGameDataHolder());
        if (characterType >= 0) {
            mCharacterType = characterType;
        }
    }
}

/**
 * @brief Checks whether the user pressed the decide button.
 * @return Whether parts control is enabled and the decide button was pressed.
 */
bool PlayerEntryParts::isUserPadTrigDecide() const {
    if (!mEntry->isEnablePartsControl()) {
        return false;
    }

    return rc::isPadTriggerDecide(getPadPortCurrentUser());
}

/** @brief Slides the current character out of the panel. */
void PlayerEntryParts::exeMoveOut() {
    if (al::isFirstStep(this)) {
        const char* actionName =
            al::isNerve(this, &NrvPlayerEntryPartsMoveOutLeft) ? "LeftOut" : "RightOut";
        al::startAction(this, actionName, "Main");
        al::startSe(mEntry, "PgEntryPartsMoveStart");
    }

    if (al::isActionEnd(this, "Main")) {
        if (al::isNerve(this, &NrvPlayerEntryPartsMoveOutLeft)) {
            al::setNerve(this, &NrvPlayerEntryPartsMoveInLeft);
        } else {
            al::setNerve(this, &NrvPlayerEntryPartsMoveInRight);
        }
    }
}

/** @brief Slides the newly selected character into the panel. */
void PlayerEntryParts::exeMoveIn() {
    if (al::isFirstStep(this)) {
        al::startSe(mEntry, "PgEntryPartsMoveIn");
        const char* characterName = GameDataConst::getPlayerCharacterName(mCharacterType);
        al::startAction(this, characterName, "CharacterSelect");
        const char* actionName =
            al::isNerve(this, &NrvPlayerEntryPartsMoveInLeft) ? "LeftIn" : "RightIn";
        al::startAction(this, actionName, "Main");
        al::setPaneString(this, "TxtCharacter",
                          rc::getPlayerCharacterMessageName(mEntry, mCharacterType));
    }

    if (al::isActionEnd(this, "Main")) {
        mIsMoving = false;
        al::setNerve(this, &NrvPlayerEntryPartsSelect);
    }
}

/** @brief Enters the player with the selected character and plays the character's voice. */
void PlayerEntryParts::exeDecide() {
    if (al::isFirstStep(this)) {
        if (!mEntry->tryEntryPlayer(mUserId, mCharacterType)) {
            al::setNerve(this, &NrvPlayerEntryPartsMoveOutRight);
            return;
        }

        al::startSe(mEntry, "PgEntryPartsDecided");

        switch (mCharacterType) {
        case EPlayerChara::Mario:
            al::startSe(mEntry, "PgDecidePlayerVoiceMario");
            break;
        case EPlayerChara::Luigi:
            al::startSe(mEntry, "PgDecidePlayerVoiceLuigi");
            break;
        case EPlayerChara::Peach:
            al::startSe(mEntry, "PgDecidePlayerVoicePeach");
            break;
        case EPlayerChara::Kinopio:
            al::startSe(mEntry, "PgDecidePlayerVoiceKinopio");
            break;
        case EPlayerChara::Rosetta:
            al::startSe(mEntry, "PgDecidePlayerVoiceRosetta");
            break;
        default:
            break;
        }

        al::startAction(
            this,
            al::StringTmp<32>("%sDecide", GameDataConst::getPlayerCharacterName(mCharacterType))
                .cstr(),
            "CharacterSelect");
    }

    if (al::isActionEnd(this, "CharacterSelect")) {
        al::setNerve(this, &NrvPlayerEntryPartsDecideWait);
    }
}

/** @brief Shows the player model and lets the user cancel the decision once it has landed. */
void PlayerEntryParts::exeDecideWait() {
    if (al::isFirstStep(this)) {
        setStartText(this, StartButton::Cancel, "PlayerEntryParts_Cancel", mEntry);

        if (PlayerEntry::isUse3dPlayer()) {
            mPlayer = mEntry->getPlayerModel(mUserId, mCharacterType, 0);
            mPlayer->appearPlayer(mUserId);
        }
    }

    if (mIsDisableInput) {
        return;
    }

    bool isLand = PlayerEntry::isUse3dPlayer() ? mPlayer->isLand() : true;
    if (!isCancelDecideWait() || !isLand) {
        return;
    }

    switch (mCharacterType) {
    case EPlayerChara::Mario:
        al::stopSeByName(mEntry, "PgDecidePlayerVoiceMario");
        break;
    case EPlayerChara::Luigi:
        al::stopSeByName(mEntry, "PgDecidePlayerVoiceLuigi");
        break;
    case EPlayerChara::Peach:
        al::stopSeByName(mEntry, "PgDecidePlayerVoicePeach");
        break;
    case EPlayerChara::Kinopio:
        al::stopSeByName(mEntry, "PgDecidePlayerVoiceKinopio");
        break;
    case EPlayerChara::Rosetta:
        al::stopSeByName(mEntry, "PgDecidePlayerVoiceRosetta");
        break;
    default:
        break;
    }

    al::setNerve(this, &NrvPlayerEntryPartsRetire);
}

/**
 * @brief Checks whether the user pressed the cancel button while decided.
 * @return Whether parts control is enabled and the cancel button was pressed.
 */
bool PlayerEntryParts::isCancelDecideWait() const {
    if (!mEntry->isEnablePartsControl()) {
        return false;
    }

    return rc::isPadTriggerCancel(getPadPortCurrentUser());
}

/** @brief Idles after the entry closed with a decided character. */
void PlayerEntryParts::exeDecideEnd() {}

/** @brief Retires the player and returns to the character select once the model is gone. */
void PlayerEntryParts::exeRetire() {
    if (al::isFirstStep(this)) {
        if (PlayerEntry::isUse3dPlayer()) {
            mPlayer->startJumpOut();
        }

        mEntry->retirePlayer(mUserId);
        al::startSe(mEntry, "PgEntryPartsRetire");
    }

    if (PlayerEntry::isUse3dPlayer() && !mPlayer->isDead()) {
        return;
    }

    mPlayer = nullptr;
    al::startAction(this, GameDataConst::getPlayerCharacterName(mCharacterType),
                    "CharacterSelect");
    al::setNerve(this, &NrvPlayerEntryPartsSelect);
}

/** @brief Plays the closing animation after a disconnect, then waits for the user again. */
void PlayerEntryParts::exeBackEntry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "End", nullptr);
        s32 port = getPadPortCurrentUser();

        if (port < 0) {
            port = rc::getPadPortByUserId(mUserId);
        }

        setRemoconNum(this, port);
    }

    if (al::isActionEnd(this, nullptr)) {
        appear();
    }
}

/** @brief Hides the panel. */
void PlayerEntryParts::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", nullptr);
    }
}

/** @brief Idles after the entry closed without a decided character. */
void PlayerEntryParts::exeDisable() {}

/** @brief Plays the closing animation of a cancelled entry. */
void PlayerEntryParts::exeEntryEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "EntryEnd", nullptr);
    }
}

/**
 * @brief Checks whether the user pressed the cancel button.
 * @return Whether parts control is enabled and the cancel button was pressed.
 */
bool PlayerEntryParts::isUserPadTrigCancel() const {
    if (!mEntry->isEnablePartsControl()) {
        return false;
    }

    return rc::isPadTriggerCancel(getPadPortCurrentUser());
}

/**
 * @brief Gets the selected character.
 * @return Character type.
 */
s32 PlayerEntryParts::getCharacter() const {
    return mCharacterType;
}

/**
 * @brief Closes the panel when the user's controller got disconnected.
 * @return Whether the controller is disconnected.
 */
bool PlayerEntryParts::processDisconnects() {
    if (isUserPadConnected()) {
        return false;
    }

    al::setNerve(this, &NrvPlayerEntryPartsBackEntry);
    return true;
}
