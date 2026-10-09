#pragma once
#include <nn/font/font_GpuBuffer.h>
#include <nn/font/font_Util.h>
#include <nn/types.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_ResourceAccessor.h>
namespace nn::ui2d {
class Screen;
class GraphicsResource;
class ArcResourceMgr;
class FontMgr;
// Partial interface for existing instances; virtual extension hooks remain unreconstructed.
class ScreenManager {
public:
    struct ConstantBufferInitializeArgs {
        ConstantBufferInitializeArgs();
        size_t size;
        bool memoryPoolRequired;
    };
    NN_RUNTIME_TYPEINFO_BASE();
    virtual ~ScreenManager();
    // The names of the pure virtual functions are not known.
    virtual void PureVirtual18_() = 0;
    virtual void PureVirtual20_() = 0;
    virtual const char* GetBodyLayoutName(int index) const;
    virtual void PureVirtual30_() = 0;
    virtual int GetScreenDrawUnitId(int index);
    virtual bool GetScreenIsTouch(int index);
    virtual const char* GetPreviewBodyLayoutName() const;
    void RegisterScreen_(Screen* screen, int index);
    int FindScreenId(Screen* screen) const;
    void UnregisterScreenById_(int index);
    void ResetScreenId(int index);
    void InactivateScreen(int index);
    void ActivateScreen(int index);
    u8 _08[0x8];
    const GraphicsResource* m_pGraphicsResource;
    const ArcResourceMgr* m_pArcResourceMgr;
    const FontMgr* m_pFontMgr;
    RegisterTextureView m_pRegisterTextureViewFunction;
    u8 _30[0x8];
    void* m_pRegisterTextureViewUserData;
    u8 _40[0x18];
    Size m_ViewportSize;
    u8 _60[0x20];
    nn::font::GpuBuffer m_ConstantBuffer;
    u8 _80[0x180 - sizeof(nn::font::GpuBuffer)];
    Screen* mScreens[8];
    int mActiveIds[8];
};
static_assert(sizeof(ScreenManager) == 0x260, "ScreenManager size");
}
