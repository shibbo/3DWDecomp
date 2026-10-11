#include "MapObj/TouchPointTransparent.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include <gfx/seadColor.h>

namespace {
    NERVE_DECL(TouchPointTransparent, Disappear);
    NERVE_DECL(TouchPointTransparent, Transparent);
    NERVES_MAKE_NOSTRUCT(TouchPointTransparent, Disappear, Transparent)
}

TouchPointTransparent::TouchPointTransparent(const char* pName) : al::LiveActor(pName) {
}

void TouchPointTransparent::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TouchPointTransparent", nullptr);
    al::setMaterialProgrammable(this);
    al::initNerve(this, &NrvTouchPointTransparentDisappear, 0);
    al::invalidateClipping(this);
    al::createRenderState(this);
    makeActorDead();
    al::setCustomRenderEnable(this, true);
}

void TouchPointTransparent::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTouchPointTransparentDisappear);
}

void TouchPointTransparent::startTransparentMode() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvTouchPointTransparentTransparent);
}

void TouchPointTransparent::setGyroDisappearAlpha(float rate) {
    mAlpha = rate * 0.5f + 0.25f;
}

void TouchPointTransparent::exeTransparent() {
    al::isFirstStep(this);
    al::setRenderStateBlendColor(this, sead::Color4f(0.0f, 0.0f, 0.0f, mAlpha));
}

void TouchPointTransparent::exeDisappear() {
    al::isFirstStep(this);
    int duration = mIsSlowDisappear ? 30 : 10;
    float alpha = al::lerpValue(al::calcNerveRate(this, duration), 1.0f, 0.0f);
    al::setRenderStateBlendColor(this, sead::Color4f(0.0f, 0.0f, 0.0f, alpha));
    if (al::isGreaterEqualStep(this, duration)) {
        kill();
    }
}

TouchPointTransparent::~TouchPointTransparent() {
}
