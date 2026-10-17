#include "Boss/KoopaLastDemoEndActorHolder.hpp"
#include "Boss/KoopaLastPartsModel.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/LiveActor/SubActorUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "System/GameDataFlagFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
/** @brief Allocates the ending actor lists. @param pHost Host actor. */
KoopaLastDemoEndActorHolder::KoopaLastDemoEndActorHolder(al::LiveActor* pHost) : mHost(pHost) {
    mActors.allocBuffer(8, nullptr);
    mDemoActors.allocBuffer(8, nullptr);
}
/** @brief Creates the fairies and bottle used by the ending. @param rInfo Initialization data. @param pMtx Attachment matrix. */
void KoopaLastDemoEndActorHolder::init(const al::ActorInitInfo& rInfo, const sead::Matrix34f* pMtx) {
    for (int i = 0; i < 7; i++) {
        al::StringTmp<128> archive("FairyPrincess%02d", i + 1);
        auto* fairy = new KoopaLastPartsModel("妖精(真クッパデモ)");
        fairy->initPartsMtx(mHost, rInfo, archive.cstr(), pMtx, "LastKoopa");
        al::startAction(fairy, "LastTopWait");
        float frame = i * 30.0f;
        float max = al::getSklAnimFrameMax(fairy, 0);
        al::setSklAnimFrame(fairy, frame > max ? max : frame, 0);
        mActors.pushBack(fairy);
        if (GameDataFlagFunction::isEnableDemoLeaveCastleFairyPrincess(GameDataHolderAccessor(mHost), i + 1))
            mDemoActors.pushBack(fairy);
        fairy->makeActorDead();
    }
    auto* bottle = new KoopaLastPartsModel("妖精ボトル(真クッパデモ)");
    bottle->initPartsMtx(mHost, rInfo, "BottleOuterBig", pMtx, "KoopaLast");
    bottle->makeActorDead();
    al::startAction(bottle, "LastTopWait");
    int subCount = al::getSubActorNum(bottle);
    for (int i = 0; i < subCount; i++) al::startAction(al::getSubActor(bottle, i), "LastTopWait");
    mActors.pushBack(bottle);
    mDemoActors.pushBack(bottle);
    al::initSubActorKeeperNoFile(mHost, rInfo, 8);
    int count = mDemoActors.size();
    for (int i = 0; i < count; i++) al::registerSubActorSyncClipping(mHost, mDemoActors[i], false);
}
/** @brief Makes the selected ending actors visible. */
void KoopaLastDemoEndActorHolder::appear() {
    int count = mDemoActors.size();
    for (int i = 0; i < count; i++) mDemoActors(i)->makeActorAppeared();
}
/** @brief Releases the attachment matrix and starts the ending animations. */
void KoopaLastDemoEndActorHolder::startEndDemo() {
    int count = mDemoActors.size();
    for (int i = 0; i < count; i++) {
        auto* actor = mDemoActors(i);
        actor->disableHostMatrix();
        al::invalidateClipping(actor);
        al::startAction(actor, "DemoBattleLastKoopaEnd");
        int subCount = al::getSubActorNum(actor);
        for (int j = 0; j < subCount; j++)
            al::tryStartAction(al::getSubActor(actor, j), "DemoBattleLastKoopaEnd");
    }
}
