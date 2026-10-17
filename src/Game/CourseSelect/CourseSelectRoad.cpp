#include "CourseSelect/CourseSelectRoad.hpp"
#include "CourseSelect/CourseSelectNode.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/Effect/Core/EffectUtil.hpp"

namespace {
float sRoadOpenSpeed = 1.0f;
NERVE_DECL(CourseSelectRoad, Wait);
NERVE_DECL(CourseSelectRoad, Stop);
NERVE_DECL(CourseSelectRoad, Open);
NERVES_MAKE_NOSTRUCT(CourseSelectRoad, Wait, Stop, Open)
}
/** @brief Create a road from a node to an endpoint.
 * @param pName Actor name. @param pNode Starting node. @param rEndPos Road endpoint.
 */
CourseSelectRoad::CourseSelectRoad(const char* pName, const CourseSelectNode* pNode, const sead::Vector3f& rEndPos)
    : al::LiveActor(pName), mpNode(pNode), mEndPos(rEndPos) {}
/** @brief Initialize the road's direction, length, and growth effect.
 * @param rInfo Actor initialization data. @param pOffset Optional offset from the node.
 */
void CourseSelectRoad::initRoad(const al::ActorInitInfo& rInfo, const sead::Vector3f* pOffset) {
    al::initActorWithArchiveName(this, rInfo, "CourseSelectRoadStraight", nullptr);
    al::initNerve(this, &NrvCourseSelectRoadWait, 0);
    al::setTrans(this, al::getTrans(mpNode));
    if (pOffset) *al::getTransPtr(this) += *pOffset;
    al::setScaleZ(this, 0.15f);
    if (al::getTrans(this).x == mEndPos.x && al::getTrans(this).z == mEndPos.z) {
        al::setNerve(this, &NrvCourseSelectRoadStop);
    } else if (al::getTrans(this).x == mEndPos.x || al::getTrans(this).z == mEndPos.z) {
        const sead::Vector3f& rTrans = al::getTrans(this);
        sead::Vector3f direction(mEndPos.x - rTrans.x, 0.0f, mEndPos.z - rTrans.z);
        mDistance = direction.length();
        al::normalize(&direction);
        al::roundOffVec(&direction);
        al::makeQuatFrontUp(al::getQuatPtr(this), direction, sead::Vector3f::ey);
        mEffectPos.set(al::getTrans(this));
        al::setEffectFollowPosPtr(this, "Glow", &mEffectPos);
    }
    makeActorDead();
}
/** @brief Set the global road opening rate. @param isSuperSpeed Whether accelerated opening is enabled. */
void CourseSelectRoad::setRoadOpenSuperSpeed(bool isSuperSpeed) { sRoadOpenSpeed = isSuperSpeed ? 1.334f : 1.0f; }
/** @brief Deactivate the road actor. */
void CourseSelectRoad::kill() { al::LiveActor::kill(); }
/** @brief Wait for an opening request. */
void CourseSelectRoad::exeWait() {}
/** @brief Extend the road toward its endpoint with sound and a moving effect. */
void CourseSelectRoad::exeOpen() {
    if (al::isFirstStep(this)) {
        al::startSe(this, "Extend", nullptr);
        al::emitEffect(this, "Glow", nullptr);
    }
    if (al::isStep(this, 1)) al::showModelIfHide(this);
    al::setScaleZ(this, al::getScaleZ(this) + sRoadOpenSpeed * 0.15f);
    al::calcTransLocalOffset(&mEffectPos, this, sead::Vector3f(0.0f, 0.0f, al::getScaleZ(this) * 100.0f));
    if (mDistance <= al::getScaleZ(this) * 100.0f) {
        al::setNerve(this, &NrvCourseSelectRoadStop);
        al::setScaleZ(this, mDistance / 100.0f);
        al::stopSeByName(this, "Extend");
    }
}
/** @brief Finish effects and configure clipping around the completed road. */
void CourseSelectRoad::exeStop() {
    if (al::isFirstStep(this)) {
        al::tryDeleteEffect(this, "Glow");
        if (mDistance < 0.1f) return;
        sead::Vector3f direction = mEndPos - al::getTrans(this);
        al::normalizeOrZero(&direction);
        mClippingCenter.set(al::getTrans(this) + direction * (mDistance * 0.5f));
        al::setClippingInfo(this, (mDistance + 100.0f) * 0.5f, &mClippingCenter);
        al::validateClipping(this);
    }
    if (al::isStep(this, 3)) al::setNeedSetBaseMtxAndCalcAnimFlag(this, false);
}
/** @brief Open the road, optionally skipping its growth animation. @param isImmediately Whether to open instantly. */
void CourseSelectRoad::startOpen(bool isImmediately) {
    if (al::isNearZero(mDistance, 0.001f)) {
        al::setNerve(this, &NrvCourseSelectRoadStop);
        return;
    }
    al::invalidateClipping(this);
    bool isStopped = al::isNerve(this, &NrvCourseSelectRoadStop);
    if (isImmediately) {
        if (isStopped) return;
        appear();
        al::setScaleZ(this, mDistance / 100.0f);
        al::setNerve(this, &NrvCourseSelectRoadStop);
    } else {
        if (isStopped) return;
        appear();
        if (mpNode->isCurve()) al::hideModelIfShow(this);
        al::setNerve(this, &NrvCourseSelectRoadOpen);
    }
}
/** @brief Finish an opening road immediately and stop its sound. */
void CourseSelectRoad::endEarly() {
    if (al::isNerve(this, &NrvCourseSelectRoadStop)) return;
    al::stopSeByName(this, "Extend");
    al::setScaleZ(this, mDistance / 100.0f);
    al::setNerve(this, &NrvCourseSelectRoadStop);
}
/** @brief Check whether opening is sufficiently complete. @return Whether the road is near its endpoint or stopped. */
bool CourseSelectRoad::isEndOpen() const {
    if (mDistance <= (al::getScaleZ(this) + 0.3f) * 100.0f) return true;
    return al::isNerve(this, &NrvCourseSelectRoadStop);
}
/** @brief Check the unopened state. @return Whether the road is waiting to open. */
bool CourseSelectRoad::isWait() const { return al::isNerve(this, &NrvCourseSelectRoadWait); }
/** @brief Shorten the road without making its length negative. @param distance Length to remove. */
void CourseSelectRoad::subDistance(float distance) { mDistance = sead::Mathf::max(0.0f, mDistance - distance); }
/** @brief Destroy the road actor. */
CourseSelectRoad::~CourseSelectRoad() = default;
