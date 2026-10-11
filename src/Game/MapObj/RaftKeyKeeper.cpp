#include "MapObj/RaftKeyKeeper.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Math/MathUtil.hpp"
RaftKeyKeeper::RaftKeyKeeper() {}
inline void RaftKeyKeeper::initKey(Key* key, const al::PlacementInfo& info) {
    al::tryGetTrans(&key->position, info);
    sead::Vector3f offset = key->position - mOrigin;
    key->axisCoord = offset.dot(mAxis);
    key->pathCoord = 0.0f;
    al::tryGetArg(&key->moveMode, info, "MoveMode");
    al::tryGetQuat(&key->rotation, info);
    al::verticalizeVec(&key->offset, mAxis, offset);
    al::tryGetArg(&key->moveSpeed, info, "MoveSpeed");
    al::tryGetArg(&key->moveAccel, info, "MoveAccel");
    al::tryGetArg(&key->gravityPower, info, "GravityPower");
}
void RaftKeyKeeper::init(const al::ActorInitInfo& info) {
    al::tryGetQuat(&mRotation, info);
    al::tryGetTrans(&mOrigin, info);
    int axis = 2;
    al::tryGetArg(&axis, info, "MoveAxis");
    al::tryGetLocalAxis(&mAxis, info, axis);
    al::verticalizeVec(&mAxis, sead::Vector3f::ey, mAxis);
    al::normalizeOrZero(&mAxis);
    mKeyCount = al::calcLinkNestNum(info, "KeyMoveNext") + 1;
    mKeys.tryAllocBuffer(mKeyCount, nullptr);
    al::PlacementInfo current = *info.mPlacementInfo;
    current = *info.mPlacementInfo;
    initKey(&mKeys[0], current);
    al::PlacementInfo next;
    for (int i = 0; i < mKeyCount - 1; ++i) {
        al::getLinksInfo(&next, current, "KeyMoveNext");
        initKey(&mKeys[i + 1], next);
        current = next;
    }
    mTotalLength = 0.0f;
    for (int i = 1; i < mKeyCount; ++i) {
        Key& nextKey = mKeys[i];
        float distance = nextKey.axisCoord - mKeys[i - 1].axisCoord;
        mTotalLength += distance > 0.0f ? distance : -distance;
        nextKey.pathCoord = mTotalLength;
    }
    mAppearEndCoord = mTotalLength;
    for (int i = 0; i < mKeyCount; ++i) {
        if (mKeys[i].moveMode == 3) {
            mAppearEndCoord = mKeys[i].pathCoord;
            break;
        }
    }
}
int RaftKeyKeeper::findNextKeyIndex(float coord) const {
    for (int i = 0; i < mKeyCount; ++i) if (mKeys[i].pathCoord > coord) return i;
    return 0;
}
void RaftKeyKeeper::calcPosAndQuatByKeyIndex(sead::Vector3f* pos, sead::Quatf* quat, int index) {
    Key key;
    key = mKeys[index];
    if (pos) *pos = key.offset + (key.axisCoord * mAxis + mOrigin);
    if (quat) quat->set(mKeys[index].rotation);
}
void RaftKeyKeeper::calcClippingSphere(sead::Vector3f* center, float* radius, float partRadius) {
    calcPosAndQuatByKeyIndex(center, nullptr, 0);
    *radius = partRadius;
    for (int i = 1; i < mKeyCount; ++i) {
        sead::Vector3f pos;
        calcPosAndQuatByKeyIndex(&pos, nullptr, i);
        al::calcSphereMargeSpheres(center, radius, *center, *radius, pos, partRadius);
    }
}
