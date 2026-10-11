#pragma once
#include "Library/LiveActor/LiveActor.hpp"
namespace al {
class PoseHistoryPath;
}
class GhostPresentBox : public al::LiveActor {
public:
    GhostPresentBox(const char*, int);
    void start();
    void stop(bool isKill);
    void setPoseHistoryPath(const al::PoseHistoryPath* pPath);
private:
    unsigned char _144[0x160 - 0x144];
};
