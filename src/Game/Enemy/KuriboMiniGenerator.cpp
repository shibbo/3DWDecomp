#include "Enemy/KuriboMiniGenerator.hpp"
#include "Enemy/KuriboMini.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include <cmath>

bool KuriboMiniKillChecker(s32 index);

namespace {
NERVE_DECL(KuriboMiniGenerator, Wait)
NERVE_DECL(KuriboMiniGenerator, AfterGenerate)
NERVES_MAKE_NOSTRUCT(KuriboMiniGenerator, Wait, AfterGenerate)
}

/** @brief Creates an empty generator. @param pName Actor name. */
KuriboMiniGenerator::KuriboMiniGenerator(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates the enemies and connects their release switch. @param rInfo Placement information. */
void KuriboMiniGenerator::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    s32 count = 6;
    al::tryGetArg(&count, rInfo, "KuriboMiniNum");
    al::initNerve(this, &NrvKuriboMiniGeneratorWait, 0);
    auto* pGroup = new al::DeriveActorGroup<KuriboMini>("マメクリボーグループ", count);
    mGroup = pGroup;
    for (s32 i = 0; i < pGroup->getMaxActorCount(); ++i) {
        auto* pActor = new KuriboMini("マメクリボー");
        al::initCreateActorWithPlacementInfo(pActor, rInfo);
        pGroup->registerActor(pActor);
    }
    for (s32 i = 0; i < mGroup->getActorCount(); ++i)
        mGroup->getDeriveActor(i)->makeActorDead();
    if (al::isValidSwitchAppear(this))
        al::listenStageSwitchOnAppear(this, al::Functor(this, &KuriboMiniGenerator::release));
    else
        generate();
    makeActorAppeared();
}

/** @brief Launches the remaining enemies outward from the generator. */
void KuriboMiniGenerator::release() {
    for (s32 i = 0; i < mGroup->getActorCount(); ++i) {
        auto* pActor = mGroup->getDeriveActor(i);
        const sead::Vector3f& rCenter = al::getTrans(this);
        sead::Vector3f pos = sead::Vector3f(
            sinf((360.0f / mGroup->getActorCount()) * i * 0.01745329238474369f) * 100.0f, 0.0f,
            cosf((360.0f / mGroup->getActorCount()) * i * 0.01745329238474369f) * 100.0f) + rCenter;
        al::setTrans(pActor, pos);
        sead::Vector3f direction = -(al::getTrans(this) - pos);
        f32 speedH = al::getRandom(0.7f, 1.3f) * 8.0f;
        f32 speedV = al::getRandom(0.8f, 1.2f) * 25.0f;
        al::setVelocitySeparateHV(pActor, direction, speedH, speedV);
        if (KuriboMiniKillChecker(i)) {
            pActor->makeActorAppeared();
            pActor->appearStart();
        } else {
            pActor->kill();
        }
    }
    al::setNerve(this, &NrvKuriboMiniGeneratorAfterGenerate);
}

/** @brief Places the remaining enemies in a circle. */
void KuriboMiniGenerator::generate() {
    for (s32 i = 0; i < mGroup->getActorCount(); ++i) {
        auto* pActor = mGroup->getDeriveActor(i);
        const sead::Vector3f& rCenter = al::getTrans(this);
        sead::Vector3f pos = sead::Vector3f(
            sinf((360.0f / mGroup->getActorCount()) * i * 0.01745329238474369f) * 100.0f, 0.0f,
            cosf((360.0f / mGroup->getActorCount()) * i * 0.01745329238474369f) * 100.0f) + rCenter;
        al::setTrans(pActor, pos);
        if (KuriboMiniKillChecker(i))
            pActor->makeActorAppeared();
        else
            pActor->kill();
    }
    al::setNerve(this, &NrvKuriboMiniGeneratorAfterGenerate);
}

/** @brief Waits for the release switch. */
void KuriboMiniGenerator::exeWait() {}

/** @brief Removes the generator once every enemy is gone. */
void KuriboMiniGenerator::exeAfterGenerate() {
    for (s32 i = 0; i < mGroup->getActorCount(); ++i)
        if (al::isAlive(mGroup->getDeriveActor(i)))
            return;
    kill();
}
