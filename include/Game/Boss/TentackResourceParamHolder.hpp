#pragma once
#include <basis/seadTypes.h>

class TentackTentacleInfo;

/** @brief Tentacle grouping of one damage stage. */
struct TentackResourceParamInfo {
    /**
     * @brief Gets the group that attacks at a position of the attack order.
     * @param index Position in the attack order.
     * @return Group index.
     */
    s32 getGroupOrder(s32 index) const { return mGroupOrder[index]; }

    unsigned char mUnknown0[0x10];
    s32 mSwingTentacleNum;   // 0x10 tentacles that swing in the attack
    s32 mPeriodNum;          // 0x14 rounds through all groups before the attack ends
    s32 mGroupNum;           // 0x18
    s32* mGroupTentacleNum;  // 0x20 tentacles per group
    s32* mGroupOrder;        // 0x28 order the groups attack in
};

/** @brief Tentacle swing parameters of one attack pattern. */
class TentackResourceParam {
public:
    void setUpTentacleInfo(TentackTentacleInfo* pInfo, s32 index) const;

private:
    unsigned char mUnknown0[0x8];

public:
    f32** mSwingYRate;  // 0x8
};

/** @brief Loads and cycles Tentack's attack parameters from its resource. */
class TentackResourceParamHolder {
public:
    explicit TentackResourceParamHolder(const char* pSuffix);
    s32 getTentacleGroupNumMax(s32 level) const;
    s32 getSwingTentacleNumMax(s32 level) const;
    TentackResourceParam* getParamAndTurnNext(s32 damage, s32 level, s32 deadHeadType);
    const TentackResourceParamInfo* getParamInfo(s32 damage, s32 level, s32 deadHeadType) const;

private:
    unsigned char mUnknown[0x10];
};
static_assert(sizeof(TentackResourceParamHolder) == 0x10);
