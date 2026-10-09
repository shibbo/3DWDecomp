#pragma once
#include "Library/LiveActor/LiveActor.hpp"
class GeneratorBox;
class GeneratorBoxChild : public al::LiveActor {
public:
    explicit GeneratorBoxChild(const char*);
    ~GeneratorBoxChild() override;
    void initWithArchive(const al::ActorInitInfo&, const char*, const char*);
    bool receiveMsg(const al::SensorMsg*, al::HitSensor*, al::HitSensor*) override;
    void appear() override;
    void control() override;
    void requestBreak();
    bool isTop();
    void trySetDisappear();
    bool requestDisappearSign();
    void requestBound();
    void exeAppear();
    void exeBound();
    void exeDisappear();
    void exeDisappearSign();
    void exeWait();
    void setChild(GeneratorBoxChild*);
    void setParent(GeneratorBoxChild*);

    /**
     * @brief Set the generator box this block belongs to.
     * @param pHost The owning generator box.
     */
    void setHost(GeneratorBox* pHost) { mHost = pHost; }

    /**
     * @brief Get the block stacked on top of this one.
     * @return The next block in the column, or nullptr for the last one.
     */
    GeneratorBoxChild* getChild() const { return mChild; }
private:
    GeneratorBox* mHost = nullptr;
    GeneratorBoxChild* mChild = nullptr;
    GeneratorBoxChild* mParent = nullptr;
    int mReactionTime = 0;
};
static_assert(sizeof(GeneratorBoxChild) == 0x168);
