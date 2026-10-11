#pragma once
#include "Library/Layout/LayoutActor.hpp"
#include <math/seadVector.h>
namespace al { class LayoutInitInfo; }
class CollectNumber : public al::LayoutActor {
public:
    CollectNumber(const al::LayoutInitInfo&, const char*, const char*);
    void appear() override;
    void appearNormal(const sead::Vector3f*, int);
    void appearNormal(const sead::Vector3f&, int);
    void appearComplete(const sead::Vector3f*, int);
    void appearComplete(const sead::Vector3f&, int);
    void exeAppear();
    bool hasAppeared() const { return mHasAppeared; }
    al::SceneCameraInfo* getSceneCameraInfo() const override;
private:
    al::SceneCameraInfo* mSceneCameraInfo;
    bool mHasAppeared;
    const sead::Vector3f* mFollowPosition;
    sead::Vector3f mPosition;
};
