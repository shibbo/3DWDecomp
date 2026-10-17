#include "Enemy/SyumockConveyerGenerator.hpp"
#include "Enemy/SyumockConveyer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/LiveActor/ConveyerKeyKeeper.hpp"

namespace {
NERVE_DECL(SyumockConveyerGenerator, Move)
NERVES_MAKE_NOSTRUCT(SyumockConveyerGenerator, Move)
}

/** @brief Creates the conveyor generator. @param pName Actor name. */
SyumockConveyerGenerator::SyumockConveyerGenerator(const char* pName) : al::LiveActor(pName) {}

/** @brief Creates and positions the conveyor's enemies. @param rInfo Placement information. */
void SyumockConveyerGenerator::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "SyumockConveyerGenerator", nullptr);
    al::initNerve(this, &NrvSyumockConveyerGeneratorMove, 0);
    if (mActorSceneInfo->isSingleMode)
        al::validateClipping(this);
    mKeys = new al::ConveyerKeyKeeper();
    mKeys->init(rInfo);
    al::tryGetArg(&mMoveSpeed, rInfo, "MoveSpeed");
    al::tryGetArg(&mInterval, rInfo, "Interval");
    if (mInterval < 10.0f)
        mInterval = 10.0f;
    f32 distance = mKeys->getTotalMoveDistance();
    if (mKeys->getConveyerKeyCount() > 1)
        al::isNearZero(distance);
    s32 count = static_cast<s32>(distance / mInterval) + 1;
    mMaxCoord = mInterval * count;
    f32 startRate = 0.0f;
    al::tryGetArg(&startRate, rInfo, "StartRate");
    mCoord = al::wrapValue(mInterval * startRate, mInterval);
    auto* pActors = new al::DeriveActorGroup<SyumockConveyer>(
        "\u30b7\u30e5\u30e2\u30c3\u30af\u30b3\u30f3\u30d9\u30a2\u7ba1\u7406", count);
    mActors = pActors;
    for (s32 i = 0; i < pActors->getMaxActorCount(); ++i) {
        auto* pActor = new SyumockConveyer("\u30b7\u30e5\u30e2\u30c3\u30af\u30b3\u30f3\u30d9\u30a2");
        al::initCreateActorWithPlacementInfo(pActor, rInfo);
        pActors->registerActor(pActor);
    }
    for (s32 i = 0; i < count; ++i) {
        auto* pActor = mActors->getDeriveActor(i);
        pActor->setConveyerKeyKeeper(mKeys, mMaxCoord);
        pActor->setTransByCoord(i * mInterval + mCoord, true, true);
    }
    makeActorAppeared();
}

/** @brief Suspends each active conveyor enemy when the generator is clipped. */
void SyumockConveyerGenerator::startClipped() {
    al::LiveActor::startClipped();
    for (s32 i = 0; i < mActors->getActorCount(); ++i) {
        auto* pActor = mActors->getDeriveActor(i);
        if (!al::isClipped(pActor) && !al::isDead(pActor))
            pActor->startClipped();
    }
}

/** @brief Resumes each living conveyor enemy when clipping ends. */
void SyumockConveyerGenerator::endClipped() {
    al::LiveActor::endClipped();
    for (s32 i = 0; i < mActors->getActorCount(); ++i) {
        auto* pActor = mActors->getDeriveActor(i);
        if (al::isClipped(pActor) && !al::isDead(pActor))
            pActor->endClipped();
    }
}

/** @brief Advances the conveyor and updates the evenly spaced enemies. */
void SyumockConveyerGenerator::exeMove() {
    mCoord = al::wrapValue(mCoord + mMoveSpeed, mMaxCoord);
    s32 count = mActors->getActorCount();
    f32 speed = mMoveSpeed;
    for (s32 i = 0; i < count; ++i)
        mActors->getDeriveActor(i)->setTransByCoord(mInterval * i + mCoord, speed >= 0.0f, false);
}
