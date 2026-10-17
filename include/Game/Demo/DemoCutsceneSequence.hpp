#pragma once

#include "Demo/DemoCutscene.hpp"

/** @brief Plays a linked sequence of cutscene actions and guide messages. */
class DemoCutsceneSequence : public DemoCutscene {
public:
    struct Action {
        /** @brief Creates a sequence entry. @param pName Animation name.
         * @param wait Whether to await a request. @param pMessage Guide message.
         * @param page Message page. @param skip Whether skipping is allowed.
         * @param count Advancement delay. @param keep Whether to continue the prior animation. */
        Action(const char* pName, bool wait, const char* pMessage, int page, bool skip,
               int count, bool keep)
        {
            message = pMessage;
            waitForRequest = wait;
            messagePage = page;
            waitCount = count;
            next = nullptr;
            name = pName;
            skippable = skip;
            continueAction = keep;
        }

        Action* next;
        const char* name;
        bool waitForRequest;
        const char* message;
        int messagePage;
        int waitCount;
        bool skippable;
        bool continueAction;
        int id;
    };
    DemoCutsceneSequence(const char* pName, alSeFunction::DemoType type);
    void addActionToSequence(const char* pName, bool waitForRequest, const char* pMessage,
                             int messagePage, bool skippable, int waitCount, bool continueAction);
    void tryStartNextActionWithMessage(const char* pMessage);
    void tryStartNextAction();
    void exePlay() override;
    Action* getUnskippableAction();
    void exeCancelDemo() override;
    void exeSkipToUnskippableAction();
    void startCurrentAction(bool cancelPrevious);
    bool isCurrentActionValid();
    bool isCurrentActionSkippable();
    int getCurrentActionID() const;
    void setFadeAtEnd(const al::ActorInitInfo& rInfo, int frames);
    void trueStartDemo() override;
    void startAction() override;
    void updateDemo() override;
    bool isCurrentActionDone() const;
    bool checkDemoEnd() const override;

private:
    Action* mActions = nullptr;
    Action* mCurrentAction = nullptr;
    Action* mPreviousAction = nullptr;
    int mStartDelay = 0;
    int mWaitCount = 0;
    int mActionCount = 0;
    bool mFadeAtEnd = false;
};

static_assert(sizeof(DemoCutsceneSequence) == 0x390);
