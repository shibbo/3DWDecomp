#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al { class LayoutActor; class LayoutInitInfo; class NetworkSystem; class ErrorViewer; class FunctorBase; }
namespace sead { class Heap; }
class MiiverseBalloonParts;
class MiiverseStateDetail;
/** @brief Manages the rotating set of Miiverse post balloons. */
class MiiverseStateTimeLine : public al::NerveExecutor {
public:
    MiiverseStateTimeLine(al::LayoutActor* pParent, const al::LayoutInitInfo& rInfo, al::NetworkSystem* pNetwork, al::ErrorViewer* pErrorViewer);
    ~MiiverseStateTimeLine() override = default;
    void decompress();
    void appear(const char* pTag);
    void update();
    bool isWait() const;
    void exeWait();
    void exeDetail();
private:
    int mPostCount = 0;
    int mPage = 0;
    int mBalloonCount = 0;
    MiiverseBalloonParts* mBalloons[6];
    MiiverseStateDetail* mDetail = nullptr;
    sead::Heap* mHeap = nullptr;
    al::FunctorBase* mDecompressFunctor = nullptr;
    al::LayoutActor* mParent;
};
static_assert(sizeof(MiiverseStateTimeLine) == 0x70);
