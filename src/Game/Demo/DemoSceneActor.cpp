#include "Demo/DemoSceneActor.hpp"

#include "Demo/DemoActionList.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/File/FileUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Nerve/NerveUtil.hpp"
#include "Project/Base/StringUtil.hpp"
#include "Project/Play/Actor/ActorAlphaCtrl.hpp"
#include "System/Data/SingleModeDataFunction.hpp"
#include "System/GameDataFunction.hpp"
#include "System/GameDataHolderAccessor.hpp"
#include "Util/ControlUserUtil.hpp"

namespace {
NERVE_DECL(DemoSceneActor, Wait);
NERVES_MAKE_NOSTRUCT(DemoSceneActor, Wait)
}

/** @brief Creates a demo model with identity placement. @param pName Actor name.
 * @param pHolder Owning demo scene. */
DemoSceneActor::DemoSceneActor(const char* pName, DemoSceneActorHolder* pHolder)
    : al::LiveActor(pName), mHolder(pHolder) {
    mPlacementMtx.makeIdentity();
    mLocalMtx.makeIdentity();
}

/** @brief Initializes model playback and placement. @param rInfo Actor placement.
 * @param rDemoInfo Parent demo placement. @param pMtx Optional parent transform. */
void DemoSceneActor::initDemoSceneActor(const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pMtx) {
    init(rInfo);
    if (mAlphaCtrl)
        mAlphaCtrl->setOn(false);
    mActions = new DemoActionList(rInfo, "Action");
    DemoSceneActorFunction::calcPlacementBaseMtx(&mPlacementMtx, rInfo, rDemoInfo, pMtx, &mLocalMtx);
}

/** @brief Recomputes placement relative to a parent. @param pMtx Parent transform. */
void DemoSceneActor::setPlacementBaseMtx(const sead::Matrix34f* pMtx) {
    mPlacementMtx.setMul(*pMtx, mLocalMtx);
}

/** @brief Loads the model and configures its color variant. @param rInfo Placement data. */
void DemoSceneActor::init(const al::ActorInitInfo& rInfo) {
    const char* modelName = nullptr;
    if (!alPlacementFunction::tryGetModelName(&modelName, rInfo) ||
        !al::isExistFile(al::StringTmp<64>("ObjectData/%s.szs", modelName).cstr())) {
        al::initActorPoseTRSV(this);
        makeActorDead();
        return;
    }
    const char* suffix = nullptr;
    al::tryGetStringArg(&suffix, rInfo, "SuffixName");
    if (al::isEqualString(modelName, "KoopaJr") &&
        SingleModeDataFunction::isMeowserJrAvailable(GameDataHolderAccessor(rInfo.mActorSceneInfo.sceneObjHolder)))
        modelName = "KoopaJrFur";
    al::initActorWithArchiveName(this, rInfo, modelName, suffix);
    if (al::isEqualSubString(modelName, "KoopaJr")) {
        mIsKoopaJr = true;
    } else if (al::isEqualString(modelName, "DarkBowser") && rInfo.mActorSceneInfo.sceneObjHolder &&
               SingleModeDataFunction::isDarkBowserV2Available(GameDataHolderAccessor(rInfo.mActorSceneInfo.sceneObjHolder))) {
        al::startMtpAnimAndSetFrameAndStop(this, "DarkBowserV2", 1.0f);
    }
    al::updatePoseMtx(this, &mPlacementMtx);
    al::invalidateClipping(this);
    al::initNerve(this, &NrvDemoSceneActorWait, 0);
    al::tryGetArg(&mWorldColor, rInfo, "EnableWorldColorAnim");
    al::tryGetArg(&mColorFrame, rInfo, "ColorFrame");
    int frame;
    if (mWorldColor) {
        int world = GameDataFunction::calcPlayingWorldId(GameDataHolderAccessor(this));
        frame = world > 1 ? world : 1;
    } else {
        frame = mColorFrame;
        if (frame < 0) {
            makeActorDead();
            return;
        }
    }
    {
        if (al::tryStartMclAnimIfExist(this, "Color"))
            al::setMclAnimFrameAndStop(this, frame);
        if (al::tryStartMtpAnimIfExist(this, "Color"))
            al::setMtpAnimFrameAndStop(this, frame);
    }
    makeActorDead();
}

/** @brief Appears at the composed demo transform. @param rMtx Demo transform. */
void DemoSceneActor::startDemo(const sead::Matrix34f& rMtx) {
    sead::Matrix34f mtx;
    mtx.setMul(rMtx, mPlacementMtx);
    al::updatePoseMtx(this, &mtx);
    makeActorAppeared();
}

/** @brief Starts an indexed action, preferring player-count variants. @param index Action index. */
void DemoSceneActor::startAction(int index) {
    const char* action = mActions->getActionName(index);
    if (DemoSceneActorFunction::isHideAction(action)) {
        if (!al::isHideModel(this))
            al::hideModel(this);
    } else if (DemoSceneActorFunction::isShowAction(action)) {
        if (al::isHideModel(this))
            al::showModel(this);
    } else if (action) {
        if (al::isHideModel(this))
            al::showModel(this);
        al::StringTmp<64> variant("%s%d", action, rc::getActiveControlUserNum(GameDataHolderAccessor(this)));
        if (al::isExistAction(this, variant.cstr()))
            al::startAction(this, variant.cstr());
        else
            al::startAction(this, action);
        if (mSubActorKeeper) {
            int count = al::getSubActorNum(this);
            for (int i = 0; i < count; i++) {
                al::LiveActor* sub = al::getSubActor(this, i);
                if (al::isExistAction(sub, variant.cstr()))
                    al::startAction(sub, variant.cstr());
                else
                    al::tryStartAction(sub, action);
            }
        }
    }
}

/** @brief Attempts a named action or show/hide command. @param pName Action name.
 * @return Whether the action or command succeeded. */
bool DemoSceneActor::tryStartActionByName(const char* pName) {
    if (DemoSceneActorFunction::isHideAction(pName)) {
        if (!al::isHideModel(this))
            al::hideModel(this);
        return true;
    }
    if (DemoSceneActorFunction::isShowAction(pName)) {
        if (al::isHideModel(this))
            al::showModel(this);
        return true;
    }
    if (!pName)
        return false;
    if (al::isHideModel(this))
        al::showModel(this);
    GameDataHolderAccessor accessor(this);
    al::StringTmp<64> variant("%s%d", pName, rc::getActiveControlUserNum(accessor));
    bool result;
    if (al::isExistAction(this, variant.cstr())) {
        al::startAction(this, variant.cstr());
        result = true;
    } else {
        result = al::tryStartAction(this, pName);
    }
    if (mSubActorKeeper) {
        int count = al::getSubActorNum(this);
        for (int i = 0; i < count; i++) {
            al::LiveActor* sub = al::getSubActor(this, i);
            if (al::isExistAction(sub, variant.cstr()))
                al::startAction(sub, variant.cstr());
            else
                al::tryStartAction(sub, pName);
        }
    }
    return result;
}

/** @brief Hides the actor when the demo ends. */
void DemoSceneActor::endDemo() { makeActorDead(); }
/** @brief Waits for externally started actions. */
void DemoSceneActor::exeWait() {}

namespace DemoSceneActorFunction {
/** @brief Computes demo placement, retaining the local transform if requested.
 * @param pOut Composed transform. @param rInfo Actor placement. @param rDemoInfo Parent placement.
 * @param pBaseMtx Optional explicit parent transform. @param pLocalMtx Optional local output. */
void calcPlacementBaseMtx(sead::Matrix34f* pOut, const al::ActorInitInfo& rInfo,
    const al::ActorInitInfo& rDemoInfo, const sead::Matrix34f* pBaseMtx, sead::Matrix34f* pLocalMtx) {
    sead::Matrix34f local;
    local.makeIdentity();
    al::tryGetMatrixTR(&local, rInfo);
    if (pLocalMtx)
        *pLocalMtx = local;
    if (pBaseMtx)
        pOut->setMul(*pBaseMtx, local);
    else
        al::calcMatrixMultParent(pOut, rInfo, rDemoInfo);
}

/** @brief Recognizes the hide command. @param pName Action name. @return Whether it hides a model. */
bool isHideAction(const char* pName) { return pName && al::isEqualString("<HideModel>", pName); }
/** @brief Recognizes the show command. @param pName Action name. @return Whether it shows a model. */
bool isShowAction(const char* pName) { return pName && al::isEqualString("<ShowModel>", pName); }
}
