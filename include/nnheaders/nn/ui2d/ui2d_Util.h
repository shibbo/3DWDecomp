#pragma once
#include <nn/gfx/gfx_Types.h>
#include <nn/util/util_MathTypes.h>

namespace nn {
namespace gfx {
class ResShaderProgram;
class ResShaderContainer;
class ResShaderVariation;
};  // namespace gfx
namespace ui2d {
struct BuildArgSet;
class Pane;
class ResourceTextureInfo;

bool IsContain(const Pane* pPane, const nn::util::Float2& rPos);
bool LoadTexture(ResourceTextureInfo* pTextureInfo, nn::gfx::Device* pDevice, const void* pResource);
// args contains the nested layout names; depth selects how many form the prefix.
size_t CalcDynamicGenerateTexturePrefixLength(const BuildArgSet& args, int depth);
// buffer/size describe the destination for the prefix selected by args and depth.
void ConcatDynamicGenerateTexturePrefixString(char* buffer, size_t size, const BuildArgSet& args, int depth);
size_t GetAlignedBufferSize(nn::gfx::Device* pDevice, nn::gfx::GpuAccess gpuAccess, size_t size);
bool IsResShaderProgramInitialized(nn::gfx::ResShaderProgram*);

class AnimTransform;
class AnimResource;
class Group;
class Layout;
class ResourceAccessor;
struct ResCaptureTexture;
struct ResCaptureTextureList;
struct ResVectorGraphicsTexture;
struct ResVectorGraphicsTextureList;

bool ComparePaneTreeTest(const Pane* pLhs, const Pane* pRhs);
void BindAnimation(AnimTransform* pTransform, Group* pGroup, bool isEnabled);

namespace detail {
class BuildPaneTreeContext;

Pane* ClonePaneTreeWithPartsLayoutImpl_(const Pane* pSource, Layout* pPartsLayout, nn::gfx::Device* pDevice,
                                        Layout* pLayout, BuildPaneTreeContext* pContext);
const ResCaptureTexture* FindCaptureTextureResource(const ResCaptureTextureList* pList, const char* pName);
const ResVectorGraphicsTexture* FindVectorGraphicsTextureResource(const ResVectorGraphicsTextureList* pList,
                                                                 const char* pName);
void ClampValue(float& rValue, float minimum, float maximum);

/** @brief Pane subtree whose animation contents are shared with other panes. */
class AnimPaneTree {
public:
    AnimPaneTree(Pane* pTargetPane, const AnimResource& rResource);
    void Bind(nn::gfx::Device* pDevice, Layout* pLayout, Pane* pTargetPane,
              ResourceAccessor* pResourceAccessor) const;

    /**
     * @brief Check whether the source pane tree has any animation contents.
     * @return True when at least one animation content targets the tree.
     */
    bool IsEnabled() const { return m_AnimCount != 0; }

    unsigned char _00[0x70];
    u16 m_AnimCount;
    unsigned char _72[0xe];
};
}  // namespace detail

bool IsResShaderContainerInitialized(nn::gfx::ResShaderContainer*);

nn::gfx::ShaderCodeType TryInitializeAndGetShaderCodeType(nn::gfx::Device*,
                                                          nn::gfx::ResShaderVariation*);
};  // namespace ui2d
};  // namespace nn