#include <nn/ui2d/ui2d_StateMachine.h>

#include <attributes.h>
#include <algorithm>
#include <cmath>
#include <cstdlib>
#include <cstring>
#include <nn/ui2d/ui2d_Animator.h>
#include <nn/ui2d/ui2d_AnimatorEx.h>
#include <nn/ui2d/ui2d_AnimResource.h>
#include <nn/ui2d/ui2d_Group.h>
#include <nn/ui2d/ui2d_Layout.h>
#include <nn/ui2d/ui2d_LayoutEx.h>
#include <nn/ui2d/ui2d_Material.h>
#include <nn/ui2d/ui2d_Pane.h>
#include <nn/ui2d/ui2d_Parts.h>
#include <nn/ui2d/ui2d_TextBox.h>
#include <nn/ui2d/ui2d_Util.h>
#include <nn/font/detail/font_ResourceFormat.h>
#include <nn/util.h>
#include <nn/util/util_StringUtil.h>

namespace nn::ui2d {
namespace detail {
int VSNPrintf(u16* pBuffer, size_t bufferLength, const u16* pFormat, ...);
}  // namespace detail

namespace {
/**
 * @brief Build a block signature from its characters.
 * @param a First character of the signature.
 * @param b Second character of the signature.
 * @param c Third character of the signature.
 * @param d Fourth character of the signature.
 * @return Signature value.
 */
constexpr u32 MakeSignature(char a, char b, char c, char d) {
    return static_cast<u32>(a) | static_cast<u32>(b) << 8 | static_cast<u32>(c) << 16 |
           static_cast<u32>(d) << 24;
}

/**
 * @brief Cast an object to a type by walking its runtime type information.
 * @tparam T Requested type; must provide GetRuntimeTypeInfoStatic.
 * @tparam U Static type of the object.
 * @param pObject Object to cast; may be nullptr.
 * @return Object as T, or nullptr for a null or incompatible object.
 */
template <typename T, typename U>
inline T* DynamicCast(U* pObject) {
    const auto* pWanted = T::GetRuntimeTypeInfoStatic();
    if (pObject == nullptr) {
        return nullptr;
    }

    const auto* pType = pObject->GetRuntimeTypeInfo();
    while (pType != nullptr) {
        if (pType == pWanted) {
            return static_cast<T*>(pObject);
        }

        pType = pType->m_ParentTypeInfo;
    }

    return nullptr;
}

/**
 * @brief Construct an animation transform and register it with a layout.
 * @tparam T Animation transform type.
 * @param pLayout Layout owning the transform.
 * @return New transform, or nullptr when the allocation fails.
 */
template <typename T>
inline T* CreateAnimTransform(Layout* pLayout) {
    void* pMemory = Layout::AllocateMemory(sizeof(T));
    T* pTransform = pMemory != nullptr ? new (pMemory) T : nullptr;
    if (pTransform != nullptr) {
        pLayout->GetAnimTransformList().push_back(*pTransform);
    }

    return pTransform;
}

/** @brief System extended user data attaching a state machine to a pane. */
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
    const auto* pData =
        static_cast<const StateMachineSystemData*>(pPane->GetSystemExtDataByType(PaneSystemDataType_StateMachine));
    return pData != nullptr ? pData->pStateMachine : nullptr;
}

/**
 * @brief Get the state machine attached to the root pane of a layout.
 * @param pLayout Layout whose root pane is searched.
 * @return State machine; the layout must have one.
 */
inline StateMachine* GetRootStateMachine(const Layout* pLayout) {
    const auto* pData = static_cast<const StateMachineSystemData*>(
        pLayout->GetRootPane()->GetSystemExtDataByType(PaneSystemDataType_StateMachine));
    return pData->pStateMachine;
}

/**
 * @brief Walk forward through a list.
 * @tparam Iterator List iterator type.
 * @param iterator First element.
 * @param count Number of elements to skip.
 * @return Element count positions after the first one.
 */
template <typename Iterator>
inline Iterator Advance(Iterator iterator, int count) {
    for (int i = 0; i != count; ++i) {
        ++iterator;
    }

    return iterator;
}

/**
 * @brief Set one element of the scale/rotate/translate block of a pane.
 * @param pPane Pane to modify.
 * @param index Index of the element, starting at the X translation.
 * @param value New value.
 */
inline void SetSrtElement(Pane* pPane, int index, float value) {
    (&pPane->mPositionX)[index] = value;
    pPane->mFlags |= 0x10;
}

/**
 * @brief Set the visibility bit of a pane.
 * @param pPane Pane to modify.
 * @param isVisible Whether the pane is visible.
 */
inline void SetVisibleFlag(Pane* pPane, bool isVisible) {
    pPane->mFlags = (pPane->mFlags & ~1) | static_cast<u8>(isVisible);
}

/**
 * @brief Convert a feature value to a color element.
 * @param value Feature value.
 * @return Rounded value clamped to [0, 255].
 */
inline int ToColorElement(float value) {
    return static_cast<int>(std::min(std::max(value + 0.5f, 0.0f), 255.0f));
}

/**
 * @brief Compare two decide buttons by their pane name, in descending order.
 *
 * qsort passes pointers to the array elements; like the game, the comparison reads the names
 * through those pointers directly.
 * @param pLhs First button.
 * @param pRhs Second button.
 * @return Comparison result for qsort.
 */
int CompareDecideButton(const void* pLhs, const void* pRhs) {
    return std::strncmp(static_cast<const Pane*>(pRhs)->GetName(), static_cast<const Pane*>(pLhs)->GetName(), 32);
}

/**
 * @brief Access the event curves of an event segment.
 * @param pParameter Event segment.
 * @return Array of the three event curves of the segment.
 */
inline const void** GetEvents(ResParameterizedAnimParameter* pParameter) {
    return *reinterpret_cast<const void***>(&pParameter->value);
}

/**
 * @brief Check whether a layer has a decide button feature parameter.
 * @param pStateLayer Layer to check, or nullptr.
 * @return True when the layer animates a decide button.
 */
inline bool HasDecideParameter(const StateLayer* pStateLayer) {
    if (pStateLayer == nullptr) {
        return false;
    }

    for (const auto& rParameter : pStateLayer->m_FeatureParameters) {
        if (rParameter.m_Kind == 25) {
            return true;
        }
    }

    return false;
}

/**
 * @brief Find the key of a track that is active at a time.
 * @param pTrack Track to search.
 * @param time Time of the timeline.
 * @return Index of the active key, the key count after the end of the track, or -1 before the first key.
 */
inline int FindKeyIndex(const TransitionTimeLineTrack* pTrack, float time) {
    int keyIndex = -1;
    for (int i = 0; i < pTrack->keyCount; ++i) {
        const TransitionTimelineKey& rKey = pTrack->pKeys[i];
        if (rKey.time <= time) {
            keyIndex = i;
            if (time < rKey.time + rKey.scale) {
                break;
            }
        }
    }

    if (keyIndex != -1 && keyIndex == pTrack->keyCount - 1 && pTrack->offset + pTrack->duration <= time) {
        ++keyIndex;
    }

    return keyIndex;
}

/** @brief Longest variable name. */
constexpr int NameLengthMax = 24;

/** @brief Size of the animation content written for a parts state layer. */
constexpr u32 PartsStateLayerContentSize = 0x4c;
}  // namespace

/**
 * @brief Collect the panes animated by the feature parameters and create the animator.
 * @param pLayout Layout owning the panes.
 * @param rFeatureParameters Feature parameters of the layer.
 */
void AnimatorSlot::Initialize(Layout* pLayout, const FeatureParameterList& rFeatureParameters) {
    m_pLayout = pLayout;
    if (rFeatureParameters.size() == 0) {
        return;
    }

    m_pGroup = Layout::NewObj<Group>();
    for (const auto& rParameter : rFeatureParameters) {
        Pane* pPane = pLayout->GetRootPane()->FindPaneByName(rParameter.m_pName, true);
        m_pGroup->AppendPane(pPane);
    }

    m_pAnimator = ConstructAndInitialzieAnimator_();
}

/** @brief Release the animator and the group of the slot. */
void AnimatorSlot::Finalzie() {
    if (m_pAnimator != nullptr) {
        m_pLayout->DeleteAnimTransform(m_pAnimator);
        m_pAnimator = nullptr;
    }

    Layout::DeleteObj(m_pGroup);
    m_pGroup = nullptr;
    m_pLayout = nullptr;
}

/**
 * @brief Bind an animation resource to the panes of the slot and start it.
 * @param pDevice Graphics device.
 * @param pResource Animation resource.
 */
void AnimatorSlot::Bind(nn::gfx::Device* pDevice, const void* pResource) {
    m_pResource = pResource;
    if (m_pGroup == nullptr) {
        return;
    }

    AnimResource resource;
    resource.Set(pResource);
    m_pAnimator->SetResource(pDevice, m_pLayout->GetResourceAccessor(), resource.mAnimation);
    AnimatorEx* pAnimatorEx = DynamicCast<AnimatorEx>(m_pAnimator);
    if (pAnimatorEx == nullptr) {
        DynamicCast<GroupAnimator>(m_pAnimator)->Setup(m_pGroup, true);
    } else {
        pAnimatorEx->SetupBasic(resource, DynamicCast<LayoutEx>(m_pLayout), true);
        pAnimatorEx->BindGroup(m_pGroup);
    }

    m_pAnimator->StopAt(0.0f);
    m_pAnimator->PlayAuto(1.0f);
}

/**
 * @brief Unbind the animation of the slot.
 * @return Animation resource that was bound, or nullptr.
 */
void* AnimatorSlot::Unbind() {
    if (m_pAnimator != nullptr) {
        m_pAnimator->UnbindAll();
        m_pAnimator->ResetAnimResource();
    }

    const void* pResource = m_pResource;
    m_pResource = nullptr;
    return const_cast<void*>(pResource);
}

/**
 * @brief Create the animator of the slot and register it with the layout.
 * @return New animator.
 */
Animator* AnimatorSlot::ConstructAndInitialzieAnimator_() {
    LayoutEx* pLayoutEx = DynamicCast<LayoutEx>(m_pLayout);
    if (pLayoutEx == nullptr) {
        return CreateAnimTransform<GroupAnimator>(pLayoutEx);
    }

    return CreateAnimTransform<AnimatorEx>(pLayoutEx);
}

/**
 * @brief Compute the size of the animation resource built for a transition.
 * @param rStateLayer Layer playing the transition.
 * @param rTransition Transition to play.
 * @return Size of the resource in bytes.
 */
size_t StateLayer::CalculateAnimationResourceSize(const StateLayer& rStateLayer, const Transition& rTransition) {
    RuntimeResAnimationBuilder builder;
    return builder.CalcAnimationBlockSize_(rStateLayer, rTransition) + sizeof(nn::font::detail::BinaryFileHeader);
}

/**
 * @brief Start the transitions of the layer triggered by an event.
 * @param pDevice Graphics device.
 * @param rEvent Event being processed.
 */
void StateLayer::UpdateStateLayerTransitions(nn::gfx::Device* pDevice, const StateMachineEvent& rEvent) {
    if (m_pLayout == nullptr || m_pCurrentState == nullptr) {
        return;
    }

    if (m_FeatureParameters.size() == 0) {
        return;
    }

    switch (m_InitialStateIndex) {
    case Mode_StateByVariable: {
        StateMachine* pStateMachine = GetRootStateMachine(m_pLayout);
        const float value = pStateMachine->m_VariableManager.FindRefOnlyByName_(m_pName)->value;
        State* pState = GetStateByIndex(static_cast<int>(value));
        if (pState == nullptr) {
            return;
        }

        const int current = m_RuntimeTransitionIndex;
        const char* pCurrentName = m_RuntimeTransitions[current].m_pSourceStateName;
        if (pCurrentName != nullptr && std::strncmp(pCurrentName, pState->m_pName, 32) == 0) {
            return;
        }

        const int next = current == 0 ? 1 : 0;
        m_RuntimeTransitionIndex = next;
        Transition& rTransition = m_RuntimeTransitions[next];
        rTransition.m_pSourceStateName = pState->m_pName;
        rTransition.m_pName = m_pCurrentTransition == nullptr ? m_States.front().m_pName
                                                              : m_pCurrentTransition->m_pSourceStateName;
        if (rTransition.m_pTimeline == nullptr) {
            rTransition.m_pTimeline = m_Transitions.front().m_pTimeline;
        }

        ChangeCurrentState_(pDevice, rTransition);
        return;
    }
    case Mode_FrameByVariable: {
        StateMachine* pStateMachine = GetRootStateMachine(m_pLayout);
        const float value = pStateMachine->m_VariableManager.FindRefOnlyByName_(m_pName)->value;
        if (m_pCurrentTransition == nullptr) {
            const char* pFromName = m_States.front().m_pName;
            const char* pToName = (++m_States.begin())->m_pName;
            const Transition* pTransition = nullptr;
            for (auto& rTransition : m_Transitions) {
                if (std::strcmp(rTransition.GetFromStateName(), pFromName) == 0 &&
                    std::strcmp(rTransition.GetToStateName(), pToName) == 0) {
                    pTransition = &rTransition;
                    break;
                }
            }

            ChangeCurrentState_(pDevice, *pTransition);
        }

        const Transition* pTransition = m_pCurrentTransition;
        const float direction = pTransition->IsReverse() ? -1.0f : 1.0f;
        float frame = value * (direction * m_AnimatorSlot.m_pAnimator->GetFrameSize() /
                               pTransition->m_pTimeline->duration);
        if (frame < 0.0f) {
            frame += m_AnimatorSlot.m_pAnimator->GetFrameSize();
        }

        Animator* pAnimator = m_AnimatorSlot.m_pAnimator;
        if (pAnimator != nullptr && pAnimator != nullptr && pAnimator->mFrame != frame) {
            pAnimator->StopAt(frame);
        }

        return;
    }
    default:
        break;
    }

    if (!m_IsTransitionLocked && m_pCurrentTransition != nullptr) {
        const Animator* pAnimator = m_AnimatorSlot.m_pAnimator;
        if (pAnimator != nullptr && pAnimator->mEnabled && pAnimator->mFrame != pAnimator->GetFrameSize()) {
            return;
        }
    }

    for (auto& rTransition : m_Transitions) {
        if (std::strncmp(rTransition.GetFromStateName(), m_pCurrentState->m_pName, 32) == 0 &&
            rTransition.m_pCondition->IsTriggered(rEvent)) {
            ChangeCurrentState_(pDevice, rTransition);
            return;
        }
    }
}

/**
 * @brief Make the destination of a transition the current state and start the transition animation.
 * @param pDevice Graphics device.
 * @param rTransition Transition to start.
 */
inline NOINLINE void StateLayer::ChangeCurrentState_(nn::gfx::Device* pDevice, const Transition& rTransition) {
    m_pCurrentState = FindStateByName(rTransition.GetToStateName());
    m_IsPlaying = false;
    m_IsPaused = false;
    m_IsTransitionLocked = rTransition.m_IsCancelable != 0 || rTransition.m_IsLoop != 0;
    const Transition* pPrevTransition = m_pCurrentTransition;
    const Animator* pAnimator = m_AnimatorSlot.m_pAnimator;
    const float frame = pAnimator != nullptr ? pAnimator->mFrame : 0.0f;
    void* pAnimation = m_AnimatorSlot.Unbind();
    if (pAnimation != nullptr && !m_HasPartsStateLayer) {
        Layout::FreeMemory(pAnimation);
    }

    m_pCurrentTransition = nullptr;
    BindSlot_(pDevice, &rTransition, pPrevTransition, frame);
}

/**
 * @brief Build the animation resource that plays a transition.
 * @param pBuffer Buffer receiving the resource.
 * @param size Size of the buffer.
 * @param rStateLayer Layer playing the transition.
 * @param pTransition Transition to play.
 * @param pPrevTransition Transition that was playing, or nullptr.
 * @param frame Frame of the transition that was playing.
 */
void StateLayer::BuildAnimationResource(void* pBuffer, size_t size, const StateLayer& rStateLayer,
                                        const Transition* pTransition, const Transition* pPrevTransition,
                                        float frame) {
    RuntimeResAnimationBuilder builder;
    builder.BuildFromCurrent(pBuffer, size, rStateLayer, pTransition, pPrevTransition, frame);
}

/**
 * @brief Write an animation resource that plays a transition from the current values of a layer.
 * @param pBuffer Buffer receiving the resource.
 * @param size Size of the buffer.
 * @param rStateLayer Layer playing the transition.
 * @param pTransition Transition to play.
 * @param pPrevTransition Transition that was playing, or nullptr.
 * @param frame Frame of the transition that was playing.
 */
inline NOINLINE void RuntimeResAnimationBuilder::BuildFromCurrent(void* pBuffer, size_t size, const StateLayer& rStateLayer,
                                                  const Transition* pTransition,
                                                  const Transition* pPrevTransition, float frame) {
    auto* pHeader = static_cast<nn::font::detail::BinaryFileHeader*>(pBuffer);
    pHeader->signature = MakeSignature('F', 'L', 'A', 'N');
    pHeader->byteOrder = 0xfeff;
    pHeader->headerSize = sizeof(nn::font::detail::BinaryFileHeader);
    pHeader->version = 0x09000000;
    pHeader->dataBlocks = 1;

    nn::util::BytePtr ptr(pBuffer, sizeof(nn::font::detail::BinaryFileHeader));
    auto* pBlock = ptr.Get<ResAnimationBlock>();
    u16 contentCount = 0;
    for (const auto& rParameter : rStateLayer.m_FeatureParameters) {
        contentCount += rParameter.m_Kind == 26 ? 4 : 1;
    }

    pBlock->contentCount = contentCount;
    pBlock->textureCount = 0;
    pBlock->frameSize = static_cast<int>(pTransition->m_pTimeline->duration);
    pBlock->signature = MakeSignature('p', 'a', 'i', '1');
    pBlock->size = size - sizeof(nn::font::detail::BinaryFileHeader);
    pBlock->loop = pTransition->m_IsLoop;
    const int offsetTableSize = pBlock->contentCount * static_cast<int>(sizeof(u32));
    ptr.Advance(sizeof(ResAnimationBlock) + offsetTableSize);
    pBlock->contentOffsets = sizeof(ResAnimationBlock);

    u32* pOffsets = reinterpret_cast<u32*>(pBlock + 1);
    int index = 0;
    for (const auto& rParameter : rStateLayer.m_FeatureParameters) {
        const u32 count = rParameter.m_Kind == 26 ? 4 : 1;
        u32* pContentOffsets = &pOffsets[index];
        u32 i = 0;
        for (; i < count; ++i) {
            pContentOffsets[i] = ptr.Get<u8>() - reinterpret_cast<u8*>(pBlock);
            WriteContent_(rStateLayer, rParameter, pTransition, pPrevTransition, frame, &ptr, i);
        }

        index += i;
    }
}

/**
 * @brief Apply translate, rotate, scale and size values to a pane.
 * @param pPane Pane to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPaneSrt_(Pane* pPane, const FeatureParameter& rParameter,
                                                 const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        SetSrtElement(pPane, rParameter.m_pAnimInfos[index].pTargets[i], rStore.m_pValues[index].pValues[i]);
    }
}

/**
 * @brief Apply visibility values to a pane.
 * @param pPane Pane to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPaneVisilility_(Pane* pPane, const FeatureParameter& rParameter,
                                                        const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        SetVisibleFlag(pPane, rStore.m_pValues[index].pValues[i] != 0.0f);
    }
}

/**
 * @brief Apply color element values to a pane.
 * @param pPane Pane to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPaneTransparancy_(Pane* pPane, const FeatureParameter& rParameter,
                                                          const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        const u8 element = ToColorElement(rStore.m_pValues[index].pValues[i]);
        pPane->SetColorElement(rParameter.m_pAnimInfos[index].pTargets[i], element);
    }
}

/**
 * @brief Apply procedural shape values to a pane.
 * @param pPane Pane to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPaneRoundRect_(Pane* pPane, const FeatureParameter& rParameter,
                                                       const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        const u8 target = rParameter.m_pAnimInfos[index].pTargets[i];
        const float value = rStore.m_pValues[index].pValues[i];
        auto* pData = static_cast<u8*>(pPane->GetSystemExtDataForModify(PaneSystemDataType_ProceduralShape));
        if (pData != nullptr) {
            reinterpret_cast<float*>(pData + 0x1c)[target] = value;
        }
    }
}

/**
 * @brief Apply per-character transform values to a text box.
 * @param pPane Text box to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPanePerCharacterTransform_(Pane* pPane, const FeatureParameter& rParameter,
                                                                   const FeatureParameterStore& rStore,
                                                                   int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        static_cast<TextBox*>(pPane)->SetPerCharacterTransform(rParameter.m_pAnimInfos[index].pTargets[i],
                                                               rStore.m_pValues[index].pValues[i]);
    }
}

/**
 * @brief Apply mask texture SRT values to a pane.
 * @param pPane Pane to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToPaneMaskTexSRT_(Pane* pPane, const FeatureParameter& rParameter,
                                                        const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        const u8 target = rParameter.m_pAnimInfos[index].pTargets[i];
        const float value = rStore.m_pValues[index].pValues[i];
        auto* pData = static_cast<u8*>(pPane->GetSystemExtDataForModify(PaneSystemDataType_Mask));
        if (pData != nullptr) {
            reinterpret_cast<float*>(pData + 0x18)[target] = value;
        }
    }
}

/**
 * @brief Apply color values to a material.
 * @param pMaterial Material to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
NOINLINE void StateLayer::ApplyFeatureParameterToMaterialColor_(Material* pMaterial,
                                                                const FeatureParameter& rParameter,
                                                                const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    for (int i = 0; i < count; ++i) {
        const int target = rParameter.m_pAnimInfos[index].pTargets[i];
        const float value = rStore.m_pValues[index].pValues[i];
        if (target <= 0x1c) {
            pMaterial->SetColorElement(target, ToColorElement(value));
        } else {
            const u32 colorIndex = static_cast<u32>((target - 0x1c) / 4);
            if (colorIndex < 2) {
                switch (target % 4) {
                case 0:
                    pMaterial->m_pFloatColors[colorIndex].x = value;
                    break;
                case 1:
                    pMaterial->m_pFloatColors[colorIndex].y = value;
                    break;
                case 2:
                    pMaterial->m_pFloatColors[colorIndex].z = value;
                    break;
                case 3:
                    pMaterial->m_pFloatColors[colorIndex].w = value;
                    break;
                default:
                    break;
                }
            }
        }
    }
}

/**
 * @brief Apply texture SRT values to a material.
 * @param pMaterial Material to modify.
 * @param rParameter Feature parameter describing the targets.
 * @param rStore Values of the feature parameter.
 * @param index Index of the animation info.
 */
void StateLayer::ApplyFeatureParameterToTextureMatrix_(Material* pMaterial, const FeatureParameter& rParameter,
                                                       const FeatureParameterStore& rStore, int index) {
    const int count = rParameter.m_pAnimInfos[index].targetCount;
    if (rParameter.m_TargetIndex < static_cast<u8>(pMaterial->m_MemNum.texMap)) {
        for (int i = 0; i < count; ++i) {
            ResTexSrt& rTexSrt = pMaterial->GetTexSrtAry()[rParameter.m_TargetIndex];
            reinterpret_cast<float*>(&rTexSrt)[rParameter.m_pAnimInfos[index].pTargets[i]] =
                rStore.m_pValues[index].pValues[i];
        }
    }
}

/**
 * @brief Apply the values of a feature parameter to the pane, material or state machine it animates.
 * @param rParameter Feature parameter.
 * @param rStore Values of the feature parameter.
 */
void StateLayer::ApplyFeatureParameterToTarget_(const FeatureParameter& rParameter,
                                                const FeatureParameterStore& rStore) {
    switch (rParameter.m_AnimContentType) {
    case 0: {
        Pane* pPane = m_pLayout->GetRootPane()->FindPaneByName(rParameter.m_pName, true);
        switch (rParameter.m_pAnimInfos->signature) {
        case MakeSignature('F', 'L', 'P', 'A'):
            ApplyFeatureParameterToPaneSrt_(pPane, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'V', 'I'):
            ApplyFeatureParameterToPaneVisilility_(pPane, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'V', 'C'):
            ApplyFeatureParameterToPaneTransparancy_(pPane, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'P', 'S'):
            ApplyFeatureParameterToPaneRoundRect_(pPane, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'C', 'C'):
            ApplyFeatureParameterToPanePerCharacterTransform_(pPane, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'M', 'T'):
            ApplyFeatureParameterToPaneMaskTexSRT_(pPane, rParameter, rStore, 0);
            break;
        default:
            break;
        }

        break;
    }
    case 1: {
        Material* pMaterial = m_pLayout->GetRootPane()->FindMaterialByName(rParameter.m_pName, true);
        switch (rParameter.m_pAnimInfos->signature) {
        case MakeSignature('F', 'L', 'M', 'C'):
            ApplyFeatureParameterToMaterialColor_(pMaterial, rParameter, rStore, 0);
            break;
        case MakeSignature('F', 'L', 'T', 'S'):
            ApplyFeatureParameterToTextureMatrix_(pMaterial, rParameter, rStore, 0);
            break;
        default:
            break;
        }

        break;
    }
    case 4: {
        Pane* pPane = m_pLayout->GetRootPane()->FindPaneByName(rParameter.m_pName, true);
        StateMachine* pStateMachine = GetStateMachine(pPane) != nullptr
                                          ? GetStateMachine(pPane)
                                          : GetStateMachine(m_pLayout->GetRootPane());
        if (pStateMachine == nullptr) {
            break;
        }

        if (rParameter.m_pAnimInfos->signature != MakeSignature('F', 'P', 'S', 'M')) {
            break;
        }

        const int count = rParameter.m_pAnimInfos->targetCount;
        for (int i = 0; i < count; ++i) {
            const float* pValues = rStore.m_pValues->pValues;
            const auto* pPartsStateLayer =
                static_cast<const ResStatePartsStateLayer*>(rParameter.m_pExtraResource);
            const char* pVariableName = nullptr;
            switch (i) {
            case 0:
                pVariableName = pPartsStateLayer->variableNames[0];
                break;
            case 1:
                pVariableName = pPartsStateLayer->variableNames[1];
                break;
            case 2:
                pVariableName = pPartsStateLayer->variableNames[2];
                break;
            case 3:
                pVariableName = pPartsStateLayer->variableNames[3];
                break;
            default:
                break;
            }

            if (pVariableName != nullptr && strnlen(pVariableName, 32) != 0) {
                pStateMachine->SetFloatValue(pVariableName, pValues[i]);
            }
        }

        break;
    }
    default:
        break;
    }
}

/**
 * @brief Change a variable and notify the listener.
 * @param pName Name of the variable.
 * @param value New value.
 */
inline NOINLINE void StateMachine::SetFloatValue(const char* pName, float value) {
    if (m_VariableManager.FindRefOnlyByName_(pName) == nullptr) {
        return;
    }

    const float previous = m_VariableManager.FindRefOnlyByName_(pName)->value;
    if (!m_VariableManager.SetFloatValue(pName, value)) {
        return;
    }

    const StateMachineVariable* pVariable = m_VariableManager.FindRefOnlyByName_(pName);
    const float current = pVariable->value;
    if (m_pListener != nullptr) {
        m_pListener->OnVariableChanged(m_pName, pName, previous, current);
    }
}

/**
 * @brief Apply the values a state stores to every feature parameter of the layer.
 * @param pState State whose values are applied.
 */
void StateLayer::ApplyFeatureParameterToTargetAll_(const State* pState) {
    int index = 0;
    for (const auto& rParameter : m_FeatureParameters) {
        const FeatureParameterStore* pStore = nullptr;
        int storeIndex = 0;
        for (const auto& rStore : pState->m_StoreSet.m_Stores) {
            if (storeIndex == index) {
                pStore = &rStore;
                break;
            }

            ++storeIndex;
        }

        ApplyFeatureParameterToTarget_(rParameter, *pStore);
        ++index;
    }
}

/**
 * @brief Name a variable and set its type and range.
 * @param pVariable Variable to set up.
 * @param pName Name of the variable.
 * @param type Type of the variable.
 * @param defaultValue Value the variable is reset to.
 * @param minimum Smallest value of the variable.
 * @param maximum Largest value of the variable.
 */
void StateMachineVariableManager::InitialzieVariable_(StateMachineVariable* pVariable, const char* pName,
                                                      StateMachineVariableType type, float defaultValue,
                                                      float minimum, float maximum) {
    nn::util::Strlcpy(pVariable->name, pName, NameLengthMax + 1);
    pVariable->type = type;
    pVariable->value = 0.0f;
    pVariable->_44 = 0;
    pVariable->defaultValue = defaultValue;
    pVariable->minimum = minimum;
    pVariable->maximum = maximum;
}

/** @brief Release every variable and calculated variable. */
void StateMachineVariableManager::Finalize() {
    auto& rVariables = GetVariables();
    auto it = rVariables.begin();
    while (it != rVariables.end()) {
        StateMachineVariable& rVariable = *it;
        ++it;
        auto& rCalculatedVariables = rVariable.GetCalculatedVariables();
        auto calculatedIt = rCalculatedVariables.begin();
        while (calculatedIt != rCalculatedVariables.end()) {
            StateMachineCalclatedVariable& rCalculated = *calculatedIt;
            ++calculatedIt;
            if (rCalculated.ppTargetPanes != nullptr) {
                Layout::FreeMemory(rCalculated.ppTargetPanes);
            }

            rCalculated.ppTargetPanes = nullptr;
            it->GetCalculatedVariables().erase(rCalculatedVariables.iterator_to(rCalculated));
            Layout::FreeMemory(&rCalculated);
        }

        rVariables.erase(rVariables.iterator_to(rVariable));
        Layout::FreeMemory(&rVariable);
    }

    m_pStateMachine = nullptr;
    m_pEventQueue = nullptr;
}

/**
 * @brief Register a variable and its calculated variables from a resource.
 * @param pResource Variable resource.
 * @return False when a variable with the same name already exists.
 */
bool StateMachineVariableManager::RegisterNewVariableByResource(const ResStateVariableDescriptions* pResource) {
    StateMachineVariable* pVariable =
        DoRegisterNewVariable_(pResource->name, StateMachineVariableType_Float, pResource->defaultValue,
                               pResource->minimum, pResource->maximum);
    if (pVariable == nullptr) {
        return false;
    }

    for (u32 i = 0; i < pResource->calculatedVariableCount; ++i) {
        const ResStateCalculatedVariables* pCalculatedResource = &pResource->pCalculatedVariables[i];
        StateMachineCalclatedVariable* pCalculated = Layout::NewObj<StateMachineCalclatedVariable>();
        InitialzieStateMachineCalclatedVariable_(pCalculated, pCalculatedResource);
        pVariable->GetCalculatedVariables().push_back(*pCalculated);
    }

    return true;
}

/**
 * @brief Create a variable unless one with the same name exists.
 * @param pName Name of the variable.
 * @param type Type of the variable.
 * @param defaultValue Initial value of the variable.
 * @param minimum Smallest value of the variable.
 * @param maximum Largest value of the variable.
 * @return New variable, or nullptr when the name is taken.
 */
StateMachineVariable* StateMachineVariableManager::DoRegisterNewVariable_(const char* pName,
                                                                          StateMachineVariableType type,
                                                                          float defaultValue, float minimum,
                                                                          float maximum) {
    if (FindRefOnlyByName_(pName) != nullptr) {
        return nullptr;
    }

    StateMachineVariable* pVariable = Layout::NewObj<StateMachineVariable>();
    InitialzieVariable_(pVariable, pName, type, defaultValue, minimum, maximum);
    ResetToDefalut_(pVariable);
    GetVariables().push_back(*pVariable);
    return pVariable;
}

/**
 * @brief Set up a calculated variable and look up the panes its actions modify.
 * @param pVariable Calculated variable to set up.
 * @param pResource Calculated variable resource.
 */
void StateMachineVariableManager::InitialzieStateMachineCalclatedVariable_(
    StateMachineCalclatedVariable* pVariable, const ResStateCalculatedVariables* pResource) {
    pVariable->changed = false;
    pVariable->elapsed = 0.0f;
    pVariable->resource = pResource;
    if (pResource->postCalculationActionCount != 0) {
        pVariable->ppTargetPanes = Layout::NewArray<Pane*>(pResource->postCalculationActionCount);
        for (u32 i = 0; i < pResource->postCalculationActionCount; ++i) {
            const char* pName = pResource->pCalculation[i].targetName;
            pVariable->ppTargetPanes[i] = m_pStateMachine->m_pLayout->GetRootPane()->FindPaneByName(pName, true);
        }
    } else {
        pVariable->ppTargetPanes = nullptr;
    }

    pVariable->targetPaneCount = pResource->postCalculationActionCount;
}

/**
 * @brief Advance the calculated variables and run their post calculation actions.
 * @param step Elapsed frames.
 */
void StateMachineVariableManager::DoCalculateCalcVar(float step) {
    for (auto& rVariable : GetVariables()) {
        for (auto& rCalculated : rVariable.GetCalculatedVariables()) {
            if (!rCalculated.changed) {
                continue;
            }

            const float previous = rCalculated.value;
            const float value = DoCalculateCalcVar_(&rCalculated, rVariable.value, step);
            if (previous != value) {
                rCalculated.value = value;
            }

            for (u32 i = 0; i < rCalculated.resource->postCalculationActionCount; ++i) {
                const ResStatePostCalculationActions* pAction = &rCalculated.resource->pCalculation[i];
                if (!CheckIsMatchPostActionCondition_(pAction, previous, &rCalculated)) {
                    continue;
                }

                if (pAction->kind == 9) {
                    m_pStateMachine->SetFloatValue(pAction->targetName, rCalculated.value);
                } else {
                    m_pStateMachine->ApplayPostCalcActionToPane(rCalculated.ppTargetPanes[i], pAction->kind,
                                                                rCalculated.value);
                }
            }
        }
    }
}

/**
 * @brief Compute the value of a calculated variable.
 * @param pVariable Calculated variable.
 * @param value Value of the source variable.
 * @param step Elapsed frames.
 * @return Calculated value.
 */
float StateMachineVariableManager::DoCalculateCalcVar_(StateMachineCalclatedVariable* pVariable, float value,
                                                       float step) {
    const ResStateCalculatedVariables* pResource = pVariable->resource;
    if (pResource->isEasingEnabled) {
        value = DoCalculateCalcVarEasing_(pVariable, value, step);
    }

    if (pResource->isLinearScalingEnabled) {
        value = DoCalculateCalcVarLinearScaling_(pResource, value, step);
    }

    if (pResource->isRangeLimitEnabled) {
        value = DoCalculateCalcVarRangeLimit_(pResource, value, step);
    }

    pVariable->changed =
        pResource->isEasingEnabled && !(pVariable->elapsed > pResource->duration + pResource->delay);
    return value;
}

/**
 * @brief Check whether the change of a calculated variable triggers a post calculation action.
 * @param pAction Post calculation action.
 * @param previous Previous value of the calculated variable.
 * @param pVariable Calculated variable.
 * @return True when the action runs.
 */
bool StateMachineVariableManager::CheckIsMatchPostActionCondition_(const ResStatePostCalculationActions* pAction,
                                                                   float previous,
                                                                   StateMachineCalclatedVariable* pVariable) {
    if (pAction->condition == 0) {
        return true;
    }

    const float value = pVariable->value;
    switch (pAction->condition) {
    case 1:
        if (value <= previous) {
            return false;
        }

        break;
    case 2:
        if (value >= previous) {
            return false;
        }

        break;
    case 3:
        if (pVariable->resource->isEasingEnabled && pVariable->nextValue - pVariable->previousValue < 0.0f) {
            return false;
        }

        break;
    case 4:
        if (pVariable->resource->isEasingEnabled && pVariable->nextValue - pVariable->previousValue > 0.0f) {
            return false;
        }

        break;
    default:
        break;
    }

    return true;
}

/**
 * @brief Apply the value of a calculated variable to a pane.
 * @param pPane Pane to modify, or nullptr.
 * @param kind Property of the pane to modify.
 * @param value New value.
 */
void StateMachine::ApplayPostCalcActionToPane(Pane* pPane, u8 kind, float value) {
    if (pPane == nullptr) {
        return;
    }

    switch (kind) {
    case 0:
        pPane->mPositionX = value;
        break;
    case 1:
        pPane->mPositionY = value;
        break;
    case 2:
        pPane->mSizeX = value;
        break;
    case 3:
        pPane->mSizeY = value;
        break;
    case 4:
        pPane->mRotationZ = value;
        break;
    case 5:
        pPane->mAlpha = static_cast<int>(std::fmin(std::fmax(value, 0.0f), 255.0f));
        return;
    case 6: {
        const int integer = static_cast<int>(std::floor(value));
        TextBox* pTextBox = DynamicCast<TextBox>(pPane);
        if (pTextBox != nullptr) {
            u16 text[8] = {};
            detail::VSNPrintf(text, 8, reinterpret_cast<const u16*>(u"%d"), integer);
            pTextBox->SetString(text, 0);
        }

        return;
    }
    case 7: {
        TextBox* pTextBox = DynamicCast<TextBox>(pPane);
        if (pTextBox != nullptr) {
            u16 text[8] = {};
            detail::VSNPrintf(text, 8, reinterpret_cast<const u16*>(u"%.1f"), static_cast<double>(value));
            pTextBox->SetString(text, 0);
        }

        return;
    }
    case 8: {
        TextBox* pTextBox = DynamicCast<TextBox>(pPane);
        if (pTextBox != nullptr) {
            u16 text[8] = {};
            detail::VSNPrintf(text, 8, reinterpret_cast<const u16*>(u"%.2f"), static_cast<double>(value));
            pTextBox->SetString(text, 0);
        }

        return;
    }
    default:
        return;
    }

    pPane->mFlags |= 0x10;
}

/**
 * @brief Find a variable by its name.
 * @param pName Name of the variable.
 * @return Variable, or nullptr when no variable has that name.
 */
const StateMachineVariable* StateMachineVariableManager::FindRefOnlyByName_(const char* pName) const {
    for (const auto& rVariable : GetVariables()) {
        if (std::strcmp(rVariable.name, pName) == 0) {
            return &rVariable;
        }
    }

    return nullptr;
}

/**
 * @brief Reset a variable to its default value.
 * @param variable Variable to reset.
 * @return Whether the value changed.
 */
bool StateMachineVariableManager::ResetToDefalut_(StateMachineVariable* variable) {
    if (variable->value != variable->defaultValue) {
        variable->value = variable->defaultValue;
        return true;
    }

    return false;
}

/**
 * @brief Ease a calculated variable from its previous value towards its next value.
 * @param pVariable Calculated variable.
 * @param value Unused value of the source variable.
 * @param step Elapsed frames.
 * @return Eased value.
 */
float StateMachineVariableManager::DoCalculateCalcVarEasing_(StateMachineCalclatedVariable* pVariable, float value,
                                                             float step) {
    const ResStateCalculatedVariables* pResource = pVariable->resource;
    const float result =
        GetParameterizedAnimValueAtFrameClamped(pVariable->elapsed, pResource->duration, pResource->delay,
                                                pVariable->previousValue, pVariable->nextValue,
                                                pResource->easingType);
    pVariable->elapsed += step;
    return result;
}

/**
 * @brief Scale and offset a value.
 * @param resource Calculated variable resource supplying the scale and the offset.
 * @param value Value to transform.
 * @param step Unused elapsed frames.
 * @return Transformed value.
 */
float StateMachineVariableManager::DoCalculateCalcVarLinearScaling_(const ResStateCalculatedVariables* resource,
                                                                    float value, float step) {
    return value * resource->scale + resource->offset;
}

/**
 * @brief Wrap or clamp a value to the range of a calculated variable.
 * @param resource Calculated variable resource supplying the range.
 * @param value Value to limit.
 * @param step Unused elapsed frames.
 * @return Limited value.
 */
float StateMachineVariableManager::DoCalculateCalcVarRangeLimit_(const ResStateCalculatedVariables* resource,
                                                                 float value, float step) {
    if (resource->limitMode == 1) {
        value = resource->maximum < value ? resource->maximum : value;
        value = value < resource->minimum ? resource->minimum : value;
    } else if (resource->limitMode == 0) {
        value -= resource->maximum * static_cast<int>(std::floor(value / resource->maximum));
    }

    return value;
}

/**
 * @brief Start a new interpolation of a calculated variable.
 * @param variable Calculated variable.
 * @param previous Value the interpolation starts from.
 * @param next Value the interpolation ends at.
 */
void StateMachineVariableManager::DoUpdateCalcVarOnValueChanged_(StateMachineCalclatedVariable* variable,
                                                                 float previous, float next) {
    variable->previousValue = previous;
    variable->nextValue = next;
    variable->elapsed = 0;
    variable->changed = true;
}

/**
 * @brief Find a variable by its name.
 * @param pName Name of the variable.
 * @return Variable, or nullptr when no variable has that name.
 */
StateMachineVariable* StateMachineVariableManager::FindByName_(const char* pName) {
    for (auto& rVariable : GetVariables()) {
        if (std::strcmp(rVariable.name, pName) == 0) {
            return &rVariable;
        }
    }

    return nullptr;
}

/**
 * @brief Queue the event reporting a variable change.
 * @param pName Name of the variable.
 * @param pVariable Changed variable.
 */
void StateMachineVariableManager::PushModifyEvent_(const char* pName, StateMachineVariable* pVariable) {
    m_pEventQueue->Push(7, pName, pVariable, nullptr, 1);
}

/** @brief Collect the parts panes that act as decide buttons, sorted by name. */
void StateMachine::SetupDecideButtons_() {
    const bool hasDecideParameter = HasDecideParameter(m_pDecideStateLayer);
    for (int i = 0; i < DecideButtonCountMax; ++i) {
        m_pDecideButtons[i] = nullptr;
    }

    int count = 0;
    for (auto it = m_pLayout->GetPartsList().begin(); it != m_pLayout->GetPartsList().end(); ++it) {
        Parts& rParts = *it;
        const StateMachine* pStateMachine = GetStateMachine(rParts.m_pLayout->GetRootPane());
        if (pStateMachine == nullptr || !pStateMachine->m_IsDecideButton) {
            continue;
        }

        if (hasDecideParameter) {
            for (const auto& rParameter : m_pDecideStateLayer->m_FeatureParameters) {
                if (std::strcmp(rParameter.m_pName, rParts.GetName()) == 0 && rParameter.m_Kind == 25) {
                    if (count < DecideButtonCountMax) {
                        m_pDecideButtons[count] = &rParts;
                    }

                    ++count;
                    break;
                }
            }
        } else {
            if (count < DecideButtonCountMax) {
                m_pDecideButtons[count] = &rParts;
            }

            ++count;
        }
    }

    std::qsort(m_pDecideButtons, count, sizeof(Pane*), CompareDecideButton);
    m_DecideButtonCount = count;
}

/**
 * @brief Queue the event reporting that a transition completed.
 * @param pStateName Name of the state the layer is in.
 * @param pLayerName Name of the layer.
 * @param pTransitionName Name of the transition.
 */
void StateMachineEventHandler::OnStateChangeCompletedRaw(const char* pStateName, const char* pLayerName,
                                                         const char* pTransitionName) {
    if (m_pStateMachine == nullptr) {
        return;
    }

    m_pStateMachine->m_EventQueue.Push(6, pLayerName, pTransitionName, pStateName, 0);
}

/**
 * @brief Report a decide event of a button to the state machine.
 * @param pPaneName Name of the button pane.
 * @param kind Kind of the event.
 */
void StateMachineEventHandler::OnStateChangeCompleted(const char* pPaneName, StateMachineUiEventKind kind) {
    if (kind != StateMachineUiEventKind_Decided) {
        return;
    }

    StateMachine* pStateMachine = m_pStateMachine;
    if (pStateMachine == nullptr || pStateMachine->m_pDecideStateLayer == nullptr) {
        return;
    }

    int index = -1;
    for (int i = 0; i < pStateMachine->m_DecideButtonCount; ++i) {
        if (std::strcmp(pStateMachine->m_pDecideButtons[i]->GetName(), pPaneName) == 0) {
            index = i;
            break;
        }
    }

    if (index == -1) {
        return;
    }

    pStateMachine->SetFloatValue("ScreenDecideIndex", index);
    m_pStateMachine->m_EventQueue.Push(11, nullptr, nullptr, nullptr, 0);
}

/**
 * @brief Forward a variable change to the state machines bound to it.
 * @param pStateMachineName Unused name of the state machine.
 * @param pVariableName Unused name of the variable.
 * @param previous Unused previous value.
 * @param current New value.
 */
void StateMachineEventHandler::OnVariablesChanged(const char* pStateMachineName, const char* pVariableName,
                                                  float previous, float current) {
    StateMachine* pStateMachine = m_pStateMachine;
    if (pStateMachine == nullptr) {
        return;
    }

    for (int i = 0; i < pStateMachine->m_VariableBindingCount; ++i) {
        const char* pVariableName = pStateMachine->m_pVariableBindings[i].variableName;
        const char* pTargetName = pStateMachine->m_pVariableBindings[i].targetName;
        StateMachine* pTarget;
        if (std::strcmp(pTargetName, "body") == 0) {
            pTarget = pStateMachine;
        } else {
            Parts* pParts = pStateMachine->m_pLayout->FindPartsPaneByName(pTargetName);
            if (pParts == nullptr) {
                continue;
            }

            pTarget = GetStateMachine(pParts->m_pLayout->GetRootPane());
            if (pTarget == nullptr) {
                continue;
            }
        }

        pTarget->SetFloatValue(pVariableName, current);
    }
}

/**
 * @brief Compute the size of the animation block built for a transition.
 * @param rStateLayer Layer playing the transition.
 * @param rTransition Transition to play.
 * @return Size of the block in bytes.
 */
inline NOINLINE size_t RuntimeResAnimationBuilder::CalcAnimationBlockSize_(const StateLayer& rStateLayer,
                                                           const Transition& rTransition) {
    size_t size = sizeof(ResAnimationBlock) + 0xc;
    for (auto it = rStateLayer.m_FeatureParameters.begin(); it != rStateLayer.m_FeatureParameters.end(); ++it) {
        size += sizeof(u32);
    }

    int index = 0;
    for (const auto& rParameter : rStateLayer.m_FeatureParameters) {
        const size_t contentCount = rParameter.m_Kind == 26 ? 4 : 1;
        size_t contentSize = rParameter.m_Kind == 26 ? PartsStateLayerContentSize : 0x20;
        for (int i = 0; i < rParameter.m_AnimInfoCount; ++i) {
            const int targetCount = rParameter.m_pAnimInfos[i].targetCount;
            contentSize += sizeof(u32) + 8;
            for (int j = 0; j < targetCount; ++j) {
                contentSize += sizeof(ResAnimationTarget) + sizeof(u32) * 2;
                const TransitionTimeLineTrack* pTrack = rTransition.m_pTimeline->GetTrack(index);
                if (pTrack != nullptr) {
                    int eventCount = 0;
                    for (int k = 0; k < pTrack->keyCount; ++k) {
                        eventCount += pTrack->pKeys[k].isCurve;
                    }

                    const s64 segmentCount = static_cast<s64>(pTrack->keyCount) + 1;
                    contentSize += segmentCount * sizeof(u32) +
                                   (segmentCount + (eventCount + 1)) * sizeof(ResParameterizedAnimParameter);
                }
            }
        }

        size += contentSize * contentCount;
        ++index;
    }

    return size;
}

/**
 * @brief Bind the animation that plays a transition.
 * @param pDevice Graphics device.
 * @param pTransition Transition to play.
 * @param pPrevTransition Transition that was playing, or nullptr.
 * @param frame Frame of the transition that was playing.
 */
inline NOINLINE void StateLayer::BindSlot_(nn::gfx::Device* pDevice, const Transition* pTransition,
                           const Transition* pPrevTransition, float frame) {
    if (m_HasPartsStateLayer) {
        if (pTransition->GetAnimationName() != nullptr) {
            const void* pResource = m_pLayout->GetAnimResourceData(pTransition->GetAnimationName());
            if (pResource != nullptr) {
                m_AnimatorSlot.Bind(pDevice, pResource);
                const float direction = pTransition->IsReverse() ? -1.0f : 1.0f;
                const float speed =
                    direction * m_AnimatorSlot.m_pAnimator->GetFrameSize() / pTransition->m_pTimeline->duration;
                m_AnimatorSlot.m_pAnimator->PlayAuto(speed);
            }
        }
    } else {
        const size_t size = CalculateAnimationResourceSize(*this, *pTransition);
        void* pBuffer = Layout::AllocateMemory(size);
        BuildAnimationResource(pBuffer, size, *this, pTransition, pPrevTransition, frame);
        m_AnimatorSlot.Bind(pDevice, pBuffer);
    }

    m_pCurrentTransition = pTransition;
}

/**
 * @brief Write one animation content of a feature parameter.
 * @param rStateLayer Layer playing the transition.
 * @param rParameter Feature parameter animated by the content.
 * @param pTransition Transition to play.
 * @param pPrevTransition Transition that was playing, or nullptr.
 * @param frame Frame of the transition that was playing.
 * @param pPtr Write cursor.
 * @param subIndex Index of the content among the contents of the feature parameter.
 */
inline NOINLINE void RuntimeResAnimationBuilder::WriteContent_(const StateLayer& rStateLayer, const FeatureParameter& rParameter,
                                               const Transition* pTransition, const Transition* pPrevTransition,
                                               float frame, nn::util::BytePtr* pPtr, int subIndex) {
    auto* pContent = pPtr->Get<ResAnimationContent>();
    pPtr->Advance(sizeof(ResAnimationContent));
    if (rParameter.m_Kind == 26) {
        u32* pHeader = pPtr->Get<u32>();
        pHeader[0] = PartsStateLayerContentSize;
        pHeader[1] = 0x28;
        pPtr->Advance(8);
        *pPtr->Get<u32>() = 4;
        pPtr->Advance(4);
        const auto* pPartsStateLayer = static_cast<const ResStatePartsStateLayer*>(rParameter.m_pExtraResource);
        const char* pName;
        switch (subIndex) {
        case 0:
            pName = pPartsStateLayer->variableNames[0];
            break;
        case 1:
            pName = pPartsStateLayer->variableNames[1];
            break;
        case 2:
            pName = pPartsStateLayer->variableNames[2];
            break;
        case 3:
            pName = pPartsStateLayer->variableNames[3];
            break;
        default:
            pName = nullptr;
            break;
        }

        std::memcpy(pPtr->Get(), pName, 32);
        pPtr->Advance(32);
    }

    int trackIndex = -1;
    int index = 0;
    for (const auto& rOther : rStateLayer.m_FeatureParameters) {
        if (std::strcmp(rOther.m_pName, rParameter.m_pName) == 0 && rOther.m_Kind == rParameter.m_Kind) {
            trackIndex = index;
            break;
        }

        ++index;
    }

    const int infoCount = rParameter.m_AnimInfoCount;
    pContent->count = infoCount;
    pContent->type = rParameter.m_AnimContentType;
    nn::util::Strlcpy(pContent->name, rParameter.m_pName, sizeof(pContent->name));
    u32* pInfoOffsets = pPtr->Get<u32>();
    pPtr->Advance(infoCount * sizeof(u32));
    for (int i = 0; i < infoCount; ++i) {
        pInfoOffsets[i] = pPtr->Get<u8>() - reinterpret_cast<u8*>(pContent);
        auto* pInfo = pPtr->Get<ResAnimationInfo>();
        pPtr->Advance(8);
        const int targetCount = rParameter.m_pAnimInfos[i].targetCount;
        pInfo->count = targetCount;
        pInfo->kind = rParameter.m_pAnimInfos[i].signature;
        u32* pTargetOffsets = pPtr->Get<u32>();
        pPtr->Advance(targetCount * sizeof(u32));
        for (int j = 0; j < targetCount; ++j) {
            pTargetOffsets[j] = pPtr->Get<u8>() - reinterpret_cast<u8*>(pInfo);
            auto* pTarget = pPtr->Get<ResAnimationTarget>();
            pPtr->Advance(sizeof(ResAnimationTarget));
            pTarget->id = rParameter.m_TargetIndex;
            pTarget->target = rParameter.m_pAnimInfos[i].pTargets[j];
            pTarget->keyCount = 1;
            pTarget->curveType = 3;
            pTarget->keysOffset = pPtr->Get<u8>() - reinterpret_cast<u8*>(pTarget);
            const TransitionTimeLineTrack* pTrack = pTransition->m_pTimeline->GetTrack(trackIndex);
            if (pTrack == nullptr) {
                return;
            }

            WriteResParameterizedAnim_(rStateLayer, rParameter, static_cast<u16>(pTrack->keyCount) + 1,
                                       pPrevTransition, frame, pTransition, trackIndex, i,
                                       rParameter.m_Kind == 26 ? subIndex : j, pPtr);
        }
    }
}

/**
 * @brief Write the parameterized animation of one animation target.
 * @param rStateLayer Layer playing the transition.
 * @param rParameter Feature parameter animated by the target.
 * @param count Number of animation segments.
 * @param pPrevTransition Transition that was playing, or nullptr.
 * @param frame Frame of the transition that was playing.
 * @param pTransition Transition to play.
 * @param trackIndex Index of the timeline track of the feature parameter.
 * @param infoIndex Index of the animation info.
 * @param targetIndex Index of the target value.
 * @param pPtr Write cursor.
 */
inline NOINLINE void RuntimeResAnimationBuilder::WriteResParameterizedAnim_(const StateLayer& rStateLayer,
                                                            const FeatureParameter& rParameter, int count,
                                                            const Transition* pPrevTransition, float frame,
                                                            const Transition* pTransition, int trackIndex,
                                                            int infoIndex, int targetIndex,
                                                            nn::util::BytePtr* pPtr) {
    auto* pAnim = pPtr->Get<u16>();
    const int contentType = rParameter.m_AnimContentType;
    *pAnim = count;
    pPtr->Advance(sizeof(u32));
    u32* pOffsets = pPtr->Get<u32>();
    pPtr->Advance(*pAnim * sizeof(u32));
    auto* pFirst = pPtr->Get<ResParameterizedAnimParameter>();
    for (int i = 0; i < *pAnim; ++i) {
        pOffsets[i] = pPtr->Get<u8>() - reinterpret_cast<u8*>(pAnim);
        auto* pParameter = pPtr->Get<ResParameterizedAnimParameter>();
        pPtr->Advance(sizeof(ResParameterizedAnimParameter));
        if (contentType == 3) {
            auto* ppEvents = pPtr->Get<const void*>();
            pPtr->Advance(sizeof(void*) * 3);
            ppEvents[0] = nullptr;
            ppEvents[1] = nullptr;
            ppEvents[2] = nullptr;
            *reinterpret_cast<const void***>(&pParameter->value) = ppEvents;
            const TransitionTimeLineTrack* pTrack = pTransition->m_pTimeline->GetTrack(trackIndex);
            if (pTrack != nullptr) {
                pTrack->SetupParametrizedAnimationEvent(pParameter, i - 1);
            }

            continue;
        }

        const State* pFrom = rStateLayer.FindStateByName(pTransition->GetFromStateName());
        const State* pTo = rStateLayer.FindStateByName(pTransition->GetToStateName());
        if (pFrom == nullptr || pTo == nullptr) {
            continue;
        }

        const TransitionTimeLineTrack* pTrack = pTransition->m_pTimeline->GetTrack(trackIndex);
        pParameter->value.startValue =
            Advance(pFrom->m_StoreSet.m_Stores.begin(), trackIndex)->m_pValues[infoIndex].pValues[targetIndex];
        pParameter->value.targetValue =
            Advance(pTo->m_StoreSet.m_Stores.begin(), trackIndex)->m_pValues[infoIndex].pValues[targetIndex];
        if (pTrack != nullptr) {
            pTrack->SetupParametrizedAnimation(pParameter, targetIndex, i - 1);
        }
    }

    if (pPrevTransition == nullptr || contentType == 3) {
        return;
    }

    const State* pPrevTo = rStateLayer.FindStateByName(pPrevTransition->GetToStateName());
    if (pPrevTo == nullptr) {
        return;
    }

    ResParameterizedAnimParameter parameter = {};
    pPrevTransition->m_pTimeline->SetupParametrizedAnimationFromTime(&parameter, trackIndex, infoIndex, targetIndex,
                                                                     frame, rStateLayer.m_StoreSet,
                                                                     pPrevTo->m_StoreSet);
    const float value = GetParameterizedAnimValueAtFrameClamped(frame, &parameter);
    pFirst->value.startValue = value;
    int index = trackIndex;
    for (auto& rStore : const_cast<FeatureParameterStoreSet&>(rStateLayer.m_StoreSet).m_Stores) {
        if (index == 0) {
            rStore.m_pValues[infoIndex].pValues[targetIndex] = value;
        }

        --index;
    }
}

/**
 * @brief Set up the segment of a track that is active at a time of the timeline.
 * @param pParameter Segment to set up.
 * @param trackIndex Index of the track.
 * @param infoIndex Index of the animation info.
 * @param targetIndex Index of the target value.
 * @param time Time of the timeline.
 * @param rFrom Values at the start of the timeline.
 * @param rTo Values at the end of the timeline.
 */
inline NOINLINE void TransitionTimeLine::SetupParametrizedAnimationFromTime(ResParameterizedAnimParameter* pParameter,
                                                            int trackIndex, int infoIndex, int targetIndex,
                                                            float time, const FeatureParameterStoreSet& rFrom,
                                                            const FeatureParameterStoreSet& rTo) const {
    const TransitionTimeLineTrack* pTrack = GetTrack(trackIndex);
    const int keyIndex = pTrack != nullptr ? FindKeyIndex(pTrack, time) : -1;
    pParameter->value.startValue =
        Advance(rFrom.m_Stores.begin(), trackIndex)->m_pValues[infoIndex].pValues[targetIndex];
    pParameter->value.targetValue =
        Advance(rTo.m_Stores.begin(), trackIndex)->m_pValues[infoIndex].pValues[targetIndex];
    if (pTrack != nullptr) {
        pTrack->SetupParametrizedAnimation(pParameter, targetIndex, keyIndex);
    }
}

/**
 * @brief Set up an event segment of the track.
 * @param pParameter Segment to set up.
 * @param keyIndex Index of the key the segment starts at, or -1 for the segment before the first key.
 */
inline NOINLINE void TransitionTimeLineTrack::SetupParametrizedAnimationEvent(ResParameterizedAnimParameter* pParameter,
                                                              int keyIndex) const {
    if (keyCount == 0) {
        return;
    }

    pParameter->parameterizedAnimType = 0x10;
    if (keyIndex < 0) {
        pParameter->duration = pKeys[0].time - offset;
        pParameter->offset = offset;
        GetEvents(pParameter)[0] = nullptr;
        GetEvents(pParameter)[1] = nullptr;
        GetEvents(pParameter)[2] = nullptr;
        return;
    }

    if (keyIndex >= keyCount) {
        pParameter->duration = offset + duration - pKeys[keyIndex - 1].time;
        pParameter->offset = pKeys[keyIndex - 1].time;
        GetEvents(pParameter)[0] = nullptr;
        GetEvents(pParameter)[1] = nullptr;
        GetEvents(pParameter)[2] = nullptr;
        return;
    }

    const TransitionTimelineKey& rKey = pKeys[keyIndex];
    const TransitionTimelineKey* pNext = keyIndex + 1 < keyCount ? &pKeys[keyIndex + 1] : nullptr;
    const float end = pNext != nullptr ? pNext->time : offset + duration;
    pParameter->duration = end - rKey.time;
    pParameter->offset = rKey.time;
    GetEvents(pParameter)[0] = rKey.pCurve0;
    GetEvents(pParameter)[1] = rKey.pCurve1;
    GetEvents(pParameter)[2] = rKey.pCurve2;
    u8 type;
    switch (rKey.parameter1) {
    case 8:
        type = 0x10;
        break;
    case 9:
        type = 0x11;
        break;
    case 10:
        type = 0x12;
        break;
    default:
        NN_UNEXPECTED_DEFAULT;
    }

    pParameter->parameterizedAnimType = type;
}

/**
 * @brief Set up an easing segment of the track.
 * @param pParameter Segment to set up.
 * @param parameterIndex Index of the easing parameter of the keys.
 * @param keyIndex Index of the key the segment starts at, or -1 for the segment before the first key.
 */
inline NOINLINE void TransitionTimeLineTrack::SetupParametrizedAnimation(ResParameterizedAnimParameter* pParameter,
                                                         int parameterIndex, int keyIndex) const {
    if (keyCount == 0) {
        pParameter->duration = duration;
        pParameter->offset = offset;
        pParameter->parameterizedAnimType = easingType;
        return;
    }

    if (keyIndex < 0) {
        pParameter->duration = pKeys[0].time - offset;
        pParameter->offset = offset;
        pParameter->parameterizedAnimType = easingType;
        *reinterpret_cast<int*>(&pParameter->value.targetValue) = pKeys[0].GetParameter(parameterIndex);
        return;
    }

    if (keyIndex >= keyCount) {
        pParameter->duration = offset + duration - pKeys[keyIndex - 1].time;
        pParameter->offset = pKeys[keyIndex - 1].time;
        pParameter->parameterizedAnimType = pKeys[keyIndex - 1].easingType;
        *reinterpret_cast<int*>(&pParameter->value.startValue) = pKeys[keyIndex - 1].GetParameter(parameterIndex);
        return;
    }

    const TransitionTimelineKey& rKey = pKeys[keyIndex];
    const TransitionTimelineKey* pNext = keyIndex + 1 < keyCount ? &pKeys[keyIndex + 1] : nullptr;
    const float end = pNext != nullptr ? pNext->time : offset + duration;
    pParameter->duration = end - rKey.time;
    pParameter->offset = rKey.time;
    pParameter->parameterizedAnimType = rKey.easingType;
    *reinterpret_cast<int*>(&pParameter->value.startValue) = rKey.GetParameter(parameterIndex);
    if (pNext != nullptr) {
        *reinterpret_cast<int*>(&pParameter->value.targetValue) = pNext->GetParameter(parameterIndex);
    }
}

/**
 * @brief Change a variable, queue the change event and restart the calculated variables.
 * @param pName Name of the variable.
 * @param value New value; clamped to the range of the variable.
 * @return Whether the value changed.
 */
inline NOINLINE bool StateMachineVariableManager::SetFloatValue(const char* pName, float value) {
    StateMachineVariable* pVariable = FindByName_(pName);
    detail::ClampValue(value, pVariable->minimum, pVariable->maximum);
    const float previous = pVariable->value;
    if (value != previous) {
        pVariable->value = value;
        PushModifyEvent_(pVariable->name, pVariable);
        for (auto& rCalculated : pVariable->GetCalculatedVariables()) {
            DoUpdateCalcVarOnValueChanged_(&rCalculated, previous, value);
        }

        return true;
    }

    return false;
}
}  // namespace nn::ui2d
