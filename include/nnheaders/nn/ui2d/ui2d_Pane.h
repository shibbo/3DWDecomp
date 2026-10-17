/**
 * @file Pane.h
 * @brief Base UI panel.
 */

#pragma once

#include <nn/font/font_Util.h>
#include <nn/types.h>

#include <nn/gfx/gfx_Types.h>
#include <nn/ui2d/ui2d_ControlSrc.h>
#include <nn/util/util_IntrusiveList.h>
#include <nn/util/util_MathTypes.h>

namespace nn::font {
struct Rectangle;
}

namespace nn::ui2d {
class AnimTransform;
class Layout;
class Material;
class DrawInfo;
class ResourceAccessor;
class StateMachine;
class BuildResultInformation;
struct Size {
    float width;
    float height;
};
struct ResPane;
struct ResExtUserData;
/** @brief Kind of a system-defined extended user data entry attached to a pane. */
enum PaneSystemDataType : int {
    PaneSystemDataType_SimpleAABBCollision = 0,
    PaneSystemDataType_SimpleOBBCollision = 1,
    PaneSystemDataType_AlignmentExInfo = 2,
    PaneSystemDataType_Mask = 3,
    PaneSystemDataType_DropShadow = 4,
    PaneSystemDataType_ProceduralShape = 6,
    PaneSystemDataType_PaneEffectInstance = 17,
    PaneSystemDataType_ProceduralShapeRuntimeInfo = 18,
    PaneSystemDataType_StateMachine = 19,
    PaneSystemDataType_DynamicInfo = 20,
    PaneSystemDataType_ReferenceTable = 21,
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
}  // namespace detail

/** @brief Common header of a system data entry stored in the extended user data of a pane. */
struct SystemDataBase {
    u32 type;
};

/** @brief Maps every system data type to the index of its entry, or -1 when it is absent. */
struct SystemDataReferenceTable : SystemDataBase {
    s8 indices[13];
};

/** @brief System data holding the pane effect instance of a pane. */
struct SystemDataPaneEffectInstance : SystemDataBase {
    detail::PaneEffect* pPaneEffect;
};

/** @brief System data holding the state machine of a pane. */
struct SystemDataStateMachineInstance : SystemDataBase {
    StateMachine* pStateMachine;
};

/** @brief System data with the mask settings of a pane. */
struct SystemDataMaskTexture : SystemDataBase {
    /** @brief Bits of flags. */
    enum Flag {
        Flag_TextureMatrixToSlot0 = 1 << 0,
        Flag_StaticCache = 1 << 1,
    };

    /** @brief Bits of textureFlags. */
    enum TextureFlag {
        TextureFlag_CaptureTexture = 1 << 0,
        TextureFlag_VectorGraphicsTexture = 1 << 1,
    };

    /** @brief Bits of captureFlags. */
    enum CaptureFlag {
        CaptureFlag_UseCaptureTexture = 1 << 0,
    };

    u8 flags;
    u8 _05[3];
    u16 textureIndex;
    u8 wrapAndFilterS;
    u8 wrapAndFilterT;
    u32 textureFlags;
    u16 captureTextureIndex;
    u8 captureWrapAndFilterS;
    u8 captureWrapAndFilterT;
    u8 captureFlags;
    float texSrt[5];

    /** @return Whether the masked image is rendered once into a static cache. */
    bool IsStaticRenderingEnabled() const { return (flags & 2) != 0; }
};

/** @brief System data with the drop shadow settings of a pane. */
struct SystemDataDropShadow : SystemDataBase {
    /** @brief Kind of a drop shadow layer. */
    enum DropShadowType {
        DropShadowType_Stroke,
        DropShadowType_OuterGlow,
        DropShadowType_DropShadow,
        DropShadowType_Max
    };

    /** @brief Bits of flags. */
    enum Flag {
        Flag_StrokeEnabled = 1 << 0,
        Flag_OuterGlowEnabled = 1 << 1,
        Flag_DropShadowEnabled = 1 << 2,
        Flag_KnockoutEnabled = 1 << 3,
        Flag_OnlyEffectEnabled = 1 << 4,
        Flag_StaticCache = 1 << 5,
        Flag_HighQualityBlur = 1 << 6,
    };

    union {
        u32 _04;
        struct {
            u16 captureTextureIndex;
            u8 captureWrapAndFilterS;
            u8 captureWrapAndFilterT;
        };
    };
    u8 flags;
    u8 _09[3];
    u8 blurPaddingSize;
    u8 blendMode[DropShadowType_Max];
    u8 _10[0x10];
    float strokeSize;
    nn::util::Float4 strokeColor;
    nn::util::Float4 outerGlowColor;
    float outerGlowSpread;
    float outerGlowSize;
    nn::util::Float4 dropShadowColor;
    float dropShadowAngle;
    float dropShadowDistance;
    float dropShadowSpread;
    float dropShadowSize;

    /** @return Whether the shadow is rendered once into a static cache. */
    bool IsStaticRenderingEnabled() const { return (flags & 0x20) != 0; }
};

/** @brief System data with extended alignment settings of a pane. */
struct SystemDataAlignmentExInfo : SystemDataBase {
    u8 flags;
    float margin;
};

/** @brief System data with the settings of a procedural shape. */
struct SystemDataProceduralShape : SystemDataBase {
    /** @brief Bits of flags. */
    enum Flag {
        Flag_InnerStroke = 1 << 0,
        Flag_InnerShadow = 1 << 1,
        Flag_ColorOverlay = 1 << 2,
        Flag_GradationOverlay = 1 << 3,
        Flag_DropShadow = 1 << 4,
        Flag_DropShadowKnockout = 1 << 5,
        Flag_IndividualCorner = 1 << 6,
        Flag_EffectOnly = 1 << 7,
    };

    u8 flags;
    u8 innerStrokeBlendMode;
    u8 innerShadowBlendMode;
    u8 innerShadowType;
    u8 colorOverlayBlendMode;
    u8 gradationOverlayBlendMode;
    u8 dropShadowBlendMode;
    u8 dropShadowType;
    u8 _0C[0x10];
    float exp[4];
    float radius[4];
    float innerStrokeSize;
    float innerStrokeColor[4];
    float innerShadowColor[4];
    float innerShadowAngle;
    float innerShadowDistance;
    float innerShadowSize;
    float colorOverlayColor[4];
    float gradationOverlayControlPoint[4];
    float gradationOverlayColor[4][4];
    float gradationOverlayAngle;
    float dropShadowColor[4];
    float dropShadowAngle;
    float dropShadowDistance;
    float dropShadowSize;
    u8 _EC[0x10];
};

/** @brief Runtime state of a procedural shape. */
struct SystemDataProceduralShapeRuntimeInfo : SystemDataBase {
    int constantBufferSlot;
    u32 constantBufferOffset;
    int dropShadowVertexConstantBufferOffset;
    int dropShadowConstantBufferOffset;
};

/** @brief System data with an axis-aligned collision rectangle relative to the pane size. */
struct SystemDataSimpleAABBCollision : SystemDataBase {
    nn::util::Float2 size;
    nn::util::Float2 offset;
};

/** @brief System data with a rotated collision rectangle relative to the pane size. */
struct SystemDataSimpleOBBCollision : SystemDataSimpleAABBCollision {
    float rotate;
};

/** @brief Runtime information added to panes built with dynamic data enabled. */
struct SystemDataDynamicInfo : SystemDataBase {
    u64 value;
};

/** @brief Payload of the system data entry: a count and the offsets of every system data. */
struct SystemDataHeader {
    u16 version;
    u16 count;
    u32 offsets[1];
};

class Pane : public detail::PaneBase {
public:
    using PaneList =
        nn::util::IntrusiveList<Pane, nn::util::IntrusiveListMemberNodeTraits<
                                          detail::PaneBase, &detail::PaneBase::m_Link, Pane,
                                          sizeof(detail::PaneBase)>>;

    class CalculateContext {
    public:
        void SetDefault();
        void Set(const DrawInfo& rDrawInfo, const Layout* pLayout);
        struct LayoutInformation {
            unsigned char _00[0x28];
            Size size;
        };
        const void* pRectDrawer;
        const nn::util::MatrixT4x3fType* pViewMtx;
        nn::util::Float2 locationAdjustScale;
        float influenceAlpha;
        bool isLocationAdjust;
        bool isInvisiblePaneCalculateMtx;
        bool isAlphaZeroPaneCalculateMtx;
        union {
            bool isInfluenceAlpha;
            /** @brief Older name of isInfluenceAlpha. */
            bool forceGlobalMatrixDirty;
        };
        const LayoutInformation* pLayoutInformation;
        bool globalMatrixDirty;
    };
    Pane();
    Pane(const Pane& rOther) { CopyImpl(rOther, nullptr, nullptr, nullptr); }
    Pane(const Pane& rOther, nn::gfx::Device* pDevice, Layout* pLayout);
    Pane(const ResPane*, const BuildArgSet&);
    Pane(BuildResultInformation*, nn::gfx::Device*, const ResPane*, const BuildArgSet&);
    ~Pane() override;

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

    void InitializeParams();
    void InitializeByResourceBlock(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                                   const ResPane* pResPane, const BuildArgSet& rBuildArgSet);
    void AllocateAndCopyAnimatedExtUserData(const ResExtUserDataList* pList);
    void SetExtUserDataList(const ResExtUserDataList* pList);
    void AddSystemExtUserDataReferenceTable();
    void CalculateScaleFromPartsRoot(nn::util::Float2* pScale, Pane* pPane) const;
    void ApplyProceduralShapeOverride(const BuildArgSet& rBuildArgSet);
    void InitializePaneEffects(BuildResultInformation* pResult, nn::gfx::Device* pDevice,
                               const BuildArgSet& rBuildArgSet);
    void* GetSystemExtDataForModify(PaneSystemDataType type);
    void CopyImpl(const Pane&, nn::gfx::Device*, const Layout*, detail::BuildPaneTreeContext*);
    void CopyImpl(const Pane& rOther, nn::gfx::Device* pDevice, ResourceAccessor* pAccessor,
                  const char* pNewRootName, const Layout* pLayout);
    void SetUserDataAsBinary(const void* pData);
    bool IsExtUserDataMemoryDynamicallyAllocated() const;
    void InsertChild(PaneList::iterator next, Pane* pChild);
    bool IsConstantBufferUpdateNeeded() const;
    void CalculateGlobalMatrixSelf(CalculateContext& rContext);
    void UpdateSystemExtDataFlag(const ResExtUserDataList* pList);
    void AddDynamicSystemExtUserDataImpl(PaneSystemDataType type, const void* pData, int dataSize,
                                         bool isReferenceTable);
    void AddDynamicSystemExtUserDataAllNewImpl(const void* pData, int dataSize);
    void AddDynamicSystemExtUserDataNewSystemDataImpl(const void* pData, int dataSize);
    void AddDynamicSystemExtUserDataToSystemDataImpl(const void* pData, int dataSize,
                                                     bool isReferenceTable);
    void DrawChildren(DrawInfo& rDrawInfo, nn::gfx::CommandBuffer& rCommands);
    bool CompareCopiedInstanceTest(const Pane& rOther) const;
    bool IsMaskEnabled() const;
    bool IsDropShadowEnabled() const;
    const ResExtUserData* GetExtUserDataArrayForAnimation() const;
    const ResExtUserData* FindExtUserDataByNameForAnimation(const char* pName) const;
    int ConvertSystemExtDataTypeToReferenceTableIndex(PaneSystemDataType type) const;
    const SystemDataAlignmentExInfo* FindAlignmentExInfo() const;
    bool IsAlignmentIgnore();
    bool IsAlignmentMarginEnabled();
    bool IsAlignmentNullPane();
    float GetAlignmentMargin();

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
    const char* GetUserData() const { return mUserData; }
    const float* GetGlobalMtx() const { return mGlobalMtx; }
    const nn::util::MatrixT4x3fType& GetGlobalMatrix() const {
        return *reinterpret_cast<const nn::util::MatrixT4x3fType*>(mGlobalMtx);
    }
    const Size& GetSize() const { return *reinterpret_cast<const Size*>(&mSizeX); }
    u8 GetAlpha() const { return mAlpha; }
    u8 GetGlobalAlpha() const { return mAlphaInfluence; }
    bool IsVisible() const { return (mFlags & PaneFlag_Visible) != 0; }
    bool IsInfluencedAlpha() const { return (mFlags & PaneFlag_InfluencedAlpha) != 0; }
    bool IsLocationAdjust() const { return (mFlags & PaneFlag_LocationAdjust) != 0; }
    bool IsUserAllocated() const { return (mFlags & PaneFlag_UserAllocated) != 0; }
    bool IsGlobalMatrixDirty() const { return (mFlags & PaneFlag_IsGlobalMatrixDirty) != 0; }
    bool IsUserMatrix() const { return (mFlags & PaneFlag_UserMatrix) != 0; }
    bool IsUserGlobalMatrix() const { return (mFlags & PaneFlag_UserGlobalMatrix) != 0; }
    bool IsCalculationFinished() const { return (mFlags & PaneFlag_IsCalculationFinished) != 0; }
    /** @brief Marks the global matrix as needing to be recalculated. */
    void SetGlobalMatrixDirty() { mFlags |= PaneFlag_IsGlobalMatrixDirty; }
    bool IsPaneEffectEnabled() const { return (mSystemFlags & 2) != 0; }
    bool IsConstantBufferReady() const { return (mFlagEx & PaneFlagEx_IsConstantBufferReady) != 0; }
    /** @brief Marks the constant buffer of the pane as not yet built for this frame. */
    void ResetConstantBufferReady() { mFlagEx &= ~0x10; }
    float GetPositionX() const { return mPositionX; }
    float GetPositionY() const { return mPositionY; }
    float GetSizeX() const { return mSizeX; }
    float GetSizeY() const { return mSizeY; }
    int GetBasePositionX() const { return mOriginFlags & 3; }
    int GetBasePositionY() const { return (mOriginFlags >> 2) & 3; }
    int GetParentRelativePositionX() const { return (mOriginFlags >> 4) & 3; }
    int GetParentRelativePositionY() const { return mOriginFlags >> 6; }

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

    /** @brief Sets the X position and marks the global matrix dirty. */
    void SetPositionX(float x) {
        mPositionX = x;
        mFlags |= 0x10;
    }

    /** @brief Sets the Y position and marks the global matrix dirty. */
    void SetPositionY(float y) {
        mPositionY = y;
        mFlags |= 0x10;
    }

    /** @brief Sets the width and marks the global matrix dirty. */
    void SetSizeX(float width) {
        mSizeX = width;
        mFlags |= 0x10;
    }

    /** @brief Sets the height and marks the global matrix dirty. */
    void SetSizeY(float height) {
        mSizeY = height;
        mFlags |= 0x10;
    }

    // Sets the X/Y position and marks the global matrix dirty.
    void SetPositionXY(float x, float y) {
        mPositionX = x;
        mPositionY = y;
        mFlags |= 0x10;
    }

    /** @brief Bits of mFlags. */
    enum PaneFlag {
        PaneFlag_Visible = 1 << 0,
        PaneFlag_InfluencedAlpha = 1 << 1,
        PaneFlag_LocationAdjust = 1 << 2,
        PaneFlag_UserAllocated = 1 << 3,
        PaneFlag_IsGlobalMatrixDirty = 1 << 4,
        PaneFlag_UserMatrix = 1 << 5,
        PaneFlag_UserGlobalMatrix = 1 << 6,
        PaneFlag_IsCalculationFinished = 1 << 7,
    };

    /** @brief Bits of mFlagEx. */
    enum PaneFlagEx {
        PaneFlagEx_IgnorePartsMagnify = 1 << 0,
        PaneFlagEx_PartsMagnifyAdjustToPartsBound = 1 << 1,
        PaneFlagEx_ExtUserDataAnimationEnabled = 1 << 2,
        PaneFlagEx_IsConstantBufferReady = 1 << 4,
        PaneFlagEx_DynamicExtUserDataEnabled = 1 << 6,
    };

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
    union {
        u32 m_SystemExtDataFlag;
        struct {
            u16 _60;
            u8 mSystemFlags;
            u8 _63;
        };
    };
    u32 _64;
    Layout* mLayout;
    float mGlobalMtx[12];
    const nn::util::MatrixT4x3fType* m_pUserMtx;
    const ResExtUserDataList* m_pExtUserDataList;
    char mPanelName[0x19];
    char mUserData[9];

private:
    /** @return The child list of the pane. */
    PaneList& GetChildList() { return *reinterpret_cast<PaneList*>(&m_Children); }

    const void* FindSystemExtData(PaneSystemDataType type) const;
    const void* GetSystemExtDataByTypeUnchecked(PaneSystemDataType type) const;
    detail::PaneEffect* GetPaneEffectInstanceUnchecked() const;
};

namespace detail {
void CalculateCaptureRootMatrix(nn::util::MatrixT4x3fType& rMtx, const DrawInfo& rDrawInfo);
}  // namespace detail
}  // namespace nn::ui2d
