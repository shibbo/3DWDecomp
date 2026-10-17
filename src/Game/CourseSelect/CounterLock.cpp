#include "CourseSelect/CounterLock.hpp"
#include "CourseSelect/CourseSelectActorInfo.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Layout/CourseSelectInfoDecideButton.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/LayoutUtil.hpp"
#include "prim/seadSafeString.hpp"

namespace {
NERVE_DECL(CounterLock, Appear);
NERVE_DECL(CounterLock, Wait);
NERVE_DECL(CounterLock, Reaction);
NERVE_DECL(CounterLock, FrameOut);
NERVE_DECL(CounterLock, End);
NERVE_DECL(CounterLock, Hide);
NERVES_MAKE_NOSTRUCT(CounterLock, Appear, Wait, Reaction, FrameOut, End, Hide)

/** @brief Project the lock counter above its associated world-space actor. */
inline void updateCounterPos(al::LayoutActor* pActor, const sead::Vector3f* pPosition) {
    sead::Vector3f position(*pPosition);
    position.y += 200.0f;
    rc::updateLayoutPosAtWorldPos(pActor, pActor, position, 1000.0f);
}
}

/** @brief Forward course-selection layout requests to the Green Star counter. */
class CounterLockController {
public:
    explicit CounterLockController(CounterLock* pActor) : mpActor(pActor) {}
    virtual void setTouchSelect(bool isTouch) { mpActor->setTouchSelect(isTouch); }
    virtual void startAppear(ICourseSelectActorController* pController) { mpActor->startShow(pController); }
    virtual void startDisappear() { mpActor->endShow(); }
    virtual void startDecide() { mpActor->startDecide(); }
    virtual bool isAppear() const { return mpActor->isAppear(); }
    virtual bool isDisappear() const { return mpActor->isDisappear(); }
    virtual bool isActive() const { return mpActor->isAlive(); }
    virtual bool isEnableEnterCourse() const { return mpActor->isEnableEnter(); }
    virtual void startPause() { mpActor->kill(); }
    virtual void endPause() { mpActor->appear(); }
private:
    CounterLock* mpActor;
};

/** @brief Initialize the Green Star counter and its confirmation prompt. */
CounterLock::CounterLock(const al::LayoutInitInfo& rInfo, const CourseSelectDirector* pDirector)
    : al::LayoutActor("コース選択グリーンスターロック"), mpDirector(pDirector), mpSceneCameraInfo(rInfo.getSceneCameraInfo()) {
    al::initLayoutActor(this, rInfo, "CounterLock", nullptr);
    initNerve(&NrvCounterLockEnd, 0);
    kill();
    mpController = new CounterLockController(this);
    mpDecideButton = new CourseSelectInfoDecideButton(this);
}
void CounterLock::control() {}
void CounterLock::setTouchSelect(bool isTouch) { mTouchSelect = isTouch; }

/** @brief Show the requirement for the selected actor and follow its world position. */
void CounterLock::startShow(ICourseSelectActorController* pController) {
    const CourseSelectActorInfo* pInfo = pController->getCourseSelectActorInfo();
    al::LiveActor* pActor = pController->getActor();
    mRequiredStars = GameDataFunction::findCourseLockGreenStarNum(GameDataHolderAccessor(pActor), pInfo->getCourseId());
    mpWorldPos = al::getTransPtr(pActor);
    al::startFreezeAction(this, "Appear", 0.0f, nullptr);
    updateCounterPos(this, mpWorldPos);
    appear();
    al::setNerve(this, &NrvCounterLockAppear);
}
void CounterLock::endShow() { al::setNerve(this, &NrvCounterLockEnd); }
void CounterLock::startDecide() { al::setNerve(this, &NrvCounterLockEnd); }
bool CounterLock::isAppear() const {
    return !al::isNerve(this, &NrvCounterLockAppear) && !al::isNerve(this, &NrvCounterLockEnd) && !al::isNerve(this, &NrvCounterLockHide);
}
bool CounterLock::isDisappear() const { return al::isNerve(this, &NrvCounterLockHide) || !isAlive(); }
bool CounterLock::isEnableEnter() const { return mpDecideButton->isEnableDecide(); }

/** @brief Format the star requirement and enable confirmation when enough stars are owned. */
void CounterLock::exeAppear() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Appear", nullptr);
        sead::WFixedSafeString<8> number;
        number.append(sead::WFormatFixedSafeString<4>(u"%d", mRequiredStars % 1000));
        al::setPaneString(this, "TxtCounter", number.cstr(), 0, -1);
        mpDecideButton->setDecideButtonTxt(CourseSelectDirector::getCourseSelectDirector(this)->getMainPlayerPortNum());
        if (mTouchSelect) {
            mpDecideButton->startDeactive(true);
        } else {
            if (mRequiredStars <= GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(this)))
                mpDecideButton->startActive(true);
            else mpDecideButton->startDeactive(true);
        }
    }
    updateCounterPos(this, mpWorldPos);
    mpDecideButton->update();
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterLockWait);
}

/** @brief Track the selected lock and reject confirmation while stars are missing. */
void CounterLock::exeWait() {
    if (al::isFirstStep(this)) al::startAction(this, "Wait", nullptr);
    updateCounterPos(this, mpWorldPos);
    mpDecideButton->update();
    al::LiveActor* pPlayer = mpDirector->getMainPlayerActor();
    sead::Vector3f front(0,0,0);
    al::calcFrontDir(&front, pPlayer);
    sead::Vector3f direction = *mpWorldPos - al::getTrans(pPlayer);
    al::verticalizeVec(&direction, sead::Vector3f::ey, direction);
    al::normalizeOrZero(&direction);
    if (direction.dot(front) < 0.0f) return;
    if (mpDirector->isTriggerDecideMainPlayer() && mRequiredStars > GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(this)))
        al::setNerve(this, &NrvCounterLockReaction);
}

/** @brief Play the insufficient-stars reaction, then return to waiting. */
void CounterLock::exeReaction() {
    if (al::isFirstStep(this)) al::startAction(this, "No", nullptr);
    updateCounterPos(this, mpWorldPos);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterLockWait);
}
/** @brief Animate the counter out when its actor leaves the frame. */
void CounterLock::exeFrameOut() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    updateCounterPos(this, mpWorldPos);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterLockHide);
}
/** @brief Animate the counter out after selection ends. */
void CounterLock::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", nullptr);
    updateCounterPos(this, mpWorldPos);
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvCounterLockHide);
}
void CounterLock::exeHide() { kill(); }
al::SceneCameraInfo* CounterLock::getSceneCameraInfo() const { return mpSceneCameraInfo; }
