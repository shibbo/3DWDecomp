#include "Boss/PunpunParam.hpp"

namespace {
const sead::Vector3f sDividePos0[3][6] = {
    {
        {-625.0f, 0.0f, 0.0f},
        {-375.0f, 0.0f, 0.0f},
        {-125.0f, 0.0f, 0.0f},
        {125.0f, 0.0f, 0.0f},
        {375.0f, 0.0f, 0.0f},
        {625.0f, 0.0f, 0.0f},
    },
    {
        {-625.0f, 0.0f, -625.0f},
        {-375.0f, 0.0f, -375.0f},
        {-125.0f, 0.0f, -125.0f},
        {125.0f, 0.0f, 125.0f},
        {375.0f, 0.0f, 375.0f},
        {625.0f, 0.0f, 625.0f},
    },
    {
        {-625.0f, 0.0f, 625.0f},
        {-375.0f, 0.0f, 375.0f},
        {-125.0f, 0.0f, 125.0f},
        {125.0f, 0.0f, -125.0f},
        {375.0f, 0.0f, -375.0f},
        {625.0f, 0.0f, -625.0f},
    },
};

const sead::Vector3f sDividePos1[2][8] = {
    {
        {-625.0f, 0.0f, -562.5f},
        {-625.0f, 0.0f, -187.5f},
        {-625.0f, 0.0f, 187.5f},
        {-625.0f, 0.0f, 562.5f},
        {625.0f, 0.0f, -562.5f},
        {625.0f, 0.0f, -187.5f},
        {625.0f, 0.0f, 187.5f},
        {625.0f, 0.0f, 562.5f},
    },
    {
        {-600.0f, 0.0f, 625.0f},
        {-200.0f, 0.0f, 625.0f},
        {200.0f, 0.0f, 625.0f},
        {600.0f, 0.0f, 625.0f},
        {-600.0f, 0.0f, -625.0f},
        {-200.0f, 0.0f, -625.0f},
        {200.0f, 0.0f, -625.0f},
        {600.0f, 0.0f, -625.0f},
    },
};

const sead::Vector3f sDividePos2[2][9] = {
    {
        sead::Vector3f::zero,
        {-600.0f, 0.0f, 600.0f},
        {-300.0f, 0.0f, 300.0f},
        {300.0f, 0.0f, -300.0f},
        {600.0f, 0.0f, -600.0f},
        {-600.0f, 0.0f, -600.0f},
        {-300.0f, 0.0f, -300.0f},
        {300.0f, 0.0f, 300.0f},
        {600.0f, 0.0f, 600.0f},
    },
    {
        sead::Vector3f::zero,
        {-625.0f, 0.0f, 0.0f},
        {-312.5f, 0.0f, 0.0f},
        {312.5f, 0.0f, 0.0f},
        {625.0f, 0.0f, 0.0f},
        {0.0f, 0.0f, 700.0f},
        {0.0f, 0.0f, 350.0f},
        {0.0f, 0.0f, -350.0f},
        {0.0f, 0.0f, -700.0f},
    },
};

} // namespace

/**
 * @brief Returns the number of clone placement patterns for the damage phase.
 * @param damageCount Number of hits received.
 * @return Available pattern count, or zero for an unsupported phase.
 */
s32 PunpunParam::getDividePosPatternNum(s32 damageCount) {
    switch (damageCount) {
    case 0: return 3;
    case 1: return 2;
    case 2: return 2;
    default: return 0;
    }
}

/**
 * @brief Returns a clone offset from the selected placement pattern.
 * @param damageCount Number of hits received.
 * @param pattern Placement pattern index.
 * @param index Clone index within the pattern.
 * @return Clone offset, or the zero vector for an unsupported phase.
 */
const sead::Vector3f& PunpunParam::getDividePos(s32 damageCount, s32 pattern, s32 index) {
    switch (damageCount) {
    case 0: return sDividePos0[pattern][index];
    case 1: return sDividePos1[pattern][index];
    case 2: return sDividePos2[pattern][index];
    default: return sead::Vector3f::zero;
    }
}
