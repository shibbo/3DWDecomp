#include "MapObj/Fury/CoinBlowConcentric.hpp"

#include <math/seadMathCalcCommon.h>
#include <math/seadQuat.h>

#include "Library/ActorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Movement/FlashingCtrl.hpp"
#include "Library/Nerve/NerveExecutor.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "MapObj/CoinBlow.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"

namespace {
/**
 * One ring of coins owned by a CoinBlowConcentric. The coins are laid out evenly on a circle
 * around the owner and blown outwards when the ring is triggered.
 */
class CoinConcentricCircle : public al::NerveExecutor {
public:
    typedef sead::PtrArray<CoinBlow> CoinArray;

    /**
     * @param radiusBegin radius the coins start from
     * @param radiusEnd radius the coins spread out to (sets how many coins fit on the ring)
     * @param appearFrame frames the owner waits after the last ring appeared
     * @param jumpHeight jump height of this ring's coins
     * @param pHost the owning concentric coin generator
     */
    CoinConcentricCircle(f32 radiusBegin, f32 radiusEnd, s32 appearFrame, f32 jumpHeight,
                         CoinBlowConcentric* pHost)
        : al::NerveExecutor("同心円コイン"), mRadiusBegin(radiusBegin), mRadiusEnd(radiusEnd),
          mAppearFrame(appearFrame), mJumpHeight(jumpHeight), mHost(pHost) {}

    void exeWait();
    void exeAppear();
    void exeBlow();

    CoinArray& getCoins() { return mCoins; }

private:
    CoinArray mCoins;           // 0x10
    f32 mRadiusBegin;           // 0x20
    f32 mRadiusEnd;             // 0x24
    s32 mAppearFrame;           // 0x28
    f32 mJumpHeight;            // 0x2c
    CoinBlowConcentric* mHost;  // 0x30
};

static_assert(sizeof(CoinConcentricCircle) == 0x38);

NERVE_DECL(CoinBlowConcentric, Appear);
NERVE_DECL(CoinBlowConcentric, Wait);
NERVE_DECL(CoinConcentricCircle, Wait);
NERVE_DECL(CoinConcentricCircle, Appear);
NERVE_DECL(CoinConcentricCircle, Blow);
NERVES_MAKE_NOSTRUCT(CoinBlowConcentric, Appear, Wait)
NERVES_MAKE_NOSTRUCT(CoinConcentricCircle, Wait, Appear, Blow)

/** Parameters of one "Param<Type>" entry in ObjectData/CoinConcentricCircle. */
struct CircleParam {
    f32 innerRadiusBegin;
    f32 innerRadiusEnd;
    f32 outerRadiusBegin;
    f32 outerRadiusEnd;
    f32 coinInterval;
    s32 circleNum;
    s32 appearInterval;
    s32 appearFrame;
    f32 coinJumpHeightMax;
    f32 coinJumpHeightMin;
};

const char* sParamTypeNames[] = {"Single", "Double", "Triple"};
}  // namespace

/**
 * @param pName actor name
 */
CoinBlowConcentric::CoinBlowConcentric(const char* pName) : al::LiveActor(pName) {}

/**
 * Loads the ring parameters for the placed "Type", creates every ring and its coins and starts
 * dead until appear() is called.
 * @param rInfo placement / init info
 */
void CoinBlowConcentric::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    al::initStageSwitch(this, rInfo);
    al::initActorAudioKeeper(this, rInfo, "CoinConcentricCircle", nullptr);
    al::initActorClipping(this, rInfo);
    al::trySyncStageSwitchAppear(this);
    al::initNerve(this, &NrvCoinBlowConcentricAppear, 0);

    s32 type = 0;
    al::tryGetArg(&type, rInfo, "Type");

    al::Resource* resource = al::findOrCreateResource("ObjectData/CoinConcentricCircle", nullptr);
    al::StringTmp<128> paramName("Param%s", sParamTypeNames[type]);
    al::ByamlIter iter(resource->getByml(paramName));

    CircleParam param;
    iter.tryGetFloatByKey(&param.innerRadiusBegin, "InnerRadiusBegin");
    iter.tryGetFloatByKey(&param.innerRadiusEnd, "InnerRadiusEnd");
    iter.tryGetFloatByKey(&param.outerRadiusBegin, "OuterRadiusBegin");
    iter.tryGetFloatByKey(&param.outerRadiusEnd, "OuterRadiusEnd");
    iter.tryGetFloatByKey(&param.coinInterval, "CoinInterval");
    iter.tryGetIntByKey(&param.circleNum, "CircleNum");
    iter.tryGetIntByKey(&param.appearInterval, "AppearInterval");
    iter.tryGetIntByKey(&param.appearFrame, "AppearFrame");
    iter.tryGetFloatByKey(&param.coinJumpHeightMax, "CoinJumpHeightMax");
    iter.tryGetFloatByKey(&param.coinJumpHeightMin, "CoinJumpHeightMin");

    al::tryGetArg(reinterpret_cast<s32*>(&mFloorCheckType), rInfo, "FloorCheckType");
    al::tryGetArg(&mHoverHeight, rInfo, "HoverHeight");

    mCircles.allocBuffer(param.circleNum, nullptr);

    for (s32 i = 0; i < param.circleNum; i++) {
        f32 rate = 1.0f;

        if (param.circleNum >= 2) {
            rate = static_cast<f32>(i) / static_cast<f32>(param.circleNum - 1);
        }

        f32 radiusBegin =
            param.innerRadiusBegin + rate * (param.outerRadiusBegin - param.innerRadiusBegin);
        f32 radiusEnd =
            param.innerRadiusEnd + rate * (param.outerRadiusEnd - param.innerRadiusEnd);
        u32 coinNum = static_cast<u32>(radiusEnd * sead::Mathf::pi2() / param.coinInterval) + 1;
        f32 jumpHeight = param.coinJumpHeightMax +
                         rate * (param.coinJumpHeightMin - param.coinJumpHeightMax);

        auto* circle =
            new CoinConcentricCircle(radiusBegin, radiusEnd, param.appearFrame, jumpHeight, this);
        circle->getCoins().allocBuffer(coinNum, nullptr);

        for (u32 j = 0; j < coinNum; j++) {
            auto* coin = new CoinBlow("コイン");
            al::initCreateActorWithPlacementInfo(coin, rInfo);
            coin->makeActorDead();
            circle->getCoins().pushBack(coin);
        }

        circle->initNerve(&NrvCoinConcentricCircleWait, 0);
        mCircles.pushBack(circle);
    }

    mAppearInterval = param.appearInterval;
    mAppearFrame = param.appearFrame;
    mFlashingCtrl = new al::FlashingCtrl(this, false, false);
    al::tryGetArg(&mTimer, rInfo, "Timer");
    makeActorDead();
}

/**
 * Measures the hover offset once after placement if FloorCheckType asks for it.
 */
void CoinBlowConcentric::initAfterPlacement() {
    if (mFloorCheckType == FloorCheckType::AfterPlacement) {
        calcHeightOffset(500.0f);
    }
}

/**
 * Casts a ray downwards and stores how far the coins have to be lifted to hover mHoverHeight
 * above the floor below.
 * @param checkDistance length of the downward ray
 */
void CoinBlowConcentric::calcHeightOffset(f32 checkDistance) {
    const al::ArrowHitInfo* hitInfo;

    if (alCollisionUtil::getFirstPolyOnArrow(this, &hitInfo, al::getTrans(this),
                                             sead::Vector3f::ey * -checkDistance, nullptr,
                                             nullptr)) {
        f32 distance = hitInfo->_70;
        mHeightOffset = mHoverHeight - distance;
    }
}

/**
 * Shows all coins and starts popping out the rings.
 */
void CoinBlowConcentric::appear() {
    al::LiveActor::appear();
    al::invalidateClipping(this);

    if (mFloorCheckType == FloorCheckType::Appear) {
        calcHeightOffset(500.0f);
    }

    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        CoinConcentricCircle::CoinArray& coins = it->getCoins();

        for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
            al::showModelIfHide(&*coin);
        }
    }

    mIsCoinHidden = false;
    al::setNerve(this, &NrvCoinBlowConcentricAppear);
}

/**
 * Kills every coin that is still alive along with the generator.
 */
void CoinBlowConcentric::kill() {
    al::validateClipping(this);

    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        CoinConcentricCircle::CoinArray& coins = it->getCoins();

        for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
            if (al::isAlive(&*coin)) {
                coin->kill();
            }
        }
    }

    al::LiveActor::kill();
}

/**
 * Triggers one ring every mAppearInterval frames, outermost first, and moves on to Wait once the
 * last ring had mAppearFrame frames to settle.
 */
void CoinBlowConcentric::exeAppear() {
    if (al::getNerveStep(this) % mAppearInterval == 0) {
        u32 index = al::getNerveStep(this) / mAppearInterval;

        if (index < static_cast<u32>(mCircles.size())) {
            CoinConcentricCircle* circle = mCircles[mCircles.size() - 1 - index];
            CoinConcentricCircle::CoinArray& coins = circle->getCoins();

            for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
                coin->appear();
                al::showModelIfHide(&*coin);
            }

            for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
                al::invalidateHitSensors(&*coin);
            }

            al::setNerve(circle, &NrvCoinConcentricCircleAppear);
            al::setSeSeqLocalVariableDefault(this, 0, index * 2);
            al::startSe(this, "Appear");
        }
    }

    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        it->updateNerve();
    }

    if (al::isGreaterEqualStep(this, mAppearInterval * mCircles.size() + mAppearFrame)) {
        al::setNerve(this, &NrvCoinBlowConcentricWait);
    }
}

/**
 * Flashes the coins while the timer runs out, removes the remaining ones when it does and kills
 * the generator once no coin is left.
 */
void CoinBlowConcentric::exeWait() {
    if (mTimer >= 1) {
        if (al::isFirstStep(this)) {
            mFlashingCtrl->start(mTimer);
        }

        if (mFlashingCtrl->isNowJustFlashed()) {
            if (mIsCoinHidden) {
                for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
                    CoinConcentricCircle::CoinArray& coins = it->getCoins();

                    for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
                        al::showModelIfHide(&*coin);
                    }
                }

                mIsCoinHidden = false;
            } else {
                for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
                    CoinConcentricCircle::CoinArray& coins = it->getCoins();

                    for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
                        al::hideModelIfShow(&*coin);
                    }
                }

                mIsCoinHidden = true;
            }
        }

        if (mFlashingCtrl->isEnded()) {
            for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
                CoinConcentricCircle::CoinArray& coins = it->getCoins();

                for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
                    if (al::isAlive(&*coin)) {
                        coin->kill();
                    }
                }
            }
        }

        mFlashingCtrl->movement();
    }

    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        it->updateNerve();
    }

    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        CoinConcentricCircle::CoinArray& coins = it->getCoins();

        for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
            if (al::isAlive(&*coin)) {
                return;
            }
        }
    }

    kill();
}

/**
 * Sets the life time of every coin.
 * @param frames life time in frames
 */
void CoinBlowConcentric::setTimerFrame(s32 frames) {
    for (auto it = mCircles.begin(); it != getCirclesEnd(); ++it) {
        CoinConcentricCircle::CoinArray& coins = it->getCoins();

        for (auto coin = coins.begin(); coin != coins.end(); ++coin) {
            coin->setLifeTime(frames);
        }
    }
}

/**
 * Nothing to do per frame; the rings are driven from the nerves.
 */
void CoinBlowConcentric::control() {}

/**
 * Before the ring is triggered: give every coin a downward velocity on the first step.
 */
void CoinConcentricCircle::exeWait() {
    if (al::isFirstStep(this)) {
        sead::Vector3f up;
        al::calcUpDir(&up, mHost);

        for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
            al::setVelocity(&*coin, up * -35.0f);
            coin->appear();
        }
    }
}

/**
 * Places the coins evenly on the ring around the host and blows them outwards.
 */
void CoinConcentricCircle::exeAppear() {
    f32 jumpHeight = mJumpHeight;
    f32 heightOffset = mHost->getHeightOffset();
    f32 radiusBegin = mRadiusBegin;
    f32 radiusEnd = mRadiusEnd;

    const f32 rate = 0.0f;
    f32 height = jumpHeight * rate + heightOffset * rate;
    f32 radius = radiusBegin + (radiusEnd - radiusBegin) * rate;

    sead::Vector3f up;
    sead::Vector3f front;
    al::calcUpDir(&up, mHost);
    al::calcFrontDir(&front, mHost);

    sead::Quatf rotation;
    rotation.setAxisRadian(up, sead::Mathf::pi2() / static_cast<f32>(mCoins.size()));

    for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
        sead::Vector3f* trans = al::getTransPtr(&*coin);
        *trans = al::getTrans(mHost) + front * radius + up * height;
        front.rotate(rotation);
    }

    al::calcUpDir(&up, mHost);
    al::calcFrontDir(&front, mHost);
    rotation.setAxisRadian(up, sead::Mathf::pi2() / static_cast<f32>(mCoins.size()));

    for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
        sead::Vector3f velocity = front + up;
        f32 speed = al::lerpValue(al::getRandomDegree() / 360.0f, 4.0f, 8.0f);
        velocity.x *= speed;
        velocity.y *= 35.0f;
        velocity.z *= speed;
        al::setVelocity(&*coin, velocity);
        front.rotate(rotation);
    }

    al::setNerve(this, &NrvCoinConcentricCircleBlow);
}

/**
 * Lets the blown coins be collected after a short delay.
 */
void CoinConcentricCircle::exeBlow() {
    if (al::isStep(this, 10)) {
        for (auto coin = mCoins.begin(); coin != mCoins.end(); ++coin) {
            al::validateHitSensors(&*coin);
        }
    }
}
