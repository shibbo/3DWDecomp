#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
class GuideObj;
class GuideObjHolder : public al::ISceneObj {
public:
    GuideObjHolder();
    const char* getSceneObjName() const override;
    void registerGuideObj(GuideObj* pObj);
    void startPause();
    void endPause();

private:
    sead::PtrArray<GuideObj> mObjects;
};
static_assert(sizeof(GuideObjHolder) == 0x18);
