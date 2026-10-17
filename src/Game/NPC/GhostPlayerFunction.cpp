#include "NPC/GhostPlayerFunction.hpp"
#include "NPC/GhostPlayerDirector.hpp"
#include "NPC/GhostPlayerRecorder.hpp"
#include "NPC/GhostPlayerPlayer.hpp"
#include "Library/Scene/SceneObjUtil.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"
#include "Library/LiveActor/Util/ActorActionUtil.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/CourseInfoHolder.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Base/StringOpUtil.hpp"
namespace {
const char* const sGhostActionPrefixes[] = {"Mini", "Giant", "RaccoonDog", "Fire", "Boomerang"};
}
namespace GhostPlayerFunction {
/** @brief Checks for a scene ghost director. @param pUser Scene access. @return Whether the
 * director exists. */
bool isExistGhostPlayerDirector(const al::IUseSceneObjHolder* pUser) {
    return pUser->getSceneObjHolder() && al::isExistSceneObj(pUser, 9);
}
/** @brief Finds the scene ghost director. @param pUser Scene access. @return Director, or nullptr.
 */
GhostPlayerDirector* getGhostPlayerDirector(const al::IUseSceneObjHolder* pUser) {
    if (!isExistGhostPlayerDirector(pUser)) {
        return nullptr;
    }
    return static_cast<GhostPlayerDirector*>(al::getSceneObj(pUser, 9));
}
/** @brief Registers scene playback. @param pPlayer Playback actor. */
void registerGhostPlayerPlayer(GhostPlayerPlayer* pPlayer) {
    getGhostPlayerDirector(pPlayer)->setPlayer(pPlayer);
}
/** @brief Checks the course ghost-data mapping. @param data Game data. @param courseId Course
 * identifier. @return Whether a ghost course exists. */
bool isCourseGhostPlayer(GameDataHolderAccessor data, int courseId) {
    return GameDataFunction::findGhostStageSerialId(data, courseId) != -1;
}
/** @brief Checks the current course ghost-data mapping. @param data Game data. @return Whether a
 * ghost course exists. */
bool isCurrentCourseGhostPlayer(GameDataHolderAccessor data) {
    return isCourseGhostPlayer(data, GameDataFunction::getPlayingCourseId(data));
}
/** @brief Checks course progress and ghost settings. @param data Game data. @param courseId Course
 * identifier. @return Whether playback is enabled. */
bool isEnablePlayGhostPlayer(GameDataHolderAccessor data, int courseId) {
    if (GameDataFunction::isInvalidCourseId(courseId) || !isCourseGhostPlayer(data, courseId) ||
        !GameDataFlagFunction::isAlreadyOpenNetworkSetting(data) ||
        GameDataFunction::getGhostSetting(data) == 1) {
        return false;
    }
    return CourseInfoFunction::isClear(data, courseId);
}
/** @brief Checks playback for the current course. @param data Game data. @return Whether playback
 * is enabled. */
bool isEnablePlayGhostPlayer(GameDataHolderAccessor data) {
    return isEnablePlayGhostPlayer(data, GameDataFunction::getPlayingCourseId(data));
}
/** @brief Reports the original build network capability. @param pPlayer Unused playback actor.
 * @return False in this build. */
bool isAvailableNetwork(const GhostPlayerPlayer* pPlayer) { return false; }
/** @brief Returns the original build loader result. @param pPlayer Unused playback actor. @param
 * pName Unused loader name. @return nullptr in this build. */
GhostPlayerLoaderBase* createGhostPlayerLoader(GhostPlayerPlayer* pPlayer, const char* pName) {
    return nullptr;
}
/** @brief Gets the recorder storage requirement. @return Buffer size in bytes. */
u32 getGhostPlayerRecorderBufferSize() { return GhostPlayerRecorder::getRecorderBufferSize(); }
/** @brief Converts power-up action names to an available ghost animation. @param pName Output
 * action name. @param pAction Source action. @param pActor Actor supplying available animations. */
void makeGhostActionName(sead::BufferedSafeString* pName, const char* pAction,
                         const al::LiveActor* pActor) {
    pName->format("%s", pAction);
    for (int i = 0; i < 5; ++i) {
        const char* pPrefix = sGhostActionPrefixes[i];
        if (pName->include(pPrefix)) {
            al::StringTmp<64> doubled("%s%s", pPrefix, pPrefix);
            if (al::isEqualString(pAction, doubled.cstr())) {
                pName->format(pPrefix);
                break;
            }
            if (al::isEqualString(pAction, pPrefix)) {
                continue;
            }
            al::tryReplaceString(pName, pPrefix, "");
            break;
        }
    }
    if (!al::isExistAction(pActor, pName->cstr())) {
        pName->format("%s", "Wait");
    }
}
/** @brief Preserves the original build stocker-copy result. @param pHolder Unused storage holder.
 * @param pData Unused ghost data. @param size Unused data size. @param pMeta Unused metadata.
 * @param pName Unused record name. @return True in this build. */
bool copyGhostDataToStocker(al::NexGhostDataHolder* pHolder, unsigned char* pData, int size,
                            const MetaBinaryData* pMeta, const char* pName) {
    return true;
}
} // namespace GhostPlayerFunction
