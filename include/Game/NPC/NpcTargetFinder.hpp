#pragma once

#include <basis/seadTypes.h>

#include <container/seadObjArray.h>
#include <container/seadTreeMap.h>
#include <math/seadVector.h>

namespace al {
class HitSensor;
class ScreenPointer;
class ScreenPointTarget;
class SensorMsg;
class LiveActor;
}  // namespace al

class IUseTargetFinderFilter;

namespace npc {

/**
 * @brief Kinds of targets an NPC target finder can search for (bit flags).
 */
enum NpcFindTargetType : u32 {
    NpcFindTargetType_None = 0,
    NpcFindTargetType_PlayerRegular = 1 << 0,
    NpcFindTargetType_PlayerClimb = 1 << 1,
    NpcFindTargetType_KoopaJr = 1 << 2,
    NpcFindTargetType_Cursor = 1 << 3,
    NpcFindTargetType_Ball = 1 << 4,
    NpcFindTargetType_Koura = 1 << 5,
    NpcFindTargetType_Bird = 1 << 6,
    NpcFindTargetType_Neko = 1 << 7,
    NpcFindTargetType_Player = NpcFindTargetType_PlayerRegular | NpcFindTargetType_PlayerClimb,
    NpcFindTargetType_All = 0xffffffff,
};

}  // namespace npc

/**
 * @brief Sight, sense and chase ranges used by an NPC target finder.
 */
struct NpcTargetFinderParam {
    NpcTargetFinderParam();
    NpcTargetFinderParam(f32 sightRange, f32 sightAngleH, f32 sightAngleV, u32 keepTargetFrame,
                         f32 senseRange, f32 upperRange, f32 lowerRange, f32 chaseRange,
                         bool isIgnoreDisregardPlayer, u32 candidateClearFrame);

    /** @return Range within which a found target keeps being chased. */
    f32 getChaseRange() const { return mChaseRange; }

    f32 mSightRange;               // 0x0
    f32 mSightAngleH;              // 0x4 (degrees)
    f32 mSightAngleV;              // 0x8 (degrees)
    u32 mKeepTargetFrame;          // 0xc
    f32 mSenseRange;               // 0x10
    f32 mUpperRange;               // 0x14 (negative: unlimited)
    f32 mLowerRange;               // 0x18 (negative: unlimited)
    f32 mChaseRange;               // 0x1c
    bool mIsIgnoreDisregardPlayer;  // 0x20
    s32 _24;                       // 0x24
    u32 mCandidateClearFrame;      // 0x28
};

static_assert(sizeof(NpcTargetFinderParam) == 0x2c);

/**
 * @brief Relation between a host actor and a potential target, filled by
 *        npc::calcTargetCheckInfo, plus the best target found so far.
 */
struct NpcTargetCheckInfo {
    /** @brief Forget the best target found so far. */
    void reset() {
        _0 = true;
        mTarget = nullptr;
        mNearestDistance = sead::Mathf::maxNumber();
        mIsFound = false;
        mPriority = 0;
        mIsInSight = false;
        mTargetType = npc::NpcFindTargetType_None;
        _34 = false;
    }

    bool _0 = true;
    al::LiveActor* mTarget = nullptr;
    f32 mNearestDistance = sead::Mathf::maxNumber();
    u8 mPriority = 0;
    bool mIsInSight = false;
    bool mIsFound = false;
    npc::NpcFindTargetType mTargetType = npc::NpcFindTargetType_None;
    sead::Vector3f mDirToTarget;  // 0x1c, not normalized
    f32 mDistance;                // 0x28
    f32 mFrontDot;                // 0x2c
    f32 mUpDot;                   // 0x30
    bool _34;
};

static_assert(sizeof(NpcTargetCheckInfo) == 0x38);

/**
 * @brief Actor seen by the eye sensor during the current search interval.
 */
struct NpcTargetCandidate {
    /**
     * @brief Construct a candidate.
     * @param pActor Candidate actor.
     * @param rType Kind of the candidate.
     */
    NpcTargetCandidate(al::LiveActor* pActor, const npc::NpcFindTargetType& rType)
        : mActor(pActor), mType(rType) {}

    al::LiveActor* mActor;
    npc::NpcFindTargetType mType;
};

/**
 * @brief Searches the surroundings of an NPC for targets (players, cats, balls...).
 */
class NpcTargetFinder {
public:
    typedef sead::FixedObjArray<NpcTargetCandidate, 5> CandidateArray;
    typedef sead::FixedTreeMap<npc::NpcFindTargetType, u8, 8> PriorityMap;

    NpcTargetFinder(al::LiveActor* pHost, const NpcTargetFinderParam* pParam);

    void changeHost(al::LiveActor* pHost, const char* pSensorName,
                    const IUseTargetFinderFilter* pFilter);
    void attackSensor(al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsg(const al::SensorMsg* pMsg, al::HitSensor* pSelf, al::HitSensor* pOther);
    bool receiveMsgScreenPoint(const al::SensorMsg* pMsg, al::ScreenPointer* pPointer,
                               al::ScreenPointTarget* pTarget);
    void findTarget();
    u8 getTargetPriority(const npc::NpcFindTargetType& rType) const;
    void update();
    void updateTarget();
    void forceUpdate();
    void refindTarget();
    void setTargetTypePriority(const npc::NpcFindTargetType& rType, const u8& rPriority);
    void clearPriorityMap();
    void clearTarget();
    void setFrontDir(sead::Vector3f* pFrontDir);
    void setSupportUpDir(sead::Vector3f* pUpDir);
    const sead::Vector3f& getTargetPos() const;
    const sead::Vector3f& getLastTargetPos() const;
    bool isInSightTarget(f32 distance) const;
    bool isInChaseRangeTarget(f32 distance) const;
    bool isInSenseAreaTarget() const;
    const sead::Vector3f& getFrontDir() const;
    const sead::Vector3f& getUpDir() const;

    /**
     * @brief Get the current target if it is of one of the given kinds.
     * @param types Kinds of target to accept (bit flags).
     * @return The target, or nullptr.
     */
    al::LiveActor* tryGetTarget(s32 types) const {
        if ((mTargetType & types) != 0 && mTarget != nullptr && mIsTargetValid) {
            return mTarget;
        }

        return nullptr;
    }

    /**
     * @brief Set the search parameters.
     * @param pParam Search parameters; must outlive the finder.
     */
    void setParam(const NpcTargetFinderParam* pParam) { mParam = pParam; }

    /**
     * @brief Set the kinds of targets to search for.
     * @param types Kinds of target (bit flags of npc::NpcFindTargetType).
     */
    void setSearchTypes(u32 types) { mSearchTypes = types; }

    al::HitSensor* getEyeSensor() const { return mEyeSensor; }

    /** @return The actor that searches for targets. */
    al::LiveActor* getHost() const { return mHost; }

    /** @return The filter that can reject targets, or nullptr. */
    const IUseTargetFinderFilter* getFilter() const { return mFilter; }

    /** @return Kinds of targets searched for (bit flags of npc::NpcFindTargetType). */
    u32 getSearchTypes() const { return mSearchTypes; }

    /** @return The current target, whatever its kind, or nullptr. */
    al::LiveActor* getTarget() const { return mTarget; }

    al::LiveActor* getLastTarget() const { return mLastTarget; }

    u32 getTargetType() const { return mTargetType; }

    u32 getLastTargetType() const { return mLastTargetType; }

    bool isTargetChanged() const { return mIsTargetChanged; }

    /** @return Whether the current target is in sight. */
    bool isTargetValid() const { return mIsTargetValid; }

    /** @return Whether the current target is within chase range. */
    bool isTargetInChaseRange() const { return mIsTargetInChaseRange; }

    /** @return The search parameters. */
    const NpcTargetFinderParam* getParam() const { return mParam; }

private:
    void addCandidate(al::LiveActor* pActor, npc::NpcFindTargetType type);
    void clearCandidates();
    void resetTarget();
    void loseTarget();

    al::LiveActor* mHost;                                          // 0x0
    const IUseTargetFinderFilter* mFilter = nullptr;               // 0x8
    al::LiveActor* mTarget = nullptr;                              // 0x10
    al::LiveActor* mLastTarget = nullptr;                          // 0x18
    npc::NpcFindTargetType mTargetType = npc::NpcFindTargetType_None;      // 0x20
    npc::NpcFindTargetType mLastTargetType = npc::NpcFindTargetType_None;  // 0x24
    u32 mSearchTypes = npc::NpcFindTargetType_All;                 // 0x28
    bool mIsTargetValid = false;                                   // 0x2c
    bool mIsTargetInChaseRange = false;                            // 0x2d
    bool mIsTargetChanged = false;                                 // 0x2e
    sead::Vector3f* mFrontDir = nullptr;                           // 0x30
    sead::Vector3f* mSupportUpDir = nullptr;                       // 0x38
    s32 mKeepTargetTime = 0;                                       // 0x40
    s32 mCandidateClearTime;                                       // 0x44
    const NpcTargetFinderParam* mParam;                            // 0x48
    NpcTargetCheckInfo* mCheckInfo;                                // 0x50
    CandidateArray mCandidates;                                    // 0x58
    al::HitSensor* mEyeSensor = nullptr;                           // 0xf0
    PriorityMap mPriorityMap;                                      // 0xf8
};

static_assert(sizeof(NpcTargetFinder) == 0x298);

namespace npc {
bool calcIsTargetInSight(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                         const NpcTargetFinderParam* pParam);
bool calcIsTargetInChaseRange(const NpcTargetFinder* pFinder, const al::LiveActor* pTarget,
                              const NpcTargetFinderParam* pParam);
bool calcTargetCheckInfo(NpcTargetCheckInfo* pInfo, const al::LiveActor* pHost,
                         const sead::Vector3f& rFront, const sead::Vector3f& rUp,
                         const al::LiveActor* pTarget);
}  // namespace npc
