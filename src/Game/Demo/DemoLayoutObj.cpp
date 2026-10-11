#include "Demo/DemoLayoutObj.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"

namespace {
NERVE_DECL(DemoLayoutObj, Wait);
NERVE_DECL(DemoLayoutObj, End);
NERVES_MAKE_NOSTRUCT(DemoLayoutObj, Wait, End)
}

/** @brief Creates a demo layout with normal playback speed. */
DemoLayoutObj::DemoLayoutObj() : al::LayoutActor("DemoLayoutObj") {}

/**
 * @brief Initializes the layout resource and reads its action and playback rate.
 * @param rInfo Actor placement and layout initialization data.
 * @param pName Layout resource name, or null to use the placement model name.
 */
void DemoLayoutObj::initDemoSceneLayout(const al::ActorInitInfo& rInfo, const char* pName) {
    const char* name = nullptr;
    if (pName)
        name = pName;
    else
        alPlacementFunction::tryGetModelName(&name, rInfo);
    al::initLayoutActor(this, *rInfo.getLayoutInitInfo(), name, nullptr);
    initNerve(&NrvDemoLayoutObjWait, 0);
    mActionName = "Wait";
    al::tryGetArg(&mStepRate, rInfo, "StepRate");
    al::tryGetStringArg(&mActionName, rInfo, "Action1");
    mMaxFrame = al::getActionFrameMax(this, mActionName, nullptr);
}

/** @brief Starts the layout animation and waits for completion. */
void DemoLayoutObj::exeWait() {
    if (al::isFirstStep(this)) {
        al::startAction(this, mActionName, nullptr);
        al::setActionFrameRate(this, mStepRate, nullptr);
    }
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvDemoLayoutObjEnd);
}

/** @brief Keeps the completed layout visible until the demo ends. */
void DemoLayoutObj::exeEnd() {}

/** @brief Shows the layout and restarts its animation state. */
void DemoLayoutObj::startDemo() {
    appear();
    al::setNerve(this, &NrvDemoLayoutObjWait);
}

/** @brief Hides the layout. */
void DemoLayoutObj::endDemo() {
    kill();
}

/**
 * @brief Checks completion with an optional early completion window.
 * @param remainingFrames Number of frames before the end to allow completion.
 * @return Whether playback has ended or entered the requested window.
 */
bool DemoLayoutObj::isEnd(int remainingFrames) const {
    if (al::isFirstStep(this))
        return false;
    if (remainingFrames > 0 && al::isActionPlaying(this, mActionName, nullptr)) {
        if (al::getActionFrame(this, nullptr) + remainingFrames >= mMaxFrame)
            return true;
    }
    return al::isNerve(this, &NrvDemoLayoutObjEnd);
}

/** @brief Returns the animation length. @return Maximum action frame truncated to an integer. */
int DemoLayoutObj::getMaxFrame() const {
    return mMaxFrame;
}

/** @brief Returns playback progress. @return Current frame, or zero when the action is not playing. */
int DemoLayoutObj::getCurrentFrame() const {
    if (al::isActionPlaying(this, mActionName, nullptr))
        return al::getActionFrame(this, nullptr);
    return 0;
}
