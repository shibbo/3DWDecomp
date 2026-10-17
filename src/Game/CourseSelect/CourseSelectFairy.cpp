#include "CourseSelect/CourseSelectFairy.hpp"
#include "Enemy/ActorJointLookController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Joint/JointControllerKeeper.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Math/MatrixUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Layout/SimpleLayoutAppearWaitEnd.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Base/StringUtil.hpp"
#include "math/seadMatrix.hpp"

namespace {
NERVE_DECL(CourseSelectFairy, Wait);
NERVE_DECL(CourseSelectFairy, Happy);
NERVE_DECL(CourseSelectFairy, WaitDemo);
NERVE_DECL(CourseSelectFairy, BackFromDemo);
NERVE_DECL(CourseSelectFairy, BackToCastle);
NERVE_DECL(CourseSelectFairy, BackEnd);
NERVE_DECL(CourseSelectFairy, WorldStartDemo);
NERVES_MAKE_NOSTRUCT(CourseSelectFairy, Wait, Happy, WaitDemo, BackFromDemo, BackToCastle, BackEnd, WorldStartDemo)
}

/** @brief Create a course-select fairy with optional help-balloon behavior. */
CourseSelectFairy::CourseSelectFairy(const char* pName, int fairyId, bool showHelp)
    : al::LiveActor(pName), mShowHelp(showHelp), mFairyId(fairyId) {}
CourseSelectFairy::~CourseSelectFairy() = default;

/** @brief Initialize the fairy model, head tracking, help balloon, and bottle. */
void CourseSelectFairy::init(const al::ActorInitInfo& rInfo) {
    al::StringTmp<32> archive("FairyPrincess%02d", sead::Mathi::max(mFairyId, 1));
    al::initActorWithArchiveName(this, rInfo, archive, "CourseSelect");
    al::initJointControllerKeeper(this, 1);
    auto* pParam = new ActorJointLookControllerParam(1.5f, sead::Vector2f(-60,60), false, al::getJointMtxPtr(this, "AllRoot"), &mLookMtx);
    mpLookController = new ActorJointLookController(this, 1);
    mpLookController->appendJoint("AllRoot", sead::Vector3f::ey, pParam);
    mpHelp = new al::SimpleLayoutAppearWaitEnd("Help吹き出し", "TalkBalloon", al::getLayoutInitInfo(rInfo), nullptr, false);
    if (mShowHelp) {
        al::initNerve(this, &NrvCourseSelectFairyWait, 0);
        mpHelp->appear();
    } else {
        al::initNerve(this, &NrvCourseSelectFairyHappy, 0);
        mpHelp->kill();
    }
    makeActorAppeared();
    mpBottle = new al::LiveActor("妖精ボトル");
    al::initActorWithArchiveName(mpBottle, rInfo, "BottleOuter", nullptr);
    al::resetPosition(mpBottle, al::getTrans(this), false);
    mpBottle->makeActorDead();
}

/** @brief Appear outside the world-clear waiting state and restore the help balloon. */
void CourseSelectFairy::appear() {
    if (!al::isNerve(this, &NrvCourseSelectFairyWaitDemo)) {
        al::LiveActor::appear();
        if (mShowHelp) mpHelp->appear();
    }
}
/** @brief Hide the fairy and its help balloon together. */
void CourseSelectFairy::kill() { al::LiveActor::kill(); if (mShowHelp) mpHelp->kill(); }
void CourseSelectFairy::startClipped() { al::LiveActor::startClipped(); if (mShowHelp) mpHelp->kill(); }
void CourseSelectFairy::endClipped() { al::LiveActor::endClipped(); if (mShowHelp) mpHelp->appear(); }

/** @brief Update the head-tracking base matrix from the animated root joint. */
void CourseSelectFairy::calcAnim() {
    al::LiveActor::calcAnim();
    mLookMtx = *getBaseMtx();
    sead::Vector3f joint(0,0,0);
    al::calcJointPos(&joint, this, "AllRoot");
    mLookMtx.setTranslation(joint);
    if (mShowHelp) al::rotateMtxYDirDegree(&mLookMtx, mLookMtx, 0.0f);
}
void CourseSelectFairy::control() { mpLookController->update(); }
void CourseSelectFairy::startPause() { if (mShowHelp) mpHelp->kill(); }

/** @brief Restore the paused balloon at its projected position above the fairy. */
void CourseSelectFairy::endPause() {
    if (mShowHelp) {
        sead::Vector2f layoutPos(0,0);
        al::calcLayoutPosFromWorldPos(&layoutPos, this, al::getTrans(this) + sead::Vector3f(-30,100,0), 0);
        al::setLocalTrans(mpHelp, layoutPos);
        mpHelp->al::LayoutActor::appear();
        mpHelp->startWait();
    }
}

/** @brief Play the opening-world bottle animation and position its help balloon. */
void CourseSelectFairy::exeWorldStartDemo() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitWorldStart");
        mpBottle->appear();
        al::resetPosition(mpBottle, al::getTrans(this), false);
        al::startAction(mpBottle, "WaitWorldStart");
        al::startAction(al::getSubActor(mpBottle, 0), "WaitWorldStart");
    }
    sead::Vector2f layoutPos(0,0);
    al::calcLayoutPosFromWorldPos(&layoutPos, this, al::getTrans(this) + sead::Vector3f(-50,120,0), 0);
    al::setLocalTrans(mpHelp, layoutPos);
}

/** @brief Wait in the bottle, look toward the nearest player, and position the balloon. */
void CourseSelectFairy::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "Wait");
        mpBottle->appear();
        al::resetPosition(mpBottle, al::getTrans(this), false);
        al::startAction(mpBottle, "Wait");
        al::startAction(al::getSubActor(mpBottle, 0), "Wait");
        if (mShowHelp && !mpHelp->isAlive()) mpHelp->appear();
    }
    mpLookController->setLookAtNearestPlayer(-1.0f);
    sead::Vector2f layoutPos(0,0);
    al::calcLayoutPosFromWorldPos(&layoutPos, this, al::getTrans(this) + sead::Vector3f(-30,100,0), 0);
    al::setLocalTrans(mpHelp, layoutPos);
}

/** @brief Celebrate after returning to the castle and remove the bottle. */
void CourseSelectFairy::exeHappy() {
    if (al::isFirstStep(this)) { al::startAction(this, "WaitHappy"); mpBottle->kill(); }
}
void CourseSelectFairy::exeWaitDemo() { if (al::isFirstStep(this)) mpLookController->clearTarget(); }

/** @brief Play the world-clear animation before beginning the flight home. */
void CourseSelectFairy::exeBackFromDemo() {
    if (al::isFirstStep(this)) {
        al::startAction(this, "WaitWorldClear");
        sead::Vector3f position;
        mDemoMtx.getTranslation(position);
        al::setTrans(this, position);
        mpLookController->clearTarget();
    }
    if (!al::isLessStep(this, 210)) al::setNerve(this, &NrvCourseSelectFairyBackToCastle);
}

/** @brief Interpolate the fairy home while steering its facing and playing flight sounds. */
void CourseSelectFairy::exeBackToCastle() {
    if (al::isFirstStep(this)) al::startSe(this, "FlyStart", nullptr);
    sead::Vector3f start;
    mDemoMtx.getTranslation(start);
    sead::Vector3f position(0,0,0);
    al::lerpVec(&position, start, mCastlePos, al::calcNerveEaseInOutRate(this, 80));
    al::setTrans(this, position);
    if (al::isLessStep(this, 50)) al::turnToTarget(this, mCastlePos, 5.0f);
    else al::turnToDirection(this, sead::Vector3f::ez, 5.0f);
    al::holdSe(this, "FlyWeak", nullptr);
    if (!al::isLessStep(this, 80)) al::setNerve(this, &NrvCourseSelectFairyBackEnd);
}

/** @brief Finish landing before returning to the happy idle state. */
void CourseSelectFairy::exeBackEnd() {
    if (al::isFirstStep(this)) al::startSe(this, "FlyEnd", nullptr);
    if (!al::isLessStep(this, 15)) {
        al::validateClipping(this);
        al::setNerve(this, &NrvCourseSelectFairyHappy);
    }
}

/** @brief Save the castle position and hide the fairy for the world-clear demonstration. */
void CourseSelectFairy::startWorldClearDemo() {
    mpHelp->kill();
    mCastlePos = al::getTrans(this);
    al::setNerve(this, &NrvCourseSelectFairyWaitDemo);
    kill();
}

/** @brief Restore the fairy at the demo's final transform and begin its return sequence. */
void CourseSelectFairy::startBackToCastleDemo(const sead::Matrix34f* pMtx) {
    mDemoMtx = *pMtx;
    al::resetPosition(this, mDemoMtx.getTranslation(), false);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCourseSelectFairyBackFromDemo);
    appear();
}
void CourseSelectFairy::startWorldStartDemo() {
    al::showModelIfHide(this);
    al::setNerve(this, &NrvCourseSelectFairyWorldStartDemo);
}
void CourseSelectFairy::endWorldStartDemo() { al::setNerve(this, &NrvCourseSelectFairyWait); }
/** @brief Immediately restore the fairy at its saved castle position. */
void CourseSelectFairy::forceBackToCastle() {
    al::resetPosition(this, mCastlePos, false);
    al::invalidateClipping(this);
    al::setNerve(this, &NrvCourseSelectFairyHappy);
    appear();
}
