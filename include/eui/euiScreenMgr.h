#pragma once
#include <heap/seadDisposer.h>
#include <hostio/seadHostIONode.h>
#include <container/seadBuffer.h>
#include <container/seadSafeArray.h>
#include <nn/ui2d/ui2d_GraphicsResource.h>
#include <nn/ui2d/ui2d_ControlCreator.h>
#include <eui/euiSharcArchive.h>
#include <eui/euiDrawInfoEx.h>
#include <eui/euiConstantBuffer.h>
namespace eui {
class Screen;
class ArcResourceMgr;
class BoxCursorMgr;
class FontMgr;
class BoxCursorNode;
class MessageMgr;
class ScreenFactory;
class ScreenViewer;
class ScreenMgr : public sead::hostio::Node {
    SEAD_SINGLETON_DISPOSER(ScreenMgr);
public:
    struct InitializeArg {
        sead::Heap* heap = nullptr;
        ScreenFactory* screenFactory = nullptr;
        ArcResourceMgr* arcResourceMgr = nullptr;
        void* _18 = nullptr;
        void* multiFilterArchiveData = nullptr;
        u32 multiFilterArchiveSize = 0;
        u8 _2c = 10;
        MessageMgr* messageMgr = nullptr;
        FontMgr* fontMgr = nullptr;
        u32 _40 = 0x200;
        ConstantBuffer::InitConfig constantBufferConfig;
    };

    ScreenMgr();
    void initialize(const InitializeArg& rArg);
    virtual ~ScreenMgr();
    void updateViewer_();
    void inactivateScreen(int index);
    void activateScreen(int index);
    void updateSystem();
    void updateScreen(s8 drawUnitId);
    void draw(s8 layer, const DrawInfoEx::RenderBufferInfo* pInfo);
    void unloadScreen(int index);
    void resetScreenId(int index);
    void eraseBoxCursorNodeFromRouteNodes(const BoxCursorNode* pNode);
    Screen* findScreenByName(const char* pName);
    const void* getMultiFilterParameterData(const sead::SafeString& rName) const;

    BoxCursorMgr* getBoxCursorMgr() const { return mBoxCursorMgr; }
    FontMgr* getFontMgr() const { return mFontMgr; }
    MessageMgr* getMessageMgr() const { return static_cast<MessageMgr*>(_430); }
    float getAnimationStep() const { return mAnimationStep; }
    ConstantBuffer* getConstantBuffer() const { return static_cast<ConstantBuffer*>(_448); }
    ScreenViewer* getViewer() const { return static_cast<ScreenViewer*>(_48); }
    ArcResourceMgr* getArcResourceMgr() const { return mArcResourceMgr; }

    /** @return Per-display flags that allow screens to receive pointer hits. */
    sead::SafeArray<bool, 2>& getHitEnableFlags() {
        return *reinterpret_cast<sead::SafeArray<bool, 2>*>(&_440);
    }

    sead::Buffer<Screen*> mScreens;
    sead::Buffer<s8> mScreenLayers;
    void* _48;
    nn::ui2d::GraphicsResource mGraphicsResource;
    sead::hostio::Node mHostIONode;
    ArcResourceMgr* mArcResourceMgr;
    BoxCursorMgr* mBoxCursorMgr;
    float mAnimationStep;
    SharcArchive mArchive;
    void* _430;
    FontMgr* mFontMgr;
    bool _440, _441, _442;
    void* _448;
};

static_assert(sizeof(nn::ui2d::GraphicsResource) == 0x3b8, "GraphicsResource size");
static_assert(sizeof(ScreenMgr) == 0x450, "ScreenMgr size");
}
