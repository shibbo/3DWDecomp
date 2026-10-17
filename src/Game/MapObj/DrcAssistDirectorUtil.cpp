#include "MapObj/DrcAssistDirectorUtil.hpp"
#include "Layout/TouchEffect.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Controller/InputFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "MapObj/DrcAssistDirector.hpp"
#include "MapObj/DrcAssistDirectorList.hpp"
#include "MapObj/DrcTouchAssistInfo.hpp"
#include "MapObj/DrcTouchPointer.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"
#include "Util/PlayerUtil.hpp"
namespace {
auto &directors(const al::IUseSceneObjHolder *holder) {
    return static_cast<DrcAssistDirectorList *>(al::getSceneObj(holder, 4))->getDirectors();
}
bool pointerEnabled(const DrcAssistDirector &director) {
    auto *info = director.getTouchInfo();
    return info && info->isTouch();
}
void disappear(DrcAssistDirector &director) {
    al::killLayoutIfActive(director.getTouchEffect());
    if (al::isAlive(director.getTouchPointerActor()))
        director.getTouchPointerActor()->startDisappearForce();
}
void slideOnScreen(sead::Vector2f *out, const DrcAssistDirector &director) {
    out->x = director.getTouchScreenPos().x - director.getPrevTouchScreenPos().x;
    out->y = director.getTouchScreenPos().y - director.getPrevTouchScreenPos().y;
}
} // namespace
namespace rc {
void setDrcAssistEnable(const al::IUseSceneObjHolder *holder, bool enable) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->setEnableUpdate(enable);
}
void validateDrcAssist(const al::IUseSceneObjHolder *holder) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->setValidFlag(true);
}
void invalidateDrcAssist(const al::IUseSceneObjHolder *holder, bool force) {
    static_cast<DrcAssistDirectorList *>(al::getSceneObj(holder, 4))->invalidate(force);
}
void setValidDrcAssist(const al::IUseSceneObjHolder *holder, bool valid) {
    validateDrcAssist(holder);
    invalidateDrcAssist(holder, !valid);
}
void setValidDrcAssist(const al::IUseSceneObjHolder *holder, bool valid, int port) {
    DrcAssistDirector *found = nullptr;
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        if (iter->getPort() == port) {
            found = iter.operator->();
            break;
        }
    }
    if (found) {
        found->setValidFlag(true);
        if (!valid) {
            found->setValidFlag(false);
            disappear(*found);
        }
    }
}
void invalidateDrcAssistImmediate(const al::IUseSceneObjHolder *holder) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        iter->setValidFlag(false);
        al::killLayoutIfActive(iter->getTouchEffect());
        al::tryKillEmitterAndParticleAll(iter->getTouchPointerActor());
        if (al::isAlive(iter->getTouchPointerActor()))
            iter->getTouchPointerActor()->kill();
    }
}
void checkPreStartDrcAssistPose(const al::IUseSceneObjHolder *holder) {
    auto &list = directors(holder);
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
void updateDrcAssistDirector(const al::IUseSceneObjHolder *holder) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        iter->update();
}
bool isTouchDrcAssist(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 && pointerEnabled(*iter))
            return true;
    return false;
}
bool isTouchDrcAssist(const al::LiveActor *actor, const sead::Vector3f &position, float radius) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 &&
            al::isTouchPosInCircleByWorldPos(position, iter->getTouchPointerActor(), radius, 10.0f,
                                             iter->getTouchPadPort()))
            return true;
    return false;
}
bool isTouchDrcAssistByPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor() == actor)
            return iter->getTouchFrame() >= 1 && pointerEnabled(*iter);
    return false;
}
bool isTouchTriggerDrcAssist(const al::LiveActor *actor, const sead::Vector3f &position,
                             float radius) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchInfo()->_1 &&
            al::isTouchPosInCircleByWorldPos(position, iter->getTouchPointerActor(), radius, 10.0f,
                                             iter->getTouchPadPort()))
            return true;
    return false;
}
bool isEnableTouchPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && pointerEnabled(*iter))
            return true;
    return false;
}
bool isEnableTouchPointer(const al::IUseSceneObjHolder *holder) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && pointerEnabled(*iter))
            return true;
    return false;
}
bool isEnableTouchPointerGrabItem(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor()->isEnableGrabItem(actor))
            return true;
    return false;
}
void releaseTouchPointerHoldItem(const al::LiveActor *actor, const al::LiveActor *pointer) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter) {
        if (iter->getTouchPointerActor() == pointer) {
            if (iter->getTouchPointerActor()->isCompletelyAlive())
                iter->getTouchPointerActor()->startRelease();
            return;
        }
    }
}
void releaseAllTouchPointerHoldItem(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor()->isCompletelyAlive())
            iter->getTouchPointerActor()->startRelease();
}
al::LiveActor *findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder *holder,
                                           const al::ScreenPointer *pointer) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isPointer(pointer)) {
            int port = iter->getPort();
            return rc::tryFindPlayerFromInputPort(iter->getPlayerHolder(), port, false);
        }
    return nullptr;
}
al::LiveActor *findDrcAssistDirectorPlayer(const al::IUseSceneObjHolder *holder,
                                           const al::HitSensor *pointer) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isSensor(pointer)) {
            int port = iter->getPort();
            return rc::tryFindPlayerFromInputPort(iter->getPlayerHolder(), port, false);
        }
    return nullptr;
}
al::LiveActor *findDrcAssistDirectorTouchPointer(const al::IUseSceneObjHolder *holder,
                                                 const al::ScreenPointer *pointer) {
    auto &list = directors(holder);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor()->isPointer(pointer))
            return iter->getTouchPointerActor();
    return nullptr;
}
al::LiveActor *getTouchPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor())
            return iter->getTouchPointerActor();
    return nullptr;
}
al::LiveActor *getFirstTouchingTouchPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 && pointerEnabled(*iter))
            return iter->getTouchPointerActor();
    return nullptr;
}
DrcTouchPointer *getFirstTouchingDrcTouchPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchFrame() >= 1 && pointerEnabled(*iter))
            return iter->getTouchPointerActor();
    return nullptr;
}
const sead::Vector3f &getTouchPointerPosition(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && (iter->getTouchInfo()->isTouch() || iter->getTouchInfo()->_49))
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}
const sead::Vector3f &getTouchedPointerPosition(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchInfo()->isTouch())
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}
const sead::Vector3f &getTouchPointerPosition(const al::IUseSceneObjHolder *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && (iter->getTouchInfo()->isTouch() || iter->getTouchInfo()->_49))
            return al::getTrans(iter->getTouchPointerActor());
    return sead::Vector3f::zero;
}
const sead::Vector3f &getTouchPointerNormal(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist())
            return iter->getTouchPointerActor()->getHitNormal();
    return sead::Vector3f::ey;
}
void calcTouchPointerUpDir(sead::Vector3f *out, const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist())
            al::calcQuatUp(out, al::getQuat(iter->getTouchPointerActor()));
}
void getTouchPointerUpDir(sead::Vector3f *out, const al::LiveActor *actor) {
    al::calcQuatUp(out, al::getQuat(actor));
}
bool tryCalcTouchPointerSlideDirOnScreenByPointer(sead::Vector2f *out, const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor() == actor) {
            slideOnScreen(out, *iter);
            return true;
        }
    return false;
}
bool tryCalcTouchPointerSlideDirOnScreen(sead::Vector2f *out, const al::LiveActor *actor,
                                         int port) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getPort() == port) {
            slideOnScreen(out, *iter);
            return true;
        }
    return false;
}
bool tryCalcTouchPointerSlideDirOnWorldByPointer(sead::Vector3f *out, const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->isPlayerExist() && iter->getTouchPointerActor() == actor) {
            iter->tryCalcTouchPointerSlideDirOnWorld(out, nullptr);
            return true;
        }
    return false;
}
float tryCalcTouchPointerSlideSpeedOnScreenByPointer(const al::LiveActor *actor) {
    auto &list = directors(actor);
    for (auto iter = list.begin(); iter != list.end(); ++iter)
        if (iter->getTouchPointerActor() == actor)
            return (iter->getTouchScreenPos() - iter->getPrevTouchScreenPos()).length();
    return 0.0f;
}
} // namespace rc
al::SceneObjHolder *DrcAssistDirector::getSceneObjHolder() const { return mSceneObjHolder; }
const char *DrcAssistDirectorList::getSceneObjName() const { return "DRC ASSIST DIRECTOR"; }
