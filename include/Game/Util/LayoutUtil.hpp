#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>
#include <prim/seadSafeString.h>

namespace al {
class IUseCamera;
class IUseLayout;
class IUseMessageSystem;
class IUseSceneObjHolder;
class LayoutActor;
class LiveActor;
} // namespace al
class ButtonGroup;
class GameDataHolder;

namespace rc {
const char16_t* getMessageStringStageName(al::IUseMessageSystem* pMsgSystem,
                                          const GameDataHolder* pHolder, s32 courseId);
void setPaneStageNameString(al::LayoutActor* pActor, al::IUseLayout* pLayout,
                            const char* pPaneName, const GameDataHolder* pHolder, s32 courseId);
void setPaneWorldString(al::LayoutActor* pActor, al::IUseLayout* pLayout, const char* pPaneName,
                        const char* pFileName, const char* pLabel, u16 worldId,
                        const char* pSubPaneName, bool isSystemMessage);
void setPaneWorldStageString(al::LayoutActor* pActor, al::IUseLayout* pLayout,
                             const char* pPaneName, const char* pFileName, const char* pLabel,
                             u16 worldId, u16 stageId, const GameDataHolder* pHolder,
                             const char* pSubPaneName, bool isSystemMessage);
void replacePaneMsgNumber(al::IUseMessageSystem* pMsgSystem, al::IUseLayout* pLayout,
                          const char* pPaneName, const char* pFileName, const char* pLabel,
                          s32 number);
void replacePaneMsgNumber2(al::IUseMessageSystem* pMsgSystem, al::IUseLayout* pLayout,
                           const char* pPaneName, const char* pFileName, const char* pLabel,
                           s32 number1, s32 number2);
void replacePaneMsgNumber3(al::IUseMessageSystem* pMsgSystem, al::IUseLayout* pLayout,
                           const char* pPaneName, const char* pFileName, const char* pLabel,
                           s32 number1, s32 number2, s32 number3);
void appearCameraChangeLayout(const al::IUseSceneObjHolder* pHolder);
void disappearCameraChangeLayout(const al::IUseSceneObjHolder* pHolder);
void disappearCameraChangeLayoutAndResetCameraMode(const al::IUseSceneObjHolder* pHolder);
void disappearGyroIconInKinopioBrigade(const al::IUseSceneObjHolder* pHolder);
void convertPlayerLifeToText(sead::WBufferedSafeString* pOut, s32 life);
void setPaneDecideIconFont(al::IUseLayout* pLayout, const char* pPaneName, s32 port);
void hideButton(ButtonGroup* pButtonGroup, const char* pButtonName);
void updateLayoutPosAtWorldPos(al::LayoutActor* pActor, const al::IUseCamera* pCamera,
                               const sead::Vector3f& rWorldPos, f32 range);
bool updateLayoutPosAtWorldPosWithInCheck(al::LayoutActor* pActor, const al::IUseCamera* pCamera,
                                          const sead::Vector3f& rWorldPos);
void updateLayoutPosAtOwner(al::LayoutActor* pActor, const al::LiveActor* pOwner,
                            const sead::Vector3f& rOffset, f32 range);
void updateLayoutPosAtOwner(al::LayoutActor* pActor, const al::LiveActor* pOwner,
                            const char* pJointName, const sead::Vector3f& rOffset,
                            f32 range);
} // namespace rc
