#include "MapObj/TestGhostPresentBox.hpp"
#include "NPC/GhostPresentBox.hpp"
#include "Library/ActorUtil.hpp"
TestGhostPresentBox::TestGhostPresentBox(const char* pName) : al::LiveActor(pName) {}
TestGhostPresentBox::~TestGhostPresentBox() {}
void TestGhostPresentBox::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::tryGetArg(&mItemPatternIndex, rInfo, "ItemPatternIndex");
    mBox = new GhostPresentBox("ゴーストボックステスト", mItemPatternIndex);
    al::initCreateActorWithPlacementInfo(mBox, rInfo);
    mBox->start();
    makeActorDead();
}
