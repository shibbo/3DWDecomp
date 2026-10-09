#pragma once
#include <nn/types.h>
namespace nn::ui2d {
struct ResAnimationBlock {
    u32 signature, size;
    u16 frameSize;
    u8 loop, padding;
    u16 textureCount, contentCount;
    u32 contentOffsets;
};
/** @brief Animated channels of one pane or material. */
struct ResAnimationContent {
    char name[28];
    u8 count;
    u8 type;
    u8 padding[2];
};
struct ResAnimationTagBlock {
    u32 signature, size;
    u16 order, groupCount;
    u32 nameOffset, groupOffset, userDataOffset;
    u32 _18;
    u8 flags;
};
struct ResAnimationShareBlock {
    u32 signature, size, infoOffset;
    u16 infoCount;
};
struct ResExtUserDataList;

/** @brief Key of a hermite animation curve. */
struct ResHermiteKey {
    float frame;
    float value;
    float slope;
};

/** @brief One animated channel of an animation info block. */
struct ResAnimationTarget {
    u8 id;
    u8 target;
    u8 curveType;
    u8 padding;
    u16 keyCount;
    u8 padding2[2];
    u32 keysOffset;

    /** @return The keys of the curve, stored relative to this target. */
    const ResHermiteKey* GetKeys() const {
        return reinterpret_cast<const ResHermiteKey*>(reinterpret_cast<const u8*>(this) + keysOffset);
    }
};

/** @brief Animated channels of one animation kind. */
struct ResAnimationInfo {
    u32 kind;
    u8 count;
    u8 padding[3];
    u32 targetOffsets[1];

    /** @return The target at index, stored relative to this block. */
    const ResAnimationTarget* GetTarget(int index) const {
        return reinterpret_cast<const ResAnimationTarget*>(reinterpret_cast<const u8*>(this) +
                                                           targetOffsets[index]);
    }
};

float GetHermiteCurveValue(float frame, const ResHermiteKey* pKeys, int keyCount);
/** @brief Animation sharing entry: a source pane's animation is copied to a group's panes. */
struct ResAnimationShareInfo {
    char srcPaneName[25];
    char targetGroupName[27];
};
class Group;
struct ResAnimationGroup { char name[0x24]; };
class AnimResource {
public:
    void Initialize();
    bool CheckResource() const;
    u16 GetTagOrder() const;
    const ResExtUserDataList* GetExtUserDataList() const;
    bool IsDescendingBind() const;
    u16 GetAnimationShareInfoCount() const;
    const ResAnimationShareInfo* GetAnimationShareInfoArray() const;
    void Set(const void* pResource);
    const char* GetTagName() const;
    u16 GetGroupCount() const;
    const ResAnimationGroup* GetGroupArray() const;
    int CalculateAnimationCount(Group* pGroup, bool isDescendingBind) const;
    const void* mFile;
    const ResAnimationBlock* mAnimation;
    const ResAnimationTagBlock* mTag;
    const ResAnimationShareBlock* mSharedAnimations;
};
static_assert(sizeof(AnimResource) == 0x20, "AnimResource size");
}
