#pragma once

#include "Library/Scene/IUseSceneCreator.hpp"
#include <basis/seadTypes.h>
class CourseSelectScene;
class StateResultAndSceneInit;
class WindowProcessing;
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class ProductSequence;
class ProductStageStartParam;
class StageWipeKeeper;
class WipeCurtainResult;

namespace al {
class LayoutInitInfo;
class ScreenCaptureExecutor;
struct SequenceInitInfo;
} // namespace al

/**
 * @brief Sequence state of the Super Mario 3D World course select.
 */
class ProductStateCourseSelect : public al::HostStateBase<ProductSequence>,
                                 public al::IUseSceneCreator {
  public:
    ProductStateCourseSelect(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                             StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                             const al::LayoutInitInfo& rLayoutInfo,
                             ControllerConnectChecker* pConnectChecker,
                             al::ScreenCaptureExecutor* pScreenCaptureExecutor,
                             WipeCurtainResult* pWipeCurtainResult);

    ~ProductStateCourseSelect() override;
    void init() override;
    void appear() override;
    void exeLoadStage();
    void exeLoadStageWithResult();
    void exeCourseSelect();
    void exeDestroyStage();
    void exeSave();
    void exeEnd();
    bool isResult() const;
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }
    bool isGotoTitle() const;
    bool isLoadGame() const;
    bool isEffectDraw() const;

  private:
    ProductStageStartParam* mpStartParam;
    StageWipeKeeper* mpWipeKeeper;
    CourseSelectScene* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    StateResultAndSceneInit* mpResultState = nullptr;
    bool mIsGotoTitle = false;
    ControllerConnectChecker* mpConnectChecker;
    WindowProcessing* mpSaveWindow;
};

static_assert(sizeof(ProductStateCourseSelect) == 0x68);
