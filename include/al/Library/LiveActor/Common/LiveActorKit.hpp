#pragma once

#include <basis/seadTypes.h>

namespace al {
class AreaObjDirector;
class CameraDirector;
class CameraDirector_RS;
class ClippingAreaDirector;
class ClippingDirectorBase;
class DemoDirector;
class IScenarioCompleteChecker;
class ItemDirectorBase;
class ShadowDirector;
struct GraphicsInitArg;
class CollisionDirector;
class EffectSystem;
class ExecuteDirector;
class GraphicsSystemInfo;
class HitSensorDirector;
class LiveActorGroup;
class MultiCoreQueueThread;
class PadRumbleDirector;
class PlayerHolder;
class ScreenPointDirector;
class StageSwitchDirector;
class SwitchAreaDirector;

class LiveActorKit {
public:
    LiveActorKit(s32, s32, bool, bool);
    ~LiveActorKit();

    void init(s32, s32, bool);
    void initGraphics(const GraphicsInitArg& rArg, const char* pStageName);
    void initHitSensorDirector(s32, bool);
    void initShadowDirector();
    void initEffectSystem();
    void endInit(IScenarioCompleteChecker* pChecker);
    void updateReducedBufferEffect();
    void setupCameraAreaObjDirector();
    void update();
    void clearGraphicsRequest();
    void updateGraphics(bool isPaused);
    bool preDrawGraphics();
    void updatePadRumble();

    AreaObjDirector* getAreaObjDirector() const { return mAreaObjDirector; }
    ExecuteDirector* getExecuteDirector() const { return mExecDirector; }
    EffectSystem* getEffectSystem() const { return mEffectSystem; }
    GraphicsSystemInfo* getGraphicsSystemInfo() const { return mGraphicsSystemInfo; }
    CameraDirector* getCameraDirector() const { return mCameraDirector; }
    CameraDirector_RS* getCameraDirector_RS() const { return mCameraDirectorRS; }
    ClippingDirectorBase* getClippingDirector() const { return mClippingDirector; }
    CollisionDirector* getCollisionDirector() const { return mCollisionDirector; }
    ItemDirectorBase* getItemDirector() const { return mItemDirector; }
    PlayerHolder* getPlayerHolder() const { return mPlayerHolder; }
    HitSensorDirector* getHitSensorDirector() const { return mSensorDirector; }
    ScreenPointDirector* getScreenPointDirector() const { return mScreenPointDirector; }
    ShadowDirector* getShadowDirector() const { return mShadowDirector; }
    StageSwitchDirector* getStageSwitchDirector() const { return mStageSwitchDirector; }
    SwitchAreaDirector* getSwitchAreaDirector() const { return mSwitchAreaDirector; }
    LiveActorGroup* getActorGroup() const { return mActorGroup; }
    DemoDirector* getDemoDirector() const { return mDemoDirector; }
    PadRumbleDirector* getPadRumbleDirector() const { return mRumbleDirector; }
    void setItemDirector(ItemDirectorBase* pDirector) { mItemDirector = pDirector; }
    void setDemoDirector(DemoDirector* pDirector) { mDemoDirector = pDirector; }

    s32 _0;
    s32 _4;
    AreaObjDirector* mAreaObjDirector;
    ExecuteDirector* mExecDirector;
    EffectSystem* mEffectSystem;
    GraphicsSystemInfo* mGraphicsSystemInfo;
    CameraDirector* mCameraDirector;
    CameraDirector_RS* mCameraDirectorRS;
    ClippingDirectorBase* mClippingDirector;
    CollisionDirector* mCollisionDirector;
    ItemDirectorBase* mItemDirector;
    PlayerHolder* mPlayerHolder;
    HitSensorDirector* mSensorDirector;
    ScreenPointDirector* mScreenPointDirector;
    ShadowDirector* mShadowDirector;
    StageSwitchDirector* mStageSwitchDirector;
    SwitchAreaDirector* mSwitchAreaDirector;
    LiveActorGroup* mActorGroup;
    DemoDirector* mDemoDirector;
    PadRumbleDirector* mRumbleDirector;
    MultiCoreQueueThread* mQueueThread;

    bool _a0;
    bool _a1;
};
}  // namespace al
