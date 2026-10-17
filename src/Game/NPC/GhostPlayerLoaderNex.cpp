#include "NPC/GhostPlayerLoaderNex.hpp"
#include "NPC/GhostPlayerDisplayInfo.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include <prim/seadStringUtil.h>
#include <basis/seadNew.h>
#include <cstring>
#include "Project/Base/StringUtil.hpp"
#include "Library/Math/MathUtil.hpp"
#include <math/seadQuat.hpp>

GhostPlayerLoaderNex::GhostPlayerLoaderNex(const char* name, al::NexDataStoreAccessor* accessor,
                                         const al::NetworkSystem* network)
    : GhostPlayerLoaderBase(name), mAccessor(accessor), mNetworkSystem(network) {
    int count = rc::getControlUserNumMax() - 1;
    mGhostData.allocBuffer(count, nullptr);
    mDisplayInfo.allocBuffer(count, nullptr);
    for (int i = 0; i < mDisplayInfo.capacity(); ++i)
        mDisplayInfo.pushBack(new GhostPlayerDisplayInfo());
}
bool GhostPlayerLoaderNex::isExistParamByOwnerId(int) const { return false; }
bool GhostPlayerLoaderNex::load(u8**, u32, int, int) { return false; }
void GhostPlayerLoaderNex::extractGhostDataUserName(sead::BufferedSafeString* out, int index) const {
    al::StringTmp<64> name;
    const GhostPlayerDisplayInfo* info = mDisplayInfo.at(index);
    sead::StringUtil::convertUtf16ToSjis(name.getBuffer(), name.getBufferSize(), info->mUserName.cstr(), -1);
    al::copyString(out->getBuffer(), name.cstr(), out->getBufferSize());
}
const GhostPlayerDisplayInfo* GhostPlayerLoaderNex::getDisplayInfo(int index) const {
    return mDisplayInfo.at(index);
}
int GhostPlayerLoaderNex::getGhostPlayDataNum() const { return mDisplayInfo.size(); }
bool GhostPlayerLoaderNex::isNeedReadBuffer() const { return false; }

namespace GhostPlayerFunction {
namespace {
struct Header {
    u32 version;
    int warpCount;
    int actionCount;
    u32 poseCount;
    u32 warpOffset, actionOffset, poseOffset;
    int markerCount;
    u32 markerOffset;
};
}
bool isValidGhostPlayerDataVersion(const void* data) {
    return static_cast<const Header*>(data)->version - 1 < 2;
}
const WarpObjData* getWarpObjData(const void* data, int index) {
    return reinterpret_cast<const WarpObjData*>(reinterpret_cast<const u8*>(static_cast<const Header*>(data)->warpOffset + reinterpret_cast<uintptr_t>(data))) + index;
}
int getWarpObjDataNum(const void* data) { return static_cast<const Header*>(data)->warpCount; }
const char* getGhostPlayActionData(const void* data, int frame) {
    const Header* header = static_cast<const Header*>(data);
    int oddIndex = static_cast<int>(header->poseCount) > (frame + 1) / 2 ? (frame + 1) / 2 : (frame - 1) / 2;
    int index = frame & 1 ? oddIndex : frame / 2;
    const u16* actions = reinterpret_cast<const u16*>(header->poseOffset + reinterpret_cast<uintptr_t>(data));
    const char* names = reinterpret_cast<const char*>(header->actionOffset + reinterpret_cast<uintptr_t>(data));
    return names + actions[header->poseCount * 11ull + index] * 32;
}
int getGhostPlayDataNum(const void* data) { return static_cast<const Header*>(data)->poseCount; }
const char* getGhostPlayActionDataRewind(const void* data, int frame) {
    const Header* header = static_cast<const Header*>(data);
    int index = frame / 2;
    if (frame & 1) index = (frame < 3 ? frame + 1 : frame - 1) / 2;
    if (frame / 2 >= static_cast<int>(header->poseCount)) index = header->poseCount - 1;
    const u16* actions = reinterpret_cast<const u16*>(header->poseOffset + reinterpret_cast<uintptr_t>(data));
    const char* names = reinterpret_cast<const char*>(header->actionOffset + reinterpret_cast<uintptr_t>(data));
    return names + actions[header->poseCount * 11ull + index] * 32;
}
int getGhostPlayActionDataNum(const void* data) { return static_cast<const Header*>(data)->actionCount; }
const GhostRecorderPoseData* getGhostPlayData(const void* data, int index) {
    return reinterpret_cast<const GhostRecorderPoseData*>(reinterpret_cast<const u8*>(static_cast<const Header*>(data)->poseOffset + reinterpret_cast<uintptr_t>(data))) + index;
}
const GhostRecorderMarkerData* getMarkerData(const void* data, int index) {
    const Header* header = static_cast<const Header*>(data);
    if (header->version != 2) return nullptr;
    return reinterpret_cast<const GhostRecorderMarkerData*>(reinterpret_cast<const u8*>(header->markerOffset + reinterpret_cast<uintptr_t>(data))) + index;
}
int getMarkerDataNum(const void* data) {
    const Header* header = static_cast<const Header*>(data);
    return header->version == 2 ? header->markerCount : 0;
}

void calcGhostPlayDataTrans(sead::Vector3f* out, const void* data, int frame, float maxDistance) {
    const Header* header = static_cast<const Header*>(data);
    const float* values = reinterpret_cast<const float*>(reinterpret_cast<const u8*>(header->poseOffset + reinterpret_cast<uintptr_t>(data)));
    if (!(frame & 1)) {
        values += frame / 2;
        out->x = values[0];
        out->y = values[header->poseCount];
        out->z = values[header->poseCount * 2ull];
        return;
    }
    int prev = (frame - 1) / 2;
    sead::Vector3f a;
    memcpy(&a.x, values + prev, sizeof(float));
    memcpy(&a.y, values + header->poseCount + prev, sizeof(float));
    memcpy(&a.z, values + header->poseCount * 2ull + prev, sizeof(float));
    int next = (frame + 1) / 2;
    if (static_cast<int>(header->poseCount) > next) {
        sead::Vector3f b;
        memcpy(&b.x, values + next, sizeof(float));
        memcpy(&b.y, values + header->poseCount + next, sizeof(float));
        memcpy(&b.z, values + header->poseCount * 2ull + next, sizeof(float));
        if ((a - b).length() > maxDistance) memcpy(out, &b, sizeof(b));
        else *out = (a + b) * 0.5f;
    } else memcpy(out, &a, sizeof(a));
}
void calcGhostPlayDataRotate(sead::Vector3f* out, const void* data, int frame) {
    const Header* header = static_cast<const Header*>(data);
    const s16* values = reinterpret_cast<const s16*>(reinterpret_cast<const u8*>(header->poseOffset + reinterpret_cast<uintptr_t>(data))
        + header->poseCount * 16ull);
    if (!(frame & 1)) {
        values += frame / 2;
        out->x = values[0];
        out->y = values[header->poseCount];
        out->z = values[header->poseCount * 2ull];
        return;
    }
    int prev = (frame - 1) / 2;
    sead::Vector3f a(values[prev], (values + header->poseCount)[prev], (values + header->poseCount * 2ull)[prev]);
    int next = (frame + 1) / 2;
    if (static_cast<int>(header->poseCount) > next) {
        sead::Vector3f b(values[next], (values + header->poseCount)[next], (values + header->poseCount * 2ull)[next]);
        sead::Quatf qa, qb, result;
        a *= sead::Mathf::deg2rad(1.0f);
        qa.setRPY(a.x, a.y, a.z);
        b *= sead::Mathf::deg2rad(1.0f);
        qb.setRPY(b.x, b.y, b.z);
        result.slerpTo(qa, qb, 0.5f);
        sead::Vector3f rotate;
        result.calcRPY(rotate);
        out->x = sead::Mathf::rad2deg(rotate.x);
        out->y = sead::Mathf::rad2deg(rotate.y);
        out->z = sead::Mathf::rad2deg(rotate.z);
    } else *out = a;
}
void calcGhostPlayDataSklAnimFrame(float* out, const void* data, int frame, bool holdOnLoop) {
    const Header* header = static_cast<const Header*>(data);
    const float* values = reinterpret_cast<const float*>(reinterpret_cast<const u8*>(header->poseOffset + reinterpret_cast<uintptr_t>(data))
        + header->poseCount * 12ull);
    if (!(frame & 1)) { *out = values[frame / 2]; return; }
    u32 prev = static_cast<int>(values[(frame - 1) / 2]);
    int nextIndex = (frame + 1) / 2;
    if (static_cast<int>(header->poseCount) > nextIndex) {
        u32 next = static_cast<int>(values[nextIndex]);
        if (next >= prev) { *out = static_cast<int>(prev + next) * 0.5f; return; }
        if (!holdOnLoop) { *out = next * 0.5f; return; }
    }
    *out = prev;
}
void calcGhostPlayDataActionName(const char** out, const void* data, int frame) {
    const Header* header = static_cast<const Header*>(data);
    if (!(frame & 1)) { *out = getGhostPlayActionData(data, frame); return; }
    int next = (frame + 1) / 2;
    const float* anim = reinterpret_cast<const float*>(reinterpret_cast<const u8*>(header->poseOffset + reinterpret_cast<uintptr_t>(data))
        + header->poseCount * 12ull);
    if (next < static_cast<int>(header->poseCount) && !al::isNearZero(anim[next], 0.001f))
        *out = getGhostPlayActionData(data, frame + 1);
    else *out = getGhostPlayActionData(data, frame - 1);
}
void calcGhostPlayDataActionNameRewind(const char** out, const void* data, int frame) {
    const Header* header = static_cast<const Header*>(data);
    if (!(frame & 1)) { *out = getGhostPlayActionDataRewind(data, frame); return; }
    if (frame == -1 || frame == 1) { *out = getGhostPlayActionDataRewind(data, 0); return; }
    const float* anim = reinterpret_cast<const float*>(reinterpret_cast<const u8*>(header->poseOffset + reinterpret_cast<uintptr_t>(data))
        + header->poseCount * 12ull);
    al::isNearZero(anim[(frame - 1) / 2], 0.001f);
    *out = getGhostPlayActionDataRewind(data, frame - 1);
}
}
