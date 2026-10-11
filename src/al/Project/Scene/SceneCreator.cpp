#include "Project/Scene/SceneCreator.hpp"

#include <heap/seadHeapMgr.h>

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Scene/SceneFunction.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/Thread/AsyncFunctorThread.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Scene/SceneInitInfo.hpp"

namespace al {
void setCpuBoost(bool isBoost, bool isUnk);

using SceneInitFunctor =
    FunctorV1M<Scene*, void (Scene::*)(const SceneInitInfo&), const SceneInitInfo&>;

/**
 * Creates the scene init info and initializes the scene on an initialize thread.
 * @param pScene scene
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 * @param priority thread priority
 * @param pHeap heap to initialize the scene in, or null for the scene heap
 */
void SceneCreator::setSceneAndThreadInit(Scene* pScene, const char* pStageName, s32 scenarioNo,
                                         const char* pSceneName, s32 priority,
                                         sead::Heap* pHeap) {
    setCpuBoost(true, false);

    if (pHeap != nullptr) {
        sead::ScopedCurrentHeapSetter setter(pHeap);
        SceneInitInfo* info =
            new SceneInitInfo(mGameSystemInfo, mAudioDirector, mGameDataHolder,
                              mScreenCaptureExecutor, pStageName, scenarioNo, pSceneName);
        mInitThread = createAndStartInitializeThread(
            pHeap, priority, SceneInitFunctor(pScene, &Scene::init, *info));
    } else {
        SceneHeapSetter setter;
        SceneInitInfo* info =
            new SceneInitInfo(mGameSystemInfo, mAudioDirector, mGameDataHolder,
                              mScreenCaptureExecutor, pStageName, scenarioNo, pSceneName);
        mInitThread = createAndStartInitializeThread(setter.mSceneHeap, priority,
                                                     SceneInitFunctor(pScene, &Scene::init, *info));
    }
}

/**
 * Creates the scene init info and initializes the scene.
 * @param pScene scene
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 */
void SceneCreator::setSceneAndInit(Scene* pScene, const char* pStageName, s32 scenarioNo,
                                   const char* pSceneName) {
    SceneHeapSetter setter;
    setCpuBoost(true, false);
    SceneInitInfo* info = new SceneInitInfo(mGameSystemInfo, mAudioDirector, mGameDataHolder,
                                            mScreenCaptureExecutor, pStageName, scenarioNo,
                                            pSceneName);
    pScene->init(*info);
    setCpuBoost(false, false);
}

/**
 * Waits for the initialize thread and destroys it once it is done.
 * @return whether no initialize thread is running
 */
bool SceneCreator::tryEndInitThread() {
    if (mInitThread != nullptr) {
        if (!tryWaitDoneAndDestroyInitializeThread(mInitThread)) {
            return false;
        }

        setCpuBoost(false, false);
        mInitThread = nullptr;
    }

    return true;
}

/**
 * Checks if an initialize thread exists.
 * @return whether an initialize thread exists
 */
bool SceneCreator::isExistInitThread() const {
    return mInitThread != nullptr;
}
}  // namespace al
