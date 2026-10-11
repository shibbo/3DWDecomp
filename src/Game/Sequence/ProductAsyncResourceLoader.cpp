#include "Sequence/ProductAsyncResourceLoader.hpp"

#include <thread/seadThread.h>

#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/Resource/ResourceFunction.hpp"
#include "Library/Thread/AsyncFunctorThread.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Audio/System/AudioSystem.hpp"

namespace {
const int cLoadThreadPriority = sead::Thread::cDefaultPriority + 5;

/**
 * @brief Load a pair of resident sound-effect and music banks.
 * @param pInfo Audio players receiving the resources.
 * @param pSeFile Sound-effect management file.
 * @param pBgmFile Music management file.
 */
inline void loadAudioBanks(const al::AudioSystemInfo* pInfo, const char* pSeFile,
                           const char* pBgmFile) {
    alAudioSystemFunction::loadResourceFromUserManagementFile(
        pSeFile, pInfo->getSeadAudioPlayerForSe(), true);
    alAudioSystemFunction::loadResourceFromUserManagementFile(
        pBgmFile, pInfo->getSeadAudioPlayerForBgm(), true);
}
}

/**
 * @brief Create the worker used to load resident sequence resources.
 * @param pAudioSystemInfo Audio players used by title resource loading.
 * @param pGameDataHolder Game-data holder retained by the loader.
 */
ProductAsyncResourceLoader::ProductAsyncResourceLoader(
    const al::AudioSystemInfo* pAudioSystemInfo, GameDataHolder* pGameDataHolder)
    : mpAudioSystemInfo(pAudioSystemInfo), mpGameDataHolder(pGameDataHolder) {
    mpThread = new al::AsyncFunctorThread("Async Resource Loader",
                                         al::Functor(this, &ProductAsyncResourceLoader::threadFunc),
                                         cLoadThreadPriority, 0x20000);
}

/** @brief Load the requested resource category and record completion. */
void ProductAsyncResourceLoader::threadFunc() {
    if (al::isEqualString(mpCategoryName, "Stationed[CourseSelect]")) {
        if (al::isEmptyCategoryResource("Stationed[2nd]")) {
            al::createCategoryResourceAll("Stationed[2nd]", nullptr);
        }
        if (!al::isEmptyCategoryResource(mpCategoryName)) {
            mIsLoading = false;
            mIsCourseSelectLoaded = true;
            return;
        }
        al::createCategoryResourceAll(mpCategoryName, nullptr);
        mIsCourseSelectLoaded = true;
        mIsLoading = false;
    } else {
        if (al::isEmptyCategoryResource(mpCategoryName)) {
            al::clearFileLoaderEntry();
            loadAudioBanks(mpAudioSystemInfo, al::UMF_SE_STATIONED_1ST, al::UMF_BGM_STATIONED_1ST);
            al::clearFileLoaderEntry();
            loadAudioBanks(mpAudioSystemInfo, al::UMF_SE_STATIONED_2ND, al::UMF_BGM_STATIONED_2ND);
            al::createCategoryResourceAll(mpCategoryName, nullptr);
        }
        mIsLoading = false;
        mIsTitleLoaded = true;
    }
}

/** @brief Destroy the resource-loading worker if it exists. */
ProductAsyncResourceLoader::~ProductAsyncResourceLoader() {
    if (mpThread != nullptr) {
        delete mpThread;
        mpThread = nullptr;
    }
}

/** @brief Start title loading unless loading is active or the title is already loaded. */
void ProductAsyncResourceLoader::tryStartTitleLoad() {
    if (mIsLoading || mIsTitleLoaded) {
        return;
    }
    mIsLoading = true;
    mpCategoryName = "Stationed[1st]";
    mpLoadName = "Stationed(Title)";
    mpThread->start();
}

/** @brief Start course-select loading unless it is active or already complete. */
void ProductAsyncResourceLoader::tryStartCourseSelectLoad() {
    if (mIsLoading || mIsCourseSelectLoaded) {
        return;
    }
    al::clearFileLoaderEntry();
    mIsLoading = true;
    mpCategoryName = "Stationed[CourseSelect]";
    mpLoadName = "Stationed(CourseSelect)";
    mpThread->start();
}

/** @brief Allow the course-select resource category to be loaded again. */
void ProductAsyncResourceLoader::resetCourseSelectLoad() {
    mIsCourseSelectLoaded = false;
}
