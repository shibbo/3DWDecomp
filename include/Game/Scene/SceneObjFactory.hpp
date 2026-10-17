#pragma once

namespace al {
class SceneObjHolder;
}  // namespace al

/**
 * Creates the scene object holder with the game's scene object table.
 */
class SceneObjFactory {
public:
    static al::SceneObjHolder* createSceneObjHolder();
};
