#pragma once

#include "Library/Nerve/NerveExecutor.hpp"
#include "Project/Audio/IUseAudioKeeper.hpp"

namespace al {
class AudioDirector;
}

class MicInputSePlayer : public al::NerveExecutor, public al::IUseAudioKeeper {
public:
    explicit MicInputSePlayer(al::AudioDirector* pDirector);
    void update();
    void exeWait();
    void exeBlow();
    al::AudioKeeper* getAudioKeeper() const override;

private:
    al::AudioKeeper* mAudioKeeper = nullptr;
};

static_assert(sizeof(MicInputSePlayer) == 0x20);
