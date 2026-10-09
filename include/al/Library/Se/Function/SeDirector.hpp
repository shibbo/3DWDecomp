#pragma once

#include <basis/seadTypes.h>
#include <math/seadMatrix.h>
#include <math/seadVector.h>

namespace sead {
class Audio3DListenerParameterNin;
class PerspectiveProjection;
} // namespace sead

namespace alSeFunction {
enum DemoType : s32;
}

namespace al {
class AudioSystemInfo;
class BgmRhythmCtrl;
class MeInfo;
class SeadAudio3DMgr;
class SeCategoryParamsController;
class SePlayParamList;
class SeRequestKeeper;
class SeResourceSpecificInfo;
class SeSource;
class SeListenerKeeper;
class MeInfoKeeper;
class SeMaterialInfoKeeper;

class SeDirector {
  public:
    SeDirector();

    void init(AudioSystemInfo* pInfo, BgmRhythmCtrl* pRhythmCtrl, s32 mainRequestNum, s32 subRequestNum,
              s32 demoRequestNum, s32 playerRequestNum, f32 volume);
    void init3D(SeadAudio3DMgr* pMgr, const sead::Vector3f* pCameraPos, const sead::Matrix34f* pCameraMtx,
                sead::PerspectiveProjection* pProjection, const sead::Vector3f* pCameraAt,
                const char* pStageName, bool isUseListenerPoser);
    void initCategoryParamsController(SeCategoryParamsController* pController);
    void finalize();
    void stopAll(u32 fadeFrames, const char* pExceptName, const char* pKeeperName);
    void update();
    void stopAllExcept(u32 fadeFrames, const char** pExceptList, u32 exceptNum);
    void stopAllMain(u32 fadeFrames);
    void stopAllDemo(u32 fadeFrames);
    void pauseSystem(bool isPause, const char* pName, u32 fadeFrames);
    void pauseSystemExceptSub(bool isPause, const char* pName, u32 fadeFrames);
    void activateRequestKeeper(const char* pName);
    void deactivateRequestKeeper(const char* pName);
    void startDemo(alSeFunction::DemoType type);
    void setAllKeeperVolumeSettingExceptMain(const char* pName, s32 fadeFrames);
    void endDemo(alSeFunction::DemoType type);
    void setAllKeeperVolumeSetting(const char* pName, s32 fadeFrames);
    void changeDemo(alSeFunction::DemoType from, alSeFunction::DemoType to);
    void setIsStateAfterGoal(bool isAfterGoal);
    void setVolumeSetting(const char* pKeeperName, const char* pSettingName, s32 fadeFrames, bool isForce);
    void setIsExcludeCmNgSe();
    void changeListenerParam(sead::Audio3DListenerParameterNin& rParam);
    void resetListenerParam();
    void changeListenerPoser(const char* pName);
    void changeListenerPoserToLast();
    SePlayParamList* addRequest(u32 id, SeSource* pSource, bool isLoop,
                                const SeResourceSpecificInfo* pSpecificInfo, MeInfo* pMeInfo,
                                const char* pMaterialName, s32 waterState, bool isBeyondWall,
                                const char* pPlayName);
    SePlayParamList* addHoldRequest(u32 id, SeSource* pSource, const SeResourceSpecificInfo* pSpecificInfo,
                                    MeInfo* pMeInfo, const char* pMaterialName, s32 waterState,
                                    bool isBeyondWall, const char* pPlayName);
    void stop(u32 id, SeSource* pSource, u32 fadeFrames, const char* pPlayName);
    void stopAllId(u32 id, SeSource* pSource, u32 fadeFrames, const char* pPlayName);
    void stopAllFromSource(SeSource* pSource, u32 fadeFrames);
    void deactivateSeFromSource(SeSource* pSource, u32 fadeFrames, bool isClipped);
    void reactivateSeFromSource(SeSource* pSource);
    void notifiedUpdateMaterial(SeSource* pSource, const char* pMaterialName, s32 waterState);

    /** @brief Gets the listener controller. @return Listener keeper, or nullptr before 3D initialization. */
    SeListenerKeeper* getListenerKeeper() const { return mListenerKeeper; }
    /** @brief Tests whether demo sound routing is active. @return True while a demo is active. */
    bool isInDemo() const { return mIsInDemo; }

  private:
    SeRequestKeeper* selectRequestKeeper(const char* pName) const;
    SeRequestKeeper* findRequestKeeper(const char* pName) const;
    /**
     * @brief Applies an operation to every initialized request keeper in playback order.
     * @tparam Callback Callable accepting a SeRequestKeeper pointer.
     * @param rCallback Operation to apply; the main, demo, and player keepers must be initialized.
     */
    template <class Callback> void forEachRequestKeeper(Callback&& rCallback) {
        rCallback(mMainKeeper);
        rCallback(mDemoKeeper);
        rCallback(mPlayerKeeper);
        if (mSubKeeper != nullptr) {
            rCallback(mSubKeeper);
        }
    }
    SeRequestKeeper* mMainKeeper = nullptr;
    SeRequestKeeper* mDemoKeeper = nullptr;
    SeRequestKeeper* mPlayerKeeper = nullptr;
    SeRequestKeeper* mSubKeeper = nullptr;
    SeListenerKeeper* mListenerKeeper = nullptr;
    MeInfoKeeper* mMeInfoKeeper = nullptr;
    SeMaterialInfoKeeper* mMaterialInfoKeeper = nullptr;
    bool mIsInDemo = false;
    bool mIsDistancePauseEnabled = false;
    SeCategoryParamsController* mCategoryParamsController = nullptr;
};
static_assert(sizeof(SeDirector) == 0x48);
} // namespace al
