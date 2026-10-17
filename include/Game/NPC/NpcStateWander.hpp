#pragma once
#include "NPC/NpcStateWanderParam.hpp"
#include "Library/Nerve/NerveStateBase.hpp"
#include <math/seadVector.h>
class NpcStateParam;
class NpcStateWanderParam;
class NpcTerritoryMover;
class NpcStateWander : public al::ActorStateBase {
public:
    NpcStateWander(al::LiveActor*, sead::Vector3f*, const NpcStateParam*,
                   const NpcStateWanderParam*);
    ~NpcStateWander() override;
    void appear() override;
    void setWanderCenter(const sead::Vector3f&);
    sead::Vector3f getWanderCenter() const;
    sead::Vector3f getTargetPos() const;
    void exeWait();
    void exeWalk();
    bool isWait();

private:
    struct Durations {
        /** @brief Stores phase durations. @param w Wait frames. @param m Walk frames. */
        Durations(int w, int m) : wait(w), walk(m) {}
        int wait;
        int walk;
    };
    sead::Vector3f* mDirection;
    NpcTerritoryMover* mMover;
    const NpcStateParam* mMoveParam;
    const NpcStateWanderParam* mParam;
    Durations* mDurations;
};
