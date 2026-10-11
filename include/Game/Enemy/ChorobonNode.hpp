#pragma once

#include "Library/LiveActor/LiveActor.hpp"

class ChorobonNode : public al::LiveActor {
public:
    ChorobonNode(const char* pName, const float* pParam);
    /** @brief Releases a Fuzzy rail node. */
    ~ChorobonNode() override = default;
    void setRailCoord(float coord);

private:
    const float* mParam;
};
