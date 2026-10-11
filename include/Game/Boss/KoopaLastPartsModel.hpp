#pragma once
#include "Library/LiveActor/LiveActor.hpp"

class KoopaLastPartsModel : public al::LiveActor {
public:
    explicit KoopaLastPartsModel(const char* pName);
    ~KoopaLastPartsModel() override;
    void initPartsMtx(al::LiveActor* pHost, const al::ActorInitInfo& rInfo,
                      const char* pArchiveName, const sead::Matrix34f* pMtx, const char* pSuffix);
    void makeActorAppeared() override;
    void updatePose();
    void control() override;

private:
    al::LiveActor* mHost = nullptr;
    sead::Matrix34f mMatrix;
    const sead::Matrix34f* mHostMtx = nullptr;
    bool mIsUseMatrix = false;
    bool mIsHidden = false;
};
static_assert(sizeof(KoopaLastPartsModel) == 0x190);
