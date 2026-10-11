#include "MapObj/TrampleSwitchGoalItem.hpp"
#include "Layout/GuideBalloon.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Collision/PartsConnectorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Camera/Info/SceneCameraInfo.hpp"
#include "Project/Camera/Info/CameraViewInfo.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "Util/ProjectMsgUtil.hpp"

namespace {
    NERVE_DECL(TrampleSwitchGoalItem, OffWait);
    NERVE_DECL(TrampleSwitchGoalItem, OnWait);
    NERVE_DECL(TrampleSwitchGoalItem, On);
    NERVE_DECL(TrampleSwitchGoalItem, Reaction);
    NERVES_MAKE_NOSTRUCT(TrampleSwitchGoalItem, OffWait, OnWait, On, Reaction)
}

TrampleSwitchGoalItem::TrampleSwitchGoalItem(const char* pName) : al::LiveActor(pName) {
}

void TrampleSwitchGoalItem::init(const al::ActorInitInfo& rInfo) {
    al::initActor(this, rInfo);
    al::tryGetArg(&mGoalItemsNeeded, rInfo, "GoalItemsNeeded");
    al::initNerve(this, &NrvTrampleSwitchGoalItemOffWait, 0);
    mMtxConnector = al::createMtxConnector(this);
    mGuide = new GuideBalloon("guide", al::getLayoutInitInfo(rInfo), al::getTransPtr(this),
                              sead::Vector3f(0.0f, 100.0f, 0.0f), false, nullptr);
    if (al::listenStageSwitchOnAppear(this, al::Functor(this, &TrampleSwitchGoalItem::appearBySwitch))) {
        makeActorDead();
    } else {
        makeActorAppeared();
    }
    al::setMtpAnimFrameAndStop(this, 0.0f);
}

void TrampleSwitchGoalItem::appearBySwitch() {
    al::LiveActor::appear();
}

void TrampleSwitchGoalItem::initAfterPlacement() {
    al::attachMtxConnectorToCollision(mMtxConnector, this, false);
}

void TrampleSwitchGoalItem::control() {
    al::connectPoseQT(this, mMtxConnector);
    if (SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(this)) >= mGoalItemsNeeded &&
        al::getMtpAnimFrame(this) == 0.0f) {
        al::setMtpAnimFrameAndStop(this, 1.0f);
    }
}

bool TrampleSwitchGoalItem::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pOther, al::HitSensor* pSelf) {
    if (al::isNerve(this, &NrvTrampleSwitchGoalItemOnWait) ||
        al::isNerve(this, &NrvTrampleSwitchGoalItemOn)) {
        return false;
    }
    if (al::isMsgPlayerFloorTouch(pMsg)) {
        if (SingleModeDataFunction::getGoalItemsCollected(GameDataHolderAccessor(this)) >= mGoalItemsNeeded) {
            al::invalidateClipping(this);
            rc::sendMsgRequestPlayerGetReaction(pOther, pSelf, "キャラクタースイッチ踏み");
            al::setNerve(this, &NrvTrampleSwitchGoalItemOn);
            return true;
        }
        if (al::isNerve(this, &NrvTrampleSwitchGoalItemOffWait)) {
            mReactionFrames = 2;
            al::setNerve(this, &NrvTrampleSwitchGoalItemReaction);
            return true;
        }
        if (al::isNerve(this, &NrvTrampleSwitchGoalItemReaction)) {
            mReactionFrames = 2;
        }
    }
    return false;
}

void TrampleSwitchGoalItem::exeOffWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::startAction(this, "OffWait");
    }
    if (al::getMtpAnimFrame(this) == 0.0f) {
        sead::Vector3f cameraPos = getSceneCameraInfo()->getViewAt(0)->getLookAtCam().getPos();
        sead::Vector3f trans = al::getTrans(this);
        sead::Vector3f toCamera = cameraPos - trans;
        bool isHidden = true;
        if (!(toCamera.squaredLength() > 1000000.0f)) {
            al::CollisionPartsFilterActor filter(this);
            isHidden = alCollisionUtil::getStrikeArrowCollisionParts(this, nullptr, trans, toCamera, &filter, nullptr) != nullptr;
        }
        if (mGuide->isAlive()) {
            if (isHidden) {
                mGuide->endShow();
            }
        } else if (!isHidden) {
            mGuide->startShowGoalItem(mGoalItemsNeeded);
        }
    }
}

void TrampleSwitchGoalItem::exeOn() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "On");
        mGuide->kill();
    }
    if (al::isActionEnd(this)) {
        al::setNerve(this, &NrvTrampleSwitchGoalItemOnWait);
    }
}

void TrampleSwitchGoalItem::exeOnWait() {
    if (al::isFirstStep(this)) {
        al::validateClipping(this);
        al::tryOnStageSwitch(this, "SwitchTrampleOn");
        al::startAction(this, "OnWait");
    }
}

void TrampleSwitchGoalItem::exeReaction() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Reaction");
        mGuide->endShow();
    }
    if (mReactionFrames <= 0) {
        al::setNerve(this, &NrvTrampleSwitchGoalItemOffWait);
    }
    --mReactionFrames;
}

TrampleSwitchGoalItem::~TrampleSwitchGoalItem() {
}
