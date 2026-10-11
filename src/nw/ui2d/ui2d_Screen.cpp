#include <nn/ui2d/ui2d_Screen.h>

#include <attributes.h>
#include <cmath>
#include <cstring>

#include <arm_neon.h>

#include <nn/ui2d/ui2d_AnimatorEx.h>
#include <nn/ui2d/ui2d_ArcResourceMgr.h>
#include <nn/ui2d/ui2d_DefaultControlCreator.h>
#include <nn/ui2d/ui2d_DrawInfo.h>
#include <nn/ui2d/ui2d_DynamicCast.h>
#include <nn/ui2d/ui2d_LayoutEx.h>
#include <nn/ui2d/ui2d_MultiArcResourceAccessorEx.h>
#include <nn/ui2d/ui2d_NormalButton.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_ScreenManager.h>
#include <nn/ui2d/ui2d_StateMachine.h>
#include <nn/util/util_Arithmetic.h>

namespace nn::ui2d {
namespace detail {
void ClampValue(float& rValue, float minimum, float maximum);
}  // namespace detail

namespace {
/** @brief System data that attaches a state machine to the root pane of a layout. */
struct StateMachineSystemData {
    PaneSystemDataType type;
    StateMachine* pStateMachine;
};

/**
 * @brief Get the state machine attached to a pane.
 * @param pPane Pane whose system data is searched.
 * @return State machine, or nullptr when the pane has none.
 */
inline StateMachine* GetStateMachine(const Pane* pPane) {
    const auto* pData = static_cast<const StateMachineSystemData*>(
        pPane->GetSystemExtDataByType(PaneSystemDataType_StateMachine));
    return pData != nullptr ? pData->pStateMachine : nullptr;
}

/**
 * @brief Get the handler that a state machine reports its changes to.
 * @param pStateMachine State machine.
 * @return Event handler of the parent state machine, or nullptr.
 */
inline StateMachineEventHandler* GetParentEventHandler(const StateMachine* pStateMachine) {
    return reinterpret_cast<StateMachineEventHandler*>(pStateMachine->m_pListener);
}

/**
 * @brief Classify the state a state layer reached.
 * @param pStateName Name of the state.
 * @return Kind of the user interface event.
 */
inline int GetUiEventKind(const char* pStateName) {
    if (std::strcmp(pStateName, "Decide") == 0) {
        return 1;
    }

    if (std::strcmp(pStateName, "Cancel") == 0) {
        return 1;
    }

    if (std::strcmp(pStateName, "In") == 0) {
        return 3;
    }

    return std::strcmp(pStateName, "Out") == 0 ? 4 : 0;
}

/**
 * @brief Make a vector with a zero w component.
 * @param x X component.
 * @param y Y component.
 * @param z Z component.
 * @return Vector.
 */
inline float32x4_t MakeVector(float x, float y, float z) {
    float32x4_t vector = vdupq_n_f32(0.0f);
    vector = vsetq_lane_f32(x, vector, 0);
    vector = vsetq_lane_f32(y, vector, 1);
    vector = vsetq_lane_f32(z, vector, 2);
    return vector;
}

/**
 * @brief Rotate the components of a vector to (y, z, x).
 * @param vector Vector.
 * @return Rotated vector.
 */
inline float32x4_t ShuffleYzx(float32x4_t vector) {
    const uint8x8_t indexYz = {4, 5, 6, 7, 8, 9, 10, 11};
    const uint8x8_t indexXw = {0, 1, 2, 3, 12, 13, 14, 15};
    uint8x8x2_t table = {
        {vreinterpret_u8_f32(vget_low_f32(vector)), vreinterpret_u8_f32(vget_high_f32(vector))}};
    return vreinterpretq_f32_u8(vcombine_u8(vtbl2_u8(table, indexYz), vtbl2_u8(table, indexXw)));
}

/**
 * @brief Rotate the components of a vector to (z, x, y).
 * @param vector Vector.
 * @return Rotated vector.
 */
inline float32x4_t ShuffleZxy(float32x4_t vector) {
    const uint8x8_t indexZx = {8, 9, 10, 11, 0, 1, 2, 3};
    const uint8x8_t indexYw = {4, 5, 6, 7, 12, 13, 14, 15};
    uint8x8x2_t table = {
        {vreinterpret_u8_f32(vget_low_f32(vector)), vreinterpret_u8_f32(vget_high_f32(vector))}};
    return vreinterpretq_f32_u8(vcombine_u8(vtbl2_u8(table, indexZx), vtbl2_u8(table, indexYw)));
}

/**
 * @brief Calculate the cross product of two vectors.
 * @param lhs First vector.
 * @param rhs Second vector.
 * @return Cross product.
 */
inline float32x4_t VectorCross(float32x4_t lhs, float32x4_t rhs) {
    return vfmsq_f32(vmulq_f32(ShuffleYzx(lhs), ShuffleZxy(rhs)), ShuffleYzx(rhs), ShuffleZxy(lhs));
}

/**
 * @brief Calculate the dot product of two vectors.
 * @param lhs First vector.
 * @param rhs Second vector.
 * @return Dot product in every lane.
 */
inline float32x4_t VectorDot(float32x4_t lhs, float32x4_t rhs) {
    float32x4_t product = vmulq_f32(lhs, rhs);
    float32x2_t sum = vadd_f32(vget_high_f32(product), vget_low_f32(product));
    sum = vpadd_f32(sum, sum);
    return vcombine_f32(sum, sum);
}

/**
 * @brief Scale a vector to unit length; a zero vector stays zero.
 * @param vector Vector.
 * @return Normalized vector.
 */
inline float32x4_t VectorNormalize(float32x4_t vector) {
    float32x4_t lengthSquared = VectorDot(vector, vector);
    float32x4_t estimate = vrsqrteq_f32(lengthSquared);
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(estimate, lengthSquared)));
    estimate = vmulq_f32(estimate, vrsqrtsq_f32(estimate, vmulq_f32(lengthSquared, estimate)));
    uint32x4_t mask = vmvnq_u32(vceqzq_f32(lengthSquared));
    return vreinterpretq_f32_u32(
        vandq_u32(vreinterpretq_u32_f32(vmulq_f32(vector, estimate)), mask));
}

/**
 * @brief Calculate the tangent of an angle with the polynomial approximations of sine and cosine.
 * @param radian Angle.
 * @return Tangent.
 */
inline float TanEst(float radian) {
    const float quotient = static_cast<float>(static_cast<int>(
        radian * nn::util::detail::Float1Divided2Pi + (radian >= 0.0f ? 0.5f : -0.5f)));
    const float reduced = radian - nn::util::detail::Float2Pi * quotient;
    float x;
    float cosSign;
    if (reduced > nn::util::detail::FloatPiDivided2) {
        x = nn::util::detail::FloatPi - reduced;
        cosSign = -1.0f;
    } else if (reduced < -nn::util::detail::FloatPiDivided2) {
        x = -nn::util::detail::FloatPi - reduced;
        cosSign = -1.0f;
    } else {
        x = reduced;
        cosSign = 1.0f;
    }

    const float* pSin = nn::util::detail::SinCoefficients;
    const float* pCos = nn::util::detail::CosCoefficients;
    const float x2 = x * x;
    const float sin =
        x * (x2 * (pSin[3] + x2 * ((x2 * (pSin[1] - pSin[0] * x2)) - pSin[2])) - pSin[4] + 1.0f);
    const float cos = cosSign * (x2 * (pCos[3] + x2 * ((x2 * (pCos[1] - x2 * pCos[0])) - pCos[2])) -
                                 pCos[4] + 1.0f);
    return sin / cos;
}

/**
 * @brief Make a right-handed perspective projection matrix.
 * @param pOut Projection matrix.
 * @param fovy Vertical field of view.
 * @param aspect Width divided by height.
 * @param nearZ Distance to the near plane.
 * @param farZ Distance to the far plane.
 */
inline void MatrixPerspectiveFieldOfViewRightHanded(nn::util::MatrixT4x4fType* pOut, float fovy,
                                                    float aspect, float nearZ, float farZ) {
    const float cot = 1.0f / TanEst(fovy * 0.5f);
    const float inverseDepth = -1.0f / (farZ - nearZ);
    pOut->_m.val[0] = MakeVector(cot / aspect, 0.0f, 0.0f);
    pOut->_m.val[1] = MakeVector(0.0f, cot, 0.0f);
    pOut->_m.val[2] =
        vsetq_lane_f32(nearZ * farZ * inverseDepth, MakeVector(0.0f, 0.0f, farZ * inverseDepth), 3);
    pOut->_m.val[3] = MakeVector(0.0f, 0.0f, -1.0f);
}

/**
 * @brief Make a right-handed orthographic projection matrix.
 * @param pOut Projection matrix.
 * @param left Left edge of the view volume.
 * @param right Right edge of the view volume.
 * @param bottom Bottom edge of the view volume.
 * @param top Top edge of the view volume.
 * @param nearZ Distance to the near plane.
 * @param farZ Distance to the far plane.
 */
inline void MatrixOrthographicOffCenterRightHanded(nn::util::MatrixT4x4fType* pOut, float left,
                                                   float right, float bottom, float top,
                                                   float nearZ, float farZ) {
    const float inverseWidth = 1.0f / (right - left);
    const float inverseHeight = 1.0f / (top - bottom);
    const float inverseDepth = 1.0f / (nearZ - farZ);
    pOut->_m.val[0] = vsetq_lane_f32(-((left + right) * inverseWidth),
                                     MakeVector(2.0f * inverseWidth, 0.0f, 0.0f), 3);
    pOut->_m.val[1] = vsetq_lane_f32(-((top + bottom) * inverseHeight),
                                     MakeVector(0.0f, 2.0f * inverseHeight, 0.0f), 3);
    pOut->_m.val[2] = vsetq_lane_f32(nearZ * inverseDepth, MakeVector(0.0f, 0.0f, inverseDepth), 3);
    pOut->_m.val[3] = vsetq_lane_f32(1.0f, MakeVector(0.0f, 0.0f, 0.0f), 3);
}

/**
 * @brief Make a right-handed view matrix from the backward axis of the camera.
 * @param pOut View matrix.
 * @param eye Position of the camera.
 * @param axisZ Normalized direction from the point the camera looks at to the camera.
 * @param up Up direction of the camera.
 */
inline void MatrixLookAtRightHanded(nn::util::MatrixT4x3fType* pOut, float32x4_t eye,
                                    float32x4_t axisZ, float32x4_t up) {
    const float32x4_t axisX = VectorNormalize(VectorCross(up, axisZ));
    const float32x4_t axisY = VectorCross(axisZ, axisX);
    const float32x4_t negativeEye = vnegq_f32(eye);
    pOut->_m.val[0] = vsetq_lane_f32(vgetq_lane_f32(VectorDot(axisX, negativeEye), 3), axisX, 3);
    pOut->_m.val[1] = vsetq_lane_f32(vgetq_lane_f32(VectorDot(axisY, negativeEye), 3), axisY, 3);
    pOut->_m.val[2] = vsetq_lane_f32(vgetq_lane_f32(VectorDot(axisZ, negativeEye), 3), axisZ, 3);
}

/**
 * @brief Find a state layer of a state machine by its name.
 * @param pStateMachine State machine.
 * @param pName Name of the layer.
 * @return State layer, or nullptr when no layer has that name.
 */
inline StateLayer* FindStateLayerByName(StateMachine* pStateMachine, const char* pName) {
    for (auto& rLayer : pStateMachine->m_StateLayers) {
        if (std::strcmp(rLayer.m_pName, pName) == 0) {
            return &rLayer;
        }
    }

    return nullptr;
}

/**
 * @brief Check whether a layer of a state machine has a state.
 * @param pStateMachine State machine.
 * @param pName Name of the state.
 * @return True when a layer has a state with that name.
 */
inline bool HasState(StateMachine* pStateMachine, const char* pName) {
    for (auto& rLayer : pStateMachine->m_StateLayers) {
        if (rLayer.FindStateByName(pName) != nullptr) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Get the event at the front of an event list.
 * @param rList Event list.
 * @return First event, or nullptr when the list is empty.
 */
inline StateMachineEvent* PeekFirstEvent(StateMachineEventQueue::EventList& rList) {
    return rList.empty() ? nullptr : reinterpret_cast<StateMachineEvent*>(rList.begin().GetNode());
}

/**
 * @brief Get the event at the back of an event list.
 * @param rList Event list.
 * @return Last event, or nullptr when the list is empty.
 */
inline StateMachineEvent* PeekLastEvent(StateMachineEventQueue::EventList& rList) {
    return rList.empty() ? nullptr
                         : reinterpret_cast<StateMachineEvent*>((--rList.end()).GetNode());
}

/**
 * @brief Change a variable of the state machine and report the change to the parent.
 * @param pStateMachine State machine.
 * @param pName Name of the variable.
 * @param value New value.
 */
ALWAYS_INLINE inline void SetVariableValue(StateMachine* pStateMachine, const char* pName,
                                           float value) {
    StateMachineVariableManager& rManager = pStateMachine->m_VariableManager;
    if (rManager.FindRefOnlyByName_(pName) == nullptr) {
        return;
    }

    const float previous = rManager.FindRefOnlyByName_(pName)->value;
    StateMachineVariable* pVariable = rManager.FindByName_(pName);
    detail::ClampValue(value, pVariable->minimum, pVariable->maximum);
    const float oldValue = pVariable->value;
    if (value == oldValue) {
        return;
    }

    pVariable->value = value;
    rManager.PushModifyEvent_(pVariable->name, pVariable);

    for (auto& rCalculated : pVariable->GetCalculatedVariables()) {
        rManager.DoUpdateCalcVarOnValueChanged_(&rCalculated, oldValue, value);
    }

    const float current = rManager.FindRefOnlyByName_(pName)->value;
    if (pStateMachine->m_pListener != nullptr) {
        pStateMachine->m_pListener->OnVariableChanged(pStateMachine->m_pName, pName, previous,
                                                      current);
    }
}
}  // namespace

// The StateMachine functions below are inline in the original headers; this unit is their only
// user, so they are defined here and emitted as weak functions like in the game.

/**
 * @brief Connect the state machine to its parent and to the state machines of its parts.
 * @param pDevice Graphics device.
 * @param pParent State machine of the parent layout, or nullptr for the screen layout.
 */
inline void StateMachine::FirstTimeSetup(nn::gfx::Device* pDevice, StateMachine* pParent) {
    m_pListener = pParent != nullptr
                      ? reinterpret_cast<StateMachineListener*>(pParent->m_pEventHandler)
                      : nullptr;
    m_pDecideStateLayer = FindStateLayerByName(this, "InOut");
    m_IsDecideButton = HasState(this, "Decide");

    for (auto it = m_pLayout->GetPartsList().begin(); it != m_pLayout->GetPartsList().end(); ++it) {
        StateMachine* pStateMachine = GetStateMachine(it->m_pLayout->GetRootPane());
        if (pStateMachine != nullptr) {
            pStateMachine->m_pName = it->GetName();
            pStateMachine->FirstTimeSetup(pDevice, this);
        }
    }

    SetupDecideButtons_();
}

/**
 * @brief Advance the state layers and the parts state machines and process the queued events.
 * @param pDevice Graphics device.
 * @param step Elapsed frames.
 * @param pParent State machine of the parent layout, or nullptr for the screen layout.
 */
inline void StateMachine::Update(nn::gfx::Device* pDevice, float step, StateMachine* pParent) {
    for (auto& rLayer : m_StateLayers) {
        UpdateStateLayer_(&rLayer, pParent);
    }

    for (auto it = m_pLayout->GetPartsList().begin(); it != m_pLayout->GetPartsList().end(); ++it) {
        StateMachine* pStateMachine = GetStateMachine(it->m_pLayout->GetRootPane());
        if (pStateMachine != nullptr) {
            pStateMachine->Update(pDevice, step, this);
        }
    }

    StateMachineEventQueue::EventList& rQueue = m_EventQueue.m_QueuedEvents;
    StateMachineEvent* pEvent = PeekFirstEvent(rQueue);
    StateMachineEvent* pLast = pEvent != nullptr ? PeekLastEvent(rQueue) : nullptr;
    while (pEvent != nullptr) {
        if (pEvent->parameter != 0) {
            pEvent->parameter--;

            StateMachineEvent* pFront = PeekFirstEvent(rQueue);
            if (pFront != nullptr) {
                rQueue.pop_front();
                rQueue.push_back(*pFront);
            }
        } else {
            if (pEvent->type == 9 || pEvent->type == 10) {
                const char* pName = static_cast<const char*>(pEvent->pArgument1);
                float value = *reinterpret_cast<const float*>(pEvent->pArgument2);
                if (pEvent->type == 10) {
                    value += m_VariableManager.FindRefOnlyByName_(pName)->value;
                }

                SetVariableValue(this, pName, value);
            } else {
                for (auto& rLayer : m_StateLayers) {
                    rLayer.UpdateStateLayerTransitions(pDevice, *pEvent);
                }
            }

            if (!rQueue.empty()) {
                StateMachineEvent& rFront = rQueue.front();
                rQueue.pop_front();
                m_EventQueue.m_FreeEvents.push_back(rFront);
            }
        }

        if (pEvent == pLast) {
            break;
        }

        pEvent = PeekFirstEvent(rQueue);
    }
}

/**
 * @brief Queue the pointer events of the state layers and forward the input to the parts.
 * @param pPosition Pointer position, or nullptr when the pointer is not available.
 * @param isDown Whether the pointer is pressed.
 * @param isRelease Whether the pointer was released.
 */
inline void StateMachine::UpdateUserInput(const nn::util::Float2* pPosition, bool isDown,
                                          bool isRelease) {
    m_VariableManager.DoCalculateCalcVar(1.0f);

    if (isDown && !_E0) {
        m_EventQueue.Push(4, nullptr, nullptr, nullptr, 0);
    }

    _E0 = isDown;

    for (auto& rLayer : m_StateLayers) {
        if (pPosition == nullptr) {
            if (rLayer.m_pTargetPane != nullptr) {
                m_EventQueue.Push(3, rLayer.m_pName, nullptr, nullptr, 0);
            }
        } else if (rLayer.m_pTargetPane != nullptr) {
            if (rLayer.m_HitBoxMin.x <= pPosition->x && pPosition->x <= rLayer.m_HitBoxMax.x &&
                rLayer.m_HitBoxMin.y <= pPosition->y && pPosition->y <= rLayer.m_HitBoxMax.y) {
                m_EventQueue.Push(2, rLayer.m_pName, nullptr, nullptr, 0);
            } else {
                m_EventQueue.Push(3, rLayer.m_pName, nullptr, nullptr, 0);
            }
        }
    }

    for (auto it = m_pLayout->GetPartsList().begin(); it != m_pLayout->GetPartsList().end(); ++it) {
        StateMachine* pStateMachine = GetStateMachine(it->m_pLayout->GetRootPane());
        if (pStateMachine != nullptr) {
            pStateMachine->UpdateUserInput(pPosition, isDown, isRelease);
        }
    }
}

/**
 * @brief Update the completion state and the hit box of a state layer and report its changes.
 * @param pStateLayer State layer.
 * @param pParent State machine of the parent layout, or nullptr for the screen layout.
 */
inline void StateMachine::UpdateStateLayer_(StateLayer* pStateLayer, StateMachine* pParent) {
    if (pStateLayer->m_pLayout == nullptr || pStateLayer->m_pCurrentState == nullptr) {
        return;
    }

    bool isCompleted;
    if (pStateLayer->m_pCurrentTransition == nullptr) {
        isCompleted = true;
    } else {
        const Animator* pAnimator = pStateLayer->m_AnimatorSlot.m_pAnimator;
        if (pAnimator == nullptr || !pAnimator->mEnabled) {
            isCompleted = true;
        } else {
            isCompleted = pAnimator->mFrame == static_cast<float>(pAnimator->GetFrameSize());
        }
    }

    const bool wasCompleted = pStateLayer->m_IsPlaying;
    pStateLayer->m_IsPlaying = isCompleted;
    pStateLayer->m_IsPaused = wasCompleted != isCompleted;

    const Pane* pPane = pStateLayer->m_pTargetPane;
    if (pPane != nullptr) {
        const nn::util::MatrixT4x3fType globalMtx =
            *reinterpret_cast<const nn::util::MatrixT4x3fType*>(pPane->GetGlobalMtx());
        const float halfWidth = std::fabs(pPane->GetSizeX() * globalMtx._m.val[0][0]) * 0.5f;
        const float halfHeight = std::fabs(pPane->GetSizeY() * globalMtx._m.val[1][1]) * 0.5f;
        const float centerX = globalMtx._m.val[0][3];
        const float centerY = globalMtx._m.val[1][3];

        const int basePositionX = pPane->GetBasePositionX();
        const int basePositionY = pPane->GetBasePositionY();

        float x;
        switch (basePositionX) {
        case 1:
            x = centerX + halfWidth;
            break;
        case 2:
            x = centerX - halfWidth;
            break;
        default:
            x = centerX;
            break;
        }

        float y;
        switch (basePositionY) {
        case 1:
            y = centerY - halfHeight;
            break;
        case 2:
            y = centerY + halfHeight;
            break;
        default:
            y = centerY;
            break;
        }

        const float left = x - halfWidth;
        const float right = halfWidth + x;
        const float bottom = y - halfHeight;
        const float top = halfHeight + y;
        pStateLayer->m_HitBoxMax.x = right;
        pStateLayer->m_HitBoxMin.x = left;
        pStateLayer->m_HitBoxMin.y = bottom;
        pStateLayer->m_HitBoxMax.y = top;
    }

    if (wasCompleted == isCompleted) {
        return;
    }

    m_EventQueue.Push(5, pStateLayer->m_pName, pStateLayer->m_pCurrentState->m_pName, nullptr, 0);

    StateMachineEventHandler* pHandler = GetParentEventHandler(this);
    if (pHandler != nullptr) {
        const char* pStateName = pStateLayer->m_pCurrentState->m_pName;
        pHandler->OnStateChangeCompletedRaw(m_pName, pStateLayer->m_pName, pStateName);
        GetParentEventHandler(this)->OnStateChangeCompleted(
            m_pName, static_cast<StateMachineUiEventKind>(GetUiEventKind(pStateName)));
    }
}

/** @brief Construct a closed screen without a layout. */
Screen::Screen()
    : mScreenManager(nullptr), mDrawInfo(nullptr), mLayout(nullptr),
      mProjectionMtx{{{{1.0f, 0.0f, 0.0f, 0.0f},
                       {0.0f, 1.0f, 0.0f, 0.0f},
                       {0.0f, 0.0f, 1.0f, 0.0f},
                       {0.0f, 0.0f, 0.0f, 1.0f}}}},
      mViewMtx{{{{1.0f, 0.0f, 0.0f, 0.0f}, {0.0f, 1.0f, 0.0f, 0.0f}, {0.0f, 0.0f, 1.0f, 0.0f}}}},
      mIsPerspective(false), mFovy(40.0f), mNear(1.0f), mFar(10000.0f), mIsViewDirty(false),
      mIsVisible(true), mIsPaused(false), mOpenRequest(0), mState(State_Closed),
      mControlCreator(nullptr), _118(nullptr), mIsUtf8(false), _151(0),
      mViewerType(ViewerType_None), mIsUpdated(false), mScreenIndex(-1), mScreenId(-1) {
    mBuildResultInformation.SetDefault();
}

/** @brief Destroy the screen; Finalize must have released its resources. */
Screen::~Screen() = default;

/**
 * @brief Create the draw info of the screen.
 * @return Draw info that uses the graphics resource and the constant buffer of the manager.
 */
DrawInfo* Screen::CreateDrawInfo_() {
    DrawInfo* pDrawInfo = Layout::NewObj<DrawInfo>();
    pDrawInfo->SetGraphicsResource(mScreenManager->m_pGraphicsResource);
    pDrawInfo->m_pConstantBuffer = &mScreenManager->m_ConstantBuffer;
    pDrawInfo->m_pFontConstantBuffer = &mScreenManager->m_ConstantBuffer;
    return pDrawInfo;
}

/**
 * @brief Create the creator of the buttons and controls of the layout.
 * @return Control creator that registers into the button group and the control list.
 */
ControlCreator* Screen::CreateControlCreator_() {
    return Layout::NewObj<DefaultControlCreatorEx>(&mButtonGroup, &mControlList);
}

/**
 * @brief Build the layout of the screen and register the screen with its manager.
 * @param pDevice Graphics device.
 * @param pScreenManager Manager that owns the screen.
 * @param pLayoutName Name of the layout resource.
 * @param screenIndex Index of the screen in the manager.
 */
void Screen::Initialize(nn::gfx::Device* pDevice, ScreenManager* pScreenManager,
                        const char* pLayoutName, int screenIndex) {
    mScreenManager = pScreenManager;
    mDrawInfo = CreateDrawInfo_();
    mControlCreator = CreateControlCreator_();
    mScreenIndex = screenIndex;
    mIsVisible = true;
    mIsPaused = false;
    mOpenRequest = 0;
    mState = State_Closed;

    ResourceAccessor* pAccessor = DoCreateResourceAccessor_();

    char fileName[260];
    std::strncpy(fileName, pLayoutName, sizeof(fileName));
    std::strncpy(fileName, ".bflyt", sizeof(fileName));
    const void* pResource = pAccessor->FindResourceByName('blyt', fileName);

    mIsUpdated = false;
    LayoutEx* pLayout = DoBuildLayout_(pDevice, fileName, pAccessor);
    mLayout = pLayout;

    if (GetStateMachine(pLayout->GetRootPane()) != nullptr) {
        GetStateMachine(pLayout->GetRootPane())->FirstTimeSetup(pDevice, nullptr);
    }

    SetupPaneAfterBuildRecursively_(pDevice, pLayout->GetRootPane(), pLayout);
    DoBuildAnimatons_(pDevice, pResource);
    DoInitialize_(pDevice);

    if (mViewerType != ViewerType_None) {
        CreateRestAnimators_(pDevice, DynamicCast<LayoutEx*>(mLayout));
    }

    pAccessor->RegisterTextureViewToDescriptorPool(mScreenManager->m_pRegisterTextureViewFunction,
                                                   mScreenManager->m_pRegisterTextureViewUserData);
}

/**
 * @brief Release the layout, the controls and the resources of the screen.
 * @param pDevice Graphics device.
 */
void Screen::Finalize(nn::gfx::Device* pDevice) {
    if (mLayout == nullptr) {
        return;
    }

    ResourceAccessor* pAccessor = mLayout->GetResourceAccessor();
    DynamicCast<MultiArcResourceAccessorEx*>(pAccessor)->Finalize(pDevice);

    mActiveAnimators.clear();

    DoFinalize_(pDevice);
    DoDestroyAnimatons_(pDevice);
    mButtonGroup.FreeAll();

    for (auto it = mControlList.begin(); it != mControlList.end();) {
        auto next = it;
        ++next;
        mControlList.erase(it);

        ControlBase& rControl = *it;
        rControl.Finalize(pDevice);
        rControl.~ControlBase();
        Layout::FreeMemory(&rControl);
        it = next;
    }

    DeleteLayout_(pDevice);

    if (mScreenIndex != -1) {
        mScreenManager->ResetScreenId(mScreenIndex);
    }

    if (mControlCreator != nullptr) {
        Layout::DeleteObj(mControlCreator);
        mControlCreator = nullptr;
    }

    if (pAccessor != nullptr && mViewerType == ViewerType_None) {
        pAccessor->Finalize(pDevice);
        Layout::DeleteObj(pAccessor);
    }

    Layout::DeleteObj(mDrawInfo);
    mScreenManager = nullptr;
    mDrawInfo = nullptr;
}

/** @brief Request the screen to open with its in animation. */
void Screen::Open() {
    OpenWithOption_(OpenOption_Normal);
}

/**
 * @brief Request the screen to open and activate it in its manager.
 * @param option How the screen opens.
 */
void Screen::OpenWithOption_(OpenOption option) {
    if (DynamicCast<LayoutEx*>(mLayout) == nullptr) {
        return;
    }

    if (option >= mOpenRequest) {
        mOpenRequest = option;
    }

    mScreenManager->ActivateScreen(mScreenIndex);
}

/** @brief Request the screen to open without its in animation. */
void Screen::OpenDirect() {
    OpenWithOption_(OpenOption_Direct);
}

/** @brief Request the screen to close with its out animation. */
void Screen::Close() {
    CloseWithOption_(CloseOption_Normal);
}

/**
 * @brief Request the screen to close.
 * @param option How the screen closes.
 */
void Screen::CloseWithOption_(CloseOption option) {
    if (DynamicCast<LayoutEx*>(mLayout) == nullptr) {
        return;
    }

    if (option <= mOpenRequest) {
        mOpenRequest = option;
    }
}

/** @brief Request the screen to close without its out animation. */
void Screen::CloseDirect() {
    CloseWithOption_(CloseOption_Direct);
}

/**
 * @brief Check whether the screen is open.
 * @return True when the screen is opened and no close is requested.
 */
bool Screen::IsOpened() const {
    return mState == State_Opened && mOpenRequest >= 0;
}

/**
 * @brief Check whether the screen is closed.
 * @return True when the screen is closed and no open is requested.
 */
bool Screen::IsClosed() const {
    return mState == State_Closed && mOpenRequest <= 0;
}

/**
 * @brief Check whether the screen is opening.
 * @return True while the screen opens or an open is requested.
 */
bool Screen::IsOpening() const {
    if (mState == State_Opening) {
        return true;
    }

    if (mOpenRequest > 0) {
        return mState == State_Closed || mState == State_Closing;
    }

    return false;
}

/**
 * @brief Check whether the screen is closing.
 * @return True while the screen closes or a close is requested.
 */
bool Screen::IsClosing() const {
    if (mState == State_Closing) {
        return true;
    }

    if (mOpenRequest < 0) {
        return mState == State_Opening || mState == State_Opened;
    }

    return false;
}

/** @brief Called when the screen starts to open. */
void Screen::DoOpenStart_() {}

/** @brief Called when the screen is open. */
void Screen::DoOpenEnd_() {}

/** @brief Called when the screen starts to close. */
void Screen::DoCloseStart_() {}

/** @brief Called when the screen is closed. */
void Screen::DoCloseEnd_() {}

/**
 * @brief Check whether the in and out animations of the parts are played.
 * @return False.
 */
bool Screen::IsPlayPartsInOut_() const {
    return false;
}

/**
 * @brief Start to open the screen.
 * @param option How the screen opens.
 */
void Screen::OpenStart_(OpenOption option) {
    switch (option) {
    case OpenOption_Normal:
        DynamicCast<LayoutEx*>(mLayout)->Open();
        break;
    case OpenOption_Direct:
        DynamicCast<LayoutEx*>(mLayout)->OpenDirect();
        break;
    default:
        break;
    }

    mState = State_Opening;
    DoOpenStart_();
}

/**
 * @brief Check whether the in animation of the layout has ended.
 * @return True when the layout is open.
 */
bool Screen::IsOpenEnd_() {
    return DynamicCast<LayoutEx*>(mLayout)->mAnimationState == 2;
}

/** @brief Finish opening the screen. */
void Screen::OpenEnd_() {
    mState = State_Opened;
    DoOpenEnd_();
}

/**
 * @brief Start to close the screen.
 * @param option How the screen closes.
 */
void Screen::CloseStart_(CloseOption option) {
    switch (option) {
    case CloseOption_Normal:
        DynamicCast<LayoutEx*>(mLayout)->Close();
        break;
    case CloseOption_Direct:
        DynamicCast<LayoutEx*>(mLayout)->CloseDirect();
        break;
    default:
        break;
    }

    mState = State_Closing;
    DoCloseStart_();
}

/**
 * @brief Check whether the out animation of the layout has ended.
 * @return True when the layout is closed.
 */
bool Screen::IsCloseEnd_() {
    return DynamicCast<LayoutEx*>(mLayout)->mAnimationState == 0;
}

/** @brief Finish closing the screen. */
void Screen::CloseEnd_() {
    mOpenRequest = 0;
    mState = State_Closed;
    DoCloseEnd_();
}

/**
 * @brief Check whether the screen is active in its manager.
 * @return True when the manager updates the screen.
 */
bool Screen::IsActive_() const {
    return mScreenManager->mActiveIds[mScreenIndex] >= 0;
}

/**
 * @brief Check whether the screen is paused.
 * @return True while the buttons, the controls and the animators of the screen are not updated.
 */
bool Screen::IsPaused() const {
    return mIsPaused;
}

/**
 * @brief Set the draw unit of the screen.
 * @param drawUnitId Draw unit that the manager draws the screen in.
 */
void Screen::SetDrawUnitId_(int drawUnitId) {
    mScreenId = drawUnitId;

    if (IsActive_()) {
        mScreenManager->ActivateScreen(mScreenIndex);
    }
}

/**
 * @brief Allocate the layout of the screen.
 * @param pDevice Graphics device.
 * @return Allocated layout.
 */
LayoutEx* Screen::DoAllocateLayout_(nn::gfx::Device* pDevice) {
    return Layout::NewObj<LayoutEx>(this);
}

/**
 * @brief Release the layout of the screen.
 * @param pDevice Graphics device.
 */
void Screen::DeleteLayout_(nn::gfx::Device* pDevice) {
    mLayout->Finalize(pDevice);
    Layout::DeleteObj(mLayout);
    mLayout = nullptr;
}

/**
 * @brief Allocate and build the layout of the screen.
 * @param pDevice Graphics device.
 * @param pName Name of the layout resource.
 * @param pAccessor Accessor of the layout resources.
 * @return Built layout.
 */
LayoutEx* Screen::DoBuildLayout_(nn::gfx::Device* pDevice, const char* pName,
                                 ResourceAccessor* pAccessor) {
    LayoutEx* pLayout = DoAllocateLayout_(pDevice);
    pLayout->BuildWithName(&mBuildResultInformation, pDevice, pAccessor, GetControlCreator(),
                           nullptr, mBuildOption, pName, mIsUtf8);
    return pLayout;
}

/**
 * @brief Count the animators that are played.
 * @return Number of active animators.
 */
int Screen::CountActiveAnimator_() const {
    return mActiveAnimators.size();
}

/**
 * @brief Create the accessor of the layout resources of the screen.
 * @return Accessor, or nullptr when no archive holds the layout.
 */
ResourceAccessor* Screen::DoCreateResourceAccessor_() {
    const ArcResourceMgr* pArcResourceMgr = mScreenManager->m_pArcResourceMgr;
    const FontMgr* pFontMgr = mScreenManager->m_pFontMgr;
    auto* pAccessor = Layout::NewObj<MultiArcResourceAccessorEx>(pArcResourceMgr, pFontMgr);

    const ArcResourceMgr::ArchiveData* pArchive = pArcResourceMgr->FindArchiveData(GetLayoutName());
    if (pArchive == nullptr) {
        return nullptr;
    }

    pAccessor->AttachArchive(pArchive->pArchive, pArchive->pTextureFile);
    return pAccessor;
}

/**
 * @brief Add an animator to the animators updated by the screen.
 * @param animator Animator to update.
 */
void Screen::SetAnimatorActive(AnimatorEx* animator) {
    mActiveAnimators.push_back(*animator);
}

/**
 * @brief Remove an animator from the animators updated by the screen.
 * @param pAnimator Animator to remove.
 */
void Screen::EraseAnimatorFromActiveListInternal_(AnimatorEx* pAnimator) {
    mActiveAnimators.erase(mActiveAnimators.iterator_to(*pAnimator));
}

/**
 * @brief Remove an animator from the animators updated by the screen.
 * @param animator Animator to remove.
 */
void Screen::EraseAnimatorFromActiveList(AnimatorEx* animator) {
    mActiveAnimators.erase(mActiveAnimators.iterator_to(*animator));
}

/**
 * @brief Build the animators of the screen.
 * @param pDevice Graphics device.
 * @param pResource Layout resource.
 */
void Screen::DoBuildAnimatons_(nn::gfx::Device* pDevice, const void* pResource) {}

/**
 * @brief Destroy the animators of the screen.
 * @param pDevice Graphics device.
 */
void Screen::DoDestroyAnimatons_(nn::gfx::Device* pDevice) {}

/**
 * @brief Create an animator for every animation tag that has none.
 * @param pDevice Graphics device.
 * @param pLayout Layout of the animators.
 */
void Screen::CreateRestAnimators_(nn::gfx::Device* pDevice, LayoutEx* pLayout) {
    const int tagCount = pLayout->AcquireAnimTagNameCount();
    for (int i = 0; i < tagCount; i++) {
        const char* pTagName = pLayout->AcquireAnimTagNameByIndex(i);
        if (pTagName != nullptr && pLayout->FindAnimator(pTagName) == nullptr) {
            pLayout->TryCreateAnimatorExAuto(pDevice, pTagName, false);
        }
    }
}

/**
 * @brief Initialize the screen after its layout is built.
 * @param pDevice Graphics device.
 */
void Screen::DoInitialize_(nn::gfx::Device* pDevice) {}

/**
 * @brief Finalize the screen before its layout is released.
 * @param pDevice Graphics device.
 */
void Screen::DoFinalize_(nn::gfx::Device* pDevice) {}

/**
 * @brief Set up a pane after the layout is built.
 * @param pDevice Graphics device.
 * @param pPane Pane to set up.
 * @param pLayout Layout of the pane.
 */
void Screen::SetupPaneAfterBuild_(nn::gfx::Device* pDevice, Pane* pPane, Layout* pLayout) {}

/**
 * @brief Set up a pane and its descendants after the layout is built.
 * @param pDevice Graphics device.
 * @param pPane Pane to set up.
 * @param pLayout Layout of the pane.
 */
void Screen::SetupPaneAfterBuildRecursively_(nn::gfx::Device* pDevice, Pane* pPane,
                                             Layout* pLayout) {
    Parts* pParts = DynamicCast<Parts*>(pPane);
    if (pParts != nullptr) {
        pLayout = pParts->m_pLayout;
    }

    SetupPaneAfterBuild_(pDevice, pPane, pLayout);

    for (nn::util::IntrusiveListNode* pNode = pPane->m_Children.GetNext();
         pNode != &pPane->m_Children; pNode = pNode->GetNext()) {
        SetupPaneAfterBuildRecursively_(pDevice, Pane::FromLink(pNode), pLayout);
    }
}

/**
 * @brief Update the screen every frame.
 * @param pDevice Graphics device.
 */
void Screen::DoUpdate_(nn::gfx::Device* pDevice) {}

/**
 * @brief Convert the pointer input to layout coordinates and update the buttons with it.
 * @param pDevice Graphics device.
 * @param rInput Pointer input.
 */
void Screen::UpdateButtons_(nn::gfx::Device* pDevice, const InputDeviceState& rInput) {
    if (!rInput.isPointerValid) {
        UpdateUserInput_(nullptr, false, false);
        return;
    }

    const nn::util::Float2 viewSize =
        mLayout != nullptr ? mLayout->mLayoutSize : nn::util::Float2{{{1920.0f, 1080.0f}}};
    const float scaleX = mLayout->mLayoutSize.x / viewSize.x;
    const float scaleY = mLayout->mLayoutSize.y / viewSize.y;

    const float viewportWidth = mDrawInfo->mViewport.GetWidth();
    const float viewportHeight = mDrawInfo->mViewport.GetHeight();
    const float x = (rInput.pointerPosition.x + rInput.pointerPosition.x) / viewportWidth + -1.0f;
    const float y = (rInput.pointerPosition.y + rInput.pointerPosition.y) / viewportHeight + -1.0f;

    const bool isTrigger = rInput.isTrigger;
    const bool isRelease = rInput.isRelease;

    nn::util::Float2 position;
    position.x = scaleX * (viewportWidth * 0.5f * x);
    position.y = -(scaleY * (viewportHeight * 0.5f * y));
    UpdateUserInput_(&position, isTrigger, isRelease);
}

/** @brief Advance the open request of the screen. */
void Screen::UpdateScreenOpening_() {
    const int request = mOpenRequest;
    if (request >= 1) {
        mOpenRequest = 0;

        if (request == OpenOption_Restart || mState == State_Closed || mState == State_Closing) {
            OpenStart_(static_cast<OpenOption>(request));

            if (request == OpenOption_Immediate) {
                OpenEnd_();
            }
        } else if (request == OpenOption_Direct && mState == State_Opening) {
            DynamicCast<LayoutEx*>(mLayout)->OpenDirect();
        }
    } else if (request < 0 && mState == State_Closed) {
        mOpenRequest = 0;
        mScreenManager->InactivateScreen(mScreenIndex);
        return;
    }

    if (mState == State_Opening && IsOpenEnd_()) {
        OpenEnd_();
    }
}

/** @brief Advance the close request of the screen. */
void Screen::UpdateScreenClosing_() {
    const int request = mOpenRequest;
    if (request < 0) {
        mOpenRequest = 0;

        if (mState == State_Opening || mState == State_Opened) {
            CloseStart_(static_cast<CloseOption>(request));
        } else if (request == CloseOption_Direct && mState == State_Closing && !IsCloseEnd_()) {
            DynamicCast<LayoutEx*>(mLayout)->CloseDirect();
        }
    }

    if (mState == State_Closing && IsCloseEnd_()) {
        if (mOpenRequest <= 0) {
            mScreenManager->InactivateScreen(mScreenIndex);
        }

        CloseEnd_();
    }
}

/**
 * @brief Update the screen and calculate its layout.
 * @param pDevice Graphics device.
 * @param rInput Pointer input.
 */
void Screen::Update(nn::gfx::Device* pDevice, const InputDeviceState& rInput) {
    if (mLayout == nullptr) {
        return;
    }

    UpdateScreenOpening_();

    if (mIsPaused) {
        UpdateScreenClosing_();
    } else {
        DoUpdate_(pDevice);
        UpdateButtons_(pDevice, rInput);
        UpdateControl_(pDevice);
        UpdateScreenClosing_();
        UpdateAnimator_();
        mIsUpdated = true;
    }

    mDrawInfo->SetProjectionMtx(mProjectionMtx);
    mDrawInfo->SetViewMtx(mViewMtx);
    mLayout->Calculate(*mDrawInfo, mIsViewDirty);
    OnPostCalculate(mLayout);
    mIsViewDirty = false;
}

/**
 * @brief Update the state machine and the controls of the screen.
 * @param pDevice Graphics device.
 */
void Screen::UpdateControl_(nn::gfx::Device* pDevice) {
    StateMachine* pStateMachine = GetStateMachine(mLayout->GetRootPane());
    if (pStateMachine != nullptr) {
        pStateMachine->Update(pDevice, 1.0f, nullptr);
    }

    for (auto& rControl : mControlList) {
        rControl.UpdateControl(1.0f);
    }
}

/** @brief Advance the active animators and stop the ones that finished. */
void Screen::UpdateAnimator_() {
    for (auto it = mActiveAnimators.begin(); it != mActiveAnimators.end();) {
        AnimatorEx& rAnimator = *it;
        ++it;
        rAnimator.Animate();

        if (rAnimator.mSpeed != 0.0f && rAnimator.mEnabled) {
            rAnimator.UpdateFrame(1.0f);
        } else if ((rAnimator.mFlags & 8) == 0) {
            rAnimator.AnimTransform::SetEnabled(false);
            rAnimator.mSpeed = 0.0f;
            rAnimator.mLayout->AnimatorDisableCallback(&rAnimator);

            const u32 flags = rAnimator.mFlags;
            rAnimator.mFlags = flags & ~0xfu;
            if ((flags & 1) != 0) {
                rAnimator.mFlags |= 8;
            } else {
                EraseAnimatorFromActiveListInternal_(&rAnimator);
            }
        } else {
            EraseAnimatorFromActiveListInternal_(&rAnimator);
            rAnimator.mFlags &= ~0xfu;
        }
    }
}

/**
 * @brief Update the buttons, the controls and the state machine with the pointer input.
 * @param pPosition Pointer position in layout coordinates, or nullptr when the pointer is not
 * available.
 * @param isDown Whether the pointer is pressed.
 * @param isRelease Whether the pointer was released.
 */
void Screen::UpdateUserInput_(const nn::util::Float2* pPosition, bool isDown, bool isRelease) {
    if (!mIsVisible) {
        return;
    }

    GetButtonGroup()->Update(pPosition, isDown, isRelease);

    for (auto& rControl : mControlList) {
        rControl.UpdateControlUserInput(pPosition, isDown, isRelease);
    }

    StateMachine* pStateMachine = GetStateMachine(mLayout->GetRootPane());
    if (pStateMachine != nullptr) {
        pStateMachine->UpdateUserInput(pPosition, isDown, isRelease);
    }
}

/**
 * @brief Draw the capture textures of the layout.
 * @param pDevice Graphics device.
 * @param rCommands Command buffer to record into.
 */
void Screen::DrawCaptureTexture(nn::gfx::Device* pDevice, nn::gfx::CommandBuffer& rCommands) {
    if (mLayout == nullptr || !mIsVisible) {
        return;
    }

    mDrawInfo->SetProjectionMtx(mProjectionMtx);
    mDrawInfo->SetViewMtx(mViewMtx);
    mLayout->DrawCaptureTexture(pDevice, *mDrawInfo, rCommands);
}

/**
 * @brief Draw the layout.
 * @param rCommands Command buffer to record into.
 */
void Screen::DrawLayout(nn::gfx::CommandBuffer& rCommands) {
    if (mLayout == nullptr || !mIsVisible || !mIsUpdated) {
        return;
    }

    mDrawInfo->SetProjectionMtx(mProjectionMtx);
    mDrawInfo->SetViewMtx(mViewMtx);
    mLayout->Draw(*mDrawInfo, rCommands);
}

/**
 * @brief Copy the buttons and controls of a layout into a copy of it.
 * @param pDevice Graphics device.
 * @param pSource Copied layout.
 * @param pDestination Copy of the layout.
 * @return Number of copied buttons and controls.
 */
int Screen::CopyLayoutBelongingControls(nn::gfx::Device* pDevice, const Layout* pSource,
                                        Layout* pDestination) {
    ControlCopier::CopyLayoutBelongingControlArg arg;
    arg.pButtonGroup = &mButtonGroup;
    arg.pControlList = &mControlList;
    arg.pDevice = pDevice;

    ControlCopier copier;
    return copier.CopyLayoutBelongingControlsRecursive(arg, pSource, pDestination);
}

/**
 * @brief Copy the buttons and controls of a layout and of its parts into a copy of it.
 * @param rArg Destination of the copied buttons and controls.
 * @param pSource Copied layout.
 * @param pDestination Copy of the layout.
 * @return Number of copied buttons and controls.
 */
int ControlCopier::CopyLayoutBelongingControlsRecursive(const CopyLayoutBelongingControlArg& rArg,
                                                        const Layout* pSource,
                                                        Layout* pDestination) {
    int count = CopyLayoutBelongingControls(rArg, pSource, pDestination);

    for (const auto& rParts : pSource->GetPartsList()) {
        Parts* pDestinationParts = pDestination->FindPartsPaneByName(rParts.GetName());
        if (pDestinationParts->m_pLayout != nullptr) {
            count += CopyLayoutBelongingControlsRecursive(rArg, rParts.m_pLayout,
                                                          pDestinationParts->m_pLayout);
        }
    }

    return count;
}

/**
 * @brief Set the size of the layout and of the view and update the projection and view matrices.
 * @param rLayoutSize Size of the layout.
 * @param rViewSize Size of the view.
 */
void Screen::SetViewSize(const Size& rLayoutSize, const Size& rViewSize) {
    mIsViewDirty = true;

    if (mLayout == nullptr) {
        return;
    }

    const float scaleX = mLayout->mLayoutSize.x / rLayoutSize.width;
    const float scaleY = mLayout->mLayoutSize.y / rLayoutSize.height;
    const float halfViewWidth = rViewSize.width * 0.5f;
    const float halfViewHeight = rViewSize.height * 0.5f;

    float32x4_t eye;
    float32x4_t axisZ;
    if (mIsPerspective) {
        const float halfHeight = scaleY * halfViewHeight;
        const float fovy = nn::util::DegreeToRadian(mFovy);
        const nn::util::AngleIndex halfFovy = nn::util::RadianToAngleIndex(fovy * 0.5f);
        const float sin = nn::util::SinTable(halfFovy);
        const float cos = nn::util::CosTable(halfFovy);
        const float aspect = (scaleX * halfViewWidth) / halfHeight;
        MatrixPerspectiveFieldOfViewRightHanded(&mProjectionMtx, fovy, aspect, mNear, mFar);

        const float distance = halfHeight / (sin / cos);
        eye = MakeVector(0.0f, 0.0f, distance);
        axisZ = VectorNormalize(vsubq_f32(eye, MakeVector(0.0f, 0.0f, 0.0f)));
    } else {
        const float halfWidth = scaleX * halfViewWidth;
        const float halfHeight = scaleY * halfViewHeight;
        const float left = -halfWidth;
        const float right = halfWidth;
        const float bottom = -halfHeight;
        const float top = halfHeight;
        MatrixOrthographicOffCenterRightHanded(&mProjectionMtx, left, right, bottom, top, 0.0f,
                                               500.0f);

        const float centerX = (left + right) * 0.5f;
        const float centerY = (bottom + top) * 0.5f;
        eye = MakeVector(centerX, centerY, 0.0f);
        axisZ = VectorNormalize(vsubq_f32(eye, MakeVector(centerX, centerY, -1.0f)));
    }

    MatrixLookAtRightHanded(&mViewMtx, eye, axisZ, MakeVector(0.0f, 1.0f, 0.0f));
}

/**
 * @brief Get the size of the viewport of the screen.
 * @return Viewport size of the manager.
 */
Size Screen::GetViewportSize() const {
    return mScreenManager->m_ViewportSize;
}

/**
 * @brief Get the name of the layout of the screen.
 * @return Layout name provided by the manager.
 */
const char* Screen::GetLayoutName() const {
    if (mViewerType == ViewerType_Preview) {
        return mScreenManager->GetPreviewBodyLayoutName();
    }

    return mScreenManager->GetBodyLayoutName(mScreenIndex);
}

/** @brief Construct a copier. */
ControlCopier::ControlCopier() = default;

/** @brief Destroy the copier. */
ControlCopier::~ControlCopier() = default;

/** @brief Construct an argument without a destination. */
ControlCopier::CopyLayoutBelongingControlArg::CopyLayoutBelongingControlArg()
    : pButtonGroup(nullptr), pControlList(nullptr), pDevice(nullptr) {}

/**
 * @brief Copy the buttons and controls that belong to a layout.
 * @param rArg Destination of the copied buttons and controls.
 * @param pSource Layout whose buttons and controls are copied.
 * @param pDestination Layout of the copies.
 * @return Number of copied buttons and controls.
 */
int ControlCopier::CopyLayoutBelongingControls(const CopyLayoutBelongingControlArg& rArg,
                                               const Layout* pSource, Layout* pDestination) {
    int count = 0;

    {
        ButtonGroup::ButtonList copiedButtons;
        ButtonGroup::ButtonList& rButtons = rArg.pButtonGroup->mButtons;
        for (auto& rButton : rButtons) {
            if (rButton.GetLayout() == pSource) {
                AnimButton* pCopy = CopySingleButton(rArg.pDevice, rButton, pDestination);
                if (pCopy != nullptr) {
                    copiedButtons.push_back(*pCopy);
                    count++;
                }
            }
        }

        for (auto it = copiedButtons.begin(); it != copiedButtons.end();) {
            auto next = it;
            ++next;
            copiedButtons.erase(it);
            rButtons.push_back(*it);
            it = next;
        }
    }

    {
        ControlList copiedControls;
        for (auto& rControl : *rArg.pControlList) {
            if (rControl.mLayout == pSource) {
                ControlBase* pCopy = CopySingleControl(rArg.pDevice, rControl, pDestination);
                if (pCopy != nullptr) {
                    copiedControls.push_back(*pCopy);
                    count++;
                }
            }
        }

        for (auto it = copiedControls.begin(); it != copiedControls.end();) {
            auto next = it;
            ++next;
            copiedControls.erase(it);
            rArg.pControlList->push_back(*it);
            it = next;
        }
    }

    return count;
}

/**
 * @brief Copy a control into a layout.
 * @param pDevice Graphics device.
 * @param rControl Copied control.
 * @param pLayout Layout of the copy.
 * @return nullptr; controls are not copied.
 */
ControlBase* ControlCopier::CopySingleControl(nn::gfx::Device* pDevice, const ControlBase& rControl,
                                              Layout* pLayout) {
    return nullptr;
}

/**
 * @brief Copy a button into a layout.
 * @param pDevice Graphics device.
 * @param rButton Copied button.
 * @param pLayout Layout of the copy.
 * @return Copy of the button, or nullptr when the button cannot be copied.
 */
AnimButton* ControlCopier::CopySingleButton(nn::gfx::Device* pDevice, const AnimButton& rButton,
                                            Layout* pLayout) {
    const auto* pNormalButton = DynamicCast<const NormalButtonEx*>(&rButton);
    if (pNormalButton == nullptr) {
        return nullptr;
    }

    return Layout::NewObj<NormalButtonEx>(pDevice, *pNormalButton, pLayout);
}
}  // namespace nn::ui2d
