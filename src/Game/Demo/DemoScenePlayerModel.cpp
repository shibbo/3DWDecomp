#include "Demo/DemoScenePlayerModel.hpp"

#include "Library/ActorUtil.hpp"
#include "Library/Controller/PadRumbleDirector.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/PartsModel.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerRetargettingSelector.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Controller/PadRumbleKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"

namespace PlayerModelHolderCreator {
PlayerModelHolder* createModelHolderAll(const al::ActorInitInfo&, const char*,
    IUsePlayerRetargettingInfoCreator*, al::IUseHioNode*, bool, unsigned int, const char*, int);
PlayerModelHolder* createModelHolder(const al::ActorInitInfo&, const char*, int,
    IUsePlayerRetargettingInfoCreator*, al::IUseHioNode*, unsigned int, const char*);
}

namespace {
/** @brief Starts a retargeted animation on a player and its subactors.
 * @param pModel Player model. @param pName Animation name. @param pSelector Retargeting selector. */
void startPlayerAction(PlayerModel* pModel, const char* pName,
                       IUsePlayerRetargettingSelector* pSelector) {
    al::StringTmp<128> name;
    volatile PlayerAnimFunc::RetargettingType type;
    type = PlayerAnimFunc::convertToRegularName(&name, pModel, pName);
    PlayerAnimFunc::controlRetargetting(pModel, pSelector, type);
    al::startAction(pModel, name.cstr());
    al::SubActorKeeper* keeper = pModel->mSubActorKeeper;
    if (!keeper)
        return;
    int count = keeper->getSubActorNum();
    for (int i = 0; i < count; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);
        if (!al::tryStartAction(info->mSubActor, name.cstr()))
            al::tryStartAction(info->mSubActor, "Wait");
    }
}
}

/** @brief Creates an unassigned demo model controller. */
DemoScenePlayerModel::DemoScenePlayerModel() = default;

/** @brief Creates every figure of a demo character. @param rInfo Initialization context.
 * @param pCharacter Character name. @param pSelector Retargeting selector.
 * @param pHost Host actor. @param pSuffix Resource suffix. @return Model controller. */
DemoScenePlayerModel* DemoScenePlayerModel::createAll(const al::ActorInitInfo& rInfo,
    const char* pCharacter, PlayerRetargettingSelector* pSelector, al::IUseHioNode* pHost,
    const char* pSuffix) {
    DemoScenePlayerModel* result = new DemoScenePlayerModel;
    result->mSelector = pSelector;
    PlayerModelHolder* models = PlayerModelHolderCreator::createModelHolderAll(
        rInfo, pCharacter, pSelector, pHost, false, 16, pSuffix, 0);
    result->mModels = models;
    models->initCurrentModel(0);
    al::PadRumbleKeeper* rumble = al::createPadRumbleKeeper(models->getModel(0), -1);
    for (int i = 1; i < 10; i++) {
        PlayerModel* model = models->getModel(i);
        if (model && !al::isExistPadRumbleKeeper(model))
            al::setPadRumbleKeeper(model, rumble);
    }
    if (al::isSingleMode(rInfo)) {
        PlayerModel* model = models->getModel(7);
        if (model) {
            result->mClimbCloth = new al::PartsModel("布パーツ");
            result->mClimbCloth->initPartsFixFile(model, rInfo, "ClimbCloth", nullptr, "ClimbCloth");
            result->mClimbCloth->makeActorDead();
            result->mClimbCloth->mIsForceHide = true;
        }
    }
    result->mModels->hideShadow();
    result->mModels->hideSilhouette();
    return result;
}

/** @brief Creates one figure of a demo character. @param rInfo Initialization context.
 * @param pCharacter Character name. @param figure Figure index. @param pSelector Retargeting selector.
 * @param pHost Host actor. @param pSuffix Resource suffix. @return Model controller. */
DemoScenePlayerModel* DemoScenePlayerModel::createSingle(const al::ActorInitInfo& rInfo,
    const char* pCharacter, int figure, PlayerRetargettingSelector* pSelector,
    al::IUseHioNode* pHost, const char* pSuffix) {
    DemoScenePlayerModel* result = new DemoScenePlayerModel;
    result->mSelector = pSelector;
    result->mModels = PlayerModelHolderCreator::createModelHolder(
        rInfo, pCharacter, figure, pSelector, pHost, 16, pSuffix);
    PlayerModel* model = result->mModels->getCurrentModel();
    if (model->mActionKeeper || model->mHitReactionKeeper)
        al::createPadRumbleKeeper(model, -1);
    pSelector->applyDefaultRetargettingInfo(model, model->getRetargettingIndex());
    result->mModels->hideShadow();
    result->mModels->hideSilhouette();
    return result;
}

/** @brief Kills the current figure and hides its optional cloth. */
void DemoScenePlayerModel::kill() {
    if (mModels->getCurrentIndex() == 7 && mClimbCloth) {
        mClimbCloth->kill();
        mClimbCloth->mIsForceHide = true;
    }
    mModels->kill();
}

/** @brief Stops the current figure's effects and sounds. */
void DemoScenePlayerModel::killAllEffect() {
    PlayerModel* model = mModels->getCurrentModel();
    if (model) {
        if (model->getEffectKeeper())
            al::tryKillEmitterAndParticleAll(model);
        if (model->getAudioKeeper())
            al::stopAllSeFromUser(model, 0);
    }
}

/** @brief Gets the current figure. @return Player model. */
PlayerModel* DemoScenePlayerModel::getPlayerModel() { return mModels->getCurrentModel(); }
/** @brief Shows the current figure. */
void DemoScenePlayerModel::appear() { mModels->appear(); }
/** @brief Clears follow and delayed-action state. */
void DemoScenePlayerModel::initialize() {
    mFollowMtx = nullptr;
    mStartDelay = 0;
    mActionStarted = false;
    mActionName.clear();
}
/** @brief Gets the current figure as an actor. @return Model actor. */
al::LiveActor* DemoScenePlayerModel::getModel() { return mModels->getCurrentModel(); }

/** @brief Shows the figure at the demo transform. @param rMtx Demo placement. */
void DemoScenePlayerModel::startDemo(const sead::Matrix34f& rMtx) {
    mModels->appear();
    al::updatePoseMtx(mModels->getCurrentModel(), &rMtx);
    if (mModels->getCurrentIndex() == 7 && mClimbCloth) {
        mClimbCloth->appear();
        mClimbCloth->mIsForceHide = false;
    }
}

/** @brief Starts or queues a retargeted action. @param pName Animation name. */
void DemoScenePlayerModel::startAction(const char* pName) {
    mActionStarted = false;
    mActionName = pName;
    if (mStartDelay > 0)
        return;
    mActionStarted = true;
    startPlayerAction(mModels->getCurrentModel(), pName, mSelector);
}

/** @brief Updates the follow transform and delayed animation. @param pActor Timing owner. */
void DemoScenePlayerModel::update(al::LiveActor* pActor) {
    if (mFollowMtx) {
        al::updatePoseMtx(mModels->getCurrentModel(), mFollowMtx);
        sead::Vector3f scale;
        scale.x = mFollowMtx->getBase(0).length();
        scale.y = mFollowMtx->getBase(1).length();
        scale.z = mFollowMtx->getBase(2).length();
        al::setScale(mModels->getCurrentModel(), scale);
    }
    if (!mActionStarted && al::isGreaterEqualStep(pActor, mStartDelay)) {
        mActionStarted = true;
        startPlayerAction(mModels->getCurrentModel(), mActionName.cstr(), mSelector);
    }
}

/** @brief Switches the current figure and its retargeting data. @param figure Figure index. */
void DemoScenePlayerModel::setFigure(int figure) {
    mModels->change(figure);
    PlayerModel* model = mModels->getCurrentModel();
    mSelector->applyDefaultRetargettingInfo(model, model->getRetargettingIndex());
    al::setSeSeqLocalVariableDefault(model, 1, figure);
}

/** @brief Sets the sound material. @param pCode Material code. */
void DemoScenePlayerModel::setSeMaterialCode(const char* pCode) {
    al::tryUpdateSeMaterialCode(mModels->getCurrentModel(), pCode);
}
/** @brief Clears the sound material override. */
void DemoScenePlayerModel::resetSeMaterialCode() {
    al::tryUpdateSeMaterialCode(mModels->getCurrentModel(), "NoCode");
}
/** @brief Assigns the actor using this model. @param pActor Demo actor. */
void DemoScenePlayerModel::setDemoActor(const al::LiveActor* pActor) { mDemoActor = pActor; }
/** @brief Releases ownership and clears the sound material override. */
void DemoScenePlayerModel::releaseDemoActor() {
    resetSeMaterialCode();
    mDemoActor = nullptr;
}
/** @brief Shows the model's shadow. */
void DemoScenePlayerModel::showShadow() { mModels->showShadow(); }
/** @brief Hides the model's shadow. */
void DemoScenePlayerModel::hideShadow() { mModels->hideShadow(); }
/** @brief Shows the model's silhouette. */
void DemoScenePlayerModel::showSilhouette() { mModels->showSilhouette(); }
/** @brief Hides the model's silhouette. */
void DemoScenePlayerModel::hideSilhouette() { mModels->hideSilhouette(); }
/** @brief Gets the optional climb-cloth part. @return Cloth actor, or null. */
al::PartsModel* DemoScenePlayerModel::getClimbCloth() { return mClimbCloth; }
