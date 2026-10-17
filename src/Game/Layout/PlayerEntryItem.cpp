#include "Layout/PlayerEntryItem.hpp"

#include <controller/nin/seadNinJoyNpadDevice.h>
#include <controller/seadControllerMgr.h>
#include <prim/seadSafeString.h>

#include "Layout/LayoutFontUtil.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "Layout/PlayerEntryMini.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Controller/NpadController.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Message/MessageHolder.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerKeyConfig.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"

namespace nn::hid {
void StopLrAssignmentMode();
}  // namespace nn::hid

/// Declares a nerve named Action that runs PlayerEntryItem::exe##Exe (several nerves share an exe).
#define PLAYER_ENTRY_ITEM_NERVE(Action, Exe)                                                      \
    class PlayerEntryItemNrv##Action : public al::Nerve {                                          \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            pKeeper->getParent<PlayerEntryItem>()->exe##Exe();                                     \
        }                                                                                          \
    };

namespace {
PLAYER_ENTRY_ITEM_NERVE(WaitEntry, WaitEntry)
NERVE_DECL(PlayerEntryItem, Hide)
NERVE_DECL(PlayerEntryItem, PlayWait)
PLAYER_ENTRY_ITEM_NERVE(EntryAppear, EntryAppear)
NERVE_DECL(PlayerEntryItem, PlayAppear)
PLAYER_ENTRY_ITEM_NERVE(EntryWait, EntryWait)
PLAYER_ENTRY_ITEM_NERVE(EntryWaitNoPad, EntryWait)
NERVE_DECL(PlayerEntryItem, End)
PLAYER_ENTRY_ITEM_NERVE(WaitEntryInactive, WaitEntry)
PLAYER_ENTRY_ITEM_NERVE(WaitEntryConnect, WaitEntry)
PLAYER_ENTRY_ITEM_NERVE(EntryAppearLrAssign, EntryAppear)
NERVE_DECL(PlayerEntryItem, EntryEnd)
NERVE_DECL(PlayerEntryItem, SelectEnd)
PLAYER_ENTRY_ITEM_NERVE(DecideEndNotActive, DecideEnd)
NERVE_DECL(PlayerEntryItem, ShuffleEnd)
PLAYER_ENTRY_ITEM_NERVE(CursorMoveLeft, CursorMove)
PLAYER_ENTRY_ITEM_NERVE(CursorMoveRight, CursorMove)
NERVE_DECL(PlayerEntryItem, Shuffle)
PLAYER_ENTRY_ITEM_NERVE(DecideEnd, DecideEnd)
PLAYER_ENTRY_ITEM_NERVE(SelectAppearFromPlay, SelectAppear)
NERVE_DECL(PlayerEntryItem, PlayEnd)
NERVE_DECL(PlayerEntryItem, HiddenAppear)
PLAYER_ENTRY_ITEM_NERVE(SelectAppear, SelectAppear)
PLAYER_ENTRY_ITEM_NERVE(SelectAppearFromEntry, SelectAppear)
NERVE_DECL(PlayerEntryItem, SelectWait)
PLAYER_ENTRY_ITEM_NERVE(Decide, Decide)
PLAYER_ENTRY_ITEM_NERVE(DecideShuffle, Decide)

NERVES_MAKE_NOSTRUCT(PlayerEntryItem, WaitEntry, Hide, PlayWait, EntryAppear, PlayAppear, EntryWait,
                     EntryWaitNoPad, End, WaitEntryInactive, WaitEntryConnect,
                     EntryAppearLrAssign, EntryEnd, SelectEnd, DecideEndNotActive, ShuffleEnd,
                     CursorMoveLeft, CursorMoveRight, Shuffle, DecideEnd, SelectAppearFromPlay,
                     PlayEnd, HiddenAppear, SelectAppear, SelectAppearFromEntry, SelectWait,
                     Decide, DecideShuffle)

typedef sead::WFormatFixedSafeString<6> CharacterFontString;
typedef sead::WFormatFixedSafeString<16> EnterMessageString;

/**
 * @brief Looks up a player-entry window message.
 * @param pMessageSystem Message system user (the entry window).
 * @param pLabel Label suffix after "PlayerEntryMiniParts_".
 * @return The message text.
 */
const char16_t* getEntryMessage(const al::IUseMessageSystem* pMessageSystem, const char* pLabel) {
    al::StringTmp<64> label("PlayerEntryMiniParts_");
    label.append(pLabel);
    return al::getSystemMessageString(pMessageSystem, "PlayerEntryMessage", label.cstr());
}

/**
 * @brief Checks the shoulder-button combination that assigns a single Joy-Con.
 * @param port Controller port.
 * @return Whether one shoulder button is held while the opposite one is pressed.
 */
bool isPadTriggerLrAssign(s32 port) {
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

/**
 * @brief Switches the item's Npad to single Joy-Con assignment.
 * @param pItem Entry item whose controller is switched.
 */
void setNpadJoyAssignmentModeSingle(const PlayerEntryItem* pItem) {
    sead::ControllerMgr* controllerMgr = sead::ControllerMgr::instance();
    auto* device = controllerMgr->getControlDeviceAs<sead::NinJoyNpadDevice*>();
    auto* controller =
        static_cast<al::NpadController*>(controllerMgr->getController(pItem->getPortNum()));
    device->setNpadJoyAssignmentModeSingle(controller->getNpadId());
}

/**
 * @brief Picks the listed character type closest at or after the current one.
 * @param characterType Current character type (negative when none).
 * @param pList Ascending list of unused character types.
 * @param num Number of entries in the list (at least one).
 * @return The chosen character type.
 */
s32 findNotUseCharacterType(s32 characterType, const s32* pList, s32 num) {
    if (characterType >= 0) {
        for (s32 i = 0; i < num; i++) {
            if (pList[i] == characterType) {
                return characterType;
            }

            if (pList[i] > characterType) {
                return pList[i];
            }
        }
    }

    return pList[0];
}

/**
 * @brief Picks the unused character type closest at or after the current one.
 * @param characterType Current character type (negative when none).
 * @param pHolder Game data holder.
 * @param pNum Receives the number of unused character types.
 * @return The chosen character type, or -1 if every character is taken.
 */
s32 calcNotUseCharacterType(s32 characterType, const GameDataHolder* pHolder, s32* pNum) {
    s32 list[8];
    s32 num = PlayerEntryFunction::calcNotUsePlayerCharacterTypeList(list, pHolder);
    *pNum = num;

    if (num < 1) {
        return -1;
    }

    return findNotUseCharacterType(characterType, list, num);
}

/**
 * @brief Picks the character to show in the select, skipping characters already taken.
 * @param characterType Current character type (negative when none).
 * @param pHolder Game data holder.
 * @param pIsSingleChoice Receives whether exactly one character is left.
 * @return The chosen character type, or -1 if every character is taken.
 */
s32 calcSelectCharacterType(s32 characterType, const GameDataHolder* pHolder,
                            bool* pIsSingleChoice) {
    s32 list[8];
    s32 num = PlayerEntryFunction::calcNotUsePlayerCharacterTypeList(list, pHolder);

    if (num < 1) {
        *pIsSingleChoice = false;
        return -1;
    }

    *pIsSingleChoice = num == 1;
    return findNotUseCharacterType(characterType, list, num);
}

/**
 * @brief Shows a character's picture font in a text pane.
 * @param pActor Layout to write to.
 * @param pPaneName Text pane name.
 * @param characterType Character type.
 */
void setCharacterFont(al::LayoutActor* pActor, const char* pPaneName, s32 characterType) {
    al::setPaneString(pActor, pPaneName,
                      CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(characterType))
                          .cstr());
}

/**
 * @brief Shows a Captain Toad brigade member's picture font on both character panes.
 * @param pLayout Layout to write to.
 * @param index Brigade member index.
 */
void setKinopioCharacterFont(al::IUseLayout* pLayout, s32 index) {
    CharacterFontString font(u"%s", LayoutFontUtil::getPictureFontKinopioBrigade(index));
    al::setPaneString(pLayout, "TxtCharacterL", font.cstr());
    al::setPaneString(pLayout, "TxtCharacterR", font.cstr());
}

/**
 * @brief Sets the sequence variables of the course-start jingle.
 * @param pEntryMini Entry window that plays the jingle.
 * @param character Character (or port) the jingle is voiced for.
 * @param userId Control user index.
 */
void setCourseStartSeParam(PlayerEntryMini* pEntryMini, s32 character, s32 userId) {
    al::setSeSeqLocalVariableDefault(pEntryMini, 0, character);
    al::setSeSeqLocalVariableDefault(pEntryMini, 3, pEntryMini->calcActiveItemNum());
    al::setSeSeqLocalVariableDefault(pEntryMini, 4, userId);
}

/**
 * @brief Starts the neutral entry color animation.
 * @param pActor Layout to animate.
 */
void startColorEntryAction(al::LayoutActor* pActor) {
    al::startAction(pActor, "ColorEntry", "Color");
}
}  // namespace

/**
 * @brief Creates the entry slot of one controller user.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Name of the layout parts pane.
 * @param pEntryMini Owning player-entry window.
 * @param userId Control user index of this slot.
 * @param pHolder Game data holder.
 */
PlayerEntryItem::PlayerEntryItem(const al::LayoutInitInfo& rInfo, const char* pName,
                                 const char* pPartsName, PlayerEntryMini* pEntryMini, s32 userId,
                                 const GameDataHolder* pHolder)
    : al::LayoutActor(pName), mEntryMini(pEntryMini), mUserId(userId), mGameDataHolder(pHolder) {
    al::initLayoutPartsActor(this, pEntryMini, rInfo, pPartsName, nullptr);
    mKeyConfig = new PlayerKeyConfig(GameDataConst::getPadPortList()[mUserId]);
    initNerve(&NrvPlayerEntryItemWaitEntry, 0);
    al::startAction(this, "Hide", "Main");

    if (mEntryMini->isKinopioAny()) {
        setKinopioCharacterFont(this, 0);
    } else {
        al::setPaneString(
            this, "TxtCharacterL",
            CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(0)).cstr());
        al::setPaneString(
            this, "TxtCharacterR",
            CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(0)).cstr());
    }
}

/** @brief Shows the slot and picks its first state from the user's entry status. */
void PlayerEntryItem::appear() {
    al::LayoutActor::appear();
    bool isDemo = mIsDemo;
    mCharacterType = rc::getControlUserCharacterType(
        GameDataHolderAccessor(mEntryMini->getGameDataHolder()), mUserId);

    if (!isDemo) {
        bool isActive = rc::isActiveControlUser(
            GameDataHolderAccessor(mEntryMini->getGameDataHolder()), mUserId);
        bool isStageScene = mEntryMini->isStageScene();

        if (!isActive) {
            if (isStageScene || mEntryMini->isKinopioStage()) {
                if (al::isPadConnected(getPortNum())) {
                    al::setNerve(this, &NrvPlayerEntryItemEntryWait);
                } else {
                    al::setNerve(this, &NrvPlayerEntryItemEntryWaitNoPad);
                }
            } else {
                al::setNerve(this, &NrvPlayerEntryItemEntryAppear);
            }
            return;
        }

        if (isStageScene) {
            al::setNerve(this, &NrvPlayerEntryItemPlayWait);
            return;
        }

        if (mEntryMini->isKinopioStage()) {
            if (mIsPlayerActive) {
                al::setNerve(this, &NrvPlayerEntryItemPlayWait);
            } else {
                al::setNerve(this, &NrvPlayerEntryItemEntryAppear);
            }
            return;
        }

        if (!mEntryMini->isCourseSelectScene() || al::isPadConnected(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemPlayAppear);
            return;
        }
    }

    al::setNerve(this, &NrvPlayerEntryItemHide);
}

/**
 * @brief Returns the controller port bound to this slot.
 * @return Controller port.
 */
s32 PlayerEntryItem::getPortNum() const {
    return mKeyConfig->getPort();
}

/**
 * @brief Checks whether a single Joy-Con is waiting to be connected to this slot.
 * @return Whether the port waits for connection and last reported a single Joy-Con style.
 */
inline bool PlayerEntryItem::isWaitingSingleJoyConnect() const {
    return al::isPadWaitingConnect(getPortNum()) &&
           (mControllerStyle == sead::NinJoyNpadDevice::cStyle_JoyLeft ||
            mControllerStyle == sead::NinJoyNpadDevice::cStyle_JoyRight);
}

/** @brief Switches a waiting single Joy-Con to single assignment mode. */
void PlayerEntryItem::control() {
    if (isWaitingSingleJoyConnect()) {
        setNpadJoyAssignmentModeSingle(this);
    }
}

/** @brief Enters the idle end state. */
void PlayerEntryItem::end() {
    al::setNerve(this, &NrvPlayerEntryItemEnd);
}

/**
 * @brief Checks whether the slot's user has not joined.
 * @return Whether the slot is in a waiting, entry or hidden state.
 */
bool PlayerEntryItem::isNotActive() const {
    return al::isNerve(this, &NrvPlayerEntryItemHide) ||
           al::isNerve(this, &NrvPlayerEntryItemWaitEntryInactive) ||
           al::isNerve(this, &NrvPlayerEntryItemWaitEntry) ||
           al::isNerve(this, &NrvPlayerEntryItemWaitEntryConnect) ||
           al::isNerve(this, &NrvPlayerEntryItemEntryAppear) ||
           al::isNerve(this, &NrvPlayerEntryItemEntryAppearLrAssign) ||
           al::isNerve(this, &NrvPlayerEntryItemEntryWaitNoPad) ||
           al::isNerve(this, &NrvPlayerEntryItemEntryWait) ||
           al::isNerve(this, &NrvPlayerEntryItemEntryEnd) ||
           al::isNerve(this, &NrvPlayerEntryItemSelectEnd) ||
           al::isNerve(this, &NrvPlayerEntryItemDecideEndNotActive);
}

/**
 * @brief Checks whether the user has settled on a character.
 * @return Whether the slot is playing or finished shuffling.
 */
bool PlayerEntryItem::isDecided() const {
    return al::isNerve(this, &NrvPlayerEntryItemPlayWait) ||
           al::isNerve(this, &NrvPlayerEntryItemShuffleEnd);
}

/** @brief Starts the random character shuffle (or ends an inactive slot). */
void PlayerEntryItem::startShuffle() {
    if (isNotActive()) {
        al::setNerve(this, &NrvPlayerEntryItemDecideEndNotActive);
        return;
    }

    if (al::isNerve(this, &NrvPlayerEntryItemCursorMoveLeft) ||
        al::isNerve(this, &NrvPlayerEntryItemCursorMoveRight)) {
        al::startAction(this, "CharacterWait", "Character");
    }

    al::setNerve(this, &NrvPlayerEntryItemShuffle);
}

/** @brief Plays the course-start jingle and closes the entry for this slot. */
void PlayerEntryItem::startEntryEnd() {
    if (isNotActive()) {
        al::setNerve(this, &NrvPlayerEntryItemDecideEndNotActive);
        return;
    }

    PlayerEntryMini* entryMini = mEntryMini;
    GameDataHolderAccessor accessor(const_cast<GameDataHolder*>(mGameDataHolder));
    s32 port = getPortNum();
    s32 characterType = mCharacterType;
    s32 userId = mUserId;

    if (!GameDataFunction::isStageEvent(accessor,
                                        GameDataFunction::getPlayingCourseId(accessor))) {
        if (GameDataFunction::isStageGateKeeper(accessor,
                                                GameDataFunction::getPlayingCourseId(accessor))) {
            setCourseStartSeParam(entryMini, characterType, userId);
            al::startSe(entryMini, "PgCourceStartGateKeeper");
        } else if (GameDataFunction::isStageKoopaCastle(
                       accessor, GameDataFunction::getPlayingCourseId(accessor))) {
            bool isNormal = GameDataFunction::isStageKoopaCastleNormal(
                accessor, GameDataFunction::getPlayingCourseId(accessor));
            setCourseStartSeParam(entryMini, characterType, userId);

            if (isNormal) {
                al::startSe(entryMini, "PgCourceStartKoopa");
            } else {
                al::startSe(entryMini, "PgCourceStartBoss");
            }
        } else if (GameDataFunction::isStageKinopioBrigade(
                       accessor, GameDataFunction::getPlayingCourseId(accessor))) {
            setCourseStartSeParam(entryMini, port, userId);
            al::startSe(entryMini, "PgCourceStartBrigade");
        } else {
            setCourseStartSeParam(entryMini, characterType, userId);
            al::startSe(entryMini, "PgCourceStart");
        }
    }

    if (!al::isNerve(this, &NrvPlayerEntryItemShuffleEnd)) {
        al::setNerve(this, &NrvPlayerEntryItemDecideEnd);
    }
}

/**
 * @brief Marks whether the slot's player is already playing.
 * @param isActive Whether the player is active.
 */
void PlayerEntryItem::setPlayerActive(bool isActive) {
    mIsPlayerActive = isActive;
}

/**
 * @brief Checks whether the shuffle has finished.
 * @return Whether the shuffle end state is active.
 */
bool PlayerEntryItem::isShuffleEnd() const {
    return al::isNerve(this, &NrvPlayerEntryItemShuffleEnd);
}

/** @brief Hides the slot while a demo plays. */
void PlayerEntryItem::startDemo() {
    mIsDemo = true;
    al::setNerve(this, &NrvPlayerEntryItemHide);
}

/** @brief Restores the slot after a demo. */
void PlayerEntryItem::endDemo() {
    mIsDemo = false;

    if (rc::isActiveControlUser(GameDataHolderAccessor(mEntryMini->getGameDataHolder()),
                                mUserId)) {
        al::setNerve(this, &NrvPlayerEntryItemHide);
    } else {
        al::setNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
    }
}

/** @brief Hides the slot. */
void PlayerEntryItem::hide() {
    al::setNerve(this, &NrvPlayerEntryItemHide);
}

/**
 * @brief Checks whether the slot's layout should be drawn.
 * @return Whether the slot is in a visible state and no demo is playing.
 */
bool PlayerEntryItem::isShowLayout() const {
    if (al::isNerve(this, &NrvPlayerEntryItemHide)) {
        return false;
    }

    if (al::isNerve(this, &NrvPlayerEntryItemEntryWaitNoPad)) {
        return false;
    }

    if (al::isNerve(this, &NrvPlayerEntryItemEntryWait)) {
        return false;
    }

    if (al::isNerve(this, &NrvPlayerEntryItemEntryEnd)) {
        return false;
    }

    return !mIsDemo;
}

/**
 * @brief Checks whether the slot's layout is currently shown.
 * @return Whether the slot is neither hidden nor waiting for entry.
 */
bool PlayerEntryItem::isLayoutVisible() const {
    bool isVisible;

    if (al::isNerve(this, &NrvPlayerEntryItemHide)) {
        isVisible = false;
    } else if (al::isNerve(this, &NrvPlayerEntryItemWaitEntry)) {
        isVisible = false;
    } else if (al::isNerve(this, &NrvPlayerEntryItemWaitEntryConnect)) {
        isVisible = false;
    } else if (al::isNerve(this, &NrvPlayerEntryItemEntryEnd)) {
        isVisible = false;
    } else {
        isVisible = !al::isNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
    }

    return isVisible;
}

/** @brief Hides the slot whose user is being swapped away. */
void PlayerEntryItem::setChangeSrcUser() {
    al::setNerve(this, &NrvPlayerEntryItemHide);
}

/** @brief Re-reads the character of the user swapped into this slot and shows it. */
void PlayerEntryItem::setChangeDstUser() {
    mCharacterType = rc::getControlUserCharacterType(
        GameDataHolderAccessor(mEntryMini->getGameDataHolder()), mUserId);
    al::setNerve(this, &NrvPlayerEntryItemPlayAppear);
}

/** @brief Shows an entered player's character and controller, then waits. */
void PlayerEntryItem::exePlayAppear() {
    if (al::isFirstStep(this)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRoot(this);
        }

        if (mEntryMini->isPreStageWipe()) {
            al::startAction(this, "PlayAppearInWipe", "Main");
            const char16_t* button = LayoutFontUtil::getSystemFontBtnCancel(getPortNum());
            al::setPaneString(this, "TxtEnter",
                              EnterMessageString(u"%s %s", button,
                                                 getEntryMessage(mEntryMini, "Change"))
                                  .cstr());
        } else if (mEntryMini->isKinopioPreStageWipe()) {
            al::startAction(this, "PlayAppearInWipe_KinopioBrigade", "Main");
            const char16_t* button = LayoutFontUtil::getSystemFontBtnCancel(getPortNum());

            if (mEntryMini->calcActiveItemNum() >= 2) {
                al::setPaneString(this, "TxtEnter",
                                  EnterMessageString(u"%s %s", button,
                                                     getEntryMessage(mEntryMini, "Cancel"))
                                      .cstr());
            } else {
                al::setPaneString(this, "TxtEnter", EnterMessageString(u"").cstr());
                al::hidePane(this, "TxtEnter");
            }
        } else {
            al::startAction(this, "PlayAppear", "Main");
            al::startAction(this, "Hide", "Battery");
        }

        if (mEntryMini->isKinopioAny()) {
            setKinopioCharacterFont(this, mUserId);
            al::startAction(this, al::StringTmp<32>("ColorCaptainKinopio%d", mUserId + 1).cstr(),
                            "Color");
        } else {
            if (mCharacterType >= 0) {
                al::startAction(
                    this,
                    al::StringTmp<32>("Color%s",
                                      GameDataConst::getPlayerCharacterName(mCharacterType))
                        .cstr(),
                    "Color");
            } else {
                al::startAction(this, "ColorEntry", "Color");
            }

            al::setPaneString(
                this, "TxtCharacterL",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());
        }

        s32 port = getPortNum();

        if (port != 0) {
            al::startAction(this, al::StringTmp<32>("RemoconNum%d", port).cstr(), "Controller");
        } else {
            al::startAction(this, "RemoconOff0", "Controller");
        }
    }

    if (al::isActionEnd(this, "Main")) {
        if (mIsDemo) {
            al::setNerve(this, &NrvPlayerEntryItemHide);
        } else {
            al::setNerve(this, &NrvPlayerEntryItemPlayWait);
        }
    }
}

/** @brief Waits as an entered player; handles cancelling during the pre-stage wipe. */
void PlayerEntryItem::exePlayWait() {
    if (al::isFirstStep(this)) {
        mControllerStyle =
            static_cast<sead::NinJoyNpadDevice::Style>(rc::getControllerStyle(getPortNum()));
        mEntryMini->requestStopLrAssignMode();

        if (mEntryMini->isPreStageWipe() || mEntryMini->isKinopioPreStageWipe()) {
            al::startAction(this, "PlayWaitInWipe", "Main");
            const char16_t* button = LayoutFontUtil::getSystemFontBtnCancel(getPortNum());

            if (!mEntryMini->isKinopioPreStageWipe()) {
                al::setPaneString(this, "TxtEnter",
                                  EnterMessageString(u"%s %s", button,
                                                     getEntryMessage(mEntryMini, "Change"))
                                      .cstr());
            }
        } else {
            al::startAction(this, "PlayWait", "Main");
        }

        if (mEntryMini->isKinopioAny()) {
            al::startAction(this, al::StringTmp<32>("ColorCaptainKinopio%d", mUserId + 1).cstr(),
                            "Color");
            CharacterFontString font(u"%s",
                                     LayoutFontUtil::getPictureFontKinopioBrigade(mUserId));
            al::setPaneString(this, "TxtCharacterL", font.cstr());
            al::setPaneString(this, "TxtCharacterR", font.cstr());
        } else {
            al::setPaneString(
                this, "TxtCharacterL",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());

            if (mCharacterType >= 0) {
                al::startAction(
                    this,
                    al::StringTmp<32>("Color%s",
                                      GameDataConst::getPlayerCharacterName(mCharacterType))
                        .cstr(),
                    "Color");
            } else {
                al::startAction(this, "ColorEntry", "Color");
            }
        }

        s32 port = getPortNum();

        if (port != 0) {
            al::startAction(this, al::StringTmp<32>("RemoconNum%d", port).cstr(), "Controller");
        } else {
            al::startAction(this, "RemoconOff0", "Controller");
        }
    }

    if (mEntryMini->isKinopioPreStageWipe()) {
        if (mEntryMini->calcActiveItemNum() >= 2) {
            if (al::isHidePane(this, "TxtEnter")) {
                const char16_t* button = LayoutFontUtil::getSystemFontBtnCancel(getPortNum());
                al::setPaneString(this, "TxtEnter",
                                  EnterMessageString(u"%s %s", button,
                                                     getEntryMessage(mEntryMini, "Cancel"))
                                      .cstr());
                al::showPane(this, "TxtEnter");
            }
        } else {
            al::setPaneString(this, "TxtEnter", EnterMessageString(u"").cstr());

            if (!al::isHidePane(this, "TxtEnter")) {
                al::hidePane(this, "TxtEnter");
            }
        }
    }

    if (mEntryMini->isPreStageWipe() || mEntryMini->isKinopioPreStageWipe()) {
        if (rc::isPadTriggerUiCancelByPort(getPortNum())) {
            mEntryMini->playerCancel(mUserId);
            al::setNerve(this, &NrvPlayerEntryItemSelectAppearFromPlay);
        }
        return;
    }

    // The user count is queried but no longer affects the wait time.
    rc::getActiveControlUserNum(GameDataHolderAccessor(mEntryMini->getGameDataHolder()));

    if (al::isGreaterEqualStep(this, 300)) {
        al::setNerve(this, &NrvPlayerEntryItemPlayEnd);
    }
}

/** @brief Plays the slot's closing animation, then hides it. */
void PlayerEntryItem::exePlayEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "PlayEnd", "Main");
    }

    if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, &NrvPlayerEntryItemHide);
    }
}

/** @brief Keeps the slot hidden until its controller asks to join. */
void PlayerEntryItem::exeHide() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
        al::hidePaneRoot(this);
    }

    bool isConnected = al::isPadConnected(getPortNum());
    bool isActive = rc::isActiveControlUser(
        GameDataHolderAccessor(mEntryMini->getGameDataHolder()), mUserId);

    if (isConnected) {
        mControllerStyle =
            static_cast<sead::NinJoyNpadDevice::Style>(rc::getControllerStyle(getPortNum()));
    } else if (!mIsDemo && isActive && al::isPadWaitingConnect(getPortNum())) {
        bool isNoPadConnected = true;

        for (s32 i = 0; i < 4; i++) {
            bool isPortConnected = al::isPadConnected(al::getPlayerControllerPort(i));
            isNoPadConnected &= !isPortConnected;

            if (isPortConnected) {
                break;
            }
        }

        if (!isNoPadConnected) {
            mEntryMini->requestStartLrAssignMode();
            al::setNerve(this, &NrvPlayerEntryItemHiddenAppear);
        }
        return;
    }

    if (!mIsDemo && !rc::isActiveControlUser(
                        GameDataHolderAccessor(mEntryMini->getGameDataHolder()), mUserId)) {
        al::setNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
    }
}

/** @brief Waits for a controller to press a button to join. */
void PlayerEntryItem::exeWaitEntry() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Hide", "Main");
        al::hidePaneRoot(this);

        if (mEntryMini->isKinopioAny()) {
            al::startAction(this, "SetPlayerEntryCaptainKinopio", "Main");
        }
    }

    if (rc::isActiveControlUser(GameDataHolderAccessor(mEntryMini->getGameDataHolder()),
                                mUserId) ||
        rc::isDeadControlUserInStage(GameDataHolderAccessor(mEntryMini->getGameDataHolder()),
                                     mUserId)) {
        if (mEntryMini->isStageScene() || mEntryMini->isKinopioStage() ||
            mEntryMini->isCourseSelectScene()) {
            if (isWaitingSingleJoyConnect()) {
                setNpadJoyAssignmentModeSingle(this);
                return;
            }

            if (al::isPadConnected(getPortNum())) {
                al::setNerve(this, &NrvPlayerEntryItemHide);
            }
            return;
        }
    }

    if (al::isNerve(this, &NrvPlayerEntryItemWaitEntryInactive)) {
        if (!al::isPadConnected(getPortNum()) && !al::isPadWaitingConnect(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemWaitEntry);
            return;
        }

        if (isPadTriggerLrAssign(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemSelectAppear);
            return;
        }

        if (!al::isPadTriggerAny(getPortNum())) {
            return;
        }

        if (!al::isPadWaitingConnect(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemEntryAppear);
            return;
        }
    } else if (al::isNerve(this, &NrvPlayerEntryItemWaitEntry)) {
        if (al::isPadConnected(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemSelectAppear);
            return;
        }

        if (!al::isPadWaitingConnect(getPortNum())) {
            return;
        }
    } else if (!al::isPadTriggerAny(getPortNum())) {
        return;
    }

    mEntryMini->requestStartLrAssignMode();
    al::setNerve(this, &NrvPlayerEntryItemEntryAppearLrAssign);
}

/** @brief Shows the "press to join" prompt for a newly noticed controller. */
void PlayerEntryItem::exeEntryAppear() {
    if (al::isFirstStep(this)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRoot(this);
        }

        if (mEntryMini->isKinopioAny()) {
            al::startAction(this, "SetPlayerEntryCaptainKinopio", "Main");
        }

        al::startAction(this, "EntryAppear", "Main");
        al::startAction(this, "ColorEntry", "Color");

        if (getPortNum() == al::getMainControllerPort()) {
            al::startAction(this, "RemoconOff5", "Controller");
        } else {
            al::startAction(this, "RemoconOff0", "Controller");
        }
    }

    if (al::isActionEnd(this, "Main")) {
        if (al::isPadConnected(getPortNum()) ||
            al::isNerve(this, &NrvPlayerEntryItemEntryAppearLrAssign)) {
            al::setNerve(this, &NrvPlayerEntryItemEntryWait);
        } else {
            al::setNerve(this, &NrvPlayerEntryItemEntryWaitNoPad);
        }
    }
}

/** @brief Waits for the controller to join; closes the prompt after a while without input. */
void PlayerEntryItem::exeEntryWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "EntryWait", "Main");
        startColorEntryAction(this);

        if (getPortNum() == al::getMainControllerPort()) {
            al::startAction(this, "RemoconOff5", "Controller");
        } else {
            al::startAction(this, "RemoconOff0", "Controller");
        }

        mIdleFrame = 0;
    }

    if (!mEntryMini->isPreStageWipe() && !mEntryMini->isKinopioPreStageWipe()) {
        if (al::isPadTriggerAny(getPortNum())) {
            mIdleFrame = 0;
        } else {
            mIdleFrame++;
        }
    }

    if (al::isPadWaitingConnect(getPortNum()) && al::isPadTriggerAny(getPortNum())) {
        mEntryMini->requestStartLrAssignMode();
    }

    if (al::isNerve(this, &NrvPlayerEntryItemEntryWait)) {
        if (isPadTriggerLrAssign(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemSelectAppearFromEntry);
            return;
        }
    } else if (al::isPadConnected(getPortNum())) {
        al::setNerve(this, &NrvPlayerEntryItemSelectAppearFromEntry);
        return;
    }

    // The user count is queried but no longer affects the timeout.
    rc::getActiveControlUserNum(GameDataHolderAccessor(mEntryMini->getGameDataHolder()));

    if (mIdleFrame > 300) {
        al::setNerve(this, &NrvPlayerEntryItemEntryEnd);
    }
}

/** @brief Closes the join prompt and waits for the next entry. */
void PlayerEntryItem::exeEntryEnd() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "EntryEnd", "Main");
        mEntryMini->requestStopLrAssignMode();
    }

    if (al::isActionEnd(this, "Main")) {
        if (al::isPadWaitingConnect(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemWaitEntryConnect);
        } else {
            al::setNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
        }
    }
}

/** @brief Waits for a Joy-Con assigned while the slot was hidden. */
void PlayerEntryItem::exeHiddenAppear() {
    // NOTE: the pad check is passed a bool, so it always checks port 0 or 1.
    if (al::isGreaterEqualStep(this, 10) &&
        al::isPadConnected(getPortNum() != 0 || al::isPadWaitingConnect(getPortNum()))) {
        nn::hid::StopLrAssignmentMode();
        al::setNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
        return;
    }

    if (al::isGreaterEqualStep(this, 60)) {
        nn::hid::StopLrAssignmentMode();
        al::setNerve(this, &NrvPlayerEntryItemHide);
    }
}

/** @brief Opens the character select for a joining user. */
void PlayerEntryItem::exeSelectAppear() {
    if (al::isFirstStep(this)) {
        if (al::isHidePaneRoot(this)) {
            al::showPaneRoot(this);
        }

        if (al::isNerve(this, &NrvPlayerEntryItemSelectAppearFromEntry)) {
            if (mEntryMini->isKinopioAny()) {
                al::startAction(this, "SelectAppear_KinopioBrigade", "Main");
            } else {
                al::startAction(this, "EntryToSelect", "Main");
            }
        } else if (al::isNerve(this, &NrvPlayerEntryItemSelectAppearFromPlay)) {
            if (mEntryMini->isPreStageWipe()) {
                al::startAction(this, "PlayInWipeToSelect", "Main");
            } else if (mEntryMini->isKinopioPreStageWipe()) {
                al::startAction(this, "PlayInWipeToSelect_KinopioBrigade", "Main");
            }
        } else if (!mEntryMini->isPreStageWipe() && !mEntryMini->isKinopioPreStageWipe()) {
            if (mEntryMini->isKinopioAny()) {
                al::startAction(this, "SelectAppear_KinopioBrigade", "Main");
            } else {
                al::startAction(this, "SelectAppear", "Main");
            }
        }

        al::startSe(mEntryMini, "PgConnected");
        al::startAction(this, "Hide", "Battery");
        const char16_t* button = LayoutFontUtil::getSystemFontBtnDecide(getPortNum());
        al::setPaneString(
            this, "TxtEnter",
            EnterMessageString(u"%s %s", button, getEntryMessage(mEntryMini, "Decide")).cstr());

        if (al::isHidePane(this, "TxtEnter")) {
            al::showPane(this, "TxtEnter");
        }

        s32 num;
        mCharacterType = calcNotUseCharacterType(mCharacterType, mGameDataHolder, &num);

        if (num == 1) {
            al::startAction(this, "Hide", "Arrow");
        } else {
            al::startAction(this, "Wait", "Arrow");
        }

        if (mEntryMini->isKinopioAny()) {
            setKinopioCharacterFont(this, mUserId);
            al::startAction(this, "Hide", "Arrow");
        } else {
            al::setPaneString(
                this, "TxtCharacterL",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());
        }

        al::startAction(this, "ColorEntry", "Color");

        s32 port = getPortNum();

        if (port != 0) {
            al::startAction(this, al::StringTmp<32>("RemoconNum%d", port).cstr(), "Controller");
        } else {
            al::startAction(this, "RemoconOff0", "Controller");
        }
    }

    if (al::isActionEnd(this, "Main")) {
        al::setNerve(this, &NrvPlayerEntryItemSelectWait);
    }
}

/** @brief Lets the joining user pick a character, decide or cancel. */
void PlayerEntryItem::exeSelectWait() {
    s32 characterType = mCharacterType;
    bool isSingleChoice;
    mCharacterType = calcSelectCharacterType(characterType, mGameDataHolder, &isSingleChoice);

    if (al::isFirstStep(this)) {
        if (!mEntryMini->isKinopioAny()) {
            al::startAction(this, "SelectWait", "Main");
            al::setPaneString(
                this, "TxtCharacterL",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());
        }

        mIdleFrame = 0;
        al::startAction(this, "ColorEntry", "Color");
    } else if (characterType != mCharacterType && !mEntryMini->isKinopioStage()) {
        al::setPaneString(
            this, "TxtCharacterL",
            CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                .cstr());
    }

    if (isSingleChoice || mEntryMini->isKinopioAny()) {
        al::startAction(this, "Hide", "Arrow");
    } else {
        al::startAction(this, "Wait", "Arrow");
    }

    bool isCancel = rc::isPadTriggerUiCancelByPort(getPortNum());

    if (al::isPadTriggerAny(getPortNum())) {
        mIdleFrame = 0;
    } else {
        mIdleFrame++;
    }

    isCancel |= mIdleFrame > 900;
    bool isConnected = al::isPadConnected(getPortNum());

    if ((isCancel || !isConnected) &&
        (!(mEntryMini->isPreStageWipe() || mEntryMini->isKinopioPreStageWipe()) ||
         mEntryMini->calcActiveItemNum() >= 2)) {
        al::setNerve(this, &NrvPlayerEntryItemSelectEnd);
    } else if (rc::isPadTriggerUiDecideByPort(getPortNum())) {
        al::setNerve(this, &NrvPlayerEntryItemDecide);
    }

    if (!mEntryMini->isKinopioAny() && !isSingleChoice) {
        if (rc::isPadTriggerUiLeftByPort(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemCursorMoveLeft);
        } else if (rc::isPadTriggerUiRightByPort(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemCursorMoveRight);
        }
    }
}

/** @brief Closes the character select after a cancel or timeout. */
void PlayerEntryItem::exeSelectEnd() {
    if (al::isFirstStep(this)) {
        if (mEntryMini->isPreStageWipe() || mEntryMini->isKinopioPreStageWipe()) {
            al::startAction(this, "SelectToEntry", "Main");
        } else if (mEntryMini->isKinopioStage()) {
            al::startAction(this, "SelectEnd_KinopioBrigade", "Main");
        } else {
            al::startAction(this, "SelectEnd", "Main");
        }
    }

    if (al::isPadTriggerAny(getPortNum())) {
        al::setNerve(this, &NrvPlayerEntryItemSelectAppearFromEntry);
    }

    if (!al::isActionEnd(this, "Main")) {
        return;
    }

    if (mEntryMini->isPreStageWipe() || mEntryMini->isKinopioPreStageWipe()) {
        if (al::isPadConnected(getPortNum())) {
            al::setNerve(this, &NrvPlayerEntryItemEntryWait);
        } else {
            al::setNerve(this, &NrvPlayerEntryItemEntryWaitNoPad);
        }
    } else if (al::isPadConnected(getPortNum())) {
        al::setNerve(this, &NrvPlayerEntryItemWaitEntryInactive);
    } else {
        al::setNerve(this, &NrvPlayerEntryItemWaitEntry);
    }
}

/** @brief Scrolls the character select one step left or right. */
void PlayerEntryItem::exeCursorMove() {
    if (al::isFirstStep(this)) {
        s32 prevCharacterType = mCharacterType;

        if (al::isNerve(this, &NrvPlayerEntryItemCursorMoveLeft)) {
            mCharacterType =
                PlayerEntryFunction::calcPrevPlayerCharacterType(mCharacterType, mGameDataHolder);
            al::setPaneString(this, "TxtCharacterR",
                              CharacterFontString(
                                  u"%s", LayoutFontUtil::getPictureFontPlayer(prevCharacterType))
                                  .cstr());
            al::setPaneString(
                this, "TxtCharacterL",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());
            al::startAction(this, "CharacterLeft", "Character");
        } else {
            mCharacterType =
                PlayerEntryFunction::calcNextPlayerCharacterType(mCharacterType, mGameDataHolder);
            al::setPaneString(this, "TxtCharacterL",
                              CharacterFontString(
                                  u"%s", LayoutFontUtil::getPictureFontPlayer(prevCharacterType))
                                  .cstr());
            al::setPaneString(
                this, "TxtCharacterR",
                CharacterFontString(u"%s", LayoutFontUtil::getPictureFontPlayer(mCharacterType))
                    .cstr());
            al::startAction(this, "CharacterRight", "Character");
        }

        al::startSe(mEntryMini, "PgMoveCursorRemote");
        al::startSe(mEntryMini, "PgMoveCursor");
    }

    if (al::isActionEnd(this, "Character")) {
        al::LayoutActor* actor = this;
        setCharacterFont(actor, "TxtCharacterL", mCharacterType);
        al::setPaneString(actor, "TxtCharacterR", u"U");
        al::startAction(this, "CharacterWait", "Character");

        if (!mEntryMini->isKinopioAny()) {
            al::startAction(this, "Wait", "Arrow");
        }

        al::setNerve(this, &NrvPlayerEntryItemSelectWait);
    }
}

/** @brief Confirms the chosen character and registers the user. */
void PlayerEntryItem::exeDecide() {
    if (al::isFirstStep(this)) {
        mEntryMini->onDecideItem(this);
        mControllerStyle =
            static_cast<sead::NinJoyNpadDevice::Style>(rc::getControllerStyle(getPortNum()));

        if (!mEntryMini->isKinopioAny()) {
            s32 num;
            s32 characterType = calcNotUseCharacterType(mCharacterType, mGameDataHolder, &num);
            mCharacterType = characterType;
            setCharacterFont(this, "TxtCharacterL", characterType);
        }

        if (!mEntryMini->playerEntry(mUserId, mCharacterType)) {
            al::setNerve(this, &NrvPlayerEntryItemSelectWait);
            return;
        }

        if (al::isNerve(this, &NrvPlayerEntryItemDecideShuffle)) {
            al::startAction(this, "CharacterDecide", "Main");
        } else if (mEntryMini->isPreStageWipe()) {
            al::startAction(this, "CharacterDecideInWipe", "Main");
        } else {
            al::startAction(this, "CharacterDecide", "Main");
        }

        if (mEntryMini->isKinopioAny()) {
            al::startAction(this, al::StringTmp<32>("ColorCaptainKinopio%d", mUserId + 1).cstr(),
                            "Color");
        } else if (mCharacterType >= 0) {
            al::startAction(
                this,
                al::StringTmp<32>("Color%s", GameDataConst::getPlayerCharacterName(mCharacterType))
                    .cstr(),
                "Color");
        } else {
            al::startAction(this, "ColorEntry", "Color");
        }

        if (!al::isNerve(this, &NrvPlayerEntryItemDecideShuffle)) {
            al::startSe(mEntryMini, "PgDecide");
            al::setSeSeqLocalVariableDefault(mEntryMini, 0,
                                             mEntryMini->isKinopioAny() ? 3 : mCharacterType);
            al::startSe(mEntryMini, "PgJoinLater");
        }
    }

    if (al::isActionEnd(this, "Main")) {
        if (al::isNerve(this, &NrvPlayerEntryItemDecideShuffle)) {
            al::setNerve(this, &NrvPlayerEntryItemShuffleEnd);
        } else {
            al::setNerve(this, &NrvPlayerEntryItemPlayWait);
        }
    }
}

/** @brief Registers the user when the stage starts after a decision in the wipe. */
void PlayerEntryItem::exeDecideEnd() {
    if (al::isFirstStep(this) && al::isNerve(this, &NrvPlayerEntryItemDecideEnd)) {
        al::startAction(this, "PlayInWipeToPlay", "Main");
        mEntryMini->playerEntry(mUserId, mCharacterType);
        mControllerStyle =
            static_cast<sead::NinJoyNpadDevice::Style>(rc::getControllerStyle(getPortNum()));
    }

    if (mEntryMini->isKinopioPreStageWipe() && mEntryMini->calcActiveItemNum() >= 2 &&
        al::isHidePane(this, "TxtEnter")) {
        const char16_t* button = LayoutFontUtil::getSystemFontBtnCancel(getPortNum());
        al::setPaneString(
            this, "TxtEnter",
            EnterMessageString(u"%s %s", button, getEntryMessage(mEntryMini, "Cancel")).cstr());
        al::showPane(this, "TxtEnter");
    }
}

/** @brief Cycles random characters, then settles on the user's own character. */
void PlayerEntryItem::exeShuffle() {
    if (al::isFirstStep(this)) {
        startColorEntryAction(this);

        if (mEntryMini->isPreStageWipe()) {
            al::startAction(this, "PlayWait", "Main");
        }

        al::startSe(mEntryMini, "PgShuffle");
    }

    if (al::getNerveStep(this) % 3 == 0) {
        s32 typeNum = rc::calcCharacterTypeNumMax(
            GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)));
        s32 characterType = static_cast<s32>(al::getRandomNonSync(0.0f, typeNum));
        setCharacterFont(this, "TxtCharacterL", characterType);
    }

    if (al::isGreaterEqualStep(this, 120)) {
        mCharacterType = rc::getControlUserCharacterType(
            GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)), mUserId);
        setCharacterFont(this, "TxtCharacterL", mCharacterType);
        al::stopSeByName(mEntryMini, "PgShuffle");
        al::startSe(mEntryMini, "PgShuffleEnd");
        al::setNerve(this, &NrvPlayerEntryItemDecideShuffle);
    }
}

/** @brief Idles after the shuffle until the entry closes. */
void PlayerEntryItem::exeShuffleEnd() {}

/** @brief Idles after the entry window closes. */
void PlayerEntryItem::exeEnd() {}
