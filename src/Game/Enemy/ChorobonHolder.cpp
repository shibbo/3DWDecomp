#include "Enemy/ChorobonHolder.hpp"
#include "Enemy/Chorobon.hpp"
#include "Enemy/ChorobonItem.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Actor/ActorInitInfo.hpp"
#include "Library/LiveActor/Util/ActorSceneInfo.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"

namespace {
NERVE_DECL(ChorobonHolder, Watch)
NERVES_MAKE_NOSTRUCT(ChorobonHolder, Watch)
}

/** @brief Creates the holder. @param pName Placement name. */
ChorobonHolder::ChorobonHolder(const char* pName) : al::LiveActor(pName) {
    mActorName = "チョロボンホルダー";
}

/** @brief Creates and spaces the train on its rail. @param rInfo Placement information. */
void ChorobonHolder::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTRSV(this);
    al::initActorSRT(this, rInfo);
    al::initExecutorWatchObj(this, rInfo);
    if (rInfo.mActorSceneInfo.isSingleMode)
        al::initActorClipping(this, rInfo);
    if (!al::isExistRail(rInfo)) {
        makeActorDead();
        return;
    }
    al::tryGetArg(&mCount, rInfo, "WidthNum");
    s32 speed = static_cast<s32>(mSpeed);
    al::tryGetArg(&speed, rInfo, "MoveSpeed");
    mSpeed = speed;
    s32 itemIndex = 0;
    al::tryGetArg(&itemIndex, rInfo, "ItemInsertPosition");
    if (itemIndex > 0) {
        if (mCount < itemIndex)
            itemIndex = -1;
        --itemIndex;
    } else {
        itemIndex = -1;
    }
    mNodes = new ChorobonNode*[mCount];
    for (s32 i = 0; i < mCount; ++i) {
        ChorobonNode* pNode;
        if (i == itemIndex) {
            mItemNode = new ChorobonItem("チョロボンアイテム", &mSpeed);
            pNode = mItemNode;
        } else {
            pNode = new Chorobon("チョロボン", &mSpeed);
        }
        mNodes[i] = pNode;
        al::initCreateActorWithPlacementInfo(mNodes[i], rInfo);
    }
    f32 coord = calcNearestCoordOfPoint(mNodes[0]);
    for (s32 i = 0; i < mCount; ++i) {
        al::setRailPosToCoord(mNodes[i], coord);
        f32 offset = (mCount - (i + 1)) * 100.0f;
        if (itemIndex != -1) {
            if (i < itemIndex)
                offset += 200.0f;
            else if (i == itemIndex)
                offset += 100.0f;
        }
        mNodes[i]->setRailCoord(coord + offset);
    }
    al::initNerve(this, &NrvChorobonHolderWatch, 0);
    makeActorAppeared();
}

/** @brief Finds the closest rail point. @param pActor Rail actor. @return Coordinate of that point. */
f32 ChorobonHolder::calcNearestCoordOfPoint(const al::LiveActor* pActor) {
    sead::Vector3f point(0.0f, 0.0f, 0.0f);
    al::calcRailPointPos(&point, pActor, 0);
    f32 distance = (al::getTrans(pActor) - point).length();
    s32 nearest = 0;
    for (s32 i = 1; i < al::getRailPointNum(pActor); ++i) {
        al::calcRailPointPos(&point, pActor, i);
        if ((al::getTrans(pActor) - point).length() < distance) {
            distance = (al::getTrans(pActor) - point).length();
            nearest = i;
        }
    }
    return al::calcRailCoordByPoint(pActor, nearest);
}

/** @brief Synchronizes item clipping and reverses the train at either rail end. */
void ChorobonHolder::exeWatch() {
    if (mItemNode && al::isClipped(mItemNode) != al::isClipped(mItemNode->getItem())) {
        if (al::isClipped(mItemNode))
            al::validateClipping(mItemNode->getItem());
        else
            al::invalidateClipping(mItemNode->getItem());
    }
    auto* pStart = tryGetNodeStart();
    auto* pEnd = tryGetNodeEnd();
    if (!pStart || !pEnd) {
        kill();
        return;
    }
    if (al::isRailReachedGoal(pStart) || al::isRailReachedGoal(pEnd))
        for (s32 i = 0; i < mCount; ++i)
            if (al::isAlive(mNodes[i]))
                al::reverseRail(mNodes[i]);
}

/** @brief Finds the first surviving Fuzzy. @return First living node, or null. */
ChorobonNode* ChorobonHolder::tryGetNodeStart() {
    for (s32 i = 0; i < mCount; ++i)
        if (al::isAlive(mNodes[i]))
            return mNodes[i];
    return nullptr;
}

/** @brief Finds the last surviving Fuzzy. @return Last living node, or null. */
ChorobonNode* ChorobonHolder::tryGetNodeEnd() {
    for (s32 i = mCount - 1; i >= 0; --i)
        if (al::isAlive(mNodes[i]))
            return mNodes[i];
    return nullptr;
}
