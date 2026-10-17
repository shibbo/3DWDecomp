#pragma once

#include "Project/AreaObj/AreaObj.hpp"

class InkArea : public al::AreaObj {
public:
    InkArea(const char* pName);
    void init(const al::AreaInitInfo& rInfo) override;
    void init(const al::AreaInitInfo& rInfo, const al::SceneObjHolder* pHolder) override;

    const char* mTextureName = nullptr;
    const char* mForceInkName = nullptr;
    bool mIsDisableShaderInk = false;
};
