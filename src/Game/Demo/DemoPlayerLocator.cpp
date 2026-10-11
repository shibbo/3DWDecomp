#include "Demo/DemoPlayerLocator.hpp"
#include "Demo/DemoActionList.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Base/StringUtil.hpp"

namespace DemoSceneActorFunction {
bool isHideAction(const char* pName);
bool isShowAction(const char* pName);
}

namespace {
NERVE_DECL(DemoPlayerLocator, Wait);
NERVES_MAKE_NOSTRUCT(DemoPlayerLocator, Wait)
}

/** @brief Creates a locator with identity placement. @param pName Actor name. */
DemoPlayerLocator::DemoPlayerLocator(const char* pName) : al::LiveActor(pName) {
    mPlacementMtx.makeIdentity();
}

/**
 * @brief Reads locator actions and computes the placement transform.
 * @param rInfo Locator actor initialization data.
 * @param rParentInfo Parent demo initialization data.
 * @param pParentMtx Explicit parent transform, or null to use parent placement data.
 */
void DemoPlayerLocator::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rParentInfo, const sead::Matrix34f* pParentMtx) {
    mActions = new DemoActionList(rInfo, "LocatorAction");
    if (pParentMtx) {
        sead::Matrix34f local;
        local.makeIdentity();
        al::tryGetMatrixTR(&local, rInfo);
        mPlacementMtx.setMul(*pParentMtx, local);
    } else {
        al::calcMatrixMultParent(&mPlacementMtx, rInfo, rParentInfo);
    }
    init(rInfo);
}

/** @brief Loads the locator model and initializes its pose. @param rInfo Actor initialization data. */
void DemoPlayerLocator::init(const al::ActorInitInfo& rInfo) {
    const char* suffix = nullptr;
    al::tryGetStringArg(&suffix, rInfo, "SuffixName");
    al::initActorWithArchiveName(this, rInfo, "DemoPlayerLocator", suffix);
    al::updatePoseMtx(this, &mPlacementMtx);
    al::invalidateClipping(this);
    al::initNerve(this, &NrvDemoPlayerLocatorWait, 0);
    makeActorDead();
}

/**
 * @brief Places and activates the locator for a demo.
 * @param rBaseMtx Demo scene transform.
 * @param playerCount Player count used to select an action variant.
 */
void DemoPlayerLocator::startDemo(const sead::Matrix34f& rBaseMtx, int playerCount) {
    mPlayerCount = playerCount;
    sead::Matrix34f mtx;
    mtx.setMul(rBaseMtx, mPlacementMtx);
    al::updatePoseMtx(this, &mtx);
    makeActorAppeared();
}

/** @brief Runs an action command, preferring the player-count variant. @param index Action list index. */
void DemoPlayerLocator::startAction(int index) {
    const char* action = mActions->getActionName(index);
    if (DemoSceneActorFunction::isHideAction(action)) {
        if (!al::isHideModel(this))
            al::hideModel(this);
    } else if (DemoSceneActorFunction::isShowAction(action)) {
        if (al::isHideModel(this))
            al::showModel(this);
    } else if (action) {
        if (al::isHideModel(this))
            al::showModel(this);
        al::StringTmp<64> variant("%s%d", action, mPlayerCount);
        if (al::isExistAction(this, variant.cstr()))
            al::startAction(this, variant.cstr());
        else
            al::startAction(this, action);
    }
}

/** @brief Finds a player's locator transform. @param index Zero-based player index. @return Joint matrix pointer. */
const sead::Matrix34f* DemoPlayerLocator::getPlayerLocatorMtxPtr(int index) const {
    al::StringTmp<32> joint("Player%d", index + 1);
    return al::getJointMtxPtr(this, joint.cstr());
}

/** @brief Hides the locator when its demo ends. */
void DemoPlayerLocator::endDemo() {
    makeActorDead();
}

/** @brief Waits for externally issued demo actions. */
void DemoPlayerLocator::exeWait() {}
