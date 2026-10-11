#pragma once

#include <container/seadPtrArray.h>

#include "Library/Scene/ISceneObj.hpp"

class CatGull;

/**
 * @brief Scene object listing every seagull flock of the scene.
 * @note Only what reconstructed code needs is declared so far.
 */
class CatGullHolder : public al::ISceneObj {
public:
    CatGullHolder();

    /**
     * @brief Get the registered seagull flocks.
     * @return The flock list.
     */
    const sead::PtrArray<CatGull>& getCatGulls() const { return mCatGulls; }

private:
    sead::PtrArray<CatGull> mCatGulls;  // 0x8
};
