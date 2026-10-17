#pragma once

#include <basis/seadTypes.h>

namespace al {
class ActorInitInfo;
class LiveActor;
}  // namespace al

class CourseSelectMiniature;
class GameDataHolderAccessor;
class GameDataHolderWriter;

namespace rc {
s32 calcOpenNodePriority(CourseSelectMiniature* pMiniature);
s32 tryFindPlacementWorldId(const al::LiveActor* pActor, bool* pIsNoWorldStartDemo);
s32 getCourseSelectObjectID(const al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
bool isDisappearCourseSelectObject(GameDataHolderAccessor accessor, s32 objectId);
void acquirerCourseSelectObject(GameDataHolderWriter writer, s32 objectId);
}  // namespace rc
