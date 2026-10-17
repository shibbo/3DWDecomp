#pragma once

#include "Library/LiveActor/LiveActor.hpp"

namespace al { template <class T> class DeriveActorGroup; }
class KuriboMini;

/** @brief Creates a ring of Mini Goombas, optionally released by a switch. */
class KuriboMiniGenerator : public al::LiveActor {
public:
    explicit KuriboMiniGenerator(const char* pName);
    void init(const al::ActorInitInfo& rInfo) override;
    void release();
    void generate();
    void exeWait();
    void exeAfterGenerate();

private:
    al::DeriveActorGroup<KuriboMini>* mGroup = nullptr;
};

static_assert(sizeof(KuriboMiniGenerator) == 0x150);
