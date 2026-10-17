#include "MapObj/KarakuriCastleDoorWatcher.hpp"
#include "MapObj/KarakuriCastleDoor.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
namespace {
    NERVE_DECL(KarakuriCastleDoorWatcher, Wait);
    NERVE_DECL(KarakuriCastleDoorWatcher, Open);
    NERVES_MAKE_NOSTRUCT(KarakuriCastleDoorWatcher, Wait, Open)
}
KarakuriCastleDoorWatcher::KarakuriCastleDoorWatcher(const char* pName) : al::LiveActor(pName) {}
KarakuriCastleDoorWatcher::~KarakuriCastleDoorWatcher() {}
void KarakuriCastleDoorWatcher::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initNerve(this, &NrvKarakuriCastleDoorWatcherWait, 0);
    float moveSpeed = 20.0f;
    al::tryGetArg(&moveSpeed, rInfo, "MoveSpeed");
    mDoorCount = al::calcLinkChildNum(rInfo, "OpenDoor");
    if (mDoorCount == 0) {
        makeActorDead();
        return;
    }
    mDoors = new KarakuriCastleDoor*[mDoorCount];
    for (int i = 0; i < mDoorCount; ++i) {
        auto* door = new KarakuriCastleDoor("からくり城襖");
        al::initLinksActor(door, rInfo, "OpenDoor", i);
        mDoors[i] = door;
        for (int j = i; j > 0; --j) {
            float distance = al::calcDistance(mDoors[j], this);
            float previousDistance = al::calcDistance(mDoors[j - 1], this);
            if (!(distance > previousDistance))
                break;
            auto* previous = mDoors[j - 1];
            auto* current = mDoors[j];
            mDoors[j] = previous;
            mDoors[j - 1] = current;
        }
    }
    for (int i = 0; i < mDoorCount; ++i)
        mDoors[i]->setParam(i, mDoorCount, moveSpeed);
    makeActorAppeared();
}
void KarakuriCastleDoorWatcher::exeWait() {
    for (int i = 0; i < mDoorCount; ++i) {
        auto* door = mDoors[i];
        if (door->isOpenReady()) {
            mOpenIndex = door->getIndex();
            al::setNerve(this, &NrvKarakuriCastleDoorWatcherOpen);
            return;
        }
    }
}
void KarakuriCastleDoorWatcher::exeOpen() {
    if (al::isFirstStep(this)) {
        sead::Vector3f destination(0.0f, 0.0f, 0.0f);
        if (mOpenIndex < mDoorCount - 1)
            destination.set(al::getTrans(mDoors[mOpenIndex + 1]));
        else
            destination.set(al::getTrans(this));
        for (int i = 0; i <= mOpenIndex; ++i)
            mDoors[i]->startOpen(destination, mOpenIndex + 1);
    }
    if (al::isValidStageSwitch(this, "SwitchOpenOn") && !al::isOnStageSwitch(this, "SwitchOpenOn")) {
        for (int i = 0; i < mDoorCount; ++i) {
            if (mDoors[i]->isOpen()) {
                al::tryOnStageSwitch(this, "SwitchOpenOn");
                return;
            }
        }
    }
    for (int i = 0; i < mDoorCount; ++i) {
        if (mDoors[i]->isOpening())
            return;
    }
    al::setNerve(this, &NrvKarakuriCastleDoorWatcherWait);
}
