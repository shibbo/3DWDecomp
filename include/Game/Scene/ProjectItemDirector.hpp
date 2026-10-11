#pragma once

#include "Library/Item/ItemDirectorBase.hpp"
#include "Project/AreaObj/IUseAreaObj.hpp"

namespace al {
class AreaObjDirector;
class LayoutInitInfo;
class PlayerHolder;
}  // namespace al
class GameDataHolder;
class ItemHolder;
class SceneLayoutBase;

/**
 * @brief The game's item director: spawns items by their placement name and hands out the
 * rewards when the player collects them.
 */
class ProjectItemDirector : public al::ItemDirectorBase, public al::IUseAreaObj {
public:
    ProjectItemDirector(const al::LayoutInitInfo& rInfo, GameDataHolder* pGameDataHolder,
                        al::PlayerHolder* pPlayerHolder, al::AreaObjDirector* pAreaObjDirector);

    void setSceneLayout(SceneLayoutBase* pSceneLayout);
    void createItemHolder(const al::ActorInitInfo& rInfo, bool isSingleMode);
    void appearItem(const char* pName, const sead::Vector3f& rTrans, const sead::Vector3f& rFront,
                    const al::HitSensor* pSensor, bool isTakeOut,
                    bool isPopUpOnCollide) const override;
    void acquirerItem(const al::LiveActor* pItem, al::HitSensor* pSensor,
                      const char* pName) const override;
    void declareItem(const char* pName, const al::ActorInitInfo& rInfo) override;
    void endInit() override;
    bool useStockItem(s32 port, s32 itemType);
    void setDemoMode();
    void resetDemoMode();

    /**
     * @brief Get the area director used for the water checks.
     * @return The scene's area-object director.
     */
    al::AreaObjDirector* getAreaObjDirector() const override { return mAreaObjDirector; }

    /**
     * @brief Get the holder of the pre-created items.
     * @return The item holder, or nullptr before createItemHolder().
     */
    ItemHolder* getItemHolder() const { return mItemHolder; }

    /**
     * @brief Get the HUD layout the items report to.
     * @return The scene layout, or nullptr before setSceneLayout().
     */
    SceneLayoutBase* getSceneLayout() const { return mSceneLayout; }

private:
    ItemHolder* mItemHolder = nullptr;           // 0x10
    SceneLayoutBase* mSceneLayout = nullptr;     // 0x18
    GameDataHolder* mGameDataHolder;             // 0x20
    al::PlayerHolder* mPlayerHolder;             // 0x28
    al::AreaObjDirector* mAreaObjDirector;       // 0x30
};
