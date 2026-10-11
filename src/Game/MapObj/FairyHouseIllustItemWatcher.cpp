#include "MapObj/FairyHouseIllustItemWatcher.hpp"
#include "MapObj/StampDirector.hpp"
#include "Layout/ListStampResult.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolder.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/Data/StageDataHolder.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(FairyHouseIllustItemWatcher, Wait);
    NERVE_DECL(FairyHouseIllustItemWatcher, ShowLayout);
    NERVES_MAKE_NOSTRUCT(FairyHouseIllustItemWatcher, Wait, ShowLayout)
}
FairyHouseIllustItemWatcher::FairyHouseIllustItemWatcher(const char* name) : al::LiveActor(name) {}
FairyHouseIllustItemWatcher::~FairyHouseIllustItemWatcher() {}
void FairyHouseIllustItemWatcher::init(const al::ActorInitInfo& info) {
    al::initActorSceneInfo(this, info);
    al::initExecutorWatchObj(this, info);
    al::initNerve(this, &NrvFairyHouseIllustItemWatcherWait, 0);
    if (GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor(this))) makeActorDead();
    else {
        mLayout = new ListStampResult(al::getLayoutInitInfo(info), GameDataFunction::getGameDataHolder(this));
        makeActorAppeared();
    }
}
void FairyHouseIllustItemWatcher::setStampDirector(rc::StampDirector* director) { mStampDirector = director; }
void FairyHouseIllustItemWatcher::exeWait() {
    if (GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor(this))) al::setNerve(this, &NrvFairyHouseIllustItemWatcherShowLayout);
}
void FairyHouseIllustItemWatcher::exeShowLayout() {
    if (al::isFirstStep(this)) {
        int course = GameDataFunction::getGameDataHolder(this)->getStageDataHolder()->getCourseId();
        mLayout->startAppear(course);
        mStampDirector->setCollectStamp(course);
    }
    if (mLayout->isEnd()) kill();
}
bool FairyHouseIllustItemWatcher::isShowLayout() const {
    return al::isAlive(this) && al::isNerve(this, &NrvFairyHouseIllustItemWatcherShowLayout);
}
