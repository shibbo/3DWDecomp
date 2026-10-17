#include "Library/Sequence/Sequence.hpp"

#include "Library/Audio/AudioDirector.hpp"
#include "Library/Audio/System/AudioKeeper.hpp"
#include "Library/Audio/System/AudioKeeperFunction.hpp"
#include "Library/Audio/System/AudioSystemInfo.hpp"
#include "Library/Scene/Scene.hpp"
#include "Library/System/GameSystemInfo.hpp"
#include "Project/Audio/System/AudioSystem.hpp"
#include "Project/Scene/SceneCreator.hpp"

#include "Library/Memory/HeapUtil.hpp"
#include "Library/Memory/SceneHeapSetter.hpp"
#include "Library/Scene/SceneFunction.hpp"
#include "Library/Thread/AsyncFunctorThread.hpp"
#include "Library/Thread/Functor.hpp"
#include "Project/Scene/SceneInitInfo.hpp"

namespace al {
void setCpuBoost(bool isBoost, bool isUnk);
using SceneInitFunctor = FunctorV1M<Scene*, void (Scene::*)(const SceneInitInfo&), const SceneInitInfo&>;

/**
 * Constructs a sequence.
 * @param pName sequence name
 */
Sequence::Sequence(const char* pName) : NerveExecutor(pName), mName(pName) {}

/**
 * Finalizes the audio director if one was created.
 */
Sequence::~Sequence() {
    if (mAudioDirector != nullptr) {
        mAudioDirector->finalize();
    }
}

/**
 * Initializes the sequence. Does nothing by default.
 * @param rInfo sequence init info
 */
void Sequence::init(const SequenceInitInfo& rInfo) {}

/**
 * Switches to the next scene if requested and updates the scene, nerve and audio.
 */
void Sequence::update() {
    if (mNextScene != nullptr && mIsChangeScene) {
        mCurrentScene = mNextScene;
        mNextScene = nullptr;
    }

    if (mCurrentScene != nullptr && mCurrentScene->isAlive()) {
        mCurrentScene->movement();
    }

    if (mIsChangeScene) {
        updateNerve();
    }

    if (mAudioDirector != nullptr) {
        mAudioDirector->update();
    }

    if (mAudioKeeper != nullptr) {
        mAudioKeeper->update();
    }
}

/**
 * Kills the sequence.
 */
void Sequence::kill() {
    mIsAlive = false;
}

/**
 * Creates the audio director and audio keeper.
 * @param rInfo game system info
 * @param pStageName stage name
 * @param seRequestNum number of se requests
 * @param unused1 passed to the audio director
 * @param unused2 passed to the audio director
 * @param pKeeperName audio keeper name
 */
void Sequence::initAudio(const GameSystemInfo& rInfo, const char* pStageName, s32 seRequestNum,
                         s32 unused1, s32 unused2, const char* pKeeperName) {
    mAudioDirector = new AudioDirector();
    AudioSystemInfo* audioSystemInfo =
        static_cast<AudioSystem*>(rInfo._0)->getAudioSystemInfo();
    audioSystemInfo->mUpperLayerAudioUser = nullptr;
    mAudioDirector->init(audioSystemInfo, pStageName, seRequestNum, unused1, unused2, "Sequence",
                         20, 1.0f);
    mAudioKeeper = createAudioKeeper(pKeeperName, mAudioDirector);
    audioSystemInfo->mUpperLayerAudioUser = this;
}

/**
 * Initializes the audio keeper. Does nothing.
 * @param pName audio keeper name
 */
void Sequence::initAudioKeeper(const char* pName) {}

/**
 * Stores the draw system info.
 * @param rInfo sequence init info
 */
void Sequence::initDrawSystemInfo(const SequenceInitInfo& rInfo) {
    mDrawSystemInfo = rInfo.mGameSystemInfo->getDrawSystemInfo();
}

/**
 * Gets the audio system info.
 * @return audio system info
 */
AudioSystemInfo* Sequence::getAudioSystemInfo() const {
    return mAudioDirector->getAudioSystemInfo();
}

/**
 * Draws the main screen of the current scene.
 */
void Sequence::drawMain() const {
    if (mCurrentScene != nullptr && mCurrentScene->isAlive()) {
        mCurrentScene->drawMain();
    }
}

/**
 * Draws the main screen of the current scene.
 * @param isForceDraw whether dead scenes are drawn too
 */
void Sequence::doDrawScene(bool isForceDraw) const {
    if (mCurrentScene != nullptr && (mCurrentScene->isAlive() || isForceDraw)) {
        mCurrentScene->drawMain();
    }
}

/**
 * Draws the sub screen of the current scene.
 */
void Sequence::drawSub() const {
    if (mCurrentScene != nullptr && mCurrentScene->isAlive()) {
        mCurrentScene->drawSub();
    }
}

/**
 * Checks if the sequence can be disposed.
 * @return true
 */
bool Sequence::isDisposable() const {
    return true;
}

/**
 * Gets the current scene.
 * @return null
 */
Scene* Sequence::getCurrentScene() const {
    return nullptr;
}

/**
 * Stores the game system info.
 * @param pGameSystemInfo game system info
 */
SequenceInitInfo::SequenceInitInfo(const GameSystemInfo* pGameSystemInfo)
    : mGameSystemInfo(pGameSystemInfo) {}

/**
 * Creates a scene creator and gives it to the user.
 * @param pUser scene creator user
 * @param rInfo sequence init info
 * @param pGameDataHolder game data holder
 * @param pAudioDirector audio director
 * @param pScreenCaptureExecutor screen capture executor
 * @param pSceneFactory scene factory
 */
void initSceneCreator(IUseSceneCreator* pUser, const SequenceInitInfo& rInfo,
                      GameDataHolderBase* pGameDataHolder, AudioDirector* pAudioDirector,
                      ScreenCaptureExecutor* pScreenCaptureExecutor,
                      alSceneFunction::SceneFactory* pSceneFactory) {
    SceneCreator* sceneCreator =
        new SceneCreator(rInfo.mGameSystemInfo, pAudioDirector, pGameDataHolder,
                         pScreenCaptureExecutor, pSceneFactory);
    pUser->setSceneCreator(sceneCreator);
}

/**
 * Creates a scene and initializes it on the current thread.
 * @param pUser scene creator user
 * @param pClassName scene class name
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 * @return created scene
 */
Scene* createSceneAndInit(IUseSceneCreator* pUser, const char* pClassName,
                          const char* pStageName, s32 scenarioNo, const char* pSceneName) {
    return pUser->getSceneCreator()->createScene(pClassName, pStageName, scenarioNo, pSceneName,
                                                 false, -1, nullptr);
}

/**
 * Creates a scene and initializes it on an initialize thread.
 * @param pUser scene creator user
 * @param pClassName scene class name
 * @param priority thread priority
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 * @param pHeapName heap name
 * @return created scene
 */
Scene* createSceneAndUseInitThread(IUseSceneCreator* pUser, const char* pClassName,
                                   s32 priority, const char* pStageName, s32 scenarioNo,
                                   const char* pSceneName, const char* pHeapName) {
    return pUser->getSceneCreator()->createScene(pClassName, pStageName, scenarioNo, pSceneName,
                                                 true, priority, pHeapName);
}

/**
 * Initializes an existing scene on the current thread.
 * @param pUser scene creator user
 * @param pScene scene
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 */
void setSceneAndInit(IUseSceneCreator* pUser, Scene* pScene, const char* pStageName,
                     s32 scenarioNo, const char* pSceneName) {
    pUser->getSceneCreator()->setSceneAndInit(pScene, pStageName, scenarioNo, pSceneName);
}

/**
 * Initializes an existing scene on an initialize thread.
 * @param pUser scene creator user
 * @param pScene scene
 * @param priority thread priority
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 * @param pHeap heap
 */
void setSceneAndUseInitThread(IUseSceneCreator* pUser, Scene* pScene, s32 priority,
                              const char* pStageName, s32 scenarioNo, const char* pSceneName,
                              sead::Heap* pHeap) {
    pUser->getSceneCreator()->setSceneAndThreadInit(pScene, pStageName, scenarioNo, pSceneName,
                                                    priority, pHeap);
}

/**
 * Ends the scene initialize thread if it has finished.
 * @param pUser scene creator user
 * @return whether the thread ended
 */
bool tryEndSceneInitThread(IUseSceneCreator* pUser) {
    return pUser->getSceneCreator()->tryEndInitThread();
}

/**
 * Checks if a scene initialize thread exists.
 * @param pUser scene creator user
 * @return whether the thread exists
 */
bool isExistSceneInitThread(const IUseSceneCreator* pUser) {
    return pUser->getSceneCreator()->isExistInitThread();
}

/**
 * Sets the sequence name for the actor pick tool. Does nothing in release builds.
 * @param pSequence sequence
 * @param pScene scene
 */
void setSequenceNameForActorPickTool(Sequence* pSequence, Scene* pScene) {}
/**
 * Constructs a scene creator.
 * @param pGameSystemInfo game system info
 * @param pAudioDirector audio director
 * @param pGameDataHolder game data holder
 * @param pScreenCaptureExecutor screen capture executor
 * @param pSceneFactory scene factory
 */
// Preserve the constructor call used by initSceneCreator.
__attribute__((noinline)) SceneCreator::SceneCreator(const GameSystemInfo* pGameSystemInfo, AudioDirector* pAudioDirector,
                           GameDataHolderBase* pGameDataHolder,
                           ScreenCaptureExecutor* pScreenCaptureExecutor,
                           alSceneFunction::SceneFactory* pSceneFactory)
    : mGameSystemInfo(pGameSystemInfo), mGameDataHolder(pGameDataHolder),
      mAudioDirector(pAudioDirector), mScreenCaptureExecutor(pScreenCaptureExecutor),
      mSceneFactory(pSceneFactory) {}

/**
 * Creates a scene in the scene heap and initializes it, optionally on an initialize thread.
 * @param pClassName scene class name
 * @param pStageName stage name
 * @param scenarioNo scenario number
 * @param pSceneName scene name
 * @param isThreadInit whether the scene is initialized on an initialize thread
 * @param priority thread priority
 * @param pHeapName scene heap name, or null to use the stage name
 * @return created scene
 */
Scene* SceneCreator::createScene(const char* pClassName, const char* pStageName, s32 scenarioNo,
                                 const char* pSceneName, bool isThreadInit, s32 priority,
                                 const char* pHeapName) {
    const char* heapName = (pHeapName != nullptr) ? pHeapName : pStageName;
    setCustomSceneHeapAlloc(mSceneFactory->tryGetCustomAlloc(pClassName));
    createSceneHeap(heapName);
    SceneHeapSetter setter;
    alSceneFunction::SceneFactory* factory = mSceneFactory;
    const char* name = factory->convertName(pClassName);
    const NameToCreator<alSceneFunction::SceneFunction>* entries = factory->mFuncs;
    s32 index = 0;

    while (!isEqualString(name, entries[index].name)) {
        index++;
    }

    Scene* scene = entries[index].func();
    SceneInitInfo* info = new SceneInitInfo(mGameSystemInfo, mAudioDirector, mGameDataHolder,
                                            mScreenCaptureExecutor, pStageName, scenarioNo,
                                            pSceneName);
    setCpuBoost(true, false);

    if (isThreadInit) {
        mInitThread = createAndStartInitializeThread(setter.mSceneHeap, priority,
                                                     SceneInitFunctor(scene, &Scene::init, *info));
    } else {
        scene->init(*info);
        setCpuBoost(false, false);
    }

    return scene;
}

}  // namespace al
