#include "AreaObj/PlayerCollisionYPlaneArea.hpp"
#include "Player/Normal/ResultCheckArrow.hpp"
#include "Project/Collision/CollisionPartsFilterBase.hpp"
#include "Project/Collision/CollisionUtil.hpp"
#include "Project/Collision/HitDb.hpp"

/**
 * @brief Constructs an area with storage for sixteen collision hits.
 * @param pName Area name.
 */
PlayerCollisionYPlaneArea::PlayerCollisionYPlaneArea(const char* pName) : al::AreaObj(pName) {
    mResultCheckArrow = new ResultCheckArrow(16);
}

/**
 * @brief Initializes the area from placement information.
 * @param rInfo Area placement and switch information.
 */
void PlayerCollisionYPlaneArea::init(const al::AreaInitInfo& rInfo) {
    al::AreaObj::init(rInfo);
}

/**
 * @brief Casts a vertical collision ray once and returns its cached results.
 * @param pCollision Collision system used for the initial query.
 * @return Cached arrow collision results.
 */
ResultCheckArrow* PlayerCollisionYPlaneArea::getResultCheckArrow(const al::IUseCollision* pCollision) {
    if (!mIsCollisionChecked) {
        sead::Vector3f pos;
        mShape->calcTrans(&pos);
        pos.y += 1500.0f;
        sead::Vector3f direction(0.0f, -3000.0f, 0.0f);
        al::CollisionPartsFilterSpecialPurpose filter("PlayerOnly");
        u32 count = alCollisionUtil::checkStrikeArrow(pCollision, pos, direction, &filter, nullptr);
        for (u32 i = 0; i < count; ++i) {
            const al::ArrowHitInfo* pHit = alCollisionUtil::getStrikeArrowInfo(pCollision, i);
            mResultCheckArrow->append(pHit->mPos, *pHit->mTriangle.getFaceNormal(),
                                     pHit->mTriangle.mCollisionParts, pHit->mTriangle);
        }
        mIsCollisionChecked = true;
    }
    return mResultCheckArrow;
}
