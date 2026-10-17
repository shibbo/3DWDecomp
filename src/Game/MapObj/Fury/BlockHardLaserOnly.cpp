#include "MapObj/Fury/BlockHardLaserOnly.hpp"

#include "MapObj/DisasterModeController.hpp"
#include "MapObj/Fury/BlockHardLaserOnlyDebris.hpp"
#include "MapObj/Fury/DisasterBlockDirector.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Controller/PadRumbleFunction.hpp"
#include "Library/LiveActor/LiveActorFunc.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Play/Placement/PlacementHolder.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ScoreUtil.hpp"
#include <attributes.h>
#include <cstdlib>
#include <new>

namespace {
/**
 * @brief Parameter table that is dynamically initialized in this unit but never read by the shipped
 * code. Kept (USED) so the data exists; the original sits merged with the nerve table in .bss.
 */
USED sead::Vector2f sUnusedParams[] = {
    {220.0f, 150.0f}, {220.0f, 0.0f}, {180.0f, 0.0f}, {210.0f, 360.0f}, {210.0f, 0.0f},
    {120.0f, 0.0f},   {220.0f, 150.0f}, {220.0f, 0.0f}, {-50.0f, 0.0f},
};

NERVE_ACTION_IMPL(BlockHardLaserOnly, Wait)
NERVE_ACTION_IMPL(BlockHardLaserOnly, Reaction)
NERVE_ACTION_IMPL(BlockHardLaserOnly, BreakStart)
NERVE_ACTION_IMPL(BlockHardLaserOnly, Breaking)
NERVE_ACTION_IMPL(BlockHardLaserOnly, FullGlowDelay)
NERVE_ACTION_IMPL(BlockHardLaserOnly, FullGlowOn)
NERVE_ACTION_IMPL(BlockHardLaserOnly, FullGlow)
NERVE_ACTION_IMPL(BlockHardLaserOnly, FullGlowOff)

NERVE_ACTIONS_MAKE_STRUCT(BlockHardLaserOnly, Wait, Reaction, BreakStart, Breaking, FullGlowDelay,
                          FullGlowOn, FullGlow, FullGlowOff)

/** @brief Squared distance from the block (cluster) center within which the player makes it glow. */
constexpr f32 cFullGlowRangeSq = 2000.0f * 2000.0f;
}  // namespace

/**
 * @brief Construct the block.
 * @param pName The placement name (ignored: the actor is always named "BlockHardLaserOnly").
 */
BlockHardLaserOnly::BlockHardLaserOnly(const char* pName) : al::LiveActor("BlockHardLaserOnly") {}

/**
 * @brief Destroy the block, freeing its mtx connector.
 */
BlockHardLaserOnly::~BlockHardLaserOnly() {
    if (mConnector != nullptr) {
        mConnector->al::MtxConnector::~MtxConnector();
        ::operator delete(mConnector);
        mConnector = nullptr;
    }
}

/**
 * @brief Initialize the block: model, break model, save id, debris and placement args.
 * @param rInfo The actor init info.
 */
void BlockHardLaserOnly::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "Wait", &NrvBlockHardLaserOnly.collector, 0);
    al::initActorChangeModelSuffix(this, rInfo, rc::getBlockSuffixName(rInfo, false));
    mFileID = rInfo.getPlacementInfo()._28;

    auto* controller = DisasterModeController::tryGetController(this);
    if (controller != nullptr) {
        auto* director = controller->getBlockDirector();
        if (director != nullptr) {
            mDirector = director;
            director->registerDisasterBlock(this);
        }
    }

    al::setClippingInfo(this, 3000.0f, nullptr);
    s32 zone = mPlacementHolder->getZoneNo();
    al::StringTmp<32> id("%s", mPlacementHolder->getId());
    mSaveId = std::atoi(id.getPart(3).cstr()) | (zone << 16);

    if (al::isEqualString(al::getModelName(this), "BlockDisaster2x2M")) {
        mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockDisaster2x2MBreak",
                                         nullptr, nullptr, "Break", true);
        mBreakModelType = BreakModelType::Size2x2;
    } else if (al::isEqualString(al::getModelName(this), "BlockDisaster4x2M")) {
        mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockDisaster4x2MBreak",
                                         nullptr, nullptr, "Break", true);
        mBreakModelType = BreakModelType::Size4x2;
    } else if (al::isEqualString(al::getModelName(this), "BlockDisaster6x2M")) {
        mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockDisaster6x2MBreak",
                                         nullptr, nullptr, "Break", true);
        mBreakModelType = BreakModelType::Size6x2;
    } else if (al::isEqualString(al::getModelName(this), "BlockDisaster8x2M")) {
        mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockDisaster8x2MBreak",
                                         nullptr, nullptr, "Break", true);
        mBreakModelType = BreakModelType::Size8x2;
    } else {
        mBreakModel = new al::BreakModel(this, "硬ブロック壊れモデル", "BlockDisaster2x2MBreak",
                                         nullptr, nullptr, "Break", true);
    }

    al::initCreateActorNoPlacementInfo(mBreakModel, rInfo);

    bool isCreateDebris;
    al::tryGetArg(&isCreateDebris, rInfo, "CreateDebris");
    if (isCreateDebris) {
        sead::Vector3f front = sead::Vector3f::zero;
        al::calcQuatFront(&front, al::getQuat(this));
        if (al::isNearZero(front.y)) {
            const char* debrisName = al::getRandom(0, 2) == 0 ? "BlockDisasterDebris2x2M" :
                                                                 "BlockDisasterDebris4x2M";
            mDebris[0] = new BlockHardLaserOnlyDebris(debrisName, this);
            mDebris[0]->init(rInfo);
            mDebris[0]->kill();

            if (mBreakModelType >= BreakModelType::Size4x2) {
                debrisName = al::getRandom(0, 2) == 0 ? "BlockDisasterDebris2x2M" :
                                                        "BlockDisasterDebris4x2M";
                mDebris[1] = new BlockHardLaserOnlyDebris(debrisName, this);
                mDebris[1]->init(rInfo);
                mDebris[1]->kill();

                sead::Vector3f side = sead::Vector3f::ex;
                al::calcSideDir(&side, this);
                al::setTrans(mDebris[0], al::getTrans(this) + side * 100.0f);
                al::setTrans(mDebris[1], al::getTrans(this) - side * 100.0f);
            }
        }
    }

    al::tryGetArg(&mMaxChainBreakDistance, rInfo, "MaxChainBreakDistance");
    al::tryGetArg(&mIsDisabledInPhase0, rInfo, "IsDisabledInPhase0");
    mConnector = al::tryCreateMtxConnector(this, rInfo);

    if (SingleModeDataFunction::isDisasterBlockDestroyed(GameDataHolderAccessor(this), mSaveId)) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }

    if (al::isExistShadow(this)) {
        al::invalidateShadow(this);
    }
}

/**
 * @brief Attach to the collision below and show the debris if the block was already destroyed.
 */
void BlockHardLaserOnly::initAfterPlacement() {
    if (mConnector != nullptr) {
        al::attachMtxConnectorToCollision(mConnector, this, false);
    }

    al::updateMaterialCodeWater(this);

    if (SingleModeDataFunction::isDisasterBlockDestroyed(GameDataHolderAccessor(this), mSaveId)) {
        appearDebris();
    }
}

/**
 * @brief Make the debris pieces left behind by the block appear.
 */
void BlockHardLaserOnly::appearDebris() {
    if (mDebris[0] != nullptr) {
        mDebris[0]->tryAppear();
    }

    if (mDebris[1] != nullptr) {
        mDebris[1]->tryAppear();
    }
}

/**
 * @brief Follow the connected collision, then run the base control.
 */
void BlockHardLaserOnly::control() {
    if (mConnector != nullptr) {
        al::connectPoseQT(this, mConnector);
    }

    al::LiveActor::control();
}

/**
 * @brief Stop all sounds and kill the block.
 */
void BlockHardLaserOnly::kill() {
    al::stopAllSeFromUser(this, 0);
    al::LiveActor::kill();
}

/**
 * @brief Start being clipped.
 */
void BlockHardLaserOnly::startClipped() {
    al::LiveActor::startClipped();
}

/**
 * @brief Stop being clipped.
 */
void BlockHardLaserOnly::endClipped() {
    al::LiveActor::endClipped();
}

/**
 * @brief Move along with a linked actor.
 * @param rTrans The new translation.
 */
void BlockHardLaserOnly::updateLinkedTrans(const sead::Vector3f& rTrans) {
    al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
    alLiveActorFunction::forceUpdateTrans(this, rTrans, true);
    al::setNeedSetBaseMtxAndCalcAnimFlag(this, mIsDisasterMode);
}

/**
 * @brief Handle a sensor message; the block only breaks to a laser attack on its body.
 * @param pMsg The message.
 * @param pSender The sending sensor.
 * @param pReceiver The receiving sensor.
 * @return True if the message was consumed.
 */
bool BlockHardLaserOnly::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSender,
                                    al::HitSensor* pReceiver) {
    if (al::isNerve(this, NrvBlockHardLaserOnly.BreakStart.data())) {
        return false;
    }

    if (al::isNerve(this, NrvBlockHardLaserOnly.Breaking.data())) {
        return false;
    }

    if (al::isSensorPlayer(pSender) && al::isSensorName(pSender, "Eye")) {
        return false;
    }

    if (!al::isSensorName(pReceiver, "Body")) {
        return false;
    }

    if (al::isMsgLaserAttack(pMsg)) {
        al::startNerveAction(this, "Breaking");
        if (mDirector != nullptr && isEqualDirectorSoundPlayer()) {
            mDirector->setGlowSoundPlayer(nullptr);
        }

        return true;
    }

    return false;
}

/**
 * @brief Break the block after a delay (chain break).
 * @param delay The number of steps before the block breaks.
 */
void BlockHardLaserOnly::breakBlock(s32 delay) {
    mBreakDelay = delay;
    al::startNerveAction(this, "Breaking");
    if (mDirector != nullptr && isEqualDirectorSoundPlayer()) {
        mDirector->setGlowSoundPlayer(nullptr);
    }
}

/**
 * @brief Wait state: follow disaster mode and start glowing when the player gets close.
 */
void BlockHardLaserOnly::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
    }

    if (isDisabled()) {
        return;
    }

    bool isDisasterMode = isDisaster();
    if (mIsDisasterMode != isDisasterMode) {
        if (isDisasterMode) {
            mIsDisasterMode = true;
        } else if (mIsDisasterMode) {
            al::getMclAnimFrame(this);
            mIsDisasterMode = false;
        }

        al::setNeedSetBaseMtxAndCalcAnimFlag(this, mIsDisasterMode);
        return;
    }

    if (al::isGreaterEqualStep(this, 3)) {
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, mIsDisasterMode);
        if (isDisasterMode && isPlayerInFullGlowRange()) {
            al::startNerveAction(this, "FullGlowDelay");
        }
    }
}

/**
 * @brief Check whether the block is disabled (in phase 0, when configured so).
 * @return True if the block is disabled.
 */
bool BlockHardLaserOnly::isDisabled() const {
    return mIsDisabledInPhase0 && SingleModeDataFunction::isPhase0(GameDataHolderAccessor(this));
}

/**
 * @brief Check whether disaster mode is active.
 * @return True while disaster mode is active.
 */
bool BlockHardLaserOnly::isDisaster() {
    auto* controller = DisasterModeController::tryGetController(this);
    return controller != nullptr && controller->isDisasterMode();
}

/**
 * @brief Check whether the nearest player is close enough to the block (or its cluster) to glow.
 * @return True if the player is within the full glow range.
 */
bool BlockHardLaserOnly::isPlayerInFullGlowRange() {
    al::LiveActor* player = al::tryFindNearestPlayerActor(this);
    if (player == nullptr || mDirector == nullptr) {
        return false;
    }

    sead::Vector3f center = sead::Vector3f::zero;
    if (mBreakModelType == BreakModelType::Size8x2) {
        center = al::getTrans(this);
    } else {
        mDirector->averageDisasterBlockPosition(center, mFileID, true);
    }

    return (al::getTrans(player) - center).squaredLength() < cFullGlowRangeSq;
}

/**
 * @brief Reaction state: play the reaction, then go back to waiting.
 */
void BlockHardLaserOnly::exeReaction() {
    if (al::isFirstStep(this)) {
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
        al::startAction(this, "Reaction");
    }

    if (al::isActionEnd(this)) {
        al::startNerveAction(this, "Wait");
    }
}

/**
 * @brief Break start state: play the break start action, then break into pieces.
 */
void BlockHardLaserOnly::exeBreakStart() {
    if (al::isFirstStep(this)) {
        al::setNeedSetBaseMtxAndCalcAnimFlag(this, true);
        al::startAction(this, "BreakStart");
    }

    if (al::isActionEnd(this)) {
        kill();
        rc::addScoreByFactor(this, mAttacker, "壊れ", 0.0f, 0);
        al::appearBreakModelRandomRotateY(mBreakModel);
        SingleModeDataFunction::destroyDisasterBlock(GameDataHolderWriter(this), mSaveId);
        al::setRotateX(mBreakModel, 0.0f);
        al::setRotateZ(mBreakModel, 0.0f);
    }
}

/**
 * @brief Breaking state: break after the chain delay, then leave the debris behind.
 */
void BlockHardLaserOnly::exeBreaking() {
    if (al::isStep(this, mBreakDelay)) {
        if (!al::isDead(this)) {
            SingleModeDataFunction::destroyDisasterBlock(GameDataHolderWriter(this), mSaveId);
        }

        al::appearBreakModelRandomRotateY(mBreakModel);
        al::setRotateX(mBreakModel, 0.0f);
        al::setRotateZ(mBreakModel, 0.0f);
        al::hideModelIfShow(this);
        alPadRumbleFunction::startPadRumble(this, "DisasterBlock", -1, true);
        if (mDirector != nullptr) {
            mDirector->notifyBreak(this);
        }
    }

    if (al::isStep(this, mBreakDelay + 40)) {
        appearDebris();
        kill();
    }
}

/**
 * @brief Full glow delay state: wait for the glow anim, or go back if the player leaves.
 */
void BlockHardLaserOnly::exeFullGlowDelay() {
    if (al::isGreaterEqualStep(this, 60) && isGlowAnimDone()) {
        al::startNerveAction(this, "FullGlowOn");
        return;
    }

    if (!isDisaster() || !isPlayerInFullGlowRange()) {
        al::startNerveAction(this, "Wait");
    }
}

/**
 * @brief Check whether the glow anim is at its start or has ended.
 * @return True if the glow anim is done.
 */
bool BlockHardLaserOnly::isGlowAnimDone() {
    if (al::isMclAnimPlaying(this, "Glow")) {
        return al::getMclAnimFrame(this) == 0.0f;
    }

    return al::isMclAnimEnd(this);
}

/**
 * @brief Full glow on state: start the glow on anim, then glow fully.
 */
void BlockHardLaserOnly::exeFullGlowOn() {
    if (al::isFirstStep(this)) {
        al::tryStartMclAnimIfNotPlaying(this, "GlowOn");
        return;
    }

    if (isGlowAnimDone()) {
        al::startNerveAction(this, "FullGlow");
    }
}

/**
 * @brief Full glow state: loop the glow (one block plays the sound) while the player stays close.
 */
void BlockHardLaserOnly::exeFullGlow() {
    if (al::isFirstStep(this)) {
        al::tryStartMclAnimIfNotPlaying(this, "Glow");
    }

    if (al::getMclAnimFrame(this) == 30.0f && mDirector != nullptr) {
        if (mDirector->getGlowSoundPlayer() == nullptr) {
            mDirector->setGlowSoundPlayer(this);
        }

        if (isEqualDirectorSoundPlayer()) {
            al::stopAllSeFromUser(this, 0);
            al::startSe(this, "FullGlow", nullptr);
        }
    }

    if (isDisaster() && isPlayerInFullGlowRange()) {
        return;
    }

    if (isEqualDirectorSoundPlayer()) {
        al::startSe(this, "FullGlow", nullptr);
        mDirector->setGlowSoundPlayer(nullptr);
    }

    al::startNerveAction(this, "FullGlowOff");
}

/**
 * @brief Check whether this block is the one playing the disaster blocks' glow sound.
 * @return True if this block is the director's glow sound player.
 */
bool BlockHardLaserOnly::isEqualDirectorSoundPlayer() {
    return mDirector->getGlowSoundPlayer() == this;
}

/**
 * @brief Full glow off state: play the glow off anim, then go back to waiting.
 */
void BlockHardLaserOnly::exeFullGlowOff() {
    if (al::isFirstStep(this)) {
        al::tryStartMclAnimIfNotPlaying(this, "GlowOff");
        al::stopAllSeFromUser(this, 0);
        return;
    }

    if (isGlowAnimDone() &&
        al::isGreaterStep(this, static_cast<s32>(al::getMclAnimFrameMax(this, "GlowOff") + 1.0f))) {
        al::startNerveAction(this, "Wait");
    }
}

/**
 * @brief Get the id of the file (island) this block belongs to.
 * @return The file id.
 */
s32 BlockHardLaserOnly::getFileID() {
    return mFileID;
}

/**
 * @brief Check whether this block can be broken by a chain break.
 * @return True if chain breaks apply to this block.
 */
bool BlockHardLaserOnly::canChainBreak() {
    return mCanChainBreak;
}

/**
 * @brief Get the max distance at which a neighbouring block's break chains to this one.
 * @return The max chain break distance.
 */
f32 BlockHardLaserOnly::getMaxChainBreakDistance() const {
    return mMaxChainBreakDistance;
}

/**
 * @brief Check whether the block is breaking.
 * @param isCheckDelay Whether to only count it once the break delay has passed.
 * @return True if the block is breaking.
 */
bool BlockHardLaserOnly::isBreaking(bool isCheckDelay) {
    if (al::isNerve(this, NrvBlockHardLaserOnly.Breaking.data())) {
        if (!isCheckDelay) {
            return true;
        }

        if (al::isGreaterEqualStep(this, mBreakDelay)) {
            return true;
        }
    }

    return false;
}
