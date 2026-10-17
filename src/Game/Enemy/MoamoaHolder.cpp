#include "Enemy/MoamoaHolder.hpp"
#include "Enemy/Moamoa.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Thread/Functor.hpp"

/** @brief Creates the Moamoa group. @param pName Placement name, replaced by the group name. */
MoamoaHolder::MoamoaHolder(const char* pName)
    : al::LiveActor("\u30e2\u30a2\u30e2\u30a2\u4fdd\u6301") {}

/** @brief Creates the enemy grid and connects the player-tracking switch. @param rInfo Placement information. */
void MoamoaHolder::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    if (al::listenStageSwitchOnOff(this, "SwitchPlayerLookAtCtrl",
        al::Functor(this, &MoamoaHolder::validateLookCtrl),
        al::Functor(this, &MoamoaHolder::invalidateLookCtrl)))
        mIsLookCtrl = false;
    sead::Quatf quat = sead::Quatf::unit;
    al::getQuat(&quat, al::getPlacementInfo(rInfo));
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    sead::Vector3f side(0.0f, 0.0f, 0.0f);
    sead::Vector3f front(0.0f, 0.0f, 0.0f);
    sead::Vector3f offset(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, quat);
    al::calcQuatSide(&side, quat);
    al::calcQuatFront(&front, quat);
    al::tryGetDisplayOffset(&offset, rInfo);
    s32 width = 1;
    s32 height = 1;
    s32 depth = 1;
    al::tryGetArg(&width, rInfo, "WidthNum");
    al::tryGetArg(&height, rInfo, "HeightNum");
    al::tryGetArg(&depth, rInfo, "DepthNum");
    if (width >= 1 && height >= 1 && depth > 0) {
        mCount = width * height * depth;
        mActors = new Moamoa*[mCount];
        for (s32 i = 0; i < mCount; ++i) {
            mActors[i] = new Moamoa("\u30e2\u30a2\u30e2\u30a2", this);
            al::initCreateActorWithPlacementInfo(mActors[i], rInfo);
        }
        for (s32 x = 0; x < width; ++x) {
            for (s32 y = 0; y < height; ++y) {
                for (s32 z = 0; z < depth; ++z) {
                    sead::Vector3f position = (x * 100.0f) * side + (y * 100.0f) * up -
                                             (z * 100.0f) * front;
                    position += offset;
                    s32 index = (z * height + y) * width + x;
                    mActors[index]->setOffset(position);
                    al::calcTransLocalOffset(al::getTransPtr(mActors[index]), mActors[index], position);
                }
            }
        }
    }
    makeActorDead();
}
