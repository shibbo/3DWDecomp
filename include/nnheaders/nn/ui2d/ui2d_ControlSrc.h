#pragma once
#include <nn/ui2d/ui2d_ExtUserData.h>
namespace nn::ui2d {
struct ResExtUserDataList {
    u32 signature;
    u32 size;
    u16 count;
    u16 reserved;
    ResExtUserData entries[1];
};
class ControlSrc {
public:
    ControlSrc();
    /**
     * @brief Construct a control description directly from a control block.
     * @param pResource Control block of a layout resource.
     * @param pExtData Extended user data following the block, or nullptr.
     */
    ControlSrc(const void* pResource, const ResExtUserDataList* pExtData) { Initialize(pResource, pExtData); }
    void Initialize(const void* pResource, const ResExtUserDataList* pExtData);
    const char* GetFunctionalPaneName(int index) const;
    const char* FindFunctionalPaneName(const char* pName) const;
    const char* GetFunctionalAnimName(int index) const;
    const char* FindFunctionalAnimName(const char* pName) const;
    int GetExtUserDataCount() const;
    const ResExtUserData* GetExtUserDataArray() const;
    const ResExtUserData* FindExtUserDataByName(const char* pName) const;
    const char* mName;
    const char* mUserName;
    u16 mPaneCount;
    u16 mAnimCount;
    const char* mPaneNames;
    const u32* mAnimNameOffsets;
    const u32* mPaneFunctionOffsets;
    const u32* mAnimFunctionOffsets;
    const ResExtUserDataList* mExtData;
};
static_assert(sizeof(ControlSrc) == 0x40, "ControlSrc size");
}
