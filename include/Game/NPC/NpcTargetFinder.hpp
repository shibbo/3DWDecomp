#pragma once
#include "NPC/NpcTargetFunction.hpp"
#include "NPC/NpcTargetFinderParam.hpp"
#include "NPC/NpcTargetMetrics.hpp"
#include <container/seadObjArray.h>
#include <container/seadTreeMap.h>
#include <math/seadVector.h>
namespace al {
class LiveActor;
class HitSensor;
class SensorMsg;
class ScreenPointer;
class ScreenPointTarget;
} // namespace al
class IUseTargetFinderFilter;

class NpcTargetFinder {
public:
    NpcTargetFinder(al::LiveActor*, const NpcTargetFinderParam*);
    void changeHost(al::LiveActor*, const char*, const IUseTargetFinderFilter*);
    void attackSensor(al::HitSensor*, al::HitSensor*);
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*);
    bool receiveMsgScreenPoint(const al::SensorMsg*, al::ScreenPointer*, al::ScreenPointTarget*);
    void findTarget();
    const al::LiveActor* tryFilterTarget(const al::LiveActor*, const npc::NpcFindTargetType&) const;
    unsigned char getTargetPriority(const npc::NpcFindTargetType&) const;
    void update();
    void updateTarget();
    void forceUpdate();
    void refindTarget();
    void setTargetTypePriority(const npc::NpcFindTargetType&, const unsigned char&);
    void clearPriorityMap();
    void clearTarget();
    void setFrontDir(sead::Vector3f*);
    void setSupportUpDir(sead::Vector3f*);
    const sead::Vector3f& getTargetPos() const;
    const sead::Vector3f& getLastTargetPos() const;
    bool isInSightTarget(float) const;
    bool isInChaseRangeTarget(float) const;
    bool isInSenseAreaTarget() const;
    /** @brief Gets a valid target in the requested categories. @param types Category mask. @return Target, or nullptr. */
    al::LiveActor* tryGetTarget(int types) const {
        if ((mType & types) != 0 && mTarget && mInSight) { return const_cast<al::LiveActor*>(mTarget); }
        return nullptr;
    }
    /** @brief Changes search settings. @param pParam Search settings. */
    void setParam(const NpcTargetFinderParam* pParam) { mParam = pParam; }
    /** @brief Changes accepted categories. @param types Category mask. */
    void setSearchTypes(unsigned int types) { mAllowedTypes = types; }
    /** @brief Gets the candidate collection sensor. @return the candidate collection sensor. */
    al::HitSensor* getEyeSensor() const { return mSensor; }
    /** @brief Gets the host actor. @return the host actor. */
    al::LiveActor* getHost() const { return mHostActor; }
    /** @brief Gets the optional target filter. @return the optional target filter. */
    const IUseTargetFinderFilter* getFilter() const { return mFilter; }
    /** @brief Gets the accepted category mask. @return the accepted category mask. */
    unsigned int getSearchTypes() const { return mAllowedTypes; }
    /** @brief Gets the current target. @return the current target. */
    al::LiveActor* getTarget() const { return const_cast<al::LiveActor*>(mTarget); }
    /** @brief Gets the previous target. @return the previous target. */
    al::LiveActor* getLastTarget() const { return const_cast<al::LiveActor*>(mLastTarget); }
    /** @brief Gets the current target category. @return the current target category. */
    unsigned int getTargetType() const { return mType; }
    /** @brief Gets the previous target category. @return the previous target category. */
    unsigned int getLastTargetType() const { return mLastType; }
    /** @brief Gets whether the target changed. @return whether the target changed. */
    bool isTargetChanged() const { return mHasTargetChanged; }
    /** @brief Gets whether the target is in sight. @return whether the target is in sight. */
    bool isTargetValid() const { return mInSight; }
    /** @brief Gets whether the target remains in chase range. @return whether the target remains in chase range. */
    bool isTargetInChaseRange() const { return mIsInChaseRange; }
    /** @brief Gets the search settings. @return the search settings. */
    const NpcTargetFinderParam* getParam() const { return mParam; }
    al::LiveActor* mHostActor;
    const IUseTargetFinderFilter* mFilter;
    const al::LiveActor* mTarget;
    const al::LiveActor* mLastTarget;
    npc::NpcFindTargetType mType;
    npc::NpcFindTargetType mLastType;
    int mAllowedTypes;
    bool mInSight;
    bool mIsInChaseRange;
    bool mHasTargetChanged;
    sead::Vector3f* mFront;
    sead::Vector3f* mGravity;
    int mGraceFrames;
    int mScanFrames;
    const NpcTargetFinderParam* mParam;
    npc::TargetMetrics* mMetrics;
    struct Candidate { /** @brief Stores a candidate. @param pActor Candidate actor. @param type
                          Candidate category. */
        Candidate(const al::LiveActor* pActor, npc::NpcFindTargetType type)
            : mActor(pActor), mType(type) {}
        const al::LiveActor* mActor;
        npc::NpcFindTargetType mType;
    };
    sead::FixedObjArray<Candidate, 5> mCandidates;
    al::HitSensor* mSensor;
    sead::FixedTreeMap<npc::NpcFindTargetType, unsigned char, 8> mPriorities;
};
