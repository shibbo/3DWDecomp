#include "MapObj/TestRouteDokanRenderMaterialIndirect.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
    NERVE_DECL(TestRouteDokanRenderMaterialIndirect, Wait);
    NERVES_MAKE_NOSTRUCT(TestRouteDokanRenderMaterialIndirect, Wait)
}

TestRouteDokanRenderMaterialIndirect::TestRouteDokanRenderMaterialIndirect(const char* pName)
    : al::LiveActor(pName) {}

TestRouteDokanRenderMaterialIndirect::~TestRouteDokanRenderMaterialIndirect() {}

void TestRouteDokanRenderMaterialIndirect::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "RouteDokanTest", nullptr);
    al::initNerve(this, &NrvTestRouteDokanRenderMaterialIndirectWait, 1);
    makeActorAppeared();
}

void TestRouteDokanRenderMaterialIndirect::exeWait() {
    al::isFirstStep(this);
    if (al::isGreaterStep(this, 30))
        al::setNerve(this, &NrvTestRouteDokanRenderMaterialIndirectWait);
}
