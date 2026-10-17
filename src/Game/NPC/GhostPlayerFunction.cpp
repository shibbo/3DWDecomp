#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerPlayer.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/CourseInfoHolder.hpp"
namespace al { class NexGhostDataHolder; }
class MetaBinaryData;
class GhostPlayerLoaderBase;
namespace {
const char* const sGhostActionPrefixes[] = {"Mini", "Giant", "RaccoonDog", "Fire", "Boomerang"};
}
namespace GhostPlayerFunction {
bool isExistGhostPlayerDirector(const al::IUseSceneObjHolder* user) {
    return user->getSceneObjHolder() && al::isExistSceneObj(user, 9);
}
GhostPlayerDirector* getGhostPlayerDirector(const al::IUseSceneObjHolder* user) {
    return isExistGhostPlayerDirector(user) ? al::getSceneObj<GhostPlayerDirector>(user, 9) : nullptr;
}
void registerGhostPlayerPlayer(GhostPlayerPlayer* player) {
    const al::IUseSceneObjHolder* user = player;
    user->getSceneObjHolder();
    al::isExistSceneObj(user, 9);
    al::getSceneObj<GhostPlayerDirector>(user, 9)->registerPlayer(player);
}
bool isCourseGhostPlayer(GameDataHolderAccessor accessor, int course) {
    return GameDataFunction::findGhostStageSerialId(accessor, course) != -1;
}
bool isCurrentCourseGhostPlayer(GameDataHolderAccessor accessor) {
    return isCourseGhostPlayer(accessor, GameDataFunction::getPlayingCourseId(accessor));
}
bool isEnablePlayGhostPlayer(GameDataHolderAccessor accessor, int course) {
    if (GameDataFunction::isInvalidCourseId(course) || !isCourseGhostPlayer(accessor, course)
        || !GameDataFlagFunction::isAlreadyOpenNetworkSetting(accessor)
        || GameDataFunction::getGhostSetting(accessor) == 1) return false;
    return CourseInfoFunction::isClear(accessor, course);
}
bool isEnablePlayGhostPlayer(GameDataHolderAccessor accessor) {
    return isEnablePlayGhostPlayer(accessor, GameDataFunction::getPlayingCourseId(accessor));
}
bool isAvailableNetwork(const GhostPlayerPlayer*) { return false; }
GhostPlayerLoaderBase* createGhostPlayerLoader(GhostPlayerPlayer*, const char*) { return nullptr; }
u32 getGhostPlayerRecorderBufferSize() { return GhostPlayerRecorder::getRecorderBufferSize(); }
void makeGhostActionName(sead::BufferedSafeString* out, const char* action, const al::LiveActor* actor) {
    out->format("%s", action);
    for (u32 i = 0; i < 5; ++i) {
        const char* prefix = sGhostActionPrefixes[i];
        if (out->findIndex(prefix) == -1) continue;
        al::StringTmp<64> doubled("%s%s", prefix, prefix);
        if (al::isEqualString(action, doubled.cstr())) {
            out->format(prefix);
            break;
        }
        if (al::isEqualString(action, prefix)) continue;
        al::tryReplaceString(out, prefix, "");
        break;
    }
    if (!al::isExistAction(actor, out->cstr())) out->format("%s", "Wait");
}
bool copyGhostDataToStocker(al::NexGhostDataHolder*, u8*, int, const MetaBinaryData*, const char*) {
    return true;
}
}