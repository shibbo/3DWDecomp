#pragma once
#include <nn/gfx/gfx_Types.h>
#include <nn/nn_SdkAssert.h>
#include <nn/types.h>
#include <nn/util/util_BytePtr.h>
#include <nn/util/util_IntrusiveList.h>
#include <nn/util/util_MathTypes.h>

#include <cstring>

namespace nn::ui2d {
class Group;
class Layout;
class Material;
class Pane;
struct ResParameterizedAnimParameter;
class FeatureParameterStoreSet;
class State;
class StateLayer;
class StateMachine;
class Transition;

struct ResStatePostCalculationActions;

/** @brief Calculated variable of a state machine variable resource. */
struct ResStateCalculatedVariables {
    u8 easingType;
    bool isEasingEnabled;
    bool isLinearScalingEnabled;
    bool isRangeLimitEnabled;
    u8 limitMode;
    u8 _05[3];
    float delay;
    float duration;
    float offset;
    float scale;
    float minimum;
    float maximum;
    const ResStatePostCalculationActions* pCalculation;
    u16 postCalculationActionCount;
    u8 _2A[6];
};
using ResStateCalculatedVariableEntry = ResStateCalculatedVariables;

/** @brief Action applied to a pane or a variable after a calculated variable changed. */
struct ResStatePostCalculationActions {
    char targetName[32];
    u8 kind;
    u8 condition;
    u8 _22[2];
};

/** @brief Variable of a state machine resource. */
struct ResStateVariableDescriptions {
    char name[36];
    float defaultValue;
    float minimum;
    float maximum;
    u8 _30[8];
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
    char variableNames[4][32];
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
    float offset;
    float duration;
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
    float duration;
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

/** @brief Type of a state machine variable. */
enum StateMachineVariableType : int {
    StateMachineVariableType_Bool = 0,
    StateMachineVariableType_Float = 1,
};

/** @brief Calculated variable derived from a state machine variable. */
struct StateMachineCalclatedVariable {
    nn::util::IntrusiveListNode m_Link;
    const ResStateCalculatedVariables* resource;
    Pane** ppTargetPanes;
    int targetPaneCount;
    float previousValue;
    float nextValue;
    float value;
    float elapsed;
    bool changed;
    u8 _35[3];
};

/** @brief Variable of a state machine. */
struct StateMachineVariable {
    using CalculatedVariableList =
        nn::util::IntrusiveList<StateMachineCalclatedVariable,
                                nn::util::IntrusiveListMemberNodeTraits<StateMachineCalclatedVariable,
                                                                        &StateMachineCalclatedVariable::m_Link>>;

    nn::util::IntrusiveListNode m_Link;
    char name[28];
    float defaultValue;
    float minimum;
    float maximum;
    u32 _38;
    u32 _3C;
    float value;
    u32 _44;
    StateMachineVariableType type = StateMachineVariableType_Bool;
    u32 _4C;
    nn::util::IntrusiveListNode calculatedVariables;

    /**
     * @brief Access the calculated variables derived from this variable.
     * @return Typed view of the calculated variable list.
     */
    CalculatedVariableList& GetCalculatedVariables() {
        return *reinterpret_cast<CalculatedVariableList*>(&calculatedVariables);
    }
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
    union {
        const void* _30;
        int parameter;
    };
};

/** @brief Fixed pool of events waiting to be processed by a state machine. */
class StateMachineEventQueue {
public:
    using EventList = nn::util::IntrusiveList<
        StateMachineEvent, nn::util::IntrusiveListMemberNodeTraits<StateMachineEvent, &StateMachineEvent::m_Link>>;

    /** @brief Construct a queue without event storage. */
    StateMachineEventQueue() : m_pEvents(nullptr) {}

    void Initialize();

    /**
     * @brief Move a free event to the end of the queue.
     * @param type Kind of the event.
     * @param pArgument0 First argument of the event.
     * @param pArgument1 Second argument of the event.
     * @param pArgument2 Third argument of the event.
     * @param parameter Integer argument of the event.
     */
    void Push(int type, const char* pArgument0, const void* pArgument1, const char* pArgument2, int parameter) {
        if (m_FreeEvents.empty()) {
            return;
        }

        StateMachineEvent& rEvent = m_FreeEvents.front();
        m_FreeEvents.pop_front();
        rEvent.type = type;
        rEvent.pArgument0 = pArgument0;
        rEvent.pArgument1 = pArgument1;
        rEvent.pArgument2 = pArgument2;
        rEvent.parameter = parameter;
        m_QueuedEvents.push_back(rEvent);
    }

    StateMachineEvent* m_pEvents;
    EventList m_QueuedEvents;
    EventList m_FreeEvents;
};

/** @brief Owns the variables of a state machine and the values derived from them. */
class StateMachineVariableManager {
public:
    using VariableList = nn::util::IntrusiveList<
        StateMachineVariable, nn::util::IntrusiveListMemberNodeTraits<StateMachineVariable, &StateMachineVariable::m_Link>>;

    /** @brief Construct a manager without variables. */
    StateMachineVariableManager() : m_pStateMachine(nullptr), m_pEventQueue(nullptr) {}

    void InitialzieVariable_(StateMachineVariable* pVariable, const char* pName, StateMachineVariableType type,
                             float defaultValue, float minimum, float maximum);
    void Finalize();
    bool RegisterNewVariableByResource(const ResStateVariableDescriptions* pResource);
    StateMachineVariable* DoRegisterNewVariable_(const char* pName, StateMachineVariableType type, float defaultValue,
                                                 float minimum, float maximum);
    void InitialzieStateMachineCalclatedVariable_(StateMachineCalclatedVariable* pVariable,
                                                  const ResStateCalculatedVariables* pResource);
    void DoCalculateCalcVar(float step);
    float DoCalculateCalcVar_(StateMachineCalclatedVariable* pVariable, float value, float step);
    bool CheckIsMatchPostActionCondition_(const ResStatePostCalculationActions* pAction, float previous,
                                          StateMachineCalclatedVariable* pVariable);
    const StateMachineVariable* FindRefOnlyByName_(const char* pName) const;
    bool ResetToDefalut_(StateMachineVariable* variable);
    float DoCalculateCalcVarEasing_(StateMachineCalclatedVariable* pVariable, float value, float step);
    float DoCalculateCalcVarLinearScaling_(const ResStateCalculatedVariables* resource, float value, float step);
    float DoCalculateCalcVarRangeLimit_(const ResStateCalculatedVariables* resource, float value, float step);
    void DoUpdateCalcVarOnValueChanged_(StateMachineCalclatedVariable* variable, float previous, float next);
    StateMachineVariable* FindByName_(const char* pName);
    void PushModifyEvent_(const char* pName, StateMachineVariable* pVariable);
    bool SetFloatValue(const char* pName, float value);

    /**
     * @brief Access the registered variables.
     * @return Typed view of the variable list.
     */
    VariableList& GetVariables() { return *reinterpret_cast<VariableList*>(&m_Variables); }

    /**
     * @brief Access the registered variables.
     * @return Typed view of the variable list.
     */
    const VariableList& GetVariables() const { return *reinterpret_cast<const VariableList*>(&m_Variables); }

    StateMachine* m_pStateMachine;
    StateMachineEventQueue* m_pEventQueue;
    nn::util::IntrusiveListNode m_Variables;
};

/** @brief Kind of a user interface event reported to a state machine. */
enum StateMachineUiEventKind : int {
    StateMachineUiEventKind_None = 0,
    StateMachineUiEventKind_Decided = 1,
};

/** @brief Receives the events of a state machine. */
class StateMachineEventHandler {
public:
    /**
     * @brief Create a handler for a state machine.
     * @param pStateMachine Owner of the handler.
     */
    explicit StateMachineEventHandler(StateMachine* pStateMachine) : m_pStateMachine(pStateMachine) {}
    /** @brief Destroy the handler. */
    virtual ~StateMachineEventHandler() {}

    virtual void OnStateChangeCompletedRaw(const char* pStateName, const char* pLayerName,
                                           const char* pTransitionName);
    virtual void OnStateChangeCompleted(const char* pPaneName, StateMachineUiEventKind kind);
    virtual void OnVariablesChanged(const char* pStateMachineName, const char* pVariableName, float previous,
                                    float current);

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
    void Finalize();

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

    /**
     * @brief Access one of the easing parameters of the key.
     * @param index Index of the parameter.
     * @return Parameter value.
     */
    int GetParameter(int index) const { return (&parameter0)[index]; }
};

/** @brief Track of a transition timeline: the easing keys of one feature parameter. */
class TransitionTimeLineTrack {
public:
    /** @brief Construct a track without keys. */
    TransitionTimeLineTrack()
        : offset(0), duration(0), easingType(0), easingExtra(0), keyCount(0), pKeys(nullptr) {}

    void SetupParametrizedAnimationEvent(ResParameterizedAnimParameter* pParameter, int keyIndex) const;
    void SetupParametrizedAnimation(ResParameterizedAnimParameter* pParameter, int parameterIndex,
                                    int keyIndex) const;

    float offset;
    float duration;
    u8 easingType;
    int easingExtra;
    int keyCount;
    TransitionTimelineKey* pKeys;
};
using TransitionTimelineTrack = TransitionTimeLineTrack;

/** @brief Timing of a transition. */
class TransitionTimeLine {
public:
    /** @brief Construct a timeline without tracks. */
    TransitionTimeLine() : duration(0), trackCount(0), pTracks(nullptr) {}

    void SetupParametrizedAnimationFromTime(ResParameterizedAnimParameter* pParameter, int trackIndex,
                                            int infoIndex, int targetIndex, float time,
                                            const FeatureParameterStoreSet& rFrom,
                                            const FeatureParameterStoreSet& rTo) const;

    /**
     * @brief Get a track of the timeline.
     * @param index Index of the track.
     * @return Track, or nullptr when the timeline has no track with that index.
     */
    const TransitionTimeLineTrack* GetTrack(int index) const {
        return index < trackCount ? &pTracks[index] : nullptr;
    }

    float duration;
    int trackCount;
    TransitionTimeLineTrack* pTracks;
};
using TransitionTimeline = TransitionTimeLine;

/**
 * @brief Transition between two states of a layer.
 *
 * The resource builder fills the members in resource order: m_pName holds the state the transition
 * starts from, m_pSourceStateName the state it ends in and m_pDestinationStateName the animation name.
 */
class Transition {
public:
    /** @brief Construct an unnamed, unlinked transition. */
    Transition()
        : m_pName(nullptr), m_pSourceStateName(nullptr), m_pDestinationStateName(nullptr), m_IsCancelable(false),
          m_IsLoop(false), m_IsEnabled(false), m_pCondition(nullptr), m_pTimeline(nullptr) {}

    void Initialzie(const char* pName, const char* pSourceStateName, const char* pDestinationStateName,
                    bool isCancelable, bool isLoop, bool isEnabled);
    void Finalize();

    /** @return Name of the state the transition starts from. */
    const char* GetFromStateName() const { return m_pName; }
    /** @return Name of the state the transition ends in. */
    const char* GetToStateName() const { return m_pSourceStateName; }
    /** @return Name of the animation played by the transition. */
    const char* GetAnimationName() const { return m_pDestinationStateName; }
    /** @return Whether the transition plays its animation backwards. */
    bool IsReverse() const { return m_IsEnabled != 0; }

    nn::util::IntrusiveListNode m_Link;
    const char* m_pName;
    const char* m_pSourceStateName;
    const char* m_pDestinationStateName;
    u8 m_IsCancelable;
    u8 m_IsLoop;
    u8 m_IsEnabled;
    TransitionCondition* m_pCondition;
    TransitionTimeline* m_pTimeline;
};

class Animator;

/** @brief Binds the animation generated by a state layer to its panes. */
class AnimatorSlot {
public:
    /** @brief Construct an unbound slot. */
    AnimatorSlot() : m_pGroup(nullptr), m_pLayout(nullptr), m_pResource(nullptr), m_pAnimator(nullptr) {}

    virtual Animator* ConstructAndInitialzieAnimator_();

    void Initialize(Layout* pLayout, const FeatureParameterList& rFeatureParameters);
    void Finalzie();
    void Bind(nn::gfx::Device* pDevice, const void* pResource);
    void* Unbind();

    Group* m_pGroup;
    Layout* m_pLayout;
    const void* m_pResource;
    Animator* m_pAnimator;
};

/** @brief Layer of a state machine: one active state of a set of states. */
class StateLayer {
public:
    using StateList = nn::util::IntrusiveList<State, nn::util::IntrusiveListMemberNodeTraits<State, &State::m_Link>>;
    using TransitionList =
        nn::util::IntrusiveList<Transition, nn::util::IntrusiveListMemberNodeTraits<Transition, &Transition::m_Link>>;

    /** @brief Kind of a layer. */
    enum Mode : int {
        Mode_Event = 0,
        Mode_StateByVariable = 1,
        Mode_FrameByVariable = 2,
    };

    /** @brief Construct an empty layer. */
    StateLayer()
        : m_pLayout(nullptr), m_pName(nullptr), m_pCurrentState(nullptr), m_pCurrentTransition(nullptr),
          m_RuntimeTransitionIndex(0), m_IsTransitionLocked(false), m_IsPlaying(false), m_IsPaused(false),
          m_HasPartsStateLayer(false), m_pTargetPane(nullptr), m_InitialStateIndex(0) {
        m_RuntimeTransitions[0].m_IsCancelable = 1;
        m_RuntimeTransitions[1].m_IsCancelable = 1;
    }

    void Initialize(nn::gfx::Device* pDevice, Layout* pLayout, const char* pName);
    void Finalize();
    void RestoreToInitialState();
    static size_t CalculateAnimationResourceSize(const StateLayer& rStateLayer, const Transition& rTransition);
    void UpdateStateLayerTransitions(nn::gfx::Device* pDevice, const StateMachineEvent& rEvent);
    void ChangeCurrentState_(nn::gfx::Device* pDevice, const Transition& rTransition);
    static void BuildAnimationResource(void* pBuffer, size_t size, const StateLayer& rStateLayer,
                                       const Transition* pTransition, const Transition* pPrevTransition,
                                       float frame);
    void ApplyFeatureParameterToPaneSrt_(Pane* pPane, const FeatureParameter& rParameter,
                                         const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToPaneVisilility_(Pane* pPane, const FeatureParameter& rParameter,
                                                const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToPaneTransparancy_(Pane* pPane, const FeatureParameter& rParameter,
                                                  const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToPaneRoundRect_(Pane* pPane, const FeatureParameter& rParameter,
                                               const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToPanePerCharacterTransform_(Pane* pPane, const FeatureParameter& rParameter,
                                                           const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToPaneMaskTexSRT_(Pane* pPane, const FeatureParameter& rParameter,
                                                const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToMaterialColor_(Material* pMaterial, const FeatureParameter& rParameter,
                                               const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToTextureMatrix_(Material* pMaterial, const FeatureParameter& rParameter,
                                               const FeatureParameterStore& rStore, int index);
    void ApplyFeatureParameterToTarget_(const FeatureParameter& rParameter, const FeatureParameterStore& rStore);
    void ApplyFeatureParameterToTargetAll_(const State* pState);
    void BindSlot_(nn::gfx::Device* pDevice, const Transition* pTransition, const Transition* pPrevTransition,
                   float frame);

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

    /**
     * @brief Find a state of the layer by its name.
     * @param pName Name of the state.
     * @return State, or nullptr when the layer has no state with that name.
     */
    const State* FindStateByName(const char* pName) const {
        for (auto& rState : m_States) {
            if (std::strcmp(rState.m_pName, pName) == 0) {
                return &rState;
            }
        }

        return nullptr;
    }

    /**
     * @brief Get a state of the layer by its position.
     * @param index Position of the state.
     * @return State, or nullptr when the layer has fewer states.
     */
    State* GetStateByIndex(int index) {
        int i = 0;
        for (auto& rState : m_States) {
            if (i == index) {
                return &rState;
            }

            ++i;
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
    union {
        const Transition* m_pCurrentTransition;
        /** @brief Older view of m_pCurrentTransition. */
        struct {
            const Transition* m_pTransition;
        } m_TransitionPlayers[1];
    };
    Transition m_RuntimeTransitions[2];
    int m_RuntimeTransitionIndex;
    bool m_IsTransitionLocked;
    bool m_IsPlaying;
    bool m_IsPaused;
    bool m_HasPartsStateLayer;
    Pane* m_pTargetPane;
    /** @brief Hit box of the target pane, updated every frame. */
    nn::util::Float2 m_HitBoxMin;
    nn::util::Float2 m_HitBoxMax;
    int m_InitialStateIndex;
    FeatureParameterStoreSet m_StoreSet;
};

/** @brief Variable forwarded from a state machine to the state machine of a parts pane. */
struct ResStateVariableBinding {
    char sourceName[64];
    char targetName[32];
    char variableName[32];
};

/** @brief State machine of a layout. */
class StateMachine {
public:
    using StateLayerList =
        nn::util::IntrusiveList<StateLayer, nn::util::IntrusiveListMemberNodeTraits<StateLayer, &StateLayer::m_Link>>;

    static constexpr int DecideButtonCountMax = 8;

    /** @brief Construct an empty state machine. */
    StateMachine()
        : m_pLayout(nullptr), m_pName(nullptr), m_pDecideStateLayer(nullptr), m_IsDecideButton(false),
          m_DecideButtonCount(0), _C0(nullptr), _C8(0), m_pEventHandler(nullptr), m_pListener(nullptr), _E0(false) {}

    void Finalize();
    void SetFloatValue(const char* pName, float value);
    void ApplayPostCalcActionToPane(Pane* pPane, u8 kind, float value);
    void SetupDecideButtons_();
    void FirstTimeSetup(nn::gfx::Device* pDevice, StateMachine* pParent);
    void Update(nn::gfx::Device* pDevice, float step, StateMachine* pParent);
    void UpdateUserInput(const nn::util::Float2* pPosition, bool isDown, bool isRelease);
    void UpdateStateLayer_(StateLayer* pStateLayer, StateMachine* pParent);

    Layout* m_pLayout;
    const char* m_pName;
    StateLayerList m_StateLayers;
    StateLayer* m_pDecideStateLayer;
    bool m_IsDecideButton;
    Pane* m_pDecideButtons[DecideButtonCountMax];
    int m_DecideButtonCount;
    StateMachineEventQueue m_EventQueue;
    StateMachineVariableManager m_VariableManager;
    union {
        const void* _C0;
        const ResStateVariableBinding* m_pVariableBindings;
    };
    union {
        int _C8;
        int m_VariableBindingCount;
    };
    StateMachineEventHandler* m_pEventHandler;
    StateMachineListener* m_pListener;
    bool _E0;
};

/** @brief Writes the animation resource that plays a transition from the current values of a layer. */
class RuntimeResAnimationBuilder {
public:

    size_t CalcAnimationBlockSize_(const StateLayer& rStateLayer, const Transition& rTransition);
    void BuildFromCurrent(void* pBuffer, size_t size, const StateLayer& rStateLayer, const Transition* pTransition,
                          const Transition* pPrevTransition, float frame);
    void WriteContent_(const StateLayer& rStateLayer, const FeatureParameter& rParameter,
                       const Transition* pTransition, const Transition* pPrevTransition, float frame,
                       nn::util::BytePtr* pPtr, int subIndex);
    void WriteResParameterizedAnim_(const StateLayer& rStateLayer, const FeatureParameter& rParameter, int count,
                                    const Transition* pPrevTransition, float frame, const Transition* pTransition,
                                    int trackIndex, int infoIndex, int targetIndex, nn::util::BytePtr* pPtr);
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
