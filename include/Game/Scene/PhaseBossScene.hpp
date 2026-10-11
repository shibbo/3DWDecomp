#pragma once

#include <basis/seadTypes.h>
#include "Scene/SingleModeScene.hpp"

namespace sead {
class Heap;
}  // namespace sead

/**
 * @brief Scene of a Bowser's Fury boss phase.
 */
class PhaseBossScene : public SingleModeScene {
  public:
    /**
     * @brief Custom scene heap allocator of the boss scenes.
     * @note Keeps the assets of the phase that follows the boss fight resident in a dedicated
     *       heap. Derives from al::MemorySceneHeapCustomAlloc in the game, whose virtual
     *       interface is declared under other names in the al headers.
     */
    class MemorySceneHeapCustomAlloc {
      public:
        MemorySceneHeapCustomAlloc();

        virtual bool createCustomSceneHeap(bool isCreate);
        virtual bool destroyCustomSceneHeap(bool isDestroy);
        virtual bool isDestroySceneResourceHeap() const;
        virtual size_t adjustStageResourceSize(size_t size);
        virtual void forceDestroySceneResourceHeap();

        /**
         * @brief Enable or disable the custom allocation.
         * @param isDisabled True to disable the custom allocation.
         */
        void setDisabled(bool isDisabled) { mIsDisabled = isDisabled; }

        /**
         * @brief Set the game data holder used to find the next phase.
         * @param pHolder The game data holder.
         */
        void setGameDataHolder(GameDataHolder* pHolder) { mGameDataHolder = pHolder; }

        s32 getNextPhase();
        bool tryLoadNextPhaseAssets();

      private:
        sead::Heap* mHeap = nullptr;               // 0x08
        bool mIsDisabled = false;                  // 0x10
        bool mIsLoadedNextPhaseAssets = false;     // 0x11
        bool mIsForceDestroy = false;              // 0x12
        GameDataHolder* mGameDataHolder = nullptr; // 0x18
    };

    PhaseBossScene();
    explicit PhaseBossScene(const char* pName);
    ~PhaseBossScene() override;

    void preInitPlacement(const al::ActorInitInfo& rInfo) override;
    void init(const al::SceneInitInfo& rInfo) override;
    void initIslandDataList() override;
    void initAreaObj(const al::ActorInitInfo& rInfo) override;
    void initPlacement(al::ActorInitInfo& rInfo) override;
    void initLighthouses(const al::ActorInitInfo& rInfo);
    void initPlacementBossLOD(const al::StageInfo* pStageInfo, const al::ActorInitInfo& rInfo,
                              const char* pListName);
    void updateDemoCutsceneAddOn() override;
    bool isGameEnd() const override;
    bool isPhaseEnd() const override;
    bool isChangePhase() const override;
    void handlePhaseEnd() override;
    void requestStageBgmStart() override;
    void exePhaseEnd();
    void exeGameEnd();
    void appear() override;
    void handleGameOver() override;
    void kill() override;
    bool allowRestartPoint() const override;
    bool isBossScene() const override;
    bool doZoneIDCheck() const override;

    static MemorySceneHeapCustomAlloc sCustomAlloc;

  private:
    bool mIsPhaseEnd = false;  // 0x379
    bool mIsGameOver = false;  // 0x37a
};
static_assert(sizeof(PhaseBossScene) == 0x380);
