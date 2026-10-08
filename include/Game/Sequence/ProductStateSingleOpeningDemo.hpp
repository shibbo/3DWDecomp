#pragma once

#include <basis/seadTypes.h>
#include "Library/Nerve/NerveStateBase.hpp"

class ControllerConnectChecker;
class ProductSequence;
class StageWipeKeeper;

namespace al {
struct SequenceInitInfo;
}  // namespace al

/**
 * @brief Sequence state of the Bowser's Fury opening demo.
 * @note Minimal declaration: only what the reconstructed code uses is declared so far.
 */
class ProductStateSingleOpeningDemo : public al::HostStateBase<ProductSequence> {
public:
    ProductStateSingleOpeningDemo(ProductSequence* pSequence, StageWipeKeeper* pWipeKeeper,
                                  const al::SequenceInitInfo& rInfo,
                                  ControllerConnectChecker* pConnectChecker);

private:
    u8 _20[0x30];  ///< Not reconstructed yet.
};

static_assert(sizeof(ProductStateSingleOpeningDemo) == 0x50);
