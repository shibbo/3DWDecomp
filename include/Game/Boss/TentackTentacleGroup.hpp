#pragma once
#include <basis/seadTypes.h>
#include "Library/Nerve/IUseNerve.hpp"
#include <container/seadPtrArray.h>
#include <math/seadVector.h>

class TentackHead;
class TentackTentacle;

/** @brief Set of tentacles that attack together under one head. */
class TentackTentacleGroup : public sead::PtrArray<TentackTentacle>, public al::IUseNerve {
public:
    explicit TentackTentacleGroup(s32 tentacleNumMax);
    void update();
    al::NerveKeeper* getNerveKeeper() const override;
    bool isDeactive() const;
    void resetAttackTentacle();
    void exeDeactive();
    void exeWait();
    void exeEndRequest();
    void exeEnd();
    void reset();
    void registerTentacle(TentackTentacle* pTentacle);
    void appearAll();
    bool requestEndSwingAll();
    void eatAttachItemAll();
    TentackHead* getHead() const;
    bool isEnableEndSwing() const;
    void endSwingAllForce();

    /** @brief Checks whether no more tentacles can be registered. @return True if full. */
    bool isFull() const { return capacity() <= size(); }

private:
    al::NerveKeeper* mNerveKeeper;

public:
    sead::Vector3f mTargetPos;  // 0x20 position the owning head turns towards

private:

};
static_assert(sizeof(TentackTentacleGroup) == 0x30);
