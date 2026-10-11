#include "MapObj/KouraGold.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Thread/Functor.hpp"
namespace {
    NERVE_DECL(KouraGold, Upper);
    NERVES_MAKE_NOSTRUCT(KouraGold, Upper)
}
KouraGold::KouraGold(const char* name) : Koura(name) {}
void KouraGold::init(const al::ActorInitInfo& info) {
    Koura::init(info);
    al::startAction(this, "StayWait");
    mLastCoinPos.set(al::getTrans(this));
    al::tryGetArg(&mCoinNum, info, "CoinNum");
    bool slide = false;
    al::tryGetArg(&slide, info, "IsSlide");
    if (slide) {
        sead::Vector3f front;
        al::calcFrontDir(&front, this);
        startMove(front, 25.0f, 20.0f);
    }
    if (al::listenStageSwitchOnAppear(this, al::Functor(this, &KouraGold::appearStart))) kill();
    al::listenStageSwitchOnKill(this, al::Functor(this, &KouraGold::killSwitch));
}
void KouraGold::appearStart() { appear(); al::setNerve(this, &NrvKouraGoldUpper); }
void KouraGold::killSwitch() { kill(); }
const char* KouraGold::getArchiveName() const { return "KouraGold"; }
void KouraGold::onHitWall() {
    if (mWallHitCooldown > 0) return;
    int remaining = mCoinNum - mCoinsSpawned;
    if (remaining < 1) return;
    if (remaining >= 3) {
        sead::Vector3f pos = al::getTrans(this) + 100.0f * sead::Vector3f::ey;
        al::appearItemTiming(this, "壁ヒット[自動取得]", pos, sead::Vector3f::ey, al::getHitSensor(this, "Body"), false);
        mCoinsSpawned += 3;
        if (remaining == 3) startBreak();
    } else {
        const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f pos = sead::Vector3f::ey * 100.0f + trans;
        al::appearItemTiming(this, "移動[自動取得]", pos, sead::Vector3f::ey, al::getHitSensor(this, "Body"), false);
        incCoin();
    }
    mWallHitCooldown = 5;
}
void KouraGold::incCoin() {
    if (++mCoinsSpawned >= mCoinNum) {
        if (isInRouteDokan() && isPlayerInside()) mBreakDelay = 5;
        else startBreak();
    }
}
void KouraGold::control() {
    Koura::control();
    if (mCoinsSpawned < mCoinNum) {
        sead::Vector3f offset = al::getTrans(this) - mLastCoinPos;
        offset.y = 0.0f;
        if (offset.length() > 200.0f) {
            const sead::Vector3f& trans = al::getTrans(this);
        sead::Vector3f pos = sead::Vector3f::ey * 100.0f + trans;
            al::appearItemTiming(this, "移動[自動取得]", pos, sead::Vector3f::ey, al::getHitSensor(this, "Body"), false);
            mLastCoinPos.set(al::getTrans(this));
            incCoin();
        }
        if (mWallHitCooldown - 1 >= 0) --mWallHitCooldown;
    } else {
        if (!isInRouteDokan()) --mBreakDelay;
        if (mBreakDelay <= 0) startBreak();
    }
}
