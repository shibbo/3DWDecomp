#include "Layout/MiiverseBalloon.hpp"
#include "Layout/CourseSelectInfoDecideButton.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Thread/Functor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(MiiverseBalloon, Appear);
NERVE_DECL(MiiverseBalloon, Decide);
NERVE_DECL(MiiverseBalloon, Disappear);
NERVE_DECL(MiiverseBalloon, Decomp);
NERVE_DECL(MiiverseBalloon, Wait);
NERVES_MAKE_NOSTRUCT(MiiverseBalloon, Appear, Decide, Disappear, Decomp, Wait)
}
/** @brief Adapts balloon controls to the world-map layout controller interface. */
class MiiverseBalloonController {
public:
    explicit MiiverseBalloonController(MiiverseBalloon* pBalloon) : mBalloon(pBalloon) {}
    virtual void setTouchSelect(bool isTouch) { mBalloon->setTouchSelect(isTouch); }
    virtual void startAppear(ICourseSelectActorController* pController) { mBalloon->startAppear(pController); }
    virtual void startDisappear() { mBalloon->startDisappear(); }
    virtual void startDecide() { mBalloon->startDecide(); }
    virtual bool isAppear() const { return mBalloon->isAppear(); }
    virtual bool isDisappear() const { return mBalloon->isDisappear(); }
    virtual bool isActive() const { return mBalloon->isAlive(); }
    virtual bool isEnableEnterCourse() const { return !mBalloon->isTouchSelect() && mBalloon->isEnableDecide(); }
    virtual void startPause() {}
    virtual void endPause() {}
private:
    MiiverseBalloon* mBalloon;
};
namespace {
/** @brief Projects the balloon's world-space attachment point into the layout. */
void setBalloonPosition(MiiverseBalloon* pBalloon, const sead::Vector3f& rTrans) {
    sead::Vector2f pos(0.0f, 0.0f);
    al::calcLayoutPosFromWorldPos(&pos, pBalloon, rTrans, 0);
    al::setLocalTrans(pBalloon, pos);
}
void updateBalloonPosition(MiiverseBalloon* pBalloon, const sead::Vector3f& rTrans) {
    sead::Vector2f pos(0.0f, 0.0f);
    sead::Vector3f trans;
    trans.setAdd(rTrans, sead::Vector3f(0.0f, 170.0f, 0.0f));
    al::calcLayoutPosFromWorldPos(&pos, pBalloon, trans, 0);
    al::setLocalTrans(pBalloon, pos);
}
}
/** @brief Creates the balloon and its world-map control helpers. */
MiiverseBalloon::MiiverseBalloon(const al::LayoutInitInfo& rInfo, CourseSelectDirector* pDirector, const al::NetworkSystem* pNetwork)
    : al::LayoutActor("Miiverse吹き出し"), mDirector(pDirector), mSceneCameraInfo(rInfo.getSceneCameraInfo()), mNetwork(pNetwork) {
    al::initLayoutActor(this, rInfo, "BalloonMiiverse", nullptr);
    initNerve(&NrvMiiverseBalloonAppear, 0);
    mController = new MiiverseBalloonController(this);
    mDecideButton = new CourseSelectInfoDecideButton(this);
    mDecompressFunctor = new al::FunctorV0M<MiiverseBalloon*, void (MiiverseBalloon::*)()>(this, &MiiverseBalloon::decompress);
    al::startAction(this, "End", nullptr);
}
/** @brief Marks the balloon content ready in this version. */
void MiiverseBalloon::decompress() { mIsDecompressed = true; }
/** @brief Updates the projected position and start-button availability. */
void MiiverseBalloon::control() {
    setBalloonPosition(this, *mActorTrans + sead::Vector3f(0.0f, 170.0f, 0.0f));
    bool active = !al::isNerve(this, &NrvMiiverseBalloonDecide) && !al::isNerve(this, &NrvMiiverseBalloonDisappear);
    if (al::isNerve(this, &NrvMiiverseBalloonAppear) && al::isLessEqualStep(this, 10)) active = false;
    if (active) {
        if (mIsTouchSelect) mDecideButton->startDeactive(false);
        else mDecideButton->startActive(false);
    }
    mDecideButton->update();
}
/** @brief Records whether selection is driven by touch. */
void MiiverseBalloon::setTouchSelect(bool isTouch) { mIsTouchSelect = isTouch; }
/** @brief Attaches to a map actor and starts preparing the opening animation. */
void MiiverseBalloon::startAppear(ICourseSelectActorController* pController) {
    mActorController = pController;
    mActorTrans = al::getTransPtr(pController->getActor());
    updateBalloonPosition(this, *mActorTrans);
    decompress();
    al::startAction(this, "Appear", nullptr);
    al::setActionFrameRate(this, 0.0f, nullptr);
    al::setNerve(this, &NrvMiiverseBalloonDecomp);
    mDecideButton->setDecideButtonTxt(mDirector->getMainPlayerPortNum());
    appear();
}
/** @brief Direct confirmation is unavailable in this version. */
void MiiverseBalloon::startDecide() {}
/** @brief Starts the closing animation. */
void MiiverseBalloon::startDisappear() {
    al::startAction(this, "End", nullptr);
    al::setNerve(this, &NrvMiiverseBalloonDisappear);
}
/** @brief Reports the fully visible balloon state. */
bool MiiverseBalloon::isAppear() const {
    return !al::isNerve(this, &NrvMiiverseBalloonDecomp) && !al::isNerve(this, &NrvMiiverseBalloonAppear) && !al::isNerve(this, &NrvMiiverseBalloonDisappear);
}
/** @brief Reports the closing state. */
bool MiiverseBalloon::isDisappear() const { return al::isNerve(this, &NrvMiiverseBalloonDisappear); }
/** @brief Reports when the decide button accepts confirmation. */
bool MiiverseBalloon::isEnableDecide() const { return mDecideButton->isEnableDecide(); }
/** @brief Resumes opening once the balloon content is ready. */
void MiiverseBalloon::exeDecomp() {
    if (mIsDecompressed) {
        al::setActionFrameRate(this, 1.0f, nullptr);
        al::setNerve(this, &NrvMiiverseBalloonAppear);
    }
}
/** @brief Waits for the opening animation. */
void MiiverseBalloon::exeAppear() { if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvMiiverseBalloonWait); }
/** @brief Holds the visible balloon. */
void MiiverseBalloon::exeWait() { al::isFirstStep(this); }
/** @brief Closes after the decide animation completes. */
void MiiverseBalloon::exeDecide() {
    if (al::isFirstStep(this)) mDecideButton->startDecide();
    if (mDecideButton->isEnd()) al::setNerve(this, &NrvMiiverseBalloonDisappear);
}
/** @brief Deactivates the start button and hides the closed balloon. */
void MiiverseBalloon::exeDisappear() {
    if (al::isFirstStep(this)) mDecideButton->startDeactive(false);
    if (al::isActionEnd(this, nullptr)) kill();
}
