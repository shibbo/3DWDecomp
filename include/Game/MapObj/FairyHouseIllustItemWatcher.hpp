#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class ListStampResult;
namespace rc { class StampDirector; }
class FairyHouseIllustItemWatcher : public al::LiveActor {
public:
    explicit FairyHouseIllustItemWatcher(const char*);
    ~FairyHouseIllustItemWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void setStampDirector(rc::StampDirector*);
    void exeWait();
    void exeShowLayout();
    bool isShowLayout() const;
private:
    ListStampResult* mLayout = nullptr;
    rc::StampDirector* mStampDirector = nullptr;
};
