#include "Layout/AmiiboLayout.hpp"
#include "Player/Normal/PlayerActor.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Screen/ScreenFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
namespace {
NERVE_DECL(AmiiboLayout, End);
NERVE_DECL(AmiiboLayout, Appear);
NERVE_DECL(AmiiboLayout, Wait);
NERVES_MAKE_NOSTRUCT(AmiiboLayout, End, Appear, Wait)
}
/** @brief Creates the amiibo scanning layout for a player.
 * @param rInfo Layout initialization context.
 * @param pPlayer Player whose position the layout follows. */
AmiiboLayout::AmiiboLayout(const al::LayoutInitInfo& rInfo, const PlayerActor* pPlayer)
    : al::LayoutActor("AmiiboLayout"), mPlayer(pPlayer) {
    al::initLayoutActor(this, rInfo, "RCS_amiibo", nullptr);
    initNerve(&NrvAmiiboLayoutEnd, 0);
}
/** @brief Reveals the layout and starts its entrance animation. */
void AmiiboLayout::appear() {
    al::LayoutActor::appear();
    al::setNerve(this, &NrvAmiiboLayoutAppear);
}
/** @brief Hides the layout immediately or starts its exit animation.
 * @param isImmediate Whether to skip the exit animation. */
void AmiiboLayout::end(bool isImmediate) {
    if (isImmediate)
        kill();
    else
        al::setNerve(this, &NrvAmiiboLayoutEnd);
}
/** @brief Plays the entrance animation while following the player. */
void AmiiboLayout::exeAppear() {
    if (al::isFirstStep(this))
        al::startAction(this, "Appear", "Main");
    updateTrans();
    if (al::isActionEnd(this, "Main"))
        al::setNerve(this, &NrvAmiiboLayoutWait);
}
/** @brief Positions the root pane one hundred layout units above the player. */
void AmiiboLayout::updateTrans() {
    if (!mPlayer)
        return;
    sead::Vector3f pos = al::getTrans(mPlayer);
    sead::Vector2f layoutPos;
    al::calcLayoutPosFromWorldPos(&layoutPos, mPlayer, pos, 0);
    layoutPos.y += 100.0f;
    al::setPaneLocalTrans(this, "RootPane", layoutPos);
}
/** @brief Keeps the scan animation positioned above the player. */
void AmiiboLayout::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait", "Main");
    updateTrans();
}
/** @brief Plays the exit animation and then hides the layout. */
void AmiiboLayout::exeEnd() {
    if (al::isFirstStep(this))
        al::startAction(this, "End", "Main");
    updateTrans();
    if (al::isActionEnd(this, "Main"))
        kill();
}
