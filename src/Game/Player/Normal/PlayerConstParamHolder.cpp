#include "Player/Normal/PlayerConstParamHolder.hpp"

/**
 * Creates a holder with room for a number of parameter sets, all empty.
 * @param paramNum The number of parameter sets the holder can hold.
 */
PlayerConstParamHolder::PlayerConstParamHolder(s32 paramNum) : PlayerConstParam() {
    mCurrentIndex = 0;
    mParams = new const PlayerConstParam*[paramNum];

    for (s32 i = 0; i < paramNum; i++) {
        mParams[i] = nullptr;
    }
}

/**
 * Defines the getter of one tuning value: it asks the override parameter set while one is
 * active and returns this set's own value otherwise.
 * @return The tuning value.
 */
#define PLAYER_CONST_PARAM_DEFINE_GETTER(Type, Name)                                         \
    Type PlayerConstParam::get##Name() const {                                               \
        if (mIsOverride) {                                                                   \
            return mOverrideParam->get##Name();                                              \
        }                                                                                    \
                                                                                             \
        return m##Name;                                                                      \
    }

PLAYER_CONST_PARAM_LIST(PLAYER_CONST_PARAM_DEFINE_GETTER)

#undef PLAYER_CONST_PARAM_DEFINE_GETTER

/**
 * Defines the holder's getter of one tuning value: it answers from the current parameter set.
 * @return The current parameter set's tuning value.
 */
#define PLAYER_CONST_PARAM_HOLDER_DEFINE_GETTER(Type, Name)                                  \
    Type PlayerConstParamHolder::get##Name() const {                                         \
        return getCurrentParam()->get##Name();                                               \
    }

PLAYER_CONST_PARAM_LIST(PLAYER_CONST_PARAM_HOLDER_DEFINE_GETTER)

#undef PLAYER_CONST_PARAM_HOLDER_DEFINE_GETTER
