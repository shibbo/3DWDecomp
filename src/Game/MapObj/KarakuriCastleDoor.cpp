#include "MapObj/KarakuriCastleDoor.hpp"
#include "Layout/GuideBalloon.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Player/PlayerUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
namespace {
    NERVE_ACTION_IMPL(KarakuriCastleDoor, OpenWait)
    NERVE_ACTION_IMPL(KarakuriCastleDoor, OpenReady)
    NERVE_ACTION_IMPL(KarakuriCastleDoor, Opening)
    NERVE_ACTION_IMPL(KarakuriCastleDoor, OpenFinish)
    NERVE_ACTIONS_MAKE_STRUCT(KarakuriCastleDoor, OpenWait, OpenReady, Opening, OpenFinish)
}
KarakuriCastleDoor::KarakuriCastleDoor(const char* pName) : al::LiveActor(pName) {}
KarakuriCastleDoor::~KarakuriCastleDoor() {}
void KarakuriCastleDoor::init(const al::ActorInitInfo& rInfo) {
    al::initNerveAction(this, "OpenWait", &NrvKarakuriCastleDoor.collector, 0);
    al::initActorWithArchiveName(this, rInfo, "KarakuriCastleDoor", nullptr);
    al::registerAreaHostMtx(this, rInfo);
    mInitialPosition.set(al::getTrans(this));
    mDestination.set(al::getTrans(this));
    al::tryGetArg(&mGuideBalloonType, rInfo, "GuideBalloonType");
    if (mGuideBalloonType != 0)
        mGuideBalloon = new GuideBalloon("ガイドバルーン", al::getLayoutInitInfo(rInfo), al::getTransPtr(this), sead::Vector3f(0.0f, 100.0f, 0.0f), false, nullptr);
    makeActorAppeared();
}
bool KarakuriCastleDoor::receiveMsg(const al::SensorMsg* pMsg, al::HitSensor*, al::HitSensor*) {
    if (al::isMsgTouchAssist(pMsg)) {
        if (al::isNerve(this, NrvKarakuriCastleDoor.OpenWait.data())) {
            al::startNerveAction(this, "OpenReady");
            return true;
        }
        return false;
    }
    return false;
}
void KarakuriCastleDoor::startClipped() {
    al::LiveActor::startClipped();
    if (mGuideBalloon && al::isNerve(this, NrvKarakuriCastleDoor.OpenWait.data()))
        mGuideBalloon->endShow();
}
void KarakuriCastleDoor::exeOpenWait() {
    if (al::isFirstStep(this))
        al::validateClipping(this);
    if (mGuideBalloon) {
        if (mGuideBalloon->isAlive()) {
            if (!al::isNearPlayer(this, 2800.0f))
                mGuideBalloon->endShow();
        } else if (al::isNearPlayer(this, 2500.0f)) {
            mGuideBalloon->startShowDrcTouch(false);
        }
    }
}
void KarakuriCastleDoor::exeOpenReady() {}
void KarakuriCastleDoor::exeOpening() {
    if (al::isFirstStep(this)) {
        float distance = al::getTrans(this).x - mDestination.x;
        if (distance > 0.0f)
            mMoveSpeed = -sead::Mathf::abs(mMoveSpeed);
        mMoveFrames = sead::Mathf::abs(distance / mMoveSpeed);
        if (mGuideBalloon)
            mGuideBalloon->endShow();
    }
    al::setVelocityX(this, mMoveSpeed);
    if (al::isGreaterEqualStep(this, mMoveFrames)) {
        al::setTrans(this, mDestination);
        al::tryStartSe(this, "OnEnd", nullptr);
        al::setVelocityZero(this);
        if (mIndex == mDoorCount)
            al::startNerveAction(this, "OpenFinish");
        else
            al::startNerveAction(this, "OpenWait");
    }
}
void KarakuriCastleDoor::exeOpenFinish() {
    if (al::isFirstStep(this))
        al::validateClipping(this);
}
void KarakuriCastleDoor::setParam(int index, int doorCount, float speed) {
    mIndex = index;
    mDoorCount = doorCount;
    mMoveSpeed = speed;
}
void KarakuriCastleDoor::startOpen(const sead::Vector3f& rDestination, int index) {
    if (al::isNerve(this, NrvKarakuriCastleDoor.Opening.data()))
        return;
    al::invalidateClipping(this);
    mIndex = index;
    mDestination.x = rDestination.x;
    al::startNerveAction(this, "Opening");
}
bool KarakuriCastleDoor::isOpenReady() const { return al::isNerve(this, NrvKarakuriCastleDoor.OpenReady.data()); }
bool KarakuriCastleDoor::isOpening() const { return al::isNerve(this, NrvKarakuriCastleDoor.Opening.data()); }
bool KarakuriCastleDoor::isOpen() const {
    return al::isNerve(this, NrvKarakuriCastleDoor.OpenWait.data()) || al::isNerve(this, NrvKarakuriCastleDoor.OpenFinish.data());
}
