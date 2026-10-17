#include "Layout/CourseSelectSceneLayout.hpp"

#include <prim/seadSafeString.h>
#include "CourseSelect/CourseSelectDirector.hpp"
#include "Layout/ButtonItemStockParts.hpp"
#include "Layout/CounterPlayerParts.hpp"
#include "Layout/CourseSelectMiniatureCursor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/InputUtil.hpp"
#include "Util/LayoutUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"

namespace {
NERVE_DECL(CourseSelectSceneLayout, Wait);
NERVE_DECL(CourseSelectSceneLayout, Appear);
NERVE_DECL(CourseSelectSceneLayout, EndDemo);
NERVE_DECL(CourseSelectSceneLayout, StartDemo);
NERVE_DECL(CourseSelectSceneLayout, End);
NERVE_DECL(CourseSelectSceneLayout, PreAppear);
NERVES_MAKE_NOSTRUCT(CourseSelectSceneLayout, PreAppear, Wait, Appear, EndDemo, StartDemo, End)
}  // namespace

/** Number of frames the counters stay visible after one of them changed. */
static const s32 cCounterShowFrame = 120;

/**
 * @brief Creates the course select main layout and its counter parts.
 * @param rInfo Layout initialization context.
 * @param pGameDataHolder Game data used for the counters and the stage names.
 * @param pCursor Miniature cursor, queried for the green-star lock display.
 * @param pDirector World-map course selection director.
 * @param pNetworkSystem Network system (unused).
 * @param pErrorViewer Error viewer (unused).
 * @param pHomeButton HOME button handler (unused).
 */
CourseSelectSceneLayout::CourseSelectSceneLayout(const al::LayoutInitInfo& rInfo,
                                                 GameDataHolder* pGameDataHolder,
                                                 const CourseSelectMiniatureCursor* pCursor,
                                                 CourseSelectDirector* pDirector,
                                                 al::NetworkSystem* pNetworkSystem,
                                                 al::ErrorViewer* pErrorViewer,
                                                 al::HomeButton* pHomeButton)
    : al::LayoutActor("コースセレクト用メインレイアウト"), mCursor(pCursor), mDirector(pDirector),
      mGameDataHolder(pGameDataHolder) {
    al::initLayoutActor(this, rInfo, "CourseSelectSceneLayout", nullptr);
    initNerve(&NrvCourseSelectSceneLayoutWait, 0);
    updateGreenStarCount();
    updateIllustItemCount();
    updateCoinCount();
    al::setPaneString(this, "TxtTitle", u"", 0);
    mCounterPlayer = new CounterPlayerParts(rInfo, "カウンタープレイヤー", "ParCounterPlayer", this,
                                            nullptr, false);
    mItemStock =
        new ButtonItemStockParts(rInfo, "アイテムストック", "ParItemStock", this, nullptr, true);
    mPlayerLife = GameDataFunction::getPlayerLife(GameDataHolderAccessor(mGameDataHolder));
    al::startFreezeActionEnd(this, "End", "Main");
    al::startFreezeActionEnd(this, "EndAll", "MainAll");
    al::startFreezeActionEnd(this, "EndAll", "GreenStar");
    mWorldTextInfo.setTextBox(getLayoutKeeper()->getLayout(), "TxtWorld", 2);
    appear();
}

/** @brief Writes the total number of collected green stars to the green-star counter. */
void CourseSelectSceneLayout::updateGreenStarCount() {
    s32 num = GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(this));
    al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<8>(u"%d", num).cstr(), 0);
}

/** @brief Writes the total number of collected stamps to the stamp counter. */
void CourseSelectSceneLayout::updateIllustItemCount() {
    s32 num = GameDataFunction::calcTotalIllustItemNum(GameDataHolderAccessor(this));
    al::setPaneString(this, "TxtCounterStamp", sead::WFormatFixedSafeString<8>(u"%d", num).cstr(),
                      0);
}

/** @brief Refreshes the coin counter and keeps the counters visible when the count changed. */
void CourseSelectSceneLayout::updateCoinCount() {
    s32 num = GameDataFunction::getCoinNum(GameDataHolderAccessor(this));
    if (mCoinNum != num) {
        mCoinNum = num;
        mCounterShowFrame = cCounterShowFrame;
        al::setPaneString(this, "TxtCounterCoin",
                          sead::WFormatFixedSafeString<8>(u"%d", num).cstr(), 0);
    }
}

/** @brief Per-frame update: counters, main/green-star group animations and button guides. */
void CourseSelectSceneLayout::control() {
    mWorldTextInfo.applyFix();
    if (mCounterShowFrame > 0) {
        mCounterShowFrame--;
    }

    if ((al::isNerve(this, &NrvCourseSelectSceneLayoutAppear) ||
         al::isNerve(this, &NrvCourseSelectSceneLayoutWait) ||
         al::isNerve(this, &NrvCourseSelectSceneLayoutEndDemo)) &&
        mCounterShowFrame == 0) {
        mCounterShowFrame = 1;
    }

    s32 life = GameDataFunction::getPlayerLife(GameDataHolderAccessor(mGameDataHolder));
    if (mPlayerLife != life) {
        mPlayerLife = life;
        mCounterShowFrame = cCounterShowFrame;
    }

    updateCoinCount();
    if (!al::isNerve(this, &NrvCourseSelectSceneLayoutStartDemo) &&
        !al::isNerve(this, &NrvCourseSelectSceneLayoutEndDemo)) {
        updateMainGroupAnim();
        updateGreenStarAnim();
    }

    if (rc::isControllerAssignmentChanged()) {
        updateButtonIcons();
    }
}

/** @brief Shows the main group while the counters are visible and hides it afterwards. */
void CourseSelectSceneLayout::updateMainGroupAnim() {
    if (mCounterShowFrame > 0) {
        if (al::isActionPlaying(this, "Appear", "Main") && al::isActionEnd(this, "Main")) {
            al::startAction(this, "Wait", "Main");
        } else if (!al::isActionPlaying(this, "Appear", "Main") &&
                   !al::isActionPlaying(this, "Wait", "Main")) {
            al::startAction(this, "Appear", "Main");
        }
    } else if (!al::isActionPlaying(this, "End", "Main")) {
        al::startAction(this, "End", "Main");
    }
}

/** @brief Drives the green-star group: lock display, appearance and disappearance. */
void CourseSelectSceneLayout::updateGreenStarAnim() {
    bool isLock = mCursor->isAppearGreenStarLock();
    bool isPlayingActive = al::isActionPlaying(this, "ActiveGreenStar", "GreenStar");
    if (isLock) {
        if (isPlayingActive && al::isActionEnd(this, "GreenStar")) {
            al::startAction(this, "ActiveWait", "GreenStar");
        }

        if (!al::isActionPlaying(this, "ActiveGreenStar", "GreenStar") &&
            !al::isActionPlaying(this, "ActiveWait", "GreenStar")) {
            al::startAction(this, "ActiveGreenStar", "GreenStar");
        }
        return;
    }

    if (isPlayingActive || al::isActionPlaying(this, "ActiveWait", "GreenStar")) {
        al::startAction(this, "DeactiveGreenStar", "GreenStar");
    }

    if (al::isNerve(this, &NrvCourseSelectSceneLayoutAppear) || mCounterShowFrame > 0) {
        bool isPlayingEnd = al::isActionPlaying(this, "EndAll", "GreenStar");
        bool isPlayingAppear = al::isActionPlaying(this, "AppearAll", "GreenStar");
        if (isPlayingEnd) {
            if (!isPlayingAppear) {
                al::startAction(this, "AppearAll", "GreenStar");
            }
        } else if (isPlayingAppear && al::isActionEnd(this, "GreenStar")) {
            al::startAction(this, "Wait", "GreenStar");
        }
    } else if (al::isNerve(this, &NrvCourseSelectSceneLayoutWait)) {
        if (!al::isActionPlaying(this, "Wait", "GreenStar")) {
            al::startAction(this, "Wait", "GreenStar");
        }
    } else if (al::isNerve(this, &NrvCourseSelectSceneLayoutEnd)) {
        if ((al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar") &&
             al::isActionEnd(this, "GreenStar")) ||
            !al::isActionPlaying(this, "EndAll", "GreenStar")) {
            al::startAction(this, "EndAll", "GreenStar");
        }
    }
}

/** @brief Sets the map/menu button guides for the controller style of the first player. */
void CourseSelectSceneLayout::updateButtonIcons() {
    s32 port = rc::tryCalcPadPortByFirstActiveUser(GameDataHolderAccessor(this));
    if (port < 0) {
        return;
    }

    if (al::isPadTypeJoySingle(port)) {
        al::setPaneSystemMessage(this, "TxtMap", "CourseSelectSceneLayout",
                                 "CourseSelectSceneLayout_GuideMap_Single");
        if (al::isPadTypeJoyLeft(port)) {
            al::setPaneSystemMessage(this, "TxtMenu", "CourseSelectSceneLayout",
                                     "CourseSelectSceneLayout_GuideMenu_Minus");
        } else {
            al::setPaneSystemMessage(this, "TxtMenu", "CourseSelectSceneLayout",
                                     "CourseSelectSceneLayout_GuideMenu_Plus");
        }
    } else {
        al::setPaneSystemMessage(this, "TxtMap", "CourseSelectSceneLayout",
                                 "CourseSelectSceneLayout_GuideMap_Dual");
        al::setPaneSystemMessage(this, "TxtMenu", "CourseSelectSceneLayout",
                                 "CourseSelectSceneLayout_GuideMenu_Plus");
    }
}

/**
 * @brief Sets the world shown when no stage is selected.
 * @param worldId World number to display.
 */
void CourseSelectSceneLayout::setWorldId(s32 worldId) {
    mWorldId = worldId;
}

/** @brief Shows the selected stage's world/stage number and name, or the world name. */
void CourseSelectSceneLayout::updateWorldStageString() {
    s32 courseId = rc::getSelectMiniatureCourseId(mDirector);
    bool isNoCourseName =
        GameDataFunction::isInvalidCourseId(courseId) ||
        GameDataFunction::isStageNoCourseName(GameDataHolderAccessor(mGameDataHolder), courseId);
    if (isNoCourseName) {
        if (mIsShowStageName || mShownWorldId != mWorldId) {
            rc::setPaneWorldString(this, this, "TxtWorld", "CourseSelectSceneLayout",
                                   "CourseSelectSceneLayout_World", mWorldId, "TxtWorld_ds",
                                   false);
            al::setPaneString(this, "TxtTitle", u"", 0);
            mShownWorldId = mWorldId;
            mIsShowStageName = false;
        }
    } else if (!mIsShowStageName || mShownCourseId != courseId) {
        s32 worldId;
        s32 stageId;
        GameDataFunction::calcWorldAndStageId(GameDataHolderAccessor(this), &worldId, &stageId,
                                              courseId);
        rc::setPaneWorldStageString(this, this, "TxtWorld", "CourseSelectSceneLayout",
                                    "CourseSelectSceneLayout_WorldStage", worldId, stageId,
                                    mGameDataHolder, "TxtWorld_ds", false);
        rc::setPaneStageNameString(this, this, "TxtTitle", mGameDataHolder, courseId);
        mIsShowStageName = true;
        mShownCourseId = courseId;
    }

    // Leftover of a stripped debug string; still constructed in the shipped code.
    al::StringTmp<256> unused("");
}

/** @brief Plays the hide animations of all groups while the start demo runs. */
void CourseSelectSceneLayout::updateStartDemoAnim() {
    if (!al::isActionPlaying(this, "EndAll", "GreenStar")) {
        if (al::isActionPlaying(this, "Wait", "GreenStar")) {
            al::startAction(this, "EndAll", "GreenStar");
        } else if (al::isActionPlaying(this, "AppearAll", "GreenStar") &&
                   al::isActionEnd(this, "GreenStar")) {
            al::startAction(this, "EndAll", "GreenStar");
        } else if (al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar") &&
                   al::isActionEnd(this, "GreenStar")) {
            al::startAction(this, "EndAll", "GreenStar");
        } else if (al::isActionPlaying(this, "ActiveGreenStar", "GreenStar") ||
                   al::isActionPlaying(this, "ActiveWait", "GreenStar")) {
            al::startAction(this, "DeactiveGreenStar", "GreenStar");
        } else if (al::isActionPlaying(this, "AppearAll", "GreenStar") &&
                   al::getActionFrame(this, "GreenStar") <= 1.0f) {
            al::startFreezeActionEnd(this, "EndAll", "GreenStar");
        }
    }

    if (!al::isActionPlaying(this, "EndAll", "MainAll")) {
        if ((al::isActionPlaying(this, "AppearAll", "MainAll") &&
             al::isActionEnd(this, "MainAll")) ||
            al::isActionPlaying(this, "Wait", "MainAll")) {
            if ((al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar") &&
                 al::isActionEnd(this, "GreenStar")) ||
                !al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar")) {
                al::startAction(this, "EndAll", "MainAll");
            }
        } else if (al::isActionPlaying(this, "AppearAll", "MainAll") &&
                   al::getActionFrame(this, "MainAll") <= 1.0f) {
            al::startFreezeActionEnd(this, "EndAll", "MainAll");
        }
    }

    if (!al::isActionPlaying(this, "End", "Main")) {
        if ((al::isActionPlaying(this, "Appear", "Main") && al::isActionEnd(this, "Main")) ||
            al::isActionPlaying(this, "Wait", "Main")) {
            if ((al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar") &&
                 al::isActionEnd(this, "GreenStar")) ||
                !al::isActionPlaying(this, "DeactiveGreenStar", "GreenStar")) {
                al::startAction(this, "End", "Main");
            }
        } else if (al::isActionPlaying(this, "Appear", "Main") &&
                   al::getActionFrame(this, "Main") <= 1.0f) {
            al::startFreezeActionEnd(this, "End", "Main");
        }
    }
}

/** @brief Starts the appear animations of all groups that are not appearing yet. */
void CourseSelectSceneLayout::updateEndDemoAnim() {
    if (!al::isActionPlaying(this, "AppearAll", "MainAll")) {
        updateButtonIcons();
        al::startAction(this, "AppearAll", "MainAll");
    }

    if (!al::isActionPlaying(this, "Appear", "Main")) {
        updateButtonIcons();
        al::startAction(this, "Appear", "Main");
    }

    if (!al::isActionPlaying(this, "AppearAll", "GreenStar")) {
        al::startAction(this, "AppearAll", "GreenStar");
    }
}

/**
 * @brief Hides the layout for a demo.
 * @param isSkipAnim Whether to hide immediately instead of playing the hide animations.
 */
void CourseSelectSceneLayout::startDemo(bool isSkipAnim) {
    if (isSkipAnim) {
        al::hidePaneRootNoRecursive(this);
    }

    if (al::isNerve(this, &NrvCourseSelectSceneLayoutStartDemo)) {
        return;
    }

    if (isSkipAnim) {
        if (!al::isActionPlaying(this, "End", "Main")) {
            al::startFreezeActionEnd(this, "End", "Main");
        }

        if (!al::isActionPlaying(this, "EndAll", "MainAll")) {
            al::startFreezeActionEnd(this, "EndAll", "MainAll");
        }

        if (!al::isActionPlaying(this, "EndAll", "GreenStar")) {
            al::startFreezeActionEnd(this, "EndAll", "GreenStar");
        }
    }

    al::setNerve(this, &NrvCourseSelectSceneLayoutStartDemo);
}

/**
 * @brief Shows the layout again after a demo.
 * @param isSkipAnim Whether to snap the groups to their appeared state instead of the hidden one.
 */
void CourseSelectSceneLayout::endDemo(bool isSkipAnim) {
    if (!al::isNerve(this, &NrvCourseSelectSceneLayoutStartDemo)) {
        return;
    }

    if (al::isHidePaneRoot(this)) {
        al::showPaneRootNoRecursive(this);
    }

    if (isSkipAnim) {
        if (!al::isActionPlaying(this, "Appear", "Main")) {
            al::startFreezeActionEnd(this, "Appear", "Main");
        }

        if (!al::isActionPlaying(this, "AppearAll", "MainAll")) {
            al::startFreezeActionEnd(this, "AppearAll", "MainAll");
        }

        if (!al::isActionPlaying(this, "AppearAll", "GreenStar")) {
            al::startFreezeActionEnd(this, "AppearAll", "GreenStar");
        }
    } else {
        if (!al::isActionPlaying(this, "Appear", "Main")) {
            al::startFreezeActionEnd(this, "End", "Main");
        }

        if (!al::isActionPlaying(this, "AppearAll", "MainAll")) {
            al::startFreezeActionEnd(this, "EndAll", "MainAll");
        }

        if (!al::isActionPlaying(this, "AppearAll", "GreenStar")) {
            al::startFreezeActionEnd(this, "EndAll", "GreenStar");
        }
    }

    al::setNerve(this, &NrvCourseSelectSceneLayoutEndDemo);
}

/** @brief Plays the pause-start animation. */
void CourseSelectSceneLayout::startPause() {
    al::startAction(this, "PauseStart", "Pause");
}

/** @brief Plays the pause-end animation. */
void CourseSelectSceneLayout::endPause() {
    al::startAction(this, "PauseEnd", "Pause");
}

/** @brief Idle state: refreshes the texts and hides the layout once the player moves. */
void CourseSelectSceneLayout::exeWait() {
    if (al::isFirstStep(this)) {
        updateButtonIcons();
        al::startAction(this, "Wait", "MainAll");
    }

    updateWorldStageString();
    updateIllustItemCount();
    if (rc::isPlayerMove(mDirector)) {
        mMoveFrame++;
        if (mMoveFrame > 0) {
            al::setNerve(this, &NrvCourseSelectSceneLayoutEnd);
        }
    } else {
        mMoveFrame = 0;
    }
}

/** @brief Appear state: waits a few frames before going idle. */
void CourseSelectSceneLayout::exeAppear() {
    updateWorldStageString();
    if (al::isGreaterStep(this, 10)) {
        al::setNerve(this, &NrvCourseSelectSceneLayoutWait);
    }
}

/** @brief Waits for the player to stay still long enough before appearing. */
void CourseSelectSceneLayout::exePreAppear() {
    if (al::isFirstStep(this)) {
        updateButtonIcons();
    }

    updateWorldStageString();
    if (rc::isPlayerMove(mDirector)) {
        al::setNerve(this, &NrvCourseSelectSceneLayoutEnd);
    } else if (al::isGreaterStep(this, 30)) {
        al::setNerve(this, &NrvCourseSelectSceneLayoutAppear);
    }
}

/** @brief Hidden state while the player moves; returns once the player stops. */
void CourseSelectSceneLayout::exeEnd() {
    updateWorldStageString();
    if (!rc::isPlayerMove(mDirector)) {
        al::setNerve(this, &NrvCourseSelectSceneLayoutPreAppear);
    }
}

/** @brief Start-demo state: plays the hide animations. */
void CourseSelectSceneLayout::exeStartDemo() {
    updateStartDemoAnim();
}

/** @brief End-demo state: shows the layout again after a delay and goes idle once it appeared. */
void CourseSelectSceneLayout::exeEndDemo() {
    if (al::isStep(this, 30)) {
        updateEndDemoAnim();
        updateWorldStageString();
    }

    if (al::isGreaterStep(this, 30) && al::isActionEnd(this, "MainAll") &&
        al::isActionEnd(this, "GreenStar")) {
        al::setNerve(this, &NrvCourseSelectSceneLayoutWait);
    }
}
