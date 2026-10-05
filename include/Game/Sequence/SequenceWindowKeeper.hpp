#pragma once

namespace al {
class LayoutInitInfo;
}
class WindowMessage;

class SequenceWindowKeeper {
public:
    explicit SequenceWindowKeeper(const al::LayoutInitInfo& rInfo);
    void appearMessage(const char* pMessageId, int padPort, bool isSequence);
    bool isMessageActive() const;

private:
    bool mIsSequence = false;
    WindowMessage* mpMessage = nullptr;
    WindowMessage* mpSequenceMessage = nullptr;
};

static_assert(sizeof(SequenceWindowKeeper) == 0x18);
