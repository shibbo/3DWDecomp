#include "MapObj/Mirror.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Draw/GraphicsSystemInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Shader/ForwardRendering/ShaderMirrorDirector.hpp"
#include "Library/Thread/Functor.hpp"

namespace {
    NERVE_DECL(Mirror, Wait);
    NERVE_DECL(Mirror, Disappear);
    NERVES_MAKE_NOSTRUCT(Mirror, Wait, Disappear)
}

Mirror::Mirror(const char* pName) : al::MirrorActorBase(pName) {}
Mirror::~Mirror() {}

void Mirror::init(const al::ActorInitInfo& rInfo) {
    initByArg(rInfo);
    al::initMapPartsActor(this, rInfo, nullptr, 0);
    al::initNerve(this, &NrvMirrorWait, 0);
    if (al::isExistModelResourceYaml(this, "InitMirror", nullptr))
        al::getActorRecourseDataString(&mDisappearAnimName, this, "InitMirror", "DisappearAnimName");
    getSceneInfo()->graphicsSystemInfo->getShaderMirrorDirector()->pushBackMirrorActor(this, rInfo);
    makeActorAppeared();
    al::listenStageSwitchOnKill(this, al::Functor(this, &Mirror::startDisappear));
}

void Mirror::startDisappear() {
    if (mDisappearAnimName)
        al::setNerve(this, &NrvMirrorDisappear);
    else
        kill();
}

void Mirror::kill() {
    al::startHitReactionDisappear(this);
    al::LiveActor::kill();
}

void Mirror::exeWait() {}

void Mirror::exeDisappear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mDisappearAnimName);
        al::tryOnStageSwitch(this, "ObjSyncSwitchKeepOn");
    }
    if (al::isActionEnd(this)) {
        al::tryOffStageSwitch(this, "ObjSyncSwitchKeepOn");
        kill();
    }
}
