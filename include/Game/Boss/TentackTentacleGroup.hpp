#pragma once
#include <basis/seadTypes.h>
#include <container/seadPtrArray.h>
#include <math/seadVector.h>

class TentackHead;
class TentackTentacle;

/** @brief Set of tentacles that attack together under one head. */
class TentackTentacleGroup {
public:
    explicit TentackTentacleGroup(s32 tentacleNumMax);
    void update();
    void reset();
    void registerTentacle(TentackTentacle* pTentacle);
    void appearAll();
    bool requestEndSwingAll();
    void eatAttachItemAll();
    TentackHead* getHead() const;
    bool isEnableEndSwing() const;
    void endSwingAllForce();

    /** @brief Checks whether no more tentacles can be registered. @return True if full. */
    bool isFull() const { return mTentacles.capacity() <= mTentacles.size(); }

private:
    unsigned char mUnknown0[0x8];
    sead::PtrArray<TentackTentacle> mTentacles;  // 0x8
    unsigned char mUnknown18[0x8];

public:
    sead::Vector3f mTargetPos;  // 0x20 position the owning head turns towards

private:
    unsigned char mUnknown2C[0x4];
};
static_assert(sizeof(TentackTentacleGroup) == 0x30);
