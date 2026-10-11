#include "MapObj/Tree.hpp"

#include <cmath>

#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Yaml/ByamlUtil.hpp"
#include "MapObj/ActorStateGiantBlow.hpp"
#include "MapObj/BgmRhythmAnimeController.hpp"
#include "MapObj/BindPuppeteerGroup.hpp"
#include "MapObj/CoinBlowGenerator.hpp"
#include "MapObj/TreeBindPuppeteer.hpp"
#include "Project/Audio/AudioSystem.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/DrcUtil.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/PlayerPuppetUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    NERVE_DECL(Tree, Wait);
    NERVE_DECL(Tree, GiantBlow);
    NERVE_DECL(Tree, AwaitRespawn);
    NERVE_DECL(Tree, ReactionStart);
    NERVE_DECL(Tree, Reaction);
    NERVE_DECL(Tree, Release);
    NERVE_DECL(Tree, HoldStart);
    NERVE_DECL(Tree, Hold);
    NERVE_DECL(Tree, ReactionEnd);
    NERVES_MAKE_NOSTRUCT(Tree, Wait, GiantBlow, AwaitRespawn, ReactionStart, Reaction, Release,
                         HoldStart, Hold, ReactionEnd)

    ActorStateGiantBlowParam sGiantBlowParam;
}  // namespace

/**
 * @brief Constructs a tree.
 * @param pName Name of the actor.
 */
Tree::Tree(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initializes the models, sensors, climb puppeteers, item and giant blow state of the tree.
 * @param rInfo Actor init info.
 */
void Tree::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = "Tree";
    alPlacementFunction::tryGetModelName(&modelName, rInfo);

    if (al::isEqualString(modelName, "TreeConiferWithSnow")) {
        al::initActorWithArchiveNameWithPlacementInfo(this, rInfo, "TreeConifer", nullptr);
        mSnowModel = new al::LiveActor("雪モデル");
        al::initActorWithArchiveNameWithPlacementInfo(mSnowModel, rInfo, "TreeConiferSnow",
                                                      nullptr);
        al::startAction(mSnowModel, "On");
        mSnowModel->makeActorAppeared();
        // Virtual slot 0x130 (getName in the current LiveActor declaration); result unused.
        mSnowModel->getName();
    } else if (al::isEqualString(modelName, "TreePalm")) {
        mIsPalm = true;
        al::initActorWithArchiveName(this, rInfo, modelName, nullptr);
    } else if (al::isEqualString(modelName, "TreeFurry")) {
        s32 color;
        al::tryGetArg(&color, rInfo, "TreeColor");

        if (color == 1) {
            al::initActorWithArchiveName(this, rInfo, modelName, "Blue");
        } else if (color == 2) {
            al::initActorWithArchiveName(this, rInfo, modelName, "Red");
        } else {
            al::initActorWithArchiveName(this, rInfo, modelName, nullptr);
        }
    } else {
        al::initActorWithArchiveName(this, rInfo, modelName, nullptr);
    }

    al::initNerve(this, &NrvTreeWait, 1);

    al::ByamlIter treeInfo(nullptr);
    sead::Vector3f clippingOffset(0.0f, 300.0f, 0.0f);
    bool isUseRhythmAnimControl = false;

    if (al::isExistModelResourceYaml(this, "TreeInfo", nullptr)) {
        treeInfo = al::ByamlIter(al::getModelResourceYaml(this, "TreeInfo", nullptr));
        al::tryGetByamlF32(&mSensorBottomOffsetY, treeInfo, "SensorBottomOffsetY");
        al::tryGetByamlF32(&mSensorTopOffsetY, treeInfo, "SensorTopOffsetY");
        al::tryGetByamlV3f(&clippingOffset, treeInfo, "ClippingOffset");
        al::tryGetByamlF32(&mItemAppearOffsetY, treeInfo, "ItemAppearOffsetY");
        isUseRhythmAnimControl = al::tryGetByamlKeyBoolOrFalse(treeInfo, "IsUseRhythmAnimControl");
        al::tryGetByamlF32(&mJointRotateRate, treeInfo, "JointRotateRate");
        al::tryGetByamlF32(&mJointRotateRateAtClimbTree, treeInfo, "JointRotateRateAtClimbTree");
    }

    mClippingCenter = al::getTrans(this) + clippingOffset;
    al::setClippingInfo(this, al::getClippingRadius(this), &mClippingCenter);

    s32 userNumMax = rc::getControlUserNumMax();
    mBindSensorPos = new sead::Vector3f[userNumMax];
    mBindSensorPos[0].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Bind1", &mBindSensorPos[0]);
    al::invalidateHitSensor(this, "Bind1");
    mBindSensorPos[1].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Bind2", &mBindSensorPos[1]);
    al::invalidateHitSensor(this, "Bind2");
    mBindSensorPos[2].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Bind3", &mBindSensorPos[2]);
    al::invalidateHitSensor(this, "Bind3");
    mBindSensorPos[3].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Bind4", &mBindSensorPos[3]);
    al::invalidateHitSensor(this, "Bind4");

    mPuppeteerGroup = new BindPuppeteerGroup("木バインド操作グループ", userNumMax);
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNumMax(); i++) {
        mPuppeteerGroup->registerPuppeteer(new TreeBindPuppeteer("木バインド操作", this));
        getPuppeteer(i)->setParam(treeInfo);
    }

    mPuppetSensorPos = new sead::Vector3f[userNumMax];
    mPuppetSensorPos[0].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Puppet1", &mPuppetSensorPos[0]);
    al::invalidateHitSensor(this, "Puppet1");
    mPuppetSensorPos[1].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Puppet2", &mPuppetSensorPos[1]);
    al::invalidateHitSensor(this, "Puppet2");
    mPuppetSensorPos[2].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Puppet3", &mPuppetSensorPos[2]);
    al::invalidateHitSensor(this, "Puppet3");
    mPuppetSensorPos[3].set(al::getTrans(this));
    al::setHitSensorPosPtr(this, "Puppet4", &mPuppetSensorPos[3]);
    al::invalidateHitSensor(this, "Puppet4");

    if (al::isEqualString(modelName, "TreeLongC")) {
        s32 lightType = 0;
        al::tryGetArg(&lightType, rInfo, "LightType");
        al::startMclAnim(this, "TreeLongCLight");
        al::setMclAnimFrameAndStop(this, lightType);
    }

    if (al::isEqualString(modelName, "Tree") || al::isEqualString(modelName, "TreeLongA") ||
        al::isEqualString(modelName, "TreeLongB") || al::isEqualString(modelName, "TreeLongC") ||
        al::isEqualString(modelName, "TreeFurry")) {
        mBreakModel = new al::LiveActor("巨大マリオ吹き飛び壊れモデル");

        if (al::isEqualString(modelName, "TreeFurry")) {
            s32 color;
            al::tryGetArg(&color, rInfo, "TreeColor");

            if (color == 1) {
                al::initActorWithArchiveName(mBreakModel, rInfo, "TreeFurryBreak", "Blue");
            } else if (color == 2) {
                al::initActorWithArchiveName(mBreakModel, rInfo, "TreeFurryBreak", "Red");
            } else {
                al::initActorWithArchiveName(mBreakModel, rInfo, "TreeFurryBreak", nullptr);
            }
        } else {
            al::initActorWithArchiveName(mBreakModel, rInfo,
                                         al::StringTmp<64>("%sBreak", modelName), nullptr);
        }

        mBreakModel->makeActorDead();
    }

    mTraceModel = new al::LiveActor("巨大マリオ吹き飛び残骸");

    if (al::isEqualString(modelName, "TreeFurry")) {
        al::initActorWithArchiveName(mTraceModel, rInfo, "TreeFurryTrace", nullptr);
    } else {
        al::initActorWithArchiveName(mTraceModel, rInfo, "TreeTrace", nullptr);
    }

    mTraceModel->makeActorDead();

    sGiantBlowParam.mDuration = 10;
    sGiantBlowParam.mSpeed = 35.0f;
    sGiantBlowParam.mUpSpeed = 30.0f;
    sGiantBlowParam.mRotateAxis = 1;
    mGiantBlowState = new ActorStateGiantBlow(this, &sGiantBlowParam, mBreakModel, mTraceModel);
    al::initNerveState(this, mGiantBlowState, &NrvTreeGiantBlow, "巨大マリオ吹き飛び");

    al::initJointControllerKeeper(this, 8);

    if (al::isExistJoint(this, "Top")) {
        mTopJointMtx = *al::getJointMtxPtr(this, "Top");
        al::calcJointPos(&mTopJointPos, this, "Top");
        al::initJointLocalZRotator(this, &mJointRotateZ[0], "Top");
        al::initJointLocalZRotator(this, &mJointRotateZ[1], "Middle02");
        al::initJointLocalZRotator(this, &mJointRotateZ[2], "Middle01");
        al::initJointLocalZRotator(this, &mJointRotateZ[3], "Under");
        al::initJointLocalYRotator(this, &mJointRotateY[0], "Top");
        al::initJointLocalYRotator(this, &mJointRotateY[1], "Middle02");
        al::initJointLocalYRotator(this, &mJointRotateY[2], "Middle01");
        al::initJointLocalYRotator(this, &mJointRotateY[3], "Under");
    }

    al::calcFrontDir(&mFrontDir, this);
    const char* itemType = "None";
    al::tryGetStringArg(&itemType, rInfo, "ItemType");

    if (al::isEqualString(itemType, "None")) {
        mItemType = -1;
    } else if (al::isEqualString(itemType, "CoinBlow")) {
        al::ByamlIter coinInfo(nullptr);
        s32 coinNum = 8;
        f32 upSpeed = 35.0f;
        f32 frontSpeed = 7.0f;
        mItemAppearOffsetY = 250.0f;

        if (al::isExistModelResourceYaml(this, "CoinInfo", nullptr)) {
            coinInfo = al::ByamlIter(al::getModelResourceYaml(this, "CoinInfo", nullptr));
            al::tryGetByamlS32(&coinNum, coinInfo, "CoinNum");
            al::tryGetByamlF32(&upSpeed, coinInfo, "UpSpeed");
            al::tryGetByamlF32(&frontSpeed, coinInfo, "FrontSpeed");
            al::tryGetByamlF32(&mItemAppearOffsetY, coinInfo, "AppearOffsetY");
        }

        mCoinBlow = new CoinBlowGenerator("放出コイン");
        CoinBlowGenerator::InitParam param = {coinNum, upSpeed, 0.0f, frontSpeed};
        mCoinBlow->initWithParam(param, rInfo);
        al::invalidateClipping(mCoinBlow);
    } else {
        rc::initItemByHostInfo(this, rInfo, 1);
        mItemType = rc::getItemType(rInfo);
    }

    al::tryExpandClippingByExpandObject(this, rInfo);
    bool isRotateY = true;
    al::tryGetArg(&isRotateY, rInfo, "IsRotateY");

    if (isRotateY) {
        f32 degree = al::getRandom(-20.0f, 20.0f);
        al::rotateQuatYDirDegree(this, degree);
        al::rotateQuatYDirDegree(mTraceModel, degree);

        if (mSnowModel != nullptr) {
            al::rotateQuatYDirDegree(mSnowModel, degree);
        }
    }

    if (isUseRhythmAnimControl) {
        mRhythmAnimCtrl = new BgmRhythmAnimeController(this, false);

        if (mSnowModel != nullptr) {
            mSnowRhythmAnimCtrl = new BgmRhythmAnimeController(mSnowModel, false);
        }
    }

    if (rInfo.getActorSceneInfo().isSingleMode) {
        s32 color;
        al::tryGetArg(&color, rInfo, "TreeColor");

        if (al::isMtpAnimExist(this, "Color")) {
            al::startMtpAnimAndSetFrameAndStop(this, "Color", color);
        }

        if (mBreakModel != nullptr && al::isMtpAnimExist(mBreakModel, "Color")) {
            al::startMtpAnimAndSetFrameAndStop(mBreakModel, "Color", color);
        }
    }

    makeActorAppeared();
    const sead::Vector3f& trans = al::getTrans(this);
    mInitTrans.x = trans.x;
    mInitTrans.y = trans.y;
    mInitTrans.z = trans.z;
    const sead::Quatf& quat = al::getQuat(this);
    mInitQuat.x = quat.x;
    mInitQuat.y = quat.y;
    mInitQuat.z = quat.z;
    mInitQuat.w = quat.w;
    al::tryGetArg(&mIsSpecialReappear, rInfo, "SpecialReappear");

    if (al::calcLinkChildNum(rInfo, "SinkedItem") != 0) {
        ProjectActorFactory factory;
        mSinkedItem = al::createLinksActorFromFactory(factory, rInfo, "SinkedItem", 0);
        mSinkedItem->kill();
    }
}

/**
 * @brief Gets the climb puppeteer of a player.
 * @param index Index of the puppeteer.
 * @return The puppeteer.
 */
TreeBindPuppeteer* Tree::getPuppeteer(s32 index) const {
    return mPuppeteerGroup->getPuppeteer<TreeBindPuppeteer>(index);
}

/**
 * @brief Brings the tree back after it was blown away, unless a TreeFarLodWatcher handles it.
 */
void Tree::respawn() {
    if (al::isNerve(this, &NrvTreeAwaitRespawn) || al::isDead(this)) {
        if (mRespawnByWatcher) {
            return;
        }

        if (al::isDead(this)) {
            makeActorAppeared();
        }

        al::showModelIfHide(this);
        al::setTrans(this, mInitTrans);
        al::setQuat(this, mInitQuat);
        al::setNerve(this, &NrvTreeWait);
        al::validateCollisionParts(this);

        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            getPuppeteer(i)->validateBind();
        }
    }
}

/**
 * @brief Updates the sensors and climb puppeteers, and shakes the tree while players climb it.
 */
void Tree::control() {
    updateSensorPos();
    bool isClimb = false;
    bool isRelease = false;

    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        getPuppeteer(i)->update();
        bool isPuppeteerClimb = getPuppeteer(i)->isClimb();
        bool isPuppeteerRelease = getPuppeteer(i)->isRelease();

        if (getPuppeteer(i)->isGiantAtClimb()) {
            al::setNerve(this, &NrvTreeGiantBlow);
            return;
        }

        isRelease |= isPuppeteerRelease;
        isClimb |= isPuppeteerClimb;
    }

    if (isRelease) {
        if (!al::isActionPlaying(this, "Jump")) {
            al::startAction(this, "Jump");
        }
    } else if (isClimb) {
        bool isShaking = al::isActionPlaying(this, "Shake") && al::getActionFrame(this) < 20.0f;

        if (!isShaking) {
            al::startAction(this, "Shake");
        }
    }

    if (al::isNerve(this, &NrvTreeReactionStart) || al::isNerve(this, &NrvTreeReaction) ||
        al::isNerve(this, &NrvTreeRelease) || al::isNerve(this, &NrvTreeHoldStart) ||
        al::isNerve(this, &NrvTreeHold)) {
        return;
    }

    if (al::isMicInputOn(this)) {
        al::startAction(this, "Reaction");
        al::setNerve(this, &NrvTreeReactionStart);
    }
}

/**
 * @brief Moves the bind sensors to the nearest point of the trunk for each player, and the
 * puppet sensors to the climbing players.
 */
void Tree::updateSensorPos() {
    sead::Vector3f bottom = al::getTrans(this);
    sead::Vector3f top = al::getTrans(this);
    sead::Vector3f up;
    al::calcQuatUp(&up, this);
    bottom.y += mSensorBottomOffsetY * up.y;
    top.y += up.y * mSensorTopOffsetY;

    static const char* const cBindSensorNames[] = {"Bind1", "Bind2", "Bind3", "Bind4"};
    for (s32 i = 0; i < 4; i++) {
        al::LiveActor* player = rc::tryFindNearestPlayerActorByUserId(this, i, -1.0f);

        if (player != nullptr) {
            al::validateHitSensor(this, cBindSensorNames[i]);
            al::calcPerpendicFootToLineInside(&mBindSensorPos[i], al::getTrans(player), bottom,
                                              top);
        } else {
            al::invalidateHitSensor(this, cBindSensorNames[i]);
        }
    }

    static const char* const cPuppetSensorNames[] = {"Puppet1", "Puppet2", "Puppet3", "Puppet4"};
    for (s32 i = 0; i < 4; i++) {
        if (getPuppeteer(i)->isBind()) {
            al::validateHitSensor(this, cPuppetSensorNames[i]);
            const sead::Vector3f& puppetTrans =
                rc::getPuppetTrans(getPuppeteer(i)->getPlayerPuppet());
            sead::Vector3f& sensorPos = mPuppetSensorPos[i];
            sensorPos.x = puppetTrans.x;
            sensorPos.y = puppetTrans.y;
            sensorPos.z = puppetTrans.z;
        } else {
            al::invalidateHitSensor(this, cPuppetSensorNames[i]);
        }
    }
}

/**
 * @brief Clips the tree together with its snow model and coin generator.
 */
void Tree::startClipped() {
    if (mSnowModel != nullptr) {
        mSnowModel->startClipped();
    }

    if (mCoinBlow != nullptr && al::isAlive(mCoinBlow)) {
        mCoinBlow->startClipped();
    }

    al::LiveActor::startClipped();
}

/**
 * @brief Unclips the tree together with its snow model and coin generator, respawning it first
 * when it waits for a respawn out of sight of every player.
 */
void Tree::endClipped() {
    if (al::isNerve(this, &NrvTreeAwaitRespawn) &&
        rc::tryFindNearestActivePlayerActorInSphere(this, 8000.0f) == nullptr) {
        respawn();
    }

    al::LiveActor::endClipped();

    if (mSnowModel != nullptr) {
        mSnowModel->endClipped();
    }

    if (mCoinBlow != nullptr && al::isAlive(mCoinBlow)) {
        mCoinBlow->endClipped();
    }
}

/**
 * @brief Gives a ghost present to the climbing players touched through their puppet sensor.
 * @param pSelf Sensor of the tree.
 * @param pOther Touched sensor.
 */
void Tree::attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther) {
    if (!al::isSensorRide(pSelf)) {
        return;
    }

    static const char* const cPuppetSensorNames[] = {"Puppet1", "Puppet2", "Puppet3", "Puppet4"};
    for (s32 i = 0; i < 4; i++) {
        if (getPuppeteer(i)->isBind() && al::getHitSensor(this, cPuppetSensorNames[i]) == pSelf) {
            rc::sendMsgGhostPresentGet(pOther,
                                       rc::getPuppetSensor(getPuppeteer(i)->getPlayerPuppet()));
        }
    }
}

/**
 * @brief Handles giant blows, climb binds and the attacks that shake the tree.
 * @param pMsg Received message.
 * @param pOther Sensor of the sender.
 * @param pSelf Sensor of the tree.
 * @return Whether the message was accepted.
 */
bool Tree::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (rc::isMsgIsEnableExitStage(pMsg) || rc::isMsgIsEnableIslandWarp(pMsg)) {
        return true;
    }

    if (al::isNerve(this, &NrvTreeGiantBlow) || al::isNerve(this, &NrvTreeAwaitRespawn)) {
        return false;
    }

    if (mGiantBlowState->tryStartBlow(pMsg, pOther, pSelf) || al::isMsgLaserAttack(pMsg)) {
        if (al::isSensorRide(pOther)) {
            rc::requestHitReactionToAttacker(pMsg, pSelf, pOther);
        }

        rc::addScore(this, pOther, 0.0f, 0);
        al::setNerve(this, &NrvTreeGiantBlow);
        return true;
    }

    if (al::isMsgBindStart(pMsg)) {
        if (rc::isPlayerHoldingSomething(pOther)) {
            return false;
        }

        if (getPuppeteer(rc::findControlUserId(pOther))->isEnableBind()) {
            return true;
        }
    }

    if (al::isMsgBindInit(pMsg)) {
        getPuppeteer(rc::findControlUserId(pOther))->startBind(pOther, pSelf);
        rc::tryRequestClearFlingPoleDashFlag(al::getSensorHost(pOther));
        return true;
    }

    if (al::isMsgBindCancel(pMsg)) {
        getPuppeteer(rc::findControlUserId(pOther))->bindCancel();
        return true;
    }

    if (al::isNerve(this, &NrvTreeRelease)) {
        return false;
    }

    if (al::isMsgPlayerItemGet(pMsg) || al::isMsgPlayerBoomerangAttack(pMsg) ||
        al::isMsgPlayerFireBallAttack(pMsg) || al::isMsgPlayerCooperationHipDrop(pMsg) ||
        al::isMsgPlayerGiantHipDrop(pMsg)) {
        if (al::isNerve(this, &NrvTreeWait)) {
            al::startAction(this, "Reaction");
            al::setNerve(this, &NrvTreeReactionStart);
            return !al::isMsgPlayerFireBallAttack(pMsg);
        }

        if (al::isNerve(this, &NrvTreeReaction)) {
            if (al::isGreaterEqualStep(this, 30) && al::getActorVelocity(pOther).length() > 1.0f) {
                al::startAction(this, "Reaction");
                al::setNerve(this, &NrvTreeReactionStart);
                return !al::isMsgPlayerFireBallAttack(pMsg);
            }
        } else if (al::isNerve(this, &NrvTreeReactionEnd)) {
            if (al::getActorVelocity(pOther).length() > 1.0f) {
                al::startAction(this, "Reaction");
                al::setNerve(this, &NrvTreeReactionStart);
            } else {
                al::setNerve(this, &NrvTreeReactionEnd);
            }

            return !al::isMsgPlayerFireBallAttack(pMsg);
        }
    }

    if ((al::isMsgBallAttack(pMsg) || al::isMsgKickKouraItemGet(pMsg)) &&
        !al::isNerve(this, &NrvTreeReaction)) {
        al::startAction(this, "Reaction");
        al::setNerve(this, &NrvTreeReactionStart);
        return false;
    }

    if (al::isMsgPlayerRollingAttack(pMsg) && !al::isNerve(this, &NrvTreeReaction)) {
        al::tryOnStageSwitch(this, "SwitchReactionOn");
        al::startAction(this, "RollingAttack");
        al::setNerve(this, &NrvTreeReactionStart);
        mItemSensor = pOther;
        appearItem();
        return true;
    }

    return false;
}

/**
 * @brief Makes the item of the tree appear above it, once.
 */
void Tree::appearItem() {
    sead::Vector3f pos = al::getTrans(this) + sead::Vector3f::ey * mItemAppearOffsetY;

    if (mCoinBlow != nullptr) {
        al::setTrans(mCoinBlow, pos);
        mCoinBlow->setConcentric();
        mCoinBlow->appear();
        mCoinBlow = nullptr;
        return;
    }

    if (mItemType != -1) {
        al::appearItem(this, pos, mFrontDir, mItemSensor);
        mItemType = -1;
    }
}

/**
 * @brief Lets the touch screen grab and shake the tree.
 * @param pMsg Received message.
 * @param pPointer Screen pointer that touched the tree.
 * @param pTarget Touched screen point target.
 * @return Whether the message was accepted.
 */
bool Tree::receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                                 al::ScreenPointTarget* pTarget) {
    if (al::isNerve(this, &NrvTreeGiantBlow)) {
        return false;
    }

    if (!al::isMsgTouchAssist(pMsg)) {
        return false;
    }

    const sead::Vector3f& touchPos = al::getHitScreenPointTargetPos(pPointer);
    mTouchPos.x = touchPos.x;
    mTouchPos.y = touchPos.y;
    mTouchPos.z = touchPos.z;

    if (al::isNerve(this, &NrvTreeHold)) {
        al::setNerve(this, &NrvTreeHold);
    } else {
        mItemSensor = DrcFunction::tryFindDrcPlayerSensor(this, pPointer);
        al::setNerve(this, &NrvTreeHoldStart);
    }

    al::tryOnStageSwitch(this, "SwitchReactionOn");
    return true;
}

/**
 * @brief Forbids hand stands while a bound player already does one.
 */
void Tree::controlEnableHandStandFlag() {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        TreeBindPuppeteer* puppeteer = getPuppeteer(i);

        if (puppeteer->isBind() && puppeteer->isHandStand()) {
            mIsEnableHandStand = false;
            return;
        }
    }

    mIsEnableHandStand = true;
}

/**
 * @brief Checks whether a player climbs the tree.
 * @return Whether one of the puppeteers is bound.
 */
bool Tree::isExistClimbPlayer() const {
    for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
        if (getPuppeteer(i)->isBind()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Respawns the tree even when its respawn is left to a TreeFarLodWatcher.
 */
void Tree::forceRespawn() {
    bool isRespawnByWatcher = mRespawnByWatcher;
    mRespawnByWatcher = false;
    respawn();
    mRespawnByWatcher = isRespawnByWatcher;
}

/**
 * @brief Takes the snow off the tree when it still holds some.
 */
inline void Tree::tryOffSnow() {
    if (mSnowModel != nullptr && mIsSnowOn) {
        al::startAction(mSnowModel, "Off");
        mIsSnowOn = false;
    }
}

/**
 * @brief Moves the tree (and its snow) to the BGM rhythm.
 */
inline void Tree::updateRhythmAnim() {
    mRhythmAnimCtrl->update();

    if (mSnowModel != nullptr && mIsSnowOn) {
        mSnowRhythmAnimCtrl->update();
    }
}

/**
 * @brief Waits, putting back the debris of a blow and moving to the BGM rhythm.
 */
void Tree::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");

        if (mBreakModel != nullptr) {
            al::resetPosition(mBreakModel, mInitTrans, false);

            if (al::isAlive(mBreakModel)) {
                mBreakModel->kill();
            }
        }

        if (mTraceModel != nullptr) {
            al::resetPosition(mTraceModel, mInitTrans, false);

            if (al::isAlive(mTraceModel)) {
                mTraceModel->kill();
            }
        }

        if (mSinkedItem != nullptr) {
            al::resetPosition(mSinkedItem, mInitTrans + sead::Vector3f(0.0f, 25.0f, 0.0f), false);

            if (al::isAlive(mSinkedItem)) {
                mSinkedItem->kill();
            }
        }
    }

    if (mRhythmAnimCtrl != nullptr && !isExistClimbPlayer()) {
        updateRhythmAnim();
    }
}

/**
 * @brief Starts a shake reaction.
 */
void Tree::exeReactionStart() {
    if (al::isFirstStep(this)) {
        tryOffSnow();
    }

    al::setNerve(this, &NrvTreeReaction);
}

/**
 * @brief Plays the shake reaction until its end.
 */
void Tree::exeReaction() {
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTreeReactionEnd);
    }
}

/**
 * @brief Goes back to the wait animation after a shake reaction.
 */
void Tree::exeReactionEnd() {
    if (mIsPalm) {
        playAnimForPalm();
    } else {
        if (al::isStep(this, 2)) {
            al::startAction(this, "Wait");
        }

        if (al::isGreaterEqualStep(this, 2) && !isExistClimbPlayer() &&
            mRhythmAnimCtrl != nullptr) {
            updateRhythmAnim();
        }
    }

    if (al::isGreaterEqualStep(this, 5)) {
        al::setNerve(this, &NrvTreeWait);
    }
}

/**
 * @brief Starts the wait animation of a palm tree once its shake animation is over.
 */
void Tree::playAnimForPalm() {
    if (al::isActionPlaying(this, "Wait")) {
        return;
    }

    if (!al::isActionPlaying(this, "Shake") || al::isActionEnd(this)) {
        al::startAction(this, "Wait");
    }
}

/**
 * @brief Starts a touch screen hold, making the item appear.
 */
void Tree::exeHoldStart() {
    al::startAction(this, "Reaction");
    tryOffSnow();
    appearItem();
    al::setNerve(this, &NrvTreeHold);
}

/**
 * @brief Sets the rotation of every rotated joint.
 * @param rotateZ Rotation around the local Z axis.
 * @param rotateY Rotation around the local Y axis.
 */
inline void Tree::setJointRotate(f32 rotateZ, f32 rotateY) {
    mRotateY = rotateY;
    mJointRotateY[3] = rotateY;
    mJointRotateY[2] = rotateY;
    mJointRotateY[1] = rotateY;
    mJointRotateY[0] = rotateY;
    mRotateZ = rotateZ;
    mJointRotateZ[3] = rotateZ;
    mJointRotateZ[2] = rotateZ;
    mJointRotateZ[1] = rotateZ;
    mJointRotateZ[0] = rotateZ;
}

/**
 * @brief Bends the tree toward the touched position.
 */
void Tree::exeHold() {
    sead::Vector3f dir = mTouchPos - mTopJointPos;
    sead::Matrix34f viewMtx = al::getCameraViewMtx(this);
    sead::Vector3f viewDir;
    viewDir.setRotated(viewMtx, dir);

    sead::Matrix34f invViewMtx;
    invViewMtx.setInverse(viewMtx);
    sead::Vector3f worldDir;
    worldDir.setRotated(invViewMtx, sead::Vector3f(viewDir.x, 0.0f, 0.0f));

    sead::Matrix34f invJointMtx;
    invJointMtx.setInverse(mTopJointMtx);
    sead::Vector3f localDir;
    localDir.setRotated(invJointMtx, worldDir);

    f32 rate = mJointRotateRate;

    if (isExistClimbPlayer()) {
        rate = mJointRotateRateAtClimbTree;
    }

    setJointRotate(localDir.y * rate, -(localDir.z * rate));

    if (al::isGreaterEqualStep(this, 2)) {
        al::setNerve(this, &NrvTreeRelease);
    }
}

/**
 * @brief Lets the bent tree swing back to rest.
 */
void Tree::exeRelease() {
    mRotateZ *= 0.95f;
    mRotateY *= 0.95f;

    f32 rotateY = mRotateY * std::cos(al::getNerveStep(this) * 0.25f);
    mJointRotateY[3] = rotateY;
    mJointRotateY[2] = rotateY;
    mJointRotateY[1] = rotateY;
    mJointRotateY[0] = rotateY;

    f32 rotateZ = mRotateZ * std::cos(al::getNerveStep(this) * 0.25f);
    mJointRotateZ[3] = rotateZ;
    mJointRotateZ[2] = rotateZ;
    mJointRotateZ[1] = rotateZ;
    mJointRotateZ[0] = rotateZ;

    if (al::isGreaterEqualStep(this, 180)) {
        al::setNerve(this, &NrvTreeWait);
    }
}

/**
 * @brief Gets blown away by a giant player, then waits for a respawn or dies.
 */
void Tree::exeGiantBlow() {
    if (al::isFirstStep(this)) {
        for (s32 i = 0; i < mPuppeteerGroup->getPuppeteerNum(); i++) {
            getPuppeteer(i)->bindEnd();
        }

        al::hideModelIfShow(this);
        al::invalidateCollisionParts(this);

        if (mSinkedItem != nullptr) {
            mSinkedItem->appear();
        }
    }

    if (al::updateNerveState(this)) {
        if (mIsSpecialReappear) {
            al::setNerve(this, &NrvTreeAwaitRespawn);
        } else {
            kill();
        }
    }
}

/**
 * @brief Waits until the tree is respawned when it gets out of sight.
 */
void Tree::exeAwaitRespawn() {}

/**
 * @brief Destroys the tree.
 */
Tree::~Tree() {}
