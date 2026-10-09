#pragma once

class BrosMoveStepKeeper;

namespace rc {
bool tryRequestSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper);
bool tryRequestEndSupportFreezeSyncCurrentStepKuribo(const BrosMoveStepKeeper* pMoveStepKeeper);
}  // namespace rc
