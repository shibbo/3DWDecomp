#include "MapObj/DisasterSpike.hpp"

#include <attributes.h>
#include <cmath>
#include <gfx/seadCamera.h>

#include "Enemy/SuperBowser.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/ShadowKeeper.hpp"
#include "Library/Shadow/ShadowMaskBase.hpp"
#include "MapObj/BlockBrick.hpp"
#include "MapObj/BlockEmpty.hpp"
#include "MapObj/BlockQuestion.hpp"
#include "MapObj/BlockStateCoinTen.hpp"
#include "MapObj/BlockStateItem.hpp"
#include "MapObj/BoxCoin.hpp"
#include "MapObj/CoinBlow.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "MapObj/Fury/CoinBlowConcentric.hpp"
#include "MapObj/Fury/DisasterSpikeDirector.hpp"
#include "MapObj/Fury/DisasterSpikeDirt.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/AreaObj/AreaShape.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Clipping/ClippingAreaActorInfo.hpp"
#include "Project/Collision/CollisionParts.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/OceanWave/OceanWaveUserInfo.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/ProjectActorFactory.hpp"
#include "Scene/SceneObjID.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/AreaObjUtil.hpp"
#include "Util/DemoUtil.hpp"

namespace {
NERVE_ACTION_IMPL(DisasterSpike, Deactivate)
NERVE_ACTION_IMPL(DisasterSpike, ShadowFadeInDelay)
NERVE_ACTION_IMPL(DisasterSpike, ShadowFadeIn)
NERVE_ACTION_IMPL(DisasterSpike, MoveDelay)
NERVE_ACTION_IMPL(DisasterSpike, Move)
NERVE_ACTION_IMPL(DisasterSpike, Land)
NERVE_ACTION_IMPL(DisasterSpike, LandWater)
NERVE_ACTION_IMPL(DisasterSpike, LandWait)
NERVE_ACTION_IMPL(DisasterSpike, LandOcean)
NERVE_ACTION_IMPL(DisasterSpike, ShakeDelay)
NERVE_ACTION_IMPL(DisasterSpike, Shake)
NERVE_ACTION_IMPL(DisasterSpike, ShakeWait)
NERVE_ACTION_IMPL(DisasterSpike, Crumble)
NERVE_ACTION_IMPL(DisasterSpike, CrumbleLaser)
NERVE_ACTION_IMPL(DisasterSpike, LaserDeath)
NERVE_ACTION_IMPL(DisasterSpike, SinkDelay)
NERVE_ACTION_IMPL(DisasterSpike, Sink)
NERVE_ACTION_IMPL(DisasterSpike, ClipInAfterDemo)

NERVE_ACTIONS_MAKE_STRUCT(DisasterSpike, Deactivate, ShadowFadeInDelay, ShadowFadeIn, MoveDelay,
                          Move, Land, LandWater, LandWait, LandOcean, ShakeDelay, Shake, ShakeWait,
                          Crumble, CrumbleLaser, LaserDeath, SinkDelay, Sink, ClipInAfterDemo)

/// Object name of the brick blocks a spike can carry.
const char* const cBlockBrickName = "レンガブロック★";
/// Object name of the question blocks a spike can carry.
const char* const cBlockQuestionName = "ハテナブロック";
/// Object name of the coins a spike can carry.
const char* const cCoinName = "コイン";

/// Cosine of the half angle (40 degrees) of the cone the camera is considered to see.
constexpr f32 cCameraViewCos = 0.76604444f;
/// Steepest ground slope (in degrees) a landed spike aligns to.
constexpr f32 cMaxGroundSlopeDegree = 40.0f;

/**
 * @brief Copy a quaternion as a plain copy of its storage.
 * @param pDst Destination quaternion.
 * @param rSrc Source quaternion.
 */
ALWAYS_INLINE void copyQuat(sead::Quatf* pDst, const sead::Quatf& rSrc) {
    static_cast<sead::BaseQuat<f32>&>(*pDst) = rSrc;
}

/**
 * @brief Get the box coin held by a block's item state.
 * @param pItem The block item state.
 * @return The box coin, or nullptr if the block holds none.
 */
ALWAYS_INLINE BoxCoin* tryGetBoxCoin(const BlockStateItem* pItem) {
    BlockStateCoinTen* coinTen = pItem->tryGetBlockStateCoinTen();
    if (coinTen == nullptr) {
        return nullptr;
    }

    return coinTen->getBoxCoin();
}

/**
 * @brief Check whether a block's box coin is alive and carried by the player.
 * @param pItem The block item state, may be nullptr.
 * @return True if the box coin is being carried.
 */
ALWAYS_INLINE bool isBoxCoinCarried(const BlockStateItem* pItem) {
    if (pItem == nullptr) {
        return false;
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    return boxCoin != nullptr && al::isAlive(boxCoin) && boxCoin->isCarried();
}

/**
 * @brief Hide the empty block or box coin held by a block.
 * @param pItem The block item state, may be nullptr.
 */
ALWAYS_INLINE void hideBlockItem(const BlockStateItem* pItem) {
    if (pItem == nullptr) {
        return;
    }

    BlockEmpty* blockEmpty = pItem->tryGetBlockEmpty();
    if (blockEmpty != nullptr && al::isAlive(blockEmpty)) {
        al::hideModelIfShow(blockEmpty);
        return;
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    if (boxCoin != nullptr && al::isAlive(boxCoin) && !boxCoin->isCarried()) {
        al::hideModelIfShow(boxCoin);
    }
}

/**
 * @brief Show the empty block or box coin held by a block.
 * @param pItem The block item state, may be nullptr.
 */
ALWAYS_INLINE void showBlockItem(const BlockStateItem* pItem) {
    if (pItem == nullptr) {
        return;
    }

    BlockEmpty* blockEmpty = pItem->tryGetBlockEmpty();
    if (blockEmpty != nullptr) {
        al::showModelIfHide(blockEmpty);
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    if (boxCoin != nullptr && !boxCoin->isCarried()) {
        al::showModelIfHide(boxCoin);
    }
}

/**
 * @brief Invalidate the clipping of the empty block and box coin held by a block.
 * @param pItem The block item state, may be nullptr.
 */
ALWAYS_INLINE void invalidateBlockItemClipping(const BlockStateItem* pItem) {
    if (pItem == nullptr) {
        return;
    }

    BlockEmpty* blockEmpty = pItem->tryGetBlockEmpty();
    if (blockEmpty != nullptr && al::isAlive(blockEmpty)) {
        al::invalidateClipping(blockEmpty);
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    if (boxCoin != nullptr && al::isAlive(boxCoin) && !boxCoin->isCarried()) {
        al::invalidateClipping(boxCoin);
    }
}

/**
 * @brief Validate the clipping of the empty block and box coin held by a block.
 * @param pItem The block item state, may be nullptr.
 */
ALWAYS_INLINE void validateBlockItemClipping(const BlockStateItem* pItem) {
    if (pItem == nullptr) {
        return;
    }

    BlockEmpty* blockEmpty = pItem->tryGetBlockEmpty();
    if (blockEmpty != nullptr) {
        al::validateClipping(blockEmpty);
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    if (boxCoin != nullptr && !boxCoin->isCarried()) {
        al::validateClipping(boxCoin);
    }
}

/**
 * @brief Kill the empty block or make the box coin held by a block disappear.
 * @param pItem The block item state, may be nullptr.
 * @param isBreak Whether the empty block breaks and the box coin disappears with an effect.
 */
ALWAYS_INLINE void killBlockItem(const BlockStateItem* pItem, bool isBreak) {
    if (pItem == nullptr) {
        return;
    }

    BlockEmpty* blockEmpty = pItem->tryGetBlockEmpty();
    if (blockEmpty != nullptr && al::isAlive(blockEmpty)) {
        if (isBreak) {
            al::startHitReactionBreak(blockEmpty);
        }

        blockEmpty->kill();
        return;
    }

    BoxCoin* boxCoin = tryGetBoxCoin(pItem);
    if (boxCoin != nullptr && al::isAlive(boxCoin) && !boxCoin->isCarried()) {
        boxCoin->disappear(isBreak);
    }
}

/**
 * @brief Show the spike length matching how deep a short spike is stuck in its dirt.
 * @param pSpike The landed spike.
 * @param rDirt The dirt around the spike's base.
 * @return True if a length animation was started.
 */
ALWAYS_INLINE bool tryStartSpikeLengthVisAnim(al::LiveActor* pSpike,
                                              DisasterSpikeDirt* const& rDirt) {
    f32 visibleLength = al::getScale(pSpike).y * 600.0f -
                        (al::getTrans(rDirt) - al::getTrans(pSpike)).length();
    if (visibleLength <= 250.5f && al::tryStartVisAnimIfExist(pSpike, "DisasterSpike2_5M")) {
        return true;
    }

    return visibleLength <= 275.5f && al::tryStartVisAnimIfExist(pSpike, "DisasterSpike2_75M");
}
}  // namespace

/**
 * @brief Construct a disaster spike.
 * @param pName Name of the actor.
 */
DisasterSpike::DisasterSpike(const char* pName) : al::LiveActor(pName) {}

/**
 * @brief Initialize the spike model, its MoveNext chain, carried children, no-fall area, coins,
 * dirt and hot collision.
 * @param rInfo Actor init info.
 */
void DisasterSpike::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Deactivate", &NrvDisasterSpike.collector, 0);
    const char* modelName = nullptr;
    const char* objectName = nullptr;

    if (mIsOcean) {
        al::initActorWithArchiveName(this, rInfo, "DisasterSpikeOcean", nullptr);
    } else if (al::tryGetObjectName(&objectName, rInfo) &&
               al::isEqualString(objectName, "DisasterSpikeBouncy")) {
        al::initActorWithArchiveName(this, rInfo, "DisasterSpikeBouncy", nullptr);
    } else if (al::isEqualString(getName(), "DisasterSpikeGold")) {
        al::initActorWithArchiveName(this, rInfo, "DisasterSpikeGold", nullptr);
        mIsGold = true;
    } else if (alPlacementFunction::tryGetModelName(&modelName, rInfo)) {
        if (al::isEqualString(modelName, "DisasterSpikeCornerCut")) {
            al::initActorWithArchiveName(this, rInfo, "DisasterSpikeCornerCut", nullptr);
        } else if (al::isEqualString(modelName, "DisasterSpikeTall")) {
            al::initActorWithArchiveName(this, rInfo, "DisasterSpike9m", nullptr);
        } else if (al::isEqualString(modelName, "DisasterSpike12m")) {
            al::initActorWithArchiveName(this, rInfo, "DisasterSpike12m", nullptr);
        } else if (al::isEqualString(modelName, "DisasterSpikeGold")) {
            al::initActorWithArchiveName(this, rInfo, "DisasterSpikeGold", nullptr);
            mIsGold = true;
        } else {
            al::initActorWithArchiveName(this, rInfo, "DisasterSpike", nullptr);
            mCanBeGold = true;
        }
    } else {
        al::initActorWithArchiveName(this, rInfo, "DisasterSpike", nullptr);
        mCanBeGold = true;
    }

    al::tryGetArg(&mShadowFadeInDelay, rInfo, "ShadowFadeInDelay");
    al::tryGetArg(&mMoveTime, rInfo, "MoveTime");
    al::tryGetArg(&mInterpolateType, rInfo, "InterpolateType");
    al::tryGetArg(&mLandHeight, rInfo, "LandHeight");
    al::tryGetArg(&mIsForeverSpike, rInfo, "ForeverSpike");
    al::tryGetArg(&mIsHideTip, rInfo, "HideTip");
    al::tryGetArg(&mHideTipStepFrames, rInfo, "HideTipStepFrames");
    al::tryGetArg(&mIsShortSpike, rInfo, "ShortSpike");
    al::tryGetArg(&mIsHideDirtBottom, rInfo, "HideDirtBottom");
    al::tryGetArg(&mIsAlwaysGold, rInfo, "AlwaysGold");
    al::tryGetArg(&mIsNeverGold, rInfo, "NeverGold");
    mPosition.e = al::getTrans(this).e;
    copyQuat(&mQuat, al::getQuat(this));

    if (al::calcLinkChildNum(rInfo, "MoveNext") != 0) {
        mMoveNext = new DisasterSpike("DisasterSpike");
        al::initLinksActor(mMoveNext, rInfo, "MoveNext", 0);
        mMoveNext->setActive(false, false);
        mMoveNext->makeActorDead();
    }

    ProjectActorFactory factory;
    s32 childNum = al::calcLinkChildNum(rInfo, "Child");
    if (childNum >= 1) {
        mChildren = new al::LiveActor*[childNum];
        mChildOffsets = new sead::Vector3f[childNum];

        for (s32 i = 0; i < childNum; i++) {
            al::ActorInitInfo childInfo;
            al::PlacementInfo placementInfo;
            al::getLinksInfoByIndex(&placementInfo, rInfo, "Child", i);
            childInfo.initViewIdHost(&placementInfo, rInfo);
            al::tryGetLayerID(childInfo);
            if (SingleModeDataFunction::isActorInPlessieChaseDisabled(childInfo)) {
                continue;
            }

            mChildren[mChildCount] = al::createLinksActorFromFactory(factory, rInfo, "Child", i);
            mChildren[mChildCount]->makeActorAppeared();
            mChildOffsets[mChildCount] =
                al::getTrans(mChildren[mChildCount]) - al::getTrans(this);
            mChildCount++;
        }
    }

    if (al::calcLinkChildNum(rInfo, "NoFallArea") != 0) {
        al::ActorInitInfo areaInfo;
        al::PlacementInfo placementInfo;
        al::getLinksInfoByIndex(&placementInfo, rInfo, "NoFallArea", 0);
        areaInfo.initViewIdSelf(&placementInfo, rInfo);
        mNoFallArea = al::createAreaObj(areaInfo, "NoFallArea");
        mNoFallArea->invalidate();

        sead::Matrix34f areaMtx = mNoFallArea->_28;
        sead::Vector3f size = mNoFallArea->getAreaShape()->mScale * 1000.0f;
        sead::Vector3f min = size * -0.5f;
        sead::Vector3f max = size * 0.5f;
        mNoFallAreaCorners[0].set(min.x, min.y, min.z);
        mNoFallAreaCorners[1].set(max.x, min.y, min.z);
        mNoFallAreaCorners[2].set(min.x, max.y, min.z);
        mNoFallAreaCorners[3].set(max.x, max.y, min.z);
        mNoFallAreaCorners[4].set(min.x, min.y, max.z);
        mNoFallAreaCorners[5].set(max.x, min.y, max.z);
        mNoFallAreaCorners[6].set(min.x, max.y, max.z);
        mNoFallAreaCorners[7].set(max.x, max.y, max.z);

        for (s32 i = 0; i < 8; i++) {
            sead::Quatf quat = sead::Quatf::unit;
            areaMtx.toQuat(quat);
            al::rotateVectorQuat(&mNoFallAreaCorners[i], quat);
            mNoFallAreaCorners[i] += areaMtx.getTranslation();
        }
    }

    if (!mIsOcean) {
        if (mIsGold) {
            auto* coinBlowConcentric = new CoinBlowConcentric("CoinBlowConcentric");
            al::initCreateActorNoPlacementInfo(coinBlowConcentric, rInfo);
            mCoinBlowConcentric = coinBlowConcentric;
        } else {
            mCoinBlow = new CoinBlow("CoinBlow");
            mCoinBlow->init(rInfo);
            mCoinBlow->setOffSensor(14);
        }

        sead::Vector3f up = sead::Vector3f::ez;
        al::calcUpDir(&up, this);
        bool isHorizontal = sead::Mathf::abs(up.y) < 0.1f;
        mDirt = new DisasterSpikeDirt("DisasterSpikeDirt");
        mDirt->init(rInfo, mIsGold, isHorizontal);
        mDirt->setGlobalAlphaPtr(&mGlobalAlphaLastFrame);
    }

    al::initSubActorKeeperNoFile(this, rInfo, 1);
    mHotCollision = al::createCollisionObj(this, rInfo, "DisasterSpikeHot",
                                           al::getHitSensor(this, "Collision"), nullptr, nullptr);
    al::registerSubActorSyncClipping(this, mHotCollision, false);
    al::setSubActorOnSyncAppear(this);
    mNpcAvoidRadius = al::getSensorRadius(this, "NPCDisasterAvoid");
    al::setHitSensorPosPtr(this, "NPCDisasterAvoid", &mPosition);
    al::setSensorRadius(this, "NPCDisasterAvoid", mNpcAvoidRadius * al::getScaleX(this));
    al::invalidateHitSensor(this, "NPCDisasterAvoid");

    al::Effect* hitCollisionEffect = getEffectKeeper()->findEffect("HitCollision");
    al::Effect* waterImpactEffect = getEffectKeeper()->findEffect("WaterImpact");
    al::Effect* waterImpactOceanEffect = getEffectKeeper()->findEffect("WaterImpactOcean");
    hitCollisionEffect->setMtxPtr(&mEffectMtx);
    waterImpactEffect->setMtxPtr(&mEffectMtx);
    waterImpactOceanEffect->setMtxPtr(&mEffectMtx);
    makeActorDead();
    setHot(false);

    sead::Vector3f maskSize = sead::Vector3f::ones;
    al::calcShadowMaskSize(&maskSize, this, "Body");
    mShadowMaskSize = maskSize.x;
    updateHotCollisionMtx();
    mHotCollision->getCollisionParts()->setSyncCollisionMtx(&mHotCollisionMtx);
    mHotCollision->getCollisionParts()->syncMtx();
    mHotCollision->getCollisionParts()->updateMtx();
    al::invalidateHitSensor(this, "EnemyDamage");
    al::invalidateHitSensor(this, "PlayerPush");
    if (!mIsOcean) {
        al::invalidateHitSensor(this, "EnemyDamageLanded");
    }

    setActive(false, false);
}

/**
 * @brief Show or hide the spike, its collision and its children.
 * @param isActive Whether the spike becomes active.
 * @param isCrumble When deactivating, whether the children crumble instead of being killed.
 */
void DisasterSpike::setActive(bool isActive, bool isCrumble) {
    if (mIsActive == isActive) {
        return;
    }

    mIsActive = isActive;
    if (isActive) {
        al::showModelIfHide(this);
        getCollisionParts()->validateBySystem();
        appearChildren();
        showChildren();
        return;
    }

    al::hideModelIfShow(this);
    getCollisionParts()->invalidateBySystem();
    al::invalidateCollisionParts(mHotCollision);
    if (isCrumble) {
        crumbleChildren();
    } else {
        killChildren();
    }
}

/**
 * @brief Switch between the normal collision and the hot (burning) collision.
 * @param isHot Whether the hot collision is used.
 */
void DisasterSpike::setHot(bool isHot) {
    if (isHot) {
        al::invalidateCollisionParts(this);
        al::validateCollisionParts(mHotCollision);
    } else {
        al::invalidateCollisionParts(mHotCollision);
        al::validateCollisionParts(this);
    }
}

/**
 * @brief Rebuild the matrix the hot collision follows from the spike's pose and scale.
 */
void DisasterSpike::updateHotCollisionMtx() {
    mHotCollisionMtx.makeQT(al::getQuat(this), sead::Vector3f(0.0f, 0.0f, 0.0f));
    mHotCollisionMtx.setTranslation(al::getTrans(this));
    sead::Vector3f scale = al::getScaleX(this) * sead::Vector3f::ones;
    mHotCollisionMtx.scaleBases(scale.x, scale.y, scale.z);
}

/**
 * @brief Nothing to do after placement.
 */
void DisasterSpike::initAfterPlacement() {}

/**
 * @brief Appear at the top of the fall and start the shadow fade in.
 */
void DisasterSpike::appear() {
    if (mPlayer == nullptr) {
        mPlayer = al::tryFindNearestPlayerActor(this);
    }

    mIsKillOutOfView = false;
    mIsLaserDead = false;
    mIsTriggered = true;
    mHasLanded = false;
    mIsEchoBlock = false;
    mAlphaFadeInStep = 0;
    mRotateOffsetDegree = al::getRandom(0, 4) * 90;
    appearChildren();
    hideChildren();
    mHotCollision->appear();
    al::invalidateCollisionParts(mHotCollision);
    al::LiveActor::appear();
    invalidateClipping();
    mIsFadingOut = false;
    mFadeOutAlpha = 1.0f;
    start();

    if (al::tryStartMclAnimIfExist(this, "DisasterSpikeOff")) {
        al::setMclAnimFrameAndStopEnd(this);
    }

    if (mDirt != nullptr) {
        mDirt->tryStopGlow();
    }

    if (!al::tryStartVisAnimIfExist(this, "DisasterSpike")) {
        if (al::tryStartVisAnimIfExist(this, "DisasterSpikeTipHide")) {
            al::setVisAnimFrameAndStop(this, 1.0f);
        }

        if (al::tryStartVisAnimIfExist(this, "DisasterSpikeShort")) {
            al::setVisAnimFrameAndStop(this, 1.0f);
        }
    }

    al::invalidateHitSensor(this, "EnemyDamage");
    al::invalidateHitSensor(this, "PlayerPush");
    if (!mIsOcean) {
        al::invalidateHitSensor(this, "EnemyDamageLanded");
    }

    if (mCoinBlowConcentric != nullptr && al::isAlive(mCoinBlowConcentric)) {
        mCoinBlowConcentric->kill();
    }
}

/**
 * @brief Revive the dead children (keeping box coins the player carries away).
 */
void DisasterSpike::appearChildren() {
    for (s32 i = 0; i < getChildCount(); i++) {
        if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            auto* brick = static_cast<BlockBrick*>(getChild(i));
            if (isBoxCoinCarried(brick->getBlockStateItem())) {
                continue;
            }

            if (al::isDead(getChild(i))) {
                getChild(i)->reappear();
            }
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            auto* question = static_cast<BlockQuestion*>(getChild(i));
            if (isBoxCoinCarried(question->getBlockStateItem())) {
                continue;
            }

            if (al::isDead(question)) {
                question->respawn();
            }
        } else if (al::isDead(getChild(i))) {
            getChild(i)->appear();
        }
    }
}

/**
 * @brief Hide the models of the children and of the objects their blocks hold.
 */
void DisasterSpike::hideChildren() {
    for (s32 i = 0; i < getChildCount(); i++) {
        if (al::isAlive(getChild(i)) && al::isExistModel(getChild(i))) {
            al::hideModelIfShow(getChild(i));
        } else if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            hideBlockItem(static_cast<BlockBrick*>(getChild(i))->getBlockStateItem());
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            hideBlockItem(static_cast<BlockQuestion*>(getChild(i))->getBlockStateItem());
        }
    }
}

/**
 * @brief Invalidate the clipping of the spike, its children and the objects their blocks hold.
 */
void DisasterSpike::invalidateClipping() {
    al::invalidateClipping(this);

    for (s32 i = 0; i < getChildCount(); i++) {
        if (al::isAlive(getChild(i))) {
            al::invalidateClipping(getChild(i));
        }

        if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            invalidateBlockItemClipping(
                static_cast<BlockBrick*>(getChild(i))->getBlockStateItem());
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            invalidateBlockItemClipping(
                static_cast<BlockQuestion*>(getChild(i))->getBlockStateItem());
        }
    }
}

/**
 * @brief Move the spike to the start of its fall and wait for the shadow fade in.
 */
void DisasterSpike::start() {
    if (mIsActive) {
        return;
    }

    mCurrentMove = this;
    resetTransform();
    updateShadowDirection();
    al::invalidateHitSensor(this, "NPCDisasterAvoid");
    startNerveAction("ShadowFadeInDelay");
}

/**
 * @brief Kill the spike, its hot collision, dirt and the gold spike replacing it.
 */
void DisasterSpike::kill() {
    al::LiveActor::kill();
    setActive(false, false);
    mIsShadowFading = false;
    startNerveAction("Deactivate");
    mHotCollision->kill();
    al::tryDeleteEffectAndParticle(this, "Falling");
    if (mDirt != nullptr) {
        mDirt->kill();
    }

    if (isReplacedByGold() && mGoldSpike != nullptr) {
        mGoldSpike->kill();
    }
}

/**
 * @brief Start a nerve action and reset the spike's own step counter.
 * @param pActionName Name of the nerve action.
 */
void DisasterSpike::startNerveAction(const char* pActionName) {
    mStep = 0;
    al::startNerveAction(this, pActionName);
}

/**
 * @brief Check whether a gold spike falls in place of this spike.
 * @return True if the spike was picked for or always uses a gold spike.
 */
bool DisasterSpike::isReplacedByGold() const {
    return mIsReplacedByGold || mIsAlwaysGold;
}

/**
 * @brief Try to start the fall of the spike (or of the gold spike replacing it).
 * @param isForce Whether to skip catching up with the current all fall time.
 * @return True if the spike is falling.
 */
bool DisasterSpike::tryAppear(bool isForce) {
    if (isReplacedByGold()) {
        if (mGoldSpike != nullptr) {
            return mGoldSpike->tryAppear(isForce);
        }

        DisasterSpike* goldSpike = mDisasterSpikeDirector->tryGetGoldSpike();
        if (goldSpike == nullptr) {
            return false;
        }

        goldSpike->mShadowFadeInDelay = mShadowFadeInDelay;
        goldSpike->mMoveTime = mMoveTime;
        goldSpike->mInterpolateType = mInterpolateType;
        goldSpike->mLandHeight = mLandHeight;
        goldSpike->mIsForeverSpike = mIsForeverSpike;
        goldSpike->mIsHideTip = mIsHideTip;
        goldSpike->mHideTipStepFrames = mHideTipStepFrames;
        goldSpike->mIsShortSpike = mIsShortSpike;
        goldSpike->mIsHideDirtBottom = mIsHideDirtBottom;
        goldSpike->mPosition = mPosition;
        goldSpike->mQuat = mQuat;
        goldSpike->mIsTriggered = mIsTriggered;
        al::setScale(goldSpike, al::getScale(this));
        al::setScale(goldSpike->mDirt, al::getScale(this));
        al::setTrans(goldSpike, al::getTrans(this));
        al::setQuat(goldSpike, al::getQuat(this));
        goldSpike->mDirt->setUseHorizontalCollision(mDirt->getUseHorizontalCollision());
        al::setHitSensorPosPtr(goldSpike, "NPCDisasterAvoid", &mPosition);
        al::setSensorRadius(goldSpike, "NPCDisasterAvoid", mNpcAvoidRadius * al::getScaleX(this));
        al::moveActorToClippingGroup(this, goldSpike);
        al::moveActorToClippingGroup(this, goldSpike->mDirt);
        mGoldSpike = goldSpike;
        goldSpike->mOriginalSpike = this;
        if (!goldSpike->tryAppear(isForce)) {
            return false;
        }
    } else {
        if (mPlayer == nullptr) {
            mPlayer = al::tryFindNearestPlayerActor(this);
        }

        if (!canFall()) {
            return false;
        }

        if (al::isNerve(this, NrvDisasterSpike.ShadowFadeInDelay.data()) ||
            al::isNerve(this, NrvDisasterSpike.ShadowFadeIn.data()) ||
            al::isNerve(this, NrvDisasterSpike.MoveDelay.data()) ||
            al::isNerve(this, NrvDisasterSpike.Move.data()) ||
            al::isNerve(this, NrvDisasterSpike.Land.data()) ||
            al::isNerve(this, NrvDisasterSpike.LandWait.data()) ||
            al::isNerve(this, NrvDisasterSpike.LandOcean.data())) {
            return true;
        }

        if (mIsActive) {
            return false;
        }

        appear();
        if (mIsOcean || isForce) {
            return true;
        }

        if (mDisasterSpikeDirector->isAllFall()) {
            clipInAtAllFallTime();
        }
    }

    return true;
}

/**
 * @brief Check whether the spike may fall: outside the no-fall radius and with its no-fall area
 * out of view.
 * @return True if the spike can fall.
 */
bool DisasterSpike::canFall() {
    if (isWithinSpikeNoFallRadius()) {
        return false;
    }

    if (mNoFallArea == nullptr) {
        return true;
    }

    for (s32 i = 0; i < 8; i++) {
        if (isPositionInCameraView(mNoFallAreaCorners[i])) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Catch up with the time elapsed since all spikes started falling.
 */
void DisasterSpike::clipInAtAllFallTime() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        startNerveAction("ClipInAfterDemo");
        return;
    }

    s32 timer = mDisasterSpikeDirector->getAllFallTimer();
    if (timer < mShadowFadeInDelay) {
        startNerveAction("ShadowFadeInDelay");
        mStep = timer;
        return;
    }

    s32 shadowStep = timer - mShadowFadeInDelay;

    al::showShadow(this);
    mShadowFadeStep = shadowStep;
    mIsShadowFading = shadowStep > mDisasterSpikeDirector->getParam().mSpikeShadowFadeTime;
    if (!mIsShadowFading) {
        fadeShadow(1.0f);
    }

    setShadowDropLengthPercent(1.0f);
    al::tryStartMclAnimIfExist(this, "DisasterSpikeOn");
    setActive(true, false);
    setHot(false);

    if (shadowStep < mDisasterSpikeDirector->getParam().mSpikeMoveDelayTime) {
        startNerveAction("MoveDelay");
        mStep = shadowStep;
        return;
    }

    s32 moveStep = shadowStep - mDisasterSpikeDirector->getParam().mSpikeMoveDelayTime;
    s32 moveTime = 0;
    for (DisasterSpike* spike = this; spike != nullptr; spike = spike->mMoveNext) {
        if (spike->mMoveNext == nullptr && mMoveNext != nullptr) {
            break;
        }

        s32 nextMoveTime =
            moveTime + (s32)(mDisasterSpikeDirector->getParam().mSpikeMoveTimeGlobalScale *
                             (f32)spike->mMoveTime);
        if (moveStep < nextMoveTime) {
            mCurrentMove = spike;
            startNerveAction("Move");
            mStep = moveStep - moveTime;
            return;
        }

        moveTime = nextMoveTime;
    }

    setHot(false);
    setTransform(getEndPosition(), getLastMove()->getStartQuatWithOffset(mRotateOffsetDegree));

    if (al::tryStartMclAnimIfExist(this, "DisasterSpikeOff")) {
        al::setMclAnimFrameAndStopEnd(this);
    }

    if (mDirt != nullptr) {
        mDirt->tryStopGlow();
    }

    setShadowDropLengthPercent(1.0f);
    startNerveAction("LandWait");
}

/**
 * @brief Set the director driving the spike and its MoveNext chain.
 * @param pDirector The disaster spike director.
 */
void DisasterSpike::setDisasterSpikeDirector(DisasterSpikeDirector* pDirector) {
    mDisasterSpikeDirector = pDirector;
    if (mMoveNext != nullptr) {
        mMoveNext->setDisasterSpikeDirector(pDirector);
    }
}

/**
 * @brief Mark the spike as one of the director's ocean spikes.
 */
void DisasterSpike::setOceanSpike() {
    mIsOcean = true;
}

/**
 * @brief Damage enemies and objects hit by the spike and push the player away.
 * @param pSender Sensor of this actor.
 * @param pReceiver Sensor that was hit.
 */
void DisasterSpike::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isSensorName(pSender, "EnemyDamage")) {
        if (al::isSensorEnemy(pReceiver) || al::isSensorKoopaJr(pReceiver) ||
            al::isSensorNpc(pReceiver) || al::isSensorKickKoura(pReceiver) ||
            al::isSensorMapObj(pReceiver)) {
            al::sendMsgDisasterSpikeAttack(pReceiver, pSender);
        }
    } else if (!mIsOcean && al::isSensorName(pSender, "EnemyDamageLanded") &&
               al::isSensorEnemy(pReceiver) && al::isSensorHostName(pReceiver, "ファイアパックン")) {
        al::getSensorHost(pReceiver)->kill();
    }

    if (al::isSensorName(pSender, "PlayerPush")) {
        if (al::isSensorKickKoura(pReceiver)) {
            al::sendMsgKickKouraReflect(pReceiver, pSender);
        } else if (al::isSensorRide(pReceiver)) {
            al::sendMsgDisasterSpikePush(pReceiver, pSender);
        } else if (al::isSensorPlayer(pReceiver)) {
            al::sendMsgPushVeryStrong(pReceiver, pSender);
        }
    }
}

/**
 * @brief Explode when hit by a laser and crumble an ocean spike pushed by Fury Bowser.
 * @param pMsg Received message.
 * @param pSender Sensor that sent the message.
 * @param pReceiver Sensor of this actor that received the message.
 * @return Whether the message was handled.
 */
bool DisasterSpike::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                               al::HitSensor* pReceiver) {
    if (al::isSensorName(pReceiver, "Collision") && al::isMsgLaserAttack(pMsg) && canCrumble()) {
        dieByLaser();
        DisasterSpike* top = mDisasterSpikeDirector->tryGetSpikeTop(this);
        if (top != nullptr) {
            top->dieByLaser();
        }

        return true;
    }

    if (mIsOcean &&
        (al::isSensorName(pReceiver, "BodySensorBottom") ||
         al::isSensorName(pReceiver, "BodySensorTop")) &&
        al::isMsgBowserPush(pMsg)) {
        if (canCrumbleOceanSpike()) {
            startNerveAction("Crumble");
            return true;
        }

        if (al::isNerve(this, NrvDisasterSpike.MoveDelay.data()) ||
            al::isNerve(this, NrvDisasterSpike.ShadowFadeInDelay.data()) ||
            al::isNerve(this, NrvDisasterSpike.ShadowFadeIn.data())) {
            startNerveAction("Deactivate");
            return true;
        }
    }

    return false;
}

/**
 * @brief Check whether a laser can destroy the spike.
 * @return True once the spike has landed.
 */
bool DisasterSpike::canCrumble() {
    return al::isNerve(this, NrvDisasterSpike.Land.data()) ||
           al::isNerve(this, NrvDisasterSpike.LandWait.data()) ||
           al::isNerve(this, NrvDisasterSpike.LandOcean.data()) ||
           al::isNerve(this, NrvDisasterSpike.SinkDelay.data()) ||
           al::isNerve(this, NrvDisasterSpike.Sink.data()) ||
           al::isNerve(this, NrvDisasterSpike.LaserDeath.data()) ||
           al::isNerve(this, NrvDisasterSpike.Shake.data());
}

/**
 * @brief Start the explosion of a spike destroyed by a laser.
 */
void DisasterSpike::dieByLaser() {
    if (!mIsLaserDead) {
        al::tryStartMclAnimIfExist(this, "DisasterSpikeExplode");
        al::tryStartMtpAnimIfExist(this, "DisasterSpikeExplode");
        al::startSe(this, "Glow");
        if (mDirt != nullptr) {
            mDirt->tryStartExplodeAnim();
        }
    }

    mIsLaserDead = true;
    startNerveAction("LaserDeath");
}

/**
 * @brief Check whether Fury Bowser can crumble the ocean spike.
 * @return True once the spike started moving.
 */
bool DisasterSpike::canCrumbleOceanSpike() {
    return al::isNerve(this, NrvDisasterSpike.Move.data()) ||
           al::isNerve(this, NrvDisasterSpike.Land.data()) ||
           al::isNerve(this, NrvDisasterSpike.LandWait.data()) ||
           al::isNerve(this, NrvDisasterSpike.LandOcean.data()) ||
           al::isNerve(this, NrvDisasterSpike.SinkDelay.data()) ||
           al::isNerve(this, NrvDisasterSpike.Sink.data()) ||
           al::isNerve(this, NrvDisasterSpike.LaserDeath.data()) ||
           al::isNerve(this, NrvDisasterSpike.Shake.data());
}

/**
 * @brief Fade the shadow and the model in, fade landed spikes out when far away and kill spikes
 * that left the view.
 */
void DisasterSpike::control() {
    if (mIsShadowFading) {
        f32 rate = 1.0f;
        if (mDisasterSpikeDirector->getParam().mSpikeShadowFadeTime != 0) {
            rate = sead::Mathf::clamp(
                (f32)mShadowFadeStep++ /
                    (f32)mDisasterSpikeDirector->getParam().mSpikeShadowFadeTime,
                0.0f, 1.0f);
        }

        fadeShadow(rate);
        if (rate >= 1.0f) {
            mIsShadowFading = false;
        }
    }

    if (mIsActive && al::isInvalidClipping(this) && mAlphaFadeInStep <= mAlphaFadeInFrames) {
        mGlobalAlphaLastFrame = (f32)mAlphaFadeInStep / (f32)mAlphaFadeInFrames;
        mAlphaFadeInStep++;
    }

    if (!mIsOcean && getActiveSpike()->mHasLanded && al::isInvalidClipping(this)) {
        if (!isPositionInCameraView(al::getTrans(this))) {
            validateClipping();
        } else if (mIsFadingOut) {
            mFadeOutAlpha -= getClippingInfoNode()->mInfo->getFadeStep();
            if (mFadeOutAlpha <= 0.0f) {
                mFadeOutAlpha = 0.0f;
                if (mIsKillOutOfView) {
                    kill();
                } else {
                    validateClipping();
                }
            }

            mGlobalAlphaLastFrame = mFadeOutAlpha;
        } else if (mPlayer != nullptr) {
            if ((al::getTrans(mPlayer) - al::getTrans(this)).squaredLength() > 15000.0f * 15000.0f ||
                mIsKillOutOfView) {
                mIsFadingOut = true;
            }
        }
    }

    if (mIsKillOutOfView && !isPositionInCameraView(al::getTrans(this))) {
        kill();
    }
}

/**
 * @brief Set the alpha and size of the falling spike's shadow.
 * @param rate Fade rate, from 0 (invisible) to 1 (fully shown).
 */
void DisasterSpike::fadeShadow(f32 rate) {
    al::ShadowMaskBase* shadowMask = getShadowKeeper()->findShadowMask("Body");
    f32 alpha = rate * 255.0f;
    u8 intensity = alpha * mDisasterSpikeDirector->getParam().mSpikeShadowMaxAlpha;
    shadowMask->mIsApplyShadowIntensityUser = true;
    shadowMask->mShadowIntensityUser = intensity;
    al::setShadowMaskSize(this, "Body", sead::Vector3f::ones * (mShadowMaskSize * rate));
}

/**
 * @brief Check whether the spike (or the gold spike replacing it) landed.
 * @return True if the shown spike landed.
 */
bool DisasterSpike::hasLanded() {
    return getActiveSpike()->mHasLanded;
}

/**
 * @brief Check whether a position is close to the player or in front of the camera.
 * @param pos Position to check.
 * @return True if the position is visible.
 */
bool DisasterSpike::isPositionInCameraView(sead::Vector3f pos) {
    bool isVisible = true;
    if (mPlayer == nullptr ||
        !((al::getTrans(mPlayer) - pos).squaredLength() < 2500.0f * 2500.0f)) {
        sead::LookAtCamera camera = getSceneCameraInfo()->getViewAt(0)->getLookAtCam();
        sead::Vector3f cameraDir = camera.getAt() - camera.getPos();
        sead::Vector3f toPos = pos - camera.getPos();
        cameraDir.normalize();
        toPos.normalize();
        isVisible = cameraDir.dot(toPos) > cCameraViewCos;
    }

    return isVisible;
}

/**
 * @brief Validate the clipping of the spike, its children and the objects their blocks hold.
 */
void DisasterSpike::validateClipping() {
    al::validateClipping(this);

    for (s32 i = 0; i < getChildCount(); i++) {
        al::validateClipping(getChild(i));

        if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            validateBlockItemClipping(static_cast<BlockBrick*>(getChild(i))->getBlockStateItem());
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            validateBlockItemClipping(
                static_cast<BlockQuestion*>(getChild(i))->getBlockStateItem());
        }
    }
}

/**
 * @brief Kill a spike marked to die out of view as soon as it is clipped.
 */
void DisasterSpike::startClipped() {
    al::LiveActor::startClipped();
    if (mIsKillOutOfView) {
        kill();
    }
}

/**
 * @brief Hide the spike when the disaster is over, or catch up with the all fall time.
 */
void DisasterSpike::endClipped() {
    al::LiveActor::endClipped();
    if (mDisasterSpikeDirector == nullptr) {
        return;
    }

    mEchoTimer = 0;
    bool isUnusedGoldSpike = false;
    if (mIsGold && mOriginalSpike != nullptr) {
        isUnusedGoldSpike =
            !mOriginalSpike->isReplacedByGold() || mOriginalSpike->mGoldSpike != this;
    }

    if (!isDisaster() || isUnusedGoldSpike || isReplacedByGold()) {
        hide();
        return;
    }

    if (!mIsOcean && mDisasterSpikeDirector->isAllFall()) {
        clipInAtAllFallTime();
        return;
    }

    if (al::tryStartMclAnimIfExist(this, "DisasterSpikeOff")) {
        al::setMclAnimFrameAndStopEnd(this);
    }

    if (mDirt != nullptr) {
        mDirt->tryStopGlow();
    }

    if (mHasLanded) {
        al::hideShadow(this);
    }
}

/**
 * @brief Check whether disaster mode is on.
 * @return True during a disaster.
 */
bool DisasterSpike::isDisaster() const {
    DisasterModeController* controller = DisasterModeController::tryGetController(this);
    return controller != nullptr && controller->isDisasterMode();
}

/**
 * @brief Hide the spike, its children and dirt and deactivate it.
 */
void DisasterSpike::hide() {
    al::hideModelIfShow(this);
    hideChildren();
    mIsShadowFading = false;
    startNerveAction("Deactivate");
    mHotCollision->kill();
    if (mDirt != nullptr) {
        al::hideModelIfShow(mDirt);
    }

    al::tryDeleteEffectAndParticle(this, "Falling");
}

/**
 * @brief Emit an echo pulse every 150 frames while the spike stands on an echo block.
 */
void DisasterSpike::updateEchoPulse() {
    if (!mIsEchoBlock) {
        return;
    }

    if (mEchoTimer == 0) {
        sead::Vector3f pos(mEffectMtx(0, 3), mEffectMtx(1, 3), mEffectMtx(2, 3));
        rc::emitEcho(this, pos, 800.0f, 300, true);
    }

    mEchoTimer = mEchoTimer > 148 ? 0 : mEchoTimer + 1;
}

/**
 * @brief Kill the spike once deactivated.
 */
void DisasterSpike::exeDeactivate() {
    if (al::isFirstStep(this)) {
        kill();
    }
}

/**
 * @brief Wait before fading the shadow in, until the spike can fall.
 */
void DisasterSpike::exeShadowFadeInDelay() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    if (mStep++ > mShadowFadeInDelay) {
        mStep = mShadowFadeInDelay;
        if (canFall()) {
            startNerveAction("ShadowFadeIn");
        }
    }

    if (!isDisaster()) {
        startNerveAction("Deactivate");
    }
}

/**
 * @brief Fade the shadow of the falling spike in.
 */
void DisasterSpike::exeShadowFadeIn() {
    if (!isDisaster()) {
        startNerveAction("Deactivate");
    }

    if (al::isFirstStep(this)) {
        al::showShadow(this);
        mShadowFadeStep = 0;
        mIsShadowFading = true;
        setShadowDropLengthPercent(1.0f);
        getShadowKeeper()->findShadowMask("Body")->mIsIgnoreHostAlpha = true;
    }

    if (al::isGreaterEqualStep(this,
                               mDisasterSpikeDirector->getParam().mSpikeShadowFadeWaitTime)) {
        startNerveAction("MoveDelay");
    }
}

/**
 * @brief Set the drop length of the shadow relative to the fall distance.
 * @param percent Fraction of the fall distance, at least 1.
 */
void DisasterSpike::setShadowDropLengthPercent(f32 percent) {
    percent = fmaxf(percent, 1.0f);
    f32 length = mDisasterSpikeDirector->getParam().mSingleSpikeStartDistance * 1.2f;
    getShadowKeeper()->findShadowMask("Body")->setDropLength(percent * length);
}

/**
 * @brief Show the spike glowing and wait before starting to fall.
 */
void DisasterSpike::exeMoveDelay() {
    if (!isDisaster()) {
        startNerveAction("Deactivate");
    }

    if (mStep == 0) {
        if (al::isMclAnimExist(this, "DisasterSpikeExplode") &&
            al::isMtpAnimExist(this, "DisasterSpikeExplode")) {
            al::startMclAnimAndSetFrameAndStop(this, "DisasterSpikeExplode", 0.0f);
            al::startMtpAnimAndSetFrameAndStop(this, "DisasterSpikeExplode", 0.0f);
            if (mDirt != nullptr) {
                mDirt->tryResetExplodeAnim();
            }
        }

        al::tryStartMclAnimIfExist(this, "DisasterSpikeOn");
        setActive(true, false);
        setHot(false);
    }

    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    s32 delay = mIsOcean ? mDisasterSpikeDirector->getParam().mOceanSpikeMoveDelay :
                           mDisasterSpikeDirector->getParam().mSpikeMoveDelayTime;
    if (mStep++ >= delay) {
        startNerveAction("Move");
    }
}

/**
 * @brief Fall from the current move point to the next one, then land.
 */
void DisasterSpike::exeMove() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    if (!isDisaster()) {
        startNerveAction("Crumble");
        return;
    }

    if (al::isFirstStep(this)) {
        if (!mIsOcean && !SingleModeDataFunction::isPhase0(GameDataHolderAccessor(this))) {
            DisasterModeController::tryGetController(this)->stopFireballs();
        }

        al::validateHitSensor(this, "NPCDisasterAvoid");
        al::validateHitSensor(this, "EnemyDamage");
        al::validateHitSensor(this, "PlayerPush");
        al::setHitSensorPosPtr(this, "PlayerPush", &mPlayerPushPos);
        al::setSensorRadius(this, "PlayerPush", 0.0f);
    }

    s32 moveTime = (s32)(mDisasterSpikeDirector->getParam().mSpikeMoveTimeGlobalScale *
                         (f32)mCurrentMove->mMoveTime);
    if (mStep <= moveTime) {
        f32 rate = sead::Mathf::clamp((f32)mStep++ / (f32)moveTime, 0.0f, 1.0f);
        rate = al::easeByType(rate, mCurrentMove->mInterpolateType);

        sead::Vector3f pos;
        al::lerpVec(&pos, getCurrentPosition(), getNextPosition(), rate);
        sead::Quatf quat;
        al::slerpQuat(&quat, getCurrentQuat(), getNextQuat(), rate);
        setTransform(pos, quat);
        updateShadowDirection();
        setShadowDropLengthPercent(1.0f - rate);
        al::setSensorRadius(this, "PlayerPush", rate * 250.0f);

        mPlayerPushPos = getEndPosition();
        if (mPlayer != nullptr) {
            sead::Vector3f fallVec = getEndPosition() - getStartPosition();
            sead::Vector3f toPlayer = al::getTrans(mPlayer) - getStartPosition();
            sead::Vector3f fallDir = fallVec;
            fallDir.normalize();
            f32 dist = sead::Mathf::clamp(toPlayer.dot(fallDir), 0.0f, fallVec.length());
            mPlayerPushPos = getStartPosition() + fallDir * dist;
        }
    }

    if (mIsHideTip && mHideTipStepFrames != -1 &&
        al::isVisAnimExist(this, "DisasterSpikeTipStepHide")) {
        f32 frameMax = al::getVisAnimFrameMax(this, "DisasterSpikeTipStepHide");
        s32 hideStartStep = moveTime - mHideTipStepFrames;
        if (hideStartStep == mStep) {
            al::tryStartVisAnimIfExist(this, "DisasterSpikeTipStepHide");
        } else {
            s32 lastFrame = (s32)(frameMax - 1.0f);
            if (hideStartStep + lastFrame == mStep) {
                al::setVisAnimFrameAndStop(this, lastFrame);
            }
        }
    }

    if (mStep > moveTime) {
        mCurrentMove = mCurrentMove->mMoveNext;
        if (mCurrentMove != nullptr && mCurrentMove->mMoveNext != nullptr) {
            startNerveAction("Move");
        } else {
            mCurrentMove = this;
            startLand();
        }
    }
}

/**
 * @brief Get the position of the current move point.
 * @return The current move start position.
 */
sead::Vector3f DisasterSpike::getCurrentPosition() {
    if (isSingleSpike()) {
        return getStartPosition();
    }

    return mCurrentMove->mPosition;
}

/**
 * @brief Get the position of the next move point.
 * @return The current move end position.
 */
sead::Vector3f DisasterSpike::getNextPosition() {
    if (isSingleSpike()) {
        return mPosition;
    }

    return mCurrentMove->mMoveNext->mPosition;
}

/**
 * @brief Get the rotation of the current move point.
 * @return The current move start rotation.
 */
sead::Quatf DisasterSpike::getCurrentQuat() {
    return mCurrentMove->getStartQuatWithOffset(mRotateOffsetDegree);
}

/**
 * @brief Get the rotation of the next move point.
 * @return The current move end rotation.
 */
sead::Quatf DisasterSpike::getNextQuat() {
    if (isSingleSpike()) {
        return getStartQuatWithOffset(mRotateOffsetDegree);
    }

    return mCurrentMove->mMoveNext->getStartQuatWithOffset(mRotateOffsetDegree);
}

/**
 * @brief Move the spike and its children and update the hot collision.
 * @param pos New position.
 * @param quat New rotation.
 */
void DisasterSpike::setTransform(sead::Vector3f pos, sead::Quatf quat) {
    al::setTrans(this, pos);
    al::setQuat(this, quat);
    updateHotCollisionMtx();

    for (s32 i = 0; i < getChildCount(); i++) {
        sead::Vector3f offset = getChildOwner()->mChildOffsets[i];
        al::setTrans(getChild(i), offset + al::getTrans(this));
    }
}

/**
 * @brief Make the shadow drop along the spike's down direction.
 */
void DisasterSpike::updateShadowDirection() {
    sead::Vector3f up;
    al::calcUpDir(&up, this);
    getShadowKeeper()->findShadowMask("Body")->setDropDir(-up);
}

/**
 * @brief Get the position where the spike lands.
 * @return Position of the last spike of the MoveNext chain.
 */
sead::Vector3f DisasterSpike::getEndPosition() {
    return getLastMove()->mPosition;
}

/**
 * @brief Get the position where the spike starts falling.
 * @return The placed position, raised by the single spike start distance for single spikes.
 */
sead::Vector3f DisasterSpike::getStartPosition() {
    if (!isSingleSpike()) {
        return mPosition;
    }

    sead::Vector3f up = sead::Vector3f::ey;
    al::calcQuatUp(&up, getStartQuatWithOffset(mRotateOffsetDegree));
    return mPosition + up * mDisasterSpikeDirector->getParam().mSingleSpikeStartDistance;
}

/**
 * @brief Find where the spike landed (ground, water or placed height) and start landing.
 */
void DisasterSpike::startLand() {
    sead::Vector3f groundPos = sead::Vector3f::zero;
    sead::Quatf groundQuat = sead::Quatf::unit;
    bool isGround = false;
    if (!mIsOcean) {
        isGround = checkGround(groundPos, groundQuat);
    }

    bool isWater = false;
    al::AreaObj* waterArea =
        rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea, al::getTrans(this));
    if (waterArea != nullptr) {
        f32 halfHeight = waterArea->getAreaShape()->mScale.y * 500.0f;
        sead::Vector3f surfacePos = al::getTrans(this);
        surfacePos.y = halfHeight + waterArea->_28(1, 3);
        if (!isGround || surfacePos.y > groundPos.y) {
            mEffectMtx.makeQT(sead::Quatf::unit, surfacePos);
            isWater = true;
        }
    }

    if (!isWater) {
        if (isGround) {
            mEffectMtx.makeQT(groundQuat, groundPos);
        } else {
            sead::Vector3f up = sead::Vector3f::ey;
            al::calcQuatUp(&up, getLastMove()->getStartQuatWithOffset(mRotateOffsetDegree));
            sead::Vector3f pos = al::getTrans(this) + up * (f32)mLandHeight;
            mEffectMtx.makeQT(al::getQuat(this), pos);
        }
    }

    if (isGround && mDirt != nullptr && !mIsOcean &&
        mDisasterSpikeDirector->tryGetSpikeBottom(this) == nullptr) {
        mDirt->appear(groundPos, groundQuat);
        al::showModelIfHide(mDirt);
        if (mIsHideDirtBottom) {
            al::tryStartVisAnimIfExist(mDirt, "DisasterSpikeDirtHide_fcl");
        }
    }

    al::invalidateHitSensor(this, "EnemyDamage");
    al::invalidateHitSensor(this, "PlayerPush");
    if (!mIsOcean && isGround) {
        al::validateHitSensor(this, "EnemyDamageLanded");
        sead::Vector3f offset(0.0f, groundPos.y - al::getTrans(this).y, 0.0f);
        al::setSensorFollowPosOffset(this, "EnemyDamageLanded", offset);
    }

    if (!mIsOcean) {
        DisasterModeController::tryGetController(this)->startFireballs();
    }

    if (mIsOcean) {
        startNerveAction("LandOcean");
    } else if (isWater) {
        startNerveAction("LandWater");
    } else {
        startNerveAction("Land");
    }
}

/**
 * @brief Land on the ground.
 */
void DisasterSpike::exeLand() {
    land();
}

/**
 * @brief Shared landing behavior: stop glowing, pick the spike length and wait before becoming
 * a platform.
 */
void DisasterSpike::land() {
    if (al::isFirstStep(this)) {
        mHasLanded = true;
        al::hideShadow(this);
        mIsEchoBlock = false;
        mEchoTimer = 0;
        al::tryStartMclAnimIfExist(this, "DisasterSpikeOff");
        if (mDirt != nullptr) {
            mDirt->tryStartGlowOffAnim();
        }

        if (mIsHideTip && al::tryStartVisAnimIfExist(this, "DisasterSpikeTipHide")) {
            al::setVisAnimFrameAndStop(this, 0.0f);
        }

        if (mIsShortSpike) {
            if (mDirt == nullptr || !al::isAlive(mDirt) ||
                !tryStartSpikeLengthVisAnim(this, mDirt)) {
                if (al::tryStartVisAnimIfExist(this, "DisasterSpikeShort")) {
                    al::setVisAnimFrameAndStop(this, 0.0f);
                }
            }
        }

        if (mIsOcean && mOceanWaveKeeper != nullptr) {
            al::startOceanWave(this, "WaterColumn");
        }

        al::Triangle triangle;
        sead::Vector3f landPos(mEffectMtx(0, 3), mEffectMtx(1, 3), mEffectMtx(2, 3));
        sead::Vector3f checkPos;
        sead::Vector3f up;
        al::calcUpDir(&up, this);
        checkPos = landPos + up * 50.0f;
        sead::Vector3f checkDir = up * -300.0f;
        if (alCollisionUtil::getFirstPolyOnArrow(this, nullptr, &triangle, checkPos, checkDir,
                                                 nullptr, nullptr) &&
            al::isMaterialCode("EchoBlock", triangle)) {
            mIsEchoBlock = true;
        }

        updateEchoPulse();
    } else {
        if (rc::isAnyActiveButDemoCameraDemo(this)) {
            return;
        }

        if (mStep++ > 60) {
            if (al::isValidCollisionParts(mHotCollision)) {
                setHot(false);
            }

            if (mIsOcean) {
                mSinkDelay =
                    al::getRandom(mDisasterSpikeDirector->getParam().mOceanSpikeSinkDelayMin,
                                  mDisasterSpikeDirector->getParam().mOceanSpikeSinkDelayMax);
                startNerveAction("SinkDelay");
            } else {
                startNerveAction("LandWait");
            }

            return;
        }
    }

    if (!isDisaster()) {
        startNerveAction("Crumble");
    }
}

/**
 * @brief Land in water.
 */
void DisasterSpike::exeLandWater() {
    land();
}

/**
 * @brief Stand as a platform until the disaster ends.
 */
void DisasterSpike::exeLandWait() {
    if (al::isExistAction(this, "Wait") && !al::isActionPlaying(this, "Wait")) {
        if (!(al::isActionPlaying(this, "Reaction") || al::isActionPlaying(this, "ReactionHigh")) ||
            al::isActionEnd(this)) {
            al::startAction(this, "Wait");
        }
    }

    if (DisasterModeController::tryGetController(this)
            ->getSuperBowser()
            ->isDoingEndingPreparations()) {
        startNerveAction("ShakeWait");
        return;
    }

    if (!isDisaster()) {
        startNerveAction("Crumble");
        return;
    }

    updateCollisionValidation();
    if (al::isMclAnimExist(this, "DisasterSpike") &&
        al::isMclAnimPlaying(this, "DisasterSpikeOff") && al::isMclAnimEnd(this)) {
        al::tryStartMclAnimIfExist(this, "DisasterSpike");
        if (mDirt != nullptr) {
            mDirt->tryStartGlowLoopAnim();
        }
    }
}

/**
 * @brief Pulse the echo and only keep the collision valid near the player.
 */
void DisasterSpike::updateCollisionValidation() {
    updateEchoPulse();

    if (isWithinCollideDistance()) {
        al::validateCollisionParts(this);
    } else {
        al::invalidateCollisionParts(mHotCollision);
        al::invalidateCollisionParts(this);
    }
}

/**
 * @brief Land in the ocean.
 */
void DisasterSpike::exeLandOcean() {
    land();
}

/**
 * @brief Wait before shaking.
 */
void DisasterSpike::exeShakeDelay() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    updateCollisionValidation();
    if (mStep++ > mDisasterSpikeDirector->getParam().mSpikeShakeDelayTime) {
        shake(mDisasterSpikeDirector->getParam().mSpikeShakeTime);
    }
}

/**
 * @brief Start shaking around the current position.
 * @param frames Duration of the shake.
 */
void DisasterSpike::shake(s32 frames) {
    mShakeBasePos = al::getTrans(this);
    mShakeFrames = frames;
    startNerveAction("Shake");
}

/**
 * @brief Shake harder and harder, then crumble.
 */
void DisasterSpike::exeShake() {
    if (rc::isAnyActiveButDemoCameraDemo(this)) {
        return;
    }

    if (mStep > mShakeFrames) {
        if (mIsLaserDead) {
            startNerveAction("CrumbleLaser");
        } else {
            startNerveAction("Crumble");
        }
    } else {
        f32 rate = sead::Mathf::clamp((f32)mStep / (f32)mShakeFrames, 0.0f, 1.0f);
        rate = al::easeByType(rate, 1);
        f32 strength = rate * mDisasterSpikeDirector->getParam().mSpikeShakeStrength;
        sead::Vector3f pos;
        al::addRandomVector(&pos, mShakeBasePos, strength);
        al::setTrans(this, pos);
        updateCollisionValidation();
    }

    mStep++;
}

/**
 * @brief Shake slightly while Fury Bowser is leaving.
 */
void DisasterSpike::exeShakeWait() {
    if (al::isFirstStep(this)) {
        mShakeBasePos = al::getTrans(this);
    }

    sead::Vector3f pos;
    al::addRandomVector(&pos, mShakeBasePos,
                        mDisasterSpikeDirector->getParam().mSpikeShakeWaitStrength);
    al::setTrans(this, pos);
    updateCollisionValidation();
    if (!isDisaster()) {
        startNerveAction("Crumble");
    }

    mStep++;
}

/**
 * @brief Crumble.
 */
void DisasterSpike::exeCrumble() {
    updateCrumble();
}

/**
 * @brief Break the spike and its children into coins, then deactivate it.
 */
void DisasterSpike::updateCrumble() {
    if (al::isFirstStep(this)) {
        if (mIsEchoBlock) {
            rc::killAllEcho(this);
        }

        setActive(false, true);
        turnIntoCoins();
        if (mDirt != nullptr) {
            mDirt->kill();
        }
    } else if (mStep++ > 30) {
        startNerveAction("Deactivate");
        if (getCollisionParts()->isValidCollision()) {
            getCollisionParts()->invalidateBySystem();
        }
    }
}

/**
 * @brief Crumble after being destroyed by a laser.
 */
void DisasterSpike::exeCrumbleLaser() {
    updateCrumble();
}

/**
 * @brief Wait for the laser explosion, then shake.
 */
void DisasterSpike::exeLaserDeath() {
    if (al::isGreaterEqualStep(this, 15)) {
        shake(15);
    }
}

/**
 * @brief Wait before sinking an ocean spike.
 */
void DisasterSpike::exeSinkDelay() {
    if (mStep++ > mSinkDelay) {
        startNerveAction("Sink");
    }

    if (DisasterModeController::tryGetController(this)
            ->getSuperBowser()
            ->isDoingEndingPreparations()) {
        startNerveAction("ShakeWait");
        return;
    }

    if (!isDisaster()) {
        startNerveAction("Crumble");
    }
}

/**
 * @brief Sink an ocean spike into the water, then crumble it.
 */
void DisasterSpike::exeSink() {
    f32 rate = al::easeInOut((f32)mStep++ /
                             (f32)mDisasterSpikeDirector->getParam().mOceanSpikeSinkTime);
    f32 sinkDistance = rate * (f32)mDisasterSpikeDirector->getParam().mOceanSpikeSinkAmount;
    sead::Vector3f pos = mPosition - sinkDistance * sead::Vector3f::ey;
    al::setTrans(this, pos);

    if (rate >= 1.0f) {
        startNerveAction("Crumble");
        return;
    }

    if (DisasterModeController::tryGetController(this)
            ->getSuperBowser()
            ->isDoingEndingPreparations()) {
        startNerveAction("ShakeWait");
        return;
    }

    if (!isDisaster()) {
        startNerveAction("Crumble");
    }
}

/**
 * @brief Catch up with the all fall time once the demo camera is done.
 */
void DisasterSpike::exeClipInAfterDemo() {
    if (!rc::isAnyActiveButDemoCameraDemo(this)) {
        clipInAtAllFallTime();
    }
}

/**
 * @brief Set the position the spike lands at.
 * @param pos The landing position.
 */
void DisasterSpike::setPosition(sead::Vector3f pos) {
    mPosition = pos;
}

/**
 * @brief Move the spike back to the start of its fall.
 */
void DisasterSpike::resetTransform() {
    setTransform(getStartPosition(), getStartQuatWithOffset(mRotateOffsetDegree));
}

/**
 * @brief Check whether the player or Plessie are too close to the spike's fall line.
 * @return True if the spike must not fall.
 */
bool DisasterSpike::isWithinSpikeNoFallRadius() {
    if (mPlayer == nullptr) {
        mPlayer = al::tryFindNearestPlayerActor(this);
        if (mPlayer == nullptr) {
            return true;
        }
    }

    if (isPositionInSpikeNoFallCylinder(al::getTrans(mPlayer),
                                        mDisasterSpikeDirector->getSpikeNoFallRadius())) {
        return true;
    }

    auto* raidon = al::tryGetSceneObj<RaidonSurf>(this, SceneObjID_RaidonSurf);
    if (raidon != nullptr &&
        isPositionInSpikeNoFallCylinder(al::getTrans(raidon),
                                        mDisasterSpikeDirector->getSpikeNoFallRadiusPlessie())) {
        return true;
    }

    return false;
}

/**
 * @brief Check whether the director may pick this spike to be replaced by a gold spike.
 * @return True for a plain spike not already picked and not excluded from gold spikes.
 */
bool DisasterSpike::canBeReplacedByGold() const {
    if (mIsReplacedByGold) {
        return false;
    }

    if (mIsAlwaysGold) {
        return false;
    }

    if (!mCanBeGold) {
        return false;
    }

    return !mIsNeverGold;
}

/**
 * @brief Show the models of the children and of the objects their blocks hold.
 */
void DisasterSpike::showChildren() {
    for (s32 i = 0; i < getChildCount(); i++) {
        al::showModelIfHide(getChild(i));

        if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            showBlockItem(static_cast<BlockBrick*>(getChild(i))->getBlockStateItem());
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            showBlockItem(static_cast<BlockQuestion*>(getChild(i))->getBlockStateItem());
        }
    }
}

/**
 * @brief Break the children: coins vanish, blocks break, their items disappear and any other
 * child is killed.
 */
void DisasterSpike::crumbleChildren() {
    for (s32 i = 0; i < getChildCount(); i++) {
        if (al::isEqualString(getChild(i)->getName(), cCoinName)) {
            if (al::isAlive(getChild(i))) {
                al::startHitReactionDeath(getChild(i));
                getChild(i)->kill();
            }
        } else if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            auto* brick = static_cast<BlockBrick*>(getChild(i));
            if (al::isAlive(brick)) {
                brick->tryAppearBreakModel();
                getChild(i)->kill();
            } else {
                killBlockItem(brick->getBlockStateItem(), true);
            }
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            auto* question = static_cast<BlockQuestion*>(getChild(i));
            if (al::isAlive(question)) {
                al::startHitReactionBreak(question);
                question->kill();
            } else {
                killBlockItem(question->getBlockStateItem(), true);
            }
        } else {
            getChild(i)->kill();
        }
    }
}

/**
 * @brief Kill the children and the objects their blocks hold.
 */
void DisasterSpike::killChildren() {
    for (s32 i = 0; i < getChildCount(); i++) {
        if (al::isAlive(getChild(i))) {
            getChild(i)->kill();
        } else if (al::isEqualString(getChild(i)->getName(), cBlockBrickName)) {
            killBlockItem(static_cast<BlockBrick*>(getChild(i))->getBlockStateItem(), false);
        } else if (al::isEqualString(getChild(i)->getName(), cBlockQuestionName)) {
            killBlockItem(static_cast<BlockQuestion*>(getChild(i))->getBlockStateItem(), false);
        }
    }
}

/**
 * @brief Get the placed rotation turned around the spike's axis.
 * @param offsetDegree Angle to turn the rotation by, in degrees.
 * @return The turned rotation.
 */
sead::Quatf DisasterSpike::getStartQuatWithOffset(f32 offsetDegree) {
    sead::Quatf quat = sead::Quatf::unit;
    al::rotateQuatLocalDirDegree(&quat, mQuat, 1, offsetDegree);
    return quat;
}

/**
 * @brief Get the last spike of the MoveNext chain.
 * @return The spike the fall ends at.
 */
DisasterSpike* DisasterSpike::getLastMove() {
    DisasterSpike* spike = this;
    while (spike->mMoveNext != nullptr) {
        spike = spike->mMoveNext;
    }

    return spike;
}

/**
 * @brief Check whether the spike still burns: while falling and shortly after landing.
 * @return True if the spike is hot.
 */
bool DisasterSpike::isHot() const {
    bool isLanding = al::isNerve(this, NrvDisasterSpike.Land.data()) ||
                     al::isNerve(this, NrvDisasterSpike.LandWait.data()) ||
                     al::isNerve(this, NrvDisasterSpike.LandOcean.data());
    bool isMoving = al::isNerve(this, NrvDisasterSpike.Move.data());
    return isMoving || (isLanding && al::isLessEqualStep(this, 60));
}

/**
 * @brief Blow coins out of a crumbling spike (a ring of coins for gold spikes).
 */
void DisasterSpike::turnIntoCoins() {
    if (mIsOcean) {
        return;
    }

    if (mCoinBlowConcentric != nullptr) {
        sead::Vector3f offset(0.0f, 150.0f, 0.0f);
        offset.rotate(al::getQuat(this));
        sead::Matrix34f poseMtx;
        poseMtx.makeQT(al::getQuat(this), al::getTrans(this));
        al::updatePoseMtx(mCoinBlowConcentric, &poseMtx);
        sead::Vector3f landPos(mEffectMtx(0, 3), mEffectMtx(1, 3), mEffectMtx(2, 3));
        al::resetPosition(mCoinBlowConcentric, landPos + offset, false);
        mCoinBlowConcentric->setTimerFrame(600);
        mCoinBlowConcentric->appear();
    } else if (mCoinBlow != nullptr) {
        sead::Vector3f offset(0.0f, 500.0f, 0.0f);
        offset.rotate(al::getQuat(this));
        al::setTrans(mCoinBlow, al::getTrans(this) + offset);
        al::resetPosition(mCoinBlow, false);
        mCoinBlow->setLifeTime(600);
        mCoinBlow->appearWithHitReaction();
    }
}

/**
 * @brief Set whether the spike dies as soon as it leaves the view.
 * @param isKill Whether to kill the spike out of view.
 */
void DisasterSpike::setKillOutOfView(bool isKill) {
    if (isReplacedByGold() && mGoldSpike != nullptr) {
        mGoldSpike->setKillOutOfView(isKill);
        return;
    }

    if (mIsForeverSpike) {
        return;
    }

    mIsKillOutOfView = isKill;
    if (isKill && al::isClipped(this)) {
        kill();
    }
}

/**
 * @brief Check whether the spike (or the gold spike replacing it) was triggered.
 * @return True if the shown spike was triggered.
 */
bool DisasterSpike::isTriggered() {
    return getActiveSpike()->mIsTriggered;
}

/**
 * @brief Set whether the spike (or the gold spike replacing it) was triggered.
 * @param isTriggered Whether the spike was triggered.
 */
void DisasterSpike::setTriggered(bool isTriggered) {
    if (isReplacedByGold() && mGoldSpike != nullptr) {
        mGoldSpike->setTriggered(isTriggered);
        return;
    }

    mIsTriggered = isTriggered;
}

/**
 * @brief Set whether a gold spike falls in place of this spike.
 * @param isReplaced Whether the spike is replaced by a gold spike.
 */
void DisasterSpike::setReplacedByGold(bool isReplaced) {
    mIsReplacedByGold = isReplaced;
    if (!isReplaced) {
        mGoldSpike = nullptr;
    }
}

/**
 * @brief Release a gold spike from the spike it replaced.
 */
void DisasterSpike::clearGoldSpikeOriginalSpike() {
    mOriginalSpike = nullptr;
}

/**
 * @brief Check whether this gold spike currently replaces a spike.
 * @return True if the gold spike is in use.
 */
bool DisasterSpike::isGoldSpikeInUse() const {
    return mOriginalSpike != nullptr;
}

/**
 * @brief Get the number of children carried by the spike.
 * @return The child count of the spike owning the children.
 */
s32 DisasterSpike::getChildCount() {
    return getChildOwner()->mChildCount;
}

/**
 * @brief Get a child carried by the spike.
 * @param index Index of the child.
 * @return The child actor.
 */
al::LiveActor* DisasterSpike::getChild(s32 index) {
    if (mIsGold && mOriginalSpike != nullptr) {
        return mOriginalSpike->getChild(index);
    }

    return mChildren[index];
}

/**
 * @brief Crumble the spike and its children into coins.
 */
void DisasterSpike::crumble() {
    setActive(false, true);
    turnIntoCoins();
}

/**
 * @brief Probe the ground below the spike's landing position with four arrows around its base.
 * @param rPos Set to the landing position on the ground.
 * @param rQuat Set to the rotation aligned to the (slope limited) ground normal.
 * @return True if ground was found.
 */
bool DisasterSpike::checkGround(sead::Vector3f& rPos, sead::Quatf& rQuat) {
    sead::Vector3f up = sead::Vector3f::ey;
    sead::Quatf quat = getLastMove()->getStartQuatWithOffset(mRotateOffsetDegree);
    al::calcQuatUp(&up, quat);
    sead::Vector3f front = sead::Vector3f::zero;
    al::calcQuatFront(&front, quat);
    sead::Vector3f side = sead::Vector3f::zero;
    al::calcQuatSide(&side, quat);

    f32 height = al::getScale(this).y * 600.0f;
    f32 radius = al::getScale(this).y * 175.0f;
    sead::Vector3f hitPos = sead::Vector3f::zero;
    sead::Vector3f hitNormal = sead::Vector3f::zero;
    sead::Vector3f normalSum = sead::Vector3f::zero;
    sead::Vector3f posSum = sead::Vector3f::zero;
    sead::Vector3f top = up * height + al::getTrans(this);
    sead::Vector3f checkPos0 = top + (side + front) * radius;
    sead::Vector3f checkPos1 = top + (side - front) * radius;
    sead::Vector3f checkPos2 = top + (front - side) * radius;
    sead::Vector3f checkPos3 = top + (-side - front) * radius;
    sead::Vector3f checkDir = up * (height * -1.5f);

    al::CollisionPartsFilterActor actorFilter(this);
    al::CollisionPartsFilterNoSpecialPurpose purposeFilter;
    al::CollisionPartsFilterOrPair filter(&actorFilter, &purposeFilter);
    s32 hitNum = 0;
    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, checkPos0, checkDir,
                                                   &filter, nullptr)) {
        normalSum += hitNormal;
        posSum += hitPos;
        hitNum++;
    }

    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, checkPos1, checkDir,
                                                   &filter, nullptr)) {
        normalSum += hitNormal;
        posSum += hitPos;
        hitNum++;
    }

    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, checkPos2, checkDir,
                                                   &filter, nullptr)) {
        normalSum += hitNormal;
        posSum += hitPos;
        hitNum++;
    }

    if (alCollisionUtil::getHitPosAndNormalOnArrow(this, &hitPos, &hitNormal, checkPos3, checkDir,
                                                   &filter, nullptr)) {
        normalSum += hitNormal;
        posSum += hitPos;
        hitNum++;
    }

    if (hitNum < 1 || al::isNear(normalSum, front, 0.001f)) {
        return false;
    }

    f32 invNum = 1.0f / hitNum;
    posSum *= invNum;
    sead::Vector3f heightOffset = up * (posSum - al::getTrans(this)).dot(up);
    rPos = al::getTrans(this) + heightOffset;
    normalSum *= invNum;
    f32 angle =
        sead::Mathf::rad2deg(sead::Mathf::acos(sead::Mathf::clamp(normalSum.dot(up), -1.0f, 1.0f)));
    sead::Vector3f groundUp = normalSum;
    if (angle > cMaxGroundSlopeDegree) {
        sead::Vector3f axis = up.cross(normalSum);
        if (!al::isNearZero(axis, 0.001f)) {
            groundUp = up;
            al::rotateVectorDegree(&groundUp, groundUp, axis, cMaxGroundSlopeDegree);
        }
    }

    al::makeQuatSideUp(&rQuat, front.cross(groundUp), groundUp);
    return true;
}

/**
 * @brief Check whether the start of the fall is visible.
 * @return True if the start position is in view.
 */
bool DisasterSpike::isAppearPositionInCameraView() {
    return isPositionInCameraView(getStartPosition());
}

/**
 * @brief Check whether a position is inside the cylinder around the spike's fall line.
 * @param pos Position to check.
 * @param radius Radius of the cylinder.
 * @return True if the position is within the radius of the fall line.
 */
bool DisasterSpike::isPositionInSpikeNoFallCylinder(sead::Vector3f pos, f32 radius) {
    sead::Vector3f start = getStartPosition();
    sead::Vector3f end = getEndPosition();
    sead::Vector3f toPos = pos - start;
    sead::Vector3f fallDir = end - start;
    f32 fallLengthSq = fallDir.squaredLength();
    fallDir.normalize();
    f32 dist = toPos.dot(fallDir);
    if (dist >= 0.0f && dist <= sead::Mathf::sqrt(fallLengthSq) + radius) {
        sead::Vector3f closest = start + fallDir * dist;
        return !((closest - pos).squaredLength() > radius * radius);
    }

    return false;
}

/**
 * @brief Check whether the player is close enough to collide with the spike.
 * @return True if the player is within 3000 units.
 */
bool DisasterSpike::isWithinCollideDistance() {
    if (mPlayer == nullptr) {
        return false;
    }

    sead::Vector3f playerTrans = al::getTrans(mPlayer);
    return (playerTrans - al::getTrans(this)).squaredLength() < 3000.0f * 3000.0f;
}
