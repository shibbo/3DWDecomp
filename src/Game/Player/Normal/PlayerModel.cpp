#include "Player/Normal/PlayerModel.hpp"

#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointLookAtController.hpp"
#include "Library/Joint/JointRumbler.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorResourceUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Player/FurKeeper.hpp"
#include "Player/FurShape.hpp"
#include "Player/Normal/PlayerAnimFunc.hpp"
#include "Player/Normal/PlayerModelHair.hpp"
#include "Player/Normal/PlayerModelIK.hpp"
#include "Util/PlayerUtil.hpp"

/**
 * @brief Constructs and initializes the model actor of a player figure.
 * @param pName Actor name.
 * @param pArchiveName Name of the model archive.
 * @param rInfo Actor init info.
 * @param pSuffix Suffix of the init files, or nullptr.
 */
PlayerModel::PlayerModel(const char* pName, const char* pArchiveName,
                         const al::ActorInitInfo& rInfo, const char* pSuffix)
    : al::LiveActor(pName) {
    mFurKeepers.allocBuffer(5, nullptr);
    al::initActorWithArchiveName(this, rInfo, pArchiveName, pSuffix);
    al::invalidateClipping(this);
    makeActorDead();
    rc::createInvincibleUboWithSubActor(this);
}

/**
 * @brief Draws the fur of the model.
 */
void PlayerModel::draw() const {
    s32 furNum = mFurKeepers.size();
    for (s32 i = 0; i < furNum; i++) {
        mFurKeepers.at(i)->draw();
    }
}

/**
 * @brief Updates the actor.
 */
void PlayerModel::movement() {
    al::LiveActor::movement();
}

/**
 * @brief Calculates the animation and updates the fur's uniform buffers.
 */
void PlayerModel::calcAnim() {
    al::LiveActor::calcAnim();

    s32 furNum = mFurKeepers.size();
    for (s32 i = 0; i < furNum; i++) {
        mFurKeepers.at(i)->updateUbo();
    }
}

/**
 * @brief Receives a message from a screen pointer.
 * @param pMsg Message.
 * @param pPointer Screen pointer.
 * @param pTarget Screen point target.
 * @return True if the message is a touch assist message.
 */
bool PlayerModel::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                        al::ScreenPointTarget* pTarget) {
    return al::isMsgTouchAssist(pMsg);
}

/**
 * @brief Starts an action on the model and its sub actors.
 * @param pActionName Action name (converted to the regular name of this model).
 * @param pSelector Selector of the retargetting info.
 */
void PlayerModel::startAction(const char* pActionName, IUsePlayerRetargettingSelector* pSelector) {
    al::StringTmp<128> actionName;
    PlayerAnimFunc::RetargettingType type =
        PlayerAnimFunc::convertToRegularName(&actionName, this, pActionName);
    PlayerAnimFunc::controlRetargetting(this, pSelector, type);
    al::startAction(this, actionName.cstr());

    al::SubActorKeeper* keeper = mSubActorKeeper;
    if (keeper == nullptr) {
        return;
    }

    s32 subActorNum = keeper->getSubActorNum();
    for (s32 i = 0; i < subActorNum; i++) {
        al::SubActorInfo* info = keeper->getSubActorInfo(i);
        if (!al::tryStartAction(info->mSubActor, actionName.cstr())) {
            al::tryStartAction(info->mSubActor, "Wait");
        }
    }
}

/**
 * @brief Sets the action frame of the model and its sub actors.
 * @param frame Frame.
 */
void PlayerModel::setFrame(s32 frame) {
    f32 actionFrame = frame;
    al::trySetActionFrame(this, actionFrame);

    al::SubActorKeeper* keeper = mSubActorKeeper;
    if (keeper == nullptr) {
        return;
    }

    s32 subActorNum = keeper->getSubActorNum();
    for (s32 i = 0; i < subActorNum; i++) {
        al::trySetActionFrame(keeper->getSubActorInfo(i)->mSubActor, actionFrame);
    }
}

namespace {

/**
 * @brief Counts the hair joints listed in the actor's "InitHairDynamics" init file.
 * @param pActor Actor.
 * @return Number of hair joints.
 */
s32 calcHairJointNum(const al::LiveActor* pActor) {
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&iter, al::getModelResource(pActor), "InitHairDynamics",
                                     nullptr)) {
        return 0;
    }

    s32 groupNum = iter.getSize();
    s32 jointNum = 0;
    for (s32 i = 0; i < groupNum; i++) {
        al::ByamlIter groupIter;
        if (iter.tryGetIterByIndex(&groupIter, i)) {
            jointNum += groupIter.getSize();
        }
    }

    return jointNum;
}

}  // namespace

/**
 * @brief Initializes the joint controllers (mashing, rumbling, spine rotation, head look-at).
 * @param rPitchRange Pitch range of the head look-at.
 * @param pIsMash Flag enabling the joint mashing, or nullptr for no mashing.
 * @param pSpineQuat Rotation applied to the spine, or nullptr for none.
 * @param lookAtRate Rate of the head look-at.
 */
void PlayerModel::initJointController(const sead::Vector2f& rPitchRange, const bool* pIsMash,
                                      const sead::Quatf* pSpineQuat, f32 lookAtRate) {
    s32 controllerNum = 3;
    if (pIsMash != nullptr) {
        controllerNum++;
    }

    if (pSpineQuat != nullptr) {
        controllerNum++;
    }

    al::initJointControllerKeeper(this, controllerNum + calcHairJointNum(this));

    if (pIsMash != nullptr) {
        al::JointMasher* masher = al::initJointMasher(this, pIsMash, 6);
        al::appendMashJoint(masher, "Head", 0.1f);
        al::appendMashJoint(masher, "ArmL1", 0.1f);
        al::appendMashJoint(masher, "ArmR1", 0.1f);
        al::appendMashJoint(masher, "HandL", 0.1f);
        al::appendMashJoint(masher, "HandR", 0.1f);
        al::appendMashJoint(masher, "Spine2", 0.8f);
    }

    mRumblers[0] = al::initJointRumbler(this, "Spine1", 2.2f, 0.4f, 30, 0);
    mRumblers[0]->initDetails(al::JointRumbler::EAxis_Y, 3, 0.3f);
    mRumblers[0]->initDetails(al::JointRumbler::EAxis_Z, 6, 0.3f);
    mRumblers[1] = al::initJointRumbler(this, "Spine1", 1.2f, 0.5f, 20, 0);
    mRumblers[1]->initDetails(al::JointRumbler::EAxis_Y, 3, 0.3f);
    mRumblers[1]->initDetails(al::JointRumbler::EAxis_Z, 6, 0.3f);

    if (pSpineQuat != nullptr) {
        al::initJointLocalQuatRotator(this, "Spine2", pSpineQuat);
    }

    mLookAtCtrl = al::initJointLookAtController(this, 1);
    al::appendJointLookAtController(mLookAtCtrl, this, "Head", lookAtRate, {-45.0f, 45.0f},
                                    rPitchRange, sead::Vector3f::ex, -sead::Vector3f::ey);
}

/**
 * @brief Starts one of the spine rumblers.
 * @param index Index of the rumbler.
 */
void PlayerModel::startRumble(u32 index) {
    mRumblers[index]->start();
}

/**
 * @brief Sets the invincible color of the model and its sub actors.
 * @param rColor Color.
 */
void PlayerModel::setInvincibleColor(const sead::Color4f& rColor) {
    rc::setInvincibleColorWithSubActor(this, rColor);
}

/**
 * @brief Creates the fur of the model and its sub actors (from their "InitFur" files), hidden.
 */
void PlayerModel::createFur() {
    if (al::isExistModelResourceYaml(this, "InitFur", nullptr)) {
        auto* furKeeper = new FurKeeper();
        al::StringTmp<128> name("FurKeeper of %s", getName());
        name.cstr();
        furKeeper->init(this, nullptr);
        mFurKeepers.pushBack(furKeeper);
    }

    al::SubActorKeeper* keeper = mSubActorKeeper;
    if (keeper != nullptr) {
        s32 subActorNum = keeper->getSubActorNum();
        for (s32 i = 0; i < subActorNum; i++) {
            al::SubActorInfo* info = keeper->getSubActorInfo(i);
            if (info->mSyncType & 8) {
                continue;
            }

            al::LiveActor* subActor = info->mSubActor;
            if (!al::isExistModelResourceYaml(subActor, "InitFur", nullptr)) {
                continue;
            }

            auto* furKeeper = new FurKeeper();
            al::StringTmp<128> name("FurKeeper of %s", subActor->getName());
            name.cstr();
            furKeeper->init(subActor, nullptr);
            mFurKeepers.pushBack(furKeeper);
        }
    }

    hideFur();
}

/**
 * @brief Hides all fur shapes.
 */
void PlayerModel::hideFur() {
    s32 furNum = mFurKeepers.size();
    for (s32 i = 0; i < furNum; i++) {
        FurKeeper* furKeeper = mFurKeepers.at(i);
        s32 shapeNum = furKeeper->getShapeNum();
        for (s32 j = 0; j < shapeNum; j++) {
            furKeeper->getShape(j)->setUnk14(true);
        }
    }
}

/**
 * @brief Shows all fur shapes.
 */
void PlayerModel::showFur() {
    s32 furNum = mFurKeepers.size();
    for (s32 i = 0; i < furNum; i++) {
        FurKeeper* furKeeper = mFurKeepers.at(i);
        s32 shapeNum = furKeeper->getShapeNum();
        for (s32 j = 0; j < shapeNum; j++) {
            furKeeper->getShape(j)->setUnk14(false);
        }
    }
}

/**
 * @brief Creates the foot IK.
 */
void PlayerModel::createIK() {
    mIK = new PlayerModelIK(this, nullptr);
}

/**
 * @brief Creates the hair controller, if the model has a hair sub actor.
 */
void PlayerModel::createHairCtrl() {
    al::LiveActor* hairActor = al::tryGetSubActor(this, "髪");
    if (hairActor != nullptr) {
        mHairCtrl = new PlayerModelHair(hairActor, this, nullptr);
    }
}

/**
 * @brief Sets the drop length of the model's shadow.
 * @param length Drop length.
 */
void PlayerModel::setShadowLength(f32 length) {
    if (al::isHideShadow(this, "Base")) {
        al::setShadowDropLength(this, length, "Circle");
        return;
    }

    al::setShadowDropLength(this, length, "Base");
    al::setShadowDropLengthEvenWithDrawCategory(this, al::ShadowMaskDrawCategory::Player, this,
                                                "Base");

    s32 subActorNum = al::getSubActorNum(this);
    for (s32 i = 0; i < subActorNum; i++) {
        al::LiveActor* subActor = al::getSubActor(this, i);
        if (al::isExistShadow(subActor)) {
            al::setShadowDropLengthEvenWithDrawCategory(
                subActor, al::ShadowMaskDrawCategory::Player, this, "Base");
        }
    }
}

/**
 * @brief Creates the spring controllers of the skirt sub actor, if there is one.
 */
void PlayerModel::createSkirtDynamics() {
    al::LiveActor* skirt = al::tryGetSubActor(this, "スカート");
    if (skirt == nullptr) {
        mIsValidSkirtDynamics = false;
        return;
    }

    mSkirtSprings.allocBuffer(5, nullptr);
    al::initJointControllerKeeper(skirt, 5);

    for (s32 i = 0; i < 5; i++) {
        al::StringTmp<128> jointName("Skirt%d", i + 1);
        al::JointSpringController* spring = al::initJointSpringController(skirt, jointName.cstr());
        spring->setStability(0.07f);
        spring->setFriction(0.8f);
        spring->setLimitDegree(5.0f);
        mSkirtSprings.pushBack(spring);
    }

    mIsValidSkirtDynamics = true;
}

/**
 * @brief Enables the skirt spring controllers.
 */
void PlayerModel::validateSkirtDynamics() {
    if (!mSkirtSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mSkirtSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mSkirtSprings.at(i)->setControlRate(1.0f);
    }

    mIsValidSkirtDynamics = true;
}

/**
 * @brief Disables the skirt spring controllers.
 */
void PlayerModel::invalidateSkirtDynamics() {
    if (!mSkirtSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mSkirtSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mSkirtSprings.at(i)->setControlRate(0.0f);
    }

    mIsValidSkirtDynamics = false;
}

/**
 * @brief Checks if the model has skirt dynamics.
 * @return True if the skirt spring controllers exist.
 */
bool PlayerModel::isExistSkirtDynamics() const {
    return mSkirtSprings.isBufferReady();
}

/**
 * @brief Checks if the skirt dynamics are enabled.
 * @return True if enabled.
 */
bool PlayerModel::isValidSkirtDynamics() const {
    return mIsValidSkirtDynamics;
}

/**
 * @brief Resets the skirt spring controllers.
 */
void PlayerModel::resetSkirtDynamics() {
    if (!mSkirtSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mSkirtSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mSkirtSprings.at(i)->reset();
    }
}

/**
 * @brief Creates the spring controllers of the tail sub actor (from its "InitTailDynamics" file).
 */
void PlayerModel::createTailJointController() {
    al::LiveActor* tail = al::tryGetSubActor(this, "尻尾");
    if (tail == nullptr) {
        return;
    }

    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&iter, al::getModelResource(tail), "InitTailDynamics",
                                     nullptr)) {
        return;
    }

    s32 controllerNum;
    if (!al::tryGetByamlS32(&controllerNum, iter, "JointControllerNum")) {
        return;
    }

    al::initJointControllerKeeper(tail, controllerNum);

    al::ByamlIter springsIter;
    if (!iter.tryGetIterByKey(&springsIter, "JointSpringController")) {
        return;
    }

    u32 springNum = springsIter.getSize();
    if (springNum == 0) {
        return;
    }

    mTailSprings.allocBuffer(springNum, nullptr);

    for (u32 i = 0; i < springNum; i++) {
        al::ByamlIter springIter;
        springsIter.tryGetIterByIndex(&springIter, i);

        const char* jointName = nullptr;
        if (!springIter.tryGetStringByKey(&jointName, "Joint")) {
            return;
        }

        al::JointSpringController* spring = al::initJointSpringController(tail, jointName);

        sead::Vector3f childLocalPos;
        if (al::tryGetByamlV3f(&childLocalPos, springIter, "ChildLocalPos")) {
            spring->setChildLocalPos(childLocalPos);
        }

        f32 stability;
        if (al::tryGetByamlF32(&stability, springIter, "Stability")) {
            spring->setStability(stability);
        }

        f32 friction;
        if (al::tryGetByamlF32(&friction, springIter, "Friction")) {
            spring->setFriction(friction);
        }

        f32 limitDegree;
        if (al::tryGetByamlF32(&limitDegree, springIter, "LimitDegree")) {
            spring->setLimitDegree(limitDegree);
        }

        mTailSprings.pushBack(spring);
    }

    mIsValidTailDynamics = true;
}

/**
 * @brief Enables the tail spring controllers.
 */
void PlayerModel::validateTailDynamics() {
    if (!mTailSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mTailSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mTailSprings.at(i)->setControlRate(1.0f);
    }

    mIsValidTailDynamics = true;
}

/**
 * @brief Disables the tail spring controllers.
 */
void PlayerModel::invalidateTailDynamics() {
    if (!mTailSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mTailSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mTailSprings.at(i)->setControlRate(0.0f);
    }

    mIsValidTailDynamics = false;
}

/**
 * @brief Checks if the model has tail dynamics.
 * @return True if the tail spring controllers exist.
 */
bool PlayerModel::isExistTailDynamics() const {
    return mTailSprings.isBufferReady();
}

/**
 * @brief Checks if the tail dynamics are enabled.
 * @return True if enabled.
 */
bool PlayerModel::isValidTailDynamics() const {
    return mIsValidTailDynamics;
}

/**
 * @brief Resets the tail spring controllers.
 */
void PlayerModel::resetTailDynamics() {
    if (!mTailSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mTailSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mTailSprings.at(i)->reset();
    }
}

/**
 * @brief Creates the hair spring controllers (from the "InitHairDynamics" file).
 */
void PlayerModel::createHairJointController() {
    al::ByamlIter iter;
    if (!al::tryGetActorInitFileIter(&iter, al::getModelResource(this), "InitHairDynamics",
                                     nullptr)) {
        return;
    }

    s32 jointNum = calcHairJointNum(this);
    u32 groupNum = iter.getSize();
    if (groupNum == 0) {
        return;
    }

    mHairSprings.allocBuffer(jointNum, nullptr);

    for (u32 i = 0; i < groupNum; i++) {
        al::ByamlIter groupIter;
        iter.tryGetIterByIndex(&groupIter, i);

        s32 springNum = groupIter.getSize();
        for (s32 j = 0; j < springNum; j++) {
            al::ByamlIter springIter;
            groupIter.tryGetIterByIndex(&springIter, j);

            const char* jointName = nullptr;
            if (!springIter.tryGetStringByKey(&jointName, "Joint")) {
                return;
            }

            al::JointSpringController* spring = al::initJointSpringController(this, jointName);

            sead::Vector3f childLocalPos;
            if (al::tryGetByamlV3f(&childLocalPos, springIter, "ChildLocalPos")) {
                spring->setChildLocalPos(childLocalPos);
            }

            f32 stability;
            if (al::tryGetByamlF32(&stability, springIter, "Stability")) {
                spring->setStability(stability);
            }

            f32 friction;
            if (al::tryGetByamlF32(&friction, springIter, "Friction")) {
                spring->setFriction(friction);
            }

            f32 limitDegree;
            if (al::tryGetByamlF32(&limitDegree, springIter, "LimitDegree")) {
                spring->setLimitDegree(limitDegree);
            }

            mHairSprings.pushBack(spring);
        }
    }

    mIsValidHairDynamics = true;
}

/**
 * @brief Enables the hair spring controllers.
 */
void PlayerModel::validateHairDynamics() {
    if (!mHairSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mHairSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mHairSprings.at(i)->setControlRate(1.0f);
    }

    mIsValidHairDynamics = true;
}

/**
 * @brief Disables the hair spring controllers.
 */
void PlayerModel::invalidateHairDynamics() {
    if (!mHairSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mHairSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mHairSprings.at(i)->setControlRate(0.0f);
    }

    mIsValidHairDynamics = false;
}

/**
 * @brief Checks if the model has hair dynamics.
 * @return True if the hair spring controllers exist.
 */
bool PlayerModel::isExistHairDynamics() const {
    return mHairSprings.isBufferReady();
}

/**
 * @brief Checks if the hair dynamics are enabled.
 * @return True if enabled.
 */
bool PlayerModel::isValidHairDynamics() const {
    return mIsValidHairDynamics;
}

/**
 * @brief Resets the hair spring controllers.
 */
void PlayerModel::resetHairDynamics() {
    if (!mHairSprings.isBufferReady()) {
        return;
    }

    s32 springNum = mHairSprings.size();
    for (s32 i = 0; i < springNum; i++) {
        mHairSprings.at(i)->reset();
    }
}

/**
 * @brief Creates the standard controllers of a model (IK, hair, skirt, tail and hair dynamics).
 * @param pModel Model.
 */
void PlayerModel::createStandardCtrl(PlayerModel* pModel) {
    pModel->createIK();
    pModel->createHairCtrl();
    pModel->createSkirtDynamics();
    pModel->createTailJointController();
    pModel->createHairJointController();
}

/**
 * @brief Updates the spine rumblers.
 */
void PlayerModel::control() {
    mRumblers[0]->update();
    mRumblers[1]->update();
}
