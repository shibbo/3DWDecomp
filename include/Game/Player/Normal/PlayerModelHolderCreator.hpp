#pragma once
#include <basis/seadTypes.h>
namespace al { class ActorInitInfo; class IUseHioNode; }
class IUsePlayerRetargettingInfoCreator;
class PlayerModelHolder;
namespace PlayerModelHolderCreator {
PlayerModelHolder* createModelHolderAll(const al::ActorInitInfo& rInfo, const char* pCharacterName,
                                       IUsePlayerRetargettingInfoCreator* pCreator,
                                       al::IUseHioNode* pHioNode, bool option, u32 flags,
                                       const char* pSuffix, int index);
}
