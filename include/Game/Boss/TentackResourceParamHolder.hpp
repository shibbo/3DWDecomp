#pragma once
#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>
namespace al { class ByamlIter; }
class TentackTentacleInfo;
/** @brief Tentacle swing parameters of one attack pattern. */
class TentackResourceParam {
public:
    TentackResourceParam(const al::ByamlIter* pIter, int tentacleNum);
    void setUpTentacleInfo(TentackTentacleInfo* pInfo, int index) const;
    sead::ObjArray<float> mSwingYRate;
    sead::ObjArray<int> mBiteIndices;
    sead::ObjArray<int> mBreakableIndices;
    sead::ObjArray<int> mItemTypes;
};
/** @brief Tentacle grouping and patterns for one damage stage. */
struct TentackResourceParamInfo {
    explicit TentackResourceParamInfo(const al::ByamlIter& rIter);
    void addParam(TentackResourceParam* pParam);
    TentackResourceParam* getParam() const;
    TentackResourceParam* getParamAndTurnNext();
    int getGroupOrder(int index) const { return mGroupOrder[index]; }
    int mCurrentPattern = 0;
    int mDamage = 0;
    int mLevel = 1;
    int mSuffixId = 0;
    int mSwingTentacleNum = 7;
    int mPeriodNum = 3;
    int mGroupNum = 1;
    int* mGroupTentacleNum = nullptr;
    int* mGroupOrder = nullptr;
    sead::PtrArray<TentackResourceParam> mPatterns;
};
/** @brief Loads and cycles Tentack's attack parameters from its resource. */
class TentackResourceParamHolder {
public:
    explicit TentackResourceParamHolder(const char* pSuffix);
    int getTentacleGroupNumMax(int level) const;
    int getSwingTentacleNumMax(int level) const;
    TentackResourceParam* getParamAndTurnNext(int damage, int level, int deadHeadType);
    TentackResourceParam* getParam(int damage, int level, int deadHeadType) const;
    const TentackResourceParamInfo* getParamInfo(int damage, int level, int deadHeadType) const;
private:
    sead::PtrArray<TentackResourceParamInfo> mInfos;
};
static_assert(sizeof(TentackResourceParam) == 0x80);
static_assert(sizeof(TentackResourceParamInfo) == 0x40);
static_assert(sizeof(TentackResourceParamHolder) == 0x10);
