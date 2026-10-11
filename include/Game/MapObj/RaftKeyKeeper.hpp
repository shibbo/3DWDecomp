#pragma once
#include <container/seadBuffer.h>
#include <math/seadQuat.h>
#include <math/seadVector.h>
namespace al { class ActorInitInfo; struct PlacementInfo; }
class RaftKeyKeeper {
public:
    RaftKeyKeeper();
    void init(const al::ActorInitInfo&);
    int findNextKeyIndex(float) const;
    void calcPosAndQuatByKeyIndex(sead::Vector3f*, sead::Quatf*, int);
    void calcClippingSphere(sead::Vector3f*, float*, float);
    int getKeyCount() const { return mKeyCount; }
    float getTotalLength() const { return mTotalLength; }
    float getAppearEndCoord() const { return mAppearEndCoord; }
private:
    struct Key {
        float axisCoord = 0.0f;
        float pathCoord = 0.0f;
        sead::Vector3f position = sead::Vector3f(0.0f, 0.0f, 0.0f);
        sead::Quatf rotation = sead::Quatf::unit;
        sead::Vector3f offset = sead::Vector3f(1.0f, 0.0f, 0.0f);
        int moveMode = -1;
        float moveSpeed = 3.0f;
        float moveAccel = 0.0f;
        float gravityPower = 0.0f;
    };
    void initKey(Key*, const al::PlacementInfo&);
    sead::Buffer<Key> mKeys;
    int mKeyCount = 0;
    sead::Quatf mRotation = sead::Quatf::unit;
    sead::Vector3f mOrigin = sead::Vector3f(0.0f, 0.0f, 0.0f);
    sead::Vector3f mAxis = sead::Vector3f::ez;
    float mTotalLength = 0.0f;
    float mAppearEndCoord = 0.0f;
};
static_assert(sizeof(RaftKeyKeeper) == 0x48);
