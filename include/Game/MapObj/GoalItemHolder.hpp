#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include <container/seadPtrArray.h>
namespace al { class LiveActor; }
class GoalItem;
class HeadIslandClear;
class DemoSkipLayout;
class WindowProcessing;
class GoalItemHolder : public al::ISceneObj {
public:
    GoalItemHolder();
    const char* getSceneObjName() const override;
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    void initSceneObj() override;
    void registerGoalItem(GoalItem*, int, int);
    const sead::PtrArray<GoalItem>& getGoalItems() const;
    GoalItem* getGoalItemByIndex(int) const;
    GoalItem* getGoalItem(int, int) const;
    int getGoalItemNum() const;
    void appearClearLayout();
    void endClearLayout();
    void appearWindowProcessing();
    bool isClearLayoutKilled();
    DemoSkipLayout* getSkipLayout();
    void setClearLayoutText(int, int);
    void killEffect();
    const char* getNextGoalItemGuideMessage(const al::LiveActor*) const;
    bool isLastShineNeko() const;
    bool isLastShineDisaster() const;

    /**
     * @brief Get the goal item currently being collected.
     * @return The goal item, or nullptr.
     */
    GoalItem* getCurrentGoalItem() const { return mCurrentGoalItem; }

    /**
     * @brief Set the goal item being collected.
     * @param pGoalItem The goal item, or nullptr once done.
     */
    void setCurrentGoalItem(GoalItem* pGoalItem) { mCurrentGoalItem = pGoalItem; }

    /**
     * @brief Set whether a collect demo is running.
     * @param isDemo Whether a collect demo is running.
     */
    void setCollectDemo(bool isDemo) { mUnknown38 = isDemo; }
    bool isCollectDemo() const { return mUnknown38; }
    void setWindowProcessing(WindowProcessing* pWindow) { mWindowProcessing = pWindow; }
private:
    sead::PtrArray<GoalItem> mGoalItems;
    GoalItem* mCurrentGoalItem = nullptr;
    HeadIslandClear* mClearLayout = nullptr;
    DemoSkipLayout* mSkipLayout = nullptr;
    WindowProcessing* mWindowProcessing = nullptr;
    bool mUnknown38 = false;
};
