#include "MapObj/EnterCatMarioMiddleViewRocket.hpp"
#include "Library/LiveActor/Util/ActorInitUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"

EnterCatMarioMiddleViewRocket::EnterCatMarioMiddleViewRocket(const char* pName)
    : al::LiveActor(pName) {}

EnterCatMarioMiddleViewRocket::~EnterCatMarioMiddleViewRocket() {}

void EnterCatMarioMiddleViewRocket::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    mRock = new al::LiveActor("登場!ネコマリオ中景岩");
    al::initActorWithArchiveName(mRock, rInfo, "EnterCatMarioRock", nullptr);
    mRocket = new al::LiveActor("登場!ネコマリオ中景ロケット");
    al::initActorWithArchiveName(mRocket, rInfo, "EnterCatMarioRocket", nullptr);
    if (GameDataFlagFunction::isAlreadyOpenCourseSelectRocket(GameDataHolderAccessor(this))) {
        mRock->kill();
        mRocket->appear();
    } else {
        mRocket->kill();
        mRock->appear();
    }
    makeActorDead();
}
