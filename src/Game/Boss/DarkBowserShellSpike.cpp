#include "Boss/DarkBowserShellSpike.hpp"
#include "Boss/DarkBowser.hpp"
#include "Enemy/BowserSpike.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
NERVE_DECL(DarkBowserShellSpike, Begin);
NERVE_DECL(DarkBowserShellSpike, LaunchSpike);
NERVE_DECL(DarkBowserShellSpike, End);
NERVES_MAKE_NOSTRUCT(DarkBowserShellSpike, Begin, LaunchSpike, End)
struct LaunchInfo { sead::Vector3f pos; sead::Quatf quat; };
const LaunchInfo sLaunchInfos[] = {
    {{1000.0f,2000.0f,1000.0f},{0.985599f,0.119573f,0.0f,-0.119573f}},
    {{1000.0f,2000.0f,-1000.0f},{0.985599f,-0.119573f,0.0f,-0.119573f}},
    {{-1000.0f,2000.0f,1000.0f},{0.985599f,0.119573f,0.0f,0.119573f}},
    {{-1000.0f,2000.0f,-1000.0f},{0.985599f,-0.119573f,0.0f,0.119573f}}
};
struct LevelParam { int batchCount; int spikeCount; int interval; float distance; };
const LevelParam sLevelParams[] = {{3,12,2,5000.0f},{3,12,2,5000.0f},{3,12,1,5000.0f}};
const float sAngles[] = {0,15,30,45,60,75,90,105,120,135,150,165,180,195,210,225,240,255,270,285,300,315,330,345};
const BowserSpikeParam sSpikeParam = {60,120.0f,100.0f,60,0.85f};
}
/** @brief Creates the shell spike state and its projectile pool. @param pHost Fury Bowser. @param rInfo Initialization data. */
DarkBowserShellSpike::DarkBowserShellSpike(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<DarkBowser>("ShellSpike", pHost) {
    initNerve(&NrvDarkBowserShellSpikeBegin, 0);
    mSpikes.tryAllocBuffer(48, nullptr, 8);
    for (int i = 0; i < 48; ++i) {
        auto* spike = new BowserSpike("BowserSpike", nullptr);
        al::initCreateActorNoPlacementInfo(spike, rInfo);
        mSpikes.pushBack(spike);
    }
}
/** @brief Destroys the attack state. */
DarkBowserShellSpike::~DarkBowserShellSpike() = default;
/** @brief Resets the launch sequence and selects the attack level tuning. */
void DarkBowserShellSpike::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvDarkBowserShellSpikeBegin);
    mLaunchCount = 0;
    mTarget = al::getTrans(getHost());
    mBatchCount = sLevelParams[mLevel].batchCount;
}
/** @brief Recalls the spikes and restores the boss sensors. */
void DarkBowserShellSpike::kill() {
    for (int i = 0; i < 48; ++i) mSpikes.at(i)->delaunch();
    getHost()->resetHitSensors();
    al::NerveStateBase::kill();
}
/** @brief Immediately removes every pooled spike. */
void DarkBowserShellSpike::forceKill() {
    for (int i = 0; i < 48; ++i) mSpikes[i]->kill();
}
/** @brief Attacks and strongly pushes contacted actors. @param pSelf Own sensor. @param pOther Target sensor. */
void DarkBowserShellSpike::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    al::sendMsgGigaEnemyAttack(pOther, pSelf);
    al::sendMsgPushVeryStrong(pOther, pSelf);
}
/** @brief Prepares a group of spikes around the shell. */
void DarkBowserShellSpike::exeBegin() {
    if (al::isFirstStep(this)) {
        al::startAction(getHost(), "ShellStart");
        for (int i = 0; i < sLevelParams[mLevel].spikeCount; ++i)
            mSpikes.at((sLevelParams[mLevel].spikeCount * mPoolGroup + i) % 48)->quickEnd();
    }
    if (al::isStep(this, 42)) {
        al::invalidateHitSensors(getHost());
        al::validateHitSensor(getHost(), "ShellDive");
    }
    if (al::isActionEnd(getHost()) && al::isGreaterEqualStep(this, 120)) {
        al::setNerve(this, &NrvDarkBowserShellSpikeLaunchSpike);
        mTarget = al::getTrans(getHost()->getPlayer());
        shuffle();
        sead::Vector3f pos = al::getTrans(getHost());
        for (int i = 0; i < sLevelParams[mLevel].spikeCount; ++i) {
            auto* spike = mSpikes.at((sLevelParams[mLevel].spikeCount * mPoolGroup + i) % 48);
            spike->prepareLaunch(pos + sLaunchInfos[i % 4].pos, sLaunchInfos[i % 4].quat, false);
        }
        al::startAction(getHost(), "ShellSpikeAttack");
        ++mAttackCount;
    }
}
/** @brief Randomizes the launch angle order. */
void DarkBowserShellSpike::shuffle() {
    for (int i = 0; i < 24; ++i) mOrder[i] = i;
    for (int i = 0; i < 24; ++i) {
        int j = al::getRandom(0, 23);
        int temp = mOrder[j];
        mOrder[j] = mOrder[i];
        mOrder[i] = temp;
    }
}
/** @brief Launches one spike and waits for the next interval. */
void DarkBowserShellSpike::exeLaunchSpike() {
    if (al::isFirstStep(this)) launchRandom();
    if (al::isGreaterEqualStep(this, sLevelParams[mLevel].interval)) al::setNerve(this, &NrvDarkBowserShellSpikeEnd);
}
/** @brief Launches the next spike toward the target with a randomized angle. */
void DarkBowserShellSpike::launchRandom() {
    int index = (mPoolGroup * sLevelParams[mLevel].spikeCount + mLaunchCount) % 48;
    float distance = ((mLevel == 2 ? mLaunchCount + 4 : mLaunchCount) / 2 + 1) * sLevelParams[mLevel].distance;
    float angle = sAngles[mOrder[mLaunchCount / 2]];
    if (mLaunchCount % 2) angle += al::getRandom(0.0f, 180.0f) + 90.0f;
    if (mLaunchCount == sLevelParams[mLevel].spikeCount - 1) distance = 0.0f;
    mSpikes.at(index)->launch(&mTarget, angle, distance, &sSpikeParam);
    ++mLaunchCount;
}
/** @brief Continues launching or finishes the shell attack. */
void DarkBowserShellSpike::exeEnd() {
    if (al::isFirstStep(this)) {
        if (mLaunchCount < sLevelParams[mLevel].spikeCount) {
            mTarget = al::getTrans(getHost()->getPlayer());
            al::setNerve(this, &NrvDarkBowserShellSpikeLaunchSpike);
            return;
        }
        al::startAction(getHost(), "ShellToStand");
    }
    if (al::isStep(this, 15)) getHost()->resetHitSensors();
    if (al::isGreaterEqualStep(this, 13) && al::isLessEqualStep(this, 51)) al::turnToTarget(getHost(), getHost()->getPlayer(), 6.3157897f);
    if (al::isActionEnd(getHost())) {
        mPoolGroup = (mPoolGroup + 1) % 3;
        kill();
    }
}
