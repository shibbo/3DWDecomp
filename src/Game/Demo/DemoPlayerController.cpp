#include "Demo/DemoPlayerController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Normal/PlayerRetargettingSelectorSceneObj.hpp"
#include "Util/PlayerUtil.hpp"

/** @brief Creates an unassigned demo controller. */
DemoPlayerController::DemoPlayerController() : mPlayerActor(nullptr), mIsStarted(false) {}

/** @brief Assigns the controlled player. @param pActor Player actor. */
void DemoPlayerController::setPlayerActor(PlayerActor* pActor) { mPlayerActor = pActor; }

/** @brief Starts the player's demo state. @return Whether the demo has started. */
bool DemoPlayerController::tryStartDemo() {
    if (mPlayerActor->startDemo()) {
        mPlayerActor->getModelHolder()->show();
        mIsStarted = true;
    }
    return mIsStarted;
}

/** @brief Restores the player on the ground and clears the assignment. */
void DemoPlayerController::endDemo() {
    mPlayerActor->restartOnGround(al::getTrans(mPlayerActor));
    mPlayerActor->endDemo();
    mPlayerActor = nullptr;
    mIsStarted = false;
}

/** @brief Gets the player position. @return Position. */
const sead::Vector3f& DemoPlayerController::getTrans() const { return mPlayerActor->getProperty()->mTrans; }
/** @brief Gets the player facing direction. @return Front vector. */
const sead::Vector3f& DemoPlayerController::getFrontVec() const { return mPlayerActor->getProperty()->mFront; }
/** @brief Gets the player up direction. @return Up vector. */
const sead::Vector3f& DemoPlayerController::getUpVec() const { return mPlayerActor->getProperty()->mGroundUp; }
/** @brief Sets the demo scale. @param scale Scale factor. */
void DemoPlayerController::setScale(float scale) { mPlayerActor->getProperty()->_78 = scale; }

/** @brief Places the player. @param rTrans New position. */
void DemoPlayerController::setTrans(const sead::Vector3f& rTrans) {
    mPlayerActor->getProperty()->mTrans = rTrans;
    al::resetPosition(mPlayerActor, rTrans, false);
}

/** @brief Places the player and resets dynamics. @param rTrans New position. */
void DemoPlayerController::setTransAndResetDynamics(const sead::Vector3f& rTrans) {
    setTrans(rTrans);
    rc::resetPlayerDynamics(mPlayerActor);
}

/** @brief Places the player and updates attached actors without advancing animation.
 * @param rTrans New position. */
void DemoPlayerController::resetPosition(const sead::Vector3f& rTrans) {
    setTrans(rTrans);
    auto* model = mPlayerActor->getModelHolder()->getCurrentModel();
    alLiveActorFunction::calcAnimDirect(model);
    auto* keeper = model->getSubActorKeeper();
    if (!keeper) return;
    int count = keeper->getSubActorNum();
    for (int i = 0; i < count; ++i) {
        auto* info = keeper->getSubActorInfo(i);
        if (info->mSyncType & 8) continue;
        auto* actor = info->mSubActor;
        float skl = 0.0f, mtp = 0.0f, vis = 0.0f;
        if (al::isSklAnimExist(actor)) {
            skl = al::getSklAnimFrameRate(actor, 0);
            al::setSklAnimBlendFrameRateAll(actor, 0.0f, false);
        }
        if (al::isMtpAnimExist(actor)) {
            mtp = al::getMtpAnimFrameRate(actor);
            al::setMtpAnimFrameRate(actor, 0.0f);
        }
        if (al::isVisAnimExist(actor)) {
            vis = al::getVisAnimFrameRate(actor);
            al::setVisAnimFrameRate(actor, 0.0f);
        }
        actor->movement();
        alLiveActorFunction::calcAnimDirect(actor);
        if (al::isSklAnimExist(actor)) al::setSklAnimBlendFrameRateAll(actor, skl, true);
        if (al::isMtpAnimExist(actor)) al::setMtpAnimFrameRate(actor, mtp);
        if (al::isVisAnimExist(actor)) al::setVisAnimFrameRate(actor, vis);
    }
}

/** @brief Sets player orientation. @param rFront Front vector. @param rUp Up vector. */
void DemoPlayerController::setFrontUpVec(const sead::Vector3f& rFront, const sead::Vector3f& rUp) {
    mPlayerActor->getProperty()->setFrontVec(rFront);
    mPlayerActor->getProperty()->setUpVec(rUp);
}

/** @brief Sets player rotation. @param rRotate Euler angles in degrees. */
void DemoPlayerController::setRotate(const sead::Vector3f& rRotate) {
    sead::Matrix34f mtx;
    mtx.makeR(rRotate * (sead::Mathf::pi() / 180.0f));
    sead::Vector3f up(mtx(0, 1), mtx(1, 1), mtx(2, 1));
    sead::Vector3f front(mtx(0, 2), mtx(1, 2), mtx(2, 2));
    setFrontUpVec(front, up);
}

/** @brief Sets player orientation. @param rQuat Rotation quaternion. */
void DemoPlayerController::setQuat(const sead::Quatf& rQuat) {
    sead::Matrix34f mtx;
    mtx.makeQT(rQuat, sead::Vector3f::zero);
    sead::Vector3f up(mtx(0, 1), mtx(1, 1), mtx(2, 1));
    sead::Vector3f front(mtx(0, 2), mtx(1, 2), mtx(2, 2));
    setFrontUpVec(front, up);
}

/** @brief Sets player velocity. @param rVelocity New velocity. */
void DemoPlayerController::setVelocity(const sead::Vector3f& rVelocity) {
    mPlayerActor->getProperty()->mVelocity = rVelocity;
}

/** @brief Starts an animation with appropriate retargetting. @param pActionName Animation name. */
void DemoPlayerController::startAction(const char* pActionName) {
    auto* selector = al::getSceneObj<PlayerRetargettingSelectorSceneObj>(mPlayerActor, 25);
    auto* model = mPlayerActor->getModelHolder()->getCurrentModel();
    IUsePlayerRetargettingSelector* useSelector = selector;
    al::StringTmp<128> name;
    volatile PlayerAnimFunc::RetargettingType type;
    type = PlayerAnimFunc::convertToRegularName(&name, model, pActionName);
    PlayerAnimFunc::controlRetargetting(model, useSelector, type);
    al::startAction(model, name.cstr());
    auto* keeper = model->getSubActorKeeper();
    if (!keeper) return;
    int count = keeper->getSubActorNum();
    for (int i = 0; i < count; ++i) {
        auto* info = keeper->getSubActorInfo(i);
        if (!al::tryStartAction(info->mSubActor, name.cstr()))
            al::tryStartAction(info->mSubActor, "Wait");
    }
}

/** @brief Sets the animation frame. @param frame Frame index. */
void DemoPlayerController::setActionFrame(int frame) {
    al::setActionFrame(mPlayerActor->getModelHolder()->getCurrentModel(), frame);
}
/** @brief Checks animation completion. @return Whether playback has ended. */
bool DemoPlayerController::isActionEnd() const {
    return al::isActionEnd(mPlayerActor->getModelHolder()->getCurrentModel());
}

/** @brief Freezes skeletal animation and removes player effects. */
void DemoPlayerController::stopSklAnimAndDeleteEffect() {
    al::setSklAnimBlendFrameRateAll(mPlayerActor->getModelHolder()->getCurrentModel(), 0.0f, false);
    al::tryDeleteEmitterAndParticleAll(mPlayerActor->getModelHolder()->getCurrentModel());
    al::tryDeleteEmitterAndParticleAll(mPlayerActor);
}

/** @brief Hides the player and its silhouette and removes the statue. */
void DemoPlayerController::hide() {
    mPlayerActor->getModelHolder()->hide();
    mPlayerActor->getModelHolder()->hideSilhouette();
    mPlayerActor->killManekineko();
}

/** @brief Shows the player and updates its attached actors. */
void DemoPlayerController::show() {
    mPlayerActor->getModelHolder()->show();
    mPlayerActor->getModelHolder()->showSilhouette();
    auto* keeper = mPlayerActor->getModelHolder()->getCurrentModel()->getSubActorKeeper();
    if (!keeper) return;
    int count = keeper->getSubActorNum();
    for (int i = 0; i < count; ++i)
        keeper->getSubActorInfo(i)->mSubActor->movement();
}
