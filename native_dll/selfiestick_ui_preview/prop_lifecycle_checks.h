#pragma once

namespace prop_lifecycle_checks {
inline int originalCalls{};
inline bool queuedBeforeOriginal{};
inline void* __fastcall OriginalChain(void*, void* entity, std::uint32_t) {
    ++originalCalls;
    queuedBeforeOriginal = selfiestick::g_propBirthCount != 0u;
    return entity; // Stand-in for the existing engine/HLAE chain result.
}
inline void* __fastcall UntouchedRemove(void*, void*, std::uint32_t) { return nullptr; }
inline bool Run() {
    using namespace selfiestick;
    const auto oldOriginal = g_originalPropOnAddEntity;
    const auto oldList = g_entityListStorage;
    const auto oldActive = g_propDiscoveryCaptureActive.load();
    void* list = reinterpret_cast<void*>(123);
    g_entityListStorage = &list;
    void* table[17]{};
    table[15] = reinterpret_cast<void*>(&OriginalChain);
    table[16] = reinterpret_cast<void*>(&UntouchedRemove);
    const auto originalAdd = table[15];
    const auto originalRemove = table[16];
    g_propBirthCount = g_propBirthHead = 0u;
    g_propDiscoveryCaptureActive.store(true);
    int stage = 1;
    const bool passed = [&]() {
        if (InstallPropLifecycleSlot(table, originalRemove, originalRemove) || table[15] != originalAdd) return false;
        if (!InstallPropLifecycleSlot(table, originalAdd, originalRemove)
            || table[16] != originalRemove) return false;
        const auto notify = reinterpret_cast<PropOnAddEntityFn>(table[15]);
        const EntityHandle handle{(1u << 15u) | 30000u};
        void* entity = reinterpret_cast<void*>(456);
        if (notify(list, entity, handle.raw) != entity || originalCalls != 1 || queuedBeforeOriginal) return false;
        std::array<EntityHandle, 64> drained{};
        if (DrainPropEntityBirths(drained) != 1u || drained[0].raw != handle.raw) return false;
        stage = 2;
        notify(reinterpret_cast<void*>(999), entity, handle.raw);
        if (DrainPropEntityBirths(drained) != 0u) return false;
        g_propDiscoveryCaptureActive.store(false);
        notify(list, entity, handle.raw);
        if (DrainPropEntityBirths(drained) != 0u || originalCalls != 3) return false;
        stage = 3;
        for (unsigned int i = 1; i <= 300; ++i) RecordPropEntityBirth(EntityHandle{i});
        if (g_propBirthCount != 256u || DrainPropEntityBirths(drained) != 64u
            || drained[0].raw != 300u || drained[63].raw != 237u) return false;
        if (DrainPropEntityBirths(drained) != 64u || drained[0].raw != 236u) return false;
        stage = 4;
        // Failed compare-exchange must not overwrite another owner's slot.
        if (ReplacePropLifecycleSlot(table + 15, originalAdd, originalRemove)
            || table[15] != reinterpret_cast<void*>(&ObservePropOnAddEntity)) return false;
        return true;
    }();
    ReplacePropLifecycleSlot(table + 15, reinterpret_cast<void*>(&ObservePropOnAddEntity), originalAdd);
    g_originalPropOnAddEntity = oldOriginal;
    g_entityListStorage = oldList;
    g_propDiscoveryCaptureActive.store(oldActive);
    g_propBirthCount = g_propBirthHead = 0u;
    if (passed) puts("prop lifecycle regression passed: atomic guarded vtable swap, original/HLAE chain first, result preservation, Remove untouched, exact handle queue, mode/system filtering and bounded newest-first overflow");
    else std::printf("prop lifecycle regression FAILED at stage %d\n", stage);
    return passed;
}
}
