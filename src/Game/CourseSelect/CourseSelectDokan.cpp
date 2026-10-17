#include "CourseSelect/CourseSelectDokan.hpp"
#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectDokanHolder.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "CourseSelect/CourseSelectNodeParts.hpp"
#include "CourseSelect/CourseSelectPuppeteer.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/CourseInfoHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"

namespace rc { int calcOpenNodePriority(CourseSelectDokan* pDokan); }
namespace {
NERVE_DECL(CourseSelectDokan, OpenPrevRoad);
NERVE_DECL(CourseSelectDokan, Wait);
NERVE_DECL(CourseSelectDokan, DemoIn);
NERVE_DECL(CourseSelectDokan, HideToShow);
NERVE_DECL(CourseSelectDokan, DemoOut);
NERVE_DECL(CourseSelectDokan, Hide);
NERVES_MAKE_NOSTRUCT(CourseSelectDokan, OpenPrevRoad, Wait, DemoIn, HideToShow, DemoOut, Hide)
}

/** @brief Adapt a pipe to the course-select cursor and demonstration interface. */
class DokanController : public ICourseSelectActorController {
public:
    explicit DokanController(CourseSelectDokan* pActor) : mpActor(pActor) {}
    al::LiveActor* getActor() override { return mpActor; }
    int getCursorLayoutType() const override { return 7; }
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return nullptr; }
    CourseSelectNode* getCourseSelectNode() const override { return mpActor->getNode(); }
    int calcOpenNodePriority() const override { return rc::calcOpenNodePriority(mpActor); }
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override { mpActor->startPuppetDemo(pGroup); }
    void endPuppetDemo() override { mpActor->endPuppetDemo(); }
    void startBind(CourseSelectPuppeteer* pPuppeteer) override { mpActor->startBind(pPuppeteer); }
    bool tryDecide(const CourseSelectDirector* pDirector) override { return mpActor->tryDecide(pDirector); }
    void startOpen() override { mpActor->activate(); }
    void startOpenImmediately() override { mpActor->activateImmediately(); }
    bool isEndOpen() const override { return al::isAlive(mpActor); }
private:
    CourseSelectDokan* mpActor;
};

/** @brief Create a linked course-select pipe. */
CourseSelectDokan::CourseSelectDokan(const char* pName) : al::LiveActor(pName) {}
CourseSelectDokan::~CourseSelectDokan() = default;

/** @brief Load the pipe and register its link, controller, and optional approach road. */
void CourseSelectDokan::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "CourseSelectDokan", nullptr);
    al::tryGetArg(&mCameraInterpolate, rInfo, "CameraInterpolate");
    al::tryGetArg(&mSyncDead, rInfo, "SyncDead");
    mpDirector = CourseSelectDirector::getCourseSelectDirector(this);
    mIsHiddenType = al::isObjectName(rInfo, "CourseSelectDokanHide");
    if (mIsHiddenType) initDokanHide(rInfo);
    else al::initNerve(this, &NrvCourseSelectDokanWait, 0);
    mpController = new DokanController(this);
    mpSensor = new CourseSelectSensor(static_cast<CourseSelectSensorType>(2), mpController);
    al::tryGetArg(&mLinkId, rInfo, "DokanID");
    al::tryGetArg(&mIsSource, rInfo, "SrcFlag");
    if (mIsSource) CourseSelectDokanFuction::registerDokanSrc(this);
    else CourseSelectDokanFuction::registerDokanDst(this);
    bool isConnectRoad = true;
    al::tryGetArg(&isConnectRoad, rInfo, "IsConnectLoad");
    if (mIsHiddenType || !mIsSource || !isConnectRoad) {
        makeActorAppeared();
    } else {
        mpNode = new CourseSelectNode("コース選択分岐点[土管]", nullptr);
        al::initCreateActorWithPlacementInfo(mpNode, rInfo);
        mpNode->setController(mpController);
        mpNode->appear();
        al::PlacementInfo placement;
        al::ActorInitInfo info;
        info.initViewIdHost(&placement, rInfo);
        mpPrevRoad = CourseSelectNodeFunction::createRoadEdge("コースセレクト道[終点(土管)]", info, al::getTrans(this), sead::Vector3f::ez, false);
        al::tryGetArg(&mPrevRoadOffset, rInfo, "PrevRoadOffset");
        mpClosedMiniature = new al::LiveActor("コース未開放ミニ");
        al::initActorWithArchiveName(mpClosedMiniature, rInfo, "MiniatureCloseSmall", nullptr);
        mpClosedMiniature->makeActorDead();
        inactivate();
    }
}

/** @brief Restore the saved visibility of a hidden pipe. */
void CourseSelectDokan::initDokanHide(const al::ActorInitInfo& rInfo) {
    mpActorInfo = new CourseSelectActorInfo(this, rInfo);
    if (CourseInfoFunction::isOpen(GameDataHolderAccessor(this), mpActorInfo->getCourseId()))
        al::initNerve(this, &NrvCourseSelectDokanWait, 0);
    else {
        al::hideModelIfShow(this);
        al::initNerve(this, &NrvCourseSelectDokanHide, 0);
    }
}

/** @brief Show the closed marker and deactivate this pipe and its synchronized destination. */
void CourseSelectDokan::inactivate() {
    if (isNeedSync()) mpDstDokan->inactivate();
    if (mpClosedMiniature) mpClosedMiniature->appear();
    mActive = false;
    kill();
}

/** @brief Align the approach road with the first connected course node. */
void CourseSelectDokan::initAfterPlacement() {
    if (!mpPrevRoad) return;
    auto* pDirector = CourseSelectDirector::getCourseSelectDirector(this);
    auto* pNode = pDirector->getNode(mpNode->getFirstNextNodeId());
    const sead::Vector3f& next = al::getTrans(pNode);
    const sead::Vector3f& current = al::getTrans(this);
    sead::Vector3f direction(next.x - current.x, 0.0f, next.z - current.z);
    al::normalize(&direction);
    al::faceToDirection(mpPrevRoad, -direction);
    sead::Vector3f* pTrans = al::getTransPtr(mpPrevRoad);
    pTrans->setScaleAdd(mPrevRoadOffset, direction, *pTrans);
    al::setTransY(mpPrevRoad, al::getTrans(pNode).y);
}

/** @brief Push players away from the body and offer the pipe entrance to the cursor. */
void CourseSelectDokan::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isSensorPlayer(pReceiver)) {
        if (al::isSensorMapObj(pSender)) {
            if (!isHideDokan()) al::sendMsgPush(pReceiver, pSender);
        } else if (al::isSensorEye(pSender)) {
            if (mIsSource || !isHideDokan()) {
                mpSensor->setSensor(pSender);
                mpDirector->touchPlayer(mpSensor, al::getSensorHost(pReceiver));
            }
        }
    }
}
bool CourseSelectDokan::isHideDokan() const { return al::isNerve(this, &NrvCourseSelectDokanHide); }
bool CourseSelectDokan::receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) { return false; }

/** @brief Activate this pipe, growing its approach road when present. */
void CourseSelectDokan::activate() {
    if (isNeedSync()) mpDstDokan->activate();
    mActive = true;
    if (mpPrevRoad) {
        al::setNerve(this, &NrvCourseSelectDokanOpenPrevRoad);
        al::LiveActor::appear();
        al::hideModelIfShow(this);
    } else {
        al::startHitReactionAppear(this);
        al::startSe(this, "PgAppear", nullptr);
        appear();
    }
}
bool CourseSelectDokan::isNeedSync() const { return mSyncDead && mIsSource; }

/** @brief Activate linked pipes without playing their approach-road animation. */
void CourseSelectDokan::activateImmediately() {
    if (isNeedSync()) mpDstDokan->activateImmediately();
    mActive = true;
    if (mpPrevRoad) mpPrevRoad->startOpenImmediately();
    appear();
}

/** @brief Store the linked destination and synchronize its initial availability. */
void CourseSelectDokan::setDstDokan(CourseSelectDokan* pDokan) {
    mpDstDokan = pDokan;
    if (isNeedSync() && !mIsHiddenType) {
        if (mActive) pDokan->inactivate();
        else pDokan->activateImmediately();
    }
}

/** @brief Appear when active and remove the closed-course marker. */
void CourseSelectDokan::appear() {
    if (mActive) {
        if (mpClosedMiniature) mpClosedMiniature->kill();
        al::LiveActor::appear();
    }
}

/** @brief Defer disappearance until an active warp demonstration completes. */
void CourseSelectDokan::kill() {
    if (isDemo()) mKillAfterDemo = true;
    else al::LiveActor::kill();
}
bool CourseSelectDokan::isDemo() const {
    return al::isNerve(this, &NrvCourseSelectDokanDemoIn) || al::isNerve(this, &NrvCourseSelectDokanDemoOut);
}
void CourseSelectDokan::control() {}

/** @brief Begin the player-entry sequence and keep the pipe visible to the scene. */
void CourseSelectDokan::startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) {
    mpGroup = pGroup;
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCourseSelectDokanDemoIn);
}

/** @brief Finish the warp sequence and honor any deferred disappearance. */
void CourseSelectDokan::endPuppetDemo() {
    mpGroup = nullptr;
    al::validateClipping(this);
    al::setNerve(this, &NrvCourseSelectDokanWait);
    if (mKillAfterDemo) {
        mKillAfterDemo = false;
        kill();
    }
}
void CourseSelectDokan::startBind(CourseSelectPuppeteer* pPuppeteer) { pPuppeteer->startBindWarpActor(this, mpDstDokan); }

/** @brief Reveal a hidden pipe or request a warp when the selected player confirms. */
bool CourseSelectDokan::tryDecide(const CourseSelectDirector* pDirector) {
    if (!pDirector->isEnableEnterSelectedSensor()) return false;
    if (!pDirector->isTriggerDecideMainPlayer()) return false;
    if (isHideDokan()) {
        CourseInfoFunction::setOpen(GameDataHolderWriter(this), tryGetCourseId());
        al::showModelIfHide(this);
        al::startHitReactionAppear(this);
        al::startSe(this, "PgFound", nullptr);
        if (isNeedSync()) mpDstDokan->syncHideEnd(this);
        al::setNerve(this, &NrvCourseSelectDokanHideToShow);
    } else mpDirector->getPuppeteerGroup()->startDokanDemo(mpController);
    return true;
}
int CourseSelectDokan::tryGetCourseId() const { return mpActorInfo ? mpActorInfo->getCourseId() : -1; }

/** @brief Reveal the other end only when it links back to the revealed source. */
void CourseSelectDokan::syncHideEnd(const CourseSelectDokan* pDokan) {
    if (mpDstDokan == pDokan) {
        al::showModelIfHide(this);
        al::startHitReactionAppear(this);
        al::setNerve(this, &NrvCourseSelectDokanWait);
    }
}

/** @brief Grow the approach road, then reveal the pipe at its endpoint. */
void CourseSelectDokan::exeOpenPrevRoad() {
    if (al::isFirstStep(this)) mpPrevRoad->startOpen();
    if (mpPrevRoad->isEndGrow()) {
        if (mpClosedMiniature) mpClosedMiniature->kill();
        al::showModelIfHide(this);
        al::startHitReactionAppear(this);
        al::startSe(this, "PgAppear", nullptr);
        al::setNerve(this, &NrvCourseSelectDokanWait);
    }
}
void CourseSelectDokan::exeWait() {}

/** @brief Reveal an unsynchronized destination without waiting for a source discovery. */
void CourseSelectDokan::exeHide() {
    if (!mSyncDead && !mIsSource) {
        al::showModelIfHide(this);
        al::setNerve(this, &NrvCourseSelectDokanWait);
    }
}
void CourseSelectDokan::exeHideToShow() {
    mpDirector->tryResetSelectedActorController(mpController);
    al::setNerve(this, &NrvCourseSelectDokanWait);
}

/** @brief Wait for entering players, then cover the screen before the warp. */
void CourseSelectDokan::exeDemoIn() {
    if (al::isFirstStep(this)) al::startSe(this, "PgEnterStart", nullptr);
    if (al::isGreaterEqualStep(this, 10)) al::tryPauseBgmIfDifferBgmArea(this, al::getTrans(mpDstDokan), 105);
    if (mpGroup) {
        int count = mpGroup->getPuppeteerCount();
        for (int i = 0; i < count; ++i) {
            auto* pPuppeteer = mpGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd() && !pPuppeteer->isDokanDemoWarpWait()) return;
        }
        if (!mCameraInterpolate) al::requestCaptureScreenCover(this, 4);
        al::setNerve(this, &NrvCourseSelectDokanDemoOut);
    }
}

/** @brief Warp all participating players and release them together at the destination. */
void CourseSelectDokan::exeDemoOut() {
    if (al::isFirstStep(this)) return;
    if (al::isStep(this, 1)) {
        int count = mpGroup->getPuppeteerCount();
        for (int i = 0; i < count; ++i) {
            auto* pPuppeteer = mpGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd()) pPuppeteer->startDokanWarp();
        }
        if (mCameraInterpolate) al::requestResetUserCameraControl(this);
        else al::requestCancelInterpole(this);
    }
    if (mpGroup) {
        int count = mpGroup->getPuppeteerCount();
        for (int i = 0; i < count; ++i) {
            auto* pPuppeteer = mpGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd() && !pPuppeteer->isDokanDemoEndWait()) return;
        }
        for (int i = 0; i < count; ++i) {
            auto* pPuppeteer = mpGroup->getPuppeteer(i);
            if (!pPuppeteer->isDemoEnd()) {
                pPuppeteer->startDokanEnd();
                al::setNerve(this, &NrvCourseSelectDokanWait);
            }
        }
    }
}
