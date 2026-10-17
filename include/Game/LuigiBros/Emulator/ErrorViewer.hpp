#pragma once

class ErrorViewer {
public:
    void Initialize();
    void Finalize();
    void Main();
    void Ev_Stat_Open_00();
    void Ev_Stat_Open_01();
    void Ev_Stat_Main_00();
    void Ev_Stat_Close_00();
    void Ev_Stat_Close_01();
    void DrawTV();
    void DrawDRC();
    void Open(int error, unsigned int flags);
    void Close();
    ErrorViewer();
    ~ErrorViewer();
    bool IsNoOperate();
    bool IsButton();
    bool IsLeftButton();
    bool IsRightButton();

private:
    volatile bool mInitialized;
    void* mpContext;
    unsigned char _10[0x10];
    bool mNoOperate;
    int mState;
    int mError;
    bool mButton;
    bool mLeftButton;
    bool mRightButton;
    unsigned char _2f[0x99];
};
static_assert(sizeof(ErrorViewer) == 0xc8, "Error viewer layout");
extern ErrorViewer g_ErrorViewer;
void chg_button_message(const void* pMessage, bool enabled, unsigned int button);
