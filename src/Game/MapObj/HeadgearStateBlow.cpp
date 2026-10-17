#include "MapObj/HeadgearStateBlow.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/ActorCollisionFunction.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Util/ScoreUtil.hpp"

namespace {
    const HeadgearStateBlowParam sDefaultParam("BlowDamage", "BlowGoal", "BlowSilent",
        23.0f, 6.0f, 23.0f, 8.0f, 1.0f, 1.0f, 15);
    NERVE_DECL(HeadgearStateBlow, Blow);
    class HeadgearStateBlowNrvBlowSilent : public al::Nerve {
        void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<HeadgearStateBlow>()->exeBlow(); }
    };
    class HeadgearStateBlowNrvBlowGoal : public al::Nerve {
        void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<HeadgearStateBlow>()->exeBlow(); }
    };
    NERVES_MAKE_NOSTRUCT(HeadgearStateBlow, Blow, BlowSilent, BlowGoal)
}

HeadgearStateBlowParam::HeadgearStateBlowParam()
    : HeadgearStateBlowParam("BlowDamage", "BlowGoal", "BlowSilent",
        23.0f, 6.0f, 23.0f, 8.0f, 1.0f, 1.0f, 15) {}

HeadgearStateBlow::HeadgearStateBlow(al::LiveActor* pActor, const HeadgearStateBlowParam* pParam)
    : al::ActorStateBase("被り物の吹き飛び状態", pActor), mParam(pParam) {
    initNerve(&NrvHeadgearStateBlowBlow, 0);
    if (!mParam)
        mParam = &sDefaultParam;
}

bool HeadgearStateBlow::tryStart(const al::SensorMsg* pMsg, al::HitSensor* pSensor) {
    mReleaseSensor = pSensor;
    if (al::isMsgPlayerReleaseEquipmentGoal(pMsg)) {
        float up = mParam->mGoalUpSpeed;
        float back = mParam->mGoalBackSpeed;
        al::calcFrontDir(&mLaunchVelocity, al::getSensorHost(pSensor));
        mLaunchVelocity *= -back;
        mLaunchVelocity.y = up;
        if (al::getPlayerReleaseEquipmentGoalType(pMsg) == 3)
            al::setNerve(this, &NrvHeadgearStateBlowBlowSilent);
        else
            al::setNerve(this, &NrvHeadgearStateBlowBlowGoal);
        return true;
    }
    if (al::isMsgPlayerReleaseEquipment(pMsg)) {
        float up = mParam->mDamageUpSpeed;
        float back = mParam->mDamageBackSpeed;
        al::calcFrontDir(&mLaunchVelocity, al::getSensorHost(pSensor));
        mLaunchVelocity *= -back;
        mLaunchVelocity.y = up;
        al::setNerve(this, &NrvHeadgearStateBlowBlow);
        al::startSe(al::getSensorHost(pSensor), "PgRemoveHeadGear", nullptr);
        return true;
    }
    return false;
}

void HeadgearStateBlow::exeBlow() {
    if (al::isFirstStep(this)) {
        al::setVelocity(mHostActor, mLaunchVelocity);
        if (al::isNerve(this, &NrvHeadgearStateBlowBlow))
            al::startAction(mHostActor, mParam->mDamageAction);
        else if (al::isNerve(this, &NrvHeadgearStateBlowBlowSilent))
            al::startAction(mHostActor, mParam->mSilentAction);
        else
            al::startAction(mHostActor, mParam->mGoalAction);
    }
    al::addVelocityToGravity(mHostActor, mParam->mGravity);
    al::scaleVelocityY(mHostActor, mParam->mVelocityScale);
    if (al::isStep(this, mParam->mCollideStep))
        al::onCollide(mHostActor);
    if (al::isActionEnd(mHostActor) ||
        (al::isGreaterEqualStep(this, mParam->mCollideStep) && al::isCollided(mHostActor))) {
        if (al::isNerve(this, &NrvHeadgearStateBlowBlowGoal))
            rc::addScoreByFactor(mHostActor, mReleaseSensor, "成功", 100.0f, 0);
        al::startHitReactionDisappear(mHostActor);
        kill();
    }
}
