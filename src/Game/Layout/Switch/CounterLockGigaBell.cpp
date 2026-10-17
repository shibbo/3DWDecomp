#include "Layout/CounterLockGigaBell.hpp"
#include <prim/seadSafeString.h>
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Sequence/DemoDirector.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
NERVE_DECL(CounterLockGigaBell, Appear);
NERVE_DECL(CounterLockGigaBell, End);
NERVE_DECL(CounterLockGigaBell, Wait);
NERVES_MAKE_NOSTRUCT(CounterLockGigaBell, Appear, End, Wait)
}
/** @brief Creates a hidden counter that follows a locked Giga Bell. */
CounterLockGigaBell::CounterLockGigaBell(const char* pName, const al::LayoutInitInfo& rInfo,
                                       const sead::Vector3f* pTrans, al::DemoDirector* pDemoDirector)
    : al::LayoutActor(pName), mSceneCameraInfo(rInfo.getSceneCameraInfo()),
      mDemoDirector(pDemoDirector), mTrans(pTrans) {
    al::initLayoutActor(this, rInfo, pName, nullptr);
    initNerve(&NrvCounterLockGigaBellAppear, 0);
    kill();
}
/** @brief Shows the remaining shine count when no demo is active. */
void CounterLockGigaBell::startShow(s32 count) {
    if (isAlive() || isDemoOn()) return;
    al::LayoutActor::appear();
    al::setNerve(this, &NrvCounterLockGigaBellAppear);
    al::setPaneString(this, "TxtCounter", sead::WFormatFixedSafeString<6>(u"%d", count).cstr(), 0, -1);
}
/** @brief Tests whether the optional demo director has an active demo. */
bool CounterLockGigaBell::isDemoOn() {
    if (mDemoDirector && mDemoDirector->isAnyActiveDemo()) return true;
    return false;
}
/** @brief Starts hiding the active counter. */
void CounterLockGigaBell::endShow() {
    if (!isAlive() || al::isNerve(this, &NrvCounterLockGigaBellEnd)) return;
    al::setNerve(this, &NrvCounterLockGigaBellEnd);
}
/** @brief Plays the entrance and follows the bell unless a demo begins. */
void CounterLockGigaBell::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear", nullptr);
    if (isDemoOn()) { kill(); return; }
    sead::Vector3f trans(*mTrans);
    trans += sead::Vector3f(0.0f, 1000.0f, 0.0f);
    rc::updateLayoutPosAtWorldPos(this, this, trans, 100.0f);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterLockGigaBellWait);
}
/** @brief Follows the bell while the counter is visible. */
void CounterLockGigaBell::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    if (isDemoOn()) { kill(); return; }
    sead::Vector3f trans(*mTrans);
    trans += sead::Vector3f(0.0f, 1000.0f, 0.0f);
    rc::updateLayoutPosAtWorldPos(this, this, trans, 100.0f);
}
/** @brief Follows the bell through the exit animation, then hides. */
void CounterLockGigaBell::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    if (isDemoOn()) { kill(); return; }
    sead::Vector3f trans(*mTrans);
    trans += sead::Vector3f(0.0f, 1000.0f, 0.0f);
    rc::updateLayoutPosAtWorldPos(this, this, trans, 100.0f);
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Returns the camera used to project the counter above the bell. */
al::SceneCameraInfo* CounterLockGigaBell::getSceneCameraInfo() const { return mSceneCameraInfo; }
