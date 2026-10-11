#pragma once
#include <basis/seadTypes.h>
#include "Project/Audio/System/SoundHeapPtrWrapper.hpp"

namespace al {
class BgmRhythmCtrl;
class MeInfo {
  public:
    s32 _0;
    s32 mChordOffset;
    s32 mScaleOffset;
    s32 mPitchOffset;
};
class SePlayParamList;
struct MeInfoEntry {
    MeInfoEntry() = default;
    /**
     * @brief Creates one musical-effect entry.
     * @param variable Sequence local-variable index receiving the note.
     * @param chord Chord degree, or a negative value for none.
     * @param scale Scale-degree offset.
     * @param pitch Pitch offset in semitones.
     */
    MeInfoEntry(s32 variable, s32 chord, s32 scale, s32 pitch)
        : mVariable(variable), mChord(chord), mScale(scale), mPitch(pitch) {}

    s32 mVariable = 0;
    s32 mChord = 0;
    s32 mScale = 0;
    s32 mPitch = 0;
};
static_assert(sizeof(MeInfoEntry) == 0x10);

class MeInfoList {
  public:
    MeInfoList() = default;
    /**
     * @brief Creates a named list; MeInfoKeeper::init() resolves the sound ID.
     * @param pName Sound name.
     * @param pEntries Entry array.
     * @param entryNum Number of entries.
     */
    MeInfoList(const char* pName, MeInfoEntry* pEntries, s32 entryNum)
        : mName(pName), mEntries(pEntries), mEntryNum(entryNum) {}

    const char* mName = nullptr;
    u32 mSoundId = AudioConst::SOUND_ID_INVALID;
    MeInfoEntry* mEntries = nullptr;
    s32 mEntryNum = 0;
};
static_assert(sizeof(MeInfoList) == 0x20);

class MeInfoKeeper {
  public:
    MeInfoKeeper();
    void init(BgmRhythmCtrl* pRhythmCtrl);
    bool isMe(s32 soundId) const;
    MeInfoList* tryFindMeInfoList(const char* pName) const;
    MeInfoList* tryFindMeInfoListById(s32 soundId) const;
    void applyMeInfoToParams(const char* pName, SePlayParamList* pParams, MeInfo* pInfo);
    void applyMeInfoToParams(s32 soundId, SePlayParamList* pParams, MeInfo* pInfo);
    void applyMeInfoToParams(MeInfoList* pList, SePlayParamList* pParams, MeInfo* pInfo, const char* pName);

  private:
    BgmRhythmCtrl* mRhythmCtrl = nullptr;
    MeInfoList** mLists = nullptr;
    s32 mListNum = 0;
};
static_assert(sizeof(MeInfoKeeper) == 0x18);
} // namespace al
