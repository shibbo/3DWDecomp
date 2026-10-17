#include "CourseSelect/MiniatureController.hpp"
#include "CourseSelect/CourseSelectLock.hpp"
#include "CourseSelect/CourseSelectMiniature.hpp"
#include "CourseSelect/CourseSelectDirector.hpp"
#include "CourseSelect/CourseSelectPuppeteerGroup.hpp"
#include "CourseSelect/CourseSelectSensor.hpp"
#include "CourseSelect/ICourseSelectActorController.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "System/Data/StageDatabaseInfo.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
NERVE_DECL(CourseSelectLock, End);
NERVE_DECL(CourseSelectLock, Disappear);
NERVE_DECL(CourseSelectLock, Wait);
class CourseSelectLockNrvDisable : public al::Nerve {
public:
    /** @brief Remain disabled. @param pKeeper Unused keeper. */
    void execute(al::NerveKeeper* pKeeper) const override {}
};
class CourseSelectLockNrvHide : public al::Nerve {
public:
    /** @brief Remain hidden. @param pKeeper Unused keeper. */
    void execute(al::NerveKeeper* pKeeper) const override {}
};
NERVE_DECL(CourseSelectLock, Appear);
NERVE_DECL(CourseSelectLock, Unlock);
NERVES_MAKE_STRUCT(CourseSelectLock, Wait, Disable, Hide, Appear, Unlock)
NERVES_MAKE_NOSTRUCT(CourseSelectLock, End, Disappear)
}
class CourseSelectLockController : public ICourseSelectActorController {
public:
    /** @brief Wrap a course lock. @param pActor Lock actor. @param pInfo Course metadata. */
    CourseSelectLockController(CourseSelectLock* pActor, const CourseSelectActorInfo* pInfo) : mpActor(pActor), mpInfo(pInfo) {}
    /** @brief Get the lock. @return Controlled actor. */
    al::LiveActor* getActor() override { return mpActor; }
    /** @brief Get the lock cursor. @return Layout type 4. */
    int getCursorLayoutType() const override { return 4; }
    /** @brief Get course metadata. @return Miniature's metadata. */
    const CourseSelectActorInfo* getCourseSelectActorInfo() const override { return mpInfo; }
    /** @brief Get a course node. @return Null for locks. */
    CourseSelectNode* getCourseSelectNode() const override { return nullptr; }
    /** @brief No puppet setup is needed. @param pGroup Unused group. */
    void startPuppetDemo(CourseSelectPuppeteerGroup* pGroup) override {}
    /** @brief No puppet cleanup is needed. */
    void endPuppetDemo() override {}
    /** @brief No binding is needed. @param pPuppeteer Unused puppeteer. */
    void startBind(CourseSelectPuppeteer* pPuppeteer) override {}
    /** @brief Try unlocking. @param pDirector Selection director. @return Whether the selection was accepted. */
    bool tryDecide(const CourseSelectDirector* pDirector) override { return mpActor->tryDecide(pDirector); }
    /** @brief Locks have no road-opening sequence. */
    void startOpen() override {}
    /** @brief Locks have no immediate road-opening sequence. */
    void startOpenImmediately() override {}
    /** @brief Check opening completion. @return False. */
    bool isEndOpen() const override { return false; }
private:
    CourseSelectLock* mpActor;
    const CourseSelectActorInfo* mpInfo;
};
/** @brief Create a course lock. @param pName Actor name. @param pMiniature Course miniature. @param requiredStars Required Green Stars. */
CourseSelectLock::CourseSelectLock(const char* pName, CourseSelectMiniature* pMiniature, int requiredStars)
    : al::LiveActor(pName), mRequiredStars(requiredStars), mpMiniature(pMiniature) {}
/** @brief Initialize the lock's model, controller, and selection sensors. @param rInfo Actor initialization data. */
void CourseSelectLock::init(const al::ActorInitInfo& rInfo) {
    const char* pArchive;
    const char* pSensorName;
    const al::Nerve* pNerve;
    if (mpMiniature->getStageInfo()->isKoopaCastle()) {
        if (mpMiniature->getWorldId() == 7) pArchive = "MiniatureLockCastleW7";
        else pArchive = mpMiniature->getWorldId() == 8 ? "MiniatureLockCastleW8" : "MiniatureLockCastle";
        pSensorName = "Region";
        pNerve = mpMiniature->isGreenStarLock() ? &NrvCourseSelectLock.Wait : static_cast<const al::Nerve*>(&NrvCourseSelectLock.Disable);
    } else {
        pArchive = "MiniatureLock";
        pSensorName = "Body";
        pNerve = mpMiniature->isGreenStarLock() ? &NrvCourseSelectLock.Wait : static_cast<const al::Nerve*>(&NrvCourseSelectLock.Hide);
    }
    al::initActorWithArchiveName(this, rInfo, pArchive, nullptr);
    al::initNerve(this, pNerve, 0);
    mpController = new CourseSelectLockController(this, mpMiniature->getCourseSelectActorInfo());
    mpSensor = new CourseSelectSensor(static_cast<CourseSelectSensorType>(1), mpController);
    al::HitSensor* pSensor = al::getHitSensor(this, pSensorName);
    mpTouchSensor = new CourseSelectSensor(static_cast<CourseSelectSensorType>(0), mpController);
    mpTouchSensor->setSensor(pSensor);
    makeActorDead();
}
/** @brief Activate the lock. */
void CourseSelectLock::appear() { al::LiveActor::appear(); }
/** @brief Deactivate the lock. */
void CourseSelectLock::kill() { al::LiveActor::kill(); }
/** @brief Check touch selection for an active lock in the current world. */
void CourseSelectLock::control() {
    CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(this);
    if (mpTouchSensor && al::isNerve(this, &NrvCourseSelectLock.Wait) && mpMiniature->getWorldId() == pDirector->getCurrentWorldId())
        pDirector->checkDrcTouch(mpTouchSensor);
}
/** @brief Push players and forward selection contacts. @param pSender Lock sensor. @param pReceiver Other sensor. */
void CourseSelectLock::attackSensor(al::HitSensor* pSender, al::HitSensor* pReceiver) {
    if (al::isSensorPlayer(pReceiver) && al::isSensorMapObj(pSender)) {
        al::sendMsgPushAndKillVelocityToTarget(this, pSender, pReceiver);
        return;
    }
    if (al::isSensorPlayer(pReceiver) && al::isSensorEye(pSender) && al::isNerve(this, &NrvCourseSelectLock.Wait)) {
        CourseSelectDirector* pDirector = CourseSelectDirector::getCourseSelectDirector(this);
        mpSensor->setSensor(pSender);
        pDirector->touchPlayer(mpSensor, al::getSensorHost(pReceiver));
    }
}
/** @brief Animate an inactive lock into view. */
void CourseSelectLock::startAppear() {
    if (al::isDead(this)) {
        al::LiveActor::appear();
        al::setNerve(this, &NrvCourseSelectLock.Appear);
    }
}
/** @brief Start the unlock animation and puppeteer demo. */
void CourseSelectLock::startUnlock() {
    if (al::isNerve(this, &NrvCourseSelectLock.Wait)) {
        al::setNerve(this, &NrvCourseSelectLock.Unlock);
        al::startSe(this, "PgDecided", nullptr);
        CourseSelectMiniature* pMiniature = mpMiniature;
        CourseSelectDirector::getCourseSelectDirector(this)->getPuppeteerGroup()->startUnLockDemo(pMiniature->getController());
    }
}
/** @brief Show the lock in its selectable state. */
void CourseSelectLock::startShow() {
    appear();
    if (al::isNerve(this, &NrvCourseSelectLock.Wait) || al::isNerve(this, &NrvCourseSelectLock.Appear)) return;
    al::setNerve(this, &NrvCourseSelectLock.Wait);
}
/** @brief Show the lock with selection disabled. */
void CourseSelectLock::startDisable() {
    appear();
    if (!al::isNerve(this, &NrvCourseSelectLock.Disable)) al::setNerve(this, &NrvCourseSelectLock.Disable);
}
/** @brief Accept selection when enough stars have been collected. @param pDirector Input director. @return Whether selection succeeded. */
bool CourseSelectLock::tryDecide(const CourseSelectDirector* pDirector) {
    if (!pDirector->isEnableEnterSelectedSensor() || !pDirector->isTriggerDecideMainPlayer()
        || mRequiredStars > GameDataFunction::calcTotalAcquireGreenStarNum(GameDataHolderAccessor(this))) return false;
    startUnlock();
    return true;
}
/** @brief Check whether unlocking has finished. @return Whether the end state is active. */
bool CourseSelectLock::isUnlock() const { return al::isNerve(this, &NrvCourseSelectLockEnd); }
/** @brief Play appearance and enter the selectable state. */
void CourseSelectLock::exeAppear() {
    if (al::isFirstStep(this)) al::startAction(this, "Appear");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvCourseSelectLock.Wait);
}
/** @brief Wait for selection. */
void CourseSelectLock::exeWait() {}
/** @brief Wait three seconds before disappearing. */
void CourseSelectLock::exeUnlock() {
    if (al::isGreaterEqualStep(this, 180)) al::setNerve(this, &NrvCourseSelectLockDisappear);
}
/** @brief Finish the disappearance action. */
void CourseSelectLock::exeDisappear() {
    if (al::isFirstStep(this)) al::startAction(this, "Disappear");
    if (al::isActionEnd(this)) al::setNerve(this, &NrvCourseSelectLockEnd);
}
/** @brief Deactivate the unlocked lock. */
void CourseSelectLock::exeEnd() { kill(); }
/** @brief Destroy the course lock. */
CourseSelectLock::~CourseSelectLock() = default;
