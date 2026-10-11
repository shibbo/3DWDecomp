#pragma once
namespace al { class BlockRailParts; struct ActorInitInfo; }
class TestKitaharaRouteBazookaPartsGroup;
class TestKitaharaRouteBazookaEntrance;
class TestKitaharaRouteBazooka;
class TestKitaharaRouteBazookaEntranceGroup {
public:
    TestKitaharaRouteBazookaEntranceGroup(const char*);
    void init(TestKitaharaRouteBazookaPartsGroup*, const al::ActorInitInfo&);
    void addEntrance(al::BlockRailParts*, TestKitaharaRouteBazookaEntrance*);
    void active();
    void deactive();
    TestKitaharaRouteBazookaEntrance* getEntrance(int) const;
    int getEntranceCount() const { return mEntranceCount; }
    void setHost(TestKitaharaRouteBazooka*);
private:
    TestKitaharaRouteBazookaEntrance** mEntrances = nullptr;
    int mEntranceCount = 0;
    int mEntranceCapacity = 0;
    const char* mModelName;
};
