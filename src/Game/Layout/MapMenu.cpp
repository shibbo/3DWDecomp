#include "Layout/MapMenu.hpp"

#include "CourseSelect/CourseSelectDirector.hpp"
#include "Layout/ButtonCursorParts.hpp"
#include "Layout/ButtonGroup.hpp"
#include "Layout/CursorTarget.hpp"
#include "Layout/MapMenuPlayerParts.hpp"
#include "Layout/RCSControlGuideBar.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/Data/WorldInfo.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"

/// Declares a nerve of the map menu whose state function may differ from the nerve's name.
#define MAP_MENU_NERVE_DECL(Action, Func)                                                          \
    class MapMenuNrv##Action : public al::Nerve {                                                  \
    public:                                                                                        \
        void execute(al::NerveKeeper* pKeeper) const override {                                    \
            (pKeeper->getParent<MapMenu>())->exe##Func();                                          \
        }                                                                                          \
    };

namespace {
MAP_MENU_NERVE_DECL(End, End)
MAP_MENU_NERVE_DECL(Wait, Wait)
MAP_MENU_NERVE_DECL(WorldJumpStart, WorldJumpStart)
MAP_MENU_NERVE_DECL(ForceEnd, End)
MAP_MENU_NERVE_DECL(WorldJumpEnd, WorldJumpEnd)
MAP_MENU_NERVE_DECL(Appear, Appear)

NERVES_MAKE_NOSTRUCT(MapMenu, Wait, Appear, End, WorldJumpStart, ForceEnd, WorldJumpEnd)

/// Frames the world jump wipe takes to close and open.
constexpr s32 cWipeFrame = 60;

/// Frames the sequence BGM fades out over when jumping to another world.
constexpr s32 cBgmFadeFrame = 60;

/// Layout and cursor names of the map, indexed by the highest open world minus one.
const char* const cMapNames[] = {"W1", "W2", "W3", "W4", "W5", "W6",
                                 "W7", "W8", "WS", "WK", "WF", "WC"};

/// Message labels of the world buttons, indexed by world id minus one.
const char* const cWorldLabels[] = {
    "MapMenu_W1",    "MapMenu_W2",      "MapMenu_W3",    "MapMenu_W4",
    "MapMenu_W5",    "MapMenu_W6",      "MapMenu_W7",    "MapMenu_W8",
    "MapMenu_WStar", "MapMenu_WKinoko", "MapMenu_WFire", "MapMenu_WChampion"};

/// Animations sliding the menu out to or in from a neighbouring menu.
constexpr const char* cTransitionActionNames[] = {"OutLeft", "OutRight", "InLeft", "InRight"};

/**
 * @brief Finds the world a world button leads to.
 * @param pButtonName Name of the decided button.
 * @return The world id, 1 when the name is unknown.
 */
inline s32 getWorldIdFromButtonName(const char* pButtonName) {
    if (al::isEqualString(pButtonName, "ワールド1")) {
        return 1;
    }

    if (al::isEqualString(pButtonName, "ワールド2")) {
        return 2;
    }

    if (al::isEqualString(pButtonName, "ワールド3")) {
        return 3;
    }

    if (al::isEqualString(pButtonName, "ワールド4")) {
        return 4;
    }

    if (al::isEqualString(pButtonName, "ワールド5")) {
        return 5;
    }

    if (al::isEqualString(pButtonName, "ワールド6")) {
        return 6;
    }

    if (al::isEqualString(pButtonName, "ワールド7")) {
        return 7;
    }

    if (al::isEqualString(pButtonName, "ワールド8")) {
        return 8;
    }

    if (al::isEqualString(pButtonName, "ワールドS")) {
        return 9;
    }

    if (al::isEqualString(pButtonName, "ワールドK")) {
        return 10;
    }

    if (al::isEqualString(pButtonName, "ワールドF")) {
        return 11;
    }

    if (al::isEqualString(pButtonName, "ワールドC")) {
        return 12;
    }

    return 1;
}

/**
 * @brief Checks whether the Bowser castle course of a world was cleared.
 * @param accessor Game data.
 * @param worldId World to check.
 * @return True when a Bowser castle course of the world is cleared.
 */
inline bool isClearKoopaCastle(GameDataHolderAccessor accessor, s32 worldId) {
    WorldInfo* pWorldInfo = GameDataFunction::findWorldInfo(accessor, worldId);
    for (s32 i = 0; i < pWorldInfo->mStageCount; i++) {
        StageDatabaseInfo* pStageInfo = pWorldInfo->getStageInfoByIndex(i);
        if (pStageInfo->isKoopaCastle() &&
            CourseInfoFunction::isClear(accessor, pStageInfo->getCourseId())) {
            return true;
        }
    }

    return false;
}
}  // namespace

/**
 * @brief Creates the map menu.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data of the running session.
 * @param pCameraInfo Camera info of the course select scene.
 * @param pDirector Course select director of the map.
 * @param pGraphicsSystemInfo Graphics system whose lerp is cancelled after a world jump.
 * @param pGuideBar Guide bar shown below the menu.
 */
MapMenu::MapMenu(const al::LayoutInitInfo& rInfo, const GameDataHolder* pGameDataHolder,
                 al::SceneCameraInfo* pCameraInfo, CourseSelectDirector* pDirector,
                 al::GraphicsSystemInfo* pGraphicsSystemInfo, RCSControlGuideBar* pGuideBar)
    : al::LayoutActor("マップメニュー"), mGameDataHolder(const_cast<GameDataHolder*>(pGameDataHolder)),
      mSceneCameraInfo(pCameraInfo), mDirector(pDirector),
      mGraphicsSystemInfo(pGraphicsSystemInfo), mGuideBar(pGuideBar) {
    al::initLayoutActor(this, rInfo, "MapMenu", nullptr);
    initNerve(&NrvMapMenuEnd, 0);
    mOpenWorldIdMax = GameDataFunction::calcOpenWorldIdMax(GameDataHolderAccessor(this));
    mButtonGroup = new ButtonGroup(rInfo, this, "MapMenu", cMapNames[mOpenWorldIdMax - 1], false);
    mPlayerParts = new MapMenuPlayerParts(rInfo, "プレイヤーパーツ", "ParIcon", this);
    mWipe = new al::WipeSimple("コースセレクトワールドジャンプワイプ", "WipeFadeWhite", rInfo, "Map");
}

/** @brief Closes the menu. */
void MapMenu::kill() {
    al::LayoutActor::kill();
}

/** @brief The menu is driven by its nerves only. */
void MapMenu::control() {}

/** @brief Plays the appear animation, then hands the cursor to the current world. */
void MapMenu::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        mButtonGroup->reset();
        mButtonGroup->getCursorParts()->hide();
        mButtonGroup->invalidate();
        s32 userId = rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mPort);
        const char* pCharacterName =
            rc::getControlUserCharacterName(GameDataHolderAccessor(mGameDataHolder), userId);
        CursorTarget* pCurrentButton = mButtonGroup->getButton(mWorldId - 1);
        mPlayerParts->startAppear(pCharacterName, pCurrentButton);
        for (s32 i = 0; i < mButtonGroup->getButtonNum(); i++) {
            al::startAction(mButtonGroup->getButton(i), "CharacterOFF", "Mover");
        }

        al::startAction(pCurrentButton, "CharacterON", "Mover");
    }

    if (al::isActionEnd(this, nullptr)) {
        mIsActive = true;
        mButtonGroup->getCursorParts()->reset();
        mButtonGroup->select(mButtonGroup->getButton(mWorldId - 1));
        mButtonGroup->showCursor();
        s32 userId = rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mGameDataHolder), mPort);
        rc::getControlUserCharacterType(GameDataHolderAccessor(mGameDataHolder), userId);
        al::setNerve(this, &NrvMapMenuWait);
    }
}

/** @brief Moves the cursor between the worlds and handles closing and deciding the menu. */
void MapMenu::exeWait() {
    if (al::isFirstStep(this)) {
        mIsActive = true;
        mIsDecided = false;
        if (!al::isActionPlaying(this, "Wait", nullptr)) {
            al::startAction(this, "Wait", nullptr);
        }
    }

    if (al::isLessStep(
            this, al::getActionFrameMax(mButtonGroup->getCursorParts(), "Appear", "Main"))) {
        return;
    }

    if (al::isStep(this,
                   al::getActionFrameMax(mButtonGroup->getCursorParts(), "Appear", "Main"))) {
        mButtonGroup->validate();
        resetButtonValidation();
    }

    mButtonGroup->update();
    if (isInTransition()) {
        return;
    }

    if (!al::isActionPlaying(this, "Wait", nullptr)) {
        al::startAction(this, "Wait", nullptr);
    }

    if (mIsActive) {
        bool isTriggerClose = rc::isPadTriggerUiCancelByPort(mPort) ||
                              (al::isPadTypeJoySingle(mPort) ? al::isPadTriggerX(mPort) :
                                                               al::isPadTriggerMinus(mPort));
        if (isTriggerClose && !mButtonGroup->isDecideAny()) {
            mGuideBar->end();
            if (mButtonGroup->isInputLocked()) {
                mButtonGroup->getCursorParts()->hide();
                mButtonGroup->getButtonTouched()->wait();
            }

            al::startAction(this, "End", nullptr);
            al::setNerve(this, &NrvMapMenuEnd);
            return;
        }
    }

    if (mButtonGroup->isDecideAny()) {
        if (!mIsActive) {
            CursorTarget* pButton = mButtonGroup->getButton(mButtonGroup->getDecideButton());
            mButtonGroup->reset();
            if (pButton != nullptr) {
                mButtonGroup->select(pButton);
                mButtonGroup->getSelectedButton()->wait();
            }

            return;
        }

        if (!mIsDecided) {
            mIsDecided = true;
            al::tryStartSe(this, "DecideWorldWarp");
            rc::setControllerConnectDisabled(true);
        }

        if (mButtonGroup->isDecideEndAny()) {
            al::setNerve(this, &NrvMapMenuWorldJumpStart);
        }

        return;
    }

    if (!mIsActive || mButtonGroup->isInputLocked()) {
        return;
    }

    if (rc::isPadTriggerUiUpByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Up);
    }

    if (rc::isPadTriggerUiDownByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Down);
    }

    if (rc::isPadTriggerUiLeftByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Left);
    }

    if (rc::isPadTriggerUiRightByPort(mPort)) {
        mButtonGroup->tryMove(ButtonGroup::Direction_Right);
    }

    if (rc::isPadTriggerUiDecideByPort(mPort)) {
        mButtonGroup->decide(mButtonGroup->getSelectedButtonName());
    }
}

/** @brief Enables the buttons of the open worlds and invalidates the others. */
void MapMenu::resetButtonValidation() {
    for (s32 i = 0; i < GameDataFunction::getWorldNum(GameDataHolderAccessor(this)); i++) {
        CursorTarget* pButton = mButtonGroup->getButtonUnsafe(i);
        u16 worldId = i + 1;
        if (worldId > mOpenWorldIdMax) {
            pButton->invalidate();
        } else {
            pButton->validate();
            rc::setPaneWorldString(this, mButtonGroup->getButton(i), "TxtButton", "MapMenu",
                                   cWorldLabels[i], worldId, nullptr, false);
        }
    }
}

/**
 * @brief Checks whether the menu is sliding in or out next to another menu.
 * @return True while a transition animation is playing.
 */
bool MapMenu::isInTransition() const {
    if (!al::isAnyActionPlaying(this, "Main")) {
        return false;
    }

    for (const char* pActionName : cTransitionActionNames) {
        if (al::isActionPlaying(this, pActionName, "Main") && !al::isActionEnd(this, "Main")) {
            return true;
        }
    }

    return false;
}

/** @brief Closes the menu, either with its end animation or immediately when forced. */
void MapMenu::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "PgEnd");
        mButtonGroup->getCursorParts()->hide();
        mPlayerParts->startEnd();
        if (mGuideBar->isAlive()) {
            mGuideBar->end();
        }
    } else if (al::isNerve(this, &NrvMapMenuForceEnd)) {
        if (al::isHidePane(this, "MapMenu")) {
            al::showPane(this, "MapMenu");
        }

        kill();
    } else if (al::isActionEnd(this, nullptr)) {
        kill();
    }
}

/** @brief Closes the wipe and moves the player to the start of the decided world. */
void MapMenu::exeWorldJumpStart() {
    if (al::isFirstStep(this)) {
        mWipe->startClose(cWipeFrame);
        mButtonGroup->getCursorParts()->hide();
        al::requestCancelInterpole(this);
        al::requestResetUserCameraControl(this);
        al::requestResetZoomCameraControl(this);
        const char* pButtonName = mButtonGroup->getDecideEndButton();
        s32 prevWorldId = mDirector->getActiveWorldId();
        s32 worldId = getWorldIdFromButtonName(pButtonName);
        mDirector->setPlayerPositionWorldStart(worldId);
        al::changeBgmSituation(this, "HideMap");
        if (prevWorldId != worldId) {
            al::stopAllSequenceBgm(this, cBgmFadeFrame);
        }
    }

    if (mWipe->isCloseEnd()) {
        al::setNerve(this, &NrvMapMenuWorldJumpEnd);
        mGraphicsSystemInfo->cancelLerp();
        rc::setControllerConnectDisabled(false);
    }
}

/** @brief Opens the wipe in the new world and closes the menu. */
void MapMenu::exeWorldJumpEnd() {
    if (al::isFirstStep(this)) {
        mWipe->startOpen(cWipeFrame);
        mPlayerParts->startEnd();
        al::startAction(this, "End", nullptr);
    }

    if (al::isGreaterEqualStep(this, cWipeFrame)) {
        kill();
    }
}

/**
 * @brief Opens the menu.
 * @param port Controller port that opened the menu.
 * @param worldId World the player is in.
 */
void MapMenu::startAppear(s32 port, s32 worldId) {
    mOpenWorldIdMax = GameDataFunction::calcOpenWorldIdMax(GameDataHolderAccessor(this));
    mButtonGroup->reloadCursorDestination("MapMenu", cMapNames[mOpenWorldIdMax - 1]);
    mPort = port;
    mButtonGroup->setPort(port);
    al::LayoutActor::appear();
    mWorldId = mOpenWorldIdMax < worldId ? mOpenWorldIdMax : worldId;
    al::startAction(this, cMapNames[mOpenWorldIdMax - 1], "Scale");
    al::startAction(this, cMapNames[mOpenWorldIdMax - 1], "ShowHide");
    if (isClearKoopaCastle(GameDataHolderAccessor(mGameDataHolder), 1)) {
        al::startAction(this, "ShowLineW2", "LineW2");
    } else {
        al::startAction(this, "HideLineW2", "LineW2");
    }

    if (isClearKoopaCastle(GameDataHolderAccessor(mGameDataHolder), 4)) {
        al::startAction(this, "ShowLineW5", "LineW5");
    } else {
        al::startAction(this, "HideLineW5", "LineW5");
    }

    resetButtonValidation();
    al::setNerve(this, &NrvMapMenuAppear);
}

/** @brief Closes the menu immediately. */
void MapMenu::forceEnd() {
    mIsActive = false;
    mButtonGroup->invalidate();
    al::setNerve(this, &NrvMapMenuForceEnd);
}

/**
 * @brief Slides the menu out to show a neighbouring menu.
 * @param isLeft True to slide out to the left.
 */
void MapMenu::transitionOut(bool isLeft) {
    if (!mIsActive || mButtonGroup->isDecideAny() || al::isNerve(this, &NrvMapMenuWorldJumpStart) ||
        al::isNerve(this, &NrvMapMenuWorldJumpEnd)) {
        return;
    }

    if (isLeft) {
        al::startAction(this, "OutLeft", "Main");
    } else {
        al::startAction(this, "OutRight", "Main");
    }

    mPlayerParts->hide();
    if (mButtonGroup->isInputLocked()) {
        CursorTarget* pTouched = mButtonGroup->getButtonTouched();
        if (pTouched != nullptr) {
            mButtonGroup->select(pTouched);
            mButtonGroup->getSelectedButton()->wait();
        }
    }

    mButtonGroup->invalidate();
    mButtonGroup->hideCursor();
    mIsActive = false;
}

/**
 * @brief Slides the menu back in from a neighbouring menu.
 * @param isLeft True to slide in from the left.
 */
void MapMenu::transitionIn(bool isLeft) {
    CursorTarget* pSelected = mButtonGroup->getSelectedButton();
    mButtonGroup->reset();
    if (pSelected != nullptr) {
        mButtonGroup->select(pSelected);
    }

    if (isLeft) {
        al::startAction(this, "InLeft", "Main");
    } else {
        al::startAction(this, "InRight", "Main");
    }

    mPlayerParts->show();
    mButtonGroup->showCursor();
    mIsActive = true;
    mButtonGroup->validate();
    resetButtonValidation();
}

/**
 * @brief Checks whether the menu started closing.
 * @return True in the end states.
 */
bool MapMenu::isStartEnd() const {
    return al::isNerve(this, &NrvMapMenuEnd) || al::isNerve(this, &NrvMapMenuForceEnd);
}

/**
 * @brief Checks whether the menu finished closing.
 * @return True once the end animation finished, or right after a forced end.
 */
bool MapMenu::isEnd() const {
    if ((al::isNerve(this, &NrvMapMenuEnd) && al::isActionEnd(this, nullptr)) ||
        al::isNerve(this, &NrvMapMenuForceEnd)) {
        return al::isGreaterEqualStep(this, 1);
    }

    return false;
}

/**
 * @brief Checks whether a world jump started.
 * @return True from the step after the world jump was decided.
 */
bool MapMenu::isWorldJumpStart() const {
    return al::isNerve(this, &NrvMapMenuWorldJumpStart) && al::isGreaterEqualStep(this, 1);
}

/**
 * @brief Checks whether the world jump wipe finished closing.
 * @return True from the step after the wipe closed.
 */
bool MapMenu::isWorldJumpFinish() const {
    return al::isNerve(this, &NrvMapMenuWorldJumpEnd) && al::isGreaterEqualStep(this, 1);
}

/**
 * @brief Checks whether a world button was decided.
 * @return True when any button is decided.
 */
bool MapMenu::isDecideAny() const {
    return mButtonGroup->isDecideAny();
}

/**
 * @brief Sets the guide bar shown below the menu.
 * @param pGuideBar Guide bar of the scene.
 */
void MapMenu::setControlGuideBar(RCSControlGuideBar* pGuideBar) {
    mGuideBar = pGuideBar;
}

/**
 * @brief Gets the camera info of the course select scene.
 * @return The scene camera info.
 */
al::SceneCameraInfo* MapMenu::getSceneCameraInfo() const {
    return mSceneCameraInfo;
}
