#pragma once
#include <math/seadVector.h>
namespace al { class BlockRailParts; class BlockRailPartsGroup; struct ActorInitInfo; }
class RouteDokanEntrance;
class IUseRouteDokan;
class RouteDokanEntranceGroup {
public:
    RouteDokanEntranceGroup(const char*, const char*);
    void init(al::BlockRailPartsGroup*, const al::ActorInitInfo&, bool);
    void addEntrance(al::BlockRailParts*, RouteDokanEntrance*);
    void calcOffset(const sead::Vector3f&);
    void updateLinkedTrans(const sead::Vector3f&);
    void active();
    void deactive();
    RouteDokanEntrance* getEntrance(int) const;
    void setHost(IUseRouteDokan*);

    s32 getEntranceCount() const { return mEntranceCount; }
private:
    RouteDokanEntrance** mEntrances = nullptr;
    int mEntranceCount = 0;
    int mEntranceCapacity = 0;
    const char* mEntranceName;
    const char* mModelName;
};
