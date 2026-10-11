#include "MapObj/IslandKeyMoveMapParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/KeyPose/KeyPoseKeeper.hpp"
#include "Library/KeyPose/KeyPoseKeeperUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"

IslandKeyMoveMapParts::IslandKeyMoveMapParts(const char* pName) : al::KeyMoveMapParts(pName) {}
IslandKeyMoveMapParts::~IslandKeyMoveMapParts() {}

void IslandKeyMoveMapParts::init(const al::ActorInitInfo& rInfo) {
    al::KeyMoveMapParts::init(rInfo);
    al::tryGetTrans(&mInitialPosition, rInfo.getPlacementInfo());
    mAttachedObjects.init(rInfo, false);
}

void IslandKeyMoveMapParts::control() { al::KeyMoveMapParts::control(); }
void IslandKeyMoveMapParts::startClipped() { al::LiveActor::startClipped(); }
void IslandKeyMoveMapParts::endClipped() { al::LiveActor::endClipped(); }

void IslandKeyMoveMapParts::changeScenarioID(int id, bool immediate) {
    int count = mKeyPoseKeeper->getKeyPoseCount();
    id = count > id ? id : count - 1;
    if (id <= mDesiredScenario)
        return;
    mDesiredScenario = id;
    if (immediate) {
        mCurrentScenario = id;
        mKeyPoseKeeper->setCurrentPoseIndex(id);
        al::getKeyPoseTrans(al::getTransPtr(this), mKeyPoseKeeper, mKeyPoseKeeper->getKeyPoseCurrentIdx());
        al::getKeyPoseQuat(al::getQuatPtr(this), mKeyPoseKeeper, mKeyPoseKeeper->getKeyPoseCurrentIdx());
        mAttachedObjects.syncObjectsToPosition(al::getTrans(this));
        al::startNerveAction(this, "Wait");
    } else {
        mKeyPoseKeeper->setCurrentPoseIndex(mCurrentScenario);
        al::startNerveAction(this, "Move");
    }
}

void IslandKeyMoveMapParts::exeWait() {}

void IslandKeyMoveMapParts::exeMove() {
    if (al::isFirstStep(this)) {
        if (al::isExistAction(this, "MoveLoop"))
            al::tryStartActionIfNotPlaying(this, "MoveLoop");
        mKeyMoveMoveTime = al::calcKeyMoveMoveTime(mKeyPoseKeeper);
        mSeMoveName = getMoveSeName(mKeyPoseKeeper->getKeyPoseCurrentIdx());
        if (mSeMoveName)
            al::tryStartSe(this, mSeMoveName, nullptr);
    }
    float rate = al::calcNerveRate(this, mKeyMoveMoveTime);
    al::calcLerpKeyTrans(al::getTransPtr(this), mKeyPoseKeeper, rate);
    al::calcSlerpKeyQuat(al::getQuatPtr(this), mKeyPoseKeeper, rate);
    mAttachedObjects.syncObjectsToPosition(al::getTrans(this));
    if (al::isGreaterEqualStep(this, mKeyMoveMoveTime)) {
        ++mCurrentScenario;
        if (mCurrentScenario == mDesiredScenario) {
            al::tryStartAction(this, "Stop");
            al::tryStartSe(this, "MoveEnd", nullptr);
            al::startNerveAction(this, "Wait");
        } else {
            al::nextKeyPose(mKeyPoseKeeper);
            al::startNerveAction(this, "Move");
        }
    }
}
