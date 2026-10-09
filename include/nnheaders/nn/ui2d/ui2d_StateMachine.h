#pragma once
#include <nn/gfx/gfx_Types.h>
#include <nn/nn_SdkAssert.h>
#include <nn/types.h>
#include <nn/util/util_IntrusiveList.h>

#include <cstring>

namespace nn::ui2d {
class Layout;
class Pane;
class State;
class StateLayer;
class StateMachine;
class Transition;

struct ResStateCalculatedVariables;

/** @brief Calculated variable entry of a state machine variable resource. */
struct ResStateCalculatedVariableEntry {
    u8 _00[0x20];
    const ResStateCalculatedVariables* pCalculation;
    u8 _28[8];
};

/** @brief Variable of a state machine resource. */
struct ResStateVariableDescriptions {
    u8 _00[0x38];
    ResStateCalculatedVariableEntry* pCalculatedVariables;
    u8 _40[4];
    u16 calculatedVariableCount;
    u16 _46;
};

/** @brief Feature parameter value of a state resource. */
struct ResStateFeatureParameter {
    char name[32];
    int kind;
    float values[4];
    u8 _34[4];
};

/** @brief Parts state layer reference of a state resource. */
struct ResStatePartsStateLayer {
    u8 _00[0x80];
};

/** @brief State of a state layer resource. */
struct ResState {
    char name[32];
    ResStateFeatureParameter* pFeatureParameters;
    ResStatePartsStateLayer* pPartsStateLayers;
    u16 featureParameterCount;
    u16 _32[3];
};

/** @brief Key of a transition track resource. */
struct ResStateTransitionKey {
    float time;
    int easingExtra;
    u8 easingType;
    u8 _09[0x27];
    int parameters[4];
    u8 _40[4];
    int curveParameter;
    u8 curveType;
    u8 _49[3];
    u8 curve0[32];
    u8 curve1[32];
    u8 curve2[32];
};

/** @brief Track of a transition resource. */
struct ResStateTransitionTrack {
    u8 _00[0x20];
    int offset;
    int duration;
    int easingExtra;
    u8 easingType;
    u8 _2D[3];
    ResStateTransitionKey* pKeys;
    u16 keyCount;
    u16 _3A[3];
};

/** @brief Data of a transition trigger resource. */
struct ResStateTransitionTriggerData {
    char name[36];
    float value;
    u8 _28[0x10];
    char layerName[56];
    char transitionName[56];
};

/** @brief Transition of a state layer resource. */
struct ResStateTransition {
    u8 _00[8];
    int duration;
    u8 _0C[4];
    u8 isCancelable;
    u8 isLoop;
    u8 _12;
    u8 isEnabled;
    u8 _14[4];
    char name[32];
    char sourceStateName[32];
    char destinationStateName[32];
    ResStateTransitionTrack* pTracks;
    u16 trackCount;
    u8 _82[6];
    int triggerKind;
    int compareOp;
    ResStateTransitionTriggerData* pTriggerData;
    u8 _98[8];
};

/** @brief Layer of a state machine resource. */
struct ResStateLayer {
    char name[64];
    char targetPaneName[32];
    u8 initialStateIndex;
    u8 _61[7];
    ResState* pStates;
    ResStateTransition* pTransitions;
    u16 stateCount;
    u16 transitionCount;
    u8 _7C[4];
};

/** @brief State machine block of a layout resource. */
struct ResStateMachine {
    u32 signature;
    u32 blockSize;
    char name[32];
    ResStateLayer* pLayers;
    ResStateVariableDescriptions* pVariables;
    const void* _38;
    u16 layerCount;
    u16 variableCount;
    u16 _44;
    u16 isRelocated;
};

struct ResStateCalculatedVariables {
    u8 easingType;
    u8 _01[3];
    u8 limitMode;
    u8 _05[3];
    float duration, delay, offset, scale, minimum, maximum;
};
struct StateMachineVariable {
    nn::util::IntrusiveListNode m_Link;
    char name[0x1c];
    float defaultValue;
    float minimum;
    float maximum;
    u8 _38[8];
    float value;
    u8 _44[0xc];
    nn::util::IntrusiveListNode calculatedVariables;
};
struct StateMachineCalclatedVariable {
    u8 _00[0x10];
    const ResStateCalculatedVariables* resource;
    u8 _18[0xc];
    float previousValue, nextValue;
    u32 _2C;
    float elapsed;
    bool changed;
};

/** @brief Event processed by the transitions of a state machine. */
struct StateMachineEvent {
    /** @brief Construct an empty, unlinked event. */
    StateMachineEvent()
        : type(0), _14(0), pArgument0(nullptr), pArgument1(nullptr), pArgument2(nullptr), _30(nullptr) {}

    nn::util::IntrusiveListNode m_Link;
    int type;
    int _14;
    const char* pArgument0;
    const void* pArgument1;
    const char* pArgument2;
    const void* _30;
};

/** @brief Fixed pool of events waiting to be processed by a state machine. */
class StateMachineEventQueue {
public:
    using EventList = nn::util::IntrusiveList<
        StateMachineEvent, nn::util::IntrusiveListMemberNodeTraits<StateMachineEvent, &StateMachineEvent::m_Link>>;

    /** @brief Construct a queue without event storage. */
    StateMachineEventQueue() : m_pEvents(nullptr) {}

    void Initialize();

    StateMachineEvent* m_pEvents;
    EventList m_QueuedEvents;
    EventList m_FreeEvents;
};

class StateMachineVariableManager {
public:
    /** @brief Construct a manager without variables. */
    StateMachineVariableManager() : m_pStateMachine(nullptr), m_pEventQueue(nullptr) {}

    bool ResetToDefalut_(StateMachineVariable* variable);
    float DoCalculateCalcVarLinearScaling_(const ResStateCalculatedVariables* resource, float value, float step);
    float DoCalculateCalcVarRangeLimit_(const ResStateCalculatedVariables* resource, float value, float step);
    void DoUpdateCalcVarOnValueChanged_(StateMachineCalclatedVariable* variable, float previous, float next);
    void RegisterNewVariableByResource(const ResStateVariableDescriptions* pResource);
    StateMachineVariable* FindByName_(const char* pName);
    const StateMachineVariable* FindRefOnlyByName_(const char* pName) const;
    void PushModifyEvent_(const char* pName, StateMachineVariable* pVariable);

    StateMachine* m_pStateMachine;
    StateMachineEventQueue* m_pEventQueue;
    nn::util::IntrusiveListNode m_Variables;
};

/** @brief Receives the events of a state machine. */
class StateMachineEventHandler {
public:
    /**
     * @brief Create a handler for a state machine.
     * @param pStateMachine Owner of the handler.
     */
    explicit StateMachineEventHandler(StateMachine* pStateMachine) : m_pStateMachine(pStateMachine) {}
    virtual ~StateMachineEventHandler();

    StateMachine* m_pStateMachine;
};

/** @brief Observer notified when a state machine changes. */
class StateMachineListener {
public:
    virtual ~StateMachineListener();
    virtual void OnStateChanged(const char* pStateMachineName, const char* pLayerName);
    virtual void OnTransitionStarted(const char* pStateMachineName, const char* pLayerName);
    virtual void OnVariableChanged(const char* pStateMachineName, const char* pVariableName, float previous,
                                   float current);
};

/** @brief Stored value of an animated feature of a state. */
struct FeatureParameterValue {
    /** @brief Construct an empty value. */
    FeatureParameterValue() : count(0), pValues(nullptr) {}

    int count;
    float* pValues;
};

/** @brief Values a state assigns to one feature parameter. */
struct FeatureParameterStore {
    nn::util::IntrusiveListNode m_Link;
    int m_Count;
    FeatureParameterValue* m_pValues;
};

/** @brief Animation content a feature parameter drives. */
struct FeatureParameterAnimInfo {
    u32 signature;
    int targetCount;
    u8* pTargets;
};

enum StateMachineFeatureParameterKind : int {};
enum AnimContentType : int {};

/** @brief Pane or material property a state layer animates. */
class FeatureParameter {
public:
    /** @brief Construct an empty, unlinked parameter. */
    FeatureParameter()
        : m_pName(nullptr), m_Kind(), m_TargetIndex(0), m_AnimContentType(0), m_AnimInfoCount(0),
          m_pAnimInfos(nullptr), m_pExtraResource(nullptr) {}

    void Initialzie(const char* pName, StateMachineFeatureParameterKind kind, int targetIndex,
                    AnimContentType animContentType, u32 signature, int targetCount, const u8* pTargets);

    nn::util::IntrusiveListNode m_Link;
    const char* m_pName;
    StateMachineFeatureParameterKind m_Kind;
    u8 m_TargetIndex;
    u8 m_AnimContentType;
    u16 _1E;
    int m_AnimInfoCount;
    FeatureParameterAnimInfo* m_pAnimInfos;
    const void* m_pExtraResource;
};
using FeatureParameterList =
    nn::util::IntrusiveList<FeatureParameter,
                            nn::util::IntrusiveListMemberNodeTraits<FeatureParameter, &FeatureParameter::m_Link>>;

/** @brief Values stored for every feature parameter of a state. */
class FeatureParameterStoreSet {
public:
    using StoreList = nn::util::IntrusiveList<
        FeatureParameterStore, nn::util::IntrusiveListMemberNodeTraits<FeatureParameterStore,
                                                                       &FeatureParameterStore::m_Link>>;

    void Initialzie(const FeatureParameterList& rFeatureParameters);

    StoreList m_Stores;
};

/** @brief Named set of feature parameter values. */
class State {
public:
    /** @brief Construct an unnamed, unlinked state. */
    State() : m_pName(nullptr) {}

    void Initialzie(const char* pName, const FeatureParameterList& rFeatureParameters);

    nn::util::IntrusiveListNode m_Link;
    const char* m_pName;
    FeatureParameterStoreSet m_StoreSet;
};

/** @brief Condition that starts a transition. */
class TransitionCondition {
public:
    /** @brief Construct a condition of the default kind. */
    TransitionCondition() : m_Type(0) {}
    /** @brief Destroy the condition. */
    virtual ~TransitionCondition() {}
    /**
     * @brief Check whether an event starts the transition.
     * @param rEvent Event being processed.
     * @return True when the transition starts.
     */
    virtual bool IsTriggered(const StateMachineEvent& rEvent) const = 0;

    u8 m_Type;
};

/** @brief Condition that is always met. */
class TransitionConditionNone : public TransitionCondition {
public:
    /** @brief Construct the condition. */
    TransitionConditionNone() {}
    /** @brief Destroy the condition. */
    ~TransitionConditionNone() override { NN_SDK_ASSERT(m_Type == 0); }
    /**
     * @brief Accept every event.
     * @param rEvent Unused event.
     * @return Always true.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override { return true; }
};

/** @brief Condition met when the layer's pane is hit. */
class TransitionConditionIsHit : public TransitionCondition {
public:
    /** @brief Destroy the condition. */
    ~TransitionConditionIsHit() override { NN_SDK_ASSERT(m_pTargetName != nullptr); }
    /** @brief Construct a condition without a target. */
    TransitionConditionIsHit() : m_pTargetName(nullptr) {}
    /**
     * @brief Check for a hit event on the target.
     * @param rEvent Event being processed.
     * @return True for a hit of the target.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override {
        if (rEvent.type != 2) {
            return false;
        }

        if (std::strcmp(m_pTargetName, rEvent.pArgument0) != 0) {
            return false;
        }

        return true;
    }

    const char* m_pTargetName;
};

/** @brief Condition met when the layer's pane stops being hit. */
class TransitionConditionIsNoHit : public TransitionCondition {
public:
    /** @brief Destroy the condition. */
    ~TransitionConditionIsNoHit() override { NN_SDK_ASSERT(m_pTargetName != nullptr); }
    /** @brief Construct a condition without a target. */
    TransitionConditionIsNoHit() : m_pTargetName(nullptr) {}
    /**
     * @brief Check for a no-hit event on the target.
     * @param rEvent Event being processed.
     * @return True for a no-hit of the target.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override {
        if (rEvent.type != 3) {
            return false;
        }

        if (std::strcmp(m_pTargetName, rEvent.pArgument0) != 0) {
            return false;
        }

        return true;
    }

    const char* m_pTargetName;
};

/** @brief Condition met by a decide event. */
class TransitionConditionIsDecided : public TransitionCondition {
public:
    /** @brief Construct the condition. */
    TransitionConditionIsDecided() {}
    /** @brief Destroy the condition. */
    ~TransitionConditionIsDecided() override { NN_SDK_ASSERT(m_Type == 0); }
    /**
     * @brief Check for a decide event.
     * @param rEvent Event being processed.
     * @return True for a decide event.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override { return rEvent.type == 4; }
};

/** @brief Condition met when another transition completes. */
class TransitionConditionIsStateTransitionCompleted : public TransitionCondition {
public:
    /** @brief Destroy the condition. */
    ~TransitionConditionIsStateTransitionCompleted() override { NN_SDK_ASSERT(m_pLayerName != nullptr); }
    /** @brief Construct a condition without a target. */
    TransitionConditionIsStateTransitionCompleted()
        : m_pLayerName(nullptr), m_pTransitionName(nullptr), m_pStateName(nullptr) {}
    /**
     * @brief Check for a completion event of the target transition.
     * @param rEvent Event being processed.
     * @return True when the target transition completed.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override {
        if (rEvent.type == 5) {
            if (std::strcmp(m_pLayerName, rEvent.pArgument0) != 0) {
                return false;
            }

            if (std::strcmp(m_pTransitionName, static_cast<const char*>(rEvent.pArgument1)) != 0) {
                return false;
            }

            return true;
        }

        if (rEvent.type == 6) {
            if (std::strcmp(m_pLayerName, rEvent.pArgument0) != 0) {
                return false;
            }

            if (std::strcmp(m_pTransitionName, static_cast<const char*>(rEvent.pArgument1)) != 0) {
                return false;
            }

            if (std::strcmp(m_pStateName, rEvent.pArgument2) != 0) {
                return false;
            }

            return true;
        }

        return false;
    }

    const char* m_pLayerName;
    const char* m_pTransitionName;
    const char* m_pStateName;
};

/** @brief Comparison of a variable condition. */
enum TransitionVariableCompareOp : int {
    TransitionVariableCompareOp_Greater = 0,
    TransitionVariableCompareOp_Less = 5,
};

/** @brief Condition met when a variable changes to a value satisfying a comparison. */
class TransitionConditionVariableChanged : public TransitionCondition {
public:
    /** @brief Destroy the condition. */
    ~TransitionConditionVariableChanged() override { NN_SDK_ASSERT(m_VariableName[0] != '\0'); }
    /** @brief Construct a condition comparing against zero. */
    TransitionConditionVariableChanged() : m_CompareOp(0), m_Value(0), _2C(0), m_ValueType(0) {}
    /**
     * @brief Check a variable change against the comparison.
     * @param rEvent Event being processed.
     * @return True when the changed variable satisfies the comparison.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override;

    /**
     * @brief Compare against a float value.
     * @param value Value the variable is compared with.
     */
    void SetFloatValue(float value) {
        m_ValueType = 1;
        if (m_Value != value) {
            m_Value = value;
        }
    }

    char m_VariableName[25];
    int m_CompareOp;
    union {
        float m_Value;
        bool m_BoolValue;
    };
    u32 _2C;
    int m_ValueType;
};

/** @brief Condition met by a request to change to a specific state. */
class TransitionConditionStateChangeRequested : public TransitionCondition {
public:
    /** @brief Destroy the condition. */
    ~TransitionConditionStateChangeRequested() override { NN_SDK_ASSERT(m_pLayerName != nullptr); }
    /** @brief Construct a condition without a target. */
    TransitionConditionStateChangeRequested() {}
    /**
     * @brief Check for a change request to the target state.
     * @param rEvent Event being processed.
     * @return True for a request to change the layer to the target state.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override {
        if (rEvent.type != 8) {
            return false;
        }

        if (std::strcmp(static_cast<const char*>(rEvent.pArgument1), m_pLayerName) != 0) {
            return false;
        }

        return std::strcmp(rEvent.pArgument2, m_pStateName) == 0;
    }

    const char* m_pLayerName;
    const char* m_pStateName;
};

/** @brief Condition met by a button decide event. */
class TransitionConditionIsButtonDecided : public TransitionCondition {
public:
    /** @brief Construct the condition. */
    TransitionConditionIsButtonDecided() {}
    /** @brief Destroy the condition. */
    ~TransitionConditionIsButtonDecided() override { NN_SDK_ASSERT(m_Type == 0); }
    /**
     * @brief Check for a button decide event.
     * @param rEvent Event being processed.
     * @return True for a button decide event.
     */
    bool IsTriggered(const StateMachineEvent& rEvent) const override { return rEvent.type == 11; }
};

/** @brief Key of a transition timeline track. */
struct TransitionTimelineKey {
    /** @brief Construct a key at time zero. */
    TransitionTimelineKey() : time(0), scale(0), easingType(0), easingExtra(0), isCurve(false) {}

    float time;
    float scale;
    u8 easingType;
    int easingExtra;
    int parameter0;
    int parameter1;
    union {
        struct {
            int parameter2;
            int parameter3;
        };
        const void* pCurve0;
    };
    const void* pCurve1;
    const void* pCurve2;
    bool isCurve;
};

/** @brief Track of a transition timeline. */
struct TransitionTimelineTrack {
    /** @brief Construct a track without keys. */
    TransitionTimelineTrack()
        : offset(0), duration(0), easingType(0), easingExtra(0), keyCount(0), pKeys(nullptr) {}

    int offset;
    int duration;
    u8 easingType;
    int easingExtra;
    int keyCount;
    TransitionTimelineKey* pKeys;
};

/** @brief Timing of a transition. */
struct TransitionTimeline {
    /** @brief Construct a timeline without tracks. */
    TransitionTimeline() : duration(0), trackCount(0), pTracks(nullptr) {}

    int duration;
    int trackCount;
    TransitionTimelineTrack* pTracks;
};

/** @brief Transition between two states of a layer. */
class Transition {
public:
    /** @brief Construct an unnamed, unlinked transition. */
    Transition()
        : m_pName(nullptr), m_pSourceStateName(nullptr), m_pDestinationStateName(nullptr), m_IsCancelable(false), m_IsLoop(false), m_IsEnabled(false), m_pCondition(nullptr), m_pTimeline(nullptr) {}

    void Initialzie(const char* pName, const char* pSourceStateName, const char* pDestinationStateName,
                    bool isCancelable, bool isLoop, bool isEnabled);

    nn::util::IntrusiveListNode m_Link;
    const char* m_pName;
    const char* m_pSourceStateName;
    const char* m_pDestinationStateName;
    bool m_IsCancelable;
    bool m_IsLoop;
    bool m_IsEnabled;
    TransitionCondition* m_pCondition;
    TransitionTimeline* m_pTimeline;
};

/** @brief Binds the animation generated by a state layer to its panes. */
class AnimatorSlot {
public:
    /** @brief Construct an unbound slot. */
    AnimatorSlot() : _08(nullptr), _10(nullptr), _18(nullptr), _20(nullptr) {}
    virtual ~AnimatorSlot();

    void Initialize(Layout* pLayout, const FeatureParameterList& rFeatureParameters);
    void* Unbind();

    void* _08;
    void* _10;
    void* _18;
    void* _20;
};

/** @brief Playback state of a transition. */
struct TransitionPlayer {
    /** @brief Construct an idle player. */
    TransitionPlayer()
        : m_pTransition(nullptr), _18(nullptr), _20(nullptr), _28(nullptr), m_IsCompleted(true), _31(false),
          _32(false), _38(nullptr) {}

    using TransitionList =
        nn::util::IntrusiveList<Transition, nn::util::IntrusiveListMemberNodeTraits<Transition, &Transition::m_Link>>;

    Transition* m_pTransition;
    TransitionList m_Transitions;
    void* _18;
    void* _20;
    void* _28;
    bool m_IsCompleted;
    bool _31;
    bool _32;
    void* _38;
};

/** @brief Layer of a state machine: one active state of a set of states. */
class StateLayer {
public:
    using StateList = nn::util::IntrusiveList<State, nn::util::IntrusiveListMemberNodeTraits<State, &State::m_Link>>;
    using TransitionList =
        nn::util::IntrusiveList<Transition, nn::util::IntrusiveListMemberNodeTraits<Transition, &Transition::m_Link>>;

    /** @brief Construct an empty layer. */
    StateLayer()
        : m_pLayout(nullptr), m_pName(nullptr), m_pCurrentState(nullptr), _100(nullptr), _108(0), _10C(false),
          m_IsPlaying(false), m_IsPaused(false), m_HasPartsStateLayer(false), m_pTargetPane(nullptr),
          m_InitialStateIndex(0) {}

    void Initialize(nn::gfx::Device* pDevice, Layout* pLayout, const char* pName);
    void RestoreToInitialState();
    void ApplyFeatureParameterToTargetAll_(const State* pState);
    /**
     * @brief Find a state of the layer by its name.
     * @param pName Name of the state.
     * @return State, or nullptr when the layer has no state with that name.
     */
    State* FindStateByName(const char* pName) {
        for (auto& rState : m_States) {
            if (std::strcmp(rState.m_pName, pName) == 0) {
                return &rState;
            }
        }

        return nullptr;
    }

    nn::util::IntrusiveListNode m_Link;
    Layout* m_pLayout;
    const char* m_pName;
    State* m_pCurrentState;
    StateList m_States;
    FeatureParameterList m_FeatureParameters;
    AnimatorSlot m_AnimatorSlot;
    TransitionList m_Transitions;
    TransitionPlayer m_TransitionPlayers[2];
    void* _100;
    int _108;
    bool _10C;
    bool m_IsPlaying;
    bool m_IsPaused;
    bool m_HasPartsStateLayer;
    Pane* m_pTargetPane;
    u8 _118[0x10];
    int m_InitialStateIndex;
    FeatureParameterStoreSet m_StoreSet;
};

/** @brief State machine of a layout. */
class StateMachine {
public:
    using StateLayerList =
        nn::util::IntrusiveList<StateLayer, nn::util::IntrusiveListMemberNodeTraits<StateLayer, &StateLayer::m_Link>>;

    /** @brief Construct an empty state machine. */
    StateMachine()
        : m_pLayout(nullptr), m_pName(nullptr), _20(nullptr), _28(false), _70(0), _C0(nullptr), _C8(0),
          m_pEventHandler(nullptr), m_pListener(nullptr), _E0(false) {}

    Layout* m_pLayout;
    const char* m_pName;
    StateLayerList m_StateLayers;
    void* _20;
    bool _28;
    u8 _29[0x47];
    int _70;
    StateMachineEventQueue m_EventQueue;
    StateMachineVariableManager m_VariableManager;
    const void* _C0;
    int _C8;
    StateMachineEventHandler* m_pEventHandler;
    StateMachineListener* m_pListener;
    bool _E0;
};

/** @brief Builds state machines from their layout resource. */
class StateMachineFactory {
public:
    /**
     * @brief Create a factory for the state machines of a layout.
     * @param pDevice Graphics device used by the state machines.
     * @param pLayout Layout owning the state machines.
     */
    StateMachineFactory(nn::gfx::Device* pDevice, Layout* pLayout) : m_pDevice(pDevice), m_pLayout(pLayout) {}

    void Build(StateMachine* pStateMachine, void* pResource);
    void DoRelocate_(ResStateMachine* pResource, void* pBase);
    StateLayer* BuildStateLayer_(ResStateLayer* pResource);
    void BuildFeatureParameters_(StateLayer* pStateLayer, ResStateLayer* pResource);
    void BuildStates_(StateLayer* pStateLayer, ResStateLayer* pResource);
    void BuildTransitions_(StateLayer* pStateLayer, ResStateLayer* pResource);
    void BuildTransitionTrigger_(Transition* pTransition, ResStateLayer* pResource,
                                 ResStateTransition* pTransitionResource);

    nn::gfx::Device* m_pDevice;
    Layout* m_pLayout;
};
}  // namespace nn::ui2d
