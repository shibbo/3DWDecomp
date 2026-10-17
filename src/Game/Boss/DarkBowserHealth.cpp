#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Layout/LayoutActorUtil.hpp"
#include "Library/Layout/LayoutInitInfo.hpp"
#include "Layout/HealthBar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Layout/LayoutActionFunction.hpp"
#include "Library/Layout/LayoutInitFunction.hpp"
#include "Library/Layout/LayoutUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Util/LayoutUtil.hpp"
namespace {
using rc::HealthBar;
NERVE_DECL(HealthBar, End);
NERVE_DECL(HealthBar, Appear);
NERVE_DECL(HealthBar, Wait);
NERVE_DECL(HealthBar, Heal);
NERVE_DECL(HealthBar, DamageStart);
NERVE_DECL(HealthBar, Damage);
class HealthBarNrvAppearInstant : public al::Nerve {
public: void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<HealthBar>()->exeAppear(); }
};
NERVES_MAKE_NOSTRUCT(HealthBar, End, AppearInstant, Wait, Appear, Heal, DamageStart, Damage)
}
namespace rc {
/** @brief Creates the boss health display. @param pActor Tracked boss. @param rInfo Layout initialization data. @param pGetRate Health fraction callback. @param pArchive Layout archive. */
HealthBar::HealthBar(const al::LiveActor* pActor, const al::LayoutInitInfo& rInfo, float (*pGetRate)(const al::LiveActor*), const char* pArchive)
    : al::LayoutActor("HealthBar"), mActor(pActor) {
    mGetRate = pGetRate;
    al::initLayoutActor(this, rInfo, pArchive, nullptr);
    initNerve(&NrvHealthBarEnd, 0);
    kill();
}
/** @brief Hides the layout. */
void HealthBar::kill() { al::LayoutActor::kill(); }
/** @brief Starts the appearance animation and initializes health. */
void HealthBar::exeAppear() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvHealthBarAppearInstant)) al::startAction(this, "Wait", nullptr);
        else al::startAction(this, "Appear", nullptr);
        if (mGetRate) {
            mTargetHealth = mGetRate(mActor);
            mHealth = mTargetHealth;
            mOldHealth = mTargetHealth;
        }
    }
    refreshHealthBar();
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvHealthBarWait);
}
/** @brief Updates the red and grey health bars and their position. */
void HealthBar::refreshHealthBar() {
    float red = (1.0f - mHealth) * 100.0f;
    float grey = (1.0f - mOldHealth) * 100.0f;
    if (red < 1.0f && mHealth > 0.0f) red = 1.0f;
    if (grey < 1.0f && mOldHealth > 0.0f) grey = 1.0f;
    al::startFreezeAction(this, "BAR_Grey", grey, "Damage_Grey");
    al::startFreezeAction(this, "BAR_Red", red, "Damage_Red");
    updatePanePos();
}
/** @brief Checks the current health fraction. */
void HealthBar::exeWait() { updateHealthBar(); }
/** @brief Detects healing and damage and refreshes the display. */
void HealthBar::updateHealthBar() {
    if (mGetRate) {
        float rate = mGetRate(mActor);
        if (rate != mTargetHealth && al::isGreaterEqualStep(this, 0)) {
            if (rate > mTargetHealth) {
                mOldHealth = mHealth;
                mTargetHealth = rate;
                al::setNerve(this, &NrvHealthBarHeal);
            } else {
                float old = mHealth;
                mHealth = rate;
                mOldHealth = old;
                mTargetHealth = rate;
                mDamageStep = (rate - old) / 5.0f;
                al::setNerve(this, &NrvHealthBarDamageStart);
            }
        }
    }
    refreshHealthBar();
}
/** @brief Interpolates the healing bar over fifteen frames. */
void HealthBar::exeHeal() {
    mHealth = al::lerpValueNew(mOldHealth, mTargetHealth, al::calcNerveRate(this, 15));
    updateHealthBar();
    if (al::isStep(this, 15)) al::setNerve(this, &NrvHealthBarWait);
}
/** @brief Plays the hit animation before draining the grey bar. */
void HealthBar::exeDamageStart() {
    if (al::isFirstStep(this)) al::startAction(this, "Hit", "Main");
    if (al::isActionEnd(this, nullptr)) al::setNerve(this, &NrvHealthBarDamage);
    updatePanePos();
}
/** @brief Drains the grey bar and optionally ends the display. */
void HealthBar::exeDamage() {
    mOldHealth = mDamageStep + mOldHealth;
    if (al::isStep(this, 5)) {
        mOldHealth = mHealth;
        if (mEndAfterDamage && mHealth == 0.0f) {
            al::setNerve(this, &NrvHealthBarEnd);
            mEndAfterDamage = false;
        } else al::setNerve(this, &NrvHealthBarWait);
    }
    refreshHealthBar();
}
/** @brief Plays the ending animation and hides the layout. */
void HealthBar::exeEnd() {
    if (al::isFirstStep(this)) al::startAction(this, "End", "Main");
    if (al::isActionEnd(this, nullptr)) kill();
}
/** @brief Hides the root pane. */
void HealthBar::hide() { al::hidePane(this, "RootPane"); }
/** @brief Shows the root pane. */
void HealthBar::show() { al::showPane(this, "RootPane"); }
/** @brief Shows the health bar. @param isInstant Whether to skip the appearance animation. */
void HealthBar::startAppear(bool isInstant) {
    appear();
    show();
    mHealth = 0.0f;
    mTargetHealth = 0.0f;
    if (isInstant) al::setNerve(this, &NrvHealthBarAppearInstant);
    else al::setNerve(this, &NrvHealthBarAppear);
}
/** @brief Starts the ending animation. */
void HealthBar::end() { al::setNerve(this, &NrvHealthBarEnd); }
/** @brief Requests an ending after damage empties the bar. */
void HealthBar::setEndAfterDamage() { mEndAfterDamage = true; }
/** @brief Provides the unused finish hook. */
void HealthBar::startFinish() {}
/** @brief Provides the unused hold-time hook. @param time Unused duration. */
void HealthBar::setHoldTime(float time) {}
/** @brief Reports the unused ending query. @return Always false. */
bool HealthBar::isEnd() const { return false; }
/** @brief Positions the layout above the tracked boss. */
void HealthBar::updatePanePos() {
    sead::Vector3f pos = al::getTrans(mActor);
    pos.y = mHeight + pos.y;
    rc::updateLayoutPosAtWorldPos(this, mActor, pos, 100.0f);
}
}
