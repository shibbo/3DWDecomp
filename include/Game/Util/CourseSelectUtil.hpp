#pragma once

class GameDataHolderAccessor;
class GameDataHolderWriter;
namespace al {
class LiveActor;
class ActorInitInfo;
}

namespace rc {
int getCourseSelectObjectID(const al::LiveActor* pActor, const al::ActorInitInfo& rInfo);
bool isDisappearCourseSelectObject(GameDataHolderAccessor accessor, int objectId);
void acquirerCourseSelectObject(GameDataHolderWriter writer, int objectId);
int getCourseSelectKinokoOneUpId(const al::LiveActor* pActor);
bool isDisappearCourseSelectKinokoOneUp(GameDataHolderAccessor accessor, int oneUpId);
void acquirerCourseSelectKinokoOneUp(GameDataHolderAccessor accessor, int oneUpId);
}
