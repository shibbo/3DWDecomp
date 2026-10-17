#pragma once

class DemoScenePlayer;

/** @brief Supplies the active characters and power-up forms for a cutscene. */
class IDemoScenePlayerInfo {
public:
    virtual void setup(DemoScenePlayer* pPlayer) = 0;
    virtual int getActivePlayerNum() const = 0;
    virtual int getCharacterType(int index) const = 0;
    virtual int getFigureType(int index) const = 0;
};
