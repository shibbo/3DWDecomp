#include "Enemy/ModelHolder.hpp"
#include "Enemy/ModelHolderActor.hpp"
#include "Library/ActorUtil.hpp"

/** @brief Allocates the model array. @param capacity Maximum number of model actors. */
ModelHolder::ModelHolder(s32 capacity) { mModels.allocBuffer(capacity, nullptr); }

/** @brief Appears the selected model. */
void ModelHolder::appear() { mModels(mCurrentIndex)->appear(); }
/** @brief Kills the selected model. */
void ModelHolder::kill() { mModels(mCurrentIndex)->kill(); }
/** @brief Gets the selected model. @return Current model, or nullptr if no model is selected. */
ModelHolderActor* ModelHolder::getCurrentModel() const { return mModels[mCurrentIndex]; }
/** @brief Selects and appears the initial model. @param index Initial model index. */
void ModelHolder::initCurrentModel(s32 index) {
    mCurrentIndex = index;
    mModels(mCurrentIndex)->appear();
}

/** @brief Registers a model and leaves it inactive. @param pModel Model actor. */
void ModelHolder::registerModel(ModelHolderActor* pModel) {
    mModels.pushBack(pModel);
    pModel->kill();
}
/** @brief Creates and registers a model actor.
 * @param pOwner Actor receiving the model's sensor messages. @param rInfo Scene information. @param pName Actor and archive name.
 */
void ModelHolder::registerModel(al::LiveActor* pOwner, const al::ActorInitInfo& rInfo, const char* pName) {
    ModelHolderActor* pModel = new ModelHolderActor(pName, pOwner);
    al::initActorWithArchiveName(pModel, rInfo, pName, nullptr);
    registerModel(pModel);
}

/** @brief Changes models while retaining pose, alive state, and visibility. @param index New model index. */
void ModelHolder::changeModel(s32 index) {
    if (mCurrentIndex == index) { return; }
    al::copyPose(mModels[index], getCurrentModel());
    al::resetPosition(mModels[index], false);
    bool isAlive = al::isAlive(getCurrentModel());
    bool isHidden = al::isHideModel(getCurrentModel());
    kill();
    mCurrentIndex = index;
    if (isAlive) {
        appear();
        mModels(mCurrentIndex)->calcAnim();
    }
    if (isHidden) { al::hideModelIfShow(getCurrentModel()); }
    else { al::showModelIfHide(getCurrentModel()); }
}

/** @brief Starts an action on the current model. @param pName Action name. */
void ModelHolder::startAction(const char* pName) { al::startAction(getCurrentModel(), pName); }
/** @brief Tests whether the current action ended. @return Whether the action ended. */
bool ModelHolder::isActionEnd() const { return al::isActionEnd(getCurrentModel()); }
/** @brief Copies the owner's pose onto the current model. @param pActor Pose source. */
void ModelHolder::updatePosture(const al::LiveActor* pActor) { al::copyPose(getCurrentModel(), pActor); }
