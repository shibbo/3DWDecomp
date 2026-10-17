#include "Boss/DarkBowserJump.hpp"
#include "Boss/DarkBowser.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
namespace {
NERVE_DECL(DarkBowserJump, JumpStart);
NERVE_DECL(DarkBowserJump, Land);
class DarkBowserJumpNrvJumpUp : public al::Nerve {
public: void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<DarkBowserJump>()->exeJump(); }
};
class DarkBowserJumpNrvJumpDown : public al::Nerve {
public: void execute(al::NerveKeeper* pKeeper) const override { pKeeper->getParent<DarkBowserJump>()->exeJump(); }
};
NERVES_MAKE_NOSTRUCT(DarkBowserJump, JumpStart)
NERVES_MAKE_STRUCT(DarkBowserJump, JumpUp, JumpDown, Land)
}
/** @brief Loads jump spots that are valid for the current phase. @param pHost Fury Bowser. @param rInfo Initialization data. */
DarkBowserJump::DarkBowserJump(DarkBowser* pHost, const al::ActorInitInfo& rInfo)
    : al::HostStateBase<DarkBowser>("DarkBowserJump", pHost) {
    initNerve(&NrvDarkBowserJumpJumpStart, 0);
    int count = al::calcLinkChildNum(rInfo, "LaserJumpPoint");
    if (count) {
        int phase = SingleModeDataFunction::getUnlockedPhase(GameDataHolderAccessor(getHost()));
        al::PlacementInfo placement;
        al::ActorInitInfo childInfo;
        mSpots = new JumpSpot[count + 1];
        mSpots[0].position = al::getTrans(getHost());
        mSpots[0].priority = 0;
        int i = 0;
        int validCount = 0;
        for (; i < count; i++) {
            al::getLinksInfoByIndex(&placement, rInfo, "LaserJumpPoint", i);
            childInfo.initNoViewId(&placement, rInfo);
            if (SingleModeDataFunction::isValidPlacement(al::tryGetLayerID(childInfo), phase, false)) {
                auto* spots = mSpots;
                ++validCount;
                al::getChildLinkT(&spots[validCount].position, rInfo, "LaserJumpPoint", i);
                if (!al::tryGetArg(&mSpots[validCount].priority, placement, "Priority")) mSpots[validCount].priority = 0;
            }
        }
        mSpotCount = validCount + 1;
    } else {
        mSpotCount = 1;
        mSpots = new JumpSpot[1];
        mSpots[0].position = al::getTrans(getHost());
        mSpots[0].priority = 0;
    }
}
/** @brief Destroys the jump state. */
DarkBowserJump::~DarkBowserJump() = default;
/** @brief Starts preparing the jump. */
void DarkBowserJump::appear() {
    al::NerveStateBase::appear();
    al::setNerve(this, &NrvDarkBowserJumpJumpStart);
}
/** @brief Restores gravity and collision and clears jump overrides. */
void DarkBowserJump::kill() {
    al::NerveStateBase::kill();
    getHost()->validateGravity();
    al::onCollide(getHost());
    mIsCenterJump = false;
    mIsIgnoreTarget = false;
}
/** @brief Builds the jump arc and plays the takeoff animation. */
void DarkBowserJump::exeJumpStart() {
    if (al::isFirstStep(this)) {
        calcJumpPosition();
        sead::Vector3f end = mSpots[mSpotIndex].position;
        const auto& hostPos = al::getTrans(getHost());
        float dx = hostPos.x - end.x;
        float dz = hostPos.z - end.z;
        if (dx * dx + dz * dz < 100000000.0f) { kill(); return; }
        sead::Vector3f start = al::getTrans(getHost());
        if (al::isNear(start.y, end.y, 1.0f)) start.y = end.y;
        mPath.initFromUpVector(start, end, sead::Vector3f::ey, 28000.0f);
        al::startAction(getHost(), "JumpStart");
        mIsIgnoreTarget = false;
    }
    if (al::isActionEnd(getHost())) {
        getHost()->invalidateGravity();
        al::offCollide(getHost());
        al::setVelocityZero(getHost());
        al::setNerve(this, &NrvDarkBowserJump.JumpUp);
    }
}
/** @brief Selects a landing spot by priority or target score. */
void DarkBowserJump::calcJumpPosition() {
    mSpotIndex = 0;
    if (mIsCenterJump) return;
    if (mIsIgnoreTarget) {
        mSpotIndex = 0;
        int best = -1;
        for (int i = 0; i < mSpotCount; i++) {
            if (best < mSpots[i].priority) { best = mSpots[i].priority; mSpotIndex = i; }
        }
        return;
    }
    sead::Vector3f hostPos = al::getTrans(getHost());
    sead::Vector3f playerPos = al::getTrans(getHost()->getPlayer());
    float best = 0.0f;
    for (int i = 0; i < mSpotCount; i++) {
        const auto* spots = mSpots;
        auto scoreFunc = mSpotScoreFunc;
        const auto& spot = spots[i].position;
        float score;
        if (scoreFunc) score = scoreFunc(hostPos, playerPos, spot);
        else {
            float dx = spot.x - playerPos.x;
            float dz = spot.z - playerPos.z;
            score = dx * dx + dz * dz;
        }
        if (score > best) { best = score; mSpotIndex = i; }
    }
}
/** @brief Advances the ascent and descent while turning toward the player. */
void DarkBowserJump::exeJump() {
    if (al::isFirstStep(this)) {
        if (al::isNerve(this, &NrvDarkBowserJump.JumpUp)) al::startAction(getHost(), "JumpUpLoop");
        else al::startAction(getHost(), "JumpDownLoop");
    }
    if (al::isGreaterEqualStep(this, 60)) {
        if (al::isNerve(this, &NrvDarkBowserJump.JumpUp)) {
            al::setNerve(this, &NrvDarkBowserJump.JumpDown);
            return;
        }
        al::setNerve(this, &NrvDarkBowserJump.Land);
        getHost()->validateGravity();
        al::onCollide(getHost());
        al::setVelocityY(getHost(), -300.0f);
        mPath.calcPosition(al::getTransPtr(getHost()), 1.0f);
        return;
    }
    float rate = al::calcNerveRate(this, 60);
    float offset = al::isNerve(this, &NrvDarkBowserJump.JumpDown) ? 0.5f : 0.0f;
    mPath.calcPosition(al::getTransPtr(getHost()), rate * 0.5f + offset);
    auto* player = getHost()->getPlayer();
    if (player) {
        float turn = al::isNerve(this, &NrvDarkBowserJump.JumpUp) ? rate : 1.0f - rate;
        const auto& playerPos = al::getTrans(player);
        const auto& end = mSpots[mSpotIndex].position;
        sead::Vector3f dir(playerPos.x - end.x, 0.0f, playerPos.z - end.z);
        al::normalizeOrDirZ(&dir);
        al::turnToDirection(getHost(), dir, turn * 4.0f);
    }
}
/** @brief Plays the landing animation and finishes. */
void DarkBowserJump::exeLand() {
    if (al::isFirstStep(this)) al::startAction(getHost(), "JumpLand");
    if (al::isActionEnd(getHost())) kill();
}
/** @brief Selects the initial arena position for the next jump. */
void DarkBowserJump::setCenterJump() { mIsCenterJump = true; }
