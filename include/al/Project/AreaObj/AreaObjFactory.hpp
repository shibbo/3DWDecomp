#pragma once

#include "Library/Factory/Factory.hpp"

namespace al {
class AreaObj;

using AreaCreatorFunction = AreaObj* (*)(const char*);

/**
 * @brief Creates an area of the requested concrete type.
 * @param pName Name assigned to the new area.
 * @return Newly allocated area.
 */
template <typename T>
AreaObj* createAreaObjFunction(const char* pName) {
    return new T(pName);
}

class AreaObjFactory : public Factory<AreaCreatorFunction> {
public:
    AreaObjFactory(const char* pName);
};
}  // namespace al
