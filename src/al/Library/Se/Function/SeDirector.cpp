#include "Library/Se/Function/SeDirector.hpp"
#include "Library/Se/Project/SeRequestKeeper.hpp"
#include "Project/Se/SeListenerKeeper.hpp"
#include "Project/Base/StringUtil.hpp"
#include <attributes.h>
#include "Library/Se/Function/MeInfoKeeper.hpp"
#include "Library/Se/Project/SePlayParamList.hpp"
#include "Library/Se/Project/SeCategory.hpp"
#include "Library/Se/Project/SeMaterialInfoKeeper.hpp"
#include "Library/Se/Info/SeAudioInfo.hpp"
#include "Project/Audio/System/SoundHeapPtrWrapper.hpp"

#include "Library/Audio/System/AudioSystemInfo.hpp"

namespace al {
/** @brief Creates a director without keepers; init() allocates them. */
SeDirector::SeDirector() = default;

/**
 * @brief Allocates the request keepers, the musical-effect keeper and the material table.
 * @param pInfo Audio system providing the 3D manager, the sound-effect player and the sound database.
 * @param pRhythmCtrl Rhythm controller used by musical effects.
 * @param mainRequestNum Request capacity of the main keeper.
 * @param subRequestNum Request capacity of the sub keeper; no sub keeper is made when not positive.
 * @param demoRequestNum Request capacity of the demo keeper.
 * @param playerRequestNum Request capacity of the always-active player keeper.
 * @param volume Initial volume of every keeper.
 */
void SeDirector::init(AudioSystemInfo* pInfo, BgmRhythmCtrl* pRhythmCtrl, s32 mainRequestNum,
                      s32 subRequestNum, s32 demoRequestNum, s32 playerRequestNum, f32 volume) {
    mMainKeeper = new SeRequestKeeper(pInfo->mAudio3DMgr, pInfo->getSeadAudioPlayerForSe(), "メイン",
                                      mainRequestNum, volume);
    mDemoKeeper = new SeRequestKeeper(pInfo->mAudio3DMgr, pInfo->getSeadAudioPlayerForSe(), "デモ",
                                      demoRequestNum, volume);
    mPlayerKeeper = new SeRequestKeeper(pInfo->mAudio3DMgr, pInfo->getSeadAudioPlayerForSe(), "常時",
                                        playerRequestNum, volume);
    if (subRequestNum > 0) {
        mSubKeeper = new SeRequestKeeper(pInfo->mAudio3DMgr, pInfo->getSeadAudioPlayerForSe(), "サブ",
                                         subRequestNum, volume);
    }

    mMeInfoKeeper = new MeInfoKeeper();
    mMeInfoKeeper->init(pRhythmCtrl);
    mMaterialInfoKeeper = new SeMaterialInfoKeeper(pInfo->mSeDataBase);
}

/**
 * @brief Creates and initializes the 3D listener keeper.
 * @param pMgr 3D audio manager.
 * @param pCameraPos Camera position.
 * @param pCameraMtx Camera matrix.
 * @param pProjection Camera projection.
 * @param pCameraAt Camera look-at point.
 * @param pStageName Listener poser name.
 * @param isUseListenerPoser Enables distance pausing and listener posers.
 */
void SeDirector::init3D(SeadAudio3DMgr* pMgr, const sead::Vector3f* pCameraPos,
                        const sead::Matrix34f* pCameraMtx, sead::PerspectiveProjection* pProjection,
                        const sead::Vector3f* pCameraAt, const char* pStageName, bool isUseListenerPoser) {
    mIsDistancePauseEnabled = isUseListenerPoser;
    mListenerKeeper = new SeListenerKeeper(isUseListenerPoser, 1);
    mListenerKeeper->init(pMgr, pCameraPos, pCameraMtx, pProjection, pCameraAt, pStageName);
}

/** @brief Sets the category mixer. @param pController Category parameter controller, or nullptr. */
void SeDirector::initCategoryParamsController(SeCategoryParamsController* pController) {
    mCategoryParamsController = pController;
}

/** @brief Stops every sound immediately before the director is finalized. */
void SeDirector::finalize() {
    forEachRequestKeeper([](SeRequestKeeper* pKeeper) { pKeeper->stopAll(0, nullptr); });
}
/**
 * @brief Finds a request keeper by its routing name.
 * @param pName Name to compare; all four keepers must exist if earlier names do not match.
 * @return Matching keeper, or nullptr when no name matches.
 */
SeRequestKeeper* SeDirector::findRequestKeeper(const char* pName) const {
    if (isEqualString(mMainKeeper->getName(), pName)) {
        return mMainKeeper;
    }
    if (isEqualString(mDemoKeeper->getName(), pName)) {
        return mDemoKeeper;
    }
    if (isEqualString(mPlayerKeeper->getName(), pName)) {
        return mPlayerKeeper;
    }
    if (isEqualString(mSubKeeper->getName(), pName)) {
        return mSubKeeper;
    }
    return nullptr;
}
/**
 * @brief Stops all requests, optionally restricted to a named keeper.
 * @param fadeFrames Fade duration in update frames; zero stops immediately.
 * @param pExceptName Optional named stop-exception policy, passed to each keeper.
 * @param pKeeperName Keeper routing name, or nullptr to stop all initialized keepers.
 */
void SeDirector::stopAll(u32 fadeFrames, const char* pExceptName, const char* pKeeperName) {
    if (pKeeperName == nullptr) {
        forEachRequestKeeper([&](SeRequestKeeper* pKeeper) { pKeeper->stopAll(fadeFrames, pExceptName); });
    } else {
        SeRequestKeeper* pKeeper = findRequestKeeper(pKeeperName);
        if (pKeeper != nullptr) {
            pKeeper->stopAll(fadeFrames, pExceptName);
        }
    }
}
/** @brief Updates request playback and the optional 3D listener controller. */
void SeDirector::update() {
    const f32 distanceLimit = mIsDistancePauseEnabled ? 7500.0f : -1.0f;
    forEachRequestKeeper([&](SeRequestKeeper* pKeeper) { pKeeper->update(distanceLimit); });
    if (mListenerKeeper != nullptr) {
        mListenerKeeper->update();
    }
}
/**
 * @brief Stops sounds other than the supplied sound names in every keeper.
 * @param fadeFrames Fade duration in update frames; zero stops immediately.
 * @param pExceptList Array of sound names to preserve, or nullptr for none.
 * @param exceptNum Number of names in pExceptList.
 */
void SeDirector::stopAllExcept(u32 fadeFrames, const char** pExceptList, u32 exceptNum) {
    forEachRequestKeeper(
        [&](SeRequestKeeper* pKeeper) { pKeeper->stopAllExceptList(fadeFrames, pExceptList, exceptNum); });
}
/** @brief Stops the main keeper's sounds. @param fadeFrames Fade duration in update frames. */
void SeDirector::stopAllMain(u32 fadeFrames) { mMainKeeper->stopAll(fadeFrames, nullptr); }
/** @brief Stops the demo keeper's sounds. @param fadeFrames Fade duration in update frames. */
void SeDirector::stopAllDemo(u32 fadeFrames) { mDemoKeeper->stopAll(fadeFrames, nullptr); }
/**
 * @brief Changes a pause reason across every keeper.
 * @param isPause True to pause, false to release this pause reason.
 * @param pName Recognized system pause reason name.
 * @param fadeFrames Fade duration for the pause transition in update frames.
 */
void SeDirector::pauseSystem(bool isPause, const char* pName, u32 fadeFrames) {
    forEachRequestKeeper([&](SeRequestKeeper* pKeeper) { pKeeper->pauseSystem(isPause, pName, fadeFrames); });
}
/**
 * @brief Changes a pause reason on the main, demo, and player keepers.
 * @param isPause True to pause, false to release this pause reason.
 * @param pName Recognized system pause reason name.
 * @param fadeFrames Fade duration for the pause transition in update frames.
 */
void SeDirector::pauseSystemExceptSub(bool isPause, const char* pName, u32 fadeFrames) {
    mMainKeeper->pauseSystem(isPause, pName, fadeFrames);
    mDemoKeeper->pauseSystem(isPause, pName, fadeFrames);
    mPlayerKeeper->pauseSystem(isPause, pName, fadeFrames);
}
/** @brief Enables a named request keeper. @param pName Routing name of the keeper to enable. */
void SeDirector::activateRequestKeeper(const char* pName) {
    SeRequestKeeper* pKeeper = findRequestKeeper(pName);
    if (pKeeper != nullptr) {
        pKeeper->activateSystem();
    }
}
/** @brief Disables a named request keeper. @param pName Routing name of the keeper to disable. */
void SeDirector::deactivateRequestKeeper(const char* pName) {
    SeRequestKeeper* pKeeper = findRequestKeeper(pName);
    if (pKeeper != nullptr) {
        pKeeper->deactivateSystem();
    }
}
/**
 * @brief Suspends playback associated with a source in every keeper.
 * @param pSource Source whose requests should be suspended; must be initialized.
 * @param fadeFrames Fade duration in update frames.
 * @param isClipped Whether suspension is caused by the source being clipped.
 */
void SeDirector::deactivateSeFromSource(SeSource* pSource, u32 fadeFrames, bool isClipped) {
    forEachRequestKeeper(
        [&](SeRequestKeeper* pKeeper) { pKeeper->deactivateSeFromSource(pSource, fadeFrames, isClipped); });
}
/** @brief Restores a source's suspended sounds. @param pSource Previously suspended source. */
void SeDirector::reactivateSeFromSource(SeSource* pSource) {
    forEachRequestKeeper([&](SeRequestKeeper* pKeeper) { pKeeper->reactivateSeFromSource(pSource); });
}
/**
 * @brief Stops all requests attached to a source.
 * @param pSource Source whose requests should stop.
 * @param fadeFrames Fade duration in update frames.
 */
void SeDirector::stopAllFromSource(SeSource* pSource, u32 fadeFrames) {
    forEachRequestKeeper([&](SeRequestKeeper* pKeeper) { pKeeper->stopAllFromSource(pSource, fadeFrames); });
}
/**
 * @brief Replaces material-dependent sounds after their source changes surfaces or water state.
 * @param pSource Source whose material changed.
 * @param pMaterialName New material name, or nullptr when no surface replacement is requested.
 * @param waterState Water-state selector passed to the material replacement table.
 */
void SeDirector::notifiedUpdateMaterial(SeSource* pSource, const char* pMaterialName, s32 waterState) {
    mMainKeeper->notifiedUpdateMaterial(pSource, pMaterialName, waterState, mMaterialInfoKeeper);
    mPlayerKeeper->notifiedUpdateMaterial(pSource, pMaterialName, waterState, mMaterialInfoKeeper);
    mDemoKeeper->notifiedUpdateMaterial(pSource, pMaterialName, waterState, mMaterialInfoKeeper);
}
/**
 * @brief Applies a volume preset to the player, demo, and optional sub keepers.
 * @param pName Volume preset name.
 * @param fadeFrames Volume transition duration in update frames.
 */
void SeDirector::setAllKeeperVolumeSettingExceptMain(const char* pName, s32 fadeFrames) {
    mPlayerKeeper->setVolumeSetting(pName, fadeFrames);
    mDemoKeeper->setVolumeSetting(pName, fadeFrames);
    if (mSubKeeper != nullptr) {
        mSubKeeper->setVolumeSetting(pName, fadeFrames);
    }
}
/**
 * @brief Applies a volume preset to every keeper.
 * @param pName Volume preset name.
 * @param fadeFrames Volume transition duration in update frames.
 */
void SeDirector::setAllKeeperVolumeSetting(const char* pName, s32 fadeFrames) {
    mMainKeeper->setVolumeSetting(pName, fadeFrames);
    setAllKeeperVolumeSettingExceptMain(pName, fadeFrames);
}
/** @brief Enters a demo sound policy. @param type Demo policy selector; stage demo is zero. */
NOINLINE void SeDirector::startDemo(alSeFunction::DemoType type) {
    switch (type) {
    case 0:
        mMainKeeper->stopAllTrigSe(0);
        mMainKeeper->pauseSystem(true, "デモ", 80);
        mPlayerKeeper->setVolumeSetting("ステージデモ", 80);
        mIsInDemo = true;
        return;
    case 1:
        mMainKeeper->setVolumeSetting("SystemDemo", 30);
        break;
    case 2:
        mMainKeeper->setVolumeSetting("AtmosphereDemo", 30);
        break;
    case 3:
        mMainKeeper->setVolumeSetting("SystemAtmosphereDemo", 30);
        break;
    case 4:
        mIsInDemo = true;
        return;
    default:
        break;
    }
    setAllKeeperVolumeSettingExceptMain("SystemVoiceDemo", 30);
    mMainKeeper->stopSeForCameraDemo(type);
    mIsInDemo = true;
}
/** @brief Leaves a demo sound policy and restores normal volumes. @param type Policy being ended; four is
 * ignored. */
NOINLINE void SeDirector::endDemo(alSeFunction::DemoType type) {
    switch (type) {
    case 4:
        return;
    case 0: {
        mMainKeeper->pauseSystem(false, "デモ", 80);
        mPlayerKeeper->setVolumeSetting("通常", 40);
        break;
    }
    default: {
        mMainKeeper->startPausedSeFromCameraDemo(type);
        setAllKeeperVolumeSetting("通常", 10);
        break;
    }
    }
    mDemoKeeper->stopAll(30, nullptr);
    mIsInDemo = false;
}
/**
 * @brief Changes between demo sound policies.
 * @param from Policy to leave.
 * @param to Policy to enter.
 */
void SeDirector::changeDemo(alSeFunction::DemoType from, alSeFunction::DemoType to) {
    endDemo(from);
    startDemo(to);
}
/** @brief Sets post-goal volume behavior in every keeper. @param isAfterGoal Whether the stage goal has been
 * reached. */
void SeDirector::setIsStateAfterGoal(bool isAfterGoal) {
    mMainKeeper->setIsStateAfterGoal(isAfterGoal);
    if (mSubKeeper != nullptr) {
        mSubKeeper->setIsStateAfterGoal(isAfterGoal);
    }
    mDemoKeeper->setIsStateAfterGoal(isAfterGoal);
    mPlayerKeeper->setIsStateAfterGoal(isAfterGoal);
}
/** @brief Enables promotional-playback sound filtering in every keeper. */
void SeDirector::setIsExcludeCmNgSe() {
    forEachRequestKeeper([](SeRequestKeeper* pKeeper) { pKeeper->setIsExcludeCmNgSe(); });
}
/**
 * @brief Changes the volume preset of a named keeper.
 * @param pKeeperName Routing name of the keeper.
 * @param pSettingName Volume preset name.
 * @param fadeFrames Transition duration in update frames.
 * @param isForce When true, the original implementation suppresses this change during a demo.
 */
void SeDirector::setVolumeSetting(const char* pKeeperName, const char* pSettingName, s32 fadeFrames,
                                  bool isForce) {
    if (isForce && mIsInDemo) {
        return;
    }
    SeRequestKeeper* pKeeper = findRequestKeeper(pKeeperName);
    if (pKeeper != nullptr) {
        pKeeper->setVolumeSetting(pSettingName, fadeFrames);
    }
}
/** @brief Replaces listener parameters. @param rParam Initialized listener parameters to copy. */
void SeDirector::changeListenerParam(sead::Audio3DListenerParameterNin& rParam) {
    mListenerKeeper->changeListenerParam(rParam);
}
/** @brief Restores default listener parameters. */
void SeDirector::resetListenerParam() { mListenerKeeper->resetListenerParam(); }
/** @brief Selects a listener-position policy. @param pName Registered poser name. */
void SeDirector::changeListenerPoser(const char* pName) { mListenerKeeper->changeListenerPoser(pName); }
/** @brief Restores the previously selected listener-position policy. */
void SeDirector::changeListenerPoserToLast() { mListenerKeeper->changeListenerPoserToLast(); }

/**
 * @brief Selects an explicit routing name or the current main/demo keeper.
 * @param pName Routing name, or nullptr to follow the current demo state.
 * @return Selected keeper, or nullptr when an explicit name is not found.
 */
inline SeRequestKeeper* SeDirector::selectRequestKeeper(const char* pName) const {
    return pName != nullptr ? findRequestKeeper(pName) : (mIsInDemo ? mDemoKeeper : mMainKeeper);
}
/**
 * @brief Applies the material-dependent low-pass filter to newly requested playback.
 * @param pParams Playback parameters, or nullptr when the request was rejected.
 * @param pSpecificInfo Non-null resource settings controlling material filtering.
 * @param waterState Water-state selector; two selects the underwater filter.
 * @param isBeyondWall Whether the source is occluded by a wall.
 */
static inline void applyMaterialLpf(SePlayParamList* pParams, const SeResourceSpecificInfo* pSpecificInfo,
                                    s32 waterState, bool isBeyondWall) {
    if (waterState != 2 && !isBeyondWall) {
        return;
    }
    if (pParams == nullptr || !pSpecificInfo->mIsValidMatCodeLpf) {
        return;
    }
    if (waterState == 2) {
        pParams->setLpfFreq(-0.39f);
    } else {
        pParams->setLpfFreq(-0.34f);
    }
}
/**
 * @brief Creates a routed sound request with material substitution and musical-effect parameters.
 * @param id Original sound identifier.
 * @param pSource Source attached to the request.
 * @param isLoop Whether the requested sound should loop.
 * @param pSpecificInfo Resource settings; nullptr rejects the request.
 * @param pMeInfo Musical-effect state passed to parameter selection when the sound is a musical effect.
 * @param pMaterialName Surface material name, or nullptr for no surface substitution.
 * @param waterState Water-state selector; nonzero enables material substitution and two enables underwater
 * filtering.
 * @param isBeyondWall Whether wall occlusion should apply material filtering.
 * @param pPlayName Keeper routing name, or nullptr for the current main/demo keeper.
 * @return Mutable playback parameters, or nullptr when the request is rejected.
 */
SePlayParamList* SeDirector::addRequest(u32 id, SeSource* pSource, bool isLoop,
                                        const SeResourceSpecificInfo* pSpecificInfo, MeInfo* pMeInfo,
                                        const char* pMaterialName, s32 waterState, bool isBeyondWall,
                                        const char* pPlayName) {
    SeRequestKeeper* pKeeper = selectRequestKeeper(pPlayName);
    if (pSpecificInfo == nullptr || pKeeper == nullptr) {
        return nullptr;
    }
    if (pMaterialName != nullptr || waterState != 0) {
        id = mMaterialInfoKeeper->findReplacedId(id, pMaterialName, waterState, pSpecificInfo);
    }
    const AudioMixVolume* pMixVolume = nullptr;
    if (mCategoryParamsController != nullptr) {
        pMixVolume = mCategoryParamsController->getMixVolume(pSpecificInfo->mPlayerId);
        if (pMixVolume == nullptr) {
            return nullptr;
        }
    }
    SePlayParamList* pParams = pKeeper->addRequest(id, pSource, isLoop, pSpecificInfo, pMixVolume);
    const bool isMe = mMeInfoKeeper->isMe(id);
    if (pParams != nullptr && isMe) {
        mMeInfoKeeper->applyMeInfoToParams(alSoundNameUtil::getSoundName(id, false), pParams, pMeInfo);
    }
    applyMaterialLpf(pParams, pSpecificInfo, waterState, isBeyondWall);
    return pParams;
}
/**
 * @brief Extends or starts a held sound request, keeping musical-effect lookup tied to its original ID.
 * @param id Original sound identifier used for musical-effect lookup.
 * @param pSource Source attached to the request.
 * @param pSpecificInfo Resource settings; nullptr rejects the request.
 * @param pMeInfo Musical-effect state passed to parameter selection when applicable.
 * @param pMaterialName Surface material name, or nullptr for no surface substitution.
 * @param waterState Water-state selector; nonzero enables substitution and two enables underwater filtering.
 * @param isBeyondWall Whether wall occlusion should apply material filtering.
 * @param pPlayName Keeper routing name, or nullptr for the current main/demo keeper.
 * @return Mutable playback parameters, or nullptr when the request is rejected.
 */
SePlayParamList* SeDirector::addHoldRequest(u32 id, SeSource* pSource,
                                            const SeResourceSpecificInfo* pSpecificInfo, MeInfo* pMeInfo,
                                            const char* pMaterialName, s32 waterState, bool isBeyondWall,
                                            const char* pPlayName) {
    SeRequestKeeper* pKeeper = selectRequestKeeper(pPlayName);
    if (pSpecificInfo == nullptr || pKeeper == nullptr) {
        return nullptr;
    }
    u32 replacedId = id;
    if (pMaterialName != nullptr || waterState != 0) {
        replacedId = mMaterialInfoKeeper->findReplacedId(id, pMaterialName, waterState, pSpecificInfo);
    }
    const AudioMixVolume* pMixVolume = nullptr;
    if (mCategoryParamsController != nullptr) {
        pMixVolume = mCategoryParamsController->getMixVolume(pSpecificInfo->mPlayerId);
        if (pMixVolume == nullptr) {
            return nullptr;
        }
    }
    SePlayParamList* pParams = pKeeper->addHoldRequest(replacedId, pSource, pSpecificInfo, pMixVolume);
    const bool isMe = mMeInfoKeeper->isMe(id);
    if (pParams != nullptr && isMe) {
        mMeInfoKeeper->applyMeInfoToParams(alSoundNameUtil::getSoundName(id, false), pParams, pMeInfo);
    }
    applyMaterialLpf(pParams, pSpecificInfo, waterState, isBeyondWall);
    return pParams;
}
/**
 * @brief Stops a sound in the explicitly named or current main/demo keeper.
 * @param id Sound identifier to stop.
 * @param pSource Source attached to the sound.
 * @param fadeFrames Fade duration in update frames.
 * @param pPlayName Keeper routing name, or nullptr for current main/demo routing.
 */
void SeDirector::stop(u32 id, SeSource* pSource, u32 fadeFrames, const char* pPlayName) {
    SeRequestKeeper* pKeeper = selectRequestKeeper(pPlayName);
    if (pKeeper != nullptr) {
        pKeeper->stop(id, pSource, fadeFrames);
    }
}
/**
 * @brief Stops a sound across the primary keepers or one explicitly named keeper.
 * @param id Sound identifier to stop.
 * @param pSource Source attached to the sound.
 * @param fadeFrames Fade duration in update frames.
 * @param pPlayName Keeper routing name, or nullptr to stop demo, main, and player requests.
 */
void SeDirector::stopAllId(u32 id, SeSource* pSource, u32 fadeFrames, const char* pPlayName) {
    if (pPlayName == nullptr) {
        mDemoKeeper->stop(id, pSource, fadeFrames);
        mMainKeeper->stop(id, pSource, fadeFrames);
        mPlayerKeeper->stop(id, pSource, fadeFrames);
    } else {
        SeRequestKeeper* pKeeper = findRequestKeeper(pPlayName);
        if (pKeeper != nullptr) {
            pKeeper->stop(id, pSource, fadeFrames);
        }
    }
}
} // namespace al

namespace al {
/** @brief Creates an empty musical-effect information keeper. */
NOINLINE MeInfoKeeper::MeInfoKeeper() = default;
/**
 * @brief Finds the musical-effect parameter list associated with a sound name.
 * @param pName Sound name, or nullptr to return no match.
 * @return First named list matching pName, or nullptr when absent.
 */
MeInfoList* MeInfoKeeper::tryFindMeInfoList(const char* pName) const {
    if (pName == nullptr) {
        return nullptr;
    }
    for (s32 i = 0; i < mListNum; ++i) {
        if (mLists[i]->mName != nullptr && isEqualString(mLists[i]->mName, pName)) {
            return mLists[i];
        }
    }
    return nullptr;
}
/**
 * @brief Finds the musical-effect parameter list associated with a sound ID.
 * @param soundId Archive sound ID; the invalid ID never matches.
 * @return First valid list with this sound ID, or nullptr when absent.
 */
MeInfoList* MeInfoKeeper::tryFindMeInfoListById(s32 soundId) const {
    if (AudioConst::SOUND_ID_INVALID == soundId) {
        return nullptr;
    }
    for (s32 i = 0; i < mListNum; ++i) {
        if (mLists[i]->mSoundId != AudioConst::SOUND_ID_INVALID && mLists[i]->mSoundId == soundId) {
            return mLists[i];
        }
    }
    return nullptr;
}
/**
 * @brief Tests whether a sound has musical-effect parameters.
 * @param soundId Archive sound ID; the invalid ID returns false.
 * @return True when a matching musical-effect list exists.
 */
NOINLINE bool MeInfoKeeper::isMe(s32 soundId) const { return tryFindMeInfoListById(soundId) != nullptr; }
} // namespace al

#include "Library/Bgm/BgmRhythmCtrl.hpp"
#include "Library/Bgm/BgmMusicalInfo.hpp"
#include "Library/Resource/Resource.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Yaml/ByamlIter.hpp"

namespace al {
extern s32 gStaticMeInfoNum;
extern MeInfoList gStaticMeInfoList[];

/**
 * @brief Reads one musical-effect entry field, defaulting an absent value to zero.
 * @param pList List owning the entry array; must be initialized.
 * @param index Valid entry-array index.
 * @param pField Integer field to populate in that entry.
 * @param rIter BYAML record containing the entry parameters.
 * @param pKey BYAML key for the integer field.
 */
static inline void readMeEntryValue(MeInfoList* pList, s32 index, s32 MeInfoEntry::* pField,
                                    const ByamlIter& rIter, const char* pKey) {
    if (!rIter.tryGetIntByKey(&(pList->mEntries[index].*pField), pKey)) {
        pList->mEntries[index].*pField = 0;
    }
}
/**
 * @brief Creates one musical-effect parameter list from a BYAML record.
 * @param rIter Record containing a sound name and local-variable entries.
 * @return Newly allocated parameter list.
 */
static inline MeInfoList* createMeInfoList(const ByamlIter& rIter) {
    MeInfoList* pList = new MeInfoList;
    rIter.tryGetStringByKey(&pList->mName, "Name");
    if (pList->mName != nullptr) {
        pList->mSoundId = alSoundNameUtil::getSoundId(pList->mName, false);
    }
    ByamlIter variables;
    rIter.tryGetIterByKey(&variables, "LocalVariableList");
    const s32 entryNum = variables.getSize();
    pList->mEntries = new MeInfoEntry[entryNum];
    pList->mEntryNum = entryNum;
    for (s32 j = 0; j < entryNum; ++j) {
        ByamlIter entry;
        variables.tryGetIterByIndex(&entry, j);
        readMeEntryValue(pList, j, &MeInfoEntry::mVariable, entry, "Var");
        readMeEntryValue(pList, j, &MeInfoEntry::mChord, entry, "Chord");
        readMeEntryValue(pList, j, &MeInfoEntry::mScale, entry, "Scale");
        readMeEntryValue(pList, j, &MeInfoEntry::mPitch, entry, "Pitch");
    }
    return pList;
}
/**
 * @brief Loads musical-effect lists and appends the built-in sound mappings.
 * @param pRhythmCtrl Rhythm controller used to select notes from the current background music.
 */
void MeInfoKeeper::init(BgmRhythmCtrl* pRhythmCtrl) {
    Resource* pResource = findOrCreateResource("SoundData/MeData", nullptr);
    const u8* pData = pResource->getByml("MeData");
    ByamlIter root(pData);
    const s32 resourceNum = root.getSize();
    mListNum = resourceNum + gStaticMeInfoNum;
    if (mListNum > 0) {
        mLists = new MeInfoList*[mListNum];
        for (s32 i = 0; i < resourceNum; ++i) {
            ByamlIter iter;
            if (!root.tryGetIterByIndex(&iter, i)) {
                continue;
            }
            MeInfoList* pList = createMeInfoList(iter);
            mLists[i] = pList;
        }
        for (s32 i = 0; i < gStaticMeInfoNum; ++i) {
            mLists[resourceNum + i] = &gStaticMeInfoList[i];
            if (gStaticMeInfoList[i].mName != nullptr) {
                gStaticMeInfoList[i].mSoundId =
                    alSoundNameUtil::getSoundId(gStaticMeInfoList[i].mName, false);
            }
        }
    }
    mRhythmCtrl = pRhythmCtrl;
}
/**
 * @brief Converts a chord degree to its semitone offset, extending higher degrees by octaves.
 * @param rChord Current chord; chordNum must be positive for a nonnegative degree.
 * @param degree Chord degree, or a negative value to use a zero semitone offset.
 * @return Chord tone in semitones including any octave displacement.
 */
static inline s32 calcMeChordPitch(const BgmChordInfo& rChord, s32 degree) {
    if (degree < 0) {
        return 0;
    }
    s32 octavePitch = 0;
    if (degree >= rChord.chordNum) {
        octavePitch = degree / rChord.chordNum * 12;
        degree %= rChord.chordNum;
    }
    return rChord.chord[degree] + octavePitch;
}
/**
 * @brief Finds the first scale tone at or above a requested pitch within an octave range.
 * @param pIndex Receives the scale degree, or zero when no tone was found.
 * @param pOctave Receives the octave offset, or zero when no tone was found.
 * @param rChord Current chord and its ascending scale tones.
 * @param pitch Lower bound in semitones.
 */
static inline void findMeScaleTone(s32* pIndex, s32* pOctave, const BgmChordInfo& rChord, s32 pitch) {
    *pIndex = 0;
    *pOctave = 0;
    for (s32 octave = -1; octave < 8; ++octave) {
        for (s32 i = 0; i < rChord.scaleNum; ++i) {
            if (rChord.scale[i] + octave * 12 >= pitch) {
                *pIndex = i;
                *pOctave = octave;
                return;
            }
        }
    }
}
/**
 * @brief Resolves musical-effect notes against the current BGM chord and writes sequence variables.
 * @param pList Musical-effect entry list, or nullptr to perform no work.
 * @param pParams Destination playback parameters; required when entries are processed.
 * @param pInfo Optional per-play chord, scale, and pitch offsets.
 * @param pName Unused diagnostic sound name.
 */
NOINLINE void MeInfoKeeper::applyMeInfoToParams(MeInfoList* pList, SePlayParamList* pParams, MeInfo* pInfo,
                                                const char* pName) {
    if (pList == nullptr || !mRhythmCtrl->isEnableRhythmAnim()) {
        return;
    }
    const BgmChordInfo* pChord = mRhythmCtrl->getChordInfoCurrent();
    mRhythmCtrl->getCurrentBpm();
    if (pChord == nullptr) {
        return;
    }
    for (s32 i = 0; i < pList->mEntryNum; ++i) {
        s32 degree = pList->mEntries[i].mChord;
        s32 scaleOffset = pList->mEntries[i].mScale;
        s32 scaleIndex = 0;
        s32 octave = 0;
        if (pInfo != nullptr) {
            if (degree < 0 && pInfo->mChordOffset < 0) {
                degree = -1;
            } else {
                degree = (degree < 0 ? 0 : degree) + (pInfo->mChordOffset < 0 ? 0 : pInfo->mChordOffset);
            }
            const s32 pitch = calcMeChordPitch(*pChord, degree);
            findMeScaleTone(&scaleIndex, &octave, *pChord, pitch + pInfo->mPitchOffset);
            scaleOffset += scaleIndex;
            scaleOffset += pInfo->mScaleOffset;
            scaleOffset += octave * pChord->scaleNum;
        } else {
            const s32 pitch = calcMeChordPitch(*pChord, degree);
            if (pitch >= 0) {
                for (s32 j = 0; j < pChord->scaleNum; ++j) {
                    if (pChord->scale[j] >= pitch) {
                        scaleIndex = j;
                        break;
                    }
                }
            }
            scaleOffset += scaleIndex;
        }
        octave = scaleOffset / pChord->scaleNum;
        if (scaleOffset < 0) {
            const s32 octaveCorrection = -scaleOffset / 12 + 1;
            scaleOffset += octaveCorrection * 12;
            octave -= octaveCorrection;
        }
        const s32 pitch = pChord->scale[scaleOffset % pChord->scaleNum] + octave * 12;
        pParams->setLocalVariable(pitch, pList->mEntries[i].mVariable);
    }
}
/**
 * @brief Applies a named musical-effect list to playback parameters.
 * @param pName Sound name, or nullptr to select no list.
 * @param pParams Destination playback parameters.
 * @param pInfo Optional per-play musical offsets.
 */
NOINLINE void MeInfoKeeper::applyMeInfoToParams(const char* pName, SePlayParamList* pParams, MeInfo* pInfo) {
    applyMeInfoToParams(tryFindMeInfoList(pName), pParams, pInfo, pName);
}
/**
 * @brief Applies a musical-effect list selected by archive sound ID.
 * @param soundId Archive sound ID; the invalid ID selects no list.
 * @param pParams Destination playback parameters.
 * @param pInfo Optional per-play musical offsets.
 */
void MeInfoKeeper::applyMeInfoToParams(s32 soundId, SePlayParamList* pParams, MeInfo* pInfo) {
    applyMeInfoToParams(tryFindMeInfoListById(soundId), pParams, pInfo, nullptr);
}

namespace {
MeInfoEntry sFlowerTouched[] = {
    MeInfoEntry(0, 1, -1, 0),
    MeInfoEntry(1, 1, 0, 0),
    MeInfoEntry(2, 2, 0, 0),
};

MeInfoEntry sRouteDokanMove[] = {
    MeInfoEntry(0, 1, -1, 0),
    MeInfoEntry(1, 1, 0, 0),
    MeInfoEntry(2, 2, 0, 0),
    MeInfoEntry(3, 3, 0, 0),
};

MeInfoEntry sTouchPointLevel[] = {
    MeInfoEntry(0, -1, 0, 0),
    MeInfoEntry(1, -1, 1, 0),
    MeInfoEntry(2, -1, 2, 0),
};

MeInfoEntry sPanelNoteLittleHit[] = {
    MeInfoEntry(0, -1, 0, 0),
};

MeInfoEntry sJumpPanelJump[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 1, 0, 0),
    MeInfoEntry(2, 2, 0, 0),
};

MeInfoEntry sCooporateHipDrop[] = {
    MeInfoEntry(3, 0, 0, 0),
};

MeInfoEntry sEchoBlock[] = {
    MeInfoEntry(3, 0, 1, 0),
    MeInfoEntry(4, 1, 0, 0),
    MeInfoEntry(5, 2, 0, 0),
    MeInfoEntry(6, 2, 1, 0),
    MeInfoEntry(7, 2, 2, 0),
    MeInfoEntry(8, 2, 3, 0),
    MeInfoEntry(9, 2, 4, 0),
};

MeInfoEntry sEchoBlockDrcTouched[] = {
    MeInfoEntry(3, 0, 1, 0),
    MeInfoEntry(4, 1, 0, 0),
    MeInfoEntry(5, 2, 0, 0),
};

MeInfoEntry sTouchPointTouchNormal[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 2, 0, 0),
};

MeInfoEntry sDashPanelDash[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 1, 1, 0),
    MeInfoEntry(5, 1, 2, 0),
};

MeInfoEntry sRaidonDashPanelDash[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 1, 1, 0),
    MeInfoEntry(5, 1, 2, 0),
};

MeInfoEntry sTrapezeSwing[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 0, 1, 0),
    MeInfoEntry(2, 0, 2, 0),
    MeInfoEntry(3, 0, 3, 0),
    MeInfoEntry(4, 0, 4, 0),
};

MeInfoEntry sTrapezeCatch[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 2, 0, 0),
    MeInfoEntry(5, 2, 1, 0),
    MeInfoEntry(6, 2, 2, 0),
};

MeInfoEntry sTrapezeJump[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 0, 1, 0),
    MeInfoEntry(5, 2, 0, 0),
};

MeInfoEntry sRouteDokanBazookaLaunch[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 1, 0, 0),
    MeInfoEntry(5, 2, 0, 0),
    MeInfoEntry(6, 3, 0, 0),
};

MeInfoEntry sRouteDokanBazookaPrapare[] = {
    MeInfoEntry(3, 0, 0, 0),
    MeInfoEntry(4, 0, 1, 0),
    MeInfoEntry(5, 2, 0, 0),
    MeInfoEntry(6, 3, 0, 0),
};

MeInfoEntry sDrcBlock[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 0, 1, 0),
    MeInfoEntry(2, 2, 0, 0),
    MeInfoEntry(3, 2, 1, 0),
};

MeInfoEntry sGondola[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 0, 1, 0),
    MeInfoEntry(2, 1, 0, 0),
    MeInfoEntry(3, 2, 0, 0),
    MeInfoEntry(4, 2, 1, 0),
};

MeInfoEntry sBellTreeJump[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 0, 1, 0),
    MeInfoEntry(2, 2, 0, 0),
};

MeInfoEntry sNeonSignOn[] = {
    MeInfoEntry(0, 0, 0, 0),
    MeInfoEntry(1, 0, 1, 0),
    MeInfoEntry(2, 0, 2, 0),
    MeInfoEntry(3, 2, 0, 0),
};

}  // namespace

MeInfoList gStaticMeInfoList[] = {
    MeInfoList("SeMeFlowerTouched", sFlowerTouched, 3),
    MeInfoList("SeMeRouteDokanMoveLv", sRouteDokanMove, 4),
    MeInfoList("SeMeTouchPointTouchedNormalLv", sTouchPointLevel, 3),
    MeInfoList("SeMeTouchPointBloomLv", sTouchPointLevel, 3),
    MeInfoList("SeOjPanelNoteLittleHit", sPanelNoteLittleHit, 1),
    MeInfoList("SeMeJumpPanelJump", sJumpPanelJump, 3),
    MeInfoList("SeMeJumpPanelJumpBak", sJumpPanelJump, 3),
    MeInfoList("SeMeCooporateHipDrop", sCooporateHipDrop, 1),
    MeInfoList("SeMeEchoBlockHipDropped", sEchoBlock, 7),
    MeInfoList("SeMeEchoBlockHipDroppedSingleMode", sEchoBlock, 7),
    MeInfoList("SeMeEchoBlockDrcTouched", sEchoBlockDrcTouched, 3),
    MeInfoList("SePfStepEchoBlockL", sEchoBlock, 7),
    MeInfoList("SePfStepEchoBlockR", sEchoBlock, 7),
    MeInfoList("SePfRunEchoBlockL", sEchoBlock, 7),
    MeInfoList("SePfRunEchoBlockR", sEchoBlock, 7),
    MeInfoList("SePfStepWetEchoBlockL", sEchoBlock, 7),
    MeInfoList("SePfStepWetEchoBlockR", sEchoBlock, 7),
    MeInfoList("SePfRunWetEchoBlockL", sEchoBlock, 7),
    MeInfoList("SePfRunWetEchoBlockR", sEchoBlock, 7),
    MeInfoList("SeMeTouchPointTouchNormal", sTouchPointTouchNormal, 2),
    MeInfoList("SeMeDashPanelDash", sDashPanelDash, 3),
    MeInfoList("SeMeRaidonDashPanelDash", sRaidonDashPanelDash, 3),
    MeInfoList("SeMeRaidonSurfDashPanelDash", sRaidonDashPanelDash, 3),
    MeInfoList("SeMeTrapezeSwing", sTrapezeSwing, 5),
    MeInfoList("SeMeTrapezeSwingBack", sTrapezeSwing, 5),
    MeInfoList("SeMeTrapezeCatch", sTrapezeCatch, 4),
    MeInfoList("SeMeTrapezeJump", sTrapezeJump, 3),
    MeInfoList("SeMeDrcBlockGoFoward", sDrcBlock, 4),
    MeInfoList("SeMeDrcBlockBackSign", sDrcBlock, 4),
    MeInfoList("SeSyDrcBlockMoveStart", sDrcBlock, 4),
    MeInfoList("SeSyDrcBlockMoveBackStart", sDrcBlock, 4),
    MeInfoList("SeMeGondolaMoveLv", sGondola, 5),
    MeInfoList("SeMeGondolaStart", sGondola, 5),
    MeInfoList("SeMeGondolaEnd", sGondola, 5),
    MeInfoList("SeMeBellTreeJump", sBellTreeJump, 3),
    MeInfoList("SeMeRouteDokanBazookaLaunch", sRouteDokanBazookaLaunch, 4),
    MeInfoList("SeMeRouteDokanBazookaPrapare", sRouteDokanBazookaPrapare, 4),
    MeInfoList("SeOjNeonSignOn", sNeonSignOn, 4),
};
// Originally defined beside the table in its own source file; WEAK keeps MeInfoKeeper::init() from
// assuming its value, as it could not in the original build.
WEAK s32 gStaticMeInfoNum = sizeof(gStaticMeInfoList) / sizeof(gStaticMeInfoList[0]);

}  // namespace al

#include <audio/seadAudioFxDelayNin.h>
#include <audio/seadAudioFxI3DL2ReverbNin.h>
#include <audio/seadAudioFxReverbHiNin.h>
#include <audio/seadAudioFxReverbStdNin.h>
#include <basis/seadNew.h>
#include <nn/atk/atk_SoundPlayer.h>
#include <nn/atk/atk_SoundSystem.h>

#include "Library/Bgm/DspValueController.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Library/Se/Function/SeEffectController.hpp"
#include "Project/Audio/System/AudioEffectDataBase.hpp"
#include "Project/Audio/System/AudioEffectFunction.hpp"
#include "Project/Audio/System/AudioPlayer.hpp"

namespace al {
/** @brief Delay parameters filled from an effect-process record. */
class SeadFxDelayParams : public sead::AudioFxDelayParamNin {
public:
    void setParams(const SeEffectProcInfo* pInfo);
};

/** @brief Standard reverb parameters filled from an effect-process record. */
class SeadFxReverbStdParams : public sead::AudioFxReverbStdParamNin {
public:
    void setParams(const SeEffectProcInfo* pInfo);
};

/** @brief High-quality reverb parameters filled from an effect-process record. */
class SeadFxReverbHiParams : public sead::AudioFxReverbHiParamNin {
public:
    void setParams(const SeEffectProcInfo* pInfo);
};

/** @brief I3DL2 reverb parameters filled from an effect-process record. */
class SeadFxReverbI3Dl2Params : public sead::AudioFxI3DL2ReverbParamNin {
public:
    void setParams(const SeEffectProcInfo* pInfo);
};

/**
 * @brief One effect instance attached to one aux bus.
 * @tparam Fx sead effect implementation.
 * @tparam Params Parameter block that can be filled from an effect-process record.
 */
template <typename Fx, typename Params>
class SoundFxUnit {
public:
    /**
     * @brief Allocates the effect, its parameters and its work buffer.
     * @param pBusName Aux bus name the effect is appended to.
     * @param pDeviceName Output device name.
     */
    SoundFxUnit(const char* pBusName, const char* pDeviceName)
        : mBusName(pBusName), mDeviceName(pDeviceName) {
        mFx = new (8) Fx;
        mParams = new Params;
        mWorkBufferSize = (mWorkBufferSize + 0xfff) & ~0xfff;
        mWorkBuffer = new (0x1000) u8[mWorkBufferSize];
    }

    void appendEffect(const SeEffectProcInfo* pInfo);

    /** @brief Disables the effect processing. */
    void disable() {
        if (mFx != nullptr) {
            mFx->SetEnabled(false);
        }
    }

    /**
     * @brief Tests whether the effect can be removed from its bus.
     * @return True when the effect is not appended or became removable.
     */
    bool isDisabled() {
        if (!mIsAppended || mFx == nullptr || mIsRemovable == true) {
            return true;
        }

        mIsRemovable = mFx->IsRemovable();
        return mIsRemovable;
    }

    /** @brief Releases the work buffer of an appended effect. */
    void releaseWorkBuffer() {
        if (mIsAppended) {
            mFx->ReleaseWorkBuffer();
            mProcInfo = nullptr;
            mIsAppended = false;
        }
    }

private:
    Fx* mFx = nullptr;
    Params* mParams = nullptr;
    u8* mWorkBuffer = nullptr;
    s32 mWorkBufferSize = 0x40000;
    const char* mBusName;
    const char* mDeviceName;
    bool mIsAppended = false;
    u8 mIsRemovable = true;  // Compared against true as a byte, so not a bool.
    const SeEffectProcInfo* mProcInfo = nullptr;
};

/**
 * @brief Per-bus table of effect units of one kind.
 * @tparam Unit Effect unit type.
 */
template <typename Unit>
struct SoundFxUnitTable {
    static const s32 cBusNum = 3;

    /** @brief Disables every unit in the table. */
    void disable() {
        for (s32 i = 0; i < cBusNum; i++) {
            if (mUnits[i] != nullptr) {
                mUnits[i]->disable();
            }
        }
    }

    /** @brief Tests whether every unit can be removed. @return True when all units are disabled. */
    ALWAYS_INLINE bool isDisabled() {
        for (s32 i = 0; i < cBusNum; i++) {
            if (mUnits[i] != nullptr && !mUnits[i]->isDisabled()) {
                return false;
            }
        }

        return true;
    }

    /** @brief Releases the work buffers of every unit. */
    void releaseWorkBuffer() {
        for (s32 i = 0; i < cBusNum; i++) {
            if (mUnits[i] != nullptr) {
                mUnits[i]->releaseWorkBuffer();
            }
        }
    }

    Unit** mUnits;
};

/**
 * @brief Copies delay settings from an effect-process record.
 * @param pInfo Delay effect-process record.
 */
void SeadFxDelayParams::setParams(const SeEffectProcInfo* pInfo) {
    auto* pDelay = static_cast<const SeDelayEffectProcInfo*>(pInfo);
    mDelayTime = pDelay->mDelayTime;
    mFeedbackGain = pDelay->mFeedbackGain;
    mOutGain = pDelay->mOutGain;
    mLpfAmount = pDelay->mLpfCutoffFreq;
    mChannelCount = pDelay->mMaxChannels;
    mSampleRate = pDelay->mSampleRate;
    _20 = pDelay->mIsUseTaskThread;
    _24 = pDelay->mNumOfWaveBuffer;
    _28 = pDelay->mNumOfPreloadWaveBuffer;
}

/**
 * @brief Copies standard reverb settings from an effect-process record.
 * @param pInfo Standard reverb effect-process record.
 */
void SeadFxReverbStdParams::setParams(const SeEffectProcInfo* pInfo) {
    auto* pReverb = static_cast<const SeReverbStdEffectProcInfo*>(pInfo);
    mPreDelayTime = pReverb->mPreDelayTime;
    mDecayTime = pReverb->mFusedTime;
    mColoration = pReverb->mColoration;
    mLpfAmount = pReverb->mDamping;
    mOutGain = pReverb->mOutGain;
    mEarlyMode = static_cast<EarlyMode>(pReverb->mEarlyMode);
    mFusedMode = pReverb->mFusedMode;
    mEarlyGain = pReverb->mEarlyGain;
    mFusedGain = pReverb->mFusedGain;
    mChannelCount = pReverb->mMaxChannels;
    mSampleRate = pReverb->mSampleRate;
    _34 = pReverb->mIsUseTaskThread;
    _38 = pReverb->mNumOfWaveBuffer;
    _3c = pReverb->mNumOfPreloadWaveBuffer;
}

/**
 * @brief Copies high-quality reverb settings from an effect-process record.
 * @param pInfo High-quality reverb effect-process record.
 */
void SeadFxReverbHiParams::setParams(const SeEffectProcInfo* pInfo) {
    auto* pReverb = static_cast<const SeReverbHiEffectProcInfo*>(pInfo);
    mPreDelayTime = pReverb->mPreDelayTime;
    mDecayTime = pReverb->mFusedTime;
    mColoration = pReverb->mColoration;
    mLpfAmount = pReverb->mDamping;
    mCrossTalk = pReverb->mCrosstalk;
    mOutGain = pReverb->mOutGain;
    mEarlyMode = pReverb->mEarlyMode;
    mFusedMode = pReverb->mFusedMode;
    mEarlyGain = pReverb->mEarlyGain;
    mFusedGain = pReverb->mFusedGain;
    mSampleRate = pReverb->mSampleRate;
    _34 = pReverb->mIsUseTaskThread;
    _38 = pReverb->mNumOfWaveBuffer;
    _3c = pReverb->mNumOfPreloadWaveBuffer;
}

/**
 * @brief Copies I3DL2 reverb settings from an effect-process record.
 * @param pInfo I3DL2 reverb effect-process record.
 */
void SeadFxReverbI3Dl2Params::setParams(const SeEffectProcInfo* pInfo) {
    auto* pReverb = static_cast<const SeReverbI3Dl2EffectProcInfo*>(pInfo);
    mRoom = pReverb->mRoom;
    mRoomHf = pReverb->mRoomHf;
    mDecayTime = pReverb->mDecayTime;
    mDecayHfRatio = pReverb->mDecayHfRatio;
    mReflections = pReverb->mReflections;
    mReflectionsDelay = pReverb->mReflectionsDelay;
    mReverb = pReverb->mReverb;
    mReverbDelay = pReverb->mReverbDelay;
    mDensity = pReverb->mDiffusion;
    mDiffusion = pReverb->mDensity;
    mHfReference = pReverb->mHfReference;
    mEarlyMode = static_cast<EarlyMode>(pReverb->mEarlyMode);
    mFusedMode = pReverb->mFusedMode;
    mChannelCount = pReverb->mMaxChannels;
    mSampleRate = pReverb->mSampleRate;
    _44 = pReverb->mIsUseTaskThread;
    _48 = pReverb->mNumOfWaveBuffer;
    _4c = pReverb->mNumOfPreloadWaveBuffer;
}

/**
 * @brief Applies the record's parameters and appends the effect to its aux bus.
 * @param pInfo Effect-process record matching this unit's effect kind.
 */
template <typename Fx, typename Params>
void SoundFxUnit<Fx, Params>::appendEffect(const SeEffectProcInfo* pInfo) {
    mProcInfo = pInfo;
    mParams->setParams(pInfo);
    mFx->SetParam(*mParams);
    if (static_cast<s32>(mFx->GetRequiredMemSize()) > mWorkBufferSize) {
        return;
    }

    mFx->AssignWorkBuffer(mWorkBuffer, mWorkBufferSize);
    mFx->SetEnabled(true);
    const auto bus = static_cast<nn::atk::AuxBus>(alAudioEffectFunction::getBusId(mBusName));
    const auto device =
        static_cast<nn::atk::OutputDevice>(alAudioEffectFunction::getOutDeviceId(mDeviceName));
    if (nn::atk::SoundSystem::AppendEffect(bus, mFx, mFx->getWorkBuffer(), mFx->getWorkBufferSize(),
                                           device)) {
        mIsAppended = true;
        mIsRemovable = false;
    }
}
}  // namespace al

namespace {
using namespace al;

NERVE_DECL(SeEffectController, Wait)
NERVE_DECL(SeEffectController, PrepareRun)
NERVE_DECL(SeEffectController, PrepareWait)
NERVE_DECL(SeEffectController, WaitFadeOutBusSend)
NERVE_DECL(SeEffectController, WaitEffectUnitDisable)
NERVE_DECL(SeEffectController, Run)
NERVE_DECL(SeEffectController, WaitClearAllEffectFromBus)
NERVE_DECL(SeEffectController, FinishedFinalize)

NERVE_MAKE_CONST(SeEffectController, Wait)
// changeEffect() selects among these three, which are mutable so they are merged into one block.
SeEffectControllerNrvPrepareRun NrvSeEffectControllerPrepareRun;
SeEffectControllerNrvPrepareWait NrvSeEffectControllerPrepareWait;
SeEffectControllerNrvWaitFadeOutBusSend NrvSeEffectControllerWaitFadeOutBusSend;
NERVES_MAKE_NOSTRUCT(SeEffectController, WaitEffectUnitDisable, Run, WaitClearAllEffectFromBus,
                     FinishedFinalize)

/**
 * @brief Counts the entries of an optional info list.
 * @param pList List, or nullptr.
 * @return Entry count, or zero for a missing list.
 */
template <typename T>
inline s32 getInfoNum(const AudioInfoList<T>* pList) {
    return pList != nullptr ? pList->getInfoNum() : 0;
}

/**
 * @brief Gets an entry of an optional info list.
 * @param pList List, or nullptr.
 * @param index Entry index.
 * @return Entry, or nullptr when the list or entry is missing.
 */
template <typename T>
inline T* tryGetInfo(const AudioInfoList<T>* pList, s32 index) {
    return pList != nullptr ? pList->getInfo(index) : nullptr;
}

/**
 * @brief Tests whether an effect setting uses an effect kind on a bus.
 * @param pEachBuses Per-bus effect lists of the effect setting.
 * @param pBusName Aux bus name.
 * @param pType Effect kind name.
 * @return True when the bus's effect list contains the kind.
 */
inline bool isUseEffectInBus(const AudioInfoList<AudioEachBusEffectInfo>* pEachBuses,
                             const char* pBusName, const char* pType) {
    for (s32 i = 0; i < pEachBuses->getInfoNum(); i++) {
        AudioEachBusEffectInfo* pEachBus = pEachBuses->getInfo(i);
        if (!isEqualString(pEachBus->mName, pBusName)) {
            continue;
        }

        AudioInfoList<SeEffectProcInfo>* pProcs = pEachBus->mEffectProcInfoList;
        if (pProcs == nullptr) {
            continue;
        }

        for (s32 j = 0; j < pProcs->getInfoNum(); j++) {
            if (isEqualString(pProcs->getInfoDirect(j)->mName, pType)) {
                return true;
            }
        }
    }

    return false;
}

/** @brief Clears the effects of all three aux buses of the main output. */
inline void clearAllBusEffect() {
    nn::atk::SoundSystem::ClearEffect(nn::atk::AuxBus_A, nn::atk::OutputDevice_Main, 0);
    nn::atk::SoundSystem::ClearEffect(nn::atk::AuxBus_B, nn::atk::OutputDevice_Main, 0);
    nn::atk::SoundSystem::ClearEffect(nn::atk::AuxBus_C, nn::atk::OutputDevice_Main, 0);
}

/** @brief Tests whether every aux bus finished clearing. @return True when all buses are clear. */
inline bool isClearAllBusEffectFinished() {
    return nn::atk::SoundSystem::IsClearEffectFinished(nn::atk::AuxBus_A, nn::atk::OutputDevice_Main) &&
           nn::atk::SoundSystem::IsClearEffectFinished(nn::atk::AuxBus_B, nn::atk::OutputDevice_Main) &&
           nn::atk::SoundSystem::IsClearEffectFinished(nn::atk::AuxBus_C, nn::atk::OutputDevice_Main);
}

/**
 * @brief Creates the unit of one effect kind for a bus when it does not exist yet.
 * @param rTable Table of that effect kind, allocated on first use.
 * @param pBusName Aux bus name.
 */
template <typename Unit>
inline void tryCreateEffectUnit(SoundFxUnitTable<Unit>*& rTable, const char* pBusName) {
    if (rTable == nullptr) {
        rTable = new SoundFxUnitTable<Unit>[1];
        rTable->mUnits = nullptr;
    }

    const s64 index = alAudioEffectFunction::getBusIndex(pBusName);  // Widened once for all uses.
    if (index < 0) {
        return;
    }

    if (rTable->mUnits == nullptr) {
        rTable->mUnits = new Unit*[SoundFxUnitTable<Unit>::cBusNum];
        for (s32 i = 0; i < SoundFxUnitTable<Unit>::cBusNum; i++) {
            rTable->mUnits[i] = nullptr;
        }
    }

    if (rTable->mUnits[index] == nullptr) {
        const char* pDeviceName = alAudioEffectFunction::getOutDeviceNameFromIndex(0);
        rTable->mUnits[index] = new Unit(pBusName, pDeviceName);
    }
}

/**
 * @brief Appends a bus's unit of one effect kind when it exists.
 * @param pTable Table of that effect kind, or nullptr.
 * @param pBusName Aux bus name.
 * @param pInfo Effect-process record.
 */
template <typename Unit>
inline void tryAppendEffectUnit(SoundFxUnitTable<Unit>* pTable, const char* pBusName,
                                const SeEffectProcInfo* pInfo) {
    if (pTable == nullptr) {
        return;
    }

    const s32 index = alAudioEffectFunction::getBusIndex(pBusName);
    Unit* pUnit = pTable->mUnits[index];
    if (pUnit != nullptr) {
        pUnit->appendEffect(pInfo);
    }
}
}  // namespace

namespace al {
/**
 * @brief Looks up an effect setting by name.
 * @param pName Effect setting name, or nullptr.
 * @return Effect setting, or nullptr when the name, the list or the setting is missing.
 */
inline const SeEffectInfo* SeEffectController::tryFindEffectInfo(const char* pName) const {
    const SeEffectInfo* pInfo = nullptr;
    if (pName != nullptr && mDataBase->mEffectInfoList != nullptr) {
        pInfo = mDataBase->mEffectInfoList->tryFindInfo(pName);
    }

    return pInfo;
}

/** @brief Creates an idle controller with a silent fade value. */
SeEffectController::SeEffectController() : NerveExecutor("SE エフェクトコントローラー") {
    mFadeController = new DspLinearValueController(0.0f);
}

/**
 * @brief Binds the audio players and effect database and silences every bus send.
 * @param pSePlayer Sound-effect player.
 * @param pBgmPlayer Music player.
 * @param pDataBase Effect database providing the "Default" bus setting.
 */
void SeEffectController::init(SeadAudioPlayer* pSePlayer, SeadAudioPlayer* pBgmPlayer,
                              const AudioEffectDataBase* pDataBase) {
    mSePlayer = pSePlayer;
    mBgmPlayer = pBgmPlayer;
    mDataBase = pDataBase;
    mBusInfoList = pDataBase->mEffectBusSettingInfoList->tryFindInfo("Default")->mEffectBusInfoList;
    stopAllBusFxSend();
    mCurEffectInfo = nullptr;
    initNerve(&NrvSeEffectControllerWait, 0);
}

/** @brief Sets every bus user's aux send to zero. */
void SeEffectController::stopAllBusFxSend() {
    for (s32 i = 0; i < getInfoNum(mBusInfoList); i++) {
        SeEffectBusInfo* pBus = mBusInfoList->getInfoDirect(i);
        AudioInfoList<SeEffectBusUserInfo>* pUsers = pBus->mEffectBusUserInfoList;
        if (pUsers == nullptr) {
            continue;
        }

        const char* pBusName = pBus->mName;
        for (s32 j = 0; j < pUsers->getInfoNum(); j++) {
            SeEffectBusUserInfo* pUser = pUsers->getInfoDirect(j);
            const char* pUserName = pUser->mName;
            getSeadAudioPlayer(pUser->mCategoryName)
                ->GetSoundPlayer(pUserName)
                .SetOutputEffectSend(
                    static_cast<nn::atk::AuxBus>(alAudioEffectFunction::getBusId(pBusName)), 0.0f);
        }
    }
}

/**
 * @brief Allocates the effect units an effect setting needs.
 * @param pName Effect setting name, or nullptr for none.
 */
void SeEffectController::createEffectUnit(const char* pName) {
    const SeEffectInfo* pInfo = tryFindEffectInfo(pName);
    if (pInfo == nullptr) {
        return;
    }

    AudioInfoList<AudioEachBusEffectInfo>* pBuses = pInfo->mEachBusEffectInfoList;
    for (s32 i = 0; i < getInfoNum(pBuses); i++) {
        AudioEachBusEffectInfo* pBus = pBuses->getInfoDirect(i);
        const char* pBusName = pBus->mName;
        AudioInfoList<SeEffectProcInfo>* pProcs = pBus->mEffectProcInfoList;
        for (s32 j = 0; j < getInfoNum(pProcs); j++) {
            const char* pType = pProcs->getInfoDirect(j)->mName;
            if (isEqualString(pType, "Delay")) {
                tryCreateEffectUnit(mDelayUnits, pBusName);
            } else if (isEqualString(pType, "ReverbStd")) {
                tryCreateEffectUnit(mReverbStdUnits, pBusName);
            } else if (isEqualString(pType, "ReverbHi")) {
                tryCreateEffectUnit(mReverbHiUnits, pBusName);
            } else if (isEqualString(pType, "ReverbI3Dl2")) {
                tryCreateEffectUnit(mReverbI3Dl2Units, pBusName);
            }
        }
    }
}

/** @brief Synchronously disables, removes and clears every effect, including low-pass filters. */
void SeEffectController::fullDisableClearAllEffects() {
    disableAllEffects();
    while (!isEffectSystemDisabled()) {
    }

    clearAllBusEffect();
    clearLpf();
    while (!isClearAllBusEffectFinished()) {
    }
}

/** @brief Disables every effect unit. */
void SeEffectController::disableAllEffects() {
    if (mDelayUnits != nullptr) {
        mDelayUnits->disable();
    }

    if (mReverbStdUnits != nullptr) {
        mReverbStdUnits->disable();
    }

    if (mReverbHiUnits != nullptr) {
        mReverbHiUnits->disable();
    }

    if (mReverbI3Dl2Units != nullptr) {
        mReverbI3Dl2Units->disable();
    }
}

/** @brief Tests whether every appended unit became removable. @return True when all are removable. */
bool SeEffectController::isEffectSystemDisabled() {
    if (mDelayUnits != nullptr && !mDelayUnits->isDisabled()) {
        return false;
    }

    if (mReverbStdUnits != nullptr && !mReverbStdUnits->isDisabled()) {
        return false;
    }

    if (mReverbHiUnits != nullptr && !mReverbHiUnits->isDisabled()) {
        return false;
    }

    if (mReverbI3Dl2Units != nullptr && !mReverbI3Dl2Units->isDisabled()) {
        return false;
    }

    return true;
}

/** @brief Resets the low-pass filter of every bus user whose bus uses an "Lpf" effect. */
void SeEffectController::clearLpf() {
    if (mCurEffectInfo == nullptr) {
        return;
    }

    for (s32 i = 0; i < getInfoNum(mBusInfoList); i++) {
        SeEffectBusInfo* pBus = tryGetInfo(mBusInfoList, i);
        AudioInfoList<AudioEachBusEffectInfo>* pEachBuses = mCurEffectInfo->mEachBusEffectInfoList;
        if (pEachBuses == nullptr) {
            continue;
        }

        if (!isUseEffectInBus(pEachBuses, pBus->mName, "Lpf")) {
            continue;
        }

        AudioInfoList<SeEffectBusUserInfo>* pUsers = pBus->mEffectBusUserInfoList;
        if (pUsers == nullptr) {
            continue;
        }

        for (s32 j = 0; j < pUsers->getInfoNum(); j++) {
            SeEffectBusUserInfo* pUser = pUsers->getInfo(j);
            getSeadAudioPlayer(pUser->mCategoryName)->GetSoundPlayer(pUser->mName).SetLowPassFilterFrequency(0.0f);
        }
    }
}

/** @brief Removes every effect and frees the effect work buffers. */
void SeEffectController::finalize() {
    fullDisableClearAllEffects();
    releaseAllWorkBuffer();
}

/** @brief Releases the work buffers of every appended effect unit. */
void SeEffectController::releaseAllWorkBuffer() {
    if (mDelayUnits != nullptr) {
        mDelayUnits->releaseWorkBuffer();
    }

    if (mReverbStdUnits != nullptr) {
        mReverbStdUnits->releaseWorkBuffer();
    }

    if (mReverbHiUnits != nullptr) {
        mReverbHiUnits->releaseWorkBuffer();
    }

    if (mReverbI3Dl2Units != nullptr) {
        mReverbI3Dl2Units->releaseWorkBuffer();
    }
}

/** @brief Advances the effect-switching state machine. */
void SeEffectController::update() {
    updateNerve();
}

/**
 * @brief Fades out the current effect and switches to another one.
 * @param pName Next effect setting name, or nullptr to only remove the current effect.
 */
void SeEffectController::changeEffect(const char* pName) {
    mNextNerve = pName != nullptr ? static_cast<const Nerve*>(&NrvSeEffectControllerPrepareRun) :
                                    &NrvSeEffectControllerPrepareWait;
    setNerve(this, &NrvSeEffectControllerWaitFadeOutBusSend);
    clearLpf();
    mCurEffectInfo = tryFindEffectInfo(pName);
}

/**
 * @brief Appends one effect-process record to the matching unit of a bus.
 * @param pBusName Aux bus name.
 * @param pInfo Effect-process record; "Lpf" records set the bus users' low-pass filter instead.
 */
void SeEffectController::appendEffectUnit(const char* pBusName, const SeEffectProcInfo* pInfo) {
    const char* pType = pInfo->mName;
    if (isEqualString(pType, "Delay")) {
        tryAppendEffectUnit(mDelayUnits, pBusName, pInfo);
    } else if (isEqualString(pType, "ReverbStd")) {
        tryAppendEffectUnit(mReverbStdUnits, pBusName, pInfo);
    } else if (isEqualString(pType, "ReverbHi")) {
        tryAppendEffectUnit(mReverbHiUnits, pBusName, pInfo);
    } else if (isEqualString(pType, "ReverbI3Dl2")) {
        tryAppendEffectUnit(mReverbI3Dl2Units, pBusName, pInfo);
    } else if (isEqualString(pType, "Lpf")) {
        const f32 freq = static_cast<const SeLpfEffectProcInfo*>(pInfo)->mLpfFreq;
        for (s32 i = 0; i < getInfoNum(mBusInfoList); i++) {
            SeEffectBusInfo* pBus = tryGetInfo(mBusInfoList, i);
            if (!isEqualString(pBusName, pBus->mName)) {
                continue;
            }

            AudioInfoList<SeEffectBusUserInfo>* pUsers = pBus->mEffectBusUserInfoList;
            if (pUsers == nullptr) {
                continue;
            }

            for (s32 j = 0; j < pUsers->getInfoNum(); j++) {
                SeEffectBusUserInfo* pUser = pUsers->getInfo(j);
                getSeadAudioPlayer(pUser->mCategoryName)
                    ->GetSoundPlayer(pUser->mName)
                    .SetLowPassFilterFrequency(freq);
            }
        }
    }
}

/**
 * @brief Selects the audio player of a bus-user category.
 * @param pCategoryName "SE" or "BGM".
 * @return Matching player, or nullptr for an unknown category.
 */
SeadAudioPlayer* SeEffectController::getSeadAudioPlayer(const char* pCategoryName) {
    if (isEqualString("SE", pCategoryName)) {
        return mSePlayer;
    }

    if (isEqualString("BGM", pCategoryName)) {
        return mBgmPlayer;
    }

    return nullptr;
}

/**
 * @brief Appends every effect-process record of an effect setting.
 * @param pInfo Effect setting, or nullptr for none.
 */
void SeEffectController::appendEffects(const SeEffectInfo* pInfo) {
    if (pInfo == nullptr) {
        return;
    }

    AudioInfoList<AudioEachBusEffectInfo>* pBuses = pInfo->mEachBusEffectInfoList;
    if (pBuses == nullptr) {
        return;
    }

    for (s32 i = 0; i < pBuses->getInfoNum(); i++) {
        AudioEachBusEffectInfo* pBus = pBuses->getInfo(i);
        if (pBus == nullptr) {
            continue;
        }

        AudioInfoList<SeEffectProcInfo>* pProcs = pBus->mEffectProcInfoList;
        if (pProcs == nullptr) {
            continue;
        }

        const char* pBusName = pBus->mName;
        for (s32 j = 0; j < pProcs->getInfoNum(); j++) {
            appendEffectUnit(pBusName, pProcs->getInfo(j));
        }
    }
}

/** @brief Restores every bus user's configured aux send. */
void SeEffectController::setFxSend() {
    for (s32 i = 0; i < getInfoNum(mBusInfoList); i++) {
        SeEffectBusInfo* pBus = mBusInfoList->getInfoDirect(i);
        AudioInfoList<SeEffectBusUserInfo>* pUsers = pBus->mEffectBusUserInfoList;
        if (pUsers == nullptr) {
            continue;
        }

        const char* pBusName = pBus->mName;
        for (s32 j = 0; j < pUsers->getInfoNum(); j++) {
            SeEffectBusUserInfo* pUser = pUsers->getInfo(j);
            const char* pUserName = pUser->mName;
            SeadAudioPlayer* pPlayer = getSeadAudioPlayer(pUser->mCategoryName);
            const f32 send = pUser->mMainOutputSend;
            pPlayer->GetSoundPlayer(pUserName).SetOutputEffectSend(
                static_cast<nn::atk::AuxBus>(alAudioEffectFunction::getBusId(pBusName)), send);
        }
    }
}

/** @brief Idles while no effect change is pending. */
void SeEffectController::exeWait() {
    isFirstStep(this);
}

/** @brief Fades every bus user's aux sends to zero, then disables the effects. */
void SeEffectController::exeWaitFadeOutBusSend() {
    if (isFirstStep(this)) {
        mFadeController->changeTarget(0.0f, 10);
    }

    mFadeController->update();
    const f32 rate = mFadeController->getValue();
    for (s32 i = 0; i < getInfoNum(mBusInfoList); i++) {
        SeEffectBusInfo* pBus = mBusInfoList->getInfoDirect(i);
        AudioInfoList<SeEffectBusUserInfo>* pUsers = pBus->mEffectBusUserInfoList;
        if (pUsers == nullptr) {
            continue;
        }

        for (s32 j = 0; j < pUsers->getInfoNum(); j++) {
            SeEffectBusUserInfo* pUser = pUsers->getInfo(j);
            nn::atk::SoundPlayer& rPlayer =
                getSeadAudioPlayer(pUser->mCategoryName)->GetSoundPlayer(pUser->mName);
            for (s32 bus = 0; bus < nn::atk::AuxBus_Count; bus++) {
                const auto auxBus = static_cast<nn::atk::AuxBus>(bus);
                rPlayer.SetOutputEffectSend(auxBus, rate * rPlayer.GetOutputEffectSend(auxBus));
            }
        }
    }

    if (mFadeController->getTarget() == mFadeController->getValue()) {
        disableAllEffects();
        setNerve(this, &NrvSeEffectControllerWaitEffectUnitDisable);
    }
}

/** @brief Waits until every effect is removable, clears the buses and moves to the pending step. */
void SeEffectController::exeWaitEffectUnitDisable() {
    if (isEffectSystemDisabled()) {
        clearAllBusEffect();
        setNerve(this, mNextNerve);
    }
}

/** @brief Waits for the buses to clear, then appends the next effect setting. */
void SeEffectController::exePrepareRun() {
    if (!isClearAllBusEffectFinished()) {
        return;
    }

    releaseAllWorkBuffer();
    if (mCurEffectInfo != nullptr) {
        appendEffects(mCurEffectInfo);
        setFxSend();
        setNerve(this, &NrvSeEffectControllerRun);
    }
}

/** @brief Waits for the buses to clear, then returns to the idle state without effects. */
void SeEffectController::exePrepareWait() {
    if (!isClearAllBusEffectFinished()) {
        return;
    }

    releaseAllWorkBuffer();
    stopAllBusFxSend();
    mCurEffectInfo = nullptr;
    setNerve(this, &NrvSeEffectControllerWait);
}

/** @brief Keeps the current effect running. */
void SeEffectController::exeRun() {
    isFirstStep(this);
}

/** @brief Disables every effect and waits until they can be removed. */
void SeEffectController::exeWaitDisableAllAudioEffectUnit() {
    if (isFirstStep(this)) {
        disableAllEffects();
    }

    if (isEffectSystemDisabled()) {
        setNerve(this, &NrvSeEffectControllerWaitClearAllEffectFromBus);
    }
}

/** @brief Clears every aux bus and waits for the clear to finish. */
void SeEffectController::exeWaitClearAllEffectFromBus() {
    if (isFirstStep(this)) {
        clearAllBusEffect();
    }

    if (isClearAllBusEffectFinished()) {
        setNerve(this, &NrvSeEffectControllerFinishedFinalize);
    }
}

/** @brief Final state after all effects were removed. */
void SeEffectController::exeFinishedFinalize() {}
}  // namespace al
