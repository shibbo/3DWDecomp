#pragma once
namespace al { class LayoutInitInfo; class WipeSimple; }
class CourseSelectScene;
class WindowConfirm;
class WindowMessage;

class CourseSelectWindowHolder {
public:
    explicit CourseSelectWindowHolder(const CourseSelectScene* pScene);
    virtual void init(const al::LayoutInitInfo& rInfo);
    void startFadeWhite(int frames);
    void endFadeWhite(int frames);
    bool isCloseFadeWhite() const;
    bool isActiveFadeWhite() const;
    void startFadeBlack(int frames);
    void endFadeBlack(int frames);
    bool isCloseFadeBlack() const;
    void appearConfirmReport(const char* pMessage);
    void appearConfirmOneBtn(const char* pMessage, const char* pLayout);
    void appearConfirmTwoBtn(const char* pMessage, bool useRButton);
    void appearMessage(const char* pMessage);
    bool isConfirmActive() const;
    bool isConfirmDecideOne() const;
    bool isConfirmDecideLeft() const;
    bool isConfirmDecideRight() const;
    bool isMessageActive() const;
private:
    const CourseSelectScene* mpScene;
    WindowConfirm* mpReport = nullptr;
    WindowConfirm* mpOneButton = nullptr;
    WindowConfirm* mpTwoButton = nullptr;
    WindowMessage* mpMessage = nullptr;
    al::WipeSimple* mpWhiteFade = nullptr;
    al::WipeSimple* mpBlackFade = nullptr;
};
static_assert(sizeof(CourseSelectWindowHolder) == 0x40);
