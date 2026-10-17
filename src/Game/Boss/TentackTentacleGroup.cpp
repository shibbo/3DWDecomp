#include "Boss/TentackTentacleGroup.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveKeeper.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include <random/seadGlobalRandom.h>
namespace {
NERVE_DECL(TentackTentacleGroup, Deactive);
NERVE_DECL(TentackTentacleGroup, Wait);
NERVE_DECL(TentackTentacleGroup, EndRequest);
NERVE_DECL(TentackTentacleGroup, End);
NERVES_MAKE_NOSTRUCT(TentackTentacleGroup, Deactive, End, Wait, EndRequest)
}
/** @brief Allocates the tentacle list and its state keeper. @param tentacleNumMax Maximum tentacle count. */
TentackTentacleGroup::TentackTentacleGroup(s32 tentacleNumMax)
    : mNerveKeeper(new al::NerveKeeper(this, &NrvTentackTentacleGroupDeactive, 0)), mTargetPos(0.0f, 0.0f, 0.0f) {
    allocBuffer(tentacleNumMax, nullptr);
}
/** @brief Updates the group state. */
void TentackTentacleGroup::update() { mNerveKeeper->update(); }
/** @brief Adds a tentacle to the group. @param pTentacle Tentacle to register. */
void TentackTentacleGroup::registerTentacle(TentackTentacle* pTentacle) { pushBack(pTentacle); }
/** @brief Staggers tentacle appearances and averages the swing target. */
void TentackTentacleGroup::appearAll() {
    shuffle(sead::GlobalRandom::instance());
    mTargetPos.set(0.0f, 0.0f, 0.0f);
    int count = 0;
    for (int i = 0; i < size(); i++) {
        at(i)->appearDelay(i * 15);
        if (!unsafeAt(i)->mCurInfo.mIsAttack) {
            mTargetPos += al::getTrans(at(i));
            count++;
        }
    }
    if (count > 0) mTargetPos *= 1.0f / count;
    al::setNerve(this, &NrvTentackTentacleGroupWait);
}
/** @brief Requests the end of a swing. @return Whether the request has already completed. */
bool TentackTentacleGroup::requestEndSwingAll() {
    if (al::isNerve(this, &NrvTentackTentacleGroupEndRequest)) return false;
    if (al::isNerve(this, &NrvTentackTentacleGroupWait)) {
        al::setNerve(this, &NrvTentackTentacleGroupEndRequest);
        return false;
    }
    return true;
}
/** @brief Ends non-attack tentacle swings and waits for their retreat. */
void TentackTentacleGroup::endSwingAllForce() {
    for (int i = 0; i < size(); i++)
        if (!unsafeAt(i)->mCurInfo.mIsAttack) at(i)->endSwing();
    al::setNerve(this, &NrvTentackTentacleGroupEnd);
}
/** @brief Consumes items attached to non-attack tentacles. */
void TentackTentacleGroup::eatAttachItemAll() {
    for (int i = 0; i < size(); i++)
        if (!unsafeAt(i)->mCurInfo.mIsAttack) at(i)->eatAttachItemIfAttached(false);
}
/** @brief Clears the group and returns to its inactive state. */
void TentackTentacleGroup::reset() {
    clear();
    al::setNerve(this, &NrvTentackTentacleGroupDeactive);
}
/** @brief Removes attack tentacles from the group. */
void TentackTentacleGroup::resetAttackTentacle() {
    for (int i = 0; i < size(); i++) {
        if (unsafeAt(i)->mCurInfo.mIsAttack) { erase(i); i--; }
    }
}
/** @brief Checks the inactive state. @return Whether inactive. */
bool TentackTentacleGroup::isDeactive() const { return al::isNerve(this, &NrvTentackTentacleGroupDeactive); }
/** @brief Checks whether the non-attack tentacles can retreat. @return Whether all can retreat. */
bool TentackTentacleGroup::isEnableEndSwing() const {
    for (int i = 0; i < size(); i++)
        if (!unsafeAt(i)->mCurInfo.mIsAttack && !at(i)->isEnableEndSwing()) return false;
    return true;
}
/** @brief Waits while the group is inactive. */
void TentackTentacleGroup::exeDeactive() {}
/** @brief Waits for an end request. */
void TentackTentacleGroup::exeWait() {}
/** @brief Waits until the group can end its swing. */
void TentackTentacleGroup::exeEndRequest() {
    if (!isEnableEndSwing()) return;
    endSwingAllForce();
}
/** @brief Waits for all tentacles to disappear. */
void TentackTentacleGroup::exeEnd() {
    bool allDead = true;
    for (int i = 0; i < size(); i++) allDead &= !al::isAlive(at(i));
    if (allDead) al::setNerve(this, &NrvTentackTentacleGroupDeactive);
}
/** @brief Gets the first tentacle head. @return Head, or null for an empty group. */
TentackHead* TentackTentacleGroup::getHead() const {
    return size() > 0 ? unsafeAt(0)->getHead() : nullptr;
}
/** @brief Gets the group state keeper. @return Nerve keeper. */
al::NerveKeeper* TentackTentacleGroup::getNerveKeeper() const { return mNerveKeeper; }
