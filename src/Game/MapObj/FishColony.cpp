#include "Game/MapObj/FishColony.hpp"
#include "Game/MapObj/Fish.hpp"
#include "Game/Util/AreaObjUtil.hpp"
#include "Library/LiveActor/ActorAreaFunction.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
#include "Library/LiveActor/Util/ActorClippingUtil.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "Library/LiveActor/Util/ActorMovementUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/LiveActor/Util/ActorSensorUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Library/Screen/ScreenPointTarget.hpp"
#include "Library/Se/Function/SeFunction.hpp"
#include "Project/AreaObj/AreaObjUtil.hpp"
namespace {
NERVE_DECL(FishColony, Group)
class FishColonyNrvScatter : public al::Nerve {
  public:
    void execute(al::NerveKeeper* keeper) const override {
        keeper->getParent<FishColony>()->exeGroup();
    }
};
class FishColonyNrvRegroup : public al::Nerve {
  public:
    void execute(al::NerveKeeper* keeper) const override {
        keeper->getParent<FishColony>()->exeGroup();
    }
};
NERVES_MAKE_NOSTRUCT(FishColony, Group, Scatter, Regroup)
} // namespace
static void updateFishGroup(Fish* fish, al::DeriveActorGroup<Fish>* group, float avoidDist) {
    for (int i = 0; i < group->getActorCount(); ++i) {
        Fish* other = group->getDeriveActor(i);
        if (other == fish)
            continue;
        sead::Vector3f delta = al::getTrans(other) - al::getTrans(fish);
        sead::Vector3f dir = {0.0f, 0.0f, 0.0f};
        al::normalizeOrZero(&dir, delta);
        float distance = delta.length();
        if (distance < 400.0f) {
            float rate = distance / 400.0f;
            float avoidRate = avoidDist / 400.0f;
            if (rate < avoidRate) {
                float strength = (-1.0f / avoidRate) * rate + 1.0f;
                al::addVelocity(other, (strength * 0.2f) * dir);
                al::addVelocity(fish, dir * -(strength * 0.2f));
            } else if (rate < 0.375f) {
                float offset = rate - 0.55f;
                float strength = (offset * (-16.0f * offset) + 1.0f) * 0.2f;
                sead::Vector3f otherDir;
                sead::Vector3f fishDir;
                al::normalizeOrZero(&otherDir, al::getVelocity(other));
                al::normalizeOrZero(&fishDir, al::getVelocity(fish));
                al::addVelocity(other, strength * fishDir);
                al::addVelocity(fish, strength * otherDir);
            } else {
                float strength = (rate - 0.375f) * 1.6f;
                al::addVelocity(other, dir * -strength);
                al::addVelocity(fish, strength * dir);
            }
        }
    }
}
FishColony::FishColony(const char* name) : LiveActor(name) {}
void FishColony::init(const al::ActorInitInfo& info) {
    al::initActorWithArchiveName(this, info, "FishColony", nullptr);
    al::initNerve(this, &NrvFishColonyGroup, 0);
    al::tryGetArg(&mAvoidDist, info, "AvoidDist");
    if (al::isExistRail(info)) {
        al::setSyncRailToNearestPos(this);
        if (al::isLoopRail(this)) {
            float radius = 0.0f;
            al::calcRailClippingInfo(&mClippingCenter, &radius, this, 100.0f, 100.0f);
            al::setClippingInfo(this, radius, &mClippingCenter);
            mSwimArea = al::createLinkAreaGroup(this, info, "SwimArea", "泳ぎ有効エリアグループ",
                                                "子供エリア");
            int count = 5;
            al::tryGetArg(&count, info, "FishNum");
            if (count < 1 || count > 10)
                count = sead::Mathi::max(sead::Mathi::min(count, 10), 1);
            al::initSubActorKeeperNoFile(this, info, count);
            mFish = new al::DeriveActorGroup<Fish>("小魚ホルダー", count);
            for (int i = 0; i < count; ++i) {
                Fish* fish = new Fish("小魚");
                al::initCreateActorWithPlacementInfo(fish, info);
                al::setClippingInfo(fish, radius, &mClippingCenter);
                al::copyPose(fish, this);
                sead::Vector3f offset = {0.0f, 0.0f, 0.0f};
                al::getRandomVector(&offset, 50.0f);
                al::setTrans(fish, al::getTrans(fish) + offset);
                mFish->registerActor(fish);
            }
            mPreviousPos.set(al::getTrans(this));
            makeActorAppeared();
            return;
        }
    }
    makeActorDead();
}
void FishColony::control() {
    for (int i = 0; i < mFish->getActorCount(); ++i) {
        if (mSwimArea) {
            Fish* fish = mFish->getDeriveActor(i);
            if (!al::tryIsInAreaObj(mSwimArea, al::getTrans(fish))) {
                sead::Vector3f dir = al::getTrans(this) - al::getTrans(fish);
                al::normalizeOrZero(&dir);
                al::addVelocity(fish, dir * 0.75f);
            }
        }
    }
}
void FishColony::attackSensor(al::HitSensor* sender, al::HitSensor* receiver) {
    if (al::isSensorMapObj(sender) &&
        (al::isSensorPlayer(receiver) || al::isSensorHoldObj(receiver)) &&
        rc::isInWaterArea(al::getSensorHost(receiver)) && al::isNerve(this, &NrvFishColonyGroup)) {
        mSpeed = 10.0f;
        mSpeedTimer = 80;
        sead::Vector3f front = mForward ? al::getRailDir(this) : -al::getRailDir(this);
        sead::Vector3f dir = al::getSensorPos(sender) - al::getSensorPos(receiver);
        al::normalizeOrZero(&dir);
        if (front.dot(dir) < 0.0f)
            mForward = !mForward;
        al::setNerve(this, &NrvFishColonyScatter);
    }
}
bool FishColony::receiveMsg(const al::SensorMsg* msg, al::HitSensor* sender, al::HitSensor*) {
    if (al::isMsgPlayerGiantHipDrop(msg)) {
        mSpeed = 10.0f;
        mSpeedTimer = 80;
        sead::Vector3f front = mForward ? al::getRailDir(this) : -al::getRailDir(this);
        sead::Vector3f dir = al::getTrans(this) - al::getSensorPos(sender);
        al::normalizeOrZero(&dir);
        if (front.dot(dir) < 0.0f)
            mForward = !mForward;
        al::setNerve(this, &NrvFishColonyScatter);
        return true;
    }
    return false;
}
bool FishColony::receiveMsgScreenPoint(const al::SensorMsg* msg, al::ScreenPointer* pointer,
                                       al::ScreenPointTarget*) {
    if (al::isNerve(this, &NrvFishColonyGroup) && al::isMsgTouchAssistTrig(msg)) {
        mSpeed = 10.0f;
        mSpeedTimer = 80;
        sead::Vector3f front = mForward ? al::getRailDir(this) : -al::getRailDir(this);
        sead::Vector3f dir = al::getHitScreenPointTargetPos(pointer) - al::getTrans(this);
        al::normalizeOrZero(&dir);
        if (front.dot(dir) < 0.0f)
            mForward = !mForward;
        al::setNerve(this, &NrvFishColonyScatter);
        return true;
    }
    return false;
}
void FishColony::exeGroup() {
    if (al::isFirstStep(this)) {
        mPreviousPos.set(al::getTrans(this));
        if (al::isNerve(this, &NrvFishColonyScatter))
            al::startSe(this, "PgScatter", nullptr);
    }
    if (mSpeedTimer == 0) {
        mSpeed = 2.5f;
        mSpeedTimer = 40;
    } else {
        --mSpeedTimer;
    }
    al::moveSyncRail(this, mForward ? mSpeed : -mSpeed);
    mSpeed *= 0.98f;
    sead::Vector3f front = mForward ? al::getRailDir(this) : -al::getRailDir(this);
    for (int i = 0; i < mFish->getActorCount(); ++i) {
        Fish* fish = mFish->getDeriveActor(i);
        al::scaleVelocity(fish, 0.98f);
        al::resetPosition(fish, al::getTrans(fish) + al::getTrans(this) - mPreviousPos, false);
        al::setFront(fish, front);
    }
    Fish* fish = mFish->getDeriveActor(al::getNerveStep(this) % mFish->getActorCount());
    if (al::isNerve(this, &NrvFishColonyScatter)) {
        updateFishGroup(fish, mFish, 300.0f);
        al::limitVelocity(fish, 1.2f);
    } else if (al::isNerve(this, &NrvFishColonyRegroup)) {
        updateFishGroup(fish, mFish, mAvoidDist);
        al::limitVelocity(fish, 1.2f);
    } else {
        updateFishGroup(fish, mFish, mAvoidDist);
        al::limitVelocity(fish, 0.4f);
    }
    mPreviousPos.set(al::getTrans(this));
    if (al::isNerve(this, &NrvFishColonyScatter) && al::isGreaterEqualStep(this, 80))
        al::setNerve(this, &NrvFishColonyRegroup);
    else if (al::isNerve(this, &NrvFishColonyRegroup) && al::isGreaterEqualStep(this, 1))
        al::setNerve(this, &NrvFishColonyGroup);
}
