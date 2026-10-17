#pragma once

namespace nn::atk::detail {
/**
 * @brief Interpolate a value over a counted transition.
 * @tparam ValueType Arithmetic type of the interpolated value.
 * @tparam CountType Arithmetic type of elapsed and total frame counts.
 */
template <typename ValueType, typename CountType>
class MoveValue {
  public:
    /**
     * @brief Jump to a value with no transition.
     * @param value Value used as both origin and target.
     */
    void InitValue(ValueType value) {
        m_Origin = value;
        m_Target = value;
        m_Frame = 0;
        m_Counter = 0;
    }

    /**
     * @brief Start a transition from the current value.
     * @param target Value reached at the end of the transition.
     * @param frames Transition length in frames; zero finishes immediately.
     */
    void SetTarget(ValueType target, CountType frames) {
        m_Origin = GetValue();
        m_Target = target;
        m_Frame = frames;
        m_Counter = 0;
    }

    /**
     * @brief Advance the transition, saturating at its end.
     * @param frames Number of elapsed frames.
     */
    void Update(CountType frames) {
        if (m_Counter < m_Frame) {
            m_Counter += frames;
            if (m_Counter > m_Frame) {
                m_Counter = m_Frame;
            }
        }
    }

    /** @brief Advance the transition by one frame, stopping at its end. */
    void Update() {
        if (m_Counter < m_Frame) {
            m_Counter++;
        }
    }

    /** @brief Test transition completion. @return Whether elapsed frames reached the duration. */
    bool IsFinished() const { return m_Counter >= m_Frame; }

    /** @brief Evaluate the transition at its current frame. @return Current value, or the target after
     * completion. */
    ValueType GetValue() const {
        if (IsFinished()) {
            return m_Target;
        }
        return static_cast<ValueType>(m_Origin + (m_Target - m_Origin) * m_Counter / m_Frame);
    }

    /** @brief Gets the value reached at the end of the transition. @return Target value. */
    ValueType GetTargetValue() const { return m_Target; }

    /** @brief Gets the frames left in the transition. @return Remaining frames, or 0 when finished. */
    CountType GetRemainingCount() const {
        if (m_Counter < m_Frame) {
            return m_Frame - m_Counter;
        }
        return 0;
    }

  private:
    ValueType m_Origin;
    ValueType m_Target;
    CountType m_Frame;
    CountType m_Counter;
};
}  // namespace nn::atk::detail
