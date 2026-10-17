#include "MapObj/TestMusaCoinChameleon.hpp"
#include "MapObj/CoinRotater.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Framework/GameFrameworkNx.hpp"
#include "Library/Shader/Block/UniformBlock.hpp"
#include "Library/Shader/Block/UniformBlockUtil.hpp"
#include "Library/Shader/ForwardRendering/ShaderHolder.hpp"
#include "Util/ItemUtil.hpp"
#include "Util/ProjectMsgUtil.hpp"
#include "Library/Memory/Util.hpp"

namespace {
NERVE_DECL(TestMusaCoinChameleon, Wait);
NERVES_MAKE_NOSTRUCT(TestMusaCoinChameleon, Wait)
const al::UniformBlockLayout sCoinChameleonLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1},
    {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1},
    {3, agl::UniformBlock::cType_Vec3, 1},
};
}
TestMusaCoinChameleon::TestMusaCoinChameleon(const char* name) : al::LiveActor(name) {}
TestMusaCoinChameleon::~TestMusaCoinChameleon() {}
void TestMusaCoinChameleon::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "TestMusaCoinChameleon", nullptr);
    al::initNerve(this, &NrvTestMusaCoinChameleonWait, 0);
    al::addTransOffsetLocalDir(this, 70.0f, 1);
    mConnector = al::tryCreateMtxConnector(this, info);
    mBaseQuat.set(al::getQuat(this));
    sead::Vector3f up(0.0f, 0.0f, 0.0f);
    al::calcQuatUp(&up, mBaseQuat);
    if (!al::isNearZero(up.cross(sead::Vector3f(0.0f, 0.0f, 1.0f)), 0.001f))
        al::makeQuatUpFront(&mBaseQuat, up, sead::Vector3f(0.0f, 0.0f, 1.0f));
    makeActorAppeared();
    mUniformBlock = al::createUniformBlock(sCoinChameleonLayout, 4, al::getCurrentHeap(), 2);
}
void TestMusaCoinChameleon::initAfterPlacement() {
    if (mConnector)
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 300.0f);
}
void TestMusaCoinChameleon::control() {
    if (mConnector) {
        sead::Quatf quat;
        al::rotateQuatYDirDegree(&quat, mBaseQuat, rc::getCoinRotateY(this));
        al::connectPoseQT(this, mConnector, quat, al::getConnectBaseTrans(mConnector));
    } else {
        al::rotateQuatYDirDegree(this, mBaseQuat, rc::getCoinRotateY(this));
    }
    sead::Vector3f playerPos(al::findNearestPlayerPos(this));
    float distance = (playerPos - al::getTrans(this)).length();
    float target = 0.001f;
    if (al::isNearPlayer(this, 600.0f)) {
        float t = 1.0f - sead::Mathf::clamp((distance - 800.0f) / 200.0f, 0.0f, 1.0f);
        target = t * 0.9f + (1.0f - t) * 0.001f;
    }
    mIndirectScale = al::lerpValue(0.05f, mIndirectScale, target);
    mAlpha = al::lerpValue(0.05f, mAlpha, target);
}
void TestMusaCoinChameleon::draw() const {
    const agl::ShaderProgram* shader = al::ShaderHolder::instance()->getShaderProgram("RenderIndirect");
    mUniformBlock->setValue(0, mIndirectScale);
    mUniformBlock->setValue(1, mIndirectOffset);
    mUniformBlock->setValue(2, mAlpha);
    sead::Vector3f color(1.0f, 1.0f, 0.0f);
    mUniformBlock->setValueRef(3, color);
    agl::UniformBlockLocation location("RenderIndirect");
    location.search(*shader);
    mUniformBlock->activate(al::GameFrameworkNx::getAglDrawContext(), location);
    mUniformBlock->flushCurrentBuffer();
}
bool TestMusaCoinChameleon::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                      al::HitSensor*) {
    if (rc::isMsgCoinGet(msg)) {
        rc::acquirerItemCoin(this, sender);
        al::startHitReactionGet(this);
        kill();
        return true;
    }
    return false;
}
void TestMusaCoinChameleon::exeWait() {}
