#pragma once

namespace al {
class AsyncFunctorThread;
class AudioSystemInfo;
}
class GameDataHolder;

class ProductAsyncResourceLoader {
public:
    ProductAsyncResourceLoader(const al::AudioSystemInfo* pAudioSystemInfo,
                               GameDataHolder* pGameDataHolder);
    ~ProductAsyncResourceLoader();
    void threadFunc();
    void tryStartTitleLoad();
    void tryStartCourseSelectLoad();
    void resetCourseSelectLoad();
    /** @brief Check title resource availability. @return Whether title loading is complete. */
    bool isTitleLoaded() const { return mIsTitleLoaded; }

private:
    al::AsyncFunctorThread* mpThread = nullptr;
    const char* mpCategoryName = nullptr;
    const char* mpLoadName = nullptr;
    bool mIsLoading = false;
    bool mIsTitleLoaded = false;
    bool mIsCourseSelectLoaded = false;
    const al::AudioSystemInfo* mpAudioSystemInfo;
    GameDataHolder* mpGameDataHolder;
    void* mUnreconstructed30 = nullptr;
};

static_assert(sizeof(ProductAsyncResourceLoader) == 0x38);
