#include "Boss/TentackResourceParamHolder.hpp"
#include "Boss/TentackTentacle.hpp"
#include "Library/Yaml/ByamlIter.hpp"
#include "Library/Math/MathUtil.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Project/Base/StringUtil.hpp"
namespace {
/** @brief Loads a list of tentacle indices. @param pArray Destination. @param pIter Pattern settings. @param pKey List key. */
void initIndexArray(sead::ObjArray<int>* pArray, const al::ByamlIter* pIter, const char* pKey) {
    al::ByamlIter iter;
    if (pIter->tryGetIterByKey(&iter, pKey)) {
        pArray->allocBuffer(iter.getSize(), nullptr);
        for (int i = 0; i < pArray->capacity(); i++) {
            int value = -1;
            if (iter.tryGetIntByIndex(&value, i)) pArray->emplaceBack(value);
        }
    }
}
/** @brief Checks whether a tentacle index is present. @return Whether found. */
bool containsIndex(const sead::ObjArray<int>& rArray, int index) {
    for (int i = 0; i < rArray.size(); i++)
        if (*rArray.unsafeAt(i) == index) return true;
    return false;
}
}
/** @brief Loads swing heights, scaffold flags, and item assignments. @param pIter Pattern settings. @param tentacleNum Number of swinging tentacles. */
TentackResourceParam::TentackResourceParam(const al::ByamlIter* pIter, int tentacleNum) {
    al::ByamlIter heightIter;
    pIter->tryGetIterByKey(&heightIter, "HeightLevels");
    mSwingYRate.allocBuffer(heightIter.getSize(), nullptr);
    for (int i = 0; i < mSwingYRate.capacity(); i++) {
        float value = 0.0f;
        heightIter.tryGetFloatByIndex(&value, i);
        mSwingYRate.emplaceBack(value);
    }
    initIndexArray(&mBiteIndices, pIter, "KillStepList");
    initIndexArray(&mBreakableIndices, pIter, "BreakableStepList");
    mItemTypes.allocBuffer(tentacleNum, nullptr);
    for (int i = 0; i < tentacleNum; i++) mItemTypes.emplaceBack(0);
    al::ByamlIter bellIter;
    al::ByamlIter coinIter;
    if (pIter->tryGetIterByKey(&bellIter, "AttachSuperBell")) {
        for (int i = 0; i < bellIter.getSize(); i++) {
            int index = -1;
            if (bellIter.tryGetIntByIndex(&index, i)) *mItemTypes.unsafeAt(index) = 2;
        }
    }
    if (pIter->tryGetIterByKey(&coinIter, "AttachCoinThree")) {
        for (int i = 0; i < coinIter.getSize(); i++) {
            int index = -1;
            if (coinIter.tryGetIntByIndex(&index, i)) *mItemTypes.unsafeAt(index) = 1;
        }
    }
}
/** @brief Applies the pattern flags for one tentacle. @param pInfo Output settings. @param index Tentacle index. */
void TentackResourceParam::setUpTentacleInfo(TentackTentacleInfo* pInfo, int index) const {
    pInfo->mIsBite = containsIndex(mBiteIndices, index);
    pInfo->mIsStepBreakable = containsIndex(mBreakableIndices, index);
    pInfo->mItemType = *mItemTypes.unsafeAt(index);
}
/** @brief Loads grouping and attack patterns for one stage. @param rIter Stage settings. */
TentackResourceParamInfo::TentackResourceParamInfo(const al::ByamlIter& rIter) {
    rIter.tryGetIntByKey(&mDamage, "Damage");
    rIter.tryGetIntByKey(&mLevel, "Level");
    rIter.tryGetIntByKey(&mSuffixId, "SuffixId");
    rIter.tryGetIntByKey(&mSwingTentacleNum, "SwingTentacleNum");
    rIter.tryGetIntByKey(&mPeriodNum, "AttackPeriod");
    {
        al::ByamlIter iter;
        if (!rIter.tryGetIterByKey(&iter, "GroupAllotPattern")) return;
        mGroupNum = iter.getSize();
        mGroupTentacleNum = new int[mGroupNum];
        for (int i = 0; i < mGroupNum; i++) iter.tryGetIntByIndex(&mGroupTentacleNum[i], i);
    }
    {
        al::ByamlIter iter;
        if (!rIter.tryGetIterByKey(&iter, "GroupAppearPattern")) return;
        mGroupOrder = new int[mGroupNum];
        for (int i = 0; i < mGroupNum; i++) iter.tryGetIntByIndex(&mGroupOrder[i], i);
    }
    {
        al::ByamlIter iter;
        if (!rIter.tryGetIterByKey(&iter, "Parameters")) return;
        mPatterns.allocBuffer(iter.getSize(), nullptr);
        for (int i = 0; i < iter.getSize(); i++) {
            al::ByamlIter param;
            if (iter.tryGetIterByIndex(&param, i)) addParam(new TentackResourceParam(&param, mSwingTentacleNum));
        }
    }
}
/** @brief Adds an attack pattern. @param pParam Pattern to append. */
void TentackResourceParamInfo::addParam(TentackResourceParam* pParam) { mPatterns.pushBack(pParam); }
/** @brief Returns the selected attack pattern. @return Current pattern. */
TentackResourceParam* TentackResourceParamInfo::getParam() const { return mPatterns.at(mCurrentPattern); }
/** @brief Advances the pattern index. @return Previously selected pattern. */
TentackResourceParam* TentackResourceParamInfo::getParamAndTurnNext() {
    int index = mCurrentPattern;
    mCurrentPattern = al::modi(index + mPatterns.size() + 1, mPatterns.size());
    return mPatterns.at(index);
}
/** @brief Loads the boss parameter resource. @param pSuffix Optional resource suffix. */
TentackResourceParamHolder::TentackResourceParamHolder(const char* pSuffix) {
    auto* resource = al::findOrCreateResource("ObjectData/Tentack", nullptr);
    al::ByamlIter root(resource->getByml(al::StringTmp<32>("%s%s", "TentackParam", pSuffix ? pSuffix : "").cstr()));
    al::ByamlIter levels;
    root.tryGetIterByKey(&levels, "Levels");
    mInfos.allocBuffer(levels.getSize(), nullptr);
    for (int i = 0; i < levels.getSize(); i++) {
        al::ByamlIter iter;
        if (levels.tryGetIterByIndex(&iter, i)) mInfos.pushBack(new TentackResourceParamInfo(iter));
    }
}
/** @brief Finds the largest swing count for a level. @param level Boss level. @return Maximum count. */
int TentackResourceParamHolder::getSwingTentacleNumMax(int level) const {
    int max = 0;
    for (int i = 0; i < mInfos.size(); i++)
        if (mInfos[i]->mLevel == level && max < mInfos[i]->mSwingTentacleNum) max = mInfos[i]->mSwingTentacleNum;
    return max;
}
/** @brief Finds the largest group count for a level. @param level Boss level. @return Maximum count. */
int TentackResourceParamHolder::getTentacleGroupNumMax(int level) const {
    int max = 0;
    for (int i = 0; i < mInfos.size(); i++)
        if (mInfos[i]->mLevel == level && max < mInfos[i]->mGroupNum) max = mInfos[i]->mGroupNum;
    return max;
}
/** @brief Finds stage settings for the current boss state. @return Matching settings, or null. */
const TentackResourceParamInfo* TentackResourceParamHolder::getParamInfo(int damage, int level, int deadHeadType) const {
    for (int i = 0; i < mInfos.size(); i++)
        if (mInfos[i]->mDamage == damage && mInfos[i]->mLevel == level && mInfos[i]->mSuffixId == deadHeadType) return mInfos.at(i);
    return nullptr;
}
/** @brief Finds the current attack pattern for the boss state. @return Matching pattern, or null. */
TentackResourceParam* TentackResourceParamHolder::getParam(int damage, int level, int deadHeadType) const {
    for (int i = 0; i < mInfos.size(); i++)
        if (mInfos[i]->mDamage == damage && mInfos[i]->mLevel == level && mInfos[i]->mSuffixId == deadHeadType) return mInfos[i]->getParam();
    return nullptr;
}
/** @brief Selects and advances the attack pattern for the boss state. @return Selected pattern, or null. */
TentackResourceParam* TentackResourceParamHolder::getParamAndTurnNext(int damage, int level, int deadHeadType) {
    for (int i = 0; i < mInfos.size(); i++)
        if (mInfos[i]->mDamage == damage && mInfos[i]->mLevel == level && mInfos[i]->mSuffixId == deadHeadType) return mInfos.at(i)->getParamAndTurnNext();
    return nullptr;
}
