#pragma once
#include "Library/Nerve/NerveExecutor.hpp"
namespace al {
class Sequence;
struct GameSystemInfo;
class GamePadSystem;
class AudioSystem;
} // namespace al
class GameSystem : public al::NerveExecutor {
  public:
    GameSystem();
    virtual void init();
    void initAudio();
    void setStartupSequence();
    virtual void movement();
    void drawMain();
    void drawSub();
    void exePlay();
    bool tryChangeSequence(const char* pName);

    /**
     * @brief Access the game pad system owned by the game system.
     * @return The game pad system.
     */
    al::GamePadSystem* getGamePadSystem() const { return mpGamePad; }

    /**
     * @brief Access the running root sequence.
     * @return The root sequence.
     */
    al::Sequence* getSequence() const { return mpSequence; }

    /**
     * @brief Access the system info shared by the sequences.
     * @return The game system info.
     */
    al::GameSystemInfo* getGameSystemInfo() const { return mpInfo; }

  private:
    al::Sequence* mpSequence;
    al::GameSystemInfo* mpInfo;
    al::GamePadSystem* mpGamePad;
    al::AudioSystem* mpAudio;
    void* mpUnknown30;
    void* mpUnknown38;
    void* mpUnknown40;
};
namespace GameSystemFunction {
GameSystem* getGameSystem();
}
static_assert(sizeof(GameSystem) == 0x48);
