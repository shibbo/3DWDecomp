#include "MapObj/PlayerPointLightingObj.hpp"
#include "Library/Light/PrePassLightBase.hpp"
#include "Library/Light/PrePassLightKeeper.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"

PlayerPointLightingObj::PlayerPointLightingObj(const char* pName)
    : al::LiveActor(pName), mParam(new al::LppPointParam) {}
PlayerPointLightingObj::~PlayerPointLightingObj() {}

void PlayerPointLightingObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorUpdate(this, rInfo, "グラフィックス要求者");
    al::initStageSwitch(this, rInfo);
    al::trySyncStageSwitchAppear(this);
    LightPrePassFunction::declareUsingPointLight(this, al::getPlayerNumMax(this));
    al::tryGetArg(&mOffset.x, rInfo, "OffsetX");
    al::tryGetArg(&mOffset.y, rInfo, "OffsetY");
    al::tryGetArg(&mOffset.z, rInfo, "OffsetZ");
    al::tryGetArg(&mColor.r, rInfo, "ColorRed");
    al::tryGetArg(&mColor.g, rInfo, "ColorGreen");
    al::tryGetArg(&mColor.b, rInfo, "ColorBlue");
    al::tryGetArg(&mIsEnableSpecular, rInfo, "IsEnableSpecular");
    mParam->initByInfo(rInfo);
}

void PlayerPointLightingObj::makeActorAppeared() { al::LiveActor::makeActorAppeared(); }
void PlayerPointLightingObj::makeActorDead() { al::LiveActor::makeActorDead(); }

void PlayerPointLightingObj::control() {
    int count = al::getPlayerNumMax(this);
    for (int i = 0; i < count; ++i) {
        al::LiveActor* player = al::getPlayerActor(this, i);
        if (al::isDead(player))
            continue;
        sead::Vector3f position = al::getTrans(player) + mOffset;
        LightPrePassFunction::requestPointLight(this, position, mParam->mRadius, mColor,
            mParam->mDampPower, mParam->mSpecExpansion, mIsEnableSpecular, false,
            sead::Color4f::cWhite, 0);
    }
}

void PlayerPointLightingObj::movementPaused(bool isPaused) { control(); }
