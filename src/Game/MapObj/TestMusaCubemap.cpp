#include "MapObj/TestMusaCubemap.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Obj/BreakModel.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "MapObj/DrcAssistDirectorUtil.hpp"
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
NERVE_DECL(TestMusaCubemap, Wait);
NERVE_DECL(TestMusaCubemap, Move);
NERVE_DECL(TestMusaCubemap, Stop);
NERVE_DECL(TestMusaCubemap, Break);
NERVES_MAKE_STRUCT(TestMusaCubemap, Move, Wait)
NERVES_MAKE_NOSTRUCT(TestMusaCubemap, Stop, Break)
const al::UniformBlockLayout sMusaCubemapLayout[] = {
    {0, agl::UniformBlock::cType_Float, 1},
    {1, agl::UniformBlock::cType_Float, 1},
    {2, agl::UniformBlock::cType_Float, 1},
    {3, agl::UniformBlock::cType_Vec3, 1},
};
}
TestMusaCubemap::TestMusaCubemap(const char* name) : al::LiveActor(name) {}
TestMusaCubemap::~TestMusaCubemap() {}
void TestMusaCubemap::init(const al::ActorInitInfo& info) {
    al::initActor(this, info);
    if (al::isExistRail(this)) {
        al::setRailPosToStart(this);
        al::initNerve(this, &NrvTestMusaCubemap.Move, 0);
    } else {
        al::initNerve(this, &NrvTestMusaCubemap.Wait, 0);
    }
    mBreakModel = new al::BreakModel(this, "壊れモデル", "BlockBrickBreak", nullptr, nullptr, "Break", true);
    al::initCreateActorNoPlacementInfo(mBreakModel, info);
    makeActorAppeared();
    mUniformBlock = al::createUniformBlock(sMusaCubemapLayout, 4, al::getCurrentHeap(), 2);
}
void TestMusaCubemap::control() {
    if (rc::isEnableTouchPointer(this)) {
        sead::Vector3f pointerPos(rc::getTouchPointerPosition(this));
        float distance = (pointerPos - al::getTrans(this)).length();
        float t = 1.0f - sead::Mathf::clamp((distance - 50.0f) / 350.0f, 0.0f, 1.0f);
        float scale = t * 0.9f + (1.0f - t) * 0.005f;
        float alpha = t * 0.9f + (1.0f - t) * 0.01f;
        mIndirectScale = al::lerpValue(0.2f, mIndirectScale, scale);
        mAlpha = al::lerpValue(0.2f, mAlpha, alpha);
    } else {
        mIndirectScale = al::lerpValue(0.05f, mIndirectScale, 0.005f);
        mAlpha = al::lerpValue(0.05f, mAlpha, 0.01f);
    }
}
void TestMusaCubemap::exeWait() {}
void TestMusaCubemap::exeBreak() {
    al::appearBreakModelRandomRotateY(mBreakModel);
    kill();
}
void TestMusaCubemap::exeMove() {
    al::moveSyncRail(this, 2.5f);
    if (al::isRailReachedEdge(this))
        al::setNerve(this, &NrvTestMusaCubemapStop);
}
void TestMusaCubemap::exeStop() {
    if (al::isGreaterEqualStep(this, 40)) {
        al::setNerve(this, &NrvTestMusaCubemap.Move);
        al::reverseRail(this);
    }
}
void TestMusaCubemap::draw() const {
    const agl::ShaderProgram* shader = al::ShaderHolder::instance()->getShaderProgram("RenderIndirect");
    mUniformBlock->setValue(0, mIndirectScale);
    mUniformBlock->setValue(1, mIndirectOffset);
    mUniformBlock->setValue(2, mAlpha);
    sead::Vector3f color(1.0f, 1.0f, 1.0f);
    mUniformBlock->setValueRef(3, color);
    agl::UniformBlockLocation location("RenderIndirect");
    location.search(*shader);
    mUniformBlock->activate(al::GameFrameworkNx::getAglDrawContext(), location);
    mUniformBlock->flushCurrentBuffer();
}
bool TestMusaCubemap::receiveMsg(const al::SensorMsg* msg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgPlayerUpperPunch(msg) || al::isMsgPlayerHipDropAll(msg) ||
        al::isMsgPlayerRollingAttack(msg)) {
        al::setNerve(this, &NrvTestMusaCubemapBreak);
        return true;
    }
    return false;
}
