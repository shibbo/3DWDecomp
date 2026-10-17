#include "MapObj/WaterAreaMoveModel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Util/ActorModelUniformBlockUtil.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
NERVE_DECL(WaterAreaMoveModel, Move);
NERVE_DECL(WaterAreaMoveModel, Stop);
NERVE_DECL(WaterAreaMoveModel, Delay);
NERVE_DECL(WaterAreaMoveModel, Wait);
NERVES_MAKE_NOSTRUCT(WaterAreaMoveModel, Move)
NERVES_MAKE_STRUCT(WaterAreaMoveModel, Stop, Delay, Wait)
const sead::Vector3f waterFront(1.0f, -1.1f, 1.0f);
const al::UniformBlockLayout waterLayout[] = {
    {0, agl::UniformBlock::cType_Vec2, 2},
    {1, agl::UniformBlock::cType_Vec4, 4},
};
}
WaterAreaMoveModel::WaterAreaMoveModel(const char* name) : al::LiveActor(name) {}
WaterAreaMoveModel::~WaterAreaMoveModel() {}
void WaterAreaMoveModel::init(const al::ActorInitInfo& info) {
    const char* type = "Default";
    al::tryGetStringArg(&type, info, "ModelType");
    const char* archive;
    if (al::isEqualString(type, "OnlyTop")) archive = "WaterAreaMoveModelOnlyTop";
    else if (al::isEqualString(type, "WaterTop")) archive = "WaterTop";
    else archive = "WaterAreaMoveModel";
    al::initActorWithArchiveName(this, info, archive, nullptr);
    mKeyPoses = al::createKeyPoseKeeper(info);
    al::registerAreaSyncHostMtx(this, info);
    float radius = 0.0f;
    al::calcKeyMoveClippingInfo(&mClippingCenter, &radius, mKeyPoses, 0.0f);
    sead::Vector3f scale = sead::Vector3f::ones;
    al::tryGetScale(&scale, info);
    mBackModel = al::tryGetSubActor(this, "移動水エリア裏面");
    if (mBackModel) {
        sead::Vector3f backScale(scale.x, scale.y, scale.z);
        al::setScale(mBackModel, backScale);
    }
    radius += al::getClippingRadius(this) * (scale.x > scale.y ? (scale.x > scale.z ? scale.x : scale.z) : (scale.y > scale.z ? scale.y : scale.z));
    al::setClippingInfo(this, radius, &mClippingCenter);
    al::tryGetArg(&mDelayTime, info, "DelayTime");
    al::tryListenStageSwitchKill(this);
    if (al::listenStageSwitchOnStart(this, al::FunctorV0M(this, &WaterAreaMoveModel::start)))
        al::initNerve(this, &NrvWaterAreaMoveModel.Stop, 0);
    else if (mDelayTime > 0) al::initNerve(this, &NrvWaterAreaMoveModel.Delay, 0);
    else al::initNerve(this, &NrvWaterAreaMoveModel.Wait, 0);
    makeActorAppeared();
    if (al::calcDistanceNextKeyTrans(mKeyPoses) > 10.0f) mPlayMoveSound = true;
    agl::UniformBlock* block = al::tryCreateModelUniformBlock(this, "cWater", waterLayout, 2);
    if (mBackModel) al::tryCreateModelUniformBlock(mBackModel, "cWater", waterLayout, 2);
    if (block) {
        al::setMaterialProgrammable(this);
        updateUbo(this, false);
        if (mBackModel) updateUbo(mBackModel, false);
        al::setMtsAnimFrameRate(this, 1.0f / scale.x);
    }
}
void WaterAreaMoveModel::start() { al::setNerve(this, &NrvWaterAreaMoveModelMove); }
void WaterAreaMoveModel::updateUbo(al::LiveActor* actor, bool flush) {
    agl::UniformBlock* block = al::findModelUniformBlock(actor, "cWater");
    if (!block) return;
    al::swapModelUniformBlock(block);
    block->setData(0, &mTexOffsetA, 0, 1);
    block->setData(0, &mTexOffsetB, 1, 1);
    sead::Matrix44f matrix;
    sead::Matrix44f projection = sead::Matrix44f::ident;
    projection.m[0][0] /= 768.0f;
    projection.m[1][1] /= 768.0f;
    projection.m[2][2] = 0.0f;
    sead::Matrix34f rotation = sead::Matrix34f::ident;
    sead::Vector3f front = waterFront;
    al::normalizeOrZero(&front);
    al::makeMtxFrontNoSupport(&rotation, front);
    matrix.setMul(projection, rotation);
    block->setData(1, &matrix, 0, 4);
    if (flush) al::flushModelUniformBlock(block);
}
void WaterAreaMoveModel::setMoveTime() {
    int time = al::calcKeyMoveSpeedByTime(mKeyPoses);
    if (time >= 1) {
        mMoveTime = time;
        return;
    }
    float speed = al::calcKeyMoveSpeed(mKeyPoses);
    if (!(speed > 0.0f))
        return;
    mMoveTime = al::calcDistanceNextKeyTrans(mKeyPoses) / speed;
    if (mMoveTime < 0)
        mMoveTime = 1;
}
void WaterAreaMoveModel::exeMove() {
    if (al::isFirstStep(this)) {
        setMoveTime();
        if (mPlayMoveSound) {
            al::tryStartSe(this, "PgMove");
            al::tryStartSe(this, "PgMoveLv");
        }
    }
    float rate = al::calcNerveRate(this, mMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoses, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoses, rate);
    if (al::isGreaterEqualStep(this, mMoveTime)) {
        al::nextKeyPose(mKeyPoses);
        if (al::isStop(mKeyPoses)) al::setNerve(this, &NrvWaterAreaMoveModel.Stop);
        else al::setNerve(this, &NrvWaterAreaMoveModel.Wait);
        if (mPlayMoveSound) {
            al::tryStopSe(this, "PgMove");
            al::tryStopSe(this, "PgMoveLv");
        }
    }
}
void WaterAreaMoveModel::exeDelay() {
    if (al::isGreaterEqualStep(this, mDelayTime)) al::setNerve(this, &NrvWaterAreaMoveModelMove);
}
void WaterAreaMoveModel::exeWait() {
    if (al::isFirstStep(this)) {
        int time = al::calcKeyMoveWaitTime(mKeyPoses);
        if (time >= 0) mWaitTime = time;
    }
    if (al::isGreaterEqualStep(this, mWaitTime)) al::setNerve(this, &NrvWaterAreaMoveModelMove);
}
void WaterAreaMoveModel::exeStop() {}
void WaterAreaMoveModel::control() {
    mTexOffsetA.x += 0.0007f;
    mTexOffsetA.y += 0.0011f;
    mTexOffsetB.x += 0.0005f;
    mTexOffsetB.y -= 0.001f;
    updateUbo(this, true);
    if (mBackModel) updateUbo(mBackModel, true);
}
