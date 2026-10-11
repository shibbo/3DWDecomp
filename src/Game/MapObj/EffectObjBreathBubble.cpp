#include "MapObj/EffectObjBreathBubble.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/AreaObjUtil.hpp"

namespace {
    NERVE_DECL(EffectObjBreathBubble, Rise);
    NERVE_DECL(EffectObjBreathBubble, Burst);
    NERVES_MAKE_NOSTRUCT(EffectObjBreathBubble, Rise, Burst)
}

EffectObjBreathBubble::EffectObjBreathBubble(const char* pName) : al::LiveActor(pName) {}
EffectObjBreathBubble::~EffectObjBreathBubble() {}

void EffectObjBreathBubble::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorEffectKeeper(this, rInfo, "EffectObjBreathBubble", true);
    al::setEffectFollowPosPtr(this, "Bubble", &mEffectPosition);
    al::initExecutorUpdate(this, rInfo, "コリジョン地形[Movement]");
    al::initNerve(this, &NrvEffectObjBreathBubbleRise, 0);
    makeActorDead();
}

void EffectObjBreathBubble::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    al::setNerve(this, &NrvEffectObjBreathBubbleRise);
}

void EffectObjBreathBubble::exeRise() {
    al::AreaObj* area = rc::tryFindAreaObj(this, rc::AreaObjType::WaterArea,
        al::getTrans(this) + sead::Vector3f(0.0f, 110.0f, 0.0f));
    if (al::isFirstStep(this)) {
        if (!area) {
            makeActorDead();
            return;
        }
        al::tryEmitEffect(this, "Bubble", nullptr);
        mEffectPosition.set(al::getTrans(this));
        sead::Vector3f* velocity = al::getVelocityPtr(this);
        al::verticalizeVec(velocity, sead::Vector3f(0.0f, 1.0f, 0.0f), *velocity);
        mWaterArea = area;
    }
    if (!area) {
        al::tryDeleteEffect(this, "Bubble");
        al::getVelocityPtr(this)->set(0.0f, 0.0f, 0.0f);
        al::setNerve(this, &NrvEffectObjBreathBubbleBurst);
        return;
    }
    if (al::isGreaterEqualStep(this, 120)) {
        al::tryDeleteEffect(this, "Bubble");
        al::getVelocityPtr(this)->set(0.0f, 0.0f, 0.0f);
        al::setTrans(this, al::getTrans(this) + sead::Vector3f(0.0f, 110.0f, 0.0f));
        al::tryEmitEffect(this, "Break", nullptr);
        kill();
        return;
    }
    float acceleration = (4.0f - al::getVelocity(this).y) * 0.5f;
    al::getVelocityPtr(this)->y += acceleration;
    al::getVelocityPtr(this)->x *= 0.95f;
    al::getVelocityPtr(this)->z *= 0.95f;
    mWaterArea = area;
    mEffectPosition = al::getTrans(this) + sead::Vector3f(0.0f, 110.0f, 0.0f);
}

void EffectObjBreathBubble::exeBurst() {
    if (al::isFirstStep(this) && mWaterArea) {
        sead::Vector3f hitPosition;
        sead::Vector3f normal;
        sead::Vector3f position = al::getTrans(this) + sead::Vector3f(0.0f, 110.0f, 0.0f);
        if (al::checkAreaObjCollisionByArrow(&hitPosition, &normal, mWaterArea,
            sead::Vector3f(position.x, position.y - 100.0f, position.z), position)) {
            al::setTrans(this, hitPosition);
            al::tryEmitEffect(this, "Ripple", nullptr);
        }
    }
    if (al::isStep(this, 5))
        al::tryDeleteEffect(this, "Ripple");
    if (al::isGreaterStep(this, 60)) {
        al::tryDeleteEffect(this, "Ripple");
        kill();
    }
}
