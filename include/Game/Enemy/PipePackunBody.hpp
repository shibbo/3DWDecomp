#pragma once

#include <basis/seadTypes.h>
#include <math/seadVector.h>

#include "Library/LiveActor/LiveActor.hpp"

class PipePackun;

/** @brief Stem of a Pipe Piranha Plant; positions the head along its curved body. */
class PipePackunBody : public al::LiveActor {
public:
    PipePackunBody(PipePackun* pHost, const char* pName);

    void init(const al::ActorInitInfo& rInfo) override;
    void kill() override;
    void killSwitch();
    void exeWait();
    void calcPosAndDir(sead::Vector3f* pPos, sead::Vector3f* pDir, f32 coord) const;
    void setCoord(f32 coord);
    f32 calcNearCoord(const sead::Vector3f& rPos) const;
    void calcClippingSphere(sead::Vector3f* pCenter, f32* pRadius, const sead::Vector3f& rHeadPos,
                            f32 headRadius) const;

    /** @brief Gets the total length of the stem, i.e. the largest coordinate the head can reach. */
    f32 getTotalLength() const { return mTotalLength; }

private:
    u8 _144[0x15c - 0x144];
    f32 mTotalLength;
};

static_assert(sizeof(PipePackunBody) == 0x160);
