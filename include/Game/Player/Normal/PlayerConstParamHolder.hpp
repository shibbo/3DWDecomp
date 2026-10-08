#pragma once

#include "Player/Normal/PlayerConstParam.hpp"

/// Declares the override of one tuning value's getter.
#define PLAYER_CONST_PARAM_HOLDER_DECLARE_GETTER(Type, Name) Type get##Name() const override;

/// Holds several player parameter sets and answers every getter from the currently
/// selected one (or from that set's override while it is active).
class PlayerConstParamHolder : public PlayerConstParam {
public:
    PlayerConstParamHolder(s32 paramNum);

    PLAYER_CONST_PARAM_LIST(PLAYER_CONST_PARAM_HOLDER_DECLARE_GETTER)

    /// The parameter set the getters currently answer from.
    const PlayerConstParam* getCurrentParam() const {
        const PlayerConstParam* pParam = mParams[mCurrentIndex];
        return pParam->isOverride() ? pParam->getOverrideParam() : pParam;
    }

private:
    const PlayerConstParam** mParams;  // 0x918
    s32 mCurrentIndex;                 // 0x920
};

#undef PLAYER_CONST_PARAM_HOLDER_DECLARE_GETTER
