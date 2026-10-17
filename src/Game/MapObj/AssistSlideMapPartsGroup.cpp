#include "MapObj/AssistSlideMapPartsGroup.hpp"
#include "MapObj/AssistSlideMapParts.hpp"
#include "Library/ActorUtil.hpp"
AssistSlideMapPartsGroup::AssistSlideMapPartsGroup(const char* pName) : al::LiveActor(pName) {}
AssistSlideMapPartsGroup::~AssistSlideMapPartsGroup() {}
void AssistSlideMapPartsGroup::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    int count = al::calcLinkChildNum(rInfo, "Parts");
    if (count != 0) {
        mParts = new al::DeriveActorGroup<AssistSlideMapParts>("パーツリスト", count);
        for (int i = 0; i < count; ++i) {
            const char* name = al::getLinksActorDisplayName(rInfo, "Parts", i);
            auto* part = new AssistSlideMapParts(name);
            part->setGroupHost(this);
            al::initLinksActor(part, rInfo, "Parts", i);
            mParts->registerActor(part);
        }
    }
    makeActorDead();
}
void AssistSlideMapPartsGroup::requestTouchAssist() {
    int count = mParts->mNumActors;
    for (int i = 0; i < count; ++i)
        mParts->getDeriveActor(i)->touchAssist();
}
