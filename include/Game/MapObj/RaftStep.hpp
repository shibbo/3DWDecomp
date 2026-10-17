#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class RaftConveyer;
class RaftKeyKeeper;
class RaftStep : public al::LiveActor {
public:
    RaftStep(const char*);
    void forceSetCurrentCoord(float);
    void update();
    void setConveyer(RaftConveyer* pConveyer) { mConveyer = pConveyer; }
    void setKeyKeeper(RaftKeyKeeper* pKeeper) { mKeyKeeper = pKeeper; }
    void setEndCoord(float coord) { mEndCoord = coord; }
    float getCurrentCoord() const { return mCurrentCoord; }
private:
    RaftConveyer* mConveyer;
    u8 mUnreconstructed150[0x180 - 0x150];
    RaftKeyKeeper* mKeyKeeper;
    u8 mUnreconstructed188[0x194 - 0x188];
    float mCurrentCoord;
    float mEndCoord;
    u8 mUnreconstructed19c[0x1f8 - 0x19c];
};
