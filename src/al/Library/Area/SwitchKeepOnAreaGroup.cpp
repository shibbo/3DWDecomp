#include "Project/AreaObj/SwitchKeepOnAreaGroup.hpp"

#include "Library/Play/Placement/PlacementFunction.hpp"
#include "Library/StageSwitch/StageSwitchFunc.hpp"
#include "Project/AreaObj/AreaObj.hpp"

namespace al {
static bool isInAreaAny(const AreaObj* pAreaObj, const sead::Vector3f* pPositions, s32 num) {
    for (s32 i = 0; i < num; i++) {
        if (pAreaObj->isInVolume(pPositions[i])) {
            return true;
        }
    }

    return false;
}

static bool isInAreaAll(const AreaObj* pAreaObj, const sead::Vector3f* pPositions, s32 num) {
    if (num <= 0) {
        return false;
    }

    for (s32 i = 0; i < num; i++) {
        if (!pAreaObj->isInVolume(pPositions[i])) {
            return false;
        }
    }

    return true;
}

/**
 * Constructs a switch keep on group for an area group.
 * @param pGroup area group
 */
SwitchKeepOnAreaGroup::SwitchKeepOnAreaGroup(AreaObjGroup* pGroup) : mGroup(pGroup) {
    mCount = pGroup->getSize();

    if (mCount > 0) {
        mKeepOnAreas = new AreaObj*[mCount];
    }
}

/**
 * Keeps the switches of the areas containing the positions on and turns the others off.
 * @param pPositions positions to check
 * @param num number of positions
 * @param isDisasterMode whether disaster mode is active
 */
void SwitchKeepOnAreaGroup::update(const sead::Vector3f* pPositions, s32 num,
                                   bool isDisasterMode) {
    mKeepOnCount = 0;
    s32 numAreas = mGroup->getSize();

    for (s32 i = 0; i < numAreas; i++) {
        AreaObj* areaObj = mGroup->getAreaObj(i);

        bool isSame = false;

        for (s32 j = 0; j < mKeepOnCount; j++) {
            if (isSameStageSwitch(areaObj, mKeepOnAreas[j], "SwitchAreaOn")) {
                isSame = true;
                break;
            }
        }

        if (isSame) {
            continue;
        }

        s32 onCondition = 0;
        tryGetArg(&onCondition, areaObj->getPlacementInfo(), "OnCondition");
        bool isIn = false;
        if (num > 0) {
            if (onCondition == 0) {
                isIn = isInAreaAny(areaObj, pPositions, num);
            } else if (onCondition == 1) {
                isIn = isInAreaAll(areaObj, pPositions, num);
            }
        }

        if (isIn) {
            if (!areaObj->mIsDisasterCameraOn || isDisasterMode) {
                tryOnStageSwitch(areaObj, "SwitchAreaOn");
                mKeepOnAreas[mKeepOnCount] = areaObj;
                mKeepOnCount++;
            }
        } else {
            tryOffStageSwitch(areaObj, "SwitchAreaOn");
        }
    }
}

/**
 * Keeps the switches of the areas containing a position on and turns the others off.
 * @param rPos position to check
 */
void SwitchKeepOnAreaGroup::update(const sead::Vector3f& rPos) {
    sead::Vector3f pos = rPos;
    update(&pos, 1, false);
}
}  // namespace al
