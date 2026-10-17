#include "Enemy/ChorobonItem.hpp"
#include "Enemy/Bomb.hpp"
#include "MapObj/Coin.hpp"
#include "MapObj/KinokoSuper.hpp"
#include "MapObj/KinokoOneUp.hpp"
#include "MapObj/SuperBell.hpp"
#include "MapObj/FireFlower.hpp"
#include "MapObj/SuperLeaf.hpp"
#include "MapObj/SuperStar.hpp"
#include "MapObj/Ball.hpp"
#include "MapObj/GreenStar.hpp"
#include "Library/ActorUtil.hpp"
#include "Library/Nerve/NerveSetup.hpp"
#include "Library/Rail/RailUtil.hpp"
#include "Util/ItemUtil.hpp"

namespace {
NERVE_DECL(ChorobonItem, RailMove)
NERVES_MAKE_NOSTRUCT(ChorobonItem, RailMove)
}

/** @brief Creates a rail carrier for an item. @param pName Actor name. @param pSpeed Shared rail speed. */
ChorobonItem::ChorobonItem(const char* pName, const f32* pSpeed) : ChorobonNode(pName, pSpeed) {}

/** @brief Creates the selected item and initializes the rail. @param rInfo Placement information. */
void ChorobonItem::init(const al::ActorInitInfo& rInfo) {
    al::initActorSceneInfo(this, rInfo);
    al::initActorPoseTFSV(this);
    al::initActorSRT(this, rInfo);
    al::initActorClipping(this, rInfo);
    al::initGroupClipping(this, rInfo, 16);
    initRailKeeper(rInfo);
    al::initNerve(this, &NrvChorobonItemRailMove, 0);
    al::initExecutorWatchObj(this, rInfo);
    mItemType = rc::getItemType(rInfo);
    const char* pItemName = "ChorobonItem";
    al::tryGetStringArg(&pItemName, rInfo, "ItemType");
    switch (mItemType) {
    case rc::ItemType_Coin:
        mItem = new Coin(pItemName);
        mIsPhysicalItem = false;
        mItemOffsetY = 0.0f;
        break;
    default:
        mItemType = rc::ItemType_KinokoSuper;
    case rc::ItemType_KinokoSuper:
        mItem = new KinokoSuper(pItemName, nullptr);
        break;
    case rc::ItemType_KinokoOneUp:
        mItem = new KinokoOneUp(pItemName, nullptr);
        break;
    case rc::ItemType_SuperBell:
        mItem = new SuperBell(pItemName, nullptr);
        break;
    case rc::ItemType_FireFlower:
        mItem = new FireFlower(pItemName, nullptr);
        mIsPhysicalItem = false;
        break;
    case rc::ItemType_SuperLeaf:
        mItem = new SuperLeaf(pItemName, nullptr);
        mIsPhysicalItem = false;
        break;
    case rc::ItemType_SuperStar:
        mItem = new SuperStar(pItemName, nullptr);
        break;
    case rc::ItemType_Ball:
        mItem = new Ball(pItemName);
        break;
    case rc::ItemType_Bomb:
        mItem = new Bomb(pItemName, false);
        break;
    case rc::ItemType_GreenStar:
        mItem = new GreenStar(pItemName, nullptr, false);
        mIsPhysicalItem = false;
        mItemOffsetY = 0.0f;
        break;
    }
    al::initCreateActorWithPlacementInfo(mItem, rInfo);
    al::offCollide(mItem);
    al::setRailPosToStart(this);
    al::setRailClippingInfo(&mClippingCenter, this, 100.0f, 100.0f);
    makeActorAppeared();
}

/** @brief Places the item at the carrier after placement and resets movement history. */
void ChorobonItem::initAfterPlacement() {
    updateItemPosture();
    al::resetPosition(this, false);
}

/** @brief Positions the item using its vertical offset. */
void ChorobonItem::updateItemPosture() {
    al::setTrans(mItem, al::getTrans(this));
    f32 offset = mItemOffsetY;
    sead::Vector3f* pTrans = al::getTransPtr(mItem);
    pTrans->y = offset + pTrans->y;
}

/** @brief Moves along the rail while the item remains attached. */
void ChorobonItem::exeRailMove() {
    if (mItemType == rc::ItemType_Bomb && !static_cast<Bomb*>(mItem)->isAttach()) {
        kill();
        return;
    }
    al::moveSyncRail(this, *mParam);
    updateItemPosture();
}
