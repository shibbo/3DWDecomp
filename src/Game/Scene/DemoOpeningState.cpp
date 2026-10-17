#include "Scene/DemoOpeningState.hpp"
#include "Demo/DemoPlayerController.hpp"
#include "Demo/DemoSceneActorHolder.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "Demo/DemoOpeningSwitch.hpp"
#include "Layout/Switch/DemoSkipLayout.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Library/LiveActor/Common/LiveActorKit.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/WipeSimple.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Scene/SceneUtil.hpp"
#include "Project/Bgm/BgmPlayingRequest.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DemoUtil.hpp"
namespace rc {
PlayerRetargettingSelector* createPlayerRetargettingSelector(const al::IUseSceneObjHolder* pUser);
}
namespace {
NERVE_DECL(DemoOpeningState, Start)
NERVE_DECL(DemoOpeningState, CancelDemo)
NERVE_DECL(DemoOpeningState, Fade)
class DemoOpeningStateNrvStartSecond : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DemoOpeningState>()->exeStart();
    }
};
NERVE_DECL(DemoOpeningState, RouteDokan)
NERVE_DECL(DemoOpeningState, CourseSelect)
class DemoOpeningStateNrvCourseSelectSecond : public al::Nerve {
public:
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<DemoOpeningState>()->exeCourseSelect();
    }
};
NERVE_DECL(DemoOpeningState, DemoEnd)
NERVES_MAKE_NOSTRUCT(DemoOpeningState, Start, CancelDemo, Fade, StartSecond, RouteDokan,
                     CourseSelect, CourseSelectSecond, DemoEnd)
} // namespace

DemoOpeningState::DemoOpeningState(al::Scene* pScene, const al::ActorInitInfo& rInfo,
                                   al::GraphicsSystemInfo* pGraphicsSystemInfo,
                                   sead::Matrix34f* pMtx)
    : al::NerveStateBase("オープニングデモステート"), mScene(pScene),
      mGraphicsSystemInfo(pGraphicsSystemInfo) {
    const al::LayoutInitInfo& layoutInfo = al::getLayoutInitInfo(rInfo);
    mWhiteWipe = new al::WipeSimple("白フェード", "WipeFadeWhite", layoutInfo, "Demo");
    mBlackWipe = new al::WipeSimple("黒フェードワイプ", "WipeFadeBlack", layoutInfo, "Demo");
    mSkipLayout = new DemoSkipLayout(layoutInfo, false);
    mSkipLayout->appear();
    mDemoActorHolders = new DemoSceneActorHolder*[3];
    PlayerRetargettingSelector* selector = rc::createPlayerRetargettingSelector(mScene);
    if (pMtx) {
        mDemoActorHolders[0] =
            rc::createDemoSceneHolder("DemoOpeningStage", rInfo, selector, &pMtx[0], true, 4);
        mDemoActorHolders[1] =
            rc::createDemoSceneHolder("DemoOpeningDokanStage", rInfo, selector, &pMtx[1], true, 4);
        mDemoActorHolders[2] =
            rc::createDemoSceneHolder("DemoOpeningExitStage", rInfo, selector, &pMtx[2], true, 4);
    } else {
        mDemoActorHolders[0] =
            rc::createDemoSceneHolder("DemoOpeningStage", rInfo, selector, nullptr, true, 4);
        mDemoActorHolders[1] =
            rc::createDemoSceneHolder("DemoOpeningDokanStage", rInfo, selector, nullptr, true, 4);
        mDemoActorHolders[2] =
            rc::createDemoSceneHolder("DemoOpeningExitStage", rInfo, selector, nullptr, true, 4);
    }
    mDemoActorHolders[2]->setCameraFovyDegree(25.0f);
    initNerve(&NrvDemoOpeningStateStart, 0);
    mPlayerHolder = rInfo.getActorSceneInfo().playerHolder;
}

void DemoOpeningState::appear() {
    mIsDead = false;
    al::setNerve(this, &NrvDemoOpeningStateStart);
}

void DemoOpeningState::control() {
    u64 activePorts = rc::getActiveInputPortList(GameDataHolderAccessor(mWhiteWipe));
    if (mSkipLayout->isSkip(activePorts) && !al::isNerve(this, &NrvDemoOpeningStateCancelDemo) &&
        !al::isNerve(this, &NrvDemoOpeningStateFade))
        al::setNerve(this, &NrvDemoOpeningStateCancelDemo);
}

void DemoOpeningState::exeFade() {
    if (al::isFirstStep(this)) {
        mWhiteWipe->startClose(60);
    } else if (mWhiteWipe->isCloseEnd()) {
        mDemoActorHolder->kill();
        mWhiteWipe->startOpen(45);
        al::setNerve(this, mNextNerve);
    }
}

void DemoOpeningState::exeStart() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvDemoOpeningStateStart)) {
            mGraphicsSystemInfo->cancelLerp();
            al::PlayerHolder* players = mPlayerHolder;
            if (players) {
                s32 count = al::getPlayerNumMax(players);
                for (s32 i = 0; i < count; ++i)
                    al::offAreaTarget(al::getPlayerActor(players, i));
            }
            rc::requestStartDemoPlayer(mScene);
            rc::hideDemoPlayerAll(mScene);
            rc::stopSklAnimAndDeleteEffectDemoPlayerAll(mScene);
            CourseSelectDirector* director =
                CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director && director->getDemoOpeningSwitch()) {
                s32 count = rc::getDemoPlayerNum(mScene);
                for (s32 i = 0; i < count; ++i) {
                    DemoPlayerController* player = rc::getDemoPlayer(mScene, i);
                    player->setTrans(al::getTrans(director->getDemoOpeningSwitch()));
                }
            }
            mDemoActorHolder = mDemoActorHolders[0];
            rc::tryStartDemo(mDemoActorHolder);
            director = CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director && director->getDemoOpeningSwitch())
                director->getDemoOpeningSwitch()->onSwitch(0);
            mDemoActorHolder->startAction(0, false);
            mNextNerve = &NrvDemoOpeningStateStartSecond;
            mIsRouteDokan = false;
            al::prepareBgm(mScene, al::BgmPlayingRequest("DemoOpening"));
        } else {
            CourseSelectDirector* director =
                CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director && director->getDemoOpeningSwitch())
                director->getDemoOpeningSwitch()->onSwitch(1);
            mDemoActorHolder->startAction(1, false);
            mNextNerve = &NrvDemoOpeningStateRouteDokan;
            mIsRouteDokan = true;
        }
    } else {
        if (al::isStep(this, 351))
            al::startBgm(mScene, "DemoOpening", -1, 0, -1, -1);
        if (al::isNerve(this, &NrvDemoOpeningStateStart)) {
            if (mDemoActorHolder->isActionEndCamera(-1))
                al::setNerve(this, mNextNerve);
        } else if (mDemoActorHolder->isActionEndCamera(60)) {
            al::setNerve(this, &NrvDemoOpeningStateFade);
        }
    }
}

void DemoOpeningState::exeRouteDokan() {
    if (al::isFirstStep(this)) {
        mDemoActorHolder = mDemoActorHolders[1];
        rc::tryStartDemo(mDemoActorHolder);
        CourseSelectDirector* director = CourseSelectDirector::tryGetCourseSelectDirector(mScene);
        if (director && director->getDemoOpeningSwitch())
            director->getDemoOpeningSwitch()->onSwitch(2);
        mDemoActorHolder->startAction(0, false);
        mNextNerve = &NrvDemoOpeningStateCourseSelect;
        mIsRouteDokan = true;
    }
    if (mDemoActorHolder->isActionEndCamera(60)) {
        al::PlayerHolder* players = mPlayerHolder;
        if (players) {
            s32 count = al::getPlayerNumMax(players);
            for (s32 i = 0; i < count; ++i)
                al::onAreaTarget(al::getPlayerActor(players, i));
        }
        al::setNerve(this, &NrvDemoOpeningStateFade);
    }
}

void DemoOpeningState::exeCourseSelect() {
    if (al::isFirstStep(this)) {
        mDemoActorHolder = mDemoActorHolders[2];
        if (al::isNerve(this, &NrvDemoOpeningStateCourseSelect)) {
            setDemoPlayerCourseSelectStartPos();
            rc::tryStartDemo(mDemoActorHolder);
            CourseSelectDirector* director =
                CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director && director->getDemoOpeningSwitch())
                director->getDemoOpeningSwitch()->onSwitch(3);
            mDemoActorHolder->startAction(0, false);
            mNextNerve = &NrvDemoOpeningStateCourseSelectSecond;
        } else {
            al::startSequenceBgm(mScene, "DemoWorldStart", -1, 210);
            CourseSelectDirector* director =
                CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director && director->getDemoOpeningSwitch())
                director->getDemoOpeningSwitch()->onSwitch(4);
            mDemoActorHolder->startAction(1, false);
            mNextNerve = &NrvDemoOpeningStateDemoEnd;
        }
        mIsRouteDokan = false;
    } else {
        if (al::isStep(this, 140) && al::isNerve(this, &NrvDemoOpeningStateCourseSelectSecond)) {
            CourseSelectDirector* director =
                CourseSelectDirector::tryGetCourseSelectDirector(mScene);
            if (director) {
                CourseSelectMiniature* castle = director->findKoopaCastle(1);
                if (castle)
                    al::emitEffect(castle, "AppearCloud", nullptr);
            }
        }
        if (mDemoActorHolder->isActionEndCamera(-1))
            al::setNerve(this, mNextNerve);
    }
}

void DemoOpeningState::setDemoPlayerCourseSelectStartPos() {
    CourseSelectDirector* director = CourseSelectDirector::tryGetCourseSelectDirector(mScene);
    if (director && director->getDemoOpeningSwitch()) {
        sead::Vector3f pos = al::getTrans(director->tryFindMiniatureObj(1));
        pos.z += 200.0f;
        s32 count = rc::getDemoPlayerNum(mScene);
        for (s32 i = 0; i < count; ++i)
            rc::getDemoPlayer(mScene, i)->setTrans(pos);
    }
}

void DemoOpeningState::exeDemoEnd() {
    if (al::isFirstStep(this)) {
        s32 userIds[4];
        s32 count = rc::findActiveUserIdList(userIds, GameDataHolderAccessor(mScene));
        for (s32 i = 0; i < count; ++i) {
            al::LiveActor* model = mDemoActorHolder->tryFindPlayerModel(i);
            s32 character =
                rc::getControlUserCharacterType(GameDataHolderAccessor(model), userIds[i]);
            DemoPlayerController* player = rc::getDemoPlayerByCharacter(mScene, character);
            if (player) {
                player->setTrans(al::getTrans(model));
                sead::Vector3f front;
                al::calcFrontDir(&front, model);
                player->setFrontUpVec(front, sead::Vector3f::ey);
                player->startAction("CourseSelectWaitPanic");
            }
        }
        rc::showDemoPlayerAll(mScene);
        rc::requestEndDemoPlayer(mScene);
    } else if (al::isStep(this, 1)) {
        mSkipLayout->kill();
        mDemoActorHolder->setEndCameraInterpolateFrame(30);
        mDemoActorHolder->kill();
        kill();
    }
}

void DemoOpeningState::exeCancelDemo() {
    if (al::isFirstStep(this)) {
        mBlackWipe->startClose(-1);
        mSkipLayout->kill();
        al::stopBgm(mScene, "DemoOpening", mBlackWipe->getWipeFrameNum(), -1);
        mDemoActorHolder->tryCancelAudio(mBlackWipe->getWipeFrameNum(), false);
    }
    if (mBlackWipe->isCloseEnd()) {
        setDemoPlayerCourseSelectStartPos();
        rc::requestEndDemoPlayer(mScene);
        al::PlayerHolder* players = mPlayerHolder;
        if (players) {
            s32 count = al::getPlayerNumMax(players);
            for (s32 i = 0; i < count; ++i)
                al::onAreaTarget(al::getPlayerActor(players, i));
        }
        mDemoActorHolder->killAllEffect();
        mDemoActorHolder->kill();
        mScene->getLiveActorKit()->getGraphicsSystemInfo()->mGraphicsAreaDirector->cancelLerp();
        kill();
        mBlackWipe->startOpen(-1);
    }
}

