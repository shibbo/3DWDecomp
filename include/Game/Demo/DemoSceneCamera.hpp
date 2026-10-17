#pragma once

#include "Project/Camera/Core/IUseCameraDirector.hpp"
#include "Project/Camera/Main/IUseCameraDirector_RS.hpp"
#include <math/seadMatrix.h>

namespace al { class ActorInitInfo; class CameraInfo; class CameraTicket; }
class GameDataHolder;
class DemoActionList;

class DemoSceneCamera : public al::IUseCamera, public al::IUseCamera_RS {
public:
    explicit DemoSceneCamera(const GameDataHolder* pHolder);
    al::SceneCameraInfo* getSceneCameraInfo() const override;
    al::CameraDirector_RS* getCameraDirector_RS() const override;
    virtual void initDemoSceneActor(const al::ActorInitInfo& rInfo, const al::ActorInitInfo& rCameraInfo, const sead::Matrix34f* pMtx);
    virtual void startAction(int index);
    virtual void tryStartActionByName(const char* pName);
    virtual void startDemo(const sead::Matrix34f& rMtx);
    virtual void endDemo();
    void setInterpolateFrame(int frames);
    void startCamera(int index);
    bool isEndAnimCamera(int index) const;
    void setPlacementBaseMtx(const sead::Matrix34f* pMtx);
    void endCameraAll();
    void endCamera(int index);
    int getMaxFrame() const;
    int getMaxFrame(int index) const;
    int getCurrentFrame() const;
    void setCurrentFrame(int frame);
    void setCameraFovyDegree(float degrees);

private:
    DemoActionList* mActions = nullptr;
    sead::Matrix34f mPlacementMtx;
    sead::Matrix34f mLocalMtx;
    const GameDataHolder* mGameDataHolder;
    al::CameraInfo* mCameraInfo = nullptr;
    al::CameraTicket* mCameraTicket = nullptr;
    int mActionCount = 0;
    sead::Matrix34f mDemoMtx;
    al::SceneCameraInfo* mSceneCameraInfo = nullptr;
    al::CameraDirector_RS* mCameraDirector = nullptr;
    int mInterpolateFrames = 0;
    float mFovyDegree = -1.0f;
    bool mUseFovAnim = false;
};
