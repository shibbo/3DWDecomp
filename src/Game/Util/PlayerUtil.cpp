#include "Util/PlayerUtil.hpp"
#include "Scene/SceneObjID.hpp"
#include "Util/ProjectInterfaceUtil.hpp"

#include <attributes.h>
#include <common/aglShaderLocation.h>
#include <g3d/aglShaderUtilG3D.h>
#include <nn/g3d/g3d_ResShader.h>
#include <gfx/seadGraphics.h>
#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <math/seadQuat.h>
#include <prim/seadSafeString.h>

#include "Library/Collision/Collider.hpp"
#include "Library/Collision/CollisionPartsKeeperUtil.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/LiveActorFlag.hpp"
#include "Library/LiveActor/SubActorKeeper.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Player/PlayerHolder.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/DeferredRendering/PeripheryRendering.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "Library/System/SystemKit.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Layout/PlayerEntryFunction.hpp"
#include "MapObj/Fury/CloudBonusWatcher.hpp"
#include "MapObj/TractorBubble.hpp"
#include "Player/BindPriority.hpp"
#include "Player/Giga/PlayerActionObserver.hpp"
#include "Player/Giga/PlayerActionGraph.hpp"
#include "Player/Giga/PlayerActionGraphRestarter.hpp"
#include "Player/IUsePlayerCeilingCheck.hpp"
#include "Player/IUsePlayerCharaQuery.hpp"
#include "Player/IUsePlayerCollision.hpp"
#include "Player/IUsePlayerDamageInvalidCheck.hpp"
#include "Player/IUsePlayerDashChecker.hpp"
#include "Player/IUsePlayerLifeControl.hpp"
#include "Player/IUsePlayerModelVisibility.hpp"
#include "Player/IUsePlayerPropellerJumpPhase.hpp"
#include "Player/IUsePlayerPuppet.hpp"
#include "Player/Normal/PlayerActionTypeFunc.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Player/Normal/PlayerAliveWatcher.hpp"
#include "Player/Normal/PlayerAmiiboDirector.hpp"
#include "Player/Normal/PlayerAudio.hpp"
#include "Player/Normal/PlayerConstParam.hpp"
#include "Player/Normal/PlayerEquipmentDirector.hpp"
#include "Player/Normal/PlayerFigureDirector.hpp"
#include "Player/Normal/PlayerGiantDirector.hpp"
#include "Player/Normal/PlayerGigaDirector.hpp"
#include "Player/Normal/PlayerGroupSceneObj.hpp"
#include "Player/Normal/PlayerInput.hpp"
#include "Player/Normal/PlayerInvincibleState.hpp"
#include "Player/Normal/PlayerKiller.hpp"
#include "Player/Normal/PlayerKoopaJr.hpp"
#include "Player/Normal/PlayerModelHolder.hpp"
#include "Player/Normal/PlayerProperty.hpp"
#include "Player/Player.hpp"
#include "Player/PlayerActionFunc.hpp"
#include "Player/PlayerDef.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionPartsTriangle.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Raidon/RaidonSurf.hpp"
#include "Scene/PlayerStocker.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {

/// Layout of the uniform block of the invincibility look: one color.
const al::UniformBlockLayout cInvincibleUboLayout = {0, agl::UniformBlock::cType_Vec4, 1};

/// Value of the first wall code in the collision attributes.
constexpr s32 cWallCodeOffset = 5;

/// Value of the first camera code in the collision attributes.
constexpr s32 cCameraCodeOffset = 7;

/// Japanese debug name of the player actors.
const char* const cPlayerActorName = "プレイヤー";

/// Japanese debug name of TractorBubble ("pull-back bubble").
const char* const cTractorBubbleName = "引き戻し泡";

/// Japanese debug name of the skate shoes.
const char* const cSkateShoesName = "スケート靴";

/// Equipment type of headgear (propeller box, cannon box, ...).
constexpr EPlayerEquipmentType cEquipmentTypeHeadgear = static_cast<EPlayerEquipmentType>(0);

/// Equipment type of the crown.
constexpr EPlayerEquipmentType cEquipmentTypeCrown = static_cast<EPlayerEquipmentType>(1);

/// Equipment action that makes enemies disregard the player.
constexpr EPlayerEquipmentAction cEquipmentActionDisregard = static_cast<EPlayerEquipmentAction>(2);

/// Equipment is released normally.
constexpr PlayerReleaseEquipmentGoalType cReleaseGoalTypeNormal =
    static_cast<PlayerReleaseEquipmentGoalType>(0);

/// Equipment is released at a goal pole.
constexpr PlayerReleaseEquipmentGoalType cReleaseGoalTypeGoalPole =
    static_cast<PlayerReleaseEquipmentGoalType>(1);

/// Equipment is released at a gate keeper.
constexpr PlayerReleaseEquipmentGoalType cReleaseGoalTypeGateKeeper =
    static_cast<PlayerReleaseEquipmentGoalType>(2);

/// Equipment is released without effects.
constexpr PlayerReleaseEquipmentGoalType cReleaseGoalTypeSilent =
    static_cast<PlayerReleaseEquipmentGoalType>(3);

/**
 * @brief View a player's actor as a PlayerActor.
 * @param pActor Actor of a player.
 * @return The player actor.
 */
inline const PlayerActor* toPlayerActor(const al::LiveActor* pActor) {
    return static_cast<const PlayerActor*>(pActor);
}

/**
 * @brief View a player's actor as a PlayerActor.
 * @param pActor Actor of a player.
 * @return The player actor.
 */
inline PlayerActor* toPlayerActor(al::LiveActor* pActor) {
    return static_cast<PlayerActor*>(pActor);
}

/**
 * @brief Get the player actor owning a sensor.
 * @param pSensor Sensor of the player.
 * @return The player actor.
 */
inline PlayerActor* getSensorPlayerActor(const al::HitSensor* pSensor) {
    // The host is looked up twice; the first lookup only served a check that is compiled out.
    al::getSensorHost(pSensor);
    return static_cast<PlayerActor*>(al::getSensorHost(pSensor));
}

/**
 * @brief Get the action observer of a player.
 * @param pActor Actor of the player.
 * @return The player's action observer.
 */
inline PlayerActionObserver* getActionObserver(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getActionObserver();
}

/**
 * @brief Get the action logic of a player.
 * @param pActor Actor of the player.
 * @return The player's action logic.
 */
inline Player* getPlayer(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getPlayer();
}

/**
 * @brief Whether a player is dying.
 * @param pActor Actor of the player, may be nullptr.
 * @return True if the player exists and is dying.
 */
inline bool isDying(const al::LiveActor* pActor) {
    return pActor != nullptr && getActionObserver(pActor)->isDead();
}

/**
 * @brief Whether every living player is bound.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that must bind the players, or nullptr for any.
 * @return True if all living players are bound (by that sensor).
 */
ALWAYS_INLINE bool isAllLivingPlayerBinded(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player)) {
            continue;
        }

        if (!getActionObserver(player)->isInBind()) {
            return false;
        }

        if (pSensor != nullptr && toPlayerActor(player)->getBindSensor() != pSensor) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether a player is carried by a TractorBubble.
 * @param pActor Actor of the player.
 * @return True if the player is bound by a TractorBubble that holds it.
 */
inline bool isInTractorBubble(const al::LiveActor* pActor) {
    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    if (bindSensor == nullptr) {
        return false;
    }

    al::LiveActor* binder = al::getSensorHost(bindSensor);
    if (binder == nullptr) {
        return false;
    }

    if (!(sead::SafeString(cTractorBubbleName) == binder->getName())) {
        return false;
    }

    return static_cast<TractorBubble*>(binder)->isPlayerInBubble();
}

/**
 * @brief Whether a player is bound by an actor with a given name.
 * @param pActor Actor of the player.
 * @param pName Name of the binding actor.
 * @return True if the player's binder has that name.
 */
inline bool isBindedByName(const al::LiveActor* pActor, const char* pName) {
    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    if (bindSensor == nullptr) {
        return false;
    }

    al::LiveActor* binder = al::getSensorHost(bindSensor);
    if (binder == nullptr) {
        return false;
    }

    return sead::SafeString(pName) == binder->getName();
}

/**
 * @brief Whether a player stands or swims on the surfing Plessie.
 * @param pActor Actor used to reach the scene objects.
 * @param pPlayer Actor of the player.
 * @return True if Plessie exists and is on the ground or in water.
 */
inline bool isRaidonSurfOnGroundOrWater(const al::LiveActor* pActor, const PlayerActor* pPlayer) {
    if (pPlayer->isRaidonExist()) {
        auto* raidon = al::tryGetSceneObj<RaidonSurf>(pActor, SceneObjID_RaidonSurf);
        if (raidon != nullptr && raidon->isOnGroundOrWaterRaidon()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Get the group of the players.
 * @param pActor Actor used to reach the scene objects.
 * @return The player group.
 */
inline PlayerGroup* getPlayerGroup(const al::LiveActor* pActor) {
    return al::getSceneObj<PlayerGroupSceneObj>(pActor, SceneObjID_PlayerGroup)->getPlayerGroup();
}

/**
 * @brief Change the figure of a player.
 * @param pPlayer The player.
 * @param figure Figure to change to.
 */
inline void changeFigure(PlayerActor* pPlayer, EPlayerFigure figure) {
    getPlayerGroup(pPlayer)->changeFigure(pPlayer, figure);
}

/**
 * @brief Change the figure of a player at once.
 * @param pPlayer The player.
 * @param figure Figure to change to.
 */
inline void changeFigureForce(PlayerActor* pPlayer, EPlayerFigure figure) {
    getPlayerGroup(pPlayer)->changeFigureForce(pPlayer, figure);
}

/**
 * @brief Whether a player has a figure.
 * @param pActor Actor of the player.
 * @param figure Figure to check.
 * @return True if the player has that figure.
 */
inline bool isFigure(const al::LiveActor* pActor, EPlayerFigure::ValueType figure) {
    return getPlayer(pActor)->getFigureDirector()->getFigure() == figure;
}

/**
 * @brief Physical state of the player (or Bowser Jr.) of a sensor.
 * @param pSensor Sensor of the player.
 * @return The property of the player.
 */
inline PlayerProperty* getSensorProperty(const al::HitSensor* pSensor) {
    if (al::isSensorKoopaJr(pSensor) || al::isSensorHostName(pSensor, "KoopaJr")) {
        return static_cast<PlayerKoopaJr*>(al::getSensorHost(pSensor))->getProperty();
    }

    return getSensorPlayerActor(pSensor)->getProperty();
}

/**
 * @brief Horizontal speed in a player's property.
 * @param pProperty Property of the player.
 * @return The length of the velocity on the XZ plane.
 */
inline f32 calcSpeedH(const PlayerProperty* pProperty) {
    const sead::Vector3f& velocity = pProperty->getVelocity();
    return sead::Mathf::sqrt(velocity.x * velocity.x + velocity.z * velocity.z);
}

/**
 * @brief Turn a player toward its stick input.
 * @param pPlayer The player.
 */
inline void setForwardToInput(PlayerActor* pPlayer) {
    PlayerInput* input = pPlayer->getInput();
    PlayerProperty* property = pPlayer->getProperty();
    if (!input->isStickOn()) {
        return;
    }

    sead::Vector3f front = input->getMoveVec();
    if (!sead::Mathf::equalsEpsilon(front.length(), 1.0f)) {
        al::normalizeOrDirZ(&front);
    }

    property->setFrontVec(front);
}

/**
 * @brief Take all equipment off the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param goalType Where the equipment goes.
 */
inline void removeAllEquip(const al::HitSensor* pSensor, PlayerReleaseEquipmentGoalType goalType) {
    getSensorPlayerActor(pSensor)->getModelHolder()->invalidateMash();
    getPlayer(getSensorPlayerActor(pSensor))->getEquipmentDirector()->releaseEquipmentForce(goalType);
}

/**
 * @brief Take all equipment off a player.
 * @param pActor Actor of the player.
 * @param goalType Where the equipment goes.
 */
inline void removeAllEquip(const al::LiveActor* pActor, PlayerReleaseEquipmentGoalType goalType) {
    toPlayerActor(pActor)->getModelHolder()->invalidateMash();
    getPlayer(pActor)->getEquipmentDirector()->releaseEquipmentForce(goalType);
}

/**
 * @brief Whether a player takes part in the game.
 * @param pActor Actor of the player, may be nullptr.
 * @return True if the player exists, lives and is not in a bubble.
 */
inline bool isActivePlayer(const al::LiveActor* pActor) {
    return pActor != nullptr && !al::isDead(pActor) && !rc::isPlayerDeadOrBubble(pActor);
}

/**
 * @brief Keep an actor if it is nearer than the nearest one so far.
 * @param pNearest Nearest actor so far.
 * @param pMinDistanceSq Squared distance of the nearest actor so far.
 * @param pActor Candidate actor.
 * @param rTrans Position to measure from.
 */
inline void updateNearest(al::LiveActor** pNearest, f32* pMinDistanceSq, al::LiveActor* pActor,
                          const sead::Vector3f& rTrans) {
    f32 distanceSq = (al::getTrans(pActor) - rTrans).squaredLength();
    if (distanceSq < *pMinDistanceSq) {
        *pNearest = pActor;
    }

    *pMinDistanceSq = sead::Mathf::min(distanceSq, *pMinDistanceSq);
}

/**
 * @brief Whether an actor is within a cylinder around another actor.
 * @param pActor Actor at the center of the cylinder.
 * @param pTarget Actor to check.
 * @param radius Radius of the cylinder.
 * @param bottom Lowest height relative to the center actor.
 * @param top Highest height relative to the center actor.
 * @return True if the target is inside the cylinder.
 */
inline bool isInCylinder(const al::LiveActor* pActor, const al::LiveActor* pTarget, f32 radius,
                         f32 bottom, f32 top) {
    sead::Vector3f trans = al::getTrans(pTarget);
    f32 distanceH = al::calcDistanceH(pActor, trans);
    f32 height = al::calcHeight(pActor, trans);
    return height >= bottom && distanceH < radius && height <= top;
}

/**
 * @brief Read a code from the collision attributes of a triangle.
 * @param rTriangle The triangle.
 * @param pKey Name of the code.
 * @param offset Value of the first code of that kind.
 * @return The code, or 0 if the triangle has none.
 */
inline s32 getTriangleCode(const al::Triangle& rTriangle, const char* pKey, s32 offset) {
    al::ByamlIter attributes;
    rTriangle.getAttributes(&attributes);
    if (!attributes.isValid()) {
        return 0;
    }

    al::ByamlIter codeIter;
    if (attributes.tryGetIterByKey(&codeIter, pKey)) {
        s32 code;
        if (codeIter.tryGetIntByIndex(&code, 1)) {
            return code - offset;
        }
    }

    return 0;
}

/**
 * @brief Whether an actor touches a floor code on the ground, a wall or the ceiling.
 * @param pActor The actor.
 * @param pCode The floor code.
 * @return True if a touched triangle has the code.
 */
inline bool isCollidedFloorCode(const al::LiveActor* pActor, const char* pCode) {
    if (al::isCollidedGround(pActor) &&
        al::isFloorCode(pCode, al::getActorCollider(pActor)->mFloor.mTriangle)) {
        return true;
    }

    if (al::isCollidedWall(pActor) &&
        al::isFloorCode(pCode, al::getActorCollider(pActor)->mWall.mTriangle)) {
        return true;
    }

    if (al::isCollidedCeiling(pActor)) {
        return al::isFloorCode(pCode, al::getActorCollider(pActor)->mCeiling.mTriangle);
    }

    return false;
}

/**
 * @brief Whether a sub actor only draws parts of its parent (silhouette, shadow, ...).
 * @param pInfo Info of the sub actor.
 * @return True if the sub actor is a drawing part.
 */
inline bool isDrawPartsSubActor(const al::SubActorInfo* pInfo) {
    return (pInfo->mSyncType & 8) != 0;
}

/**
 * @brief Set the invincibility color of an actor's model.
 * @param pActor Actor to color.
 * @param rColor Color to set.
 */
inline void setInvincibleColorImpl(al::LiveActor* pActor, const sead::Color4f& rColor) {
    al::UniformBlock* uniformBlock = al::getModelUniformBlock(pActor, "cInvincible");
    uniformBlock->setData(0, &rColor, 0, 1);
    uniformBlock->flushCurrentBuffer();
}

/**
 * @brief Append the figure specific suffix of a giga hit reaction name.
 * @param pActor Actor of the player.
 * @param pName Hit reaction name to append to.
 */
void appendGigaHitReactionSuffix(const al::LiveActor* pActor, sead::BufferedSafeString* pName) {
    switch (getPlayer(pActor)->getFigureDirector()->getFigure()) {
    case EPlayerFigure::Climb:
    case EPlayerFigure::ClimbGiga:
        pName->append("Climb");
        break;
    case EPlayerFigure::Mini:
        pName->append("Short");
        break;
    default:
        break;
    }
}

} // namespace

/// Whether the old player parameters are used (debug toggle).
static bool sIsUsingOldPlayerParams = false;

/// Whether holding uses the input instead of the key config (debug toggle).
static bool sIsUsingHoldInput = true;

/// The main player actor.
static al::LiveActor* sMainPlayerActor = nullptr;

/// Character names of every character type.
static const char* const sPlayerCharacterNameTrue[] = {
    "Mario",          "Luigi",
    "Peach",          "Kinopio",
    "Rosetta",        "KinopioBrigade",
    "KinopioBrigadeMember", "KinopioBrigadeMember",
    "KinopioBrigadeMember",
};

/// Bubble material animation names of every character type.
static const char* const sPlayerCharacterBubbleMatAnimName[] = {
    "Mario",          "Luigi",
    "Peach",          "Kinopio",
    "Rosetta",        "KinopioBrigade",
    "KinopioBrigadeMember", "KinopioBrigadeMember1",
    "KinopioBrigadeMember2",
};

/// Names of the floor codes.
static const char* const sFloorCodeName[] = {
    "Ground", "Needle", "DamageFire", "Poison", "Slide", "Slip", "NoSlip", "Skate",
};

/// Names of the wall codes.
static const char* const sWallCodeName[] = {
    "Wall",
    "NoAction",
};

/// Names of the camera codes.
static const char* const sCameraCodeName[] = {
    "NoThrough",
    "Through",
};

/// Names of the material codes.
static const char* const sMaterialCodeName[] = {
    "null",     "Soil",    "Lawn",  "FallenLeavesMetal", "MetalHeavy", "Stone", "StoneWet",
    "Sand",     "WoodThick", "WoodThin", "Wood", "WoodWet", "Snow", "Ice", "Glass", "Marble",
    "Carpet",   "Cloth",   "Cloud", "InWater",
};

namespace rc {

/**
 * @brief Name of the player parameter set in use.
 * @return "OLD" or "NEW".
 */
const char* getPlayerParamsName() {
    return sIsUsingOldPlayerParams ? "OLD" : "NEW";
}

/**
 * @brief Switch between the old and the new player parameters.
 * @return Whether the old parameters are now used.
 */
bool toggleUsingOldPlayerParams() {
    sIsUsingOldPlayerParams = !sIsUsingOldPlayerParams;
    return sIsUsingOldPlayerParams;
}

/**
 * @brief Choose the player parameter set.
 * @param isOld Whether to use the old parameters.
 */
void setUsingOldPlayerParams(bool isOld) {
    sIsUsingOldPlayerParams = isOld;
}

/**
 * @brief Whether the old player parameters are used.
 * @return True if the old parameters are used.
 */
bool isUsingOldPlayerParams() {
    return sIsUsingOldPlayerParams;
}

/**
 * @brief Name of the source used for holding.
 * @return "mInput" or "mKeyConfig".
 */
const char* getPlayerInputName() {
    return sIsUsingHoldInput ? "mInput" : "mKeyConfig";
}

/**
 * @brief Choose the source used for holding.
 * @param isUseInput Whether to use the input instead of the key config.
 */
void toggleUsingHoldInput(bool isUseInput) {
    sIsUsingHoldInput = isUseInput;
}

/**
 * @brief Whether an actor is a real player actor.
 * @param pActor Actor to check.
 * @return True if the actor is named like the player actors.
 */
bool isReallyPlayerActor(const al::LiveActor* pActor) {
    return al::isEqualString(pActor->getName(), cPlayerActorName);
}

/**
 * @brief Whether a sensor belongs to a real player actor.
 * @param pSensor Sensor to check.
 * @return True if the sensor's host is named like the player actors.
 */
bool isReallyPlayerActor(const al::HitSensor* pSensor) {
    return al::isEqualString(al::getSensorHost(pSensor)->getName(), cPlayerActorName);
}

/**
 * @brief Start a giga hit reaction, adjusted to the player's figure.
 * @param pActor Actor of the player.
 * @param pName Base name of the hit reaction.
 */
void doGigaHitReaction(const al::LiveActor* pActor, const char* pName) {
    al::StringTmp<128> name(pName);
    appendGigaHitReactionSuffix(pActor, &name);
    al::startHitReaction(pActor, name.cstr());
}

/**
 * @brief Start a giga hit reaction at a position, adjusted to the player's figure.
 * @param pActor Actor of the player.
 * @param pName Base name of the hit reaction.
 * @param rPos Position of the hit effect.
 */
void doGigaHitReaction(const al::LiveActor* pActor, const char* pName, const sead::Vector3f& rPos) {
    al::StringTmp<128> name(pName);
    appendGigaHitReactionSuffix(pActor, &name);
    al::startHitReactionHitEffect(pActor, name.cstr(), rPos);
}

/**
 * @brief Number of selectable player characters.
 * @return Always 5.
 */
s32 getPlayerCharacterNumMax() {
    return 5;
}

/**
 * @brief Number of all character types, including Captain Toad's brigade.
 * @return Always 9.
 */
s32 getPlayerCharacterNumMaxTrue() {
    return 9;
}

/**
 * @brief Name of a selectable player character.
 * @param characterType Character type.
 * @return The character name.
 */
const char* getPlayerCharacterName(s32 characterType) {
    return GameDataConst::getPlayerCharacterName(characterType);
}

/**
 * @brief Name of a player's character.
 * @param pActor Actor of the player.
 * @return The character name.
 */
const char* getPlayerCharacterName(const al::LiveActor* pActor) {
    return GameDataConst::getPlayerCharacterName(getPlayer(pActor)->getCharaQuery()->getChara());
}

/**
 * @brief Character type of a player.
 * @param pActor Actor of the player.
 * @return The character type.
 */
s32 getPlayerCharaType(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getCharaQuery()->getChara();
}

/**
 * @brief Name of a character type.
 * @param characterType Character type.
 * @return The character name.
 */
const char* getPlayerCharacterNameTrue(s32 characterType) {
    return sPlayerCharacterNameTrue[characterType];
}

/**
 * @brief Name of a player's character type.
 * @param pActor Actor of the player.
 * @return The character name.
 */
const char* getPlayerCharacterNameTrue(const al::LiveActor* pActor) {
    return sPlayerCharacterNameTrue[getPlayer(pActor)->getCharaQuery()->getChara()];
}

/**
 * @brief Bubble material animation of a character type.
 * @param characterType Character type.
 * @return The material animation name.
 */
const char* getPlayerCharacterBubbleMatAnimName(s32 characterType) {
    return sPlayerCharacterBubbleMatAnimName[characterType];
}

/**
 * @brief Bubble material animation of a player's character.
 * @param pActor Actor of the player.
 * @return The material animation name.
 */
const char* getPlayerCharacterBubbleMatAnimName(const al::LiveActor* pActor) {
    return sPlayerCharacterBubbleMatAnimName[getPlayer(pActor)->getCharaQuery()->getChara()];
}

/**
 * @brief Number of player figures (power-up forms).
 * @return Always 10.
 */
s32 getPlayerFigureNumMax() {
    return 10;
}

/**
 * @brief Gets the default player transformation for a new or recovered player.
 * @return The default player figure type identifier.
 */
s32 getPlayerFigureTypeDefault() {
    return 0;
}

/**
 * @brief Ignore the input of every player for a while.
 * @param pActor Actor used to reach the players.
 * @param frame Number of frames to ignore the input for.
 */
void invalidatePlayerInput(const al::LiveActor* pActor, s32 frame) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        toPlayerActor(al::getPlayerActor(pActor, i))->getInput()->invalidateFrame(frame);
    }
}

/**
 * @brief Whether any living player pressed jump.
 * @param pActor Actor used to reach the players.
 * @return True if a player that is neither dead nor in a bubble triggered a jump.
 */
bool isAnyPlayerJumpTrigOn(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (getActionObserver(player)->isJumpTrig()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether a player is dead or carried in a bubble.
 * @param pActor Actor of the player.
 * @return True if the player is dead, dying or in a TractorBubble.
 */
bool isPlayerDeadOrBubble(const al::LiveActor* pActor) {
    return al::isDead(pActor) || getActionObserver(pActor)->isDead() || isInTractorBubble(pActor);
}

/**
 * @brief Whether a player triggered a jump.
 * @param pActor Actor of the player.
 * @return True if the player triggered a jump.
 */
bool isPlayerJumpTrigOn(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isJumpTrig();
}

/**
 * @brief Whether a player triggered a jump.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player triggered a jump.
 */
bool isPlayerJumpTrigOn(const al::LiveActor* pActor, s32 index) {
    return isPlayerJumpTrigOn(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether a player pressed the jump input.
 * @param pActor Actor of the player.
 * @param isButton Whether to check the button itself instead of the arranged input.
 * @return True if jump was triggered.
 */
bool isPlayerInputJumpTrigOn(const al::LiveActor* pActor, bool isButton) {
    const PlayerInput* input = toPlayerActor(pActor)->getInput();
    if (isButton) {
        return input->isJumpButtonTrigOn();
    }

    return input->isJumpTrigOn();
}

/**
 * @brief Hide every player carried in a bubble along with its bubble.
 * @param pActor Actor used to reach the players.
 */
void requestHideBubbledPlayers(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || !isInTractorBubble(player)) {
            continue;
        }

        IUsePlayerPuppet* puppet = toPlayerActor(player)->getPlayerPuppet();
        puppet->hide();
        puppet->hideShadow();
        puppet->hideSilhouette();

        al::LiveActor* bubble = al::getSensorHost(toPlayerActor(player)->getBindSensor());
        al::hideModelIfShow(bubble);
        al::hideSilhouetteModelIfShow(bubble);
        al::hideShadow(bubble);
    }
}

/**
 * @brief Whether a player is carried in a bubble.
 * @param pActor Actor of the player.
 * @return True if the player is in a TractorBubble.
 */
bool isPlayerBubble(const al::LiveActor* pActor) {
    return isInTractorBubble(pActor);
}

/**
 * @brief Request every living player to be bound by a sensor.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor binding the players.
 */
void requestBindAllPlayer(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));

    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!al::isDead(player)) {
            toPlayerActor(player)->requestBind(pSensor, 0.0f, 0);
        }
    }
}

/**
 * @brief Keep the players from being revived in bubbles.
 * @param pActor Actor requesting it.
 */
void setDisableReviveBubbleForAllPlayer(al::LiveActor* pActor) {
    PlayerAliveWatcher::getPlayerAliveWatcher(pActor)->setDisableReviveBubble(pActor);
}

/**
 * @brief Request every living player outside a bubble to be bound by a sensor.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor binding the players.
 */
void requestBindAllPlayerButBubble(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));

    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!al::isDead(player) && !isInTractorBubble(player)) {
            toPlayerActor(player)->requestBind(pSensor, 0.0f, 0);
        }
    }
}

/**
 * @brief Request every living player outside a bubble but one to be bound by a sensor.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor binding the players.
 * @param pExcludeActor Player to leave alone.
 */
void requestBindAllPlayerButBubbleExcludeActor(const al::LiveActor* pActor, al::HitSensor* pSensor,
                                               const al::LiveActor* pExcludeActor) {
    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));

    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!al::isDead(player) && !isInTractorBubble(player) && player != pExcludeActor) {
            toPlayerActor(player)->requestBind(pSensor, 0.0f, 0);
        }
    }
}

/**
 * @brief Request every living player to be bound by a sensor, bubbles may still revive them.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor binding the players.
 */
void requestBindAllPlayerAcceptReviveBubble(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!al::isDead(player)) {
            toPlayerActor(player)->requestBind(pSensor, 0.0f, 0);
        }
    }
}

/**
 * @brief Cancel the bind requests of a sensor for every player.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that requested the binds.
 */
void cancelRequestBindAllPlayer(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        toPlayerActor(al::getPlayerActor(pActor, i))->cancelRequestBind(pSensor);
    }
}

/**
 * @brief Whether every living player is bound.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that must bind the players, or nullptr for any.
 * @return True if all living players are bound (by that sensor).
 */
bool isAllPlayerBinded(const al::LiveActor* pActor, al::HitSensor* pSensor) {
    return isAllLivingPlayerBinded(pActor, pSensor);
}

/**
 * @brief Whether every living player is bound by an actor.
 * @param pActor Actor that must bind the players.
 * @return True if all living players are bound by that actor.
 */
bool isAllPlayerBinded(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player)) {
            continue;
        }

        if (!getActionObserver(player)->isInBind()) {
            return false;
        }

        al::HitSensor* bindSensor = toPlayerActor(player)->getBindSensor();
        if (bindSensor == nullptr || al::getSensorHost(bindSensor) != pActor) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Once every living player is bound, keep them from being revived in bubbles.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that must bind the players.
 * @return True if all living players are bound.
 */
bool checkAllPlayerBindedAndDisableReviveBubble(const al::LiveActor* pActor,
                                                al::HitSensor* pSensor) {
    if (!isAllLivingPlayerBinded(pActor, pSensor)) {
        return false;
    }

    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));
    return true;
}

/**
 * @brief Once every living player is bound or in a bubble, keep them from being revived in bubbles.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that must bind the players.
 * @return True if all living players outside bubbles are bound.
 */
bool checkAllPlayerBindedOrBubbleAndDisableReviveBubble(const al::LiveActor* pActor,
                                                        al::HitSensor* pSensor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || isInTractorBubble(player) || isDying(player)) {
            continue;
        }

        if (!getActionObserver(player)->isInBind()) {
            return false;
        }

        if (pSensor != nullptr && toPlayerActor(player)->getBindSensor() != pSensor) {
            return false;
        }
    }

    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));
    return true;
}

/**
 * @brief Once every living player but one is bound or in a bubble, keep them from being revived
 * in bubbles.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that must bind the players.
 * @param pExcludeActor Player to leave out of the check.
 * @return True if all living players outside bubbles are bound.
 */
bool checkAllPlayerBindedOrBubbleAndDisableReviveBubbleExcludeActor(
    const al::LiveActor* pActor, al::HitSensor* pSensor, const al::LiveActor* pExcludeActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || isInTractorBubble(player)) {
            continue;
        }

        bool isPlayerDying = isDying(player);
        if (player == pExcludeActor || isPlayerDying) {
            continue;
        }

        if (!getActionObserver(player)->isInBind()) {
            return false;
        }

        if (pSensor != nullptr && toPlayerActor(player)->getBindSensor() != pSensor) {
            return false;
        }
    }

    setDisableReviveBubbleForAllPlayer(al::getSensorHost(pSensor));
    return true;
}

/**
 * @brief Hide the model of every bound player that is dying.
 * @param pActor Actor used to reach the players.
 */
void hideAllDyingPlayers(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || isInTractorBubble(player)) {
            continue;
        }

        if (getActionObserver(player)->isInBind() && isDying(player) &&
            !toPlayerActor(player)->getModelVisibility()->isHidden()) {
            toPlayerActor(player)->getModelVisibility()->hide();
        }
    }
}

/**
 * @brief Whether a player is invincible.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player is invincible.
 */
bool isPlayerInvincible(const al::LiveActor* pActor, s32 index) {
    return isPlayerInvincible(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether the player of a sensor is invincible.
 * @param pActor Actor asking.
 * @param pSensor Sensor of the player.
 * @return True if the player is invincible.
 */
bool isPlayerInvincible(const al::LiveActor* pActor, const al::HitSensor* pSensor) {
    return isPlayerInvincible(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is invincible.
 * @param pActor Actor of the player.
 * @return True if the player is invincible.
 */
bool isPlayerInvincible(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getInvincibleState()->isInvincible();
}

/**
 * @brief Pause the invincibility of every player.
 * @param pActor Actor used to reach the players.
 * @param isPauseBgm Whether to pause the invincibility music too.
 */
void tryPauseAllPlayerInvincible(const al::LiveActor* pActor, bool isPauseBgm) {
    tryPauseAllPlayerInvincible(pActor->getSceneInfo()->playerHolder, isPauseBgm);
}

/**
 * @brief Pause the invincibility of every player.
 * @param pHolder Holder of the players.
 * @param isPauseBgm Whether to pause the invincibility music too.
 */
void tryPauseAllPlayerInvincible(al::PlayerHolder* pHolder, bool isPauseBgm) {
    for (s32 i = 0; i < pHolder->getPlayerNum(); i++) {
        al::LiveActor* player = pHolder->tryGetPlayer(i);
        if (player != nullptr) {
            toPlayerActor(player)->pauseInvincible(isPauseBgm);
        }
    }
}

/**
 * @brief Pause the invincibility of a player.
 * @param pActor Actor of the player.
 * @param isPauseBgm Whether to pause the invincibility music too.
 */
void tryPausePlayerInvincible(al::LiveActor* pActor, bool isPauseBgm) {
    toPlayerActor(pActor)->pauseInvincible(isPauseBgm);
}

/**
 * @brief Resume the invincibility of every player.
 * @param pActor Actor used to reach the players.
 * @param isResumeBgm Whether to resume the invincibility music too.
 */
void tryResumeAllPlayerInvincible(const al::LiveActor* pActor, bool isResumeBgm) {
    tryResumeAllPlayerInvincible(pActor->getSceneInfo()->playerHolder, isResumeBgm);
}

/**
 * @brief Resume the invincibility of every player.
 * @param pHolder Holder of the players.
 * @param isResumeBgm Whether to resume the invincibility music too.
 */
void tryResumeAllPlayerInvincible(al::PlayerHolder* pHolder, bool isResumeBgm) {
    for (s32 i = 0; i < pHolder->getPlayerNum(); i++) {
        al::LiveActor* player = pHolder->tryGetPlayer(i);
        if (player != nullptr) {
            toPlayerActor(player)->resumeInvincible(isResumeBgm);
        }
    }
}

/**
 * @brief Resume the invincibility of a player.
 * @param pActor Actor of the player.
 * @param isResumeBgm Whether to resume the invincibility music too.
 */
void tryResumePlayerInvincible(al::LiveActor* pActor, bool isResumeBgm) {
    toPlayerActor(pActor)->resumeInvincible(isResumeBgm);
}

/**
 * @brief Turn the invincibility music of a player on or off.
 * @param pSensor Sensor of the player.
 * @param isOn Whether the music plays.
 */
void trySetPlayerInvicibleBgmState(al::HitSensor* pSensor, bool isOn) {
    getSensorPlayerActor(pSensor)->setInvicibleBgmState(isOn);
}

/**
 * @brief Whether every player is dead or carried in a bubble.
 * @param pActor Actor used to reach the players.
 * @return True if no player is alive outside a bubble.
 */
bool isAllPlayerDeadOrBubble(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player != nullptr && !isPlayerDeadOrBubble(player)) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether every living player is on the ground.
 * @param pActor Actor used to reach the players.
 * @return True if all living players are on the ground.
 */
bool isAllPlayerOnGround(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!al::isDead(player) && !getActionObserver(player)->isOnGround()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether a player is on the ground.
 * @param pActor Actor of the player.
 * @return True if the player is on the ground.
 */
bool isPlayerOnGround(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isOnGround();
}

/**
 * @brief Whether every living player is on the ground or in a bubble.
 * @param pActor Actor used to reach the players.
 * @return True if all living players are on the ground or in bubbles.
 */
bool isAllPlayerOnGroundOrBubble(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || isInTractorBubble(player)) {
            continue;
        }

        if (!getActionObserver(player)->isOnGround()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether every living player is on the ground or in water.
 * @param pActor Actor used to reach the players.
 * @return The result for the first living player that is neither on the ground nor in water.
 */
bool isAllPlayerOnGroundOrWater(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player)) {
            continue;
        }

        if (toPlayerActor(player)->isInRouteDokanOrDokan()) {
            return false;
        }

        if (!isPlayerOnGroundOrWater(player)) {
            if (isRaidonSurfOnGroundOrWater(pActor, toPlayerActor(player))) {
                return true;
            }

            return toPlayerActor(player)->isTreeClimbing();
        }
    }

    return true;
}

/**
 * @brief Whether a player is in water.
 * @param pActor Actor of the player.
 * @return True if the player swims under or at the surface.
 */
bool isPlayerInWater(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isWaterAction() ||
           getActionObserver(pActor)->isWaterSurfaceAction();
}

/**
 * @brief Whether every living player is on the ground, in water or in a shell.
 * @param pActor Actor used to reach the players.
 * @return True if all living players stand somewhere.
 */
bool isAllPlayerOnGroundOrWaterOrKoura(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player)) {
            continue;
        }

        if (toPlayerActor(player)->isInRouteDokanOrDokan()) {
            return false;
        }

        if (isPlayerOnGroundOrWater(player) ||
            isRaidonSurfOnGroundOrWater(pActor, toPlayerActor(player)) ||
            toPlayerActor(player)->isTreeClimbing() || isPlayerInKouraOnGround(player)) {
            continue;
        }

        return false;
    }

    return true;
}

/**
 * @brief Whether every player that is neither dead nor in a bubble is on the ground.
 * @param pActor Actor used to reach the players.
 * @return True if all those players are on the ground.
 */
bool isAllPlayerOnGroundNoDeadOrBubble(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isPlayerDeadOrBubble(player) && !getActionObserver(player)->isOnGround()) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Whether any player that is neither dead nor in a bubble is on the ground.
 * @param pActor Actor used to reach the players.
 * @return True if one of those players is on the ground.
 */
bool isAnyPlayerOnGroundNoDeadOrBubble(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isPlayerDeadOrBubble(player) && getActionObserver(player)->isOnGround()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether a player fell into the abyss.
 * @param pActor Actor of the player.
 * @return True if the player is falling into the abyss.
 */
bool isPlayerAbyss(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isAbyss();
}

/**
 * @brief Whether a player is vanishing (dying without a death animation).
 * @param pActor Actor of the player.
 * @return True if the player is vanishing.
 */
bool isPlayerVanishDying(al::LiveActor* pActor) {
    return getPlayer(pActor)->getLifeControl()->isVanishDying();
}

/**
 * @brief Make a player vanish (die without a death animation).
 * @param pActor Actor of the player.
 */
void setPlayerVanishDying(al::LiveActor* pActor) {
    getPlayer(pActor)->getLifeControl()->forceVanish();
}

/**
 * @brief Whether a player is in a pipe or a route pipe.
 * @param pActor Actor of the player.
 * @return True if the player is inside a pipe.
 */
bool isPlayerInRouteDokanOrDokan(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isInRouteDokanOrDokan();
}

/**
 * @brief Whether a player is in a pipe that is not a route pipe.
 * @param pActor Actor of the player.
 * @return True if the player is inside a normal pipe.
 */
bool isPlayerInDokanNotRouteDokan(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isInDokanNotRouteDokan();
}

/**
 * @brief Whether a player is in a route pipe, or any pipe in single mode.
 * @param pActor Actor of the player.
 * @return True if the player is inside such a pipe.
 */
bool isPlayerInRouteDokanSM(const al::LiveActor* pActor) {
    if (toPlayerActor(pActor)->isSingleMode()) {
        return toPlayerActor(pActor)->isInRouteDokanOrDokan();
    }

    return pActor->isInRouteDokan();
}

/**
 * @brief Whether a player is bound by a route pipe entrance.
 * @param pActor Actor of the player.
 * @return True if the player's binder is a route pipe entrance.
 */
bool isPlayerInRouteDokan(const al::LiveActor* pActor) {
    return isBindedByName(pActor, "ルート土管出入口");
}

/**
 * @brief Whether the player of a sensor is bound by a route pipe entrance.
 * @param pSensor Sensor of the player.
 * @return True if the player's binder is a route pipe entrance.
 */
bool isPlayerInRouteDokan(const al::HitSensor* pSensor) {
    return rc::isPlayerInRouteDokan(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is bound by a route pipe cannon entrance.
 * @param pActor Actor of the player.
 * @return True if the player's binder is a route pipe cannon entrance.
 */
bool isPlayerInRouteDokanBazooka(const al::LiveActor* pActor) {
    return isBindedByName(pActor, "ルート土管大砲出入口");
}

/**
 * @brief Whether the player of a sensor is bound by a route pipe cannon entrance.
 * @param pSensor Sensor of the player.
 * @return True if the player's binder is a route pipe cannon entrance.
 */
bool isPlayerInRouteDokanBazooka(const al::HitSensor* pSensor) {
    return isPlayerInRouteDokanBazooka(al::getSensorHost(pSensor));
}

/**
 * @brief Whether a player rides Plessie.
 * @param pActor Actor of the player.
 * @return True if the player is bound by Plessie.
 */
bool isPlayerOnRaidon(const al::LiveActor* pActor) {
    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    return bindSensor != nullptr && al::isSensorPlessie(bindSensor);
}

/**
 * @brief Whether the player of a sensor rides Plessie.
 * @param pSensor Sensor of the player, may be nullptr.
 * @return True if the sensor belongs to a player bound by Plessie.
 */
bool isPlayerOnRaidon(const al::HitSensor* pSensor) {
    return pSensor != nullptr && isReallyPlayerActor(pSensor) &&
           isPlayerOnRaidon(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player rides Plessie while she is on the ground.
 * @param pActor Actor of the player.
 * @return True if the player rides Plessie and she is on the ground.
 */
bool isPlayerOnRaidonGround(const al::LiveActor* pActor) {
    if (!isPlayerOnRaidon(pActor)) {
        return false;
    }

    al::LiveActor* raidon = al::getSensorHost(toPlayerActor(pActor)->getBindSensor());
    return static_cast<RaidonBase*>(raidon)->isOnGroundRaidon();
}

/**
 * @brief Whether a player wears skates.
 * @param pActor Actor of the player.
 * @return True if the player is bound by skate shoes.
 */
bool isPlayerOnSkateShoes(const al::LiveActor* pActor) {
    if (!isReallyPlayerActor(pActor)) {
        return false;
    }

    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    return bindSensor != nullptr && al::isSensorHostName(bindSensor, cSkateShoesName);
}

/**
 * @brief Whether a player is on the ground, or skates on the ground.
 * @param pActor Actor of the player.
 * @return True if the player or its skates are on the ground.
 */
bool isPlayerOnSkateShoesGround(const al::LiveActor* pActor) {
    if (isReallyPlayerActor(pActor)) {
        al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
        if (bindSensor != nullptr && al::isSensorHostName(bindSensor, cSkateShoesName) &&
            al::isOnGround(al::getSensorHost(bindSensor), 0, 0.0f)) {
            return true;
        }
    }

    return getActionObserver(pActor)->isOnGround();
}

/**
 * @brief Whether a player cannot take damage.
 * @param pActor Actor of the player.
 * @return True if damage is invalidated.
 */
bool isPlayerDamageInvalid(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getDamageInvalidater()->isInvalid();
}

/**
 * @brief Whether the player of a sensor cannot take damage.
 * @param pSensor Sensor of the player.
 * @return True if damage is invalidated.
 */
bool isPlayerDamageInvalid(const al::HitSensor* pSensor) {
    return isPlayerDamageInvalid(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is in an ink limiter.
 * @param pActor Actor of the player.
 * @return True if the player is in an ink limiter.
 */
bool isPlayerInInkLimiter(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isInInkLimiter();
}

/**
 * @brief Whether the players are in a cloud bonus stage.
 * @param pActor Actor used to reach the scene objects.
 * @return True if a cloud bonus stage is being played.
 */
bool isPlayerInCloudBonus(const al::LiveActor* pActor) {
    auto* watcher = al::getSceneObj<CloudBonusWatcher>(pActor, SceneObjID_CloudBonusWatcher);
    if (watcher == nullptr) {
        return false;
    }

    return watcher->isPlayerInCloudBonusStage();
}

/**
 * @brief Whether a player waits or pivots.
 * @param pActor Actor of the player.
 * @return True if the player waits.
 */
bool isPlayerWait(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isWaitOrPivot();
}

/**
 * @brief Whether a player dashes.
 * @param pActor Actor of the player.
 * @return True if the player dashes.
 */
bool isPlayerDash(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getDashChecker()->isDashing();
}

/**
 * @brief Whether the player of a sensor dashes.
 * @param pSensor Sensor of the player.
 * @return True if the player dashes.
 */
bool isPlayerDash(const al::HitSensor* pSensor) {
    return isPlayerDash(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player dashes fast.
 * @param pActor Actor of the player.
 * @return True if the player dashes fast.
 */
bool isPlayerDashFast(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getDashChecker()->isDashingFast();
}

/**
 * @brief Whether the player of a sensor runs faster than the super dash.
 * @param pSensor Sensor of the player.
 * @return True if the player is faster than the super dash's top speed.
 */
bool isPlayerGreaterSuperDashMaxSpeed(const al::HitSensor* pSensor) {
    return getPlayer(getSensorPlayerActor(pSensor))->getDashChecker()->isGreaterSuperDashMaxSpeed();
}

/**
 * @brief Whether a player crouches.
 * @param pActor Actor of the player.
 * @return True if the player crouches.
 */
bool isPlayerSquat(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isSquatAction();
}

/**
 * @brief Whether the player of a sensor crouches.
 * @param pSensor Sensor of the player.
 * @return True if the player crouches.
 */
bool isPlayerSquat(const al::HitSensor* pSensor) {
    return isPlayerSquat(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player turns around.
 * @param pActor Actor of the player.
 * @return True if the player turns.
 */
bool isPlayerTurn(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isTurn();
}

/**
 * @brief Whether a player is dead or dying.
 * @param pActor Actor of the player.
 * @return True if the player is dead or dying.
 */
bool isPlayerDead(const al::LiveActor* pActor) {
    return al::isDead(pActor) || getActionObserver(pActor)->isDead();
}

/**
 * @brief Whether a bubble may take a player away from its binder.
 * @param pActor Actor of the player.
 * @param pSensor Sensor of the bubble.
 * @return True if the player is unbound or the bubble has the higher bind priority.
 */
bool isPlayerEnableBubble(const al::LiveActor* pActor, const al::HitSensor* pSensor) {
    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    if (bindSensor == nullptr) {
        return true;
    }

    BindPriority priority;
    return priority.isGreater(pSensor, bindSensor);
}

/**
 * @brief Whether a player is dead or carried in a bubble.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player is dead, dying or in a TractorBubble.
 */
bool isPlayerDeadOrBubble(const al::LiveActor* pActor, s32 index) {
    return isPlayerDeadOrBubble(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether a player plays its death animation.
 * @param pActor Actor of the player.
 * @return True if a one-time animation runs while the player is dead or dying.
 */
bool isPlayingDeadAnim(const al::LiveActor* pActor) {
    return al::isSklAnimOneTime(pActor, 0) && !al::isActionEnd(pActor) && isPlayerDead(pActor);
}

/**
 * @brief Cut the death animation of every unbound dying player short.
 * @param pActor Actor used to reach the players.
 */
void forceEndPlayerDeadAnim(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (al::isDead(player) || isInTractorBubble(player)) {
            continue;
        }

        if (!getActionObserver(player)->isInBind() && isDying(player)) {
            toPlayerActor(player)->cancelDeathAnim();
        }
    }
}

/**
 * @brief Whether the player of a sensor is on the ground.
 * @param pSensor Sensor of the player.
 * @return True if the player is on the ground.
 */
bool isPlayerOnGround(const al::HitSensor* pSensor) {
    return isPlayerOnGround(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is on the ground or in water.
 * @param pActor Actor of the player.
 * @return True if the player stands or swims.
 */
bool isPlayerOnGroundOrWater(const al::LiveActor* pActor) {
    if (getActionObserver(pActor)->isOnGround() || isPlayerInWater(pActor)) {
        return true;
    }

    return false;
}

/**
 * @brief Whether the player of a sensor is on the ground or in water.
 * @param pSensor Sensor of the player.
 * @return True if the player stands or swims.
 */
bool isPlayerOnGroundOrWater(const al::HitSensor* pSensor) {
    return isPlayerOnGroundOrWater(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is on the ground.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player is on the ground.
 */
bool isPlayerOnGround(const al::LiveActor* pActor, s32 index) {
    return isPlayerOnGround(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether a player is bound.
 * @param pActor Actor of the player.
 * @return True if the player is bound.
 */
bool isPlayerBinded(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isInBind();
}

/**
 * @brief Whether a player is in a shell that is on the ground.
 * @param pActor Actor of the player.
 * @return True if the player's shell is on the ground.
 */
bool isPlayerInKouraOnGround(const al::LiveActor* pActor) {
    if (toPlayerActor(pActor)->isInKoura() &&
        al::isOnGround(al::getSensorHost(toPlayerActor(pActor)->getBindSensor()), 0, 0.0f)) {
        return true;
    }

    return false;
}

/**
 * @brief Whether a player is in a shell.
 * @param pActor Actor of the player.
 * @return True if the player is in a shell.
 */
bool isPlayerInKoura(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isInKoura();
}

/**
 * @brief Get a player out of its shell.
 * @param pActor Actor of the player.
 */
void cancelPlayerInKoura(const al::LiveActor* pActor) {
    const_cast<PlayerActor*>(toPlayerActor(pActor))->cancelBindForDemo();
}

/**
 * @brief Sensor that binds a player.
 * @param pActor Actor of the player.
 * @return The binding sensor, or nullptr if the player is not bound.
 */
al::HitSensor* getPlayerBindedSenser(al::LiveActor* pActor) {
    if (!getActionObserver(pActor)->isInBind()) {
        return nullptr;
    }

    return toPlayerActor(pActor)->getBindSensor();
}

/**
 * @brief Whether the player of a sensor is bound.
 * @param pSensor Sensor of the player.
 * @return True if the player is bound.
 */
bool isPlayerBinded(const al::HitSensor* pSensor) {
    return isPlayerBinded(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is bound.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player is bound.
 */
bool isPlayerBinded(const al::LiveActor* pActor, s32 index) {
    return isPlayerBinded(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether the player of a sensor is in water.
 * @param pSensor Sensor of the player.
 * @return True if the player swims under or at the surface.
 */
bool isPlayerInWater(const al::HitSensor* pSensor) {
    return getActionObserver(getSensorPlayerActor(pSensor))->isWaterAction() ||
           isPlayerInWaterSurface(pSensor);
}

/**
 * @brief Whether the player of a sensor swims at the surface.
 * @param pSensor Sensor of the player.
 * @return True if the player swims at the surface.
 */
bool isPlayerInWaterSurface(const al::HitSensor* pSensor) {
    return getActionObserver(getSensorPlayerActor(pSensor))->isWaterSurfaceAction();
}

/**
 * @brief Whether the player of a sensor ground pounds.
 * @param pSensor Sensor of the player.
 * @return True if the player ground pounds.
 */
bool isPlayerHipDropping(const al::HitSensor* pSensor) {
    return isPlayerHipDropping(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player ground pounds.
 * @param pActor Actor of the player.
 * @return True if the player ground pounds.
 */
bool isPlayerHipDropping(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isHipDropping();
}

/**
 * @brief Whether a player ground pounds.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player ground pounds.
 */
bool isPlayerHipDropping(const al::LiveActor* pActor, s32 index) {
    return isPlayerHipDropping(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether the player of a sensor rolls on the ground.
 * @param pSensor Sensor of the player.
 * @return True if the player rolls on the ground.
 */
bool isPlayerRollingOnGround(const al::HitSensor* pSensor) {
    return getActionObserver(getSensorPlayerActor(pSensor))->isRollingOnGround();
}

/**
 * @brief Whether a player slides.
 * @param pActor Actor of the player.
 * @return True if the player slides.
 */
bool isPlayerSliding(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isSliding();
}

/**
 * @brief Whether a player slides.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player slides.
 */
bool isPlayerSliding(const al::LiveActor* pActor, s32 index) {
    return isPlayerSliding(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether the player of a sensor took damage this frame.
 * @param pSensor Sensor of the player.
 * @return True if the player was damaged this frame.
 */
bool isPlayerDamageTrigOn(const al::HitSensor* pSensor) {
    return isPlayerDamageTrigOn(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player took damage this frame.
 * @param pActor Actor of the player.
 * @return True if the player was damaged this frame.
 */
bool isPlayerDamageTrigOn(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isDamageTrigOn();
}

/**
 * @brief Whether the player of a sensor clings to a wall.
 * @param pSensor Sensor of the player.
 * @return True if the player clings to a wall.
 */
bool isPlayerWallSnap(const al::HitSensor* pSensor) {
    return isPlayerWallSnap(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player clings to a wall.
 * @param pActor Actor of the player.
 * @return True if the player clings to a wall.
 */
bool isPlayerWallSnap(const al::LiveActor* pActor) {
    return getActionObserver(pActor)->isWallSnapAction();
}

/**
 * @brief Whether a player spins on the ground.
 * @param pActor Actor of the player.
 * @return True if a ground spin action plays.
 */
bool isPlayerSpinGround(const al::LiveActor* pActor) {
    return al::isActionPlaying(pActor, "SpinGroundL") || al::isActionPlaying(pActor, "SpinGroundR");
}

/**
 * @brief Whether a player spins in the air.
 * @param pActor Actor of the player.
 * @return True if a spin jump action plays.
 */
bool isPlayerSpinJump(const al::LiveActor* pActor) {
    return al::isActionPlaying(pActor, "SpinJumpL") || al::isActionPlaying(pActor, "SpinJumpR") ||
           al::isActionPlaying(pActor, "JumpSkate");
}

/**
 * @brief Whether a player does a body attack.
 * @param pActor Actor of the player.
 * @return True if the body attack action plays.
 */
bool isPlayerBodyAttack(const al::LiveActor* pActor) {
    return al::isActionPlaying(pActor, "BodyAttack");
}

/**
 * @brief Whether the player of a sensor does a giga climb body attack.
 * @param pSensor Sensor of the player.
 * @return True if the giga climb body attack action plays.
 */
bool isPlayerClimbGigaBodyAttack(const al::HitSensor* pSensor) {
    return al::isActionPlaying(getSensorPlayerActor(pSensor), "ClimbGigaBodyAttack");
}

/**
 * @brief Whether a player skates on the ground.
 * @param pActor Actor of the player.
 * @return True if the player moves on skating ground.
 */
bool isPlayerSkating(const al::LiveActor* pActor) {
    Player* player = getPlayer(pActor);
    return PlayerActionFunc::isMapCodeSkate(player->getCollision()) &&
           PlayerActionTypeFunc::isGroundMove(player->getActionGraph());
}

/**
 * @brief Whether a player stands on an actor.
 * @param pActor Actor of the player.
 * @param pFloorActor Actor that may be the floor.
 * @return True if the player stands on that actor.
 */
bool isPlayerOnColliderGround(const al::LiveActor* pActor, const al::LiveActor* pFloorActor) {
    return getActionObserver(pActor)->isOnGround() && toPlayerActor(pActor)->isOnFloor(pFloorActor);
}

/**
 * @brief First player that is not dead.
 * @param pHolder Holder of the players.
 * @return The first alive player, or nullptr.
 */
al::LiveActor* getActivePlayer(al::PlayerHolder* pHolder) {
    s32 playerNum = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pHolder, i);
        if (player->getFlags()->isDead) {
            continue;
        }

        return player;
    }

    return nullptr;
}

/**
 * @brief First player that is not dead.
 * @param pActor Actor used to reach the players.
 * @return The first alive player, or nullptr.
 */
al::LiveActor* getActivePlayer(const al::LiveActor* pActor) {
    return getActivePlayer(pActor->getSceneInfo()->playerHolder);
}

/**
 * @brief Whether any player plays a figure change demo.
 * @param pHolder Holder of the players.
 * @return True if a living player is in a change demo.
 */
bool isPlayerChangeDemoAny(const al::PlayerHolder* pHolder) {
    for (s32 i = 0; i < pHolder->getPlayerNum(); i++) {
        al::LiveActor* player = pHolder->getPlayer(i);
        if (player != nullptr && !al::isDead(player) &&
            getActionObserver(player)->isChangeDemo()) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Whether any player plays a figure change demo.
 * @param pActor Actor used to reach the players.
 * @return True if a living player is in a change demo.
 */
bool isPlayerChangeDemoAny(const al::LiveActor* pActor) {
    return isPlayerChangeDemoAny(pActor->getSceneInfo()->playerHolder);
}

/**
 * @brief Whether a player is a character.
 * @param pActor Actor of the player.
 * @param chara Character to check.
 * @return True if the player is that character.
 */
bool isPlayerChara(const al::LiveActor* pActor, EPlayerChara chara) {
    return getPlayer(pActor)->getCharaQuery()->isChara(chara);
}

/**
 * @brief Whether the player of a sensor is a character.
 * @param pSensor Sensor of the player.
 * @param chara Character to check.
 * @return True if the player is that character.
 */
bool isPlayerChara(const al::HitSensor* pSensor, EPlayerChara chara) {
    return getPlayer(getSensorPlayerActor(pSensor))->getCharaQuery()->isChara(chara);
}

/**
 * @brief Whether the player of a sensor is Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Mario.
 */
bool isPlayerCharaMario(const al::HitSensor* pSensor) {
    return isPlayerChara(pSensor, EPlayerChara::Mario);
}

/**
 * @brief Whether the player of a sensor is Luigi.
 * @param pSensor Sensor of the player.
 * @return True if the player is Luigi.
 */
bool isPlayerCharaLuigi(const al::HitSensor* pSensor) {
    return isPlayerChara(pSensor, EPlayerChara::Luigi);
}

/**
 * @brief Whether the player of a sensor is Peach.
 * @param pSensor Sensor of the player.
 * @return True if the player is Peach.
 */
bool isPlayerCharaPeach(const al::HitSensor* pSensor) {
    return isPlayerChara(pSensor, EPlayerChara::Peach);
}

/**
 * @brief Whether the player of a sensor is Toad.
 * @param pSensor Sensor of the player.
 * @return True if the player is Toad.
 */
bool isPlayerCharaKinopio(const al::HitSensor* pSensor) {
    return isPlayerChara(pSensor, EPlayerChara::Kinopio);
}

/**
 * @brief Whether the player of a sensor is Rosalina.
 * @param pSensor Sensor of the player.
 * @return True if the player is Rosalina.
 */
bool isPlayerCharaRosetta(const al::HitSensor* pSensor) {
    return isPlayerChara(pSensor, EPlayerChara::Rosetta);
}

/**
 * @brief Character type of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The character type, or 0 if none matches.
 */
s32 getPlayerCharaType(const al::HitSensor* pSensor) {
    for (s32 i = 0; i < getPlayerCharacterNumMaxTrue(); i++) {
        if (isPlayerChara(pSensor, EPlayerChara(i))) {
            return i;
        }
    }

    return 0;
}

/**
 * @brief Turn the player of a sensor into Super Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToSuperMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Super);
}

/**
 * @brief Turn a player into Super Mario.
 * @param pActor Actor of the player, may be nullptr.
 */
void tryChangeToSuperMario(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigure(toPlayerActor(pActor), EPlayerFigure::Super);
    }
}

/**
 * @brief Turn the player of a control user into Super Mario.
 * @param pActor Actor used to reach the players.
 * @param userId Control user index.
 */
void tryChangeToSuperMario(al::LiveActor* pActor, s32 userId) {
    tryChangeToSuperMario(findPlayerActorFirstByUserId(pActor, userId));
}

/**
 * @brief First player playing the character of a control user.
 * @param pActor Actor used to reach the players.
 * @param userId Control user index.
 * @return The player, or nullptr.
 */
al::LiveActor* findPlayerActorFirstByUserId(const al::LiveActor* pActor, s32 userId) {
    EPlayerChara chara = getControlUserCharacterType(GameDataHolderAccessor(pActor), userId);
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (toPlayerActor(player)->isChara(chara)) {
            return player;
        }
    }

    return nullptr;
}

/**
 * @brief Turn the player held by the player of a sensor into Super Mario.
 * @param pSensor Sensor of the holding player.
 */
void tryChangeHoldedPlayerToSuperMario(const al::HitSensor* pSensor) {
    PlayerActor* player = getSensorPlayerActor(pSensor);
    if (player->isHoldingAnotherPlayer()) {
        tryChangeToSuperMario(getSensorPlayerActor(player->getHoldingSensor()));
    }
}

/**
 * @brief Turn the player of a sensor into Fire Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToFireMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Fire);
}

/**
 * @brief Turn the player of a sensor into Cat Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToClimbMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Climb);
}

/**
 * @brief Turn the player of a sensor into Tanooki Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToRaccoonDogMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::RaccoonDog);
}

/**
 * @brief Turn the player of a sensor into Boomerang Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToBoomerangMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Boomerang);
}

/**
 * @brief Make the player of a sensor invincible, unless it is dead or in a bubble.
 * @param pSensor Sensor of the player.
 */
void tryChangeToInvincibleMario(const al::HitSensor* pSensor) {
    if (isPlayerDeadOrBubble(getSensorPlayerActor(pSensor))) {
        return;
    }

    getPlayer(getSensorPlayerActor(pSensor))->getInvincibleState()->getStar(true);
}

/**
 * @brief Make a player invincible, unless it is dead or in a bubble.
 * @param pActor Actor of the player.
 */
void tryChangeToInvincibleMario(al::LiveActor* pActor) {
    if (isPlayerDeadOrBubble(pActor)) {
        return;
    }

    getPlayer(pActor)->getInvincibleState()->getStar(true);
}

/**
 * @brief Make every player invincible that is neither dead nor in a bubble.
 * @param pActor Actor used to reach the players.
 * @param isPlayBgm Whether to play the invincibility music.
 */
void tryChangeToInvincibleAllPlayer(const al::LiveActor* pActor, bool isPlayBgm) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isPlayerDeadOrBubble(player)) {
            getPlayer(player)->getInvincibleState()->getStar(isPlayBgm);
        }
    }
}

/**
 * @brief End the invincibility of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param isStopBgm Whether to stop the invincibility music.
 * @param isKeepModel Whether to keep the invincible look.
 */
void cancelInvincibleMarioForce(const al::HitSensor* pSensor, bool isStopBgm, bool isKeepModel) {
    getPlayer(getSensorPlayerActor(pSensor))->getInvincibleState()->endForce(isStopBgm, isKeepModel);
}

/**
 * @brief End the invincibility of a player.
 * @param pActor Actor of the player.
 * @param isStopBgm Whether to stop the invincibility music.
 * @param isKeepModel Whether to keep the invincible look.
 */
void cancelInvincibleMarioForce(al::LiveActor* pActor, bool isStopBgm, bool isKeepModel) {
    getPlayer(pActor)->getInvincibleState()->endForce(isStopBgm, isKeepModel);
}

/**
 * @brief Turn the player of a sensor giant, unless it is dead or in a bubble.
 * @param pSensor Sensor of the player.
 */
void tryChangeToGiantMario(const al::HitSensor* pSensor) {
    if (isPlayerDeadOrBubble(getSensorPlayerActor(pSensor))) {
        return;
    }

    changeToSuperMarioForce(pSensor);
    getPlayer(getSensorPlayerActor(pSensor))->getGiantDirector()->start();

    if (isPlayerEquipHeadgear(pSensor)) {
        removePlayerEquipHeadgear(pSensor, false);
    }
}

/**
 * @brief Turn the player of a sensor into Super Mario at once.
 * @param pSensor Sensor of the player.
 */
void changeToSuperMarioForce(const al::HitSensor* pSensor) {
    changeFigureForce(getSensorPlayerActor(pSensor), EPlayerFigure::Super);
}

/**
 * @brief Whether the player of a sensor wears headgear.
 * @param pSensor Sensor of the player.
 * @return True if headgear is equipped.
 */
bool isPlayerEquipHeadgear(const al::HitSensor* pSensor) {
    return isPlayerEquipHeadgear(getSensorPlayerActor(pSensor));
}

/**
 * @brief Take the headgear off the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param isGoal Whether the headgear is released for a goal.
 */
void removePlayerEquipHeadgear(const al::HitSensor* pSensor, bool isGoal) {
    getSensorPlayerActor(pSensor)->getModelHolder()->invalidateMash();
    getPlayer(getSensorPlayerActor(pSensor))
        ->getEquipmentDirector()
        ->releaseEquipment(cEquipmentTypeHeadgear,
                           static_cast<PlayerReleaseEquipmentGoalType>(isGoal));
}

/**
 * @brief Turn a player giant, unless it is dead or in a bubble.
 * @param pActor Actor of the player.
 */
void tryChangeToGiantMario(al::LiveActor* pActor) {
    if (isPlayerDeadOrBubble(pActor)) {
        return;
    }

    changeToSuperMarioForce(pActor);
    getPlayer(pActor)->getGiantDirector()->start();

    if (isPlayerEquipHeadgear(pActor)) {
        removePlayerEquipHeadgear(pActor, false);
    }
}

/**
 * @brief Turn a player into Super Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToSuperMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Super);
    }
}

/**
 * @brief Whether a player wears headgear.
 * @param pActor Actor of the player.
 * @return True if headgear is equipped.
 */
bool isPlayerEquipHeadgear(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getEquipmentDirector()->isEquipped(cEquipmentTypeHeadgear);
}

/**
 * @brief Take the headgear off a player.
 * @param pActor Actor of the player.
 * @param isGoal Whether the headgear is released for a goal.
 */
void removePlayerEquipHeadgear(const al::LiveActor* pActor, bool isGoal) {
    toPlayerActor(pActor)->getModelHolder()->invalidateMash();
    getPlayer(pActor)->getEquipmentDirector()->releaseEquipment(
        cEquipmentTypeHeadgear, static_cast<PlayerReleaseEquipmentGoalType>(isGoal));
}

/**
 * @brief Turn the player of a sensor into Giga Mario.
 * @param pBell The giga bell.
 * @param pSensor Sensor of the player.
 */
void tryChangeToGigaMario(const al::LiveActor* pBell, const al::HitSensor* pSensor) {
    tryChangeToGigaMario(pBell, getSensorPlayerActor(pSensor));
}

/**
 * @brief Turn a player into Giga Mario, unless it is dead or in a bubble.
 * @param pBell The giga bell.
 * @param pActor Actor of the player.
 */
void tryChangeToGigaMario(const al::LiveActor* pBell, al::LiveActor* pActor) {
    if (isPlayerDeadOrBubble(pActor)) {
        return;
    }

    getPlayer(pActor)->getGigaDirector()->start(pBell, false, false);
    tryChangeToSuperMario(pActor);

    if (isPlayerEquipHeadgear(pActor)) {
        removePlayerEquipHeadgear(pActor, false);
    }
}

/**
 * @brief Turn the player of a sensor into Giga Cat Mario at once.
 * @param pBell The giga bell.
 * @param pSensor Sensor of the player.
 */
void tryChangeToGigaClimbMario(const al::LiveActor* pBell, const al::HitSensor* pSensor) {
    tryChangeToGigaClimbMario(pBell, getSensorPlayerActor(pSensor), true);
}

/**
 * @brief Turn a player into Giga Cat Mario, unless it is dead or in a bubble.
 * @param pBell The giga bell.
 * @param pActor Actor of the player.
 * @param isForce Whether to change the figure at once.
 */
void tryChangeToGigaClimbMario(const al::LiveActor* pBell, al::LiveActor* pActor, bool isForce) {
    if (isPlayerDeadOrBubble(pActor)) {
        return;
    }

    sead::Vector3f velocity = sead::Vector3f::zero;
    if (getPlayer(pActor)->getActionGraph()->getAction() != nullptr) {
        velocity = getPlayer(pActor)->getProperty()->getVelocity();
    }

    if (isForce) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::ClimbGiga);
    } else {
        changeFigure(toPlayerActor(pActor), EPlayerFigure::ClimbGiga);
    }

    PlayerGigaDirector* giga = getPlayer(pActor)->getGigaDirector();
    giga->start(pBell, true, !giga->isGiga());

    if (velocity != sead::Vector3f::zero) {
        getPlayer(pActor)->getProperty()->mVelocity = velocity;
    }

    if (isPlayerEquipHeadgear(pActor)) {
        removePlayerEquipHeadgear(pActor, false);
    }
}

/**
 * @brief Whether a player is Giga Mario.
 * @param pActor Actor of the player.
 * @return True if the player is in its giga form.
 */
bool isPlayerGiga(const al::LiveActor* pActor) {
    PlayerGigaDirector* giga = getPlayer(pActor)->getGigaDirector();
    return giga != nullptr && giga->isGiga();
}

/**
 * @brief Whether the first alive player is Giga Mario.
 * @param pHolder Holder of the players.
 * @return True if that player is in its giga form.
 */
bool isPlayerGiga(al::PlayerHolder* pHolder) {
    al::LiveActor* player = getActivePlayer(pHolder);
    if (player == nullptr) {
        return false;
    }

    return isPlayerGiga(player);
}

/**
 * @brief Scale of a player's giga form.
 * @param pActor Actor of the player.
 * @return The scale rate.
 */
f32 getGigaScaleRate(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getGigaDirector()->getScaleRate();
}

/**
 * @brief Keep a player in its giga form.
 * @param pActor Actor of the player.
 * @param isResetEndTimer Whether to restart the end timer.
 */
void setStayInGigaMario(al::LiveActor* pActor, bool isResetEndTimer) {
    getPlayer(pActor)->getGigaDirector()->stay(isResetEndTimer);
}

/**
 * @brief Let a player's giga form run out again.
 * @param pActor Actor of the player.
 * @param isResetEndTimer Whether to restart the end timer.
 */
void unsetStayInGigaMario(al::LiveActor* pActor, bool isResetEndTimer) {
    getPlayer(pActor)->getGigaDirector()->unstay(isResetEndTimer);
}

/**
 * @brief Restart the giga form's action graph.
 * @param pActor Actor of the player.
 */
void restartGigaMarioGraph(al::LiveActor* pActor) {
    getPlayer(pActor)->getGigaDirector()->start(nullptr, true, true);
}

/**
 * @brief End the giant form of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void cancelGiantMario(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))->getGiantDirector()->end();
}

/**
 * @brief End the giant form of a player.
 * @param pActor Actor of the player.
 */
void cancelGiantMario(al::LiveActor* pActor) {
    getPlayer(pActor)->getGiantDirector()->end();
}

/**
 * @brief End the giant form of the player of a sensor at once.
 * @param pSensor Sensor of the player.
 */
void cancelGiantMarioForce(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))->getGiantDirector()->forceEnd();
}

/**
 * @brief End the giant form of a player at once.
 * @param pActor Actor of the player.
 */
void cancelGiantMarioForce(al::LiveActor* pActor) {
    getPlayer(pActor)->getGiantDirector()->forceEnd();
}

/**
 * @brief Turn the player of a sensor into White Tanooki Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToRaccoonDogWhiteMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::RaccoonDogWhite);
}

/**
 * @brief Turn the player of a sensor into Lucky Cat Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToClimbMarioSpecial(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Manekineko);
}

/**
 * @brief Turn the player of a sensor into White Cat Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToClimbWhiteMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::ClimbWhite);
}

/**
 * @brief Turn the player of a sensor into Giga Cat Mario.
 * @param pSensor Sensor of the player.
 */
void tryChangeToClimbGigaMario(const al::HitSensor* pSensor) {
    changeFigure(getSensorPlayerActor(pSensor), EPlayerFigure::ClimbGiga);
}

/**
 * @brief Turn a player into Mini Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToMiniMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Mini);
    }
}

/**
 * @brief Turn a player into Cat Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToClimbMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Climb);
    }
}

/**
 * @brief Turn a player into Tanooki Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToRaccoonDogMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::RaccoonDog);
    }
}

/**
 * @brief Turn a player into Fire Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToFireMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Fire);
    }
}

/**
 * @brief Turn a player into Boomerang Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToBoomerangMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Boomerang);
    }
}

/**
 * @brief Turn a player into White Tanooki Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToRaccoonDogWhiteMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::RaccoonDogWhite);
    }
}

/**
 * @brief Turn a player into Lucky Cat Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToClimbMarioSpecialForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::Manekineko);
    }
}

/**
 * @brief Turn a player into White Cat Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToClimbWhiteMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::ClimbWhite);
    }
}

/**
 * @brief Turn a player into Giga Cat Mario at once.
 * @param pActor Actor of the player, may be nullptr.
 */
void changeToClimbGigaMarioForce(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        changeFigureForce(toPlayerActor(pActor), EPlayerFigure::ClimbGiga);
    }
}

/**
 * @brief Set the figure a player starts with.
 * @param pActor Actor of the player.
 * @param figure Figure to start with.
 * @param isForce Whether to change at once, with the change effects.
 */
void initPlayerFigureType(al::LiveActor* pActor, u32 figure, bool isForce) {
    PlayerFigureDirector* figureDirector = getPlayer(pActor)->getFigureDirector();
    if (isForce) {
        figureDirector->forceChange(EPlayerFigure(figure));
        figureDirector->update();
    } else {
        figureDirector->set(EPlayerFigure(figure));
    }
}

/**
 * @brief Figure of a player.
 * @param pActor Actor of the player.
 * @return The current figure.
 */
s32 getPlayerFigureType(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getFigureDirector()->getFigure();
}

/**
 * @brief Name of a player's figure.
 * @param pActor Actor of the player.
 * @return The figure name.
 */
const char* getPlayerFigureName(al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getFigureTypeName();
}

/**
 * @brief Real name of a player's figure, telling the cat variants apart.
 * @param pActor Actor of the player.
 * @return The figure name.
 */
const char* getPlayerRealFigureName(al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getRealFigureTypeName();
}

/**
 * @brief Figure a player has, or is about to have.
 * @param pActor Actor of the player.
 * @return The requested next figure, or the current one.
 */
s32 getPlayerFigureTypeWithNext(const al::LiveActor* pActor) {
    PlayerFigureDirector* figureDirector = getPlayer(pActor)->getFigureDirector();
    if (figureDirector->isNextFigureRequested()) {
        return figureDirector->getNextFigure();
    }

    return figureDirector->getFigure();
}

/**
 * @brief Whether the player of a sensor is Super Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Super Mario.
 */
bool isPlayerSuper(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Super);
}

/**
 * @brief Whether the player of a sensor is Mini Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Mini Mario.
 */
bool isPlayerMini(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Mini);
}

/**
 * @brief Whether a player is Mini Mario.
 * @param pActor Actor of the player.
 * @return True if the player is Mini Mario.
 */
bool isPlayerMini(al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::Mini);
}

/**
 * @brief Whether the player of a sensor is Cat Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Cat Mario.
 */
bool isPlayerClimb(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Climb);
}

/**
 * @brief Whether a player is Cat Mario.
 * @param pActor Actor of the player.
 * @return True if the player is Cat Mario.
 */
bool isPlayerClimb(const al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::Climb);
}

/**
 * @brief Whether the player of a sensor is Tanooki Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Tanooki Mario.
 */
bool isPlayerRaccoonDog(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::RaccoonDog);
}

/**
 * @brief Whether the player of a sensor is Fire Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Fire Mario.
 */
bool isPlayerFire(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Fire);
}

/**
 * @brief Whether the player of a sensor is Boomerang Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Boomerang Mario.
 */
bool isPlayerBoomerang(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Boomerang);
}

/**
 * @brief Whether the player of a sensor is giant.
 * @param pSensor Sensor of the player.
 * @return True if the player is giant.
 */
bool isPlayerGiant(const al::HitSensor* pSensor) {
    return isPlayerGiant(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is giant.
 * @param pActor Actor of the player.
 * @return True if the player is giant.
 */
bool isPlayerGiant(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getGiantDirector()->isGiant();
}

/**
 * @brief Whether the player of a sensor is White Tanooki Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is White Tanooki Mario.
 */
bool isPlayerRaccoonDogWhite(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::RaccoonDogWhite);
}

/**
 * @brief Whether a player is White Tanooki Mario.
 * @param pActor Actor of the player.
 * @return True if the player is White Tanooki Mario.
 */
bool isPlayerRaccoonDogWhite(const al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::RaccoonDogWhite);
}

/**
 * @brief Whether the player of a sensor is Lucky Cat Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Lucky Cat Mario.
 */
bool isPlayerClimbSpecial(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::Manekineko);
}

/**
 * @brief Whether a player is Lucky Cat Mario.
 * @param pActor Actor of the player.
 * @return True if the player is Lucky Cat Mario.
 */
bool isPlayerClimbSpecial(const al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::Manekineko);
}

/**
 * @brief Whether the player of a sensor is any kind of Cat Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player has a cat figure.
 */
bool isPlayerClimbOrClimbSpecial(const al::HitSensor* pSensor) {
    return isPlayerClimb(pSensor) || isPlayerClimbSpecial(pSensor) || isPlayerClimbWhite(pSensor) ||
           isPlayerClimbGiga(pSensor);
}

/**
 * @brief Whether the player of a sensor is White Cat Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is White Cat Mario.
 */
bool isPlayerClimbWhite(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::ClimbWhite);
}

/**
 * @brief Whether the player of a sensor is Giga Cat Mario.
 * @param pSensor Sensor of the player.
 * @return True if the player is Giga Cat Mario.
 */
bool isPlayerClimbGiga(const al::HitSensor* pSensor) {
    return isFigure(getSensorPlayerActor(pSensor), EPlayerFigure::ClimbGiga);
}

/**
 * @brief Whether a player is any kind of Cat Mario.
 * @param pActor Actor of the player.
 * @return True if the player has a cat figure.
 */
bool isPlayerClimbOrClimbSpecial(const al::LiveActor* pActor) {
    return isPlayerClimb(pActor) || isPlayerClimbSpecial(pActor) || isPlayerClimbWhite(pActor) ||
           isPlayerClimbGiga(pActor);
}

/**
 * @brief Whether a player is White Cat Mario.
 * @param pActor Actor of the player.
 * @return True if the player is White Cat Mario.
 */
bool isPlayerClimbWhite(const al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::ClimbWhite);
}

/**
 * @brief Whether a player is Giga Cat Mario.
 * @param pActor Actor of the player.
 * @return True if the player is Giga Cat Mario.
 */
bool isPlayerClimbGiga(const al::LiveActor* pActor) {
    return isFigure(pActor, EPlayerFigure::ClimbGiga);
}

/**
 * @brief Whether the player of a sensor is a lucky cat statue.
 * @param pSensor Sensor of the player.
 * @return True if the player turned into a statue.
 */
bool isPlayerManekinekoStatueOn(const al::HitSensor* pSensor) {
    return isPlayerManekinekoStatueOn(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether a player is a lucky cat statue.
 * @param pActor Actor of the player.
 * @return True if the player turned into a statue.
 */
bool isPlayerManekinekoStatueOn(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isManekinekoAlive();
}

/**
 * @brief Whether a player is a lucky cat statue.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return True if the player turned into a statue.
 */
bool isPlayerManekinekoStatueOn(const al::LiveActor* pActor, s32 index) {
    return isPlayerManekinekoStatueOn(al::getPlayerActor(pActor, index));
}

/**
 * @brief Whether a player is about to change its figure.
 * @param pActor Actor of the player.
 * @return True if a next figure is requested.
 */
bool isPlayerNextFigureRequested(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getFigureDirector()->isNextFigureRequested();
}

/**
 * @brief Figure a player is about to change to.
 * @param pActor Actor of the player.
 * @return The requested next figure.
 */
s32 getPlayerNextFigureType(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getFigureDirector()->getNextFigure();
}

/**
 * @brief Whether any player is Mini Mario.
 * @param pActor Actor used to reach the players.
 * @return True if a player that is neither dead nor in a bubble is Mini Mario.
 */
bool isAnyPlayerMini(const al::LiveActor* pActor) {
    for (s32 i = 0; i < al::getPlayerNumMax(pActor); i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (isFigure(player, EPlayerFigure::Mini)) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Revive the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void revivePlayer(const al::HitSensor* pSensor) {
    revivePlayer(al::getSensorHost(pSensor));
}

/**
 * @brief Revive a player on the ground.
 * @param pActor Actor of the player, may be nullptr.
 */
void revivePlayer(al::LiveActor* pActor) {
    if (pActor == nullptr) {
        return;
    }

    Player* player = getPlayer(pActor);
    player->getLifeControl()->revive();
    player->getRestarter()->restartOnGround();
    player->getActionGraph()->checkShift();
}

/**
 * @brief Let the players be revived in bubbles again.
 * @param pActor Actor requesting it.
 */
void resetDisableReviveBubbleForAllPlayer(al::LiveActor* pActor) {
    PlayerAliveWatcher::getPlayerAliveWatcher(pActor)->resetDisableReviveBubble(pActor);
}

/**
 * @brief Keep the players from being put in bubbles when they leave the screen.
 * @param pActor Actor requesting it.
 */
void setDisableFrameOutBubbleForAllPlayer(al::LiveActor* pActor) {
    PlayerAliveWatcher::getPlayerAliveWatcher(pActor)->setDisableBubbleFrameOut(pActor);
}

/**
 * @brief Let the players be put in bubbles again when they leave the screen.
 * @param pActor Actor requesting it.
 */
void resetDisableFrameOutBubbleForAllPlayer(al::LiveActor* pActor) {
    PlayerAliveWatcher::getPlayerAliveWatcher(pActor)->resetDisableBubbleFrameOut(pActor);
}

/**
 * @brief Cancel the bind requests of a sensor and let the players be revived in bubbles again.
 * @param pActor Actor used to reach the players.
 * @param pSensor Sensor that requested the binds.
 */
void cancelRequestBindAndResetDisableReviveBubbleForAllPlayer(al::LiveActor* pActor,
                                                              al::HitSensor* pSensor) {
    cancelRequestBindAllPlayer(pActor, pSensor);
    resetDisableReviveBubbleForAllPlayer(pActor);
}

/**
 * @brief Kill a player as if the time ran out.
 * @param pActor Actor of the player.
 */
void forceKillPlayer(al::LiveActor* pActor) {
    getPlayer(pActor)->getKiller()->killTimeUp();
}

/**
 * @brief Kill a player as if the time ran out, if it can be killed.
 * @param pActor Actor of the player.
 */
void tryForceKillPlayer(al::LiveActor* pActor) {
    Player* player = getPlayer(pActor);
    if (player != nullptr && player->getKiller() != nullptr) {
        player->getKiller()->killTimeUp();
    }
}

/**
 * @brief Front direction of a player.
 * @param pActor Actor of the player.
 * @return The front direction.
 */
const sead::Vector3f& getPlayerFront(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getProperty()->getFront();
}

/**
 * @brief Front direction of the player (or Bowser Jr.) of a sensor.
 * @param pSensor Sensor of the player.
 * @return The front direction.
 */
const sead::Vector3f& getPlayerFront(const al::HitSensor* pSensor) {
    return getSensorProperty(pSensor)->getFront();
}

/**
 * @brief Velocity of the player (or Bowser Jr.) of a sensor.
 * @param pSensor Sensor of the player.
 * @return The velocity.
 */
const sead::Vector3f& getPlayerVelocity(const al::HitSensor* pSensor) {
    return getSensorProperty(pSensor)->getVelocity();
}

/**
 * @brief Velocity of a player.
 * @param pActor Actor of the player.
 * @return The velocity.
 */
const sead::Vector3f& getPlayerVelocity(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getProperty()->getVelocity();
}

/**
 * @brief Velocity of a player.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return The velocity.
 */
const sead::Vector3f& getPlayerVelocity(const al::LiveActor* pActor, s32 index) {
    return toPlayerActor(al::getPlayerActor(pActor, index))->getProperty()->getVelocity();
}

/**
 * @brief Horizontal speed of the player (or Bowser Jr.) of a sensor.
 * @param pSensor Sensor of the player.
 * @return The length of the velocity on the XZ plane.
 */
f32 getPlayerSpeedH(const al::HitSensor* pSensor) {
    return calcSpeedH(getSensorProperty(pSensor));
}

/**
 * @brief Horizontal speed of a player.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return The length of the velocity on the XZ plane.
 */
f32 getPlayerSpeedH(const al::LiveActor* pActor, s32 index) {
    return calcSpeedH(toPlayerActor(al::getPlayerActor(pActor, index))->getProperty());
}

/**
 * @brief View matrix of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The view matrix.
 */
const sead::Matrix34f* getPlayerViewMtx(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getViewMtx();
}

/**
 * @brief Pad port of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The input port.
 */
s32 getPlayerInputPort(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getInputPort();
}

/**
 * @brief Pad port of a player.
 * @param pActor Actor of the player.
 * @return The input port.
 */
s32 getPlayerInputPort(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getInputPort();
}

/**
 * @brief Pad port of a player.
 * @param pActor Actor used to reach the players.
 * @param index Index of the player.
 * @return The input port.
 */
s32 getPlayerInputPort(const al::LiveActor* pActor, s32 index) {
    return getPlayerInputPort(al::getPlayerActor(pActor, index));
}

/**
 * @brief Player controlled from a pad port.
 * @param pHolder Holder of the players.
 * @param port Pad port.
 * @return The player, preferring one alive outside a bubble, or nullptr.
 */
al::LiveActor* findPlayerFromInputPort(const al::PlayerHolder* pHolder, s32 port) {
    return tryFindPlayerFromInputPort(pHolder, port, false);
}

/**
 * @brief Player controlled from a pad port.
 * @param pHolder Holder of the players.
 * @param port Pad port.
 * @param isAcceptBubble Whether a player in a bubble counts as alive.
 * @return The player, preferring an alive one, or nullptr.
 */
al::LiveActor* tryFindPlayerFromInputPort(const al::PlayerHolder* pHolder, s32 port,
                                          bool isAcceptBubble) {
    al::LiveActor* deadPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pHolder, i);
        if (toPlayerActor(player)->getInputPort() != port) {
            continue;
        }

        if (isPlayerDead(player) || (!isAcceptBubble && isInTractorBubble(player))) {
            if (deadPlayer == nullptr) {
                deadPlayer = player;
            }

            continue;
        }

        return player;
    }

    return deadPlayer;
}

/**
 * @brief Player controlled from a pad port.
 * @param pActor Actor used to reach the players.
 * @param port Pad port.
 * @return The player, preferring one alive outside a bubble, or nullptr.
 */
al::LiveActor* findPlayerFromInputPort(const al::LiveActor* pActor, s32 port) {
    return tryFindPlayerFromInputPort(pActor->getSceneInfo()->playerHolder, port, false);
}

/**
 * @brief Player controlled from a pad port.
 * @param pActor Actor used to reach the players.
 * @param port Pad port.
 * @param isAcceptBubble Whether a player in a bubble counts as alive.
 * @return The player, preferring an alive one, or nullptr.
 */
al::LiveActor* tryFindPlayerFromInputPort(const al::LiveActor* pActor, s32 port,
                                          bool isAcceptBubble) {
    return tryFindPlayerFromInputPort(pActor->getSceneInfo()->playerHolder, port, isAcceptBubble);
}

/**
 * @brief Position of a player's head collider.
 * @param pPos Position to write.
 * @param pActor Actor of the player.
 */
void calcPlayerHeadColliderPos(sead::Vector3f* pPos, const al::LiveActor* pActor) {
    toPlayerActor(pActor)->calcHeadColliderPos(pPos);
}

/**
 * @brief Position of a player's body collider.
 * @param pPos Position to write.
 * @param pActor Actor of the player.
 */
void calcPlayerBodyColliderPos(sead::Vector3f* pPos, const al::LiveActor* pActor) {
    toPlayerActor(pActor)->calcBodyColliderPos(pPos);
}

/**
 * @brief Side direction of a player.
 * @param pSide Direction to write.
 * @param pActor Actor of the player.
 */
void calcPlayerSide(sead::Vector3f* pSide, const al::LiveActor* pActor) {
    const PlayerProperty* property = toPlayerActor(pActor)->getProperty();
    pSide->setCross(property->getGroundUp(), property->getFront());
    al::normalize(pSide);
}

/**
 * @brief Turn a player toward its stick input.
 * @param pPlayer The player.
 */
void trySetPlayerForwardToInput(PlayerActor* pPlayer) {
    setForwardToInput(pPlayer);
}

/**
 * @brief Turn a player toward its stick input.
 * @param pActor Actor of the player.
 */
void trySetPlayerForwardToInput(al::LiveActor* pActor) {
    setForwardToInput(toPlayerActor(pActor));
}

/**
 * @brief Turn the player of a sensor toward its stick input.
 * @param pSensor Sensor of the player.
 */
void trySetPlayerForwardToInput(al::HitSensor* pSensor) {
    setForwardToInput(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether the model of the player of a sensor is hidden.
 * @param pSensor Sensor of the player.
 * @return True if the current model is hidden.
 */
bool isPlayerHideModel(const al::HitSensor* pSensor) {
    return isPlayerHideModel(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether the model of a player is hidden.
 * @param pActor Actor of the player.
 * @return True if the current model is hidden.
 */
bool isPlayerHideModel(const al::LiveActor* pActor) {
    return al::isHideModel(toPlayerActor(pActor)->getModelHolder()->getCurrentModel());
}

/**
 * @brief Whether the silhouette of the player of a sensor is hidden.
 * @param pSensor Sensor of the player.
 * @return True if the silhouette is hidden.
 */
bool isPlayerHideSilhouette(const al::HitSensor* pSensor) {
    return isPlayerHideSilhouette(getSensorPlayerActor(pSensor));
}

/**
 * @brief Whether the silhouette of a player is hidden.
 * @param pActor Actor of the player.
 * @return True if the silhouette is hidden.
 */
bool isPlayerHideSilhouette(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getModelHolder()->isSilhouetteHidden();
}

/**
 * @brief Joint matrix of the model of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param pJointName Name of the joint.
 * @return The joint matrix.
 */
sead::Matrix34f* getPlayerModelJointMtxPtr(const al::HitSensor* pSensor, const char* pJointName) {
    return getPlayerModelJointMtxPtr(getSensorPlayerActor(pSensor), pJointName);
}

/**
 * @brief Joint matrix of the model of a player.
 * @param pActor Actor of the player.
 * @param pJointName Name of the joint.
 * @return The joint matrix.
 */
sead::Matrix34f* getPlayerModelJointMtxPtr(const al::LiveActor* pActor, const char* pJointName) {
    return al::getJointMtxPtr(toPlayerActor(pActor)->getModelHolder()->getCurrentModel(),
                              pJointName);
}

/**
 * @brief Position of a joint of the model of the player of a sensor.
 * @param pPos Position to write.
 * @param pSensor Sensor of the player.
 * @param pJointName Name of the joint.
 */
void calcPlayerModelJointPos(sead::Vector3f* pPos, const al::HitSensor* pSensor,
                             const char* pJointName) {
    al::calcJointPos(pPos, getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                     pJointName);
}


/**
 * @brief Position between the hands of the player of a sensor.
 * @param pPos Position to write.
 * @param pSensor Sensor of the player.
 */
void calcPlayerHoldPos(sead::Vector3f* pPos, const al::HitSensor* pSensor) {
    sead::Vector3f handL;
    al::calcJointPos(&handL, getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                     "HandL");
    sead::Vector3f handR;
    al::calcJointPos(&handR, getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                     "HandR");
    *pPos = (handL + handR) * 0.5f;
}

/**
 * @brief Pose of an object held by the player of a sensor.
 * @param pMtx Matrix to write.
 * @param pSensor Sensor of the player.
 */
void calcPlayerHoldMtx(sead::Matrix34f* pMtx, const al::HitSensor* pSensor) {
    sead::Vector3f handL;
    al::calcJointPos(&handL, getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                     "HandL");
    sead::Vector3f handR;
    al::calcJointPos(&handR, getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                     "HandR");
    sead::Vector3f holdPos = (handL + handR) * 0.5f;

    const sead::Matrix34f* spineMtx = al::getJointMtxPtr(
        getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(), "Spine2");
    sead::Quatf rotate;
    rotate.setAxisAngle(sead::Vector3f::ex, -90.0f);
    sead::Matrix34f rotateMtx;
    rotateMtx.fromQuat(rotate);
    pMtx->setMul(*spineMtx, rotateMtx);
    pMtx->setTranslation(holdPos);
}

/**
 * @brief Key config of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The key config.
 */
const IUsePlayerKeyConfig* getPlayerKeyConfig(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getKeyConfig();
}

/**
 * @brief Key config of a player.
 * @param pActor Actor of the player.
 * @return The key config.
 */
const IUsePlayerKeyConfig* getPlayerKeyConfig(const al::LiveActor* pActor) {
    return toPlayerActor(pActor)->getKeyConfig();
}

/**
 * @brief Put headgear on the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param pHeadgearSensor Sensor of the headgear.
 * @param pDelegate Called back by the equipment.
 * @param action Action the headgear provides.
 * @return True if the headgear was equipped.
 */
bool tryPlayerEquipHeadgear(const al::HitSensor* pSensor, al::HitSensor* pHeadgearSensor,
                            sead::IDelegate* pDelegate, u32 action) {
    if (getPlayer(getSensorPlayerActor(pSensor))
            ->getEquipmentDirector()
            ->tryEquip(cEquipmentTypeHeadgear, static_cast<EPlayerEquipmentAction>(action),
                       pHeadgearSensor, pDelegate)) {
        getSensorPlayerActor(pSensor)->getModelHolder()->validateMash();
        return true;
    }

    return false;
}

/**
 * @brief Take the headgear off the player of a sensor without effects.
 * @param pSensor Sensor of the player.
 */
void removePlayerEquipHeadgearSilent(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->invalidateMash();
    getPlayer(getSensorPlayerActor(pSensor))
        ->getEquipmentDirector()
        ->releaseEquipment(cEquipmentTypeHeadgear, cReleaseGoalTypeSilent);
}

/**
 * @brief Take the headgear off a player without effects.
 * @param pActor Actor of the player.
 */
void removePlayerEquipHeadgearSilent(const al::LiveActor* pActor) {
    toPlayerActor(pActor)->getModelHolder()->invalidateMash();
    getPlayer(pActor)->getEquipmentDirector()->releaseEquipment(cEquipmentTypeHeadgear,
                                                                cReleaseGoalTypeSilent);
}

/**
 * @brief Put a crown on the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param pCrownSensor Sensor of the crown.
 * @param action Action the crown provides.
 * @return True if the crown was equipped.
 */
bool tryPlayerEquipCrown(const al::HitSensor* pSensor, al::HitSensor* pCrownSensor, u32 action) {
    PlayerEquipmentDirector* equipmentDirector =
        getPlayer(getSensorPlayerActor(pSensor))->getEquipmentDirector();
    return equipmentDirector->tryEquip(cEquipmentTypeCrown,
                                       static_cast<EPlayerEquipmentAction>(action), pCrownSensor,
                                       nullptr);
}

/**
 * @brief Take the crown off the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void removePlayerEquipCrown(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))
        ->getEquipmentDirector()
        ->releaseEquipment(cEquipmentTypeCrown, cReleaseGoalTypeNormal);
}

/**
 * @brief Take the crown off the player of a sensor at a goal pole.
 * @param pSensor Sensor of the player.
 */
void removePlayerEquipCrownGoalPole(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))
        ->getEquipmentDirector()
        ->releaseEquipment(cEquipmentTypeCrown, cReleaseGoalTypeGoalPole);
}

/**
 * @brief Take the crown off the player of a sensor at a gate keeper.
 * @param pSensor Sensor of the player.
 */
void removePlayerEquipCrownGateKeeper(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))
        ->getEquipmentDirector()
        ->releaseEquipment(cEquipmentTypeCrown, cReleaseGoalTypeGateKeeper);
}

/**
 * @brief Whether the player of a sensor rises with a propeller box.
 * @param pSensor Sensor of the player.
 * @return True if the propeller jump rises.
 */
bool isPlayerPropellerRising(const al::HitSensor* pSensor) {
    return getPlayer(getSensorPlayerActor(pSensor))
        ->getPropellerJumpPhase()
        ->isPropellerJumpRising();
}

/**
 * @brief Whether the player of a sensor jumps with a propeller box.
 * @param pSensor Sensor of the player.
 * @return True if the player does a propeller jump.
 */
bool isPlayerPropellerJumping(const al::HitSensor* pSensor) {
    return getPlayer(getSensorPlayerActor(pSensor))->getPropellerJumpPhase()->isPropellerJumping();
}

/**
 * @brief Whether the player of a sensor glides with a propeller box.
 * @param pSensor Sensor of the player.
 * @return True if the propeller jump glides.
 */
bool isPlayerPropellerGlide(const al::HitSensor* pSensor) {
    return getPlayer(getSensorPlayerActor(pSensor))
        ->getPropellerJumpPhase()
        ->isPropellerJumpGlide();
}

/**
 * @brief Whether a player's equipment provides the disregard action.
 * @param pActor Actor of the player.
 * @return True if the equipment action is active.
 */
bool isPlayerEquipDisregard(const al::LiveActor* pActor) {
    return getPlayer(pActor)->getEquipmentDirector()->isEquipmentAction(
        cEquipmentActionDisregard);
}

/**
 * @brief Take all equipment off the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void removeAllEquipFromPlayer(const al::HitSensor* pSensor) {
    removeAllEquip(pSensor, cReleaseGoalTypeNormal);
}

/**
 * @brief Take all equipment off a player.
 * @param pActor Actor of the player.
 */
void removeAllEquipFromPlayer(al::LiveActor* pActor) {
    removeAllEquip(pActor, cReleaseGoalTypeNormal);
}

/**
 * @brief Take all equipment off the player of a sensor at a goal pole.
 * @param pSensor Sensor of the player.
 */
void removeAllEquipFromPlayerGoalPole(const al::HitSensor* pSensor) {
    removeAllEquip(pSensor, cReleaseGoalTypeGoalPole);
}

/**
 * @brief Take all equipment off a player at a goal pole.
 * @param pActor Actor of the player.
 */
void removeAllEquipFromPlayerGoalPole(al::LiveActor* pActor) {
    removeAllEquip(pActor, cReleaseGoalTypeGoalPole);
}

/**
 * @brief Take all equipment off the player of a sensor at a gate keeper.
 * @param pSensor Sensor of the player.
 */
void removeAllEquipFromPlayerGateKeeper(const al::HitSensor* pSensor) {
    removeAllEquip(pSensor, cReleaseGoalTypeGateKeeper);
}

/**
 * @brief Take all equipment off a player at a gate keeper.
 * @param pActor Actor of the player.
 */
void removeAllEquipFromPlayerGateKeeper(al::LiveActor* pActor) {
    removeAllEquip(pActor, cReleaseGoalTypeGateKeeper);
}

/**
 * @brief Take all equipment off the player of a sensor without effects.
 * @param pSensor Sensor of the player.
 */
void removeAllEquipFromPlayerSilent(const al::HitSensor* pSensor) {
    removeAllEquip(pSensor, cReleaseGoalTypeSilent);
}

/**
 * @brief Take all equipment off a player without effects.
 * @param pActor Actor of the player.
 */
void removeAllEquipFromPlayerSilent(al::LiveActor* pActor) {
    removeAllEquip(pActor, cReleaseGoalTypeSilent);
}

/**
 * @brief Pause the headgear of a player.
 * @param pActor Actor of the player.
 */
void pausePlayerEquip(al::LiveActor* pActor) {
    getPlayer(pActor)->getEquipmentDirector()->pauseHeadgear();
}

/**
 * @brief Resume the headgear of a player.
 * @param pActor Actor of the player.
 */
void resumePlayerEquip(al::LiveActor* pActor) {
    getPlayer(pActor)->getEquipmentDirector()->resumeHeadgear();
}

/**
 * @brief Make the player of a sensor let go of what it holds.
 * @param pSensor Sensor of the player.
 */
void requestPlayerRelease(al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->requestRelease();
}

/**
 * @brief Whether the player of a sensor is held by someone.
 * @param pSensor Sensor of the player.
 * @return True if the player is held.
 */
bool isPlayerHolded(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getHoldedSensor() != nullptr;
}

/**
 * @brief Whether the player of a sensor holds another player.
 * @param pSensor Sensor of the player.
 * @return True if the player holds another player.
 */
bool isPlayerHoldingAnotherPlayer(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->isHoldingAnotherPlayer();
}

/**
 * @brief Whether the player of a sensor holds something.
 * @param pSensor Sensor of the player.
 * @return True if the player holds something.
 */
bool isPlayerHoldingSomething(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getHoldingSensor() != nullptr;
}

/**
 * @brief Whether the player of a sensor holds an actor.
 * @param pSensor Sensor of the player.
 * @param pHoldActor Actor that may be held.
 * @return True if the player holds that actor.
 */
bool isPlayerHolding(const al::HitSensor* pSensor, const al::LiveActor* pHoldActor) {
    al::HitSensor* holdingSensor = getSensorPlayerActor(pSensor)->getHoldingSensor();
    if (holdingSensor == nullptr) {
        return false;
    }

    return al::getSensorHost(holdingSensor) == pHoldActor;
}

/**
 * @brief Whether the player of a control user holds the host of a sensor.
 * @param pActor Actor used to reach the players.
 * @param userId Control user index.
 * @param pSensor Sensor of the object that may be held.
 * @return True if a player of that user holds the object.
 */
bool isPlayerHolding(al::LiveActor* pActor, s32 userId, const al::HitSensor* pSensor) {
    al::LiveActor* holdActor = al::getSensorHost(pSensor);
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDeadOrBubble(player) || findControlUserId(player) != userId) {
            continue;
        }

        al::HitSensor* holdingSensor = toPlayerActor(player)->getHoldingSensor();
        if (holdingSensor != nullptr && al::getSensorHost(holdingSensor) == holdActor) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Actor a player holds.
 * @param pActor Actor of the player.
 * @return The held actor, or nullptr.
 */
al::LiveActor* getPlayerHoldingActor(const al::LiveActor* pActor) {
    al::HitSensor* holdingSensor = toPlayerActor(pActor)->getHoldingSensor();
    if (holdingSensor == nullptr) {
        return nullptr;
    }

    return al::getSensorHost(holdingSensor);
}

/**
 * @brief Hide the item the player of a sensor holds.
 * @param pSensor Sensor of the player.
 * @param isHide Whether to hide the item.
 * @param isHideShadow Whether to hide the item's shadow.
 */
void hidePlayerHoldingItem(al::HitSensor* pSensor, bool isHide, bool isHideShadow) {
    hidePlayerHoldingItem(al::getSensorHost(pSensor), isHide, isHideShadow);
}

/**
 * @brief Hide the item a player holds.
 * @param pActor Actor of the player.
 * @param isHide Whether to hide the item.
 * @param isHideShadow Whether to hide the item's shadow.
 */
void hidePlayerHoldingItem(al::LiveActor* pActor, bool isHide, bool isHideShadow) {
    if (isReallyPlayerActor(pActor)) {
        toPlayerActor(pActor)->hideHoldingItem(isHide, isHideShadow);
    }
}

/**
 * @brief Stop the sinking sound of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void cancelSinkSe(al::HitSensor* pSensor) {
    cancelSinkSe(al::getSensorHost(pSensor));
}

/**
 * @brief Stop the sinking sound of a player.
 * @param pActor Actor of the player.
 */
void cancelSinkSe(al::LiveActor* pActor) {
    if (isReallyPlayerActor(pActor)) {
        toPlayerActor(pActor)->cancelSinkSe();
    }
}

/**
 * @brief Let the player of a sensor collect items.
 * @param pSensor Sensor of the player.
 */
void validatePlayerGetItem(al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->setValidGetItem(true);
}

/**
 * @brief Keep the player of a sensor from collecting items.
 * @param pSensor Sensor of the player.
 */
void invalidatePlayerGetItem(al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->setValidGetItem(false);
}

/**
 * @brief Request the player of a sensor to be bound, with the binder's priority.
 * @param pSensor Sensor of the player.
 * @param pBinderSensor Sensor binding the player.
 */
void requestPlayerBindWithPriority(al::HitSensor* pSensor, al::HitSensor* pBinderSensor) {
    getSensorPlayerActor(pSensor)->requestBind(pBinderSensor, 0.0f,
                                               getSensorPriority(pBinderSensor));
}

/**
 * @brief Request the player of a sensor to be bound.
 * @param pSensor Sensor of the player.
 * @param pBinderSensor Sensor binding the player.
 */
void requestPlayerBind(al::HitSensor* pSensor, al::HitSensor* pBinderSensor) {
    getSensorPlayerActor(pSensor)->requestBind(pBinderSensor, 0.0f, 0);
}

/**
 * @brief Request a player to be bound by a TractorBubble.
 * @param pActor Actor of the player.
 * @param pBubbleSensor Sensor of the bubble.
 */
void requestPlayerBindTractorBubble(al::LiveActor* pActor, al::HitSensor* pBubbleSensor) {
    toPlayerActor(pActor)->requestBind(pBubbleSensor, 0.0f, 0);
}

/**
 * @brief Whether the player of a sensor is bound by a sensor.
 * @param pSensor Sensor of the player.
 * @param pBinderSensor Sensor that may bind the player.
 * @return True if the living player is bound by that sensor.
 */
bool isPlayerBinded(al::HitSensor* pSensor, al::HitSensor* pBinderSensor) {
    PlayerActor* player = getSensorPlayerActor(pSensor);
    if (al::isDead(player)) {
        return false;
    }

    if (getActionObserver(player)->isInBind()) {
        return player->getBindSensor() == pBinderSensor;
    }

    return false;
}

/**
 * @brief Whether a sensor may bind the player of another sensor.
 * @param pSensor Sensor of the player.
 * @param pBinderSensor Sensor that wants to bind the player.
 * @return True if the player lives and is unbound or bound with a lower priority.
 */
bool isPlayerEnableBind(al::HitSensor* pSensor, const al::HitSensor* pBinderSensor) {
    PlayerActor* player = getSensorPlayerActor(pSensor);
    if (al::isDead(player)) {
        return false;
    }

    return isPlayerEnableBubble(player, pBinderSensor);
}

/**
 * @brief Add a double cherry copy to the player group.
 * @param pPlayer The new copy.
 */
void appendDoubleMario(PlayerActor* pPlayer) {
    getPlayerGroup(pPlayer)->append(pPlayer);
}

/**
 * @brief Center of a player and its double cherry copies.
 * @param pPos Position to write.
 * @param pPlayer The player.
 */
void calcDoubleMarioCenterPos(sead::Vector3f* pPos, const PlayerActor* pPlayer) {
    getPlayerGroup(pPlayer)->calcGroupCenterPos(pPos, pPlayer);
}

/**
 * @brief Head position of a player and its double cherry copies.
 * @param pPos Position to write.
 * @param pPlayer The player.
 * @param rOffset Offset from the head.
 */
void calcDoubleMarioHeadPos(sead::Vector3f* pPos, const PlayerActor* pPlayer,
                            const sead::Vector3f& rOffset) {
    getPlayerGroup(pPlayer)->calcGroupHeadPos(pPos, pPlayer, rOffset);
}

/**
 * @brief Position of a player.
 * @param pPlayer The player.
 * @return The position.
 */
const sead::Vector3f& getPlayerTrans(const PlayerActor* pPlayer) {
    return al::getTrans(pPlayer);
}

/**
 * @brief Whether a player is the last of its double cherry copies.
 * @param pPlayer The player.
 * @return True if no other copy is left.
 */
bool isLastDoubleMario(const PlayerActor* pPlayer) {
    return getPlayerGroup(pPlayer)->isLast(pPlayer);
}

/**
 * @brief Kill every double cherry copy but one.
 * @param pPlayer The copy to keep.
 */
void killAllDoubleMarioExcept(PlayerActor* pPlayer) {
    getPlayerGroup(pPlayer)->killAllExcept(pPlayer);
}

/**
 * @brief Kill every double cherry copy but the player of a sensor.
 * @param pSensor Sensor of the copy to keep.
 */
void killAllDoubleMarioExcept(al::HitSensor* pSensor) {
    PlayerActor* player = getSensorPlayerActor(pSensor);
    getPlayerGroup(player)->killAllExcept(player);
}

/**
 * @brief Kill every double cherry copy but the player of a sensor, scoring for each.
 * @param pSensor Sensor of the copy to keep.
 */
void killAllDoubleMarioExceptWithScore(al::HitSensor* pSensor) {
    getPlayerGroup(getSensorPlayerActor(pSensor))->killAllExceptWithScore(pSensor);
}

/**
 * @brief Number of players including the double cherry copies.
 * @param pActor Actor used to reach the scene objects.
 * @return The number of players.
 */
s32 calcDoubleMarioTotalNum(const al::LiveActor* pActor) {
    return getPlayerGroup(pActor)->calcDoubleMarioTotalNum();
}

/**
 * @brief Bring a player into the game in a bubble (unused).
 * @param pPlayer The player.
 * @param port Pad port of the player.
 * @param pTrans Position, or nullptr.
 * @param pFront Front direction, or nullptr.
 */
void activatePlayerWithBubble(PlayerActor* pPlayer, s32 port, const sead::Vector3f* pTrans,
                              const sead::Vector3f* pFront) {}

/**
 * @brief Bring a player into the game.
 * @param pPlayer The player.
 * @param port Pad port of the player, or 0 to keep its port.
 * @param pTrans Position, or nullptr.
 * @param pFront Front direction, or nullptr.
 */
void activatePlayer(PlayerActor* pPlayer, s32 port, const sead::Vector3f* pTrans,
                    const sead::Vector3f* pFront) {
    if (port > 0) {
        pPlayer->replaceInputPort(port);
    }

    if (pTrans != nullptr) {
        pPlayer->getProperty()->mTrans = *pTrans;
        al::resetPosition(pPlayer, *pTrans, false);
    }

    if (pFront != nullptr) {
        pPlayer->getProperty()->setFrontVec(*pFront);
    }

    pPlayer->updatePosture();
    pPlayer->appear();
    pPlayer->getModelHolder()->appear();
}

/**
 * @brief Take a player out of the game.
 * @param pPlayer The player.
 */
void deactivatePlayer(PlayerActor* pPlayer) {
    pPlayer->forceKill();
    pPlayer->getModelHolder()->kill();
}

/**
 * @brief Make a player immune to pipe damage.
 * @param pPlayer The player.
 */
void invalidatePlayerDamagePipe(PlayerActor* pPlayer) {
    pPlayer->getPlayer()->getDamageInvalidater()->invalidateForPipe();
}

/**
 * @brief Let a player take pipe damage again.
 * @param pPlayer The player.
 */
void validatePlayerDamagePipe(PlayerActor* pPlayer) {
    pPlayer->getPlayer()->getDamageInvalidater()->validateForPipe();
}

/**
 * @brief Make a player immune to damage for a while.
 * @param pPlayer The player.
 * @param frame Number of frames.
 */
void invalidatePlayerDamage(PlayerActor* pPlayer, u32 frame) {
    pPlayer->getPlayer()->getDamageInvalidater()->invalidateDamage(frame);
}

/**
 * @brief Stop a player from flashing while damage is invalid.
 * @param pPlayer The player.
 */
void invalidatePlayerFlash(PlayerActor* pPlayer) {
    pPlayer->getPlayer()->getDamageInvalidater()->invalidateFlash();
}

/**
 * @brief Let a player flash while damage is invalid.
 * @param pPlayer The player.
 */
void validatePlayerFlash(PlayerActor* pPlayer) {
    pPlayer->getPlayer()->getDamageInvalidater()->validateFlash();
}

/**
 * @brief Take all equipment off every double cherry copy but the player of a sensor, scoring for
 * each.
 * @param pSensor Sensor of the copy to keep.
 */
void removeAllEquipOfAllDoubleMarioExceptWithScore(al::HitSensor* pSensor) {
    getPlayerGroup(getSensorPlayerActor(pSensor))
        ->removeAllEquipOfAllDoubleMarioExceptWithScore(pSensor);
}

/**
 * @brief Set the main player actor.
 * @param pActor Actor of the main player.
 */
void setMainPlayerActor(al::LiveActor* pActor) {
    sMainPlayerActor = pActor;
}

/**
 * @brief Range the active players cover along a direction.
 * @param pMin Minimum to write, or nullptr.
 * @param pMax Maximum to write, or nullptr.
 * @param pActor Actor used to reach the players.
 * @param rOrigin Origin of the projection.
 * @param rDir Direction to project on.
 * @return True if an active player was found.
 */
bool calcPlayerDotMinMax(f32* pMin, f32* pMax, const al::LiveActor* pActor,
                         const sead::Vector3f& rOrigin, const sead::Vector3f& rDir) {
    f32 min = sead::Mathf::maxNumber();
    f32 max = sead::Mathf::minNumber();
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == nullptr || isPlayerDeadOrBubble(player)) {
            continue;
        }

        f32 dot = (al::getTrans(player) - rOrigin).dot(rDir);
        if (dot < min) {
            min = dot;
        }

        if (dot > max) {
            max = dot;
        }
    }

    if (min > max) {
        return false;
    }

    if (pMin != nullptr) {
        *pMin = min;
    }

    if (pMax != nullptr) {
        *pMax = max;
    }

    return true;
}

/**
 * @brief Nearest active player within a sphere.
 * @param pActor Actor at the center of the sphere.
 * @param radius Radius of the sphere, or 0 for no limit.
 * @return The nearest active player, or nullptr.
 */
al::LiveActor* tryFindNearestActivePlayerActorInSphere(const al::LiveActor* pActor, f32 radius) {
    const sead::Vector3f& trans = al::getTrans(pActor);
    f32 minDistanceSq = sead::Mathf::maxNumber();
    al::LiveActor* nearestPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isActivePlayer(player)) {
            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, player, trans);
    }

    if (radius * radius < minDistanceSq && radius > 0.0f) {
        return nullptr;
    }

    return nearestPlayer;
}

/**
 * @brief Nearest active player or Bowser Jr. within a sphere.
 * @param pActor Actor at the center of the sphere.
 * @param radius Radius of the sphere, or 0 for no limit.
 * @return The nearest one, or nullptr.
 */
al::LiveActor* tryFindNearestActivePlayerOrKoopaJrActorInSphere(const al::LiveActor* pActor,
                                                                f32 radius) {
    const sead::Vector3f& trans = al::getTrans(pActor);
    f32 minDistanceSq = sead::Mathf::maxNumber();
    al::LiveActor* nearestPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isActivePlayer(player)) {
            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, player, trans);

        al::LiveActor* koopaJr = toPlayerActor(player)->getKoopaJr();
        if (koopaJr == nullptr) {
            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, koopaJr, trans);
    }

    if (radius * radius < minDistanceSq && radius > 0.0f) {
        return nullptr;
    }

    return nearestPlayer;
}

/**
 * @brief Last active player found within a cylinder.
 * @param pActor Actor at the center of the cylinder.
 * @param radius Radius of the cylinder.
 * @param bottom Lowest height relative to the actor.
 * @param top Highest height relative to the actor.
 * @return The player, or nullptr.
 */
al::LiveActor* tryFindNearestActivePlayerActorInCylinder(const al::LiveActor* pActor, f32 radius,
                                                         f32 bottom, f32 top) {
    al::LiveActor* foundPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isActivePlayer(player)) {
            continue;
        }

        if (isInCylinder(pActor, player, radius, bottom, top)) {
            foundPlayer = player;
        }
    }

    return foundPlayer;
}

/**
 * @brief Last active player or Bowser Jr. found within a cylinder.
 * @param pActor Actor at the center of the cylinder.
 * @param radius Radius of the cylinder.
 * @param bottom Lowest height relative to the actor.
 * @param top Highest height relative to the actor.
 * @return The player or Bowser Jr., or nullptr.
 */
al::LiveActor* tryFindNearestActivePlayerOrKoopaJrActorInCylinder(const al::LiveActor* pActor,
                                                                  f32 radius, f32 bottom,
                                                                  f32 top) {
    al::LiveActor* foundPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (!isActivePlayer(player)) {
            continue;
        }

        if (isInCylinder(pActor, player, radius, bottom, top)) {
            foundPlayer = player;
        }

        al::LiveActor* koopaJr = toPlayerActor(player)->getKoopaJr();
        if (koopaJr == nullptr) {
            continue;
        }

        if (isInCylinder(pActor, koopaJr, radius, bottom, top)) {
            foundPlayer = koopaJr;
        }
    }

    return foundPlayer;
}

/**
 * @brief Nearest active player, or a dead one if none is active.
 * @param pActor Actor to measure from.
 * @return The nearest active player, or the first dead or bubbled one, or nullptr.
 */
al::LiveActor* findNearestActivePlayerActor(const al::LiveActor* pActor) {
    const sead::Vector3f& trans = al::getTrans(pActor);
    al::LiveActor* deadPlayer = nullptr;
    f32 minDistanceSq = sead::Mathf::maxNumber();
    al::LiveActor* nearestPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == nullptr || al::isDead(player)) {
            continue;
        }

        if (isPlayerDeadOrBubble(player)) {
            if (deadPlayer == nullptr) {
                deadPlayer = player;
            }

            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, player, trans);
    }

    return nearestPlayer != nullptr ? nearestPlayer : deadPlayer;
}


/**
 * @brief Nearest active player or Bowser Jr., or a dead player if none is active.
 * @param pActor Actor to measure from.
 * @return The nearest active one, or the first dead or bubbled player, or nullptr.
 */
al::LiveActor* findNearestActivePlayerOrKoopaJrActor(const al::LiveActor* pActor) {
    const sead::Vector3f& trans = al::getTrans(pActor);
    al::LiveActor* deadPlayer = nullptr;
    f32 minDistanceSq = sead::Mathf::maxNumber();
    al::LiveActor* nearestPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == nullptr || al::isDead(player)) {
            continue;
        }

        if (isPlayerDeadOrBubble(player)) {
            if (deadPlayer == nullptr) {
                deadPlayer = player;
            }

            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, player, trans);

        al::LiveActor* koopaJr = toPlayerActor(player)->getKoopaJr();
        if (koopaJr == nullptr) {
            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, koopaJr, trans);
    }

    return nearestPlayer != nullptr ? nearestPlayer : deadPlayer;
}

/**
 * @brief Active player closest to a direction on the horizontal plane.
 * @param pActor Actor used to reach the players.
 * @param rDir Direction to look in.
 * @param rOrigin Origin of the look.
 * @return The active player best in line with the direction, or a dead one, or nullptr.
 */
al::LiveActor* findNearestAnglePlayerActorH(const al::LiveActor* pActor, const sead::Vector3f& rDir,
                                            const sead::Vector3f& rOrigin) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    sead::Vector3f dirH(rDir.x, 0.0f, rDir.z);
    if (al::normalizeOrZero(&dirH)) {
        return nullptr;
    }

    al::LiveActor* deadPlayer = nullptr;
    f32 maxDot = -10.0f;
    al::LiveActor* nearestPlayer = nullptr;
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == nullptr || al::isDead(player)) {
            continue;
        }

        if (isPlayerDeadOrBubble(player)) {
            if (deadPlayer == nullptr) {
                deadPlayer = player;
            }

            continue;
        }

        const sead::Vector3f& trans = al::getTrans(player);
        sead::Vector3f dirToPlayerH(trans.x - rOrigin.x, 0.0f, trans.z - rOrigin.z);
        al::normalizeOrZero(&dirToPlayerH);
        f32 dot = dirToPlayerH.dot(dirH);
        if (maxDot < dot) {
            maxDot = dot;
            nearestPlayer = player;
        }
    }

    return nearestPlayer != nullptr ? nearestPlayer : deadPlayer;
}

/**
 * @brief Active player closest to a direction from an actor on the horizontal plane.
 * @param pActor Actor to look from.
 * @param rDir Direction to look in.
 * @return The active player best in line with the direction, or a dead one, or nullptr.
 */
al::LiveActor* findNearestAnglePlayerActorH(const al::LiveActor* pActor, const sead::Vector3f& rDir) {
    return findNearestAnglePlayerActorH(pActor, rDir, al::getTrans(pActor));
}

/**
 * @brief Nearest active player of a control user within a sphere.
 * @param pActor Actor at the center of the sphere.
 * @param userId Control user index.
 * @param radius Radius of the sphere, or 0 for no limit.
 * @return The nearest player of that user, or nullptr.
 */
al::LiveActor* tryFindNearestPlayerActorByUserId(const al::LiveActor* pActor, s32 userId,
                                                 f32 radius) {
    GameDataHolderAccessor accessor(pActor);
    const sead::Vector3f& trans = al::getTrans(pActor);
    f32 minDistanceSq = sead::Mathf::maxNumber();
    al::LiveActor* nearestPlayer = nullptr;
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (tryCalcControlUserIdFromPortNum(accessor, toPlayerActor(player)->getInputPort()) !=
            userId) {
            continue;
        }

        updateNearest(&nearestPlayer, &minDistanceSq, player, trans);
    }

    if (radius * radius < minDistanceSq && radius > 0.0f) {
        return nullptr;
    }

    return nearestPlayer;
}

/**
 * @brief A random active player.
 * @param pActor Actor used to reach the players.
 * @return One of the active players, or nullptr.
 */
al::LiveActor* findRandomPlayerActor(const al::LiveActor* pActor) {
    s32 activePlayerNum = calcActivePlayerNum(pActor);
    s32 playerNum = al::getPlayerNumMax(pActor);
    s32 randomIndex = al::getRandom(activePlayerNum);
    s32 activeIndex = 0;
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == nullptr || isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (activeIndex == randomIndex) {
            return player;
        }

        activeIndex++;
    }

    return nullptr;
}

/**
 * @brief Number of active players.
 * @param pActor Actor used to reach the players.
 * @return The number of players that are neither dead nor in a bubble.
 */
s32 calcActivePlayerNum(const al::LiveActor* pActor) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    s32 activePlayerNum = 0;
    for (s32 i = 0; i < playerNum; i++) {
        if (!isPlayerDeadOrBubble(al::getPlayerActor(pActor, i))) {
            activePlayerNum++;
        }
    }

    return activePlayerNum;
}

/**
 * @brief List the players from the nearest to the farthest.
 * @param pActor Actor to measure from.
 * @param pPlayerList List to fill.
 * @param listSize Size of the list.
 * @return Number of players written.
 */
u32 calcPlayerListOrderByDistance(const al::LiveActor* pActor, const al::LiveActor** pPlayerList,
                                  u32 listSize) {
    u32 playerNum = al::getPlayerNumMax(pActor);
    const sead::Vector3f& trans = al::getTrans(pActor);
    f32 distanceSqList[64];
    for (u32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        f32 distanceSq = sead::Mathf::maxNumber();
        if (!isPlayerDeadOrBubble(player)) {
            distanceSq = (al::getTrans(player) - trans).squaredLength();
        }

        distanceSqList[i] = distanceSq;
    }

    for (u32 i = 0; i < listSize; i++) {
        s32 nearestIndex = -1;
        f32 minDistanceSq = 1e35f;
        for (u32 j = 0; j < playerNum; j++) {
            if (distanceSqList[j] <= minDistanceSq) {
                minDistanceSq = distanceSqList[j];
                nearestIndex = j;
            }
        }

        if (nearestIndex == -1) {
            return i;
        }

        pPlayerList[i] = al::tryGetPlayerActor(pActor, nearestIndex);
        distanceSqList[nearestIndex] = sead::Mathf::maxNumber();
    }

    return listSize;
}

/**
 * @brief First player that is a character.
 * @param pActor Actor used to reach the players.
 * @param characterType Character type.
 * @return The player, or nullptr.
 */
al::LiveActor* findPlayerActorFirstByCharacterType(const al::LiveActor* pActor, s32 characterType) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (toPlayerActor(player)->isChara(characterType)) {
            return player;
        }
    }

    return nullptr;
}

/**
 * @brief First player playing the character of a control user.
 * @param pHolder Holder of the players.
 * @param userId Control user index.
 * @return The player, or nullptr.
 */
al::LiveActor* findPlayerActorFirstByUserId(const al::PlayerHolder* pHolder, s32 userId) {
    s32 characterType =
        getControlUserCharacterType(GameDataHolderAccessor(al::getPlayerActor(pHolder, 0)), userId);
    s32 playerNum = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pHolder, i);
        if (getPlayerCharaType(player) == characterType) {
            return player;
        }
    }

    return nullptr;
}

/**
 * @brief First active player of a control user.
 * @param pActor Actor used to reach the players.
 * @param userId Control user index.
 * @return The player, or nullptr.
 */
al::LiveActor* tryFindActivePlayerActorFirstByUserId(const al::LiveActor* pActor, s32 userId) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (findControlUserId(player) == userId) {
            return player;
        }
    }

    return nullptr;
}

/**
 * @brief First alive player of a control user.
 * @param pActor Actor used to reach the players.
 * @param userId Control user index.
 * @return The player, or nullptr.
 */
al::LiveActor* tryFindAlivePlayerActorFirstByUserId(const al::LiveActor* pActor, s32 userId) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (isPlayerDead(player)) {
            continue;
        }

        if (findControlUserId(player) == userId) {
            return player;
        }
    }

    return nullptr;
}

/**
 * @brief Whether a player is the only active one of its control user.
 * @param pActor Actor of the player.
 * @return True if no other active player belongs to the same user.
 */
bool isOnlyActivePlayerWithUserId(const al::LiveActor* pActor) {
    s32 playerNum = al::getPlayerNumMax(pActor);
    s32 userId = findControlUserId(pActor);
    for (s32 i = 0; i < playerNum; i++) {
        al::LiveActor* player = al::getPlayerActor(pActor, i);
        if (player == pActor || isPlayerDeadOrBubble(player)) {
            continue;
        }

        if (findControlUserId(player) == userId) {
            return false;
        }
    }

    return true;
}

/**
 * @brief Pause the amiibo director of every player.
 * @param pHolder Holder of the players.
 * @param isPause Whether to pause.
 */
void pauseAllPlayerAmiiboDirector(al::PlayerHolder* pHolder, bool isPause) {
    s32 playerNum = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNum; i++) {
        toPlayerActor(al::getPlayerActor(pHolder, i))->pausePlayerAmiiboDirector(isPause);
    }
}

/**
 * @brief End the pause of the amiibo director of every player.
 * @param pHolder Holder of the players.
 */
void endPauseAllPlayerAmiiboDirector(al::PlayerHolder* pHolder) {
    s32 playerNum = al::getPlayerNumMax(pHolder);
    for (s32 i = 0; i < playerNum; i++) {
        toPlayerActor(al::getPlayerActor(pHolder, i))->endPausePlayerAmiiboDirector();
    }
}

/**
 * @brief Color an actor after the character of the player of a sensor.
 * @param pActor Actor to color.
 * @param pSensor Sensor of the player.
 * @param pAnimName Name of the color material animation.
 */
void setPlayerColorAnimBySensor(al::LiveActor* pActor, const al::HitSensor* pSensor,
                                const char* pAnimName) {
    setPlayerColorAnimByCharacterType(pActor, getPlayerCharaType(pSensor), pAnimName);
}

/**
 * @brief Color an actor after a character.
 * @param pActor Actor to color.
 * @param characterType Character type.
 * @param pAnimName Name of the color material animation.
 */
void setPlayerColorAnimByCharacterType(al::LiveActor* pActor, s32 characterType,
                                       const char* pAnimName) {
    if (al::isMtpAnimExist(pActor, pAnimName)) {
        al::startMtpAnimAndSetFrameAndStop(pActor, pAnimName, characterType + 1.0f);
    }
}

/**
 * @brief Color an actor after the character of a control user.
 * @param pActor Actor to color.
 * @param userId Control user index.
 * @param pAnimName Name of the color material animation.
 */
void setPlayerColorAnimByControlUserId(al::LiveActor* pActor, s32 userId, const char* pAnimName) {
    s32 characterType = getControlUserCharacterType(GameDataHolderAccessor(pActor), userId);
    setPlayerColorAnimByCharacterType(pActor, characterType, pAnimName);
}

/**
 * @brief Give an actor its default player color.
 * @param pActor Actor to color.
 * @param pAnimName Name of the color material animation.
 */
void setPlayerColorAnimDefault(al::LiveActor* pActor, const char* pAnimName) {
    if (al::isMtpAnimExist(pActor, pAnimName)) {
        al::startMtpAnimAndSetFrameAndStop(pActor, pAnimName, 0.0f);
    }
}

/**
 * @brief Give the player of a sensor its limited air actions back.
 * @param pSensor Sensor of the player.
 */
void resetPlayerAirLimitedAction(al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->resetAirLimitedAction();
}

/**
 * @brief Create the uniform block of the invincibility look of an actor.
 * @param pActor Actor to set up.
 */
void createInvincibleUbo(al::LiveActor* pActor) {
    sead::Graphics::instance()->lockDrawContext();

    const nn::g3d::ResShadingModel* shadingModel =
        al::ShaderHolder::instance()->getShadingModel("RenderInvincible");
    al::UniformBlockAssignArray* assignArray =
        pActor->getModelKeeper()->getModelCafe()->getModelG3D()->getUniformBlockAssignArray();
    al::UniformBlock* uniformBlock = al::createUniformBlock(&cInvincibleUboLayout, 1, nullptr, 2);
    for (s32 i = 0; i < uniformBlock->getBufferNum(); i++) {
        uniformBlock->setValueRef(0, sead::Color4f(0.2f, 0.2f, 0.0f, 1.0f));
        uniformBlock->flushCurrentBuffer();
        uniformBlock->swap();
    }

    al::UniformBlockAssign* assign = assignArray->emplaceBack();
    assign->mUniformBlock = uniformBlock;
    agl::UniformBlockLocation* location = new agl::UniformBlockLocation();
    agl::g3d::ShaderUtilG3D::search(location, shadingModel, shadingModel->GetProgram(0),
                                    "cInvincible");
    assign->mLocation = location;

    sead::Graphics::instance()->unlockDrawContext();
}

/**
 * @brief Create the invincibility uniform blocks of an actor and its sub actors.
 * @param pActor Actor to set up.
 */
void createInvincibleUboWithSubActor(al::LiveActor* pActor) {
    createInvincibleUbo(pActor);

    al::SubActorKeeper* subActorKeeper = pActor->getSubActorKeeper();
    if (subActorKeeper == nullptr) {
        return;
    }

    s32 subActorNum = subActorKeeper->getSubActorNum();
    for (s32 i = 0; i < subActorNum; i++) {
        al::SubActorInfo* info = subActorKeeper->getSubActorInfo(i);
        if (!isDrawPartsSubActor(info)) {
            createInvincibleUbo(info->mSubActor);
        }
    }
}

/**
 * @brief Set the invincibility color of an actor.
 * @param pActor Actor to color.
 * @param rColor Color to set.
 */
void setInvincibleColor(al::LiveActor* pActor, const sead::Color4f& rColor) {
    setInvincibleColorImpl(pActor, rColor);
}

/**
 * @brief Set the invincibility color of an actor and its sub actors.
 * @param pActor Actor to color.
 * @param rColor Color to set.
 */
void setInvincibleColorWithSubActor(al::LiveActor* pActor, const sead::Color4f& rColor) {
    setInvincibleColorImpl(pActor, rColor);

    al::SubActorKeeper* subActorKeeper = pActor->getSubActorKeeper();
    if (subActorKeeper == nullptr) {
        return;
    }

    s32 subActorNum = subActorKeeper->getSubActorNum();
    for (s32 i = 0; i < subActorNum; i++) {
        al::SubActorInfo* info = subActorKeeper->getSubActorInfo(i);
        if (!isDrawPartsSubActor(info)) {
            setInvincibleColorImpl(info->mSubActor, rColor);
        }
    }
}

/**
 * @brief Invincibility color of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The invincibility color.
 */
const sead::Color4f& getPlayerInvincibleColor(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getInvincibleColor();
}

/**
 * @brief Whether the invincible look of the player of a sensor shows.
 * @param pSensor Sensor of the player.
 * @return True if the invincible model appears.
 */
bool isPlayerInvincibleModelAppear(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->isInvincibleModelAppear();
}

/**
 * @brief Let the player of a sensor show its button mash effect.
 * @param pSensor Sensor of the player.
 */
void validatePlayerMash(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->validateMash();
}

/**
 * @brief Stop the button mash effect of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void invalidatePlayerMash(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->invalidateMash();
}


/**
 * @brief Whether the player of a sensor shows its button mash effect.
 * @param pSensor Sensor of the player.
 * @return True if the mash effect is valid.
 */
bool isPlayerMash(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getModelHolder()->isMash();
}

/**
 * @brief Whether the giant player of a sensor walks.
 * @param pSensor Sensor of the player.
 * @return True if the giant player moves slowly on the ground.
 */
bool isGiantPlayerWalking(const al::HitSensor* pSensor) {
    return isPlayerGiant(pSensor) &&
           PlayerActionTypeFunc::isGroundMove(
               getPlayer(getSensorPlayerActor(pSensor))->getActionGraph()) &&
           al::getSklAnimBlendWeight(
               getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(), 2) < 0.5f;
}

/**
 * @brief Whether the giant player of a sensor runs.
 * @param pSensor Sensor of the player.
 * @return True if the giant player moves fast on the ground.
 */
bool isGiantPlayerRunning(const al::HitSensor* pSensor) {
    return isPlayerGiant(pSensor) &&
           PlayerActionTypeFunc::isGroundMove(
               getPlayer(getSensorPlayerActor(pSensor))->getActionGraph()) &&
           al::getSklAnimBlendWeight(
               getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(), 2) >= 0.5f;
}

/**
 * @brief Whether the giant player of a sensor lands.
 * @param pSensor Sensor of the player.
 * @return True if the giant player lands.
 */
bool isGiantPlayerLanding(const al::HitSensor* pSensor) {
    return isPlayerGiant(pSensor) && getSensorPlayerActor(pSensor)->isGiantLanding();
}

/**
 * @brief End the dash panel boost of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void cancelPanelDash(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->clearPanelDash();
}

/**
 * @brief End the dash panel boost of a player.
 * @param pActor Actor of the player.
 */
void cancelPanelDash(al::LiveActor* pActor) {
    toPlayerActor(pActor)->clearPanelDash();
}

/**
 * @brief Show the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void showPlayer(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelVisibility()->show();
}

/**
 * @brief Show a player.
 * @param pActor Actor of the player.
 */
void showPlayer(const al::LiveActor* pActor) {
    toPlayerActor(pActor)->getModelVisibility()->show();
}

/**
 * @brief Hide the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void hidePlayer(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelVisibility()->hide();
}

/**
 * @brief Hide a player.
 * @param pActor Actor of the player.
 */
void hidePlayer(const al::LiveActor* pActor) {
    toPlayerActor(pActor)->getModelVisibility()->hide();
}

/**
 * @brief Show the fur of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void showPlayerFur(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->showFur();
}

/**
 * @brief Hide the fur of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void hidePlayerFur(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->hideFur();
}

/**
 * @brief Enable the dynamics (skirt, tail, ...) of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void validatePlayerDynamics(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->validateDynamics();
}

/**
 * @brief Disable the dynamics (skirt, tail, ...) of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void invalidatePlayerDynamics(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->invalidateDynamics();
}

/**
 * @brief Reset the skirt and tail dynamics of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void resetPlayerDynamics(const al::HitSensor* pSensor) {
    getSensorPlayerActor(pSensor)->getModelHolder()->resetSkirtDynamics();
    getSensorPlayerActor(pSensor)->getModelHolder()->resetTailDynamics();
}

/**
 * @brief Reset the skirt and tail dynamics of a player.
 * @param pActor Actor of the player.
 */
void resetPlayerDynamics(const al::LiveActor* pActor) {
    toPlayerActor(pActor)->getModelHolder()->resetSkirtDynamics();
    toPlayerActor(pActor)->getModelHolder()->resetTailDynamics();
}

/**
 * @brief Let the player of a sensor take damage again.
 * @param pSensor Sensor of the player.
 */
void validatePlayerDamage(const al::HitSensor* pSensor) {
    validatePlayerDamage(getSensorPlayerActor(pSensor));
}

/**
 * @brief Let a player take damage again.
 * @param pActor Actor of the player.
 */
void validatePlayerDamage(const al::LiveActor* pActor) {
    getPlayer(pActor)->getDamageInvalidater()->reset();
}

/**
 * @brief Switch the effect material of the player of a sensor for route pipes.
 * @param pSensor Sensor of the player.
 * @param isInRouteDokan Whether the player is in a route pipe.
 */
void updateMaterialRouteDokan(al::HitSensor* pSensor, bool isInRouteDokan) {
    al::updateEffectMaterialRouteDokan(getSensorPlayerActor(pSensor), isInRouteDokan);
    al::updateEffectMaterialRouteDokan(
        getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(), isInRouteDokan);
}

/**
 * @brief Set the material of the player of a sensor while it slides down a goal pole.
 * @param pSensor Sensor of the player.
 * @param pMaterialCode Material of the goal pole.
 */
void updateMaterialGoalPole(al::HitSensor* pSensor, const char* pMaterialCode) {
    PlayerActor* player = getSensorPlayerActor(pSensor);
    al::tryUpdateEffectMaterialCode(player, pMaterialCode);
    al::tryUpdateSeMaterialCode(player, pMaterialCode);

    al::LiveActor* model = getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel();
    al::tryUpdateEffectMaterialCode(model, pMaterialCode);
    al::tryUpdateSeMaterialCode(model, pMaterialCode);
}

/**
 * @brief Give the alive watcher to the audio of every player.
 * @param pWatcher The alive watcher.
 * @param pHolder Holder of the players.
 */
void setAliveWatcherToAudio(PlayerAliveWatcher* pWatcher, const al::PlayerHolder* pHolder) {
    for (s32 i = 0; i < pHolder->getPlayerNum(); i++) {
        toPlayerActor(pHolder->getPlayer(i))->getAudio()->setAliveWatcher(pWatcher);
    }
}

/**
 * @brief Clear the collision results of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void clearPlayerCollisionInfo(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))->getCollision()->clear();
}

/**
 * @brief Clear the external push of the player of a sensor.
 * @param pSensor Sensor of the player.
 */
void clearPlayerExPush(const al::HitSensor* pSensor) {
    getPlayer(getSensorPlayerActor(pSensor))->getCollision()->clearPush();
}

/**
 * @brief Space above the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The ceiling check level.
 */
s32 getPlayerCeilingCheckLevel(const al::HitSensor* pSensor) {
    return getPlayer(getSensorPlayerActor(pSensor))->getCeilingCheck()->getSpaceLevel();
}

/**
 * @brief Stop updating and drawing the effects of a player's model.
 * @param pActor Actor of the player.
 */
void offCalcAndDrawPlayerEffect(al::LiveActor* pActor) {
    al::offCalcAndDrawEffect(toPlayerActor(pActor)->getModelHolder()->getCurrentModel());
}

/**
 * @brief Update and draw the effects of a player's model again.
 * @param pActor Actor of the player.
 */
void onCalcAndDrawPlayerEffect(al::LiveActor* pActor) {
    al::onCalcAndDrawEffect(toPlayerActor(pActor)->getModelHolder()->getCurrentModel());
}

/**
 * @brief Enable the effects of a player.
 * @param pActor Actor of the player.
 */
void validatePlayerEffect(al::LiveActor* pActor) {
    toPlayerActor(pActor)->validateEffect();
}

/**
 * @brief Disable the effects of a player.
 * @param pActor Actor of the player.
 */
void invalidatePlayerEffect(al::LiveActor* pActor) {
    toPlayerActor(pActor)->invalidateEffect();
}

/**
 * @brief Whether the effects of a player are enabled.
 * @param pActor Actor of the player.
 * @return True if the effects are enabled.
 */
bool isValidPlayerEffect(al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isValidEffect();
}

/**
 * @brief Enable the water effects of a player.
 * @param pActor Actor of the player.
 */
void validatePlayerWaterEffect(al::LiveActor* pActor) {
    toPlayerActor(pActor)->validateWaterEffect();
}

/**
 * @brief Disable the water effects of a player.
 * @param pActor Actor of the player.
 */
void invalidatePlayerWaterEffect(al::LiveActor* pActor) {
    toPlayerActor(pActor)->invalidateWaterEffect();
}

/**
 * @brief Whether the water effects of a player are enabled.
 * @param pActor Actor of the player.
 * @return True if the water effects are enabled.
 */
bool isValidPlayerWaterEffect(al::LiveActor* pActor) {
    return toPlayerActor(pActor)->isValidWaterEffect();
}

/**
 * @brief Delete the effects of every player and of Bowser Jr.
 * @param pActor Actor used to reach the players.
 */
void killAllPlayersEffect(al::LiveActor* pActor) {
    al::PlayerHolder* playerHolder = pActor->getSceneInfo()->playerHolder;
    for (s32 i = 0; i < playerHolder->getPlayerNum(); i++) {
        killAllPlayerEffect(playerHolder->getPlayer(i));
    }

    auto* koopaJr = al::tryGetSceneObj<PlayerKoopaJr>(pActor, SceneObjID_PlayerKoopaJr);
    if (koopaJr != nullptr) {
        al::tryDeleteEmitterAndParticleAll(koopaJr);
    }
}

/**
 * @brief Delete the effects of a player and its model.
 * @param pActor Actor of the player.
 */
void killAllPlayerEffect(al::LiveActor* pActor) {
    al::tryDeleteEmitterAndParticleAll(toPlayerActor(pActor)->getModelHolder()->getCurrentModel());
    al::tryDeleteEmitterAndParticleAll(pActor);
}

/**
 * @brief Length of the shadow of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @return The shadow length.
 */
f32 getPlayerShadowLength(const al::HitSensor* pSensor) {
    return getSensorPlayerActor(pSensor)->getModelHolder()->getShadowLength();
}

/**
 * @brief Light a pre-pass light of the model of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param pName Name of the light.
 */
void appearPlayerPrePassLight(const al::HitSensor* pSensor, const char* pName) {
    if (al::isExistPrePassLight(getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                                pName)) {
        al::appearPrePassLight(getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                               pName, -1);
    }
}

/**
 * @brief Put out a pre-pass light of the model of the player of a sensor.
 * @param pSensor Sensor of the player.
 * @param pName Name of the light.
 */
void killPlayerPrePassLight(const al::HitSensor* pSensor, const char* pName) {
    if (al::isExistPrePassLight(getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                                pName)) {
        al::killPrePassLight(getSensorPlayerActor(pSensor)->getModelHolder()->getCurrentModel(),
                             pName, 1);
    }
}

}  // namespace rc

namespace alProjectInterface {

/**
 * @brief Position of the main player.
 * @return The main player's position, or the origin if there is none.
 */
const sead::Vector3f& getPlayerPos() {
    if (sMainPlayerActor != nullptr) {
        return al::getTrans(sMainPlayerActor);
    }

    return sead::Vector3f::zero;
}

}  // namespace alProjectInterface

namespace rc {

/**
 * @brief Move a player, or ask its binder to move it.
 * @param pActor Actor of the player.
 * @param rTrans Position to move to.
 */
void setPlayerTrans(al::LiveActor* pActor, const sead::Vector3f& rTrans) {
    al::HitSensor* bindSensor = toPlayerActor(pActor)->getBindSensor();
    if (bindSensor != nullptr) {
        sendMsgDebugMovePosition(bindSensor, al::getHitSensor(pActor, "Body"), rTrans);
        return;
    }

    toPlayerActor(pActor)->getProperty()->mTrans = rTrans;
}

/**
 * @brief Set the front direction of a player.
 * @param pActor Actor of the player.
 * @param rFront Front direction.
 */
void setPlayerFrontVec(al::LiveActor* pActor, const sead::Vector3f& rFront) {
    toPlayerActor(pActor)->getProperty()->setFrontVec(rFront);
}

/**
 * @brief Set the up direction of a player.
 * @param pActor Actor of the player.
 * @param rUp Up direction.
 */
void setPlayerUpVec(al::LiveActor* pActor, const sead::Vector3f& rUp) {
    toPlayerActor(pActor)->getProperty()->setUpVec(rUp);
}

/**
 * @brief Set the velocity of a player.
 * @param pActor Actor of the player.
 * @param rVelocity Velocity.
 */
void setPlayerVelocity(al::LiveActor* pActor, const sead::Vector3f& rVelocity) {
    toPlayerActor(pActor)->getProperty()->mVelocity = rVelocity;
}

/**
 * @brief Make every player use the old or the new parameters.
 * @param pHolder Holder of the players.
 * @param isOld Whether to use the old parameters.
 */
void setPlayerUseOldParams(al::PlayerHolder* pHolder, bool isOld) {
    for (s32 i = 0; i < al::getPlayerNumMax(pHolder); i++) {
        toPlayerActor(al::getPlayerActor(pHolder, i))->getConstParam()->setOverride(isOld);
    }
}

/**
 * @brief Make every player hold with the input.
 * @param pHolder Holder of the players.
 */
void setPlayerUseInputForHold(al::PlayerHolder* pHolder) {
    for (s32 i = 0; i < al::getPlayerNumMax(pHolder); i++) {
        toPlayerActor(al::getPlayerActor(pHolder, i))->setUseInputForHold(true);
    }

    sIsUsingHoldInput = true;
}

/**
 * @brief Make every player hold with the key config.
 * @param pHolder Holder of the players.
 */
void setPlayerUseKeyConfigForHold(al::PlayerHolder* pHolder) {
    for (s32 i = 0; i < al::getPlayerNumMax(pHolder); i++) {
        toPlayerActor(al::getPlayerActor(pHolder, i))->setUseInputForHold(false);
    }

    sIsUsingHoldInput = false;
}

/**
 * @brief Switch the source a player uses for holding.
 * @param pActor Actor of the player, may be nullptr.
 */
void setRequestToggleInputForHold(al::LiveActor* pActor) {
    if (pActor == nullptr) {
        return;
    }

    bool isUseInput = !sIsUsingHoldInput;
    toPlayerActor(pActor)->setUseInputForHold(isUseInput);
    sIsUsingHoldInput = isUseInput;
}

/**
 * @brief Whether the player of a pad port may change its character.
 * @param pHolder Holder of the players.
 * @param port Pad port.
 * @return True if the player stands on the ground, unhurt and unbound.
 */
bool isPlayerChangeAllowed(al::PlayerHolder* pHolder, s32 port) {
    al::LiveActor* player = tryFindPlayerFromInputPort(pHolder, port, false);
    if (player == nullptr) {
        return false;
    }

    if (getActionObserver(player)->isOnGround() && !toPlayerActor(player)->isDamageTrigOn() &&
        !getActionObserver(player)->isInBind()) {
        return true;
    }

    return false;
}

/**
 * @brief Character after or before a player's character in the selection.
 * @param pHolder The game data holder.
 * @param pActor Actor of the player.
 * @param isNext Whether to take the next character instead of the previous one.
 * @return The character type.
 */
s32 getNextValidCharType(GameDataHolder* pHolder, al::LiveActor* pActor, bool isNext) {
    s32 characterType = getPlayerCharaType(pActor);
    if (isNext) {
        return characterType > 3 ? 0 : characterType + 1;
    }

    return characterType - 1 < 0 ? 4 : characterType - 1;
}

/**
 * @brief Replace a player by an unused player of another character.
 * @param pHolder The game data holder.
 * @param pActor Actor of the player.
 * @param characterType Character type to change to.
 * @return The new player, or nullptr if none is available.
 */
PlayerActor* changeCharType(GameDataHolder* pHolder, al::LiveActor* pActor, s32 characterType) {
    PlayerActor* player = toPlayerActor(pActor);
    s32 oldCharacterType = getPlayerCharaType(pActor);
    auto* stocker = al::getSceneObj<PlayerStocker>(pActor, SceneObjID_PlayerStocker);
    PlayerActor* newPlayer = stocker->getUnusedPlayer(characterType);
    if (newPlayer == nullptr) {
        return nullptr;
    }

    newPlayer->setViewMtx(player->getViewMtx());
    newPlayer->replaceInputPort(player->getInputPort());
    PlayerProperty* property = newPlayer->getProperty();
    property->mTrans = al::getTrans(pActor);
    al::onAreaTarget(newPlayer);

    sead::Vector3f front;
    al::calcFrontDir(&front, pActor);
    newPlayer->getProperty()->setFrontVec(front);
    newPlayer->updatePosture();
    newPlayer->appear();
    newPlayer->getModelHolder()->appear();
    newPlayer->getPlayer()->getDamageInvalidater()->invalidateDamage(10);
    al::startHitReaction(newPlayer, "ダブルマリオ出現");

    PlayerFigureDirector* figureDirector = getPlayer(pActor)->getFigureDirector();
    s32 figure = figureDirector->getFigure();
    if (figureDirector->isNextFigureRequested()) {
        figure = figureDirector->getNextFigure();
    }

    switch (figure) {
    case EPlayerFigure::Super:
        changeToSuperMarioForce(newPlayer);
        break;
    case EPlayerFigure::Mini:
        changeToMiniMarioForce(newPlayer);
        break;
    case EPlayerFigure::Fire:
        changeToFireMarioForce(newPlayer);
        break;
    case EPlayerFigure::Climb:
        changeToClimbMarioForce(newPlayer);
        break;
    case EPlayerFigure::RaccoonDog:
        changeToRaccoonDogMarioForce(newPlayer);
        break;
    case EPlayerFigure::Boomerang:
        changeToBoomerangMarioForce(newPlayer);
        break;
    case EPlayerFigure::RaccoonDogWhite:
        changeToRaccoonDogWhiteMarioForce(newPlayer);
        break;
    case EPlayerFigure::Manekineko:
        changeToClimbMarioSpecialForce(newPlayer);
        break;
    case EPlayerFigure::ClimbGiga:
        changeToClimbGigaMarioForce(newPlayer);
        break;
    case EPlayerFigure::ClimbWhite:
        changeToClimbWhiteMarioForce(newPlayer);
        break;
    default:
        break;
    }

    al::setSeSeqLocalVariableDefault(newPlayer, 1, figure);
    al::startSe(newPlayer, "PgDoublePlayerAppear");
    setPlayerVanishDying(pActor);

    GameDataHolderWriter writer(pHolder);
    PlayerEntryFunction::retirePlayer(writer, 0);
    PlayerEntryFunction::entryPlayer(writer, 0, characterType);
    PlayerAliveWatcher::getPlayerAliveWatcher(pActor)->deactivatePlayer(oldCharacterType);
    return newPlayer;
}

/**
 * @brief Ask a player to clear its fling pole dash flag.
 * @param pActor Actor of the player, may be nullptr.
 */
void tryRequestClearFlingPoleDashFlag(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        toPlayerActor(pActor)->requestFlingPoleFlagClear();
    }
}

/**
 * @brief Ask a player to clear its dash flag.
 * @param pActor Actor of the player, may be nullptr.
 */
void tryRequestClearDashFlag(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        toPlayerActor(pActor)->requestDashFlagClear();
    }
}

/**
 * @brief Make the next landing of a player silent.
 * @param pActor Actor of the player, may be nullptr.
 */
void setSilentLand(al::LiveActor* pActor) {
    if (pActor != nullptr) {
        toPlayerActor(pActor)->setSilentLand();
    }
}

/**
 * @brief Cancel the actions of every living player for a demo.
 * @param pActor Actor used to reach the players.
 */
void cancelAllPlayersForDemo(al::LiveActor* pActor) {
    al::PlayerHolder* playerHolder = pActor->getSceneInfo()->playerHolder;
    for (s32 i = 0; i < playerHolder->getPlayerNum(); i++) {
        al::LiveActor* player = playerHolder->tryGetPlayer(i);
        if (player != nullptr && !player->getFlags()->isDead) {
            toPlayerActor(player)->cancelForDemo();
        }
    }
}

/**
 * @brief Forget the amiibo a player scanned.
 * @param pActor Actor of the player.
 */
void clearScannedAmiiboList(al::LiveActor* pActor) {
    if (isReallyPlayerActor(pActor)) {
        toPlayerActor(pActor)->getAmiiboDirector()->clear();
    }
}

/**
 * @brief Fill the data of a boss play report (does nothing).
 * @param pTask The report task.
 * @param pHolder The game data holder.
 */
void setBossPlayReportTaskData(al::BossPlayReportTask* pTask, const GameDataHolder* pHolder) {}

/**
 * @brief Register a boss play report (does nothing).
 * @param pNetworkSystem The network system.
 * @param pHolder The game data holder.
 * @return Always false.
 */
bool tryRegisterBossPlayReport(al::NetworkSystem* pNetworkSystem, const GameDataHolder* pHolder) {
    return false;
}

/**
 * @brief Name of a floor code.
 * @param floorCode Floor code.
 * @return The floor code name.
 */
const char* getFloorCodeName(s32 floorCode) {
    return sFloorCodeName[floorCode];
}

/**
 * @brief Second floor code of a triangle (unused).
 * @param rTriangle The triangle.
 * @return Always 0.
 */
s32 getFloor2Code(const al::Triangle& rTriangle) {
    return 0;
}

/**
 * @brief Name of a wall code.
 * @param wallCode Wall code.
 * @return The wall code name.
 */
const char* getWallCodeName(s32 wallCode) {
    return sWallCodeName[wallCode];
}

/**
 * @brief Wall code of a triangle.
 * @param rTriangle The triangle.
 * @return The wall code.
 */
s32 getWallCode(const al::Triangle& rTriangle) {
    return getTriangleCode(rTriangle, "WallCode", cWallCodeOffset);
}

/**
 * @brief Name of a camera code.
 * @param cameraCode Camera code.
 * @return The camera code name.
 */
const char* getCameraCodeName(s32 cameraCode) {
    return sCameraCodeName[cameraCode];
}

/**
 * @brief Camera code of a triangle.
 * @param rTriangle The triangle.
 * @return The camera code.
 */
s32 getCameraCode(const al::Triangle& rTriangle) {
    return getTriangleCode(rTriangle, "CameraCode", cCameraCodeOffset);
}

/**
 * @brief Name of a material code.
 * @param materialCode Material code.
 * @return The material code name.
 */
const char* getMaterialCodeName(s32 materialCode) {
    return sMaterialCodeName[materialCode];
}

/**
 * @brief Whether an actor touches fire.
 * @param pActor The actor.
 * @return True if the ground, a wall or the ceiling it touches burns.
 */
bool isCollidedDamageFire(const al::LiveActor* pActor) {
    return isCollidedFloorCode(pActor, "DamageFire");
}

/**
 * @brief Whether an actor touches poison.
 * @param pActor The actor.
 * @return True if the ground, a wall or the ceiling it touches is poison.
 */
bool isCollidedPoison(const al::LiveActor* pActor) {
    return isCollidedFloorCode(pActor, "Poison");
}

/**
 * @brief Whether an actor touches slowing ink.
 * @param pActor The actor.
 * @return True if the ground, a wall or the ceiling it touches is slowing ink.
 */
bool isCollidedInkSlow(const al::LiveActor* pActor) {
    return isCollidedFloorCode(pActor, "InkSlow");
}

/**
 * @brief Whether an actor touches spikes.
 * @param pActor The actor.
 * @return True if the ground, a wall or the ceiling it touches is spiky.
 */
bool isCollidedNeedle(const al::LiveActor* pActor) {
    return isCollidedFloorCode(pActor, "Needle");
}

/**
 * @brief Effect code to use for a material.
 * @param pCode The floor code.
 * @param pMaterialCode The material code.
 * @return The floor code for poison, else the material code.
 */
const char* getEffectCodeName(const char* pCode, const char* pMaterialCode) {
    return al::isEqualString(pCode, "Poison") ? pCode : pMaterialCode;
}

}  // namespace rc

namespace alProjectInterface {

/**
 * @brief Name used to pick an effect for a collision code pair.
 * @param effectCode The effect code of the touched collision.
 * @param materialCode The material code of the touched collision.
 * @return "Poison" for poison collisions, otherwise the material code's name.
 */
const char* getEffectCodeName(u32 effectCode, u32 materialCode) {
    if (effectCode == 3) {
        return "Poison";
    }

    return sMaterialCodeName[static_cast<s32>(materialCode)];
}

}  // namespace alProjectInterface
