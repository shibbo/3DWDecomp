#include "CourseSelect/CourseSelectBubble.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
NERVE_DECL(CourseSelectBubble, Wait);
NERVE_DECL(CourseSelectBubble, Appear);
NERVE_DECL(CourseSelectBubble, Burst);
NERVE_DECL(CourseSelectBubble, Hide);
NERVES_MAKE_NOSTRUCT(CourseSelectBubble, Wait, Appear, Burst, Hide)
}

/** @brief Create the bubble that carries a course-select player.
 * @param pPlayer Player carried by this bubble.
 */
CourseSelectBubble::CourseSelectBubble(al::LiveActor* pPlayer)
    : al::LiveActor("コース選択泡"), mpPlayer(pPlayer) {}

/** @brief Initialize the bubble model and disable clipping and collision.
 * @param rInfo Actor placement and scene initialization data.
 */
void CourseSelectBubble::init(const al::ActorInitInfo& rInfo) {
    al::initActorWithArchiveName(this, rInfo, "TractorBubble", "CourseSelect");
    al::initNerve(this, &NrvCourseSelectBubbleWait, 0);
    al::invalidateClipping(this);
    al::offCollide(this);
    makeActorDead();
}

/** @brief Activate the bubble and schedule its appearance animation. */
void CourseSelectBubble::startAppear() {
    appear();
    al::hideModelIfShow(this);
    al::setNerve(this, &NrvCourseSelectBubbleAppear);
}

/** @brief Burst the bubble and place its player on the ground below. */
void CourseSelectBubble::startBurst() {
    al::setNerve(this, &NrvCourseSelectBubbleBurst);
    sead::Vector3f hitPos;
    al::Triangle triangle;
    if (!alCollisionUtil::getFirstPolyOnArrow(mpPlayer, &hitPos, &triangle,
            al::getTrans(mpPlayer) + sead::Vector3f::ey * 400.0f * 0.5f,
            sead::Vector3f::ey * -400.0f, nullptr, nullptr))
        hitPos = al::getTrans(mpPlayer);
    rc::setPlayerTrans(mpPlayer, hitPos);
}

/** @brief Follow the player while the bubble appears, then enter idle. */
void CourseSelectBubble::exeAppear() {
    if (al::isFirstStep(this)) {
        al::showModelIfHide(this);
        al::startAction(this, "Appear");
    }
    sead::Vector3f trans = al::getTrans(mpPlayer);
    trans.y += 70.0f;
    al::setTrans(this, trans);
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvCourseSelectBubbleWait);
}

/** @brief Keep the idle bubble centered above its player. */
void CourseSelectBubble::exeWait() {
    if (al::isFirstStep(this))
        al::startAction(this, "Wait");
    sead::Vector3f trans = al::getTrans(mpPlayer);
    trans.y += 70.0f;
    al::setTrans(this, trans);
}

/** @brief Follow the player through the disappearance animation. */
void CourseSelectBubble::exeBurst() {
    if (al::isFirstStep(this))
        al::startAction(this, "Disappear");
    sead::Vector3f trans = al::getTrans(mpPlayer);
    trans.y += 70.0f;
    al::setTrans(this, trans);
    if (al::isActionEnd(this))
        al::setNerve(this, &NrvCourseSelectBubbleHide);
}

/** @brief Deactivate the bubble after it disappears. */
void CourseSelectBubble::exeHide() { kill(); }

/** @brief Destroy the player's course-select bubble. */
CourseSelectBubble::~CourseSelectBubble() = default;
