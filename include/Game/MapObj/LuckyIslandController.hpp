#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "MapObj/DisasterModeController.hpp"
#include <container/seadObjArray.h>
class LuckyIslandHolder;
class LuckyIslandController : public al::ISceneObj, public al::LiveActor, public DisasterModeStateListener {
public:
    explicit LuckyIslandController(const char* name);
    ~LuckyIslandController() override;
    void init(const al::ActorInitInfo&) override;
    void initAfterPlacement() override;
    void onDisasterModeStateChange(DisasterModeController::State) override;
    void setActiveIsland();
    const sead::Vector3f* getClosestCamera(const sead::Vector3f&);
private:
    LuckyIslandHolder* mHolder = nullptr;
    bool mSelectIsland = true;
    sead::ObjArray<sead::Vector3f> mCameraPositions;
};
static_assert(sizeof(LuckyIslandController) == 0x188);
