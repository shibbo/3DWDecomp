#include "Boss/DarkBowserUtil.hpp"

#include <cmath>
#include <cstring>

#include <math/seadMathCalcCommon.h>
#include <math/seadMatrix.h>
#include <nn/g3d/g3d_ModelObj.h>
#include <nn/g3d/g3d_Resources.h>
#include <prim/seadSafeString.h>

#include "Boss/DarkBowser.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/Joint/JointAimInfo.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Joint/JointSpringController.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Util/ActorModelUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Model/ModelKeeper.hpp"
#include "Library/Model/alModelCafe.hpp"
#include "Library/Obj/SkyProjection.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shadow/Common/ShadowUtil.hpp"
#include "MapObj/DisasterModeController.hpp"
#include "MapObj/EchoEmitterHolder.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Draw/GraphicsAreaDirector.hpp"
#include "Project/Effect/Core/EffectKeeper.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Project/Effect/Effect.hpp"
#include "Project/Effect/EffectInfo.hpp"
#include "Project/Model/SimpleModelG3D.hpp"
#include "Project/Rail/LinearCurve.hpp"
#include "Util/PlayerUtil.hpp"

namespace {

/**
 * @brief Number of rays cast by the laser: the centre one and one on each side of it.
 */
constexpr s32 cLaserRayNum = 5;

/**
 * @brief Rotates a local direction around the local Z axis.
 * @param pDir Direction to rotate in place.
 * @param rRotMtx Rotation to apply.
 */
inline void rotateDir(sead::Vector3f* pDir, const sead::Matrix33f& rRotMtx) {
    pDir->setMul(rRotMtx, *pDir);
}

/**
 * @brief Builds the rotation used to pitch a joint aim around its side axis.
 * @param pMtx Output rotation.
 * @param pitchDegree Pitch angle in degrees.
 */
inline void makePitchMtx(sead::Matrix33f* pMtx, f32 pitchDegree) {
    pMtx->makeR({0.0f, 0.0f, sead::Mathf::deg2rad(pitchDegree)});
}

/**
 * @brief Adds a spring controller to the optional output array.
 * @param pSprings Output array, may be nullptr.
 * @param pSpring Spring controller to add.
 */
inline void tryAddSpring(sead::PtrArray<al::JointSpringController>* pSprings,
                         al::JointSpringController* pSpring) {
    if (pSprings != nullptr) {
        pSprings->pushBack(pSpring);
    }
}

/**
 * @brief Initializes a hair spring, tightening it for the final battle model.
 * @param pActor Actor owning the joint.
 * @param pSprings Output array, may be nullptr.
 * @param pJointName Name of the joint.
 * @param isFinal Whether the final battle model is used.
 * @param friction Friction for the final battle model.
 * @param limitDegree Limit angle for the final battle model.
 */
inline void initHairSpring(al::LiveActor* pActor,
                           sead::PtrArray<al::JointSpringController>* pSprings,
                           const char* pJointName, bool isFinal, f32 friction, f32 limitDegree) {
    al::JointSpringController* spring = al::initJointSpringController(pActor, pJointName);

    if (isFinal) {
        spring->setFriction(friction);
        spring->setLimitDegree(limitDegree);
    }

    tryAddSpring(pSprings, spring);
}

/**
 * @brief Initializes an eyebrow spring, loosening it for the final battle model.
 * @param pActor Actor owning the joint.
 * @param pSprings Output array, may be nullptr.
 * @param pJointName Name of the joint.
 * @param isFinal Whether the final battle model is used.
 * @param finalFriction Friction for the final battle model.
 * @param friction Friction for the other models.
 * @param finalLimitDegree Limit angle for the final battle model.
 * @param limitDegree Limit angle for the other models.
 */
inline void initBrowSpring(al::LiveActor* pActor,
                           sead::PtrArray<al::JointSpringController>* pSprings,
                           const char* pJointName, bool isFinal, f32 finalFriction,
                           f32 friction, f32 finalLimitDegree, f32 limitDegree) {
    al::JointSpringController* spring = al::initJointSpringController(pActor, pJointName);
    spring->setStability(0.05f);
    spring->setFriction(isFinal ? finalFriction : friction);
    spring->setLimitDegree(isFinal ? finalLimitDegree : limitDegree);
    spring->setControlRate(1.0f);
    tryAddSpring(pSprings, spring);
}

/**
 * @brief Clips a laser segment at the water surface (y = 0) when it goes below it.
 * @param pEnd Segment end, moved onto the surface when clipped.
 * @param rStart Segment start.
 */
inline void clipAtWaterSurface(sead::Vector3f* pEnd, const sead::Vector3f& rStart) {
    sead::Vector3f diff = *pEnd - rStart;

    if (diff.dot(sead::Vector3f::ey) != 0.0f) {
        f32 rate = rStart.y / std::abs(diff.y);

        if (rate >= 0.0f && rate < 1.0f) {
            *pEnd = rStart + diff * rate;
        }
    }
}

}  // namespace

namespace DarkBowserUtil {

/**
 * @brief Calculates a point on an arc between two positions.
 * @param start Start of the arc.
 * @param end End of the arc.
 * @param height Height of the arc at its middle.
 * @param rate Progress along the arc in [0, 1].
 * @param moveEaseType Easing applied to the horizontal movement.
 * @param heightEaseType Easing applied to the height.
 * @return The point on the arc.
 */
sead::Vector3f calculateArc(sead::Vector3f start, sead::Vector3f end, f32 height, f32 rate,
                            s32 moveEaseType, s32 heightEaseType) {
    sead::Vector3f pos = sead::Vector3f::zero;
    al::lerpVec(&pos, start, end, al::easeByType(rate, moveEaseType));
    f32 heightRate = al::easeByType(rate, heightEaseType);
    pos.y += std::sin(sead::Mathf::deg2rad(heightRate * 180.0f)) * height;
    return pos;
}

/**
 * @brief Starts disaster mode if the disaster controller exists.
 * @param pHolder Scene object holder user.
 */
void startDisasterMode(const al::IUseSceneObjHolder* pHolder) {
    DisasterModeController* controller = DisasterModeController::tryGetController(pHolder);

    if (controller != nullptr) {
        controller->begin(false);
    }
}

/**
 * @brief Ends disaster mode if the disaster controller exists.
 * @param pHolder Scene object holder user.
 */
void endDisasterMode(const al::IUseSceneObjHolder* pHolder) {
    DisasterModeController* controller = DisasterModeController::tryGetController(pHolder);

    if (controller != nullptr) {
        controller->end();
    }
}

/**
 * @brief Checks whether disaster mode is active.
 * @param pHolder Scene object holder user.
 * @return Whether the disaster controller exists and is in disaster mode.
 */
bool isDisasterMode(const al::IUseSceneObjHolder* pHolder) {
    DisasterModeController* controller = DisasterModeController::tryGetController(pHolder);
    return controller != nullptr && controller->isDisasterMode();
}

/**
 * @brief Sets the day/night blend shader parameter on every material of a sky.
 * @param pSky Sky to edit.
 * @param percentage Blend amount.
 */
void setSkyboxBlendPercentage(al::SkyProjection* pSky, f32 percentage) {
    al::SimpleModelG3D* model = pSky->getModelKeeper()->getModelCafe()->getModelG3D();
    s32 shapeNum = model->getModelObj()->GetNumShapes();

    for (s32 i = 0; i < shapeNum; i++) {
        nn::g3d::ModelObj* modelObj = model->getModelObj();
        const nn::g3d::ShapeObj* shape = modelObj->GetShape(i);
        nn::g3d::MaterialObj* material =
            modelObj->GetMaterial(shape->GetResource()->GetMaterialIndex());
        s32 index = material->GetResource()->FindShaderParamIndex("uBlendPercentage");

        if (index != -1) {
            *material->EditShaderParam<f32>(index) = percentage;
        }
    }
}

/**
 * @brief Sets the priority of the graphics areas with a given placement ID.
 * @param pActor Actor used to access the scene.
 * @param pPlacementId Placement ID of the areas to edit.
 * @param priority New area priority.
 */
void graphicsAreaPrioritySet(al::LiveActor* pActor, const char* pPlacementId, s32 priority) {
    al::PlacementId placementId;
    al::GraphicsAreaDirector* director =
        pActor->getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector();
    s32 areaNum = director->getGraphicsAreaNum();

    for (s32 i = 0; i < areaNum; i++) {
        al::GraphicsAreaInfo* info = director->getGraphicsAreaInfoByIndex(i);

        if (info == nullptr) {
            continue;
        }

        if (!al::tryGetPlacementID(&placementId, info->getAreaObj()->getPlacementInfo())) {
            continue;
        }

        if (strcmp(placementId.mPlacementID, pPlacementId) == 0) {
            const_cast<al::AreaObj*>(info->getAreaObj())->mPriority = priority;
        }
    }
}

/**
 * @brief Switches the skies and graphics areas back to day.
 * @param pActor Actor used to access the scene.
 * @param pSkyDay Day sky, may be nullptr.
 * @param pSkyNight Disaster sky, may be nullptr.
 * @param isInstant Whether the rain effect is removed immediately.
 */
void setSkyToDay(al::LiveActor* pActor, al::SkyProjection* pSkyDay, al::SkyProjection* pSkyNight,
                 bool isInstant) {
    pActor->getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpStep(1);
    graphicsAreaPrioritySet(pActor, "obj0", -1);

    if (isInstant) {
        al::tryDeleteEffect(pActor, "EffectObjDarkBowserRain");
    }

    if (pSkyDay != nullptr) {
        pSkyDay->appear();
    }

    if (pSkyNight != nullptr) {
        pSkyNight->kill();
    }
}

/**
 * @brief Switches the skies and graphics areas to the disaster night.
 * @param pActor Actor used to access the scene.
 * @param pSkyDay Day sky, may be nullptr.
 * @param pSkyNight Disaster sky, may be nullptr.
 * @param isInstant Whether the rain effect is started immediately.
 */
void setSkyToNight(al::LiveActor* pActor, al::SkyProjection* pSkyDay,
                   al::SkyProjection* pSkyNight, bool isInstant) {
    pActor->getSceneInfo()->graphicsSystemInfo->getGraphicsAreaDirector()->setLerpStep(1);
    graphicsAreaPrioritySet(pActor, "obj0", 1);

    if (isInstant) {
        al::emitEffect(pActor, "EffectObjDarkBowserRain", nullptr);
    }

    if (pSkyDay != nullptr) {
        pSkyDay->kill();
    }

    if (pSkyNight != nullptr) {
        pSkyNight->appear();
    }
}

/**
 * @brief Hides Fury Bowser's model, shadows and health bar.
 * @param pDarkBowser Fury Bowser.
 */
void hideDarkBowser(DarkBowser* pDarkBowser) {
    al::hideModelIfShow(pDarkBowser);
    al::hideShadow(pDarkBowser);
    al::hideShadowDepth(pDarkBowser);
    pDarkBowser->hideHealthBar();
}

/**
 * @brief Shows Fury Bowser's model, shadows and health bar.
 * @param pDarkBowser Fury Bowser.
 */
void showDarkBowser(DarkBowser* pDarkBowser) {
    al::showModelIfHide(pDarkBowser);
    al::showShadow(pDarkBowser);
    al::showShadowDepth(pDarkBowser);
    pDarkBowser->showHealthBar();
}

/**
 * @brief Initializes the aim chains (body and beam) and the hair springs of Fury Bowser.
 * @param pActor Fury Bowser.
 * @param rInfo Actor init info.
 * @param pSuffix Archive suffix (unused).
 * @param rChainA Body aim chain.
 * @param rChainB Beam aim chain.
 * @param pSprings Output array for the hair springs, may be nullptr.
 */
void initDarkBowserJointControllers(al::LiveActor* pActor, const al::ActorInitInfo& rInfo,
                                    const char* pSuffix, ControlledJointChain& rChainA,
                                    ControlledJointChain& rChainB,
                                    sead::PtrArray<al::JointSpringController>* pSprings) {
    al::initJointControllerKeeper(pActor, rChainA.mCapacity + 25);

    jointChainAppend(pActor, rChainA, "Face", 0.25f, -45.0f, 17.0f);
    jointChainAppend(pActor, rChainA, "Neck", 0.125f, -45.0f, 17.0f);
    jointChainAppend(pActor, rChainA, "Spine", 0.0625f, -45.0f, 32.0f);
    jointChainAppend(pActor, rChainA, "Pelvis", 0.0625f, 0.0f, 0.0f);
    jointChainAppend(pActor, rChainA, "Hip1", 0.0625f, 0.0f, 0.0f);

    jointChainAppend(pActor, rChainB, "Beam01", 0.25f, 0.0f, 360.0f);
    jointChainAppend(pActor, rChainB, "Face_P", 0.25f, -45.0f, 17.0f);
    jointChainAppend(pActor, rChainB, "Neck_P", 0.125f, -45.0f, 17.0f);
    jointChainAppend(pActor, rChainB, "Spine_P", 0.0625f, -45.0f, 32.0f);
    jointChainAppend(pActor, rChainB, "Hip1_P", 0.0625f, 0.0f, 0.0f);

    initDarkBowserHairJointController(
        pActor, pSprings,
        al::isEqualString("FinalKoopa", pActor->getModelKeeper()->getModelName()));
}

/**
 * @brief Adds a joint aim controller to a chain.
 * @param pActor Actor owning the joint.
 * @param rChain Chain to add the joint to.
 * @param pJointName Name of the joint.
 * @param interpoleRate Aim interpolation rate.
 * @param pitchDegree Pitch of the base aim direction, in degrees.
 * @param limitDegree Aim limit angle.
 * @return Index of the joint in the chain, or -1 if the joint does not exist.
 */
s32 jointChainAppend(const al::LiveActor* pActor, ControlledJointChain& rChain,
                     const char* pJointName, f32 interpoleRate, f32 pitchDegree,
                     f32 limitDegree) {
    if (!al::isExistJoint(pActor, pJointName)) {
        return -1;
    }

    ControlledJointChain::Entry& entry = rChain.mEntries[rChain.mCount++];
    sead::Vector3f aimDir = sead::Vector3f::ex;
    sead::Vector3f upDir = sead::Vector3f::ey;
    sead::Vector3f sideDir = sead::Vector3f::ez;
    sead::Matrix33f rotMtx;
    makePitchMtx(&rotMtx, pitchDegree);
    rotateDir(&aimDir, rotMtx);
    rotateDir(&upDir, rotMtx);

    entry.mAimInfo = new al::JointAimInfo();
    entry.mAimInfo->setBaseAimLocalDir(aimDir);
    entry.mAimInfo->setBaseSideLocalDir(sideDir);
    entry.mAimInfo->setBaseUpLocalDir(upDir);
    entry.mAimInfo->setInterpoleRate(interpoleRate);
    entry.mAimInfo->setLimitDegreeCircle(limitDegree);
    al::initJointAimController(pActor, entry.mAimInfo, pJointName);
    entry.mMtx = al::getJointMtxPtr(pActor, pJointName);
    return rChain.mCount - 1;
}

/**
 * @brief Initializes the hair and eyebrow spring controllers of Fury Bowser.
 * @param pActor Fury Bowser.
 * @param pSprings Output array for the springs, may be nullptr.
 * @param isFinal Whether the final battle model is used.
 */
void initDarkBowserHairJointController(al::LiveActor* pActor,
                                       sead::PtrArray<al::JointSpringController>* pSprings,
                                       bool isFinal) {
    initHairSpring(pActor, pSprings, "HairFront1", isFinal, 0.95f, 55.0f);
    initHairSpring(pActor, pSprings, "HairFront2", isFinal, 0.95f, 60.0f);
    initHairSpring(pActor, pSprings, "HairFront3", isFinal, 0.95f, 65.0f);
    initHairSpring(pActor, pSprings, "HairL1", isFinal, 0.94f, 55.0f);
    initHairSpring(pActor, pSprings, "HairL2", isFinal, 0.94f, 60.0f);
    initHairSpring(pActor, pSprings, "HairL3", isFinal, 0.94f, 65.0f);
    initHairSpring(pActor, pSprings, "HairR1", isFinal, 0.94f, 55.0f);
    initHairSpring(pActor, pSprings, "HairR2", isFinal, 0.94f, 60.0f);
    initHairSpring(pActor, pSprings, "HairR3", isFinal, 0.94f, 65.0f);
    initHairSpring(pActor, pSprings, "HairMiddle1", isFinal, 0.94f, 55.0f);
    initHairSpring(pActor, pSprings, "HairMiddle2", isFinal, 0.94f, 60.0f);
    initHairSpring(pActor, pSprings, "HairMiddle3", isFinal, 0.94f, 65.0f);

    initBrowSpring(pActor, pSprings, "BrowL1", isFinal, 0.8f, 0.7f, 10.0f, 2.5f);
    initBrowSpring(pActor, pSprings, "BrowL2", isFinal, 0.85f, 0.7f, 20.0f, 5.0f);
    initBrowSpring(pActor, pSprings, "BrowL3", isFinal, 0.85f, 0.7f, 25.0f, 10.0f);
    initBrowSpring(pActor, pSprings, "BrowR1", isFinal, 0.8f, 0.7f, 10.0f, 2.5f);
    initBrowSpring(pActor, pSprings, "BrowR2", isFinal, 0.85f, 0.7f, 20.0f, 5.0f);
    initBrowSpring(pActor, pSprings, "BrowR3", isFinal, 0.85f, 0.7f, 25.0f, 10.0f);
}

/**
 * @brief Initializes the joint controller keeper for a single chain.
 * @param pActor Actor owning the joints.
 * @param rChain Chain to make room for.
 */
void jointChainInit(const al::LiveActor* pActor, ControlledJointChain& rChain) {
    al::initJointControllerKeeper(pActor, rChain.mCapacity);
}

/**
 * @brief Changes the pitch of the base aim direction of one joint of a chain.
 * @param rChain Chain to edit.
 * @param index Index of the joint in the chain.
 * @param pitchDegree Pitch in degrees.
 */
void jointChainPitch(ControlledJointChain& rChain, s32 index, f32 pitchDegree) {
    sead::Vector3f aimDir = sead::Vector3f::ex;
    sead::Vector3f upDir = sead::Vector3f::ey;
    sead::Vector3f sideDir = sead::Vector3f::ez;
    sead::Matrix33f rotMtx;
    makePitchMtx(&rotMtx, pitchDegree);
    rotateDir(&aimDir, rotMtx);
    rotateDir(&upDir, rotMtx);

    rChain.mEntries[index].mAimInfo->setBaseAimLocalDir(aimDir);
    rChain.mEntries[index].mAimInfo->setBaseSideLocalDir(sideDir);
    rChain.mEntries[index].mAimInfo->setBaseUpLocalDir(upDir);
}

/**
 * @brief Changes the aim limit angle of one joint of a chain.
 * @param rChain Chain to edit.
 * @param index Index of the joint in the chain.
 * @param limitDegree Limit angle in degrees.
 */
void jointChainConstraint(ControlledJointChain& rChain, s32 index, f32 limitDegree) {
    rChain.mEntries[index].mAimInfo->setLimitDegreeCircle(limitDegree);
}

/**
 * @brief Aims every joint of a chain at a target.
 * @param rChain Chain to aim.
 * @param rTarget Target position.
 * @param interpoleRate Aim interpolation rate.
 */
void jointChainAim(ControlledJointChain& rChain, const sead::Vector3f& rTarget,
                   f32 interpoleRate) {
    for (s32 i = 0; i < rChain.mCount; i++) {
        rChain.mEntries[i].mAimInfo->setPowerRate(1.0f);
        rChain.mEntries[i].mAimInfo->setInterpoleRate(interpoleRate);
        rChain.mEntries[i].mAimInfo->setTargetPos(rTarget);
    }
}

/**
 * @brief Sets the aim target of every joint of a chain without changing the interpolation.
 * @param rChain Chain to aim.
 * @param rTarget Target position.
 */
void jointChaimAimNoInterpolate(ControlledJointChain& rChain, const sead::Vector3f& rTarget) {
    for (s32 i = 0; i < rChain.mCount; i++) {
        rChain.mEntries[i].mAimInfo->setTargetPos(rTarget);
    }
}

/**
 * @brief Sets the aim power rate of every joint of a chain.
 * @param rChain Chain to edit.
 * @param rate Power rate.
 */
void jointChainSetPowerRate(ControlledJointChain& rChain, f32 rate) {
    for (s32 i = 0; i < rChain.mCount; i++) {
        rChain.mEntries[i].mAimInfo->setPowerRate(rate);
    }
}

/**
 * @brief Stops aiming every joint of a chain.
 * @param rChain Chain to release.
 */
void jointChainRelease(ControlledJointChain& rChain) {
    for (s32 i = 0; i < rChain.mCount; i++) {
        rChain.mEntries[i].mAimInfo->setPowerRate(0.0f);
    }
}

/**
 * @brief Checks whether two directions face the same way on the horizontal plane.
 * @param rDirA First direction.
 * @param rDirB Second direction.
 * @param threshold Minimum cosine of the angle between the directions.
 * @return Whether the horizontal directions are within the threshold.
 */
bool isFacingWithinThreshold(const sead::Vector3f& rDirA, const sead::Vector3f& rDirB,
                             f32 threshold) {
    sead::Vector2f dirA = {rDirA.x, rDirA.z};
    sead::Vector2f dirB = {rDirB.x, rDirB.z};
    dirA.normalize();
    dirB.normalize();
    return dirA.dot(dirB) > threshold;
}

/**
 * @brief Attaches and emits the laser effects.
 * @param pActor Actor emitting the laser.
 * @param pParam Laser state.
 */
void startLaserEffects(al::LiveActor* pActor, LaserParam* pParam) {
    al::setEffectFollowMtxPtr(pActor, "LaserCap", &pParam->mMtx);
    al::setEffectFollowPosPtr(pActor, "LaserWaterHit", &pParam->mHitPos);
    al::setEffectFollowPosPtr(pActor, "LaserLandHit", &pParam->mHitPos);
    al::tryEmitEffect(pActor, "LaserMouth", nullptr);
    al::tryEmitEffect(pActor, "Laser", nullptr);
    al::tryEmitEffect(pActor, "LaserCap", nullptr);
}

/**
 * @brief Deletes the laser effects.
 * @param pActor Actor emitting the laser.
 */
void stopLaserEffects(al::LiveActor* pActor) {
    al::tryDeleteEffect(pActor, "LaserMouth");
    al::tryDeleteEffect(pActor, "Laser");
    al::tryDeleteEffect(pActor, "LaserCap");
    al::tryDeleteEffect(pActor, "LaserWaterHit");
    al::tryDeleteEffect(pActor, "LaserLandHit");
}

/**
 * @brief Casts the laser rays and finds the nearest hit position.
 * @param pActor Actor emitting the laser.
 * @param pAxes Laser side, up and front axes.
 * @param rOrigin Laser origin.
 * @param pHitPos Output hit position, left untouched when nothing is found.
 */
void calcLaserHitPos(al::LiveActor* pActor, const sead::Vector3f* pAxes,
                     const sead::Vector3f& rOrigin, sead::Vector3f* pHitPos) {
    sead::Vector3f hitPos;
    sead::Vector3f arrow;
    sead::Vector3f starts[cLaserRayNum] = {
        rOrigin,
        rOrigin + pAxes[0] * 600.0f,
        rOrigin - pAxes[0] * 600.0f,
        rOrigin + pAxes[1] * 600.0f,
        rOrigin - pAxes[1] * 600.0f,
    };
    sead::Vector3f ends[cLaserRayNum];
    arrow = pAxes[2] * 256000.0f;
    f32 nearestDist = sead::Mathf::maxNumber();
    bool isNoHit = true;
    sead::Vector3f nearestPos;

    for (s32 i = 0; i < cLaserRayNum; i++) {
        const sead::Vector3f& start = starts[i];
        al::HitSensor* sensor;

        if (!alCollisionUtil::getFirstCollisionSensorOnArrow(pActor, &hitPos, &sensor, start,
                                                             arrow, nullptr, nullptr)) {
            ends[i] = start + pAxes[2] * 256000.0f;
            continue;
        }

        if (!(start.y < hitPos.y)) {
            clipAtWaterSurface(&hitPos, start);
        }

        ends[i] = hitPos;
        isNoHit = false;
        f32 dist = (hitPos - start).dot(pAxes[2]);

        if (dist < nearestDist) {
            nearestPos = starts[0] + pAxes[2] * dist;
            nearestDist = dist;
        }
    }

    if (isNoHit) {
        for (s32 i = 0; i < cLaserRayNum; i++) {
            const sead::Vector3f& start = starts[i];
            ends[i] = start + pAxes[2] * 256000.0f;

            if (!(start.y < ends[i].y)) {
                clipAtWaterSurface(&ends[i], start);

                f32 rate = (256000.0f - start.y) / std::abs(ends[i].y - start.y);

                if (rate >= 0.0f && rate < 1.0f) {
                    ends[i] = start + (ends[i] - start) * rate;
                }
            }
        }

        nearestPos = ends[0];
        nearestDist = (nearestPos - starts[0]).length();
    }

    if (nearestDist != sead::Mathf::maxNumber()) {
        *pHitPos = nearestPos;
    }
}

/**
 * @brief Plays the laser hit effects, sounds and beam models.
 * @param pActor Actor emitting the laser.
 * @param pIsHitLand Whether the land hit effect is playing.
 * @param pHitFrame Frames since the laser started hitting.
 * @param pParam Laser state.
 */
void playLaserEffects(al::LiveActor* pActor, bool* pIsHitLand, s32* pHitFrame,
                      LaserParam* pParam) {
    if (!*pIsHitLand) {
        al::tryDeleteEffect(pActor, "LaserWaterHit");
        al::tryEmitEffect(pActor, "LaserLandHit", nullptr);
        *pHitFrame = 0;
        *pIsHitLand = true;
    }

    rc::emitEcho(pActor, pParam->mHitPos, 600.0f, 10, false);

    if (*pHitFrame % 30 == 0) {
        if (*pIsHitLand) {
            al::tryStartSe(pActor, "LaserHitLand");
        } else {
            al::tryStartSe(pActor, "LaserHitWater");
        }
    }

    al::holdSe(pActor, "PgLaserBeamLv");
    al::updatePoseMtx(pParam->mBeamActor, &pParam->mMtx);
    al::holdSe(pParam->mBeamActor, "PgLaserBeamTipLv");

    sead::Vector3f playerPos = al::getTrans(rc::findNearestActivePlayerActor(pActor));
    sead::Vector3f nearPos;
    al::LinearCurve curve;
    curve.set(pParam->mOrigin, pParam->mHitPos);
    curve.calcNearestPos(&nearPos, playerPos);

    sead::Matrix34f nearMtx;
    al::makeMtxFrontUpPos(&nearMtx, pParam->mHitPos - pParam->mOrigin, sead::Vector3f::ey,
                          nearPos);
    al::updatePoseMtx(pParam->mNearActor, &nearMtx);
    al::holdSe(pParam->mNearActor, "PgLaserBeamNearLv");
    (*pHitFrame)++;
}

/**
 * @brief Scales the laser effects to the laser length.
 * @param pActor Actor emitting the laser.
 * @param rHitPos Laser hit position.
 * @param rOrigin Laser origin.
 * @param scale Laser width scale.
 */
void scaleLaserEffects(al::LiveActor* pActor, const sead::Vector3f& rHitPos,
                       const sead::Vector3f& rOrigin, f32 scale) {
    f32 width = scale * (600.0f / 47.0f);
    f32 length = (rHitPos - rOrigin).length();
    al::Effect* effect = pActor->getEffectKeeper()->tryFindEffect("Laser");

    if (effect != nullptr) {
        f32 lengthScale = length / 100.0f;
        const_cast<al::EffectInfo*>(effect->getEffectInfo())->mParam.mScale = lengthScale;
        f32 widthScale = width / lengthScale;
        sead::Vector3f laserScale = {1.0f, widthScale, widthScale};
        al::setEffectParticleScale(pActor, "Laser", laserScale);
        al::setEffectEmitterScale(pActor, "Laser", laserScale);
    }

    al::setEffectParticleScale(pActor, "LaserCap", {width, width, width});
}

/**
 * @brief Updates the laser direction, hit position, effects and sounds.
 * @param pActor Actor emitting the laser.
 * @param pParam Laser state.
 */
void updateLaserEffects(al::LiveActor* pActor, LaserParam* pParam) {
    pParam->mFront = pParam->mHitPos - pParam->mOrigin;
    pParam->mFront.normalize();
    pParam->mSide.setCross(sead::Vector3f::ey, pParam->mFront);
    pParam->mUp.setCross(pParam->mFront, pParam->mSide);

    calcLaserHitPos(pActor, &pParam->mSide, pParam->mOrigin, &pParam->mHitPos);

    pParam->mMtx.setBase(0, pParam->mFront);
    pParam->mMtx.setBase(1, pParam->mUp);
    pParam->mMtx.setBase(2, -pParam->mSide);
    pParam->mMtx.setTranslation(pParam->mHitPos);

    playLaserEffects(pActor, &pParam->mIsHitLand, &pParam->mHitFrame, pParam);
    scaleLaserEffects(pActor, pParam->mHitPos, pParam->mOrigin, 1.0f);
}

}  // namespace DarkBowserUtil
