#include "Game/MapObj/FlipPanel.hpp"
#include "Game/Util/DrcUtil.hpp"
#include "Game/Util/ProjectMsgUtil.hpp"
#include "Game/Util/ScoreUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Connector/MtxConnector.hpp"
#include "Library/Light/PrePassLightFunction.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Movement/KeyMoveMovement.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Obj/CollisionObj.hpp"
#include "Library/Obj/PartsFunction.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
namespace {
NERVE_DECL(FlipPanel, Watch)
NERVE_DECL(FlipPanel, Fix)
NERVE_DECL(FlipPanelParts, Question)
NERVE_DECL(FlipPanelParts, QuestionOn)
NERVE_DECL(FlipPanelParts, Exclamation)
NERVE_DECL(FlipPanelParts, ExclamationOn)
NERVE_DECL(FlipPanelParts, PrevEnd)
NERVE_DECL(FlipPanelParts, End)
NERVES_MAKE_NOSTRUCT(FlipPanel, Watch, Fix)
NERVES_MAKE_NOSTRUCT(FlipPanelParts, Question, QuestionOn, Exclamation, ExclamationOn, PrevEnd, End)
} // namespace
void FlipPanel::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "FlipPanelWatcher", nullptr);
    al::initNerve(this, &NrvFlipPanelWatch, 0);
    int count = al::calcLinkChildNum(info, "FlipPanelParts");
    if (count == 0) {
        makeActorDead();
        return;
    }
    al::invalidateClipping(this);
    mPanels.tryAllocBuffer(count, nullptr);
    for (int i = 0; i < count; ++i) {
        mPanels[i] = new FlipPanelParts("フリップパネルパーツ", this);
        al::initLinksActor(mPanels[i], info, "FlipPanelParts", i);
    }
    makeActorAppeared();
}
void FlipPanel::exeWatch() {
    int count = mPanels.size();
    bool complete = true;
    for (int i = 0; i < count; ++i) {
        if (!mPanels[i]->isExclamation())
            complete = false;
    }
    if (complete) {
        al::setNerve(this, &NrvFlipPanelFix);
        for (int i = 0; i < count; ++i)
            mPanels[i]->setPrevEndNerve();
        rc::addScore(this, mLastSensor, al::getTrans(mLastPanel), 0);
    }
}
bool FlipPanelParts::isExclamation() const {
    return al::isGreaterStep(this, 1) && (al::isNerve(this, &NrvFlipPanelPartsExclamation) ||
                                          al::isNerve(this, &NrvFlipPanelPartsExclamationOn));
}
void FlipPanelParts::setPrevEndNerve() { al::setNerve(this, &NrvFlipPanelPartsPrevEnd); }
void FlipPanel::exeFix() {
    if (al::isFirstStep(this))
        al::startSe(mPanels[0], "Complete", nullptr);
    if (al::isStep(this, 15)) {
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        int count = mPanels.size();
        for (int i = 0; i < count; ++i)
            mPanels[i]->setEndNerve();
    }
}
void FlipPanelParts::setEndNerve() { al::setNerve(this, &NrvFlipPanelPartsEnd); }
void FlipPanelParts::init(const al::ActorInitInfo& info) {
    al::tryGetArg(&mLightType, info, "LightPrePassStressType");
    if (info.getActorSceneInfo().isSingleMode)
        al::initActorWithArchiveName(this, info, "FlipPanel1P", nullptr);
    else if (mLightType == 2)
        al::initActorWithArchiveName(this, info, "FlipPanel", "NoLpp");
    else
        al::initActorWithArchiveName(this, info, "FlipPanel", nullptr);
    al::initNerve(this, &NrvFlipPanelPartsQuestion, 0);
    mMovement = new al::KeyMoveMovement(this, info);
    mConnector = al::tryCreateMtxConnector(this, info);
    al::initSubActorKeeperNoFile(this, info, 1);
    mCollision = al::createCollisionObj(this, info, "FlipPanelBase",
                                        al::getHitSensor(this, "BaseCollision"), nullptr, nullptr);
    al::registerSubActorSyncClipping(this, mCollision, false);
    makeActorAppeared();
    mCollision->makeActorAppeared();
}
void FlipPanelParts::initAfterPlacement() {
    if (mConnector)
        al::attachMtxConnectorToCollision(mConnector, this, 50.0f, 400.0f);
}
void FlipPanelParts::makeActorAppeared() {
    al::LiveActor::makeActorAppeared();
    mCollision->appear();
}
void FlipPanelParts::makeActorDead() {
    al::LiveActor::makeActorDead();
    mCollision->kill();
}
void FlipPanelParts::control() {
    if (mMovement) {
        mMovement->update();
        al::setTrans(this, mMovement->getMoveTrans());
    }
    mPreviousTouchTimer = mTouchTimer;
    if (mTouchTimer - 1 >= 0)
        --mTouchTimer;
    if (mConnector)
        al::connectPoseQT(this, mConnector);
}
bool FlipPanelParts::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender,
                                al::HitSensor* receiver) {
    if (receiver == al::getHitSensor(this, "BaseCollision"))
        return false;
    if (al::isNerve(this, &NrvFlipPanelPartsPrevEnd))
        return false;
    if (al::isNerve(this, &NrvFlipPanelPartsEnd))
        return false;
    if (al::isMsgPlayerTouch(msg) || al::isMsgBallAttackCollide(msg) ||
        al::isMsgBallTrampleCollide(msg) || rc::isMsgKillerTouch(msg) ||
        (al::isMsgTouchAssist(msg) && receiver == al::getHitSensor(this, "CollisionParts"))) {
        mTouchTimer = 4;
        if (!isExclamation() && mPreviousTouchTimer == 0 && mTouchTimer > 0) {
            if (al::isMsgTouchAssist(msg))
                sender = DrcFunction::tryFindDrcPlayerSensor(this, sender);
            mHost->setLastSensor(sender);
            mHost->setLastPanel(this);
        }
        return true;
    }
    return false;
}
void FlipPanelParts::exeQuestion() {
    if (al::isFirstStep(this)) {
        al::startSklAnim(this, "Off");
        al::startVisAnimAndSetFrameAndStop(this, "Visibility", 0.0f);
    }
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Blue", 1.0f);
    if (mPreviousTouchTimer == 0 && mTouchTimer > 0)
        al::setNerve(this, &NrvFlipPanelPartsExclamationOn);
}
void FlipPanelParts::exeQuestionOn() {
    if (al::isFirstStep(this)) {
        al::startSklAnim(this, "On");
        al::startVisAnimAndSetFrameAndStop(this, "Visibility", 0.0f);
        al::startSe(this, "QuestionOn", nullptr);
    }
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Blue", 1.0f);
    if (mTouchTimer <= 0)
        al::setNerve(this, &NrvFlipPanelPartsQuestion);
}
void FlipPanelParts::exeExclamation() {
    if (al::isFirstStep(this)) {
        al::startSklAnim(this, "Off");
        al::startVisAnimAndSetFrameAndStop(this, "Visibility", 1.0f);
    }
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Yellow", 1.0f);
    if (mPreviousTouchTimer == 0 && mTouchTimer > 0)
        al::setNerve(this, &NrvFlipPanelPartsQuestionOn);
}
void FlipPanelParts::exeExclamationOn() {
    if (al::isFirstStep(this)) {
        al::startSklAnim(this, "On");
        al::startVisAnimAndSetFrameAndStop(this, "Visibility", 1.0f);
        al::startSe(this, "ExclamationOn", nullptr);
    }
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Yellow", 1.0f);
    if (mTouchTimer <= 0)
        al::setNerve(this, &NrvFlipPanelPartsExclamation);
}
void FlipPanelParts::exePrevEnd() {
    al::isFirstStep(this);
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Yellow", 1.0f);
}
void FlipPanelParts::exeEnd() {
    if (al::isFirstStep(this)) {
        al::startSklAnim(this, "Off");
        al::startVisAnimAndSetFrameAndStop(this, "Visibility", 2.0f);
    }
    if (mLightType != 2)
        al::requestPrePassLightColor(this, "フリップパネル点光源", "Green", 1.0f);
}
