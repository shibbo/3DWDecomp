#include "Enemy/BombBoundGenerator.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Factory/Factory.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"

namespace {
NERVE_DECL(BombBoundGenerator, WaitStarting)
NERVE_DECL(BombBoundGenerator, Idle)
NERVE_DECL(BombBoundGenerator, Interval)
NERVE_DECL(BombBoundGenerator, Reload)
NERVE_DECL(BombBoundGenerator, Launch)
NERVES_MAKE_NOSTRUCT(BombBoundGenerator, WaitStarting, Idle, Interval, Reload, Launch)

/** @brief Checks whether a player obstructs the launcher. @param pActor Launcher. @return Whether a player is inside its sensor. */
bool isPlayerBlocking(const al::LiveActor* pActor) {
    al::HitSensor* pSensor = al::getHitSensor(pActor, "NoLaunch");
    s32 count = AttackSensorFunction::getAttackSensorNum(pSensor);
    for (s32 i = 0; i < count; ++i) {
        if (al::isSensorPlayer(AttackSensorFunction::getAttackSensor(pSensor, i))) { return true; }
    }
    return false;
}
}

/** @brief Creates the launcher. @param pName Actor name. */
BombBoundGenerator::BombBoundGenerator(const char* pName) : al::LiveActor(pName) {}

/** @brief Initializes the launcher and its reusable bomb pool. @param rInfo Placement and scene information. */
void BombBoundGenerator::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "BombBoundGenerator", nullptr);
    al::initNerve(this, &NrvBombBoundGeneratorWaitStarting, 0);
    al::calcJointPos(&mClippingCenter, this, "Body");
    al::setClippingInfo(this, 200.0f, &mClippingCenter);
    const char* pObjectName = nullptr;
    if (al::tryGetObjectName(&pObjectName, rInfo) && al::isEqualString(pObjectName, "BombBoundQuickGenerator")) {
        mType = BombBound::Type_Bound;
    }
    al::tryGetArg(&mShootSpeed, rInfo, "ShootSpeed");
    al::tryGetArg(&mShootInterval, rInfo, "ShootInterval");
    al::tryGetArg(&mIsImmediateStarting, rInfo, "IsImmediateStarting");
    al::tryGetArg(&mInitialDelay, rInfo, "InitialDelay");
    auto* pBombs = new al::DeriveActorGroup<BombBound>("\u306f\u306d\u308b\u7206\u5f3e\u4fdd\u6301", 10);
    mBombs = pBombs;
    for (s32 i = 0; i < pBombs->getMaxActorCount(); ++i) {
        BombBound* pBomb = new BombBound("\u306f\u306d\u308b\u7206\u5f3e");
        al::initCreateActorNoPlacementInfo(pBomb, rInfo);
        pBombs->registerActor(pBomb);
    }
    mBombs->makeActorDeadAll();
    bool hasArmedState = true;
    al::tryGetArg(&hasArmedState, rInfo, "HasArmedState");
    if (!hasArmedState) {
        for (s32 i = 0; i < 10; ++i) { mBombs->getDeriveActor(i)->disableFuse(); }
    }
    if (al::listenStageSwitchOnStart(this, al::Functor(this, &BombBoundGenerator::start))) {
        al::setNerve(this, &NrvBombBoundGeneratorIdle);
    }
    if (al::listenStageSwitchOnOffAppear(this, al::Functor(this, &BombBoundGenerator::switchAppear),
                                       al::Functor(this, &BombBoundGenerator::switchKill))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
}

/** @brief Starts firing, respecting nearby players. */
void BombBoundGenerator::start() {
    mTimer = 0;
    if (mIsImmediateStarting) {
        al::setNerve(this, isPlayerBlocking(this) ? static_cast<const al::Nerve*>(&NrvBombBoundGeneratorInterval) : &NrvBombBoundGeneratorReload);
    } else {
        al::setNerve(this, &NrvBombBoundGeneratorWaitStarting);
    }
}

/** @brief Appears when the stage switch is activated. */
void BombBoundGenerator::switchAppear() {
    al::LiveActor::makeActorAppeared();
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) { mTimer = 0; }
    al::setNerve(this, &NrvBombBoundGeneratorWaitStarting);
}

/** @brief Removes the launcher and, in single-player mode, its bombs. */
void BombBoundGenerator::switchKill() {
    if (GameDataFunction::isSingleMode(GameDataHolderAccessor(this))) { mBombs->makeActorDeadAll(); }
    al::LiveActor::kill();
}

/** @brief Advances the firing cycle timer. */
void BombBoundGenerator::control() { ++mTimer; }
/** @brief Waits for the start switch. */
void BombBoundGenerator::exeIdle() {}
/** @brief Waits out the initial launch delay. */
void BombBoundGenerator::exeWaitStarting() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); }
    if (al::isGreaterEqualStep(this, mInitialDelay + 45)) {
        mTimer -= mInitialDelay;
        al::setNerve(this, &NrvBombBoundGeneratorReload);
    }
}

/** @brief Loads a dead bomb into the launcher joint. */
void BombBoundGenerator::exeReload() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reload");
        mCurrentBomb = mBombs->tryFindDeadDeriveActor();
        if (!mCurrentBomb) { return; }
        sead::Matrix34f* pMtx = al::getJointMtxPtr(this, "BombPosition");
        pMtx->toQuat(*al::getQuatPtr(mCurrentBomb));
        pMtx->getTranslation(*al::getTransPtr(mCurrentBomb));
        mCurrentBomb->reload(mType == BombBound::Type_Bound);
    }
    if (al::isActionEnd(this)) { al::setNerve(this, &NrvBombBoundGeneratorLaunch); }
}

/** @brief Launches the loaded bomb along its front direction. */
void BombBoundGenerator::exeLaunch() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Launch");
        sead::Vector3f front;
        al::calcQuatFront(&front, al::getQuat(mCurrentBomb));
        mCurrentBomb->launch(mType, mShootSpeed * front, mType == BombBound::Type_Bound);
        al::startAction(this, "Launch");
    }
    if (al::isActionEnd(this)) { al::setNerve(this, &NrvBombBoundGeneratorInterval); }
}

/** @brief Waits for the next launch cycle or a nearby player. */
void BombBoundGenerator::exeInterval() {
    if (al::isFirstStep(this)) { al::startAction(this, "Wait"); }
    if (isPlayerBlocking(this)) { return; }
    if (mTimer % (mShootInterval + 90) == 0) { al::setNerve(this, &NrvBombBoundGeneratorReload); }
}
