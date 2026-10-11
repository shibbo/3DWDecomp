#include "MapObj/Fury/DisasterLightning.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
    NERVE_DECL(DisasterLightning, Strike);
    NERVES_MAKE_NOSTRUCT(DisasterLightning, Strike)
}

DisasterLightning::DisasterLightning(const char* pName) : al::LiveActor(pName) {}
DisasterLightning::~DisasterLightning() {}

void DisasterLightning::init(const al::ActorInitInfo& rInfo) {
    al::initNerve(this, &NrvDisasterLightningStrike, 0);
    al::initActorWithArchiveName(this, rInfo, "DisasterLightning", nullptr);
    makeActorDead();
}

void DisasterLightning::appear(int level) {
    al::LiveActor::appear();
    al::setNerve(this, &NrvDisasterLightningStrike);
    al::invalidateClipping(this);
    mEffectName = level == 1 ? "DisasterModeLightningHard" :
                  level == 2 ? "DisasterModeLightningSuperHard" : "DisasterModeLightning";
}

void DisasterLightning::exeStrike() {
    if (al::isFirstStep(this)) {
        al::emitEffect(this, mEffectName, nullptr);
        al::startSe(this, "Lightning", nullptr);
    } else if (al::isGreaterEqualStep(this, 300)) {
        kill();
    }
}
