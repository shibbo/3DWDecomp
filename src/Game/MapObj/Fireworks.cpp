#include "MapObj/Fireworks.hpp"
#include "Scene/ProjectActorFactoryLightActors.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"

namespace {
    NERVE_DECL(Fireworks, Wait);
    NERVE_DECL(Fireworks, Fire);
    NERVES_MAKE_NOSTRUCT(Fireworks, Wait, Fire)
}

Fireworks::Fireworks(const char* pName) : al::LiveActor(pName) {}
Fireworks::~Fireworks() {}

void Fireworks::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initActorAudioKeeperWithout3D(this, rInfo, "FireWorks", nullptr);
    mLightCount = al::calcLinkChildNum(rInfo, "FireworksLight");
    mLights = new al::PrePassProjLight*[mLightCount];
    for (int i = 0; i < mLightCount; i++) {
        mLights[i] = new al::PrePassProjLight("投影ライト");
        al::initLinksActor(mLights[i], rInfo, "FireworksLight", i);
    }
    al::tryGetArg(&mWaitTime, rInfo, "WaitTime");
    al::initNerve(this, &NrvFireworksWait, 0);
    makeActorAppeared();
}

void Fireworks::exeWait() {
    setLightPower(0.0f);
    if (al::isGreaterEqualStep(this, mWaitTime))
        al::setNerve(this, &NrvFireworksFire);
    else if (al::isStep(this, mWaitTime - 60))
        al::startSe(this, "Hyu", nullptr);
}

void Fireworks::setLightPower(float power) {
    for (int i = 0; i < mLightCount; i++) {
        sead::Color4f color;
        mLights[i]->getLight()->calcColor(&color, 1.0f);
        color *= power;
        mLights[i]->getLight()->requestUserColor(color);
    }
}

void Fireworks::exeFire() {
    setLightPower(al::easeIn(al::getNerveStep(this) / -240.0f + 1.0f) * 8.0f);
    if (al::isGreaterEqualStep(this, 240))
        al::setNerve(this, &NrvFireworksWait);
    else if (al::isStep(this, 10))
        al::startSe(this, "Bang", nullptr);
}
