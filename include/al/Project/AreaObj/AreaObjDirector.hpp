#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

namespace al {
class AreaInitInfo;
class AreaObj;
class AreaObjFactory;
class AreaObjGroup;
class AreaObjMtxConnecterHolder;
class Scene;
class SceneObjHolder;

class AreaObjDirectorGrid {
public:
    __attribute__((noinline)) AreaObjDirectorGrid(s32, s32);

    void expandGrid(AreaObj*);
    void endInit();

    s32 _0;
    s32 _4;
    void* _8;
    sead::Vector2f mMin = {3.4028235e38f, 3.4028235e38f};
    sead::Vector2f mMax = {-3.4028235e38f, -3.4028235e38f};
};

class AreaObjDirector {
public:
    AreaObjDirector(bool isUseGrid);

    void init(const AreaObjFactory* pFactory);
    void endInit();
    void update();
    void placement(const AreaInitInfo& rInfo);
    void placement(const AreaInitInfo* pInfos, s32 num, const SceneObjHolder* pHolder,
                   Scene* pScene);
    void createAreaObjGroup(const AreaInitInfo& rInfo);
    void createAreaObjGroupBuffer();
    void placementAreaObj(const AreaInitInfo& rInfo, const SceneObjHolder* pHolder, Scene* pScene);
    s32 getTotalAreaObjs() const;
    AreaObjGroup* getAreaObjGroup(const char* pName) const;
    bool isExistAreaGroup(const char* pName);
    void addToExtraAreaGroup(AreaObj* pAreaObj);
    AreaObj* tryFindInExtraAreaObjGroup(const sead::Vector3f& rPos);
    void setEnableAll(bool isEnable);
    AreaObj* getInVolumeAreaObj(const char* pName, const sead::Vector3f& rPos);
    AreaObj* getInVolumeAreaObj(const char* pName, const sead::Vector3f& rStart,
                                const sead::Vector3f& rEnd, sead::Vector3f* pHitPos,
                                sead::Vector3f* pNormal);
    AreaObjMtxConnecterHolder* getMtxConnecterHolder() const;
    s32 getAreaObjGroupIndex(const char* pName) const;

    const AreaObjFactory* mFactory = nullptr;
    AreaObjMtxConnecterHolder* mMtxConnecterHolder = nullptr;
    AreaObjGroup** mAreaGroups = nullptr;
    s32 mAreaGroupCount = 0;
    AreaObj* mExtraAreaObjs[100];
    s32 mExtraAreaObjCount = 0;
    AreaObjDirectorGrid* mGrid;
};
}  // namespace al
