#include "Demo/StageStartEventCamera.hpp"
#include "Demo/DemoSceneCamera.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Thread/Functor.hpp"
#include "System/GameDataFunction.hpp"

namespace rc {
void requestStartDemoPlayer(const al::LiveActor* pActor);
void requestEndDemoPlayer(const al::LiveActor* pActor);
void setAlreadyShowBossDemo(const al::LiveActor* pActor);
bool tryCancelStageDemo(const al::LiveActor* pActor);
}

namespace {
NERVE_DECL(StageStartEventCamera, Play);
NERVE_DECL(StageStartEventCamera, End);
NERVES_MAKE_NOSTRUCT(StageStartEventCamera, Play, End)
}

/** @brief Creates the camera event. @param pName Actor name. */
StageStartEventCamera::StageStartEventCamera(const char* pName) : StageStartEventBase(pName) {}

/** @brief Initializes the scene camera and appearance switch. @param rInfo Actor initialization data. */
void StageStartEventCamera::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initNerve(this, &NrvStageStartEventCameraPlay, 0);
    al::initExecutorUpdate(this, rInfo, "デモオブジェクト");
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, nullptr, nullptr);
    makeActorDead();
    al::PlacementInfo placement;
    al::ActorInitInfo cameraInfo;
    cameraInfo.initViewIdHost(&placement, rInfo);
    mCamera = new DemoSceneCamera(GameDataFunction::getGameDataHolder(this));
    mCamera->initDemoSceneActor(rInfo, cameraInfo, nullptr);
    mCamera->setInterpolateFrame(30);
    al::listenStageSwitchOnAppear(this, al::Functor(this, &StageStartEventCamera::startDemo));
}

/** @brief Starts playback and signals the demo stage switch. */
void StageStartEventCamera::startDemo() {
    appear();
    al::tryOnStageSwitch(this, "SwitchDemoPlayOn");
    al::setNerve(this, &NrvStageStartEventCameraPlay);
}

/** @brief Ends an established camera demo and releases players. */
void StageStartEventCamera::endDemo() {
    if (al::isNerve(this, &NrvStageStartEventCameraPlay) && al::isGreaterStep(this, 2)) {
        al::tryOffStageSwitch(this, "SwitchDemoPlayOn");
        mCamera->endDemo();
        rc::requestEndDemoPlayer(this);
        rc::setAlreadyShowBossDemo(this);
        al::setNerve(this, &NrvStageStartEventCameraEnd);
    }
}

/** @brief Checks completion. @return Whether the event is in its end state. */
bool StageStartEventCamera::isEndDemo() const {
    return al::isNerve(this, &NrvStageStartEventCameraEnd);
}

/** @brief Runs the opening camera and permits cancellation after 60 frames. */
void StageStartEventCamera::exePlay() {
    if (al::isFirstStep(this)) {
        rc::requestStartDemoPlayer(this);
        mCamera->startCamera(0);
        al::startBgm(this, "Stage", -1, 0, -1, -1);
    }
    if (al::isGreaterEqualStep(this, 60) && rc::tryCancelStageDemo(this)) {
        al::requestCaptureScreenCover(this, 4);
        mCamera->setInterpolateFrame(0);
        endDemo();
    } else if (mCamera->isEndAnimCamera(0)) {
        endDemo();
    }
}

/** @brief Removes the completed event actor. */
void StageStartEventCamera::exeEnd() {
    kill();
}
