#include "Enemy/KillerGenerator.hpp"
#include "Enemy/Killer.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Execute/ExecuteUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
NERVE_DECL(KillerGenerator, ShootKeepAppear)
NERVE_DECL(KillerGenerator, Delay)
NERVE_DECL(KillerGenerator, Deactive)
class KillerGeneratorNrvShootDelay : public al::Nerve {
public:
    /** @brief Runs the switch-triggered firing delay.
     * @param pKeeper Generator nerve keeper.
     */
    void execute(al::NerveKeeper* pKeeper) const override {
        pKeeper->getParent<KillerGenerator>()->exeDelay();
    }
};
NERVE_DECL(KillerGenerator, Wait)
NERVE_DECL(KillerGenerator, StandBy)
NERVE_DECL(KillerGenerator, StandByAppear)
NERVE_DECL(KillerGenerator, ShootKeepShoot)
NERVES_MAKE_NOSTRUCT(KillerGenerator, ShootKeepAppear, Delay, Deactive, ShootDelay,
                    Wait, StandBy, StandByAppear, ShootKeepShoot)

/** @brief Selects the projectile actor name for a generator type.
 * @param type Normal, search, Magnum, or search Magnum projectile type.
 * @return Localized actor name for the selected projectile.
 */
const char* getKillerName(int type) {
    switch (type) {
    case 1:
        return "サーチキラー";
    case 2:
        return "マグナムキラー";
    case 3:
        return "サーチマグナムキラー";
    default:
        return "キラー";
    }
}
}

/** @brief Constructs a generator with default firing and projectile settings.
 * @param pName Actor name.
 * @param pHost Launcher whose actions accompany shooting.
 * @param type Normal, search, Magnum, or search Magnum projectile type.
 * @param pFilter Collision filter shared by the generated projectiles.
 */
KillerGenerator::KillerGenerator(const char* pName, al::LiveActor* pHost, int type,
                               const al::CollisionPartsFilterBase* pFilter)
    : al::LiveActor(pName), mHost(pHost), mFilter(pFilter), mType(type) {}

/** @brief Creates the projectile pool and registers firing and death switches.
 * @param rInfo Actor placement and scene information.
 */
void KillerGenerator::init(const al::ActorInitInfo& rInfo) {
    bool isSingleMode = al::isSingleMode(rInfo);
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 64);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initNerve(this, &NrvKillerGeneratorShootKeepAppear, 0);
    bool isPlacedKinopioBrigade = false;
    bool usingDepthShadow = false;
    al::tryGetArg(&mStepWait, rInfo, "StepWait");
    al::tryGetArg(&mStepDelay, rInfo, "StepDelay");
    al::tryGetArg(&mStepDisappear, rInfo, "StepDisappear");
    al::tryGetArg(&mAccelFly, rInfo, "AccelFly");
    al::tryGetArg(&mIsShootImmediatelySwitchOn, rInfo, "IsShootImmediatelySwitchOn");
    al::tryGetArg(&isPlacedKinopioBrigade, rInfo, "IsPlacedKinopioBrigade");
    al::tryGetArg(&usingDepthShadow, rInfo, "UsingDepthShadow");
    if (mStepDelay > 0) {
        al::setNerve(this, &NrvKillerGeneratorDelay);
    }
    if (mStepWait < 100) {
        makeActorDead();
        return;
    }
    int count = mStepDisappear / mStepWait + 2;
    mKillers = new al::DeriveActorGroup<Killer>("キラーグループ", count);
    for (int i = 0; i < count; i++) {
        Killer* pKiller = new Killer(getKillerName(mType), mType, this, mFilter);
        if (isSingleMode) {
            pKiller->init(rInfo);
        } else {
            al::initCreateActorNoPlacementInfo(pKiller, rInfo);
        }
        if (isPlacedKinopioBrigade) {
            if (Killer::isMagnum(mType)) {
                al::setSensorRadius(pKiller, "Attack1", 240.0f);
                al::setSensorRadius(pKiller, "Attack2", 240.0f);
            } else {
                al::setSensorRadius(pKiller, "Attack", 60.0f);
            }
        }
        if (usingDepthShadow) {
            al::invalidateShadowIntensityAll(pKiller);
            al::registerExecutorActorDraw(pKiller, rInfo.getExecuteDirector(),
                                          "デプスシャドウ[キャラクター]");
        }
        mKillers->registerActor(pKiller);
    }
    mStepAppear = static_cast<int>(al::getActionFrameMax(mKillers->getDeriveActor(0), "Appear"));
    al::tryGetArg(&mIsValidateCollision, rInfo, "IsValidateCollision");
    if (al::listenStageSwitchOnOffStart(this, al::Functor(this, &KillerGenerator::startShoot),
                                       al::Functor(this, &KillerGenerator::stopShoot))) {
        mIsShoot = false;
        mStandByKiller = mKillers->tryFindDeadDeriveActor();
        mStandByKiller->forceStandBy();
        al::setNerve(this, &NrvKillerGeneratorDeactive);
    }
    al::listenStageSwitchOnKill(this, al::Functor(this, &KillerGenerator::killBySwitch));
    makeActorAppeared();
}

/** @brief Enables firing and applies the configured switch-on delay. */
void KillerGenerator::startShoot() {
    mIsShoot = true;
    if (mIsShootImmediatelySwitchOn || al::isNerve(this, &NrvKillerGeneratorDeactive)) {
        if (mStepDelay > 0) {
            al::setNerve(this, &NrvKillerGeneratorShootDelay);
        } else {
            shoot();
            al::setNerve(this, &NrvKillerGeneratorWait);
        }
    }
}

/** @brief Kills the generator and all live projectiles when the death switch activates. */
void KillerGenerator::killBySwitch() {
    if (al::isDead(this)) {
        return;
    }
    if (mStandByKiller) {
        al::tryStartActionIfNotPlaying(mHost, "Attack");
    }
    kill();
    for (int i = 0; i < mKillers->mNumActors; i++) {
        if (al::isAlive(mKillers->getDeriveActor(i))) {
            mKillers->getDeriveActor(i)->killBySwitch();
        }
    }
}

/** @brief Hides the generator and any projectile waiting at the muzzle.
 * @return Result of the base actor's hide operation.
 */
bool KillerGenerator::hideActor() {
    if (mStandByKiller) {
        mStandByKiller->hideActor();
    }
    return al::LiveActor::hideActor();
}

/** @brief Shows the generator and any projectile waiting at the muzzle.
 * @return Result of the base actor's show operation.
 */
bool KillerGenerator::showActor() {
    if (mStandByKiller) {
        mStandByKiller->showActor();
    }
    return al::LiveActor::showActor();
}

/** @brief Kills the generator and clears its waiting projectile. */
void KillerGenerator::kill() {
    al::LiveActor::kill();
    if (mStandByKiller) {
        mStandByKiller->kill();
        mStandByKiller = nullptr;
    }
}

/** @brief Updates the generator pose and any projectile still attached to it.
 * @param rTrans World position of the muzzle.
 * @param rQuat World orientation of the launcher.
 */
void KillerGenerator::updateQT(const sead::Vector3f& rTrans, const sead::Quatf& rQuat) {
    al::updatePoseQuat(this, rQuat);
    al::setTrans(this, rTrans);
    if (mStandByKiller) {
        al::updatePoseQuat(mStandByKiller, al::getQuat(this));
        al::setTrans(mStandByKiller, al::getTrans(this));
    }
}

/** @brief Releases the waiting projectile when its standby animation is complete. */
void KillerGenerator::shoot() {
    if (mStandByKiller && mStandByKiller->isStandBy()) {
        mStandByKiller->startFlyWait();
        mStandByKiller = nullptr;
    }
}

/** @brief Starts an action on the launcher if its model contains that action.
 * @param pAction Action name.
 * @return Whether the action exists and was started.
 */
bool KillerGenerator::tryStartHostAction(const char* pAction) {
    if (al::isExistAction(mHost, pAction)) {
        al::startAction(mHost, pAction);
        return true;
    }
    return false;
}

/** @brief Calculates projectile acceleration relative to its default value.
 * @return Acceleration ratio, or zero for a near-zero acceleration.
 */
float KillerGenerator::getAccelRate() const {
    if (al::isNearZero(8.0f, 0.001f) || al::isNearZero(mAccelFly, 0.001f)) {
        return 0.0f;
    }
    return mAccelFly / 8.0f;
}

/** @brief Waits for the firing switch while inactive. */
void KillerGenerator::exeDeactive() {}

/** @brief Finishes the initial or switch-triggered firing delay. */
void KillerGenerator::exeDelay() {
    if (al::isGreaterEqualStep(this, mStepDelay)) {
        if (al::isNerve(this, &NrvKillerGeneratorShootDelay)) {
            shoot();
            al::setNerve(this, &NrvKillerGeneratorWait);
        } else {
            al::setNerve(this, &NrvKillerGeneratorShootKeepAppear);
        }
    }
}

/** @brief Appears a pooled projectile and waits for its appearance animation. */
void KillerGenerator::exeStandByAppear() {
    if (al::isFirstStep(this) && !mStandByKiller) {
        mStandByKiller = mKillers->tryFindDeadDeriveActor();
        if (mStandByKiller) {
            mStandByKiller->startStandByAppear();
        }
    }
    if (al::isGreaterEqualStep(this, mStepAppear)) {
        al::setNerve(this, &NrvKillerGeneratorStandBy);
    }
}

/** @brief Holds the projectile at the muzzle before the next shot. */
void KillerGenerator::exeStandBy() {
    if (al::isGreaterEqualStep(this, 60)) {
        if (mIsShoot) {
            shoot();
        }
        al::setNerve(this, &NrvKillerGeneratorWait);
    }
}

/** @brief Waits for the remainder of the firing interval. */
void KillerGenerator::exeWait() {
    if (al::isGreaterEqualStep(this, mStepWait - 61 - mStepAppear)) {
        al::setNerve(this, &NrvKillerGeneratorStandByAppear);
    }
}

/** @brief Appears the first projectile before sustained firing begins. */
void KillerGenerator::exeShootKeepAppear() {
    if (al::isFirstStep(this)) {
        mStandByKiller = mKillers->tryFindDeadDeriveActor();
        if (mStandByKiller) {
            mStandByKiller->startStandByAppear();
        }
    }
    if (al::isGreaterEqualStep(this, 104)) {
        al::setNerve(this, &NrvKillerGeneratorShootKeepShoot);
    }
}

/** @brief Fires once and waits before restarting sustained firing. */
void KillerGenerator::exeShootKeepShoot() {
    if (al::isFirstStep(this)) {
        shoot();
    }
    if (al::isGreaterEqualStep(this, mStepWait)) {
        al::setNerve(this, &NrvKillerGeneratorShootKeepAppear);
    }
}
