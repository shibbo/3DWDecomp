#include "NPC/GhostPlayerRecorder.hpp"

#include <attributes.h>

#include <stream/seadRamStream.h>

#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Library/LiveActor/Util/ActorAnimUtil.hpp"
#include "Library/LiveActor/Util/ActorFlagUtil.hpp"
#include "Library/LiveActor/Util/ActorPoseUtil.hpp"
#include "Library/Play/Placement/PlacementId.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "MapObj/CheckpointFlag.hpp"
#include "NPC/GhostPlayerDirector.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Scene/SceneInitInfo.hpp"
#include "Scene/ProjectActorFactoryTypes.hpp"
#include "System/GameDataConst.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"
#include "Util/GhostPlayerUtil.hpp"
#include "Util/PlayerUtil.hpp"

namespace {
/** @brief Number of player characters whose name prefixes recorded action names. */
constexpr s32 cPlayerCharacterNum = 5;

/** @brief Number of built-in ghost owners. */
constexpr s32 cGhostPlayerUserInfoNum = 72;

/** @brief Built-in ghost owners (staff ghosts shipped in ROM). */
const rc::GhostPlayerUserInfo sGhostPlayerUserInfoTable[cGhostPlayerUserInfoNum] = {
    {"n0612", "こんどう", true},
    {"n0692", "パトリオット", false},
    {"n1047", "ツナマヨ", false},
    {"n1269", "Ｉｔ’ｓゴールド", false},
    {"n1296", "ハ　ル　キ", false},
    {"n1390", "ｐｏｎｙｏ", false},
    {"n1468", "ピカチュー", false},
    {"n1489", "ろぜった。", false},
    {"n1507", "みねぎし", true},
    {"n1573", "はやかわ", true},
    {"n1585", "てづか", false},
    {"n1600", "かずみ", true},
    {"n1605", "しらい", true},
    {"n1640", "はやしだ", true},
    {"n1672", "くろねこ", false},
    {"n1704", "ビャビョーン", false},
    {"n1981", "ＤＩＣＥ", false},
    {"n1985", "よこた", true},
    {"n1986", "あつし", true},
    {"n1993", "Ｓｈｉｎ", false},
    {"N2034", "ミクチー", false},
    {"N2056", "シーブリーズ", false},
    {"n2080", "Ｇ３０", false},
    {"n2081", "あおやぎ", true},
    {"n2093", "まつだ", true},
    {"n2125", "よしだ", true},
    {"n2154", "いで", true},
    {"n2156", "ＮＯＲＩ", false},
    {"n2185", "コージ", false},
    {"n2186", "わたなべ", true},
    {"n2187", "ＭＯＭＯ(^ω^)♪", false},
    {"n2229", "にのみやかずなり", false},
    {"N2239", "きだ", true},
    {"n2240", "ごうはら", true},
    {"n2305", "あかぴく", false},
    {"n2349", "Ｃｈａｒｒｙ", false},
    {"n2433", "みねた", true},
    {"n2476", "むさ", true},
    {"n2502", "まーちゃん", false},
    {"n2505", "むらた", true},
    {"n2506", "つじ", true},
    {"n2606", "ルーヒー", false},
    {"n2639", "ねこにん", false},
    {"n2642", "のんべー", false},
    {"n2849", "ひしぬま", true},
    {"n2826", "カツカレー", false},
    {"n2827", "ツナかん", false},
    {"n2830", "みやかわ", true},
    {"n2884", "きたぞの", true},
    {"n2917", "くままにあ", false},
    {"n2937", "スリーパー", false},
    {"n2973", "こいわヒロヤ", false},
    {"n2984", "ＴＯＭＯＺＯＮ", false},
    {"n3022", "えぬでぃお", true},
    {"n3051", "きりやま", true},
    {"n3061", "しんじろう", false},
    {"n3157", "ベルリー", false},
    {"n3160", "たにかわ", true},
    {"N3224", "よしだあ", true},
    {"kadoi", "(・∀・)", false},
    {"iwasa", "ザッキー", false},
    {"ishioka", "コジコジ", false},
    {"sasaki", "あんざいせんせい", false},
    {"kikuchi", "きくち", true},
    {"kawasaki", "かわさき", true},
    {"nihei", "Ｄｏｒｏｒｏｎ", false},
    {"1-up-doi", "どい", true},
    {"1-up-inoue", "いのうえ", true},
    {"1-up-yaeno", "はらにし", false},
    {"1-up-araya", "モロッキュ", false},
    {"1-up-sato", "ザーボン", false},
    {"ead-tokyo", "モノクマ", false},
};

/** @brief Owner info returned for an unknown ghost owner. */
const rc::GhostPlayerUserInfo sUnknownGhostPlayerUserInfo = {"", "????", false};

/**
 * @brief Ghost recorder of the scene.
 * @param pHolder Scene object holder of the scene.
 * @return The recorder, or nullptr if there is no ghost director.
 */
GhostPlayerRecorder* getGhostPlayerRecorder(const al::IUseSceneObjHolder* pHolder) {
    if (!GhostPlayerFunction::isExistGhostPlayerDirector(pHolder)) {
        return nullptr;
    }

    return GhostPlayerFunction::getGhostPlayerDirector(pHolder)->getRecorder();
}

/**
 * @brief Check whether the scene has a ghost player.
 * @param pHolder Scene object holder of the scene.
 * @return True if a ghost director with a ghost player exists.
 */
bool isExistGhostPlayerPlayer(const al::IUseSceneObjHolder* pHolder) {
    if (!GhostPlayerFunction::isExistGhostPlayerDirector(pHolder)) {
        return false;
    }

    return GhostPlayerFunction::getGhostPlayerDirector(pHolder)->getPlayer() != nullptr;
}

/**
 * @brief Ghost player of the scene.
 * @param pHolder Scene object holder of the scene.
 * @return The ghost player, or nullptr if there is no ghost director.
 */
GhostPlayerPlayer* getGhostPlayerPlayer(const al::IUseSceneObjHolder* pHolder) {
    if (!GhostPlayerFunction::isExistGhostPlayerDirector(pHolder)) {
        return nullptr;
    }

    return GhostPlayerFunction::getGhostPlayerDirector(pHolder)->getPlayer();
}

/**
 * @brief Calculate the time attack rank of a clear time, in steps of 5 from 0.
 * @param pStageName Stage the time was made on.
 * @param accessor Accessor to the game data.
 * @param time Clear time.
 * @return The rank, counted from the fastest one.
 */
s32 calcTimeAttackRank(GameDataHolderAccessor accessor, const char* pStageName, s32 time) {
    s32 baseTime = GameDataFunction::findGhostBaseTime(accessor, pStageName);
    s32 roundedBaseTime = baseTime >= 0 ? (baseTime + 4) / 5 * 5 : (baseTime - 4) / 5 * 5;
    s32 rankTimeMax = roundedBaseTime + 50;
    s32 rank = 0;


    for (s32 rankTime = 0; rankTime < rankTimeMax; rankTime += 5) {
        if (rankTime <= time && time < rankTime + 5) {
            break;
        }

        rank++;
    }

    return rank;
}
}  // namespace

/**
 * @brief Size of the buffer the recorded ghost data is written to.
 * @return The buffer size in bytes.
 */
u32 GhostPlayerRecorder::getRecorderBufferSize() {
    return 0x5b864;
}

/**
 * @brief Construct the recorder.
 * @param pHolder Scene object holder of the scene.
 * @param rInfo Scene initialization info.
 * @param pGameDataHolder Game data holder of the running game.
 * @param pPlayerHolder Holder of the players to record.
 */
GhostPlayerRecorder::GhostPlayerRecorder(const al::IUseSceneObjHolder* pHolder,
                                         const al::SceneInitInfo& rInfo,
                                         const GameDataHolder* pGameDataHolder,
                                         al::PlayerHolder* pPlayerHolder)
    : mSceneObjHolder(pHolder), mGameDataHolder(pGameDataHolder), mPlayerHolder(pPlayerHolder),
      mStageName(rInfo.mStageName) {
    al::NetworkSystem* pNetworkSystem = rInfo.mGameSystemInfo->getNetworkSystem();

    if (pNetworkSystem != nullptr) {
        mNetworkSystem = pNetworkSystem;
    }
}

/**
 * @brief Check whether the data store (online ghost upload) is available.
 * @return Always false.
 */
bool GhostPlayerRecorder::isAvailableDataStore() const {
    return false;
}

/** @brief Clear all recorded data buffers. */
void GhostPlayerRecorder::clearRecordedData() {
    memset(mDataBuffer, 0, getRecorderBufferSize());
    memset(mWarpObjData, 0, sizeof(GhostWarpObjData) * cWarpObjNumMax);
    memset(mActionNames, 0, sizeof(ActionName) * cActionNameNumMax);
    memset(mPlayData, 0, sizeof(GhostPlayData) * cPlayDataNumMax);
    memset(mMarkers, 0, sizeof(GhostMarkerData) * cMarkerNumMax);
}

/**
 * @brief Record the pose of a frame.
 * @param frame Index of the recorded frame.
 * @param rTrans Position of the player.
 * @param rRotate Rotation of the player.
 */
void GhostPlayerRecorder::recordPosRotData(s32 frame, const sead::Vector3f& rTrans,
                                           const sead::Vector3<s16>& rRotate) {
    mPlayData[frame].mTrans = rTrans;
    mPlayData[frame].mRotate = rRotate;
}

/** @brief Record the player every other frame while recording. */
void GhostPlayerRecorder::update() {
    if (mDataBuffer == nullptr || !mIsRecording) {
        return;
    }

    if (tryCancelRecording()) {
        return;
    }

    mRecordFrame++;

    if ((mRecordFrame & 1) != 0) {
        return;
    }

    sead::Vector3f trans = al::getTrans(mPlayer);
    const sead::Vector3f& rRotate = al::getRotate(mPlayer);
    sead::Vector3<s16> rotate(rRotate.x, rRotate.y, rRotate.z);


    if (mIsPlayerRTByUser) {
        trans.set(mUserTrans);
        rotate.set(mUserRotate);
    }

    recordPosRotData(mPlayDataNum, trans, rotate);
    mPlayData[mPlayDataNum].mSklAnimFrame = al::getSklAnimFrame(mPlayer, 0);

    al::StringTmp<64> actionName(al::getActionName(mPlayer));


    for (s32 i = 0; i < cPlayerCharacterNum; i++) {
        if (actionName.findIndex(GameDataConst::getPlayerCharacterName(i)) == 0) {
            al::tryReplaceString(&actionName, GameDataConst::getPlayerCharacterName(i), "");
            break;
        }
    }

    mPlayData[mPlayDataNum].mActionIndex = mActionNameNum;

    bool isFound = false;

    for (u32 i = 0; i < mActionNameNum; i++) {
        if (al::isEqualString(actionName.cstr(), mActionNames[i])) {
            mPlayData[mPlayDataNum].mActionIndex = i;
            isFound = true;
            break;
        }
    }

    if (!isFound) {
        if (actionName.calcLength() < sizeof(ActionName)) {
            al::copyString(mActionNames[mActionNameNum], actionName.cstr(), cActionNameLength);
            mActionNameNum++;
        } else {
            mPlayData[mPlayDataNum].mActionIndex = mActionNameNum - 1;
        }
    }

    mPlayDataNum++;

    if (mPlayDataNum >= cPlayDataNumMax) {
        endRecord();
    }
}

/**
 * @brief Start recording the player of the first active control user.
 * @return True if the recording started.
 */
bool GhostPlayerRecorder::tryStartRecord() {
    if (mDataBuffer == nullptr || mRecordFrame != 0) {
        return false;
    }

    if (rc::getActiveControlUserNum(mSceneObjHolder) > 1) {
        return false;
    }

    s32 port = rc::getControlUserPortNumber(mSceneObjHolder,
                                            rc::getActiveControlUserFirst(mSceneObjHolder));
    mPlayer = rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);

    if (mPlayer == nullptr ||
        !GhostPlayerFunction::isCurrentCourseGhostPlayer(
            GameDataHolderAccessor(const_cast<GameDataHolder*>(mGameDataHolder)))) {
        return false;
    }

    mIsRecording = true;
    return true;
}

/**
 * @brief End the recording.
 * @return The number of recorded frames, or -1 if nothing was being recorded.
 */
s32 GhostPlayerRecorder::endRecord() {
    if (mDataBuffer == nullptr || !mIsRecording) {
        return -1;
    }

    mIsRecording = false;
    return mRecordFrame;
}

/** @brief Cancel the recording and drop the recorded frames. */
NOINLINE void GhostPlayerRecorder::cancelRecord() {
    resetRecording();
}

/**
 * @brief Set the clear time of the recorded play.
 * @param time The clear time.
 */
void GhostPlayerRecorder::setClearTime(s32 time) {
    mClearTime = time;
}

/**
 * @brief Cancel the recording when it can't be used as a ghost (multiplayer, white power-ups or
 * a death that isn't the goal kill).
 * @return True if the recording was cancelled.
 */
bool GhostPlayerRecorder::tryCancelRecording() {
    if (rc::getActiveControlUserNum(mSceneObjHolder) >= 2) {
        resetRecording();
        return true;
    }

    if (rc::isPlayerRaccoonDogWhite(mPlayer) || rc::isPlayerClimbWhite(mPlayer)) {
        resetRecording();
        return true;
    }

    if (al::isDead(mPlayer)) {
        if (!mIsGoalKill) {
            resetRecording();
            return true;
        }

        s32 port = rc::getControlUserPortNumber(mSceneObjHolder,
                                                rc::getActiveControlUserFirst(mSceneObjHolder));
        mPlayer = rc::tryFindPlayerFromInputPort(mPlayerHolder, port, false);
    }

    return false;
}

/**
 * @brief Check whether the recorded data can be uploaded to the data store.
 * @return Always false.
 */
bool GhostPlayerRecorder::isUploadableToDataStore() const {
    return false;
}

/**
 * @brief Version of the written ghost data header.
 * @return The header version.
 */
s32 GhostPlayerRecorder::getHeaderVersion() const {
    return 2;
}

/**
 * @brief Write the recorded data (header, warp objects, action names, markers and frames) to the
 * data buffer.
 * @return The number of bytes written.
 */
s32 GhostPlayerRecorder::writeGhostData() {
    GhostPlayDataHeader header;
    header.mVersion = getHeaderVersion();
    header.mWarpObjNum = mWarpObjNum;
    header.mActionNameNum = mActionNameNum;
    header.mPlayDataNum = mPlayDataNum - 1;
    header.mWarpObjOffset = sizeof(GhostPlayDataHeader);
    header.mActionNameOffset =
        header.mWarpObjOffset + header.mWarpObjNum * sizeof(GhostWarpObjData);
    header.mMarkerNum = mMarkerNum;
    header.mMarkerOffset = header.mActionNameOffset + header.mActionNameNum * sizeof(ActionName);
    header.mPlayDataOffset = header.mMarkerOffset + header.mMarkerNum * sizeof(GhostMarkerData);
    u32 size = header.mPlayDataOffset + (mPlayDataNum - 1) * sizeof(GhostPlayData);

    sead::RamWriteStream stream(mDataBuffer, size, sead::Stream::Modes::Binary);
    stream.writeMemBlock(&header, sizeof(GhostPlayDataHeader));
    stream.writeMemBlock(mWarpObjData, mWarpObjNum * sizeof(GhostWarpObjData));
    stream.writeMemBlock(mActionNames, mActionNameNum * sizeof(ActionName));
    stream.writeMemBlock(mMarkers, mMarkerNum * sizeof(GhostMarkerData));


    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeF32(mPlayData[i].mTrans.x);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeF32(mPlayData[i].mTrans.y);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeF32(mPlayData[i].mTrans.z);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeF32(mPlayData[i].mSklAnimFrame);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeS16(mPlayData[i].mRotate.x);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeS16(mPlayData[i].mRotate.y);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeS16(mPlayData[i].mRotate.z);
    }

    for (s32 i = 0; i < mPlayDataNum - 1; i++) {
        stream.writeU16(mPlayData[i].mActionIndex);
    }

    return stream.getSrc().getCurrentPos();
}

/**
 * @brief Write the recorded data so it can be saved.
 * @return Always false.
 */
bool GhostPlayerRecorder::saveRecord() {
    if (mRecordFrame == 0 || mStageName == nullptr || al::isEqualString(mStageName, "")) {
        return false;
    }

    s32 size = writeGhostData();
    mSaveData = mDataBuffer;
    mSaveDataSize = size;
    return false;
}

/**
 * @brief Record that the player passed a warp object (checkpoint), once per object.
 * @param pName Placement id string of the object.
 * @param isRestartPoint Whether the object is a restart point.
 */
NOINLINE void GhostPlayerRecorder::setPlacementIdObj(const char* pName, bool isRestartPoint) {
    if (mDataBuffer == nullptr || mWarpObjNum >= cWarpObjNumMax) {
        return;
    }

    for (s32 i = 0; i < mWarpObjNum; i++) {
        if (al::isEqualString(mWarpObjData[i].mName, pName)) {
            return;
        }
    }

    mWarpObjData[mWarpObjNum].mFrame = mRecordFrame;
    mWarpObjData[mWarpObjNum].mIsRestartPoint = isRestartPoint;
    al::copyString(mWarpObjData[mWarpObjNum].mName, pName, sizeof(GhostWarpObjData::mName));
    mWarpObjNum++;
}

/**
 * @brief Record that a checkpoint flag was shaken.
 * @param pPlacementId Placement id of the flag.
 */
NOINLINE void GhostPlayerRecorder::setFlagShakeAfter(const al::PlacementId* pPlacementId) {
    if (mDataBuffer != nullptr) {
        mIsFlagShakeAfter = true;
        mFlagPlacementId = pPlacementId;
    }
}

/**
 * @brief Override the recorded pose of the player.
 * @param rRotate Rotation to record.
 * @param rTrans Position to record.
 */
NOINLINE void GhostPlayerRecorder::setPlayerRTByUser(const sead::Vector3f& rRotate,
                                            const sead::Vector3f& rTrans) {
    if (mIsRecording) {
        mIsPlayerRTByUser = true;
        mUserRotate.set(rRotate.x, rRotate.y, rRotate.z);
        mUserTrans.set(rTrans);
    }
}

/** @brief Record the actual pose of the player again. */
NOINLINE void GhostPlayerRecorder::resetPlayerRTByUser() {
    if (mIsRecording) {
        mIsPlayerRTByUser = false;
    }
}

/**
 * @brief Record a marker at the current frame.
 * @param type Type of the marker.
 * @param rPos Position of the marker.
 * @param arg Unused.
 */
void GhostPlayerRecorder::recordMarker(s32 type, const sead::Vector3f& rPos, bool arg) {
    GhostMarkerData& rMarker = mMarkers[mMarkerNum];
    rMarker.mFrame = mPlayDataNum;
    rMarker.mType = type;
    rMarker.mPosX = rPos.x;
    rMarker.mPosY = rPos.y;
    rMarker.mPosZ = rPos.z;
    mMarkerNum++;
}

/**
 * @brief Format the time attack rank of a clear time.
 * @param pOut Output string ("RankNN"), cleared when there is no time.
 * @param accessor Accessor to the game data.
 * @param pStageName Stage the time was made on.
 * @param time Clear time.
 */
void GhostPlayerFunction::getTimeAttackRank(sead::BufferedSafeString* pOut,
                                            GameDataHolderAccessor accessor,
                                            const char* pStageName, s32 time) {
    if (time > 0) {
        pOut->format("Rank%02d", calcTimeAttackRank(accessor, pStageName, time));
        return;
    }

    pOut->clear();
}

/**
 * @brief Format the time attack rank of a rival's clear time (one rank faster).
 * @param pOut Output string ("RankNN"), cleared when there is no time.
 * @param accessor Accessor to the game data.
 * @param pStageName Stage the time was made on.
 * @param time Clear time.
 */
void GhostPlayerFunction::getTimeAttackRivalRank(sead::BufferedSafeString* pOut,
                                                 GameDataHolderAccessor accessor,
                                                 const char* pStageName, s32 time) {
    if (time > 0) {
        s32 rank = calcTimeAttackRank(accessor, pStageName, time) - 1;
        pOut->format("Rank%02d", rank < 0 ? 0 : rank);
        return;
    }

    pOut->clear();
}

namespace rc {

/**
 * @brief Find a built-in ghost owner.
 * @param pKey Key of the owner.
 * @return The owner info, or nullptr if the key is unknown.
 */
const GhostPlayerUserInfo* tryFindGhostPlayerUserInfo(const char* pKey) {
    for (s32 i = 0; i < cGhostPlayerUserInfoNum; i++) {
        if (al::isEqualStringCase(pKey, sGhostPlayerUserInfoTable[i].mKey)) {
            return &sGhostPlayerUserInfoTable[i];
        }
    }

    return nullptr;
}

/**
 * @brief Find a built-in ghost owner.
 * @param pKey Key of the owner.
 * @return The owner info, or the unknown owner info if the key is unknown.
 */
const GhostPlayerUserInfo* findGhostPlayerUserInfo(const char* pKey) {
    const GhostPlayerUserInfo* pInfo = tryFindGhostPlayerUserInfo(pKey);
    return pInfo != nullptr ? pInfo : &sUnknownGhostPlayerUserInfo;
}

/**
 * @brief Check whether ghosts appear in a course.
 * @param accessor Accessor to the game data.
 * @param courseId Course to check.
 * @return True if ghosts can be played in the course.
 */
bool isCourseAppearGhostPlayer(GameDataHolderAccessor accessor, s32 courseId) {
    return GhostPlayerFunction::isEnablePlayGhostPlayer(accessor, courseId);
}

/**
 * @brief Check whether the scene has a ghost recorder.
 * @param pUser Scene object holder of the scene.
 * @return True if a ghost director with a recorder exists.
 */
bool isExistGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (!GhostPlayerFunction::isExistGhostPlayerDirector(pUser)) {
        return false;
    }

    return GhostPlayerFunction::getGhostPlayerDirector(pUser)->getRecorder() != nullptr;
}

/**
 * @brief End the ghost recording.
 * @param pUser Scene object holder of the scene.
 * @param isClear Whether the stage was cleared.
 */
void endRecordGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser, bool isClear) {
    if (isExistGhostPlayerRecorder(pUser)) {
        GhostPlayerFunction::getGhostPlayerDirector(pUser)->tryEndRecord(isClear);
    }
}

/**
 * @brief Cancel the ghost recording.
 * @param pUser Scene object holder of the scene.
 */
void cancelRecordGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (isExistGhostPlayerRecorder(pUser)) {
        getGhostPlayerRecorder(pUser)->cancelRecord();
    }
}

/**
 * @brief Notify the ghost recorder that the player was killed by reaching the goal.
 * @param pUser Scene object holder of the scene.
 */
void notifyGoalKillGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (isExistGhostPlayerRecorder(pUser)) {
        getGhostPlayerRecorder(pUser)->notifyGoalKill();
    }
}

/**
 * @brief Request downloading online ghost data (unsupported).
 * @return Always false.
 */
bool tryRequestDownloadGhostData(al::NetworkSystem* pNetworkSystem,
                                 const GameDataHolder* pGameDataHolder, const char* pStageName,
                                 s32 arg, bool arg2) {
    return false;
}

/**
 * @brief Calculate the ids of the online ghosts to enter (unsupported).
 * @return Always false.
 */
bool tryCalcEntryGhostId(s32* pOutA, s32* pOutB, s32* pOutC, bool* pOutD,
                         al::NetworkSystem* pNetworkSystem, GameDataHolderAccessor accessor,
                         s32 courseId) {
    return false;
}

/**
 * @brief Check whether a time attack rival ghost exists (unsupported).
 * @return Always false.
 */
bool isExistTimeAttackRivalGhost(al::NetworkSystem* pNetworkSystem,
                                 GameDataHolderAccessor accessor, s32 courseId) {
    return false;
}

/**
 * @brief Create the Mii icon of the online ghost (unsupported).
 * @param pNetworkSystem Network system.
 * @param accessor Accessor to the game data.
 * @param courseId Course of the ghost.
 * @return Always false.
 */
bool tryCreateGhostMiiIcon(al::NetworkSystem* pNetworkSystem, GameDataHolderAccessor accessor,
                           s32 courseId) {
    if (!GhostPlayerFunction::isEnablePlayGhostPlayer(accessor, courseId)) {
        return false;
    }

    return false;
}

/**
 * @brief Name of the stage being recorded.
 * @param pUser Scene object holder of the scene.
 * @return The stage name, or nullptr if there is no recorder.
 */
const char* getStageNameGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (!isExistGhostPlayerRecorder(pUser)) {
        return nullptr;
    }

    return getGhostPlayerRecorder(pUser)->getStageName();
}

/**
 * @brief Record that the player passed a warp object.
 * @param pUser Scene object holder of the scene.
 * @param pId Placement id string of the object.
 * @param isStart Whether the object is a restart point.
 */
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser, const char* pId,
                                          bool isStart) {
    if (isExistGhostPlayerRecorder(pUser)) {
        getGhostPlayerRecorder(pUser)->setPlacementIdObj(pId, isStart);
    }
}

/**
 * @brief Record that the player passed a warp object.
 * @param pUser Scene object holder of the scene.
 * @param pPlacementId Placement id of the object.
 * @param isStart Whether the object is a restart point.
 */
void setPlacementIdObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                          const al::PlacementId* pPlacementId, bool isStart) {
    al::StringTmp<32> id("%s%s", pPlacementId->mPlacementID,
                         pPlacementId->mZoneID != nullptr ? pPlacementId->mZoneID : "");
    const char* pId = id.cstr();


    if (isExistGhostPlayerRecorder(pUser)) {
        getGhostPlayerRecorder(pUser)->setPlacementIdObj(pId, isStart);
    }
}

/**
 * @brief Record that a checkpoint flag was shaken.
 * @param flag The shaken flag.
 */
void setFlagShakeAfterGhostPlayerRecorder(const CheckpointFlag* flag) {
    if (isExistGhostPlayerRecorder(flag)) {
        getGhostPlayerRecorder(flag)->setFlagShakeAfter(flag->getPlacementId());
    }
}

/**
 * @brief Check whether a checkpoint flag was shaken during the recording.
 * @param pUser Scene object holder of the scene.
 * @return True after a checkpoint flag was shaken.
 */
bool isFlagShakeAfterGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (!isExistGhostPlayerRecorder(pUser)) {
        return false;
    }

    return getGhostPlayerRecorder(pUser)->isFlagShakeAfter();
}

/**
 * @brief Placement id of the checkpoint flag shaken during the recording.
 * @param pUser Scene object holder of the scene.
 * @return The placement id, or nullptr if there is none.
 */
const al::PlacementId* tryGetPlacementIdFlagGhostPlayerRecorder(
    const al::IUseSceneObjHolder* pUser) {
    if (!isExistGhostPlayerRecorder(pUser)) {
        return nullptr;
    }

    return getGhostPlayerRecorder(pUser)->getFlagPlacementId();
}

/**
 * @brief Restart the ghosts from a warp object.
 * @param pUser Scene object holder of the scene.
 * @param pId Placement id string of the object.
 */
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser, const char* pId) {
    if (isExistGhostPlayerPlayer(pUser)) {
        getGhostPlayerPlayer(pUser)->tryStartFromObj(pId);
    }
}

/**
 * @brief Restart the ghosts from a warp object.
 * @param pUser Scene object holder of the scene.
 * @param pPlacementId Placement id of the object.
 */
void tryStartFromObjGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                        const al::PlacementId* pPlacementId) {
    al::StringTmp<32> id("%s%s", pPlacementId->mPlacementID,
                         pPlacementId->mZoneID != nullptr ? pPlacementId->mZoneID : "");
    const char* pId = id.cstr();


    if (isExistGhostPlayerPlayer(pUser)) {
        getGhostPlayerPlayer(pUser)->tryStartFromObj(pId);
    }
}

/**
 * @brief Stop and hide all ghosts.
 * @param pHolder Scene object holder of the scene.
 */
void stopAndHideGhostPlayerAll(const al::IUseSceneObjHolder* pHolder) {
    if (isExistGhostPlayerPlayer(pHolder)) {
        getGhostPlayerPlayer(pHolder)->stopAndHideGhostPlayerAll();
    }
}

/**
 * @brief Restart all ghosts.
 * @param pHolder Scene object holder of the scene.
 */
void restartGhostPlayerAll(const al::IUseSceneObjHolder* pHolder) {
    if (isExistGhostPlayerPlayer(pHolder)) {
        getGhostPlayerPlayer(pHolder)->restartGhostPlayerAll();
    }
}

/**
 * @brief Override the recorded pose of the player.
 * @param pUser Scene object holder of the scene.
 * @param rRotate Rotation to record.
 * @param rTrans Position to record.
 */
void setPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser,
                                          const sead::Vector3f& rRotate,
                                          const sead::Vector3f& rTrans) {
    if (isExistGhostPlayerPlayer(pUser)) {
        getGhostPlayerRecorder(pUser)->setPlayerRTByUser(rRotate, rTrans);
    }
}

/**
 * @brief Record the actual pose of the player again.
 * @param pUser Scene object holder of the scene.
 */
void resetPlayerRTByUserGhostPlayerRecorder(const al::IUseSceneObjHolder* pUser) {
    if (isExistGhostPlayerPlayer(pUser)) {
        getGhostPlayerRecorder(pUser)->resetPlayerRTByUser();
    }
}

/**
 * @brief Notify the ghosts that the player started warping.
 * @param pUser Scene object holder of the scene.
 */
void notifyPlayerStartWarpToGhostPlayer(const al::IUseSceneObjHolder* pUser) {
    if (isExistGhostPlayerPlayer(pUser)) {
        getGhostPlayerPlayer(pUser)->waitForStartFromWarpObj();
    }
}

}  // namespace rc
