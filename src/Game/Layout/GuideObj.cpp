#include "MapObj/GuideObj.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMapUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Scene/SceneObjHolder.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
namespace {
NERVE_DECL(GuideObj, Wait);
NERVE_DECL(GuideObj, Appear);
NERVES_MAKE_NOSTRUCT(GuideObj, Wait, Appear)
void updateGuidePosition(al::LayoutActor* pLayout, const al::LiveActor* pActor) {
    sead::Vector2f layoutPos(0.0f, 0.0f);
    al::calcLayoutPosFromWorldPos(&layoutPos, pActor, al::getTrans(pActor), 0);
    al::setLocalTrans(pLayout, layoutPos);
}
}
/** @brief Creates an uninitialized guide actor. */
GuideObj::GuideObj(const char* pName) : al::LiveActor(pName) {}
/** @brief Initializes the guide layout, placement, and stage switch listeners. */
void GuideObj::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTQSV(this);
    al::initActorSRT(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initExecutorUpdate(this, rInfo, "地形オブジェ[Movement]");
    al::initNerve(this, &NrvGuideObjWait, 0);
    mGuide = new al::SimpleLayoutAppearWaitEnd("ガイドオブジェ", "GuideGoal", *rInfo.getLayoutInitInfo(), nullptr, false);
    updateGuidePosition(mGuide, this);
    al::trySyncStageSwitchAppear(this);
    al::tryListenStageSwitchKill(this);
}
/** @brief Registers the placed guide with the stage's optional guide holder. */
void GuideObj::initAfterPlacement() {
    al::LiveActor::initAfterPlacement();
    auto* pHolder = static_cast<GuideObjHolder*>(mActorSceneInfo->sceneObjHolder->tryGetObj(55));
    if (pHolder) pHolder->registerGuideObj(this);
}
/** @brief Activates the guide after a short placement delay. */
void GuideObj::appear() {
    al::LiveActor::appear();
    al::setNerve(this, &NrvGuideObjAppear);
}
/** @brief Deactivates the actor and ends the guide layout. */
void GuideObj::kill() {
    al::LiveActor::kill();
    mGuide->end();
}
/** @brief Saves visibility and hides the guide while paused. */
void GuideObj::startPause() {
    mWasGuideAlive = mGuide->isAlive();
    mGuide->kill();
}
/** @brief Restores a visible guide without restarting its animation. */
void GuideObj::endPause() {
    if (al::isNerve(this, &NrvGuideObjWait) && mWasGuideAlive)
        mGuide->al::LayoutActor::appear();
}
/** @brief Follows the actor and shows the guide after two frames. */
void GuideObj::exeAppear() {
    updateGuidePosition(mGuide, this);
    if (al::isGreaterStep(this, 2)) {
        mGuide->appear();
        al::setNerve(this, &NrvGuideObjWait);
    }
}
/** @brief Follows the actor while the guide is active. */
void GuideObj::exeWait() { updateGuidePosition(mGuide, this); }
/** @brief Allocates space for ten guide actors. */
GuideObjHolder::GuideObjHolder() { mGuides.allocBuffer(10, nullptr); }
/** @brief Registers a guide if the holder has free capacity. */
void GuideObjHolder::registerGuideObj(GuideObj* pGuide) { mGuides.pushBack(pGuide); }
/** @brief Hides all registered guide layouts. */
void GuideObjHolder::startPause() {
    for (int i = 0; i < mGuides.size(); ++i) mGuides[i]->startPause();
}
/** @brief Restores guide layouts that were visible before pausing. */
void GuideObjHolder::endPause() {
    for (int i = 0; i < mGuides.size(); ++i) mGuides[i]->endPause();
}
