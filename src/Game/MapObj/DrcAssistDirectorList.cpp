#include "MapObj/DrcAssistDirectorList.hpp"
#include "Layout/TouchEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcTouchAssistInfo.hpp"
#include "MapObj/DrcTouchPointer.hpp"
#include "MapObj/Fury/DrcTouchEffectTraceTracker.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
bool pointerEnabled(const DrcAssistDirector& director) {
    auto* info = director.getTouchInfo();
    return info && info->isTouch();
}
void disappear(DrcAssistDirector& director) {
    al::killLayoutIfActive(director.getTouchEffect());
    if (al::isAlive(director.getTouchPointerActor()))
        director.getTouchPointerActor()->startDisappearForce();
}
void slideOnScreen(sead::Vector2f* out, const DrcAssistDirector& director) {
    out->x = director.getTouchScreenPos().x - director.getPrevTouchScreenPos().x;
    out->y = director.getTouchScreenPos().y - director.getPrevTouchScreenPos().y;
}
} // namespace

DrcAssistDirectorList::DrcAssistDirectorList(int maxNum) {
    mDirectors.allocBuffer(maxNum, nullptr);
    mTraceTracker = new DrcTouchEffectTraceTracker(maxNum);
}

void DrcAssistDirectorList::setEnable(bool enable) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->setEnableUpdate(enable);
}

void DrcAssistDirectorList::validate() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->setValidFlag(true);
}

void DrcAssistDirectorList::invalidateImmediate() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        iter->setValidFlag(false);
        al::killLayoutIfActive(iter->getTouchEffect());
        al::tryKillEmitterAndParticleAll(iter->getTouchPointerActor());
        if (al::isAlive(iter->getTouchPointerActor()))
            iter->getTouchPointerActor()->kill();
    }
}

void DrcAssistDirectorList::checkPreStartPose() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        if (!iter->getValidFlag())
            continue;
        sead::Vector3f threshold(0.2f, 0.125f, 0.2f);
        sead::Vector3f limit(0.6f, -0.35f, 0.6f);
        sead::Vector3f accel;
        if (iter->isPoseExtreme(&accel, threshold, limit) < 0) {
            float diff = sead::Mathf::abs(-1.0f - accel.y);
            if (diff < iter->getMinPoseDiff()) {
                iter->setMinPoseDiff(diff);
                iter->calcJoyConBaseAngle();
                iter->setPoseChecked();
            }
        }
    }
}

void DrcAssistDirectorList::update() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->update();
}

bool DrcAssistDirectorList::isTouch() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 && pointerEnabled(*iter))
            return true;
    return false;
}

bool DrcAssistDirectorList::isTouch(const sead::Vector3f& position, float radius) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 &&
            al::isTouchPosInCircleByWorldPos(position, iter->getTouchPointerActor(), radius, 10.0f,
                                             iter->getTouchPadPort()))
            return true;
    return false;
}

bool DrcAssistDirectorList::isTouchByPointer(const al::LiveActor* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor() == actor)
            return iter->getTouchFrame() >= 1 && pointerEnabled(*iter);
    return false;
}

bool DrcAssistDirectorList::isTouchTrigger(const sead::Vector3f& position, float radius) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchInfo()->_1 &&
            al::isTouchPosInCircleByWorldPos(position, iter->getTouchPointerActor(), radius, 10.0f,
                                             iter->getTouchPadPort()))
            return true;
    return false;
}

bool DrcAssistDirectorList::isEnableTouchPointer() const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && pointerEnabled(*iter))
            return true;
    return false;
}

bool DrcAssistDirectorList::isEnableTouchPointerGrabItem(const al::LiveActor* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor()->isEnableGrabItem(actor))
            return true;
    return false;
}

void DrcAssistDirectorList::releaseTouchPointerHoldItem(const al::LiveActor* pointer) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        if (iter->getTouchPointerActor() == pointer) {
            if (iter->getTouchPointerActor()->isCompletelyAlive())
                iter->getTouchPointerActor()->startRelease();
            return;
        }
    }
}

void DrcAssistDirectorList::releaseTouchPointerAllHoldItem() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor()->isCompletelyAlive())
            iter->getTouchPointerActor()->startRelease();
}

al::LiveActor*
DrcAssistDirectorList::findDrcAssistDirectorPlayer(const al::ScreenPointer* pointer) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isPointer(pointer)) {
            int port = iter->getPort();
            return rc::tryFindPlayerFromInputPort(iter->getPlayerHolder(), port, false);
        }
    return nullptr;
}

al::LiveActor* DrcAssistDirectorList::findDrcAssistDirectorPlayer(const al::HitSensor* pointer) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isSensor(pointer)) {
            int port = iter->getPort();
            return rc::tryFindPlayerFromInputPort(iter->getPlayerHolder(), port, false);
        }
    return nullptr;
}

al::LiveActor*
DrcAssistDirectorList::findDrcAssistDirectorTouchPointer(const al::ScreenPointer* pointer) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isPointer(pointer))
            return iter->getTouchPointerActor();
    return nullptr;
}

DrcTouchPointer* DrcAssistDirectorList::getTouchPointer() const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor())
            return iter->getTouchPointerActor();
    return nullptr;
}

al::LiveActor* DrcAssistDirectorList::getFirstTouchingTouchPointer() const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 && pointerEnabled(*iter))
            return iter->getTouchPointerActor();
    return nullptr;
}

const sead::Vector3f&
DrcAssistDirectorList::getTouchPointerPosition(const al::LiveActor* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && (iter->getTouchInfo()->isTouch() || iter->getTouchInfo()->_49))
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}

const sead::Vector3f&
DrcAssistDirectorList::getTouchedPointerPosition(const al::LiveActor* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchInfo()->isTouch())
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}

const sead::Vector3f&
DrcAssistDirectorList::getTouchPointerPosition(const al::IUseSceneObjHolder* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && (iter->getTouchInfo()->isTouch() || iter->getTouchInfo()->_49))
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}

const sead::Vector3f&
DrcAssistDirectorList::getTouchPointerNormal(const al::LiveActor* actor) const {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist())
            return iter->getTouchPointerActor()->getHitNormal();
    return sead::Vector3f::ey;
}

void DrcAssistDirectorList::calcTouchPointerUpDir(sead::Vector3f* out) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist())
            al::calcQuatUp(out, al::getQuat(iter->getTouchPointerActor()));
}

bool DrcAssistDirectorList::tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* out,
                                                                const al::LiveActor* actor) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor() == actor) {
            slideOnScreen(out, *iter);
            return true;
        }
    return false;
}

bool DrcAssistDirectorList::tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f* out, int port) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getPort() == port) {
            slideOnScreen(out, *iter);
            return true;
        }
    return false;
}

bool DrcAssistDirectorList::tryCalcTouchPointerSlideDirOnWorldByPointer(
    sead::Vector3f* out, const al::LiveActor* actor) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor() == actor) {
            iter->tryCalcTouchPointerSlideDirOnWorld(out, nullptr);
            return true;
        }
    return false;
}

void DrcAssistDirectorList::setStampDirector(rc::StampDirector* director) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        iter->mStampDirector = director;
}

void DrcAssistDirectorList::setPlayerAliveWatcher(PlayerAliveWatcher* watcher) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        iter->mPlayerAliveWatcher = watcher;
}

void DrcAssistDirectorList::setPlayerHolder(al::PlayerHolder* holder) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        iter->mPlayerHolder = holder;
}

void DrcAssistDirectorList::setIgnoreTriggerFrames(int frames) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        iter->mIgnoreTriggerFrames = frames;
}

void DrcAssistDirectorList::setKinopioBrigadeFlag(bool brigade) {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->setMiddleRange(brigade);
}

void DrcAssistDirectorList::addTouchAssist(int port, bool isEnable) {
    mDirectors.emplaceFront(port, isEnable);
}

void DrcAssistDirectorList::initAfterPlacementSceneObj(const al::ActorInitInfo& info) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        iter->initAfterPlacementSceneObj(info, mTraceTracker);
}

void DrcAssistDirectorList::removeTouchAssist(int port) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        if (iter->getPort() == port) {
            mDirectors.erase(iter.operator->());
            return;
        }
    }
}

void DrcAssistDirectorList::setTouchDisabled(bool disabled) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
    }
}

void DrcAssistDirectorList::disappearTouchPointer() {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        disappear(*iter);
}

void DrcAssistDirectorList::disappearTouchPointerImmediately() {
    auto& list = mDirectors;
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        al::killLayoutIfActive(iter->getTouchEffect());
        al::tryKillEmitterAndParticleAll(iter->getTouchPointerActor());
        if (al::isAlive(iter->getTouchPointerActor()))
            iter->getTouchPointerActor()->kill();
    }
}

void DrcAssistDirectorList::disappearTouchPointerEffect() {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter)
        al::killLayoutIfActive(iter->getTouchEffect());
}

void DrcAssistDirectorList::invalidate(bool force) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        if (force)
            iter->setValidFlag(false);
        if (!iter->getValidFlag())
            disappear(*iter);
    }
}

bool DrcAssistDirectorList::tryCalcTouchPointerSlideDirOnWorld(sead::Vector3f* out,
                                                               const al::IUseCamera* camera) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        if (iter->isPlayerExist()) {
            iter->tryCalcTouchPointerSlideDirOnWorld(out, camera);
            return true;
        }
    }
    return false;
}

void DrcAssistDirectorList::keepAppearPointer() {}

void DrcAssistDirectorList::startSnapshotMode() {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        iter->getTouchPointerActor()->startSnapshotMode();
        int port = iter->getPort();
        if (port == al::getMainControllerPort()) {
            iter->getTouchEffect()->setStampIcon(true);
            iter->getTouchEffect()->appear();
        } else {
            iter->getTouchEffect()->kill();
            iter->mIsSnapshotMode = true;
        }
    }
}

void DrcAssistDirectorList::endSnapshotMode() {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        iter->mIsSnapshotMode = false;
        iter->getTouchEffect()->setStampIcon(false);
        iter->getTouchPointerActor()->endSnapshotMode();
        if (!iter->mIsEnableUpdate && iter->getTouchEffect()->isAlive())
            iter->getTouchEffect()->kill();
    }
}

bool DrcAssistDirectorList::isPlayerHoldingStamp() {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        if (iter->getTouchPointerActor()->isHoldStamp())
            return true;
    }
    return false;
}

DrcAssistDirector* DrcAssistDirectorList::getDrcAssist(int port) {
    for (auto iter = mDirectors.begin(); iter != mDirectors.end(); ++iter) {
        if (iter->getPort() == port)
            return iter.operator->();
    }
    return nullptr;
}
