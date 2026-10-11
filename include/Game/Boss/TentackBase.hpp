#pragma once
#include <math/seadVector.h>
class TentackHead;
class TentackRockBase;
class TentackAttachItemHolder;

class TentackBase {
public:
    virtual void receiveDamage(const TentackHead* pHead, bool isLast) = 0;
    virtual TentackHead* getHead() const = 0;
    virtual TentackAttachItemHolder* getAttachItemHolder() const = 0;
    virtual int getLevel() const = 0;
    virtual const sead::Vector3f& getTentackTrans() const = 0;
    virtual TentackRockBase* tryGetDeadRock() const = 0;
    /**
     * @brief Gives a rock fall point back to the host's pool. Does nothing by default.
     * @param pPoint Fall point.
     */
    virtual void returnRockAppearPointPtr(const sead::Vector2f* pPoint) {}
    virtual bool tryFindTransNearPlayer(sead::Vector3f* pPosition) = 0;
};
