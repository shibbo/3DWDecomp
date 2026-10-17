#include "MapObj/FireworksEffectObj.hpp"
#include "Scene/ProjectActorFactoryLightActors.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Obj/EffectObjFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
    NERVE_DECL(FireworksEffectObj, Wait);
    NERVE_DECL(FireworksEffectObj, Fire);
    NERVES_MAKE_NOSTRUCT(FireworksEffectObj, Wait, Fire)
}

FireworksEffectObj::FireworksEffectObj(const char* pName) : al::LiveActor(pName) {}
FireworksEffectObj::~FireworksEffectObj() {}

void FireworksEffectObj::init(const al::ActorInitInfo& rInfo) {
    al::EffectObjFunction::initActorEffectObj(this, rInfo);
    al::makeMtxSRT(&mBaseMtx, this);
    al::initActorAudioKeeper(this, rInfo, "FireworksEffectObj", "FireworksEffectObj", al::getTransPtr(this), &mBaseMtx);
    mLightCount = al::calcLinkChildNum(rInfo, "ProjectionLight");
    if (mLightCount != 0) {
        mLights = new al::PrePassProjLight*[mLightCount];
        for (int i = 0; i < mLightCount; i++) {
            mLights[i] = new al::PrePassProjLight("花火ライト");
            al::initLinksActor(mLights[i], rInfo, "ProjectionLight", i);
        }
    }
    al::tryGetArg(&mIsOneShot, rInfo, "IsFireAtOnce");
    al::initNerve(this, &NrvFireworksEffectObjWait, 0);
    al::listenStageSwitchOnAppear(this, al::Functor(this, &FireworksEffectObj::makeActorAppeared));
    makeActorDead();
}

void FireworksEffectObj::makeActorAppeared() {
    if (mHasFired)
        return;
    if (mIsOneShot)
        mHasFired = true;
    al::LiveActor::makeActorAppeared();
    al::makeMtxSRT(&mBaseMtx, this);
    al::emitEffect(this, "Wait", nullptr);
    al::tryOnStageSwitch(this, "ObjSyncSwitchKeepOn");
    al::setNerve(this, &NrvFireworksEffectObjFire);
}

void FireworksEffectObj::exeWait() {
    if (mLights && al::isFirstStep(this))
        setLightPower(0.0f);
}

void FireworksEffectObj::setLightPower(float power) {
    for (int i = 0; i < mLightCount; i++) {
        sead::Color4f color;
        mLights[0]->getLight()->calcColor(&color, 1.0f);
        color *= power;
        mLights[0]->getLight()->requestUserColor(color);
    }
}

void FireworksEffectObj::exeFire() {
    if (al::isGreaterEqualStep(this, 240)) {
        al::tryOffStageSwitch(this, "ObjSyncSwitchKeepOn");
        al::setNerve(this, &NrvFireworksEffectObjWait);
        return;
    }
    if (al::isStep(this, al::getRandom(0, 10)))
        al::startSe(this, "PgBang", nullptr);
    if (mLights)
        setLightPower(al::easeIn(al::getNerveStep(this) / -240.0f + 1.0f) * 8.0f);
}
