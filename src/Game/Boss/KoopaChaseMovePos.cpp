#include "Boss/KoopaChaseMovePos.hpp"
#include "Boss/KoopaChaseFunction.hpp"
#include "Boss/KoopaChaseWarpDummy.hpp"
#include "MapObj/WarpCube.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/Play/Placement/PlacementInfo.hpp"
#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Library/StageSwitch/Core/StageSwitchKeeper.hpp"

/**
 * @brief Creates a waypoint, its linked actors, and the following route points.
 * @param rPlacement Waypoint placement data.
 * @param rInfo Host actor initialization data.
 * @param pPrevious Previous waypoint; unused by the original implementation.
 * @param pPoints Array collecting every waypoint on the route.
 */
KoopaChaseMovePos::KoopaChaseMovePos(const al::PlacementInfo& rPlacement,
    const al::ActorInitInfo& rInfo, KoopaChaseMovePos* pPrevious,
    sead::PtrArray<KoopaChaseMovePos>* pPoints)
    : mTrans(sead::Vector3f::zero), mPoint18(nullptr), mType(0), mValue24(0),
      mWarpActor(nullptr), mWarpCube(nullptr), mWarpDummy(nullptr),
      mName("クッパチェイス移動位置"), mStageSwitchKeeper(nullptr) {
    al::getTrans(&mTrans, rPlacement);
    al::tryGetArg(&mType, rPlacement, "MoveType");
    al::tryGetArg(&mValue24, rPlacement, "ThrowType");
    pPoints->pushBack(this);
    if (al::calcLinkChildNum(rPlacement, "MoveAppearObj") > 0) {
        al::ActorInitInfo info;
        info.initViewIdSelf(&rPlacement, rInfo);
        mWarpActor = KoopaChaseFunction::tryCreateLinkObj(info, "MoveAppearObj", 0);
    }
    if (al::calcLinkChildNum(rPlacement, "WarpCube") > 0) {
        al::ActorInitInfo info;
        info.initViewIdSelf(&rPlacement, rInfo);
        if (auto* actor = KoopaChaseFunction::tryCreateLinkObj(info, "WarpCube", 0)) {
            mWarpCube = static_cast<WarpCube*>(actor);
        }
    }
    if (al::calcLinkChildNum(rPlacement, "WarpDummy") > 0) {
        al::ActorInitInfo info;
        info.initViewIdSelf(&rPlacement, rInfo);
        if (auto* actor = KoopaChaseFunction::tryCreateLinkObj(info, "WarpDummy", 0)) {
            mWarpDummy = static_cast<KoopaChaseWarpDummy*>(actor);
        }
    }
    al::initStageSwitch(this, rInfo.getStageSwitchDirector(), rPlacement);
    al::PlacementInfo links;
    if (al::tryGetLinksInfo(&links, rPlacement, "MovePos")) {
        mPoint18 = new KoopaChaseMovePos(links, rInfo, this, pPoints);
    }
}

/** @brief Allocates the waypoint's stage-switch keeper. */
void KoopaChaseMovePos::initStageSwitchKeeper() {
    mStageSwitchKeeper = new al::StageSwitchKeeper;
}

/** @brief Returns the waypoint's name. @return Debug name. */
const char* KoopaChaseMovePos::getName() const { return mName; }

/** @brief Returns the waypoint's switch keeper. @return Stage-switch keeper. */
al::StageSwitchKeeper* KoopaChaseMovePos::getStageSwitchKeeper() const {
    return mStageSwitchKeeper;
}
