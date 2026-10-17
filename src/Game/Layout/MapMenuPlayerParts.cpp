#include "Layout/MapMenuPlayerParts.hpp"

#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"

namespace {
NERVE_DECL(MapMenuPlayerParts, Hide);
NERVE_DECL(MapMenuPlayerParts, Wait);
NERVE_DECL(MapMenuPlayerParts, Appear);
NERVE_DECL(MapMenuPlayerParts, End);
NERVES_MAKE_NOSTRUCT(MapMenuPlayerParts, Hide, Wait, Appear, End)

void updatePosition(al::LayoutActor* pActor, const al::IUseLayout* pTarget) {
    sead::Vector3f position(0.0f, 0.0f, 0.0f);
    al::calcPaneTrans(&position, pTarget, "IconPosition");
    al::setLocalTrans(pActor, position);
}
}

/** @brief Creates hidden player-icon parts in the parent layout.
 * @param rInfo Layout initialization context.
 * @param pName Actor name.
 * @param pPartsName Parts pane name.
 * @param pParent Parent layout. */
MapMenuPlayerParts::MapMenuPlayerParts(const al::LayoutInitInfo& rInfo, const char* pName,
                                     const char* pPartsName, al::LayoutActor* pParent)
    : al::LayoutActor(pName) {
    al::initLayoutPartsActor(this, pParent, rInfo, pPartsName, nullptr);
    initNerve(&NrvMapMenuPlayerPartsHide, 0);
}

/** @brief Plays the appearance animation while following the target pane. */
void MapMenuPlayerParts::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", "Main");
    updatePosition(this, mTarget);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvMapMenuPlayerPartsWait);
}

/** @brief Keeps the idle icon at the target pane. */
void MapMenuPlayerParts::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait", "Main");
    updatePosition(this, mTarget);
    sead::Vector3f position(0.0f, 0.0f, 0.0f);
    al::calcPaneTrans(&position, mTarget, "IconPosition");
    al::setLocalTrans(this, position);
}

/** @brief Plays the end animation while following the target pane. */
void MapMenuPlayerParts::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", "Main");
    updatePosition(this, mTarget);
    if (al::isActionEnd(this, nullptr))
        al::setNerve(this, &NrvMapMenuPlayerPartsHide);
}

/** @brief Hides the icon through its main animation slot. */
void MapMenuPlayerParts::exeHide() {
    if (al::isFirstStep(this))
        al::startAction(this, "Hide", "Main");
}

/** @brief Chooses a character icon and attaches it to a map-menu target.
 * @param pCharacterName Character animation name.
 * @param pTarget Layout containing the target icon-position pane. */
void MapMenuPlayerParts::startAppear(const char* pCharacterName, const al::IUseLayout* pTarget) {
    mTarget = pTarget;
    mCharacterName = pCharacterName;
    al::startAction(this, pCharacterName, "Icon");
    al::startAction(this, "LoopIcon", "LoopIcon");
    al::startAction(this, "Hide", "Main");
    al::setNerve(this, &NrvMapMenuPlayerPartsAppear);
    updatePosition(this, mTarget);
}

/** @brief Requests the end animation. */
void MapMenuPlayerParts::startEnd() { al::setNerve(this, &NrvMapMenuPlayerPartsEnd); }
/** @brief Immediately selects the hidden state. */
void MapMenuPlayerParts::hide() { al::setNerve(this, &NrvMapMenuPlayerPartsHide); }
/** @brief Immediately selects the idle state. */
void MapMenuPlayerParts::show() { al::setNerve(this, &NrvMapMenuPlayerPartsWait); }
