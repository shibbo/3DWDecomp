#include "CourseSelect/DemoEventGateKeeperChecker.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectPuppeteer.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectRouteDokan.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/PlayerPuppetUtil.hpp"

namespace {
NERVE_DECL(DemoEventGateKeeperChecker, Wait);
NERVE_DECL(DemoEventGateKeeperChecker, BindWait);
NERVE_DECL(DemoEventGateKeeperChecker, Camera);
NERVE_DECL(DemoEventGateKeeperChecker, DemoStart);
NERVE_DECL(DemoEventGateKeeperChecker, CameraStopper);
NERVE_DECL(DemoEventGateKeeperChecker, AppearStopper);
NERVE_DECL(DemoEventGateKeeperChecker, DemoEnd);
NERVES_MAKE_NOSTRUCT(DemoEventGateKeeperChecker, Wait, BindWait, Camera, DemoStart, CameraStopper,
                     AppearStopper, DemoEnd)
} // namespace

class DemoEventGateKeeperController : public ICourseSelectActorController {
  public:
    explicit DemoEventGateKeeperController(DemoEventGateKeeperChecker* actor) : mActor(actor) {}

    al::LiveActor* getActor() override { return mActor; }

    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }

    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }

    void startPuppetDemo(CourseSelectPuppeteerGroup* group) override {
        mActor->startPuppetDemo(group);
    }

    void endPuppetDemo() override { mActor->endPuppetDemo(); }

    void startBind(CourseSelectPuppeteer* puppeteer) override { mActor->startBind(puppeteer); }

    bool tryDecide(const CourseSelectDirector*) override { return false; }

    void startOpen() override {}

    void startOpenImmediately() override {}

    bool isEndOpen() const override { return true; }

  private:
    DemoEventGateKeeperChecker* mActor;
};

DemoEventGateKeeperChecker::DemoEventGateKeeperChecker(const char* name) : al::LiveActor(name) {}

DemoEventGateKeeperChecker::~DemoEventGateKeeperChecker() = default;

static const char* getGateCameraName(int type) {
    return type == 0 ? "DemoEventGateKeeperW4" : type == 1 ? "DemoEventGateKeeperW7" : nullptr;
}

static const char* getStopperCameraName(int type) {
    return type == 0   ? "DemoEventGateKeeperAfterW3"
           : type == 1 ? "DemoEventGateKeeperAfterW6"
                       : nullptr;
}

void DemoEventGateKeeperChecker::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initExecutorWatchObj(this, info);
    al::initStageSwitch(this, info);
    al::tryGetArg(&mType, info, "GateKeeperType");
    mDirector = CourseSelectDirector::getCourseSelectDirector(this);
    mController = new DemoEventGateKeeperController(this);
    al::PlacementInfo miniatureInfo;
    al::getLinksInfo(&miniatureInfo, al::getPlacementInfo(info), "NoDelete_Miniature");
    int world = 0, stage = 0;
    al::tryGetArg(&world, miniatureInfo, "WorldID");
    al::tryGetArg(&stage, miniatureInfo, "StageID");
    mCourseId = GameDataFunction::calcCourseId(GameDataHolderAccessor(this), world, stage);
    al::listenStageSwitchOnStart(this, al::Functor(this, &DemoEventGateKeeperChecker::start));
    al::Resource* resource = al::findOrCreateResource("ObjectData/DemoCamera", nullptr);
    mCamera = al::initAnimCamera(this, info, resource, getGateCameraName(mType), false);
    mStopperCamera = al::initAnimCamera(this, info, resource, getStopperCameraName(mType), false);
    al::initNerve(this, &NrvDemoEventGateKeeperCheckerWait, 0);
    makeActorAppeared();
}

void DemoEventGateKeeperChecker::start() {
    if (!al::isNerve(this, &NrvDemoEventGateKeeperCheckerWait))
        return;
    if (mType == 0) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperFirst(GameDataHolderAccessor(this)))
            return;
    } else if (mType == 1) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperSecond(GameDataHolderAccessor(this)))
            return;
    }
    if (mDirector->isMapPauseWorldJump())
        return;
    mDirector->setEventGateKeeper(true);
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerBindWait);
}

void DemoEventGateKeeperChecker::initAfterPlacement() {
    mMiniature = mDirector->findMiniatureObj(mCourseId);
    if (mType == 0) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperFirst(GameDataHolderAccessor(this))) {
            if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), mCourseId)) {
                al::tryOnStageSwitch(this, "SwitchDeadOn");
                if (GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor(this)) &&
                    GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(this)) ==
                        mCourseId)
                    mMiniature->getRouteDokan()->appearStopper(false);
            } else {
                al::tryOnStageSwitch(this, "SwitchEventOn");
                mMiniature->getRouteDokan()->appearStopper(false);
            }
            makeActorDead();
        }
    } else if (mType == 1) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperSecond(
                GameDataHolderAccessor(this))) {
            if (CourseInfoFunction::isClear(GameDataHolderAccessor(this), mCourseId)) {
                al::tryOnStageSwitch(this, "SwitchDeadOn");
                if (GameDataFunction::isLastPlayCourseFirstClear(GameDataHolderAccessor(this)) &&
                    GameDataFunction::getLastPlayCourseId(GameDataHolderAccessor(this)) ==
                        mCourseId)
                    mMiniature->getRouteDokan()->appearStopper(false);
            } else {
                al::tryOnStageSwitch(this, "SwitchEventOn");
                mMiniature->getRouteDokan()->appearStopper(false);
            }
            makeActorDead();
        }
    }
}

void DemoEventGateKeeperChecker::startPuppetDemo(CourseSelectPuppeteerGroup*) {}

void DemoEventGateKeeperChecker::endPuppetDemo() {
    if (al::isNerve(this, &NrvDemoEventGateKeeperCheckerCamera))
        al::endCamera(this, mCamera, -1);
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerWait);
}

void DemoEventGateKeeperChecker::startBind(CourseSelectPuppeteer*) {}

void DemoEventGateKeeperChecker::exeWait() {
    if (mType == 0) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperFirst(GameDataHolderAccessor(this)))
            return;
    } else if (mType == 1) {
        if (GameDataFlagFunction::isAlreadyShowEventGateKeeperSecond(GameDataHolderAccessor(this)))
            return;
    }
    al::hideModelIfShow(mMiniature);
    al::startAction(mMiniature, "DemoEventGateKeeper");
    al::setActionFrameRate(mMiniature, 0.0f);
}

void DemoEventGateKeeperChecker::exeBindWait() {
    if (al::isFirstStep(this))
        al::invalidateHitSensors(mMiniature);
    int count = mDirector->getPuppeteerGroup()->getPuppeteerNum();
    for (int i = 0; i < count; i++)
        if (!mDirector->getPuppeteerGroup()->getPuppeteer(i)->isDemoEnd())
            return;
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerDemoStart);
}

void DemoEventGateKeeperChecker::exeDemoStart() {
    if (al::isLessStep(this, 60))
        return;
    al::requestCaptureScreenCover(this, 4);
    al::tryOnStageSwitch(this, "SwitchEventOn");
    mDirector->getPuppeteerGroup()->startEventGateKeeperDemo(mController);
    al::startAnimCamera(mMiniature, mCamera, getGateCameraName(mType), 0);
    al::setCameraFovyDegree(mCamera, 25.0f);
    al::invalidateClipping(mMiniature);
    al::startAction(mMiniature, "DemoEventGateKeeper");
    al::setActionFrameRate(mMiniature, 1.0f);
    al::showModelIfHide(mMiniature);
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerCamera);
}

void DemoEventGateKeeperChecker::exeCamera() {
    if (al::isFirstStep(this)) {
        int count = mDirector->getPuppeteerGroup()->getPuppeteerNum();
        for (int i = 0; i < count; i++) {
            auto* puppet = mDirector->getPuppeteerGroup()->getPuppeteer(i);
            if (puppet->isBind()) {
                rc::hidePuppet(puppet->getPlayerPuppet());
                rc::hidePuppetSilhouette(puppet->getPlayerPuppet());
            }
        }
    }
    if (al::isEndAnimCamera(mCamera)) {
        al::startAction(mMiniature, "ShowWait");
        al::requestCaptureScreenCover(this, 2);
        al::endCamera(this, mCamera, -1);
        al::startAnimCamera(mMiniature, mStopperCamera, getStopperCameraName(mType), 0);
        al::setCameraFovyDegree(mStopperCamera, 25.0f);
        al::setNerve(this, &NrvDemoEventGateKeeperCheckerCameraStopper);
    }
}

void DemoEventGateKeeperChecker::exeCameraStopper() {
    if (al::isFirstStep(this)) {
        int count = mDirector->getPuppeteerGroup()->getPuppeteerNum();
        for (int i = 0; i < count; i++) {
            auto* puppet = mDirector->getPuppeteerGroup()->getPuppeteer(i);
            if (puppet->isBind()) {
                rc::startPuppetAction(puppet->getPlayerPuppet(), "CourseSelectSurprise");
                rc::showPuppet(puppet->getPlayerPuppet());
                rc::showPuppetSilhouette(puppet->getPlayerPuppet());
            }
        }
    }
    if (al::isEndAnimCamera(mStopperCamera))
        al::setNerve(this, &NrvDemoEventGateKeeperCheckerAppearStopper);
}

void DemoEventGateKeeperChecker::exeAppearStopper() {
    if (al::isFirstStep(this))
        mMiniature->getRouteDokan()->appearStopper(true);
    if (al::isLessStep(this, 120))
        return;
    al::endCamera(this, mStopperCamera, -1);
    al::validateClipping(mMiniature);
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerDemoEnd);
}

void DemoEventGateKeeperChecker::exeDemoEnd() {
    int count = mDirector->getPuppeteerGroup()->getPuppeteerNum();
    for (int i = 0; i < count; i++) {
        auto* puppet = mDirector->getPuppeteerGroup()->getPuppeteer(i);
        if (!puppet->isDemoEnd())
            puppet->endDemo();
    }
    if (mType == 0)
        GameDataFlagFunction::setShowEventGateKeeperFirst(GameDataHolderWriter(this));
    else if (mType == 1)
        GameDataFlagFunction::setShowEventGateKeeperSecond(GameDataHolderWriter(this));
    al::validateHitSensors(mMiniature);
    mDirector->setEventGateKeeper(false);
    al::setNerve(this, &NrvDemoEventGateKeeperCheckerWait);
}
