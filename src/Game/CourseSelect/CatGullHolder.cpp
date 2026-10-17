#include "MapObj/CatGullHolder.hpp"

CatGullHolder::CatGullHolder() {
    mCatGulls.allocBuffer(100, nullptr);
}

CatGullHolder::~CatGullHolder() = default;
