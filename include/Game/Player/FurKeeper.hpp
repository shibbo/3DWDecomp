#pragma once

#include <basis/seadTypes.h>
#include <container/seadObjArray.h>
#include <container/seadPtrArray.h>

namespace al {
class LiveActor;
}

class FurShape;

/**
 * Holds the shell-fur shapes of an actor, read from the actor's "InitFur" init file.
 */
class FurKeeper {
public:
    /**
     * Fur settings of one shape, as read from the "InitFur" init file.
     */
    struct FurParam {
        const char* shapeName = nullptr;
        const char* diameterTextureName = nullptr;
        s32 layerNum = 5;
        f32 height = 4.0f;
        f32 startAlpha = 1.0f;
        f32 endAlpha = 0.7f;
    };

    static_assert(sizeof(FurParam) == 0x20);

    FurKeeper();

    void init(al::LiveActor* pActor, const char* pName);
    void updateUbo();
    void draw() const;
    FurShape* find(const char* pShapeName) const;
    s32 getShapeNum() const;
    FurShape* getShape(u32 index) const;

private:
    sead::ObjArray<FurParam> mParams;
    sead::PtrArray<FurShape> mShapes;
};

static_assert(sizeof(FurKeeper) == 0x30);
