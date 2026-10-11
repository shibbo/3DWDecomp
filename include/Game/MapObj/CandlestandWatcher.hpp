#pragma once
#include "Library/LiveActor/LiveActor.hpp"
#include "Library/LiveActor/Common/LiveActorGroup.hpp"
class Candlestand;
class CandlestandWatcher : public al::LiveActor {
public:
    explicit CandlestandWatcher(const char*);
    ~CandlestandWatcher() override;
    void init(const al::ActorInitInfo&) override;
    void exeWatch();
    bool isEnableAddScore();
private:
    al::DeriveActorGroup<Candlestand>* mCandlestands = nullptr;
};
