#pragma once

#include <basis/seadTypes.h>

namespace al {
class SePlayParamList;

class ISeModifier {
public:
    // Names and parameter types from PlayerAudio's overrides (PlayerAudio::modifyId, ...).
    virtual u32 modifyId(u32 id) = 0;
    virtual void modifyParams(s32 id, SePlayParamList* pParamList) = 0;
    virtual void modifyHoldParams(s32 id, SePlayParamList* pParamList) = 0;

    /** @brief Lets the modifier replace a sound id. @param id Sound id. @return Id to play. */
    u32 modifySoundId(u32 id) { return modifyId(id); }

    /** @brief Lets the modifier change start parameters. @param id Sound id. @param pParamList Parameters. */
    void modifyStartParam(u32 id, SePlayParamList* pParamList) { modifyParams(id, pParamList); }

    /** @brief Lets the modifier change hold parameters. @param id Sound id. @param pParamList Parameters. */
    void modifyHoldParam(u32 id, SePlayParamList* pParamList) { modifyHoldParams(id, pParamList); }
};
}  // namespace al
