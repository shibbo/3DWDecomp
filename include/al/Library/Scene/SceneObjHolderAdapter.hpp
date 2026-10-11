#pragma once

#include "Library/Scene/IUseSceneObjHolder.hpp"

namespace al {
/** @brief Exposes a scene object holder through the common user interface. */
class SceneObjHolderAdapter : public IUseSceneObjHolder {
public:
    /** @brief Wraps a holder. @param pHolder Scene object holder to expose. */
    explicit SceneObjHolderAdapter(SceneObjHolder* pHolder) : mHolder(pHolder) {}
    /** @brief Gets the wrapped holder. @return The supplied scene object holder. */
    SceneObjHolder* getSceneObjHolder() const override { return mHolder; }

private:
    SceneObjHolder* mHolder;
};
}
