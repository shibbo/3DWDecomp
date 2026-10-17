#pragma once

#include <attributes.h>
#include <nn/atk/atk_Global.h>

namespace nn::atk::detail::Util {
/** @brief Double-buffered collector of warnings printed once per sound archive player update. */
class WarningLogger {
public:
    /** @brief Creates a logger whose current buffer is the first one. */
    WarningLogger() : m_pCurrentBuffer(&m_Buffers[0]) {}

    void SwapBuffer();
    void Print();

private:
    struct LogBuffer {
        /** @brief Creates an empty log buffer. */
        LogBuffer() : m_Count(0) {}

        u8 m_Entries[0x300];
        int m_Count;
    };

    LogBuffer m_Buffers[2];
    LogBuffer* m_pCurrentBuffer;
};
static_assert(sizeof(WarningLogger) == 0x610, "WarningLogger size");

/**
 * @brief Gets the process-wide warning logger, creating it on first use.
 * @return The warning logger instance.
 */
template <>
inline NOINLINE WarningLogger& Singleton<WarningLogger>::GetInstance() {
    static WarningLogger instance;
    return instance;
}
}  // namespace nn::atk::detail::Util
