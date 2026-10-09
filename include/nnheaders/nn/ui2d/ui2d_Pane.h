/**
 * @file Pane.h
 * @brief Base UI panel.
 */

#pragma once

#include <nn/font/font_Util.h>
#include <nn/types.h>

#include <nn/gfx/gfx_Types.h>
#include <nn/util/util_MathTypes.h>
#include <nn/util/util_IntrusiveList.h>

namespace nn::font {
struct Rectangle;
}

namespace nn::ui2d {
class AnimTransform;
class Layout;
class Material;
class DrawInfo;
class BuildResultInformation;
struct Size {
    float width;
    float height;
};
struct ResPane;
struct ResExtUserData;
/** @brief Kind of a system-defined extended user data entry attached to a pane. */
enum PaneSystemDataType : int {
    PaneSystemDataType_StateMachine = 19,
};
struct BuildArgSet;
namespace detail {
class BuildPaneTreeContext;
class PaneEffect;
class PaneBase {
public:
    PaneBase();
    virtual ~PaneBase();
    nn::util::IntrusiveListNode m_Link;
};
}

class Pane : public detail::PaneBase {
public:
    class CalculateContext {
    public:
        void Set(const DrawInfo& rDrawInfo, const Layout* pLayout);
        struct LayoutInformation {
            unsigned char _00[0x28];
            Size size;
        };
        unsigned char _00[0x1f];
        bool forceGlobalMatrixDirty;
        const LayoutInformation* pLayoutInformation;
        bool globalMatrixDirty;
    };
    Pane();
    Pane(const Pane& rOther) { CopyImpl(rOther, nullptr, nullptr, nullptr); }
    Pane(const Pane& rOther, nn::gfx::Device* pDevice, Layout* pLayout);
    Pane(const ResPane*, const BuildArgSet&);
    Pane(BuildResultInformation*, nn::gfx::Device*, const ResPane*, const BuildArgSet&);
    ~Pane() override = default;

    NN_RUNTIME_TYPEINFO_BASE();
    virtual void Finalize(nn::gfx::Device*);
    virtual nn::util::Unorm8x4 GetVertexColor(int) const;
    virtual void SetVertexColor(int, const nn::util::Unorm8x4&);
    virtual u8 GetColorElement(int) const;
    virtual void SetColorElement(int, u8);
    virtual u8 GetVertexColorElement(int) const;
    virtual void SetVertexColorElement(int, u8);
    virtual u32 GetMaterialCount() const;
    virtual Material* GetMaterial(int) const;
    virtual void GetSizeWithCaptureEffect(Size*) const;
    virtual void GetVertexPosWithCaptureEffect(nn::util::Float2*) const;
    virtual float GetItalicSize() const;
    virtual Pane* FindPaneByName(const char*, bool);
    virtual const Pane* FindPaneByName(const char*, bool) const;
    virtual Material* FindMaterialByName(const char*, bool);
    virtual const Material* FindMaterialByName(const char*, bool) const;
    virtual void BindAnimation(AnimTransform*, bool, bool);
    virtual void UnbindAnimation(AnimTransform*, bool);
    virtual void UnbindAnimationSelf(AnimTransform*);
    virtual void Calculate(DrawInfo&, CalculateContext&, bool);
    void CalculateGlobalMatrix(CalculateContext& rContext, bool forceDirty);
    virtual void Draw(DrawInfo&, nn::gfx::CommandBuffer&);
    virtual void DrawSelf(DrawInfo&, nn::gfx::CommandBuffer&);
    virtual void SetupPaneEffectSourceImageRenderState(nn::gfx::CommandBuffer&) const;
    virtual void LoadMtx(DrawInfo&);
    virtual Pane* FindPaneByNameRecursive(const char*);
    virtual const Pane* FindPaneByNameRecursive(const char*) const;
    virtual Material* FindMaterialByNameRecursive(const char*);
    virtual const Material* FindMaterialByNameRecursive(const char*) const;

    void CopyImpl(const Pane&, nn::gfx::Device*, const Layout*, detail::BuildPaneTreeContext*);
    Material* GetMaterial() const;
    const ResExtUserData* FindExtUserDataByName(const char* pName) const;
    void SetName(const char*);
    void SetUserData(const char*);
    void AppendChild(Pane*);
    void PrependChild(Pane*);
    void InsertChild(Pane*, Pane*);
    void RemoveChild(Pane*);
    nn::util::Float2 GetVertexPos() const;
    bool CheckInvisibleAndUpdateConstantBufferReady();
    detail::PaneEffect* GetPaneEffectInstance() const;
    const void* GetSystemExtDataByType(PaneSystemDataType type) const;
    void AddDynamicSystemExtUserData(PaneSystemDataType type, const void* pData, int dataSize);
    bool IsPaneEffectStaticCacheRenderingNeeded() const;
    void UpdateMaterialConstantBufferForEffectCapture(const DrawInfo& rDrawInfo);
    void CalculateCaptureProjectionMatrix(nn::util::MatrixT4x4fType& rMtx) const;
    void CalculateCaptureRootMatrix(nn::util::MatrixT4x3fType& rMtx,
                                    const DrawInfo& rDrawInfo) const;
    void UpdateRenderStateForPaneEffectCapture(nn::gfx::CommandBuffer& rCommands,
                                               const DrawInfo& rDrawInfo);
    const ResExtUserData* GetExtUserDataArray() const;
    int GetExtUserDataCount() const;
    const nn::font::Rectangle GetPaneRect() const;

    static Pane* FromLink(nn::util::IntrusiveListNode* node) {
        return reinterpret_cast<Pane*>(reinterpret_cast<char*>(node) - 8);
    }

    static const Pane* FromLink(const nn::util::IntrusiveListNode* node) {
        return reinterpret_cast<const Pane*>(reinterpret_cast<const char*>(node) - 8);
    }

    Pane* GetParent() const { return mParent; }
    const char* GetName() const { return mPanelName; }
    const float* GetGlobalMtx() const { return mGlobalMtx; }
    const nn::util::MatrixT4x3fType& GetGlobalMatrix() const {
        return *reinterpret_cast<const nn::util::MatrixT4x3fType*>(mGlobalMtx);
    }
    const Size& GetSize() const { return *reinterpret_cast<const Size*>(&mSizeX); }
    u8 GetGlobalAlpha() const { return mAlphaInfluence; }
    bool IsPaneEffectEnabled() const { return (mSystemFlags & 2) != 0; }
    /** @brief Marks the constant buffer of the pane as not yet built for this frame. */
    void ResetConstantBufferReady() { mFlagEx &= ~0x10; }
    float GetPositionX() const { return mPositionX; }
    float GetPositionY() const { return mPositionY; }
    float GetSizeX() const { return mSizeX; }
    float GetSizeY() const { return mSizeY; }
    int GetBasePositionX() const { return mOriginFlags & 3; }
    int GetBasePositionY() const { return (mOriginFlags >> 2) & 3; }

    const nn::util::Float3& GetTranslate() const {
        return *reinterpret_cast<const nn::util::Float3*>(&mPositionX);
    }

    void SetTranslate(const nn::util::Float3& rTranslate) {
        *reinterpret_cast<nn::util::Float3*>(&mPositionX) = rTranslate;
        mFlags |= 0x10;
    }

    void SetVisible(bool isVisible) {
        if (isVisible) {
            mFlags |= 1;
        } else {
            mFlags &= ~1;
        }
    }

    /** @brief Sets the pane size and marks the global matrix dirty. */
    void SetSize(const Size& rSize) {
        mSizeX = rSize.width;
        mSizeY = rSize.height;
        mFlags |= 0x10;
    }

    // Sets the X/Y position and marks the global matrix dirty.
    void SetPositionXY(float x, float y) {
        mPositionX = x;
        mPositionY = y;
        mFlags |= 0x10;
    }

    Pane* mParent;
    nn::util::IntrusiveListNode m_Children;
    float mPositionX;
    float mPositionY;
    float mPositionZ;
    float mRotationX;
    float mRotationY;
    float mRotationZ;
    float mScaleX;
    float mScaleY;
    float mSizeX;
    float mSizeY;
    u8 mFlags;
    u8 mAlpha;
    u8 mAlphaInfluence;
    u8 mOriginFlags;
    union {
        u32 _5C;
        u8 mFlagEx;
    };
    u16 _60;
    u8 mSystemFlags;
    u8 _63;
    u32 _64;
    Layout* mLayout;
    float mGlobalMtx[12];
    u64 _A0;
    void* mAnimExtUserData;
    char mPanelName[0x19];
    char mUserData[9];
};

namespace detail {
void CalculateCaptureRootMatrix(nn::util::MatrixT4x3fType& rMtx, const DrawInfo& rDrawInfo);
}  // namespace detail
}  // namespace nn::ui2d
