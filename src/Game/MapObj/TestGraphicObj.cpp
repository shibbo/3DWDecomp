#include "MapObj/TestGraphicObj.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(TestGraphicObj, Wait);
    NERVES_MAKE_NOSTRUCT(TestGraphicObj, Wait)
}

TestGraphicObj::TestGraphicObj(const char* pName) : al::LiveActor(pName) {}

TestGraphicObj::~TestGraphicObj() {}

void TestGraphicObj::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestGraphicObjWait, 0);
    makeActorAppeared();
}

void TestGraphicObj::exeWait() {
    al::isFirstStep(this);
}
