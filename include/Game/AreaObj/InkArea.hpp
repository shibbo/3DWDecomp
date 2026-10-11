#pragma once

#include "Project/AreaObj/AreaObj.hpp"

/**
 * @brief Area covered with ink, described by an ink lookup texture.
 */
class InkArea : public al::AreaObj {
public:
    explicit InkArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

    /**
     * @brief Get the name of the ink lookup texture.
     * @return The texture name.
     */
    const char* getTextureName() const { return mTextureName; }

    /**
     * @brief Get the name of the water data file forced for this area.
     * @return The data name, or nullptr to use the one of the current phase.
     */
    const char* getForceInkName() const { return mForceInkName; }

    /**
     * @brief Check whether the ink is left out of the water shader.
     * @return True when the shader ink is disabled.
     */
    bool isDisableShaderInk() const { return mIsDisableShaderInk; }

private:
    const char* mTextureName = nullptr;
    const char* mForceInkName = nullptr;
    bool mIsDisableShaderInk = false;
};
