#pragma once

#include <container/seadStrTreeMap.h>
#include <heap/seadExpHeap.h>
#include <heap/seadFrameHeap.h>
#include <heap/seadHeapMgr.h>

namespace al {
class AudioResourceDirector;
class MemorySceneHeapCustomAlloc;

class StageSizeAdjuster {
public:
    virtual s32 adjustStageSize(const char* pStageName, s32 size,
                                MemorySceneHeapCustomAlloc* pAlloc) = 0;

    /**
     * @brief Former name of adjustStageSize, kept for existing callers.
     * @param pStageName Name of the stage whose resource heap is created.
     * @param size Default resource heap size.
     * @param pAlloc Custom scene heap allocator, or nullptr.
     * @return The adjusted resource heap size.
     */
    s32 adjustSceneResourceSize(const char* pStageName, s32 size,
                                MemorySceneHeapCustomAlloc* pAlloc) {
        return adjustStageSize(pStageName, size, pAlloc);
    }
};

class MemorySceneHeapCustomAlloc {
public:
    virtual void createSceneHeap(bool isCreatedResourceHeap) = 0;
    virtual bool isFreeSceneResource(bool isRemoveCategory) = 0;
    virtual bool isReallyFreeSceneResource() const = 0;
    virtual s64 adjustStageResourceSize(s64 size) = 0;
    virtual void setForceSceneHeapResourceDestroy() = 0;
};

class MemorySystem {
public:
    MemorySystem(sead::Heap* pHeap, u64 a, u64 b, u64 c);

    void allocFailedCallbackFunc(const sead::HeapMgr::AllocFailedCallbackArg* pArg);
    void createSequenceHeap();
    void createCourseSelectStationedHeap();
    void destroyCourseSelectStationedHeap();
    void freeAllSequenceHeap();
    bool createSceneHeap(const char* pStageName);
    void createSceneResourceHeap(const char* pStageName);
    void destroySceneHeap(bool removeCategory);
    bool isReallyFreeSceneResource(bool removeCategory) const;
    void setForceSceneHeapResourceDestroy();
    void createCourseSelectHeap();
    void destroyCourseSelectHeap();
    void freeAllPlayerHeap();
    sead::Heap* tryFindNamedHeap(const char* pHeapName) const;
    sead::Heap* findNamedHeap(const char* pHeapName) const;
    void addNamedHeap(sead::Heap* pHeap, const char* pHeapName);
    void removeNamedHeap(const char* pHeapName);

    sead::Heap* getStationedHeap() { return mStationedHeap; }
    sead::Heap* getSequenceHeap() { return mSequenceHeap; }
    sead::Heap* getSceneResourceHeap() { return mSceneResourceHeap; }
    sead::Heap* getSceneHeap() { return mSceneHeap; }
    sead::Heap* getCourseSelectResourceHeap() { return mCourseSelectResourceHeap; }
    sead::Heap* getCourseSelectHeap() { return mCourseSelectHeap; }
    AudioResourceDirector* getAudioResourceDirector() { return mAudioResourceDirector; }
    void setAudioResourceDirector(AudioResourceDirector* pDirector) {
        mAudioResourceDirector = pDirector;
    }

    void setStageSizeAdjuster(StageSizeAdjuster* pAdjuster) { mStageSizeAdjuster = pAdjuster; }
    void setCustomSceneHeapAlloc(MemorySceneHeapCustomAlloc* pAlloc) { mCustomAlloc = pAlloc; }

    sead::ExpHeap* mStationedHeap = nullptr;
    sead::ExpHeap* mSequenceHeap = nullptr;
    sead::FrameHeap* mSceneResourceHeap = nullptr;
    sead::FrameHeap* mSceneHeap = nullptr;
    sead::Heap* mPlayerHeap = nullptr;
    sead::ExpHeap* mCourseSelectStationedHeap = nullptr;
    sead::FrameHeap* mCourseSelectResourceHeap = nullptr;
    sead::FrameHeap* mCourseSelectHeap = nullptr;
    sead::StrTreeMap<64, sead::Heap*> mHeapList;
    AudioResourceDirector* mAudioResourceDirector = nullptr;
    bool mIsExistFileResource = false;
    StageSizeAdjuster* mStageSizeAdjuster = nullptr;
    MemorySceneHeapCustomAlloc* mCustomAlloc = nullptr;
    sead::HeapMgr::AllocFailedCallback<MemorySystem> mAllocFailedCallback;
};

static_assert(sizeof(MemorySystem) == 0xa0);
}  // namespace al
