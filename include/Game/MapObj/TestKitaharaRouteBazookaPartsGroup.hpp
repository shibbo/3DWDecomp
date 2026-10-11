#pragma once
namespace al { class BlockRailParts; struct ActorInitInfo; }
class TestKitaharaRouteBazookaPartsGroup {
public:
    TestKitaharaRouteBazookaPartsGroup();
    void init(const al::ActorInitInfo&);
    void active();
    void deactive();
    al::BlockRailParts* getParts(int) const;
    int getPartCount() const { return mPartCount; }
private:
    al::BlockRailParts** mParts = nullptr;
    int mPartCount = 0;
};
