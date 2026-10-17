#include "MapObj/SuperbViewArea.hpp"
#include "MapObj/GuideObj.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Camera/CameraUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Bgm/BgmLineFunction.hpp"
#include "Project/AreaObj/AreaObj.hpp"
#include "System/GameDataFunction.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
NERVE_DECL(SuperbViewArea, Wait);
NERVE_DECL(SuperbViewArea, Look);
NERVES_MAKE_NOSTRUCT(SuperbViewArea, Wait, Look)
}
SuperbViewArea::SuperbViewArea(const char* name) : al::LiveActor(name) {}
SuperbViewArea::~SuperbViewArea() {}
void SuperbViewArea::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "SuperbViewArea", nullptr);
    al::initNerve(this, &NrvSuperbViewAreaWait, 0);
    mCamera = al::initObjectMapCamera(this, info, nullptr);
    mArea = al::createAreaObj(info, "チェックエリア");
    mArea->getName();
    mAudioDirector = al::getAudioDirector(info);
    bool enableFilter = true;
    al::tryGetArg(&enableFilter, info, "IsEnableFilter");
    if (enableFilter)
        mFilter = new al::SimpleLayoutAppearWaitEnd("望遠鏡フィルターレイアウト", "FilterTelescope", al::getLayoutInitInfo(info), nullptr, false);
    al::tryGetArg(&mCheckOnGround, info, "IsCheckOnGround");
    mGuideCount = al::calcLinkChildNum(info, "GuideObj");
    if (mGuideCount > 0) {
        mGuides = new GuideObj*[mGuideCount];
        for (int i = 0; i < mGuideCount; ++i) {
            mGuides[i] = new GuideObj("ガイドオブジェ");
            al::initLinksActor(mGuides[i], info, "GuideObj", i);
            mGuides[i]->makeActorDead();
            mGuides[i]->getName();
        }
    }
    al::listenStageSwitchOnOff(this, "SwitchFilterTelescopeOffSync", al::FunctorV0M(this, &SuperbViewArea::offFilterLayout), al::FunctorV0M(this, &SuperbViewArea::onFilterLayout));
    al::trySyncStageSwitchAppear(this);
}
void SuperbViewArea::offFilterLayout() {
    if (al::isNerve(this, &NrvSuperbViewAreaLook) && mFilter) mFilter->end();
}
void SuperbViewArea::onFilterLayout() {
    if (al::isNerve(this, &NrvSuperbViewAreaLook) && mFilter) mFilter->appear();
}
void SuperbViewArea::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    auto* holder = static_cast<SuperbViewAreaHolder*>(mActorSceneInfo->sceneObjHolder->tryGetObj(54));
    if (holder) holder->registerArea(this);
}
void SuperbViewAreaHolder::registerArea(SuperbViewArea* area) { mAreas.pushBack(area); }
void SuperbViewArea::exeWait() {
    if (al::isFirstStep(this) && mFilter) mFilter->end();
    if (al::isInAreaObjPlayerAll(this, mArea) && (!mCheckOnGround || rc::isAnyPlayerOnGroundNoDeadOrBubble(this)))
        al::setNerve(this, &NrvSuperbViewAreaLook);
}
void SuperbViewArea::exeLook() {
    if (al::isFirstStep(this)) {
        GameDataFunction::setIsInsideSuperbView(GameDataHolderWriter(this));
        al::onStageSwitch(this, "SwitchAreaOn");
        al::startSe(this, "LookStart");
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "景観ポイント", 60, false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "景観ポイント", 60, false);
        al::changeBgmSituation(this, "SuperbViewAreaIn");
        al::startCamera(this, mCamera, -1);
        for (int i = 0; i < mGuideCount; ++i) mGuides[i]->appear();
        if (mFilter) mFilter->appear();
    }
    if (!al::isInAreaObjPlayerAll(this, mArea)) {
        GameDataFunction::resetIsInsideSuperbView(GameDataHolderWriter(this));
        al::endCamera(this, mCamera, -1);
        al::requestOffGyroMode(this);
        al::setNerve(this, &NrvSuperbViewAreaWait);
        al::startSe(this, "LookEnd");
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "メイン", "通常", 80, false);
        alSeFunction::setRequestKeeperVolumeSetting(mAudioDirector, "常時", "通常", 80, false);
        al::offStageSwitch(this, "SwitchAreaOn");
        al::changeBgmSituation(this, "SuperbViewAreaOut");
        for (int i = 0; i < mGuideCount; ++i) mGuides[i]->kill();
        if (mFilter) mFilter->end();
    }
}
void SuperbViewArea::startPause() {
    for (int i = 0; i < mGuideCount; ++i) mGuides[i]->startPause();
    if (mFilter) mFilter->kill();
}
void SuperbViewArea::endPause() {
    for (int i = 0; i < mGuideCount; ++i) mGuides[i]->endPause();
    if (al::isNerve(this, &NrvSuperbViewAreaLook) && mFilter) {
        mFilter->appear();
        al::startFreezeActionEnd(mFilter, "Appear", nullptr);
    }
}
SuperbViewAreaHolder::SuperbViewAreaHolder() { mAreas.allocBuffer(10, nullptr); }
void SuperbViewAreaHolder::startPause() {
    for (int i = 0; i < mAreas.size(); ++i) mAreas[i]->startPause();
}
void SuperbViewAreaHolder::endPause() {
    for (int i = 0; i < mAreas.size(); ++i) mAreas[i]->endPause();
}
const char* SuperbViewAreaHolder::getSceneObjName() const { return "SuperbViewAreaHolder"; }
