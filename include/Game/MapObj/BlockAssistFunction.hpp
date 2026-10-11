#pragma once
class BlockAssistLeaf;
class CheckpointFlag;
namespace al { class IUseSceneObjHolder; }

namespace BlockAssistFunction {
void setCheckpointFlag(const al::IUseSceneObjHolder*, const CheckpointFlag*);
void setBlockAssist(BlockAssistLeaf* pBlock);
bool isBlockAssistSetCheckPoint(const BlockAssistLeaf* pBlock);
bool isBlockAssistSetStartPoint(const BlockAssistLeaf* pBlock);
bool isEnableAppearBlockAssist(const BlockAssistLeaf* pBlock);
}
