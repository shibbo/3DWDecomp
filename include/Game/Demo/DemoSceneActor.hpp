#pragma once

#include "Library/LiveActor/LiveActor.hpp"
#include "attributes.h"

class DemoActionList;
class DemoSceneActorHolder;

/** @brief A model actor participating in a demo scene. */
class DemoSceneActor : public al::LiveActor {
public:
    DemoSceneActor(const char* pName, DemoSceneActorHolder* pHolder);
    void init(const al::ActorInitInfo& rInfo) override;
    virtual void initDemoSceneActor(const al::ActorInitInfo& rInfo,
        const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pMtx);
    virtual void startDemo(const sead::Matrix34f& rMtx);
    void setPlacementBaseMtx(const sead::Matrix34f* pMtx);
    void startAction(int index);
    bool tryStartActionByName(const char* pName);
    void endDemo();
    void exeWait();

private:
    friend class DemoSceneActorHolder;
    DemoSceneActorHolder* mHolder;
    DemoActionList* mActions = nullptr;
    sead::Matrix34f mPlacementMtx;
    sead::Matrix34f mLocalMtx;
    bool mWorldColor = false;
    bool mIsKoopaJr = false;
    int mColorFrame = -1;
};

static_assert(sizeof(DemoSceneActor) == 0x1c0);

namespace DemoSceneActorFunction {
NOINLINE void calcPlacementBaseMtx(sead::Matrix34f* pOut, const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx, sead::Matrix34f* pLocalMtx);
NOINLINE bool isHideAction(const char* pName);
NOINLINE bool isShowAction(const char* pName);
}
