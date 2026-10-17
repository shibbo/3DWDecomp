#include "MapObj/TestAlphaMaskMapObj.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(TestAlphaMaskMapObj, Wait);
    NERVE_DECL(TestAlphaMaskMapObj, AlphaMaskOut);
    NERVE_DECL(TestAlphaMaskMapObj, AlphaMaskIn);
    NERVES_MAKE_NOSTRUCT(TestAlphaMaskMapObj, Wait, AlphaMaskOut, AlphaMaskIn)
}

TestAlphaMaskMapObj::TestAlphaMaskMapObj(const char* pName) : al::LiveActor(pName) {}
TestAlphaMaskMapObj::~TestAlphaMaskMapObj() {}

void TestAlphaMaskMapObj::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::initNerve(this, &NrvTestAlphaMaskMapObjWait, 0);
    makeActorAppeared();
}

void TestAlphaMaskMapObj::exeWait() {
    if (al::isFirstStep(this))
        al::setNerve(this, &NrvTestAlphaMaskMapObjAlphaMaskOut);
}

void TestAlphaMaskMapObj::exeAlphaMaskIn() {
    if (al::isFirstStep(this))
        al::startAction(this, "AlphaMaskIn");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvTestAlphaMaskMapObjWait);
}

void TestAlphaMaskMapObj::exeAlphaMaskOut() {
    if (al::isFirstStep(this))
        al::startAction(this, "AlphaMaskOut");
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvTestAlphaMaskMapObjAlphaMaskIn);
}

void TestAlphaMaskMapObj::exeAlphaMaskWait() {}
