#pragma once

#include <container/seadPtrArray.h>

#include "Library/Scene/ISceneObj.hpp"

class CatGull;

/**
 * @brief Scene object listing every seagull flock of the scene.
 */
class CatGullHolder : public al::ISceneObj {
public:
    CatGullHolder();
    ~CatGullHolder();

    /**
     * @brief Get the registered seagull flocks.
     * @return The flock list.
     */
    const sead::PtrArray<CatGull>& getCatGulls() const { return mCatGulls; }
    void registerCatGull(CatGull* gull) { if (!mCatGulls.isFull()) mCatGulls.pushBack(gull); }

private:
    sead::PtrArray<CatGull> mCatGulls;  // 0x8
};
