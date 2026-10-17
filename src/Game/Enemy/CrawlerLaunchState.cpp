#include "Enemy/CrawlerLaunchState.hpp"
#include "Enemy/Crawler.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"

namespace {
NERVE_DECL(CrawlerLaunchState, LaunchSet)
NERVE_DECL(CrawlerLaunchState, AppearSign)
NERVE_DECL(CrawlerLaunchState, Appear)
NERVE_DECL(CrawlerLaunchState, AppearEnd)
NERVES_MAKE_NOSTRUCT(CrawlerLaunchState, LaunchSet)
NERVES_MAKE_STRUCT(CrawlerLaunchState, AppearSign, Appear, AppearEnd)
}

/** @brief Creates default launch settings. */
CrawlerLaunchStateParam::CrawlerLaunchStateParam()
    : mSignSpeed(1.0f), mLaunchSpeed(9.0f), mJumpSpeed(6.0f), mGravity(0.7f),
      mSignMoveTime(20), mSignTime(60), mLaunchTime(17) {}

/** @brief Creates launch settings. @param signSpeed Preparatory speed. @param launchSpeed Forward speed. @param jumpSpeed Initial upward speed. @param gravity Vertical deceleration. @param signMoveTime Preparatory movement duration. @param signTime Total preparation duration. @param launchTime Launch duration. */
CrawlerLaunchStateParam::CrawlerLaunchStateParam(f32 signSpeed, f32 launchSpeed, f32 jumpSpeed,
                                               f32 gravity, s32 signMoveTime, s32 signTime, s32 launchTime)
    : mSignSpeed(signSpeed), mLaunchSpeed(launchSpeed), mJumpSpeed(jumpSpeed), mGravity(gravity),
      mSignMoveTime(signMoveTime), mSignTime(signTime), mLaunchTime(launchTime) {}

/** @brief Creates the launcher state. @param pHost Launcher actor. @param pParam Launch settings. @param pGroup Reusable crawlers. */
CrawlerLaunchState::CrawlerLaunchState(al::LiveActor* pHost, const CrawlerLaunchStateParam* pParam,
                                     al::DeriveActorGroup<Crawler>* pGroup)
    : al::ActorStateBase("クローラー発射状態", pHost), mParam(pParam), mGroup(pGroup) {
    initNerve(&NrvCrawlerLaunchStateLaunchSet, 0);
}

/** @brief Begins selecting the next crawler to launch. */
void CrawlerLaunchState::appear() {
    mIsDead = false;
    al::setNerve(this, &NrvCrawlerLaunchStateLaunchSet);
}

/** @brief Keeps an undamaged crawler aligned with the launcher during launch. */
void CrawlerLaunchState::control() {
    al::LiveActor* pHost = mHostActor;
    if ((al::isNerve(this, &NrvCrawlerLaunchState.AppearSign) ||
         al::isNerve(this, &NrvCrawlerLaunchState.Appear) ||
         al::isNerve(this, &NrvCrawlerLaunchState.AppearEnd)) && !mCrawler->isDamaged()) {
        sead::Vector3f front = sead::Vector3f::zero;
        sead::Vector3f up = sead::Vector3f::zero;
        al::calcFrontDir(&front, pHost);
        al::calcUpDir(&up, pHost);
        al::setFront(mCrawler, front);
        Crawler* pCrawler = mCrawler;
        const sead::Vector3f& rTrans = al::getTrans(pHost);
        al::setTrans(pCrawler, mHeight * up + rTrans + mDistance * front);
    }
}

/** @brief Selects and prepares the next available crawler when the start switch allows it. */
void CrawlerLaunchState::exeLaunchSet() {
    al::LiveActor* pHost = mHostActor;
    if (al::isValidStageSwitch(pHost, "SwitchStart") && !al::isOnStageSwitch(pHost, "SwitchStart"))
        return;
    s32 count;
    for (s32 i = 0; i < (count = mGroup->getActorCount()); ++i) {
        Crawler* pCrawler = mGroup->getDeriveActor((i + mNextIndex) % count);
        if (al::isDead(pCrawler)) {
            mCrawler = pCrawler;
            break;
        }
    }
    if (!mCrawler)
        return;
    sead::Vector3f front = sead::Vector3f::zero;
    sead::Vector3f up = sead::Vector3f::zero;
    al::calcFrontDir(&front, pHost);
    al::calcUpDir(&up, pHost);
    al::setNerve(this, &NrvCrawlerLaunchState.AppearSign);
    mCrawler->resetParam(front, up, al::getTrans(pHost));
    mCrawler->appear();
    mHeight = al::getColliderRadius(mCrawler);
    mDistance = 0.0f;
    mNextIndex = (mNextIndex + 1) % mGroup->getActorCount();
    al::tryStartAction(mCrawler, "AppearSign");
}

/** @brief Moves the crawler outward before launch. */
void CrawlerLaunchState::exeAppearSign() {
    if (al::isLessEqualStep(this, mParam->mSignMoveTime))
        mDistance = mParam->mSignSpeed + mDistance;
    if (al::isGreaterEqualStep(this, mParam->mSignTime)) {
        al::setNerve(this, &NrvCrawlerLaunchState.Appear);
        mCrawler->startLaunch();
    }
}

/** @brief Advances the launch trajectory. */
void CrawlerLaunchState::exeAppear() {
    if (al::isFirstStep(this) && !mCrawler->isDamaged()) {
        al::tryStartAction(mCrawler, "Appear");
        mJumpSpeed = mParam->mJumpSpeed;
    }
    mDistance = mParam->mLaunchSpeed + mDistance;
    mHeight = mJumpSpeed + mHeight;
    mJumpSpeed -= mParam->mGravity;
    if (al::isGreaterEqualStep(this, mParam->mLaunchTime))
        al::setNerve(this, &NrvCrawlerLaunchState.AppearEnd);
}

/** @brief Finishes the launch animation and releases the crawler to walk. */
void CrawlerLaunchState::exeAppearEnd() {
    if (al::isFirstStep(this) && !mCrawler->isDamaged())
        al::tryStartAction(mCrawler, "AppearEnd");
    if (al::isGreaterEqualStep(this, 19)) {
        mCrawler->startWalk();
        kill();
    }
}
