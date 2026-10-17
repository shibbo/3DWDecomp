#pragma once

#include <basis/seadTypes.h>
#include <prim/seadSafeString.h>

class IUsePlayerRetargettingSelector;
class PlayerModel;

namespace PlayerAnimFunc {
/// Which retargetting info an animation is played with.
enum RetargettingType : s32 {
    cRetargettingType_Default,
    cRetargettingType_Chara,
    cRetargettingType_Figure,
    cRetargettingType_None,
};

RetargettingType convertToRegularName(sead::BufferedSafeString* pOut, PlayerModel* pModel,
                                      const sead::SafeString& rName);
bool tryConvertToReverseName(sead::BufferedSafeString* pName, PlayerModel* pModel);
void controlRetargetting(PlayerModel* pModel, IUsePlayerRetargettingSelector* pSelector,
                         s32 type);
}  // namespace PlayerAnimFunc
