#include "Enemy/PipePackunBody.hpp"
#include "Enemy/PipePackunBodyParts.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include <cfloat>

namespace {
NERVE_DECL(PipePackunBody, Wait)
NERVES_MAKE_NOSTRUCT(PipePackunBody, Wait)
}

/** @brief Creates a segmented stem. @param pHost Owning plant. @param pName Actor name. */
PipePackunBody::PipePackunBody(PipePackun* pHost, const char* pName) : al::LiveActor(pName), mHost(pHost) {}

/** @brief Removes the stem and all of its segments. */
void PipePackunBody::kill() {
    al::LiveActor::kill();
    for (s32 i = 0; i < mPartCount; ++i)
        mParts[i]->kill();
}

/** @brief Removes the stem through each segment's switch reaction. */
void PipePackunBody::killSwitch() {
    al::LiveActor::kill();
    for (s32 i = 0; i < mPartCount; ++i)
        mParts[i]->killSwitch();
}

/** @brief Creates the linked stem segments and their cumulative lengths. @param rInfo Placement information. */
void PipePackunBody::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initExecutorEnemyMovement(this, rInfo);
    al::initNerve(this, &NrvPipePackunBodyWait, 0);
    mPartCount = al::calcLinkNestNum(rInfo, "BodyNext") + 1;
    mParts = new PipePackunBodyParts*[mPartCount];
    al::PlacementInfo placement = al::getPlacementInfo(rInfo);
    mTotalLength = 0.0f;
    for (s32 i = 0; i < mPartCount; ++i) {
        mParts[i] = new PipePackunBodyParts(mHost, "ボディパーツ");
        al::ActorInitInfo info;
        info.initViewIdSelf(&placement, rInfo);
        al::initCreateActorWithPlacementInfo(mParts[i], info);
        mTotalLength = mParts[i]->getLength() + mTotalLength;
        mParts[i]->setTotalLength(mTotalLength);
        al::tryGetLinksInfo(&placement, placement, "BodyNext");
    }
    for (s32 i = 1; i < mPartCount; ++i)
        mParts[i]->setTurnPattern(mParts[i - 1]);
    makeActorAppeared();
}

/** @brief Waits for the owning plant to update the stem coordinate. */
void PipePackunBody::exeWait() {}

/** @brief Calculates the head position and direction along the stem. @param pPos Output position. @param pDir Output direction. @param coord Stem coordinate. */
void PipePackunBody::calcPosAndDir(sead::Vector3f* pPos, sead::Vector3f* pDir, f32 coord) const {
    if (mPartCount <= 0)
        return;
    s32 index = 0;
    f32 rate = 0.0f;
    f32 previous = 0.0f;
    for (s32 i = 0; i < mPartCount; ++i) {
        f32 end = mParts[i]->getTotalLength();
        if (end >= coord) {
            index = i;
            rate = (coord - previous) / (end - previous);
            break;
        }
        previous = end;
    }
    mParts[index]->calcPos(pPos, rate);
    mParts[index]->calcDir(pDir, rate);
}

/** @brief Sets the head coordinate on every segment. @param coord Stem coordinate. */
void PipePackunBody::setCoord(f32 coord) {
    for (s32 i = 0; i < mPartCount; ++i)
        mParts[i]->setCoord(coord);
}

/** @brief Finds the nearest stem coordinate. @param rPos World position. @return Nearest coordinate. */
f32 PipePackunBody::calcNearCoord(const sead::Vector3f& rPos) const {
    f32 bestDistance = FLT_MAX;
    f32 bestCoord = 0.0f;
    for (s32 i = 0; i < mPartCount; ++i) {
        f32 distance = 0.0f;
        f32 coord = mParts[i]->calcNearCoord(&distance, rPos);
        if (distance < bestDistance) {
            bestCoord = coord;
            bestDistance = distance;
        }
    }
    return bestCoord;
}

/** @brief Encloses the head and stem segments in a clipping sphere. @param pCenter Output center. @param pRadius Output radius. @param rHeadPos Head position. @param headRadius Head radius. */
void PipePackunBody::calcClippingSphere(sead::Vector3f* pCenter, f32* pRadius,
                                      const sead::Vector3f& rHeadPos, f32 headRadius) const {
    pCenter->set(rHeadPos);
    *pRadius = headRadius;
    for (s32 i = 0; i < mPartCount; ++i)
        al::calcSphereMargeSpheres(pCenter, pRadius, *pCenter, *pRadius,
                                   al::getTrans(mParts[i]), al::getClippingRadius(mParts[i]));
}
