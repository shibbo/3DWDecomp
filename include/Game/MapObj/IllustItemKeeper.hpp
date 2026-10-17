#pragma once

#include "Library/Scene/ISceneObj.hpp"
#include "Library/Scene/IUseSceneObjHolder.hpp"

namespace al { class LiveActor; }

class IllustItemKeeper : public al::ISceneObj, public al::IUseSceneObjHolder {
public:
    IllustItemKeeper();
    void initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) override;
    const char* getSceneObjName() const override;
    al::SceneObjHolder* getSceneObjHolder() const override;
    void declare();
    void acquire();

    bool isDeclared() const { return mDeclared; }

private:
    al::SceneObjHolder* mSceneObjHolder = nullptr;
    bool mDeclared = false;
    bool mAcquired = false;
    bool mAcquiredThisStage = false;
};

static_assert(sizeof(IllustItemKeeper) == 0x20);

namespace rc {
void declareIllustItem(const al::LiveActor* pActor);
void acquireIllustItem(const al::LiveActor* pActor);
}
