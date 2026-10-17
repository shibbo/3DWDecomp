#include "Enemy/CrawlerGenerator.hpp"
#include "Enemy/Crawler.hpp"
#include "Enemy/CrawlerLaunchState.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(CrawlerGenerator, Stop)
NERVE_DECL(CrawlerGenerator, LaunchInterval)
NERVE_DECL(CrawlerGenerator, Launch)
NERVE_DECL(CrawlerGenerator, LaunchSign)
NERVES_MAKE_NOSTRUCT(CrawlerGenerator, Stop, LaunchInterval, Launch, LaunchSign)
struct GeneratorParam {
    sead::Vector3f offset = sead::Vector3f(0.0f, 40.0f, 0.0f);
    CrawlerLaunchStateParam normal;
    CrawlerLaunchStateParam big = CrawlerLaunchStateParam(3.0f, 27.0f, 18.0f, 2.1f, 20, 60, 17);
};
GeneratorParam sGeneratorParam;
}

/** @brief Creates an inactive launcher. @param pName Actor name. */
CrawlerGenerator::CrawlerGenerator(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the crawler pool and launch state. @param rInfo Placement information. */
void CrawlerGenerator::init(const al::ActorInitInfo& rInfo) {
    const char* pObjectName = nullptr;
    al::tryGetObjectName(&pObjectName, rInfo);
    if (al::isEqualSubString(pObjectName, "Launcher")) {
        mIsLauncher = true;
        al::initActorWithArchiveName(this, rInfo, "CrawlerLauncher", nullptr);
    } else if (al::isEqualSubString(pObjectName, "Generator")) {
        al::initActorSceneInfo(this, rInfo);
        al::initActorPoseTQSV(this);
        al::initActorSRT(this, rInfo);
        al::initExecutorWatchObj(this, rInfo);
        al::initStageSwitch(this, rInfo);
        al::initActorClipping(this, rInfo);
        al::initGroupClipping(this, rInfo, 64);
    } else {
        makeActorDead();
        return;
    }
    f32 walkSpeed = 1.0f;
    s32 count = 3;
    s32 appearFrame = 1800;
    bool isCoinAttach = false;
    bool isEnableTrample = true;
    al::tryGetArg(&count, rInfo, "CrawlerMax");
    al::tryGetArg(&mWaitFrame, rInfo, "WaitFrame");
    al::tryGetArg(&walkSpeed, rInfo, "WalkSpeed");
    al::tryGetArg(&appearFrame, rInfo, "AppearFrame");
    al::tryGetArg(&isCoinAttach, rInfo, "IsCoinAttach");
    al::tryGetArg(&isEnableTrample, rInfo, "IsEnableTrampleLaunched");
    if (mWaitFrame * (count - 1) < appearFrame) {
        makeActorDead();
        return;
    }
    mConnector = al::createMtxConnector(this);
    mGroup = new al::DeriveActorGroup<Crawler>("クローラーリスト", count);
    for (s32 i = 0; i < count; ++i) {
        auto* pCrawler = new Crawler("クローラー");
        if (isCoinAttach)
            pCrawler->setIsCoinItem(true);
        al::initCreateActorWithPlacementInfo(pCrawler, rInfo);
        pCrawler->setWalkSpeed(walkSpeed);
        pCrawler->setKillStep(static_cast<f32>(appearFrame));
        pCrawler->setEnableHitInLaunch(isEnableTrample);
        mGroup->registerActor(pCrawler);
    }
    bool isBig = al::isEqualSubString(pObjectName, "Big");
    al::initNerve(this, &NrvCrawlerGeneratorLaunch, 1);
    if (isBig)
        mLaunch = new CrawlerLaunchState(this, &sGeneratorParam.big, mGroup);
    else
        mLaunch = new CrawlerLaunchState(this, &sGeneratorParam.normal, mGroup);
    al::initNerveState(this, mLaunch, &NrvCrawlerGeneratorLaunch, "[state]クローラー発射");
    al::listenStageSwitchOnKill(this, al::Functor(this, &CrawlerGenerator::stopBySwitch));
    makeActorAppeared();
}

/** @brief Stops future launches. */
void CrawlerGenerator::stopBySwitch() { al::setNerve(this, &NrvCrawlerGeneratorStop); }

/** @brief Attaches the launcher to nearby collision. */
void CrawlerGenerator::initAfterPlacement() {
    if (al::isAlive(this))
        al::attachMtxConnectorToCollision(mConnector, this, false);
}

/** @brief Follows the attached collision. */
void CrawlerGenerator::control() { al::connectPoseQT(this, mConnector); }

/** @brief Waits indefinitely after the kill switch. */
void CrawlerGenerator::exeStop() {}

/** @brief Waits between launches. */
void CrawlerGenerator::exeLaunchInterval() {
    if (al::isGreaterEqualStep(this, mWaitFrame)) {
        if (mIsLauncher)
            al::setNerve(this, &NrvCrawlerGeneratorLaunchSign);
        else
            al::setNerve(this, &NrvCrawlerGeneratorLaunch);
    }
}

/** @brief Plays the visible launcher's warning animation. */
void CrawlerGenerator::exeLaunchSign() {
    if (al::isFirstStep(this))
        al::startAction(this, "AppearSignStart");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvCrawlerGeneratorLaunch);
}

/** @brief Runs the launch state and starts the next interval. */
void CrawlerGenerator::exeLaunch() {
    if (mIsLauncher && al::isFirstStep(this))
        al::startAction(this, "Wait");
    al::updateNerveStateAndNextNerve(this, &NrvCrawlerGeneratorLaunchInterval);
}
