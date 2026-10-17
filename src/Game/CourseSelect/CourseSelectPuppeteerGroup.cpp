#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectPuppeteer.hpp"
#include "CourseSelect/CourseSelectScene.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/SaveDataAccessFunction.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(CourseSelectPuppeteerGroup, Wait);
NERVE_DECL(CourseSelectPuppeteerGroup, Save);
NERVES_MAKE_NOSTRUCT(CourseSelectPuppeteerGroup, Wait, Save)
} // namespace

CourseSelectPuppeteerGroup::CourseSelectPuppeteerGroup(CourseSelectScene* pScene)
    : al::LiveActor("コース選択パペッターグループ"), mScene(pScene) {}

CourseSelectPuppeteerGroup::~CourseSelectPuppeteerGroup() = default;

void CourseSelectPuppeteerGroup::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initExecutorWatchObj(this, info);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, info);
    initHitSensor(1);
    mPuppeteerNum = rc::getControlUserNumMax();
    mPuppeteers = new CourseSelectPuppeteer*[mPuppeteerNum];
    for (int i = 0; i < mPuppeteerNum; i++)
        mPuppeteers[i] = new CourseSelectPuppeteer(this, info, i);
    al::addHitSensorBindable(this, info, "Bind", 0.0f, 0, sead::Vector3f(0, 0, 0));
    al::initNerve(this, &NrvCourseSelectPuppeteerGroupWait, 0);
    makeActorAppeared();
}

bool CourseSelectPuppeteerGroup::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                            al::HitSensor* receiver) {
    if (al::isMsgBindStart(msg) || al::isMsgBindInit(msg) || al::isMsgBindCancel(msg)) {
        int port = rc::getPlayerInputPort(sender);
        int user = rc::calcControlUserIdFromPortNum(GameDataHolderAccessor(mScene), port);
        return mPuppeteers[user]->receiveMsg(msg, sender, receiver);
    }
    return false;
}

void CourseSelectPuppeteerGroup::control() {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->update();
}

void CourseSelectPuppeteerGroup::startEnterDemo(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startEnterDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startExitDemo(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startExitDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startEventGateKeeperDemo(
    ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startEventGateKeeperDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startOpenRoadDemo(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startOpenRoadDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startUnLockDemo(ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startUnLockDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startLockAppearDemo(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startLockAppearDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startOpenGateKeeperDemo(ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startOpenGateKeeperDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startDokanDemo(ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startDokanDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startWorldWarpDokanDemo(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startWorldWarpDokanDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startRocketDemo(ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startRocketDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startRocketBreakDemo(ICourseSelectActorController* controller) {
    mScene->getCourseSelectDirector()->invalidateButtonDemo();
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startRocketBreakDemo(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startRouteDokan(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
        al::requestResetUserCameraControl(controller->getActor());
        al::requestResetZoomCameraControl(controller->getActor());
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startRouteDokan(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startWarpToCourse(ICourseSelectActorController* controller) {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    mController = controller;
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startWarpToCourse(mController);
    mController->startPuppetDemo(this);
}

void CourseSelectPuppeteerGroup::startWorldStartDemo() {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startWorldStartDemo();
}

void CourseSelectPuppeteerGroup::startWorldStartDemoW8() {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startWorldStartDemoW8();
}

void CourseSelectPuppeteerGroup::startHidePlayerDemo() {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->setActor(
            static_cast<PlayerActor*>(mScene->getCourseSelectDirector()->tryFindPlayerByUserId(i)));
    if (mController) {
        mController->endPuppetDemo();
    }
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->startHidePlayerDemo();
}

void CourseSelectPuppeteerGroup::endDemo() {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        mPuppeteers[i]->endDemo();
    if (mController) {
        mController->endPuppetDemo();
        mController = nullptr;
    }
}

bool CourseSelectPuppeteerGroup::isPlayDemo(int userId) const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (!mPuppeteers[i]->isDemoEnd() && mPuppeteers[i]->getUserId() == userId)
            return true;
    return false;
}

bool CourseSelectPuppeteerGroup::isPlayDemo() const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (!mPuppeteers[i]->isDemoEnd())
            return true;
    return false;
}

bool CourseSelectPuppeteerGroup::isPlayDemoAll() const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (mPuppeteers[i]->isDemoEnd() &&
            rc::isActiveControlUser(GameDataHolderAccessor(mScene->getGameDataHolderDirect()), i))
            return false;
    return true;
}

bool CourseSelectPuppeteerGroup::isPlayDemoAny() const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (!mPuppeteers[i]->isDemoEnd() &&
            rc::isActiveControlUser(GameDataHolderAccessor(mScene->getGameDataHolderDirect()), i))
            return true;
    return false;
}

void CourseSelectPuppeteerGroup::startWaitDemo(int userId) {
    auto* mainPlayer = mScene->getCourseSelectDirector()->getMainPlayer();
    mPuppeteers[userId]->setActor(
        static_cast<PlayerActor*>(rc::findPlayerActorFirstByUserId(mainPlayer, userId)));
    mPuppeteers[userId]->startWaitDemo();
}

void CourseSelectPuppeteerGroup::reviveUser(int userId, ICourseSelectActorController* controller,
                                            bool appearDemo) {
    auto* mainPlayer = mScene->getCourseSelectDirector()->getMainPlayer();
    mPuppeteers[userId]->setActor(
        static_cast<PlayerActor*>(rc::findPlayerActorFirstByUserId(mainPlayer, userId)));
    mPuppeteers[userId]->reviveUser(controller, appearDemo);
}

void CourseSelectPuppeteerGroup::enterUser(int userId) {
    auto* actor = rc::findPlayerActorFirstByUserId(
        mScene->getCourseSelectDirector()->getMainPlayer(), userId);
    mPuppeteers[userId]->setActor(static_cast<PlayerActor*>(actor));
    mPuppeteers[userId]->enterUser();
}

void CourseSelectPuppeteerGroup::leaveUser(int userId) { mPuppeteers[userId]->leaveUser(); }

bool CourseSelectPuppeteerGroup::isPlayEntryDemo() const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (mPuppeteers[i]->isPlayEntryDemo())
            return true;
    return false;
}

bool CourseSelectPuppeteerGroup::isPlayEntryDemo(int userId) const {
    return mPuppeteers[userId]->isPlayEntryDemo();
}

bool CourseSelectPuppeteerGroup::isPlayLeaveDemo(int userId) const {
    return mPuppeteers[userId]->isPlayLeaveDemo();
}

void CourseSelectPuppeteerGroup::startSaveDataWriteInDemo(ICourseSelectActorController*) {
    al::setNerve(this, &NrvCourseSelectPuppeteerGroupSave);
}

bool CourseSelectPuppeteerGroup::isEndSaveDataWriteInDemo(ICourseSelectActorController*) {
    return al::isNerve(this, &NrvCourseSelectPuppeteerGroupWait);
}

bool CourseSelectPuppeteerGroup::isDemoEnd() const {
    for (int i = 0; i < rc::getControlUserNumMax(); i++)
        if (!mPuppeteers[i]->isDemoEnd())
            return false;
    return true;
}

void CourseSelectPuppeteerGroup::exeWait() {}

void CourseSelectPuppeteerGroup::exeSave() {
    if (al::isFirstStep(this))
        SaveDataAccessFunction::startSaveDataWriteNoMessage(mScene->getGameDataHolderDirect(),
                                                            false);
    if (SaveDataAccessFunction::updateSaveDataAccess(mScene->getGameDataHolderDirect(), false))
        al::setNerve(this, &NrvCourseSelectPuppeteerGroupWait);
}
