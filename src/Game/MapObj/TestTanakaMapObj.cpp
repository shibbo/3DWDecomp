#include "MapObj/TestTanakaMapObj.hpp"
#include "MapObj/TouchTrackDrawer.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(TestTanakaMapObj, Wait);
    NERVES_MAKE_NOSTRUCT(TestTanakaMapObj, Wait)
}
TestTanakaMapObj::TestTanakaMapObj(const char* pName) : al::LiveActor(pName) {}
TestTanakaMapObj::~TestTanakaMapObj() {}
void TestTanakaMapObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TestSnowField", nullptr);
    al::initNerve(this, &NrvTestTanakaMapObjWait, 0);
    mTouchTrackDrawer = new TouchTrackDrawer(this);
    mTouchTrackDrawer->init(rInfo, "FootPrint", 100);
    makeActorAppeared();
}
void TestTanakaMapObj::exeWait() { mTouchTrackDrawer->update(); }
