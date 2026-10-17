#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ChainModel : public al::LiveActor {
public:
    ChainModel(const char* pName, const char* pArchiveName, const char* pSuffix);
    ~ChainModel() override;
    void init(const al::ActorInitInfo& rInfo) override;
    void exeWait();
    void exeFall();
    bool isFallEnd() const;
    void startFall(const sead::Vector3f& rVelocity);

private:
    const char* mArchiveName;
    const char* mSuffix;
    bool mHasFallAction = false;
};
