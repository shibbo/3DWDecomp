#include "Demo/DemoEffectObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace DemoSceneActorFunction {
void calcPlacementBaseMtx(sead::Matrix34f* pOut, const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx, sead::Matrix34f* pLocalMtx);
}

namespace {
/** @brief Initializes model and effect services. @param pActor Effect actor. @param rInfo Placement data. @param effectName Effect resource name. */
void initEffectActor(al::LiveActor* pActor, const al::ActorInitInfo& rInfo, const char* effectName) {
    if (al::isExistArchive(al::StringTmp<128>("ObjectData/%s", effectName))) {
        al::initMapPartsActor(pActor, rInfo, nullptr, 0);
    } else {
        al::initActorSceneInfo(pActor, rInfo);
        al::initActorPoseTQSV(pActor);
        al::initActorSRT(pActor, rInfo);
        al::initActorClipping(pActor, rInfo);
        al::initStageSwitch(pActor, rInfo);
        al::initExecutorUpdate(pActor, rInfo, "エフェクトオブジェ");
        alPlacementFunction::isEnableGroupClipping(rInfo);
    }
    al::initActorEffectKeeper(pActor, rInfo, effectName, true);
}
}

/** @brief Creates an effect actor with identity transforms. @param pName Actor name. */
DemoEffectObj::DemoEffectObj(const char* pName) : al::LiveActor(pName), mBaseMtx(sead::Matrix34f::ident) {
    mLocalMtx.makeIdentity();
}

/**
 * @brief Initializes the actor and resolves its demo placement transform.
 * @param rInfo Effect actor initialization data.
 * @param rDemoInfo Parent demo initialization data.
 * @param pBaseMtx Optional parent placement matrix.
 */
void DemoEffectObj::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx) {
    init(rInfo);
    mPlacementBaseMtx = pBaseMtx;
    DemoSceneActorFunction::calcPlacementBaseMtx(&mBaseMtx, rInfo, rDemoInfo, pBaseMtx, &mLocalMtx);
    al::updatePoseMtx(this, &mBaseMtx);
}

/** @brief Updates the effect's parent placement. @param pBaseMtx Parent transform. */
void DemoEffectObj::setPlacementBaseMtx(const sead::Matrix34f* pBaseMtx) {
    mPlacementBaseMtx = pBaseMtx;
    mBaseMtx.setMul(*pBaseMtx, mLocalMtx);
    al::updatePoseMtx(this, &mBaseMtx);
}

/** @brief Initializes the optional model, effects, and appearance switches. @param rInfo Actor initialization data. */
void DemoEffectObj::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = nullptr;
    alPlacementFunction::tryGetModelName(&modelName, rInfo);
    initEffectActor(this, rInfo, modelName);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
    al::listenStageSwitchOnOff(this, "OnKillOffAppearSwitch",
        al::Functor(this, &DemoEffectObj::kill), al::Functor(this, &DemoEffectObj::appear));
    al::invalidateClipping(this);
}

/** @brief Requires no additional setup after placement. */
void DemoEffectObj::initAfterPlacement() {}

/** @brief Activates the actor and starts its waiting effect and sound. */
void DemoEffectObj::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::emitEffect(this, "Wait", nullptr);
    al::tryStartSe(this, "Wait", nullptr);
}

/** @brief Leaves the externally controlled demo transform unchanged. */
void DemoEffectObj::control() {}

/** @brief Shows the actor through the standard appearance path. */
void DemoEffectObj::appear() {
    al::LiveActor::appear();
}

/** @brief Removes all emitted particles and kills the actor. */
void DemoEffectObj::kill() {
    al::tryDeleteEmitterAndParticleAll(this);
    al::LiveActor::kill();
}
