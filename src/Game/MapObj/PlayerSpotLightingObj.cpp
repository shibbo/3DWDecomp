#include "MapObj/PlayerSpotLightingObj.hpp"
#include "Library/Math/MathUtil.hpp"
#include <math/seadMathCalcCommon.h>
#include "Library/Light/PrePassLightKeeper.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"

PlayerSpotLightingObj::PlayerSpotLightingObj(const char* pName)
    : al::LiveActor(pName) {}
PlayerSpotLightingObj::~PlayerSpotLightingObj() {}

void PlayerSpotLightingObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initExecutorUpdate(this, rInfo, "グラフィックス要求者");
    al::initStageSwitch(this, rInfo);
    al::trySyncStageSwitchAppear(this);
    LightPrePassFunction::declareUsingSpotLight(this, al::getPlayerNumMax(this));
    al::tryGetArg(&mOffset.x, rInfo, "OffsetX");
    al::tryGetArg(&mOffset.y, rInfo, "OffsetY");
    al::tryGetArg(&mOffset.z, rInfo, "OffsetZ");
    al::tryGetArg(&mRotateOffset.x, rInfo, "RotateOffsetX");
    al::tryGetArg(&mRotateOffset.y, rInfo, "RotateOffsetY");
    al::tryGetArg(&mRotateOffset.z, rInfo, "RotateOffsetZ");
    al::tryGetArg(&mColor.r, rInfo, "ColorRed");
    al::tryGetArg(&mColor.g, rInfo, "ColorGreen");
    al::tryGetArg(&mColor.b, rInfo, "ColorBlue");
    al::tryGetArg(&mSpotLightDegree, rInfo, "SpotLightDegree");
    al::tryGetArg(&mSpotLightLength, rInfo, "SpotLightLength");
    al::tryGetArg(&mLightDistDamp, rInfo, "LightDistDamp");
    al::tryGetArg(&mSpotLightAngleDamp, rInfo, "SpotLightAngleDamp");
}


void PlayerSpotLightingObj::control() {
    int count = al::getPlayerNumMax(this);
    for (int i = 0; i < count; ++i) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        if (al::isDead(player))
            continue;
        const sead::Vector3f& playerPosition = al::getTrans(player);
        sead::Vector3f direction = -sead::Vector3f::ey;
        al::rotateVectorDegreeZ(&direction, mRotateOffset.z);
        al::rotateVectorDegreeX(&direction, mRotateOffset.x);
        al::rotateVectorDegreeY(&direction, mRotateOffset.y);
        sead::Vector3f offset = mOffset;
        al::rotateVectorDegreeZ(&offset, mRotateOffset.z);
        al::rotateVectorDegreeX(&offset, mRotateOffset.x);
        al::rotateVectorDegreeY(&offset, mRotateOffset.y);
        sead::Vector3f position = playerPosition + offset;
        LightPrePassFunction::requestSpotLight(this, position, direction,
            sead::Mathf::deg2rad(mSpotLightDegree), mSpotLightLength, mColor,
            mSpotLightAngleDamp, mLightDistDamp, mLightDistDamp, true, false, mColor);
    }
}

void PlayerSpotLightingObj::movementPaused(bool isPaused) { control(); }
