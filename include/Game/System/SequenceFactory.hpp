#pragma once

namespace al {
class Sequence;
} // namespace al

/**
 * @brief Creates game sequences by class name.
 */
class SequenceFactory {
  public:
    static al::Sequence* createSequence(const char* pName);
};
