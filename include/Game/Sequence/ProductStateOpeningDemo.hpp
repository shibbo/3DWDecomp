#pragma once

#include "Library/Nerve/NerveStateBase.hpp"
#include "Library/Scene/IUseSceneCreator.hpp"

class ProductSequence;
class DemoScene;
namespace al { struct SequenceInitInfo; }

class ProductStateOpeningDemo : public al::HostStateBase<ProductSequence>, public al::IUseSceneCreator {
public:
    ProductStateOpeningDemo(ProductSequence* pSequence, const al::SequenceInitInfo& rInfo);
    ~ProductStateOpeningDemo() override;
    void init() override;
    void appear() override;
    void exeLoad();
    void exeWait();
    void exeDestroy();
    void exeEnd();

    /**
     * @brief Get the opening-demo scene creator.
     * @return Scene creator installed during initialization.
     */
    al::SceneCreator* getSceneCreator() const override { return mpSceneCreator; }
    /**
     * @brief Store the opening-demo scene creator.
     * @param pCreator Scene creator to retain.
     */
    void setSceneCreator(al::SceneCreator* pCreator) override { mpSceneCreator = pCreator; }

private:
    DemoScene* mpScene = nullptr;
    al::SceneCreator* mpSceneCreator = nullptr;
};
static_assert(sizeof(ProductStateOpeningDemo) == 0x38);
