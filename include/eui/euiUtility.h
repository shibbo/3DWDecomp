#pragma once

#include <nn/ui2d/ui2d_Pane.h>
#include <prim/seadEnum.h>
#include <common/aglTextureEnum.h>
#include <math/seadVector.h>
#include <prim/seadStringBuilder.h>

#include <math/seadBoundBox.h>
#include <message/seadMessageSet.h>
#include <prim/seadDelegate.h>
#include <eui/euiScalableFontMgr.h>

namespace nn::gfx { class DescriptorSlot; }
namespace nn::ui2d {
class DrawInfo;
class Group;
class TexMap;
class TextureInfo;
struct ResExtUserData;
struct ResExtUserDataList;
}  // namespace nn::ui2d
namespace sead { class Heap; }
namespace agl { class TextureData; }
namespace agl::utl { class MultiFilter; }

namespace eui {
class FontMgr;
class LayoutEx;
class MessageString;
class TextBoxEx;

template <typename T, typename U>
T* DynamicCast(U* pObj) {
    const auto* pType = T::GetRuntimeTypeInfoStatic();

    if (pObj != nullptr) {
        for (auto* type = pObj->GetRuntimeTypeInfo(); type != nullptr; type = type->m_ParentTypeInfo) {
            if (type == pType) {
                return static_cast<T*>(pObj);
            }
        }
    }

    return nullptr;
}

void CalcPaneBoundBox(sead::BoundBox2f* pBox, const nn::ui2d::Pane& rPane);
bool AglTextureFormatToLytFormat(nn::gfx::ImageFormat* pFormat, agl::TextureFormat format,
                                 bool isSrgb);
nn::ui2d::Pane* FindPaneByPath(const char* pPath, LayoutEx* pLayout, LayoutEx** ppLayout);
void IteratePaneForRequestCapture(nn::ui2d::Pane* pPane);
void SetupDrawInfoOrtho(nn::ui2d::DrawInfo* pDrawInfo, const nn::ui2d::Size& rSize);
void SetupDrawInfoPerspective(nn::ui2d::DrawInfo* pDrawInfo, const nn::ui2d::Size& rSize,
                              float fovy);
nn::ui2d::Pane* ClonePaneTree(const nn::ui2d::Pane* pPane, LayoutEx* pLayout, bool isRoot);
nn::ui2d::Group* CloneGroup(const nn::ui2d::Group& rGroup, nn::ui2d::Pane* pRoot);
void CreateLayoutItemUniqueName(sead::StringBuilder* pName, const char* pItem, const LayoutEx* pLayout);
void AppendLayoutItemUniqueName(sead::StringBuilder* pName, const char* pItem,
                                const LayoutEx* pLayout);
void CreateLayoutItemUniqueNameByPath(sead::StringBuilder* pName, const char* pPath, const LayoutEx* pLayout);
void ProcessMessageAppTag(const MessageString& rMessage,
                          sead::IDelegate1<const sead::MessageSet<char16_t>::TagInfo*>* pDelegate);
void SetTextureInfoFromTexMap(nn::ui2d::TextureInfo* pInfo, const nn::ui2d::TexMap& rTexMap);
LayoutEx* LocateBelongingLayout(const nn::ui2d::Pane* pPane);
void PrintRapidPaneTreeMacro(const LayoutEx& rLayout, const FontMgr& rFontMgr);
void PrintRapidPaneTreeMacroRecursive_(const nn::ui2d::Pane* pPane, int depth,
                                       const FontMgr& rFontMgr);
void PrintPaneTree(const nn::ui2d::Pane* pPane, int depth);
int FindTextBoxWidthOverPosition(const TextBoxEx& rTextBox, const MessageString& rMessage);
const void* GetResSubFontFromBfcpx(const void* pData);
s32 SetupScalableFontMgrInitializeArgParameterFromArchiveBinary(
    nn::font::TextureCache::InitializeArg* pArg,
    sead::Buffer<ScalableFontMgr::FontParameter>* pParameters, sead::Heap* pHeap,
    const void* pData, size_t size);
void* AcquireFontFunction_(size_t* pSize, const char* pName, u32 type, void* pUserData);
// Draw targets are passed by value as a four-byte index.
class DrawTarget {
public:
    // index identifies one of the two display targets.
    explicit DrawTarget(int index) : mIndex(index) {}
    operator int() const volatile { return mIndex; }
private:
    int mIndex;
};

const nn::ui2d::ResExtUserData* FindExtUserDataFromList(const nn::ui2d::ResExtUserDataList* pList, const char* pName);
void AdjustPaneSizeToTextSize(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
void CenteringPanePair(nn::ui2d::Pane* pPane);
void ApplyCaptureUse(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
void ApplyDynamicCaptureUse(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
void SetupPaneAfterBuild(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
void IteratePaneForSetupPaneAfterBuild(nn::ui2d::Pane* pPane, LayoutEx* pLayout);
LayoutEx* FindHitLayout(const sead::Vector2f& rPosition, LayoutEx* pLayout);
LayoutEx* FindHitLayoutRecursive_(const sead::Vector2f& rPosition, LayoutEx* pHit, LayoutEx* pLayout, const nn::ui2d::Pane* pPane);
bool IsHitPane(const sead::Vector2f& rPosition, const nn::ui2d::Pane* pPane);
agl::utl::MultiFilter* InitializeMultiFilter(sead::Heap* pHeap,
    const nn::ui2d::Pane& rPane, LayoutEx* pLayout);
SEAD_ENUM(Direction, cUp, cDown, cLeft, cRight)

Direction GetOppositeDirection(Direction direction);
f32 GetRadAngleOfDirection(Direction direction);
bool RegisterSlotForTexture(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::TextureView& rView,
                            void* pUserData);
bool RegisterSlotForSampler(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::Sampler& rSampler,
                            void* pUserData);
void UnregisterSlotForTexture(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::TextureView& rView,
                              void* pUserData);
void UnregisterSlotForSampler(nn::gfx::DescriptorSlot* pSlot, const nn::gfx::Sampler& rSampler,
                              void* pUserData);
sead::Heap* GetNwAllocatorHeap();
void ApplyTextureInfoToMaterial(nn::ui2d::Pane* pPane, const nn::ui2d::TextureInfo& rTexture, int index);
bool SetupTextureInfoByAglTextureData(nn::ui2d::TextureInfo* pInfo,
    const agl::TextureData& rTexture, const agl::TextureCompSel* pComponentSelection);
bool SetupAglTextureDataByTextureInfo(agl::TextureData* pTexture,
    const nn::ui2d::TextureInfo& rInfo);
}
