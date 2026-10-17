#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class KoopaSignBoard : public al::LiveActor {
public:
    explicit KoopaSignBoard(const char*);
    ~KoopaSignBoard() override;
    void init(const al::ActorInitInfo&) override;
    void attackSensor(al::HitSensor*, al::HitSensor*) override;
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    bool isTouchFront(const al::HitSensor*) const;
    void setScoreSensor(const al::SensorMsg*, al::HitSensor*);
    void addScoreToSensor();
    void exeWait();
    void exeBreakFront();
    void exeBreakBack();
    void exeBreakSignBack();
    void exeBreakSignFront();
    void exeBreakAttacked();
    void exeBreakEndFront();
    void exeBreakEndBack();
    void exeBreakEnd();
private:
    al::LiveActor* mBreakModel = nullptr;
    al::HitSensor* mScoreSensor = nullptr;
};
static_assert(sizeof(KoopaSignBoard) == 0x158);
