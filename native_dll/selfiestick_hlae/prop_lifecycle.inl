// Exact-build CGameEntitySystem observer. Only the virtual OnAddEntity slot is
// replaced; the original target (including any HLAE entry detour) runs first.
// No function prologue, Remove slot, graphics hook or engine listener ABI is edited.
using PropOnAddEntityFn = void* (__fastcall*)(void*, void*, std::uint32_t);
PropOnAddEntityFn g_originalPropOnAddEntity{};
std::atomic<bool> g_propLifecycleAttempted{false};
std::atomic<bool> g_propLifecycleInstalled{false};
std::mutex g_propBirthMutex;
std::array<EntityHandle, 256> g_propBirthQueue{};
std::size_t g_propBirthHead{}, g_propBirthCount{};

void RecordPropEntityBirth(EntityHandle handle) {
    if (!handle.IsValid()) return;
    std::unique_lock lock(g_propBirthMutex, std::try_to_lock);
    if (!lock.owns_lock()) return; // bounded producer; full sweep is the fallback
    if (g_propBirthCount == g_propBirthQueue.size()) {
        g_propBirthHead = (g_propBirthHead + 1u) % g_propBirthQueue.size();
        --g_propBirthCount;
    }
    g_propBirthQueue[(g_propBirthHead + g_propBirthCount) % g_propBirthQueue.size()] = handle;
    ++g_propBirthCount;
}

std::size_t DrainPropEntityBirths(std::array<EntityHandle, 64>& handles) {
    std::unique_lock lock(g_propBirthMutex, std::try_to_lock);
    if (!lock.owns_lock()) return 0u;
    const auto count = std::min(g_propBirthCount, handles.size());
    // Newest births get the first camera opportunity after a loading burst.
    for (std::size_t i = 0; i < count; ++i)
        handles[i] = g_propBirthQueue[(g_propBirthHead + g_propBirthCount - 1u - i) % g_propBirthQueue.size()];
    g_propBirthCount -= count;
    return count;
}

void* __fastcall ObservePropOnAddEntity(void* system, void* entity, std::uint32_t handle) {
    void* result = g_originalPropOnAddEntity(system, entity, handle);
    if (g_propDiscoveryCaptureActive.load(std::memory_order_acquire)
        && g_entityListStorage != nullptr && system == *g_entityListStorage) {
        // Store only the serial-qualified handle AFTER the engine/HLAE chain.
        // Never retain a naked entity pointer or scan/classify from this callback.
        RecordPropEntityBirth(EntityHandle{handle});
    }
    return result;
}

bool ReplacePropLifecycleSlot(void** slot, void* expected, void* replacement) {
    DWORD protection{};
    if (!VirtualProtect(slot, sizeof(void*), PAGE_READWRITE, &protection)) return false;
    const auto previous = InterlockedCompareExchangePointer(
        reinterpret_cast<PVOID volatile*>(slot), replacement, expected);
    DWORD ignored{};
    VirtualProtect(slot, sizeof(void*), protection, &ignored);
    return previous == expected;
}

bool InstallPropLifecycleSlot(void** vtable, void* expectedAdd, void* expectedRemove) {
    void* add = nullptr;
    void* remove = nullptr;
    if (vtable == nullptr || !TryReadMemoryValue(vtable + 15, add)
        || !TryReadMemoryValue(vtable + 16, remove) || add != expectedAdd || remove != expectedRemove) return false;
    g_originalPropOnAddEntity = reinterpret_cast<PropOnAddEntityFn>(add);
    return ReplacePropLifecycleSlot(vtable + 15, add, reinterpret_cast<void*>(&ObservePropOnAddEntity));
}

void EnsurePropLifecycleObserver() {
    if (g_propLifecycleAttempted.load(std::memory_order_acquire)
        || g_entityListStorage == nullptr || *g_entityListStorage == nullptr) return;
    if (g_propLifecycleAttempted.exchange(true, std::memory_order_acq_rel)) return;
    if (!g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)) return;
    void** vtable = nullptr;
    const auto base = reinterpret_cast<std::uintptr_t>(g_clientModule);
    if (!TryReadMemoryValue(reinterpret_cast<void***>(*g_entityListStorage), vtable)
        || reinterpret_cast<std::uintptr_t>(vtable) != base + kPropLifecycleVtableRva
        || !IsAddressReadableInModule(g_clientModule, vtable + 16)
        || !IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(base + kPropLifecycleAddRva))
        || !IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(base + kPropLifecycleRemoveRva))) {
        TracePrintf("prop-birth-observer unavailable; using complete-slot fallback");
        return;
    }
    // The camera/UI already require process-lifetime loading. Pin explicitly
    // before publishing another callback address to the engine vtable.
    HMODULE pinned{};
    if (!GetModuleHandleExW(GET_MODULE_HANDLE_EX_FLAG_FROM_ADDRESS | GET_MODULE_HANDLE_EX_FLAG_PIN,
            reinterpret_cast<LPCWSTR>(&ObservePropOnAddEntity), &pinned)) return;
    const bool installed = InstallPropLifecycleSlot(vtable,
        reinterpret_cast<void*>(base + kPropLifecycleAddRva), reinterpret_cast<void*>(base + kPropLifecycleRemoveRva));
    g_propLifecycleInstalled.store(installed, std::memory_order_release);
    TracePrintf("prop-birth-observer installed=%d add-slot=15 remove-untouched=16", installed ? 1 : 0);
}
