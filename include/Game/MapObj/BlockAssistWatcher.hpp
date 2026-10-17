#pragma once
#include "Library/Scene/ISceneObj.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"
#include <math/seadVector.h>
class GameDataHolder;
class BlockAssistLeaf;
class CheckpointFlag;
class BlockAssistWatcher : public al::ISceneObj, public al::IUseSceneObjHolder {
public:
    BlockAssistWatcher(int, GameDataHolder*);
    void initAfterPlacementSceneObj(const al::ActorInitInfo&) override;
    const char* getSceneObjName() const override;
    al::SceneObjHolder* getSceneObjHolder() const override;
    void setCheckpointFlagTrans(const CheckpointFlag*);
    void setBlockAssist(BlockAssistLeaf*);
    bool isBlockAssistSetStartPoint(const BlockAssistLeaf*) const;
    bool isBlockAssistSetCheckPoint(const BlockAssistLeaf*) const;
    bool isEnableAppearBlockAssist(const BlockAssistLeaf*) const;
private:
    struct BlockInfo {
        BlockAssistLeaf* block = nullptr;
        float distance = 0.0f;
        bool isStartPoint = true;
    };
    al::SceneObjHolder* mSceneObjHolder = nullptr;
    BlockInfo** mBlocks;
    GameDataHolder* mGameDataHolder;
    int mBlockCount = 0;
    int mCourseId;
    sead::Vector3f mCheckpointTrans = sead::Vector3f(0.0f, 0.0f, 0.0f);
    bool mHasCheckpoint = false;
};
