#pragma once

#include <basis/seadTypes.h>

namespace al {
class LayoutInitInfo;
}  // namespace al

/**
 * @brief Keeps the message windows shown by the product sequence.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class SequenceWindowKeeper {
public:
    SequenceWindowKeeper(const al::LayoutInitInfo& rInfo);

private:
    u8 _0[0x18];  ///< Not reconstructed yet.
};

static_assert(sizeof(SequenceWindowKeeper) == 0x18);
