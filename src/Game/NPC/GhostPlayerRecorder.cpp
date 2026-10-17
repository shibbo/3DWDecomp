#include "NPC/GhostPlayerRecorder.hpp"
#include "MapObj/CheckpointFlag.hpp"
#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerPlayer.hpp"
#include "NPC/GhostPlayerUserInfo.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataConst.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/PlayerUtil.hpp"
#include <cstring>
#include <stream/seadRamStream.h>

namespace {
const GhostPlayerUserInfo sGhostPlayerUserInfo[] = {
    {"n0612", "\343\201\223\343\202\223\343\201\251\343\201\206", true},
    {"n0692", "\343\203\221\343\203\210\343\203\252\343\202\252\343\203\203\343\203\210", false},
    {"n1047", "\343\203\204\343\203\212\343\203\236\343\203\250", false},
    {"n1269", "\357\274\251\357\275\224\342\200\231\357\275\223\343\202\264\343\203\274\343\203\253\343\203\211", false},
    {"n1296", "\343\203\217\343\200\200\343\203\253\343\200\200\343\202\255", false},
    {"n1390", "\357\275\220\357\275\217\357\275\216\357\275\231\357\275\217", false},
    {"n1468", "\343\203\224\343\202\253\343\203\201\343\203\245\343\203\274", false},
    {"n1489", "\343\202\215\343\201\234\343\201\243\343\201\237\343\200\202", false},
    {"n1507", "\343\201\277\343\201\255\343\201\216\343\201\227", true},
    {"n1573", "\343\201\257\343\202\204\343\201\213\343\202\217", true},
    {"n1585", "\343\201\246\343\201\245\343\201\213", false},
    {"n1600", "\343\201\213\343\201\232\343\201\277", true},
    {"n1605", "\343\201\227\343\202\211\343\201\204", true},
    {"n1640", "\343\201\257\343\202\204\343\201\227\343\201\240", true},
    {"n1672", "\343\201\217\343\202\215\343\201\255\343\201\223", false},
    {"n1704", "\343\203\223\343\203\243\343\203\223\343\203\247\343\203\274\343\203\263", false},
    {"n1981", "\357\274\244\357\274\251\357\274\243\357\274\245", false},
    {"n1985", "\343\202\210\343\201\223\343\201\237", true},
    {"n1986", "\343\201\202\343\201\244\343\201\227", true},
    {"n1993", "\357\274\263\357\275\210\357\275\211\357\275\216", false},
    {"N2034", "\343\203\237\343\202\257\343\203\201\343\203\274", false},
    {"N2056", "\343\202\267\343\203\274\343\203\226\343\203\252\343\203\274\343\202\272", false},
    {"n2080", "\357\274\247\357\274\223\357\274\220", false},
    {"n2081", "\343\201\202\343\201\212\343\202\204\343\201\216", true},
    {"n2093", "\343\201\276\343\201\244\343\201\240", true},
    {"n2125", "\343\202\210\343\201\227\343\201\240", true},
    {"n2154", "\343\201\204\343\201\247", true},
    {"n2156", "\357\274\256\357\274\257\357\274\262\357\274\251", false},
    {"n2185", "\343\202\263\343\203\274\343\202\270", false},
    {"n2186", "\343\202\217\343\201\237\343\201\252\343\201\271", true},
    {"n2187", "\357\274\255\357\274\257\357\274\255\357\274\257(^\317\211^)\342\231\252", false},
    {"n2229", "\343\201\253\343\201\256\343\201\277\343\202\204\343\201\213\343\201\232\343\201\252\343\202\212", false},
    {"N2239", "\343\201\215\343\201\240", true},
    {"n2240", "\343\201\224\343\201\206\343\201\257\343\202\211", true},
    {"n2305", "\343\201\202\343\201\213\343\201\264\343\201\217", false},
    {"n2349", "\357\274\243\357\275\210\357\275\201\357\275\222\357\275\222\357\275\231", false},
    {"n2433", "\343\201\277\343\201\255\343\201\237", true},
    {"n2476", "\343\202\200\343\201\225", true},
    {"n2502", "\343\201\276\343\203\274\343\201\241\343\202\203\343\202\223", false},
    {"n2505", "\343\202\200\343\202\211\343\201\237", true},
    {"n2506", "\343\201\244\343\201\230", true},
    {"n2606", "\343\203\253\343\203\274\343\203\222\343\203\274", false},
    {"n2639", "\343\201\255\343\201\223\343\201\253\343\202\223", false},
    {"n2642", "\343\201\256\343\202\223\343\201\271\343\203\274", false},
    {"n2849", "\343\201\262\343\201\227\343\201\254\343\201\276", true},
    {"n2826", "\343\202\253\343\203\204\343\202\253\343\203\254\343\203\274", false},
    {"n2827", "\343\203\204\343\203\212\343\201\213\343\202\223", false},
    {"n2830", "\343\201\277\343\202\204\343\201\213\343\202\217", true},
    {"n2884", "\343\201\215\343\201\237\343\201\236\343\201\256", true},
    {"n2917", "\343\201\217\343\201\276\343\201\276\343\201\253\343\201\202", false},
    {"n2937", "\343\202\271\343\203\252\343\203\274\343\203\221\343\203\274", false},
    {"n2973", "\343\201\223\343\201\204\343\202\217\343\203\222\343\203\255\343\203\244", false},
    {"n2984", "\357\274\264\357\274\257\357\274\255\357\274\257\357\274\272\357\274\257\357\274\256", false},
    {"n3022", "\343\201\210\343\201\254\343\201\247\343\201\203\343\201\212", true},
    {"n3051", "\343\201\215\343\202\212\343\202\204\343\201\276", true},
    {"n3061", "\343\201\227\343\202\223\343\201\230\343\202\215\343\201\206", false},
    {"n3157", "\343\203\231\343\203\253\343\203\252\343\203\274", false},
    {"n3160", "\343\201\237\343\201\253\343\201\213\343\202\217", true},
    {"N3224", "\343\202\210\343\201\227\343\201\240\343\201\202", true},
    {"kadoi", "(\343\203\273\342\210\200\343\203\273)", false},
    {"iwasa", "\343\202\266\343\203\203\343\202\255\343\203\274", false},
    {"ishioka", "\343\202\263\343\202\270\343\202\263\343\202\270", false},
    {"sasaki", "\343\201\202\343\202\223\343\201\226\343\201\204\343\201\233\343\202\223\343\201\233\343\201\204", false},
    {"kikuchi", "\343\201\215\343\201\217\343\201\241", true},
    {"kawasaki", "\343\201\213\343\202\217\343\201\225\343\201\215", true},
    {"nihei", "\357\274\244\357\275\217\357\275\222\357\275\217\357\275\222\357\275\217\357\275\216", false},
    {"1-up-doi", "\343\201\251\343\201\204", true},
    {"1-up-inoue", "\343\201\204\343\201\256\343\201\206\343\201\210", true},
    {"1-up-yaeno", "\343\201\257\343\202\211\343\201\253\343\201\227", false},
    {"1-up-araya", "\343\203\242\343\203\255\343\203\203\343\202\255\343\203\245", false},
    {"1-up-sato", "\343\202\266\343\203\274\343\203\234\343\203\263", false},
    {"ead-tokyo", "\343\203\242\343\203\216\343\202\257\343\203\236", false},
};
const GhostPlayerUserInfo sDefaultGhostPlayerUserInfo = {"", "????", false};
}

u32 GhostPlayerRecorder::getRecorderBufferSize() { return 0x5b864; }
GhostPlayerRecorder::GhostPlayerRecorder(const al::IUseSceneObjHolder* user,
    const al::SceneInitInfo& info, const GameDataHolder* gameData, al::PlayerHolder* players)
    : mSceneUser(user), mGameData(gameData), mPlayerHolder(players), mStageName(info.mStageName) {
    if (info.mGameSystemInfo->_20)
        mNetworkSystem = static_cast<al::NetworkSystem*>(info.mGameSystemInfo->_20);
}
bool GhostPlayerRecorder::isAvailableDataStore() const { return false; }
void GhostPlayerRecorder::clearRecordedData() {
    memset(mBuffer, 0, getRecorderBufferSize());
    memset(mWarpData, 0, 0x600);
    memset(mActionData, 0, 0x2000);
    memset(mPoseData, 0, 0x57e40);
    memset(mMarkerData, 0, 0x1400);
}
void GhostPlayerRecorder::recordPosRotData(int index, const sead::Vector3f& trans,
                                          const sead::Vector3<s16>& rotate) {
    mPoseData[index].trans = trans;
    mPoseData[index].rotate = rotate;
}
void GhostPlayerRecorder::update() {
    if (!mBuffer || !mIsRecording || tryCancelRecording()) return;
    if (++mFrame & 1) return;
    sead::Vector3f trans(al::getTrans(mPlayer));
    const sead::Vector3f& playerRotate = al::getRotate(mPlayer);
    sead::Vector3<s16> rotate(playerRotate.x, playerRotate.y, playerRotate.z);
    if (mHasUserPose) {
        memcpy(&trans, &mUserTrans, sizeof(trans));
        memcpy(&rotate, &mUserRotate, sizeof(rotate));
    }
    recordPosRotData(mPoseCount, trans, rotate);
    mPoseData[mPoseCount].animFrame = al::getSklAnimFrame(mPlayer, 0);
    al::StringTmp<64> action(al::getActionName(mPlayer));
    for (int i = 0; i < 5; ++i) {
        if (action.findIndex(GameDataConst::getPlayerCharacterName(i)) == 0) {
            al::tryReplaceString(&action, GameDataConst::getPlayerCharacterName(i), "");
            break;
        }
    }
    mPoseData[mPoseCount].actionIndex = mActionCount;
    bool found = false;
    for (int i = 0; i < mActionCount; ++i) {
        if (al::isEqualString(action.cstr(), mActionData[i].name)) {
            mPoseData[mPoseCount].actionIndex = i;
            found = true;
            break;
        }
    }
    if (!found) {
        if (static_cast<u32>(action.calcLength()) < 32) {
            al::copyString(mActionData[mActionCount].name, action.cstr(), 32);
            ++mActionCount;
        } else mPoseData[mPoseCount].actionIndex = mActionCount - 1;
    }
    if (++mPoseCount >= 15000u) endRecord();
}
bool GhostPlayerRecorder::tryStartRecord() {
    if (!mBuffer || mFrame != 0) return false;
    if (rc::getActiveControlUserNum(GameDataHolderAccessor(mSceneUser)) > 1) return false;
    int port = rc::getControlUserPortNumber(GameDataHolderAccessor(mSceneUser),
        rc::getActiveControlUserFirst(GameDataHolderAccessor(mSceneUser)));
    mPlayer = rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);
    if (!mPlayer) return false;
    if (!GhostPlayerFunction::isCurrentCourseGhostPlayer(
            GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameData)))) return false;
    mIsRecording = true;
    return true;
}
int GhostPlayerRecorder::endRecord() {
    if (!mBuffer || !mIsRecording) return -1;
    mIsRecording = false;
    return mFrame;
}
__attribute__((noinline)) void GhostPlayerRecorder::cancelRecord() {
    if (mBuffer) { mIsRecording = false; mFrame = 0; }
}
void GhostPlayerRecorder::setClearTime(int time) { mClearTime = time; }
bool GhostPlayerRecorder::tryCancelRecording() {
    if (rc::getActiveControlUserNum(GameDataHolderAccessor(mSceneUser)) > 1) {
        if (mBuffer) { mIsRecording = false; mFrame = 0; }
        return true;
    }
    if (rc::isPlayerRaccoonDogWhite(mPlayer) || rc::isPlayerClimbWhite(mPlayer)) {
        if (mBuffer) { mIsRecording = false; mFrame = 0; }
        return true;
    }
    if (al::isDead(mPlayer)) {
        if (!mIsGoalKilled) {
            if (mBuffer) { mIsRecording = false; mFrame = 0; }
            return true;
        }
        int port = rc::getControlUserPortNumber(GameDataHolderAccessor(mSceneUser),
            rc::getActiveControlUserFirst(GameDataHolderAccessor(mSceneUser)));
        mPlayer = rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);
    }
    return false;
}
bool GhostPlayerRecorder::isUploadableToDataStore() const { return false; }
int GhostPlayerRecorder::getHeaderVersion() const { return 2; }
u32 GhostPlayerRecorder::writeGhostData() {
    struct Header {
        int version, warpCount, actionCount, poseCount;
        u32 warpOffset, actionOffset, poseOffset;
        int markerCount;
        u32 markerOffset;
    } header;
    header.version = getHeaderVersion();
    header.warpCount = mWarpCount;
    header.actionCount = mActionCount;
    header.poseCount = mPoseCount - 1;
    header.warpOffset = sizeof(Header);
    header.actionOffset = header.warpOffset + mWarpCount * sizeof(*mWarpData);
    header.markerCount = mMarkerCount;
    header.markerOffset = header.actionOffset + mActionCount * sizeof(*mActionData);
    header.poseOffset = header.markerOffset + mMarkerCount * sizeof(*mMarkerData);
    sead::RamWriteStream stream(mBuffer, header.poseOffset + (mPoseCount - 1) * sizeof(*mPoseData),
                               sead::Stream::Modes::Binary);
    stream.writeMemBlock(&header, sizeof(header));
    stream.writeMemBlock(mWarpData, mWarpCount * sizeof(*mWarpData));
    stream.writeMemBlock(mActionData, mActionCount * sizeof(*mActionData));
    stream.writeMemBlock(mMarkerData, mMarkerCount * sizeof(*mMarkerData));
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeF32(mPoseData[i].trans.x);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeF32(mPoseData[i].trans.y);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeF32(mPoseData[i].trans.z);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeF32(mPoseData[i].animFrame);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeS16(mPoseData[i].rotate.x);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeS16(mPoseData[i].rotate.y);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeS16(mPoseData[i].rotate.z);
    for (int i = 0; i < mPoseCount - 1; ++i) stream.writeU16(mPoseData[i].actionIndex);
    return stream.getSrc().getCurrentPos();
}
bool GhostPlayerRecorder::saveRecord() {
    if (mFrame != 0 && mStageName && !al::isEqualString(mStageName, "")) {
        u32 size = writeGhostData(); mSavedData = mBuffer; mSavedSize = size;
    }
    return false;
}
__attribute__((noinline)) void GhostPlayerRecorder::setPlacementIdObj(const char* name, bool isStart) {
    if (!mBuffer || mWarpCount >= 64) return;
    for (int i = 0; i < mWarpCount; ++i)
        if (al::isEqualString(mWarpData[i].name, name)) return;
    mWarpData[mWarpCount].frame = mFrame;
    mWarpData[mWarpCount].isStart = isStart;
    al::copyString(mWarpData[mWarpCount].name, name, 20);
    ++mWarpCount;
}
__attribute__((noinline)) void GhostPlayerRecorder::setFlagShakeAfter(const al::PlacementId* id) {
    if (mBuffer) { mIsFlagShakeAfter = true; mCheckpointId = id; }
}
__attribute__((noinline)) void GhostPlayerRecorder::setPlayerRTByUser(const sead::Vector3f& rotate,
                                            const sead::Vector3f& trans) {
    if (mIsRecording) {
        mHasUserPose = true;
        mUserRotate.set(rotate.x, rotate.y, rotate.z);
        memcpy(&mUserTrans, &trans, sizeof(trans));
    }
}
__attribute__((noinline)) void GhostPlayerRecorder::resetPlayerRTByUser() {
    if (mIsRecording) mHasUserPose = false;
}
void GhostPlayerRecorder::recordMarker(int id, const sead::Vector3f& trans, bool) {
    GhostRecorderMarkerData* marker = &mMarkerData[mMarkerCount];
    marker->poseIndex = mPoseCount; marker->markerId = id;
    marker->trans.x = trans.x; marker->trans.y = trans.y; marker->trans.z = trans.z;
    ++mMarkerCount;
}
namespace {
inline GhostPlayerRecorder* getRecorder(const al::IUseSceneObjHolder* user) {
    return GhostPlayerFunction::isExistGhostPlayerDirector(user)
        ? GhostPlayerFunction::getGhostPlayerDirector(user)->getRecorder() : nullptr;
}
inline GhostPlayerPlayer* getPlayer(const al::IUseSceneObjHolder* user) {
    return GhostPlayerFunction::isExistGhostPlayerDirector(user)
        ? GhostPlayerFunction::getGhostPlayerDirector(user)->getPlayer() : nullptr;
}
}
namespace GhostPlayerFunction {
void getTimeAttackRank(sead::BufferedSafeString* out, GameDataHolderAccessor accessor,
                       const char* stage, int time) {
    if (time < 1) { out->clear(); return; }
    int base = GameDataFunction::findGhostBaseTime(accessor, stage);
    int limit = (base >= 0 ? (base + 4) / 5 : (base - 4) / 5) * 5 + 50;
    int rank = 0;
    for (int lower = 0; lower < limit; lower += 5, ++rank)
        if (lower <= time && time < lower + 5) break;
    out->format("Rank%02d", rank);
}
void getTimeAttackRivalRank(sead::BufferedSafeString* out, GameDataHolderAccessor accessor,
                       const char* stage, int time) {
    if (time < 1) { out->clear(); return; }
    int base = GameDataFunction::findGhostBaseTime(accessor, stage);
    int limit = (base >= 0 ? (base + 4) / 5 : (base - 4) / 5) * 5 + 50;
    int rank = 0;
    for (int lower = 0; lower < limit; lower += 5, ++rank)
        if (lower <= time && time < lower + 5) break;
    out->format("Rank%02d", sead::Mathi::max(rank - 1, 0));
}
}
namespace rc {
const GhostPlayerUserInfo* tryFindGhostPlayerUserInfo(const char* id) {
    for (u32 i = 0; i < 72; ++i)
        if (al::isEqualStringCase(id, sGhostPlayerUserInfo[i].id)) return &sGhostPlayerUserInfo[i];
    return nullptr;
}
const GhostPlayerUserInfo* findGhostPlayerUserInfo(const char* id) {
    const GhostPlayerUserInfo* info = tryFindGhostPlayerUserInfo(id);
    return info ? info : &sDefaultGhostPlayerUserInfo;
}

bool isCourseAppearGhostPlayer(GameDataHolderAccessor accessor, int course) {
    return GhostPlayerFunction::isEnablePlayGhostPlayer(accessor, course);
}
bool isExistGhostPlayerRecorder(const al::IUseSceneObjHolder* user) { return getRecorder(user) != nullptr; }
void endRecordGhostPlayerRecorder(const al::IUseSceneObjHolder* user, bool useBaseTime) {
    if (isExistGhostPlayerRecorder(user))
        GhostPlayerFunction::getGhostPlayerDirector(user)->tryEndRecord(useBaseTime);
}
void cancelRecordGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    if (isExistGhostPlayerRecorder(user)) getRecorder(user)->cancelRecord();
}
void notifyGoalKillGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    if (isExistGhostPlayerRecorder(user)) getRecorder(user)->notifyGoalKill();
}
bool tryRequestDownloadGhostData(al::NetworkSystem*, const GameDataHolder*, const char*, int, bool) { return false; }
bool tryCalcEntryGhostId(int*, int*, int*, bool*, al::NetworkSystem*, GameDataHolderAccessor, int) { return false; }
bool isExistTimeAttackRivalGhost(al::NetworkSystem*, GameDataHolderAccessor, int) { return false; }
bool tryCreateGhostMiiIcon(al::NetworkSystem*, GameDataHolderAccessor accessor, int course) {
    GhostPlayerFunction::isEnablePlayGhostPlayer(accessor, course);
    return false;
}
const char* getStageNameGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    return isExistGhostPlayerRecorder(user) ? getRecorder(user)->getStageName() : nullptr;
}
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* user, const char* name, bool isStart) {
    if (isExistGhostPlayerRecorder(user)) getRecorder(user)->setPlacementIdObj(name, isStart);
}
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* user, const al::PlacementId* id, bool isStart) {
    al::StringTmp<32> name("%s%s", id->mPlacementID, id->mZoneID ? id->mZoneID : "");
    setPlacementIdObjGhostPlayerRecorder(user, name.cstr(), isStart);
}
void setFlagShakeAfterGhostPlayerRecorder(const CheckpointFlag* flag) {
    if (isExistGhostPlayerRecorder(flag))
        getRecorder(flag)->setFlagShakeAfter(flag->getPlacementId());
}
bool isFlagShakeAfterGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    return isExistGhostPlayerRecorder(user) ? getRecorder(user)->isFlagShakeAfter() : false;
}
const al::PlacementId* tryGetPlacementIdFlagGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    return isExistGhostPlayerRecorder(user) ? getRecorder(user)->getCheckpointId() : nullptr;
}
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* user, const char* name) {
    if (getPlayer(user)) getPlayer(user)->tryStartFromObj(name);
}
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* user, const al::PlacementId* id) {
    al::StringTmp<32> name("%s%s", id->mPlacementID, id->mZoneID ? id->mZoneID : "");
    tryStartFromObjGhostPlayerRecorder(user, name.cstr());
}
void stopAndHideGhostPlayerAll(const al::IUseSceneObjHolder* user) {
    if (getPlayer(user)) getPlayer(user)->stopAndHideGhostPlayerAll();
}
void restartGhostPlayerAll(const al::IUseSceneObjHolder* user) {
    if (getPlayer(user)) getPlayer(user)->restartGhostPlayerAll();
}
void setPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* user,
                                        const sead::Vector3f& rotate, const sead::Vector3f& trans) {
    if (getPlayer(user)) getRecorder(user)->setPlayerRTByUser(rotate, trans);
}
void resetPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* user) {
    if (getPlayer(user)) getRecorder(user)->resetPlayerRTByUser();
}
void notifyPlayerStartWarpToGhostPlayer(const al::IUseSceneObjHolder* user) {
    if (getPlayer(user)) getPlayer(user)->waitForStartFromWarpObj();
}
}