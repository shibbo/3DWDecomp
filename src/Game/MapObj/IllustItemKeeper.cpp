#include "MapObj/IllustItemKeeper.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "System/GameDataHolderWriter.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/Scene/SceneObjUtil.hpp"

IllustItemKeeper::IllustItemKeeper() {}

void IllustItemKeeper::initAfterPlacementSceneObj(const al::ActorInitInfo& rInfo) {
    mSceneObjHolder = rInfo.getActorSceneInfo().sceneObjHolder;
    mAcquired = GameDataFunction::isAcquireIllustItem(GameDataHolderAccessor(this));
}

void IllustItemKeeper::declare() {
    mDeclared = true;
}

void IllustItemKeeper::acquire() {
    mAcquiredThisStage = true;
    if (!mAcquired) {
        mAcquired = true;
        GameDataFunction::acquireIllustItem(GameDataHolderWriter(this));
    }
}

namespace rc {
void declareIllustItem(const al::LiveActor* pActor) {
    static_cast<IllustItemKeeper*>(al::getSceneObj(pActor, 11))->declare();
}

void acquireIllustItem(const al::LiveActor* pActor) {
    static_cast<IllustItemKeeper*>(al::getSceneObj(pActor, 11))->acquire();
}
}

const char* IllustItemKeeper::getSceneObjName() const {
    return "イラストアイテム保持者";
}

al::SceneObjHolder* IllustItemKeeper::getSceneObjHolder() const {
    return mSceneObjHolder;
}
