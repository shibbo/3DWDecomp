#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class BlockChoice;
class BlockChoiceWatcher : public al::LiveActor {
public:
    BlockChoiceWatcher(const char*);
    ~BlockChoiceWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWait();
    void disappearOtherBlock(int);
    void exeReleasedItem();

    /**
     * Gets whether the item of the chosen block was released.
     * @return true if the item was released.
     */
    bool isReleasedItem() const { return mIsReleasedItem; }
private:
    BlockChoice** mBlocks = nullptr;
    int mBlockCount = 0;
    bool mIsReleasedItem = false;
};
