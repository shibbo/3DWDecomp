#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class CourseSelectScene;
class StateResultAndSceneInit;
class StageWipeKeeper;
class ProductStageStartParam;
class WipeCurtainResult;
namespace al { struct SequenceInitInfo; class LayoutInitInfo; class ScreenCaptureExecutor; }

class ProductStateAfterEndingEvent : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateAfterEndingEvent(ProductSequence* pSequence, ProductStageStartParam* pStartParam,
                                 StageWipeKeeper* pWipeKeeper, const al::SequenceInitInfo& rInfo,
                                 const al::LayoutInitInfo& rLayoutInfo,
                                 al::ScreenCaptureExecutor* pCapture, WipeCurtainResult* pWipe);
    ~ProductStateAfterEndingEvent() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeEvent();
    bool isResult();
    bool isEffectDraw();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the post-ending scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the post-ending scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    ProductStageStartParam* mpStartParam;
    StageWipeKeeper* mpWipeKeeper;
    CourseSelectScene* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
    StateResultAndSceneInit* mpResultState = nullptr;
    unsigned char mUnreconstructed50[8];
};
static_assert(sizeof(ProductStateAfterEndingEvent) == 0x58);
