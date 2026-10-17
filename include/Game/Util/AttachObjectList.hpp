#pragma once

#include <container/seadObjArray.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>

namespace al {
class ActorInitInfo;
class LiveActor;
}

namespace rc {
class AttachObjectList {
public:
    void init(const al::ActorInitInfo& rInfo, bool isDead);
    void syncObjectsToPosition(const sead::Vector3f& rPosition);
    void syncObjectsToPositionWithRotate(const sead::Vector3f& rPosition,
                                         const sead::Quatf& rRotate);
    int getObjectNum() const { return mObjects.size(); }
    al::LiveActor* getActor(int index) const { return mObjects.unsafeAt(index)->actor; }

private:
    struct Object {
        sead::Vector3f offset;
        al::LiveActor* actor;
    };
    sead::ObjArray<Object> mObjects;
};
static_assert(sizeof(AttachObjectList) == 0x20);
}
