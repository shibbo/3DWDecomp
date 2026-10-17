#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>
namespace al { class LayoutInitInfo; }
namespace nn::ui2d { class TextureInfo; }
class ListStampParts;
/** @brief Animated presentation of a stamp being added to the collection. */
class ListStampDemoParts : public al::LayoutActor {
public:
    ListStampDemoParts(const al::LayoutInitInfo& rInfo, const char* pName,
                       const char* pPartsName, al::LayoutActor* pParent, ListStampParts* pList);
    void appear() override;
    void startDecide(const char* pStampName, const sead::Vector3f& rTrans, const char* pPaneName);
    bool isDecideEnd() const;
    void exeWait();
    void exeDecide();
private:
    al::LayoutActor* mParent;
    ListStampParts* mList;
    sead::Vector3f mEffectPos = sead::Vector3f::zero;
    nn::ui2d::TextureInfo* mTextureInfo;
};
static_assert(sizeof(ListStampDemoParts) == 0x150);
