// Same exact-build demo interface as HLAE, plus its read-only pause query.
constexpr std::uint32_t kPlayerDemoPauseRva = 0x00035930u;
bool TryReadPlayerMotionClock(PlayerMotionClock& clock) {
    clock = {};
    clock.wall = HighResolutionSeconds();
    if (!g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)) return false;
    const HMODULE module = GetModuleHandleW(L"engine2.dll");
    if (module == nullptr) return false;
    static thread_local HMODULE cachedModule{};
    static thread_local void* cachedEngine{};
    __try {
        const auto* base = reinterpret_cast<const std::uint8_t*>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->FileHeader.TimeDateStamp != kDemoEngineTimestamp
            || nt->OptionalHeader.SizeOfImage != kDemoEngineImageSize
            || nt->OptionalHeader.CheckSum != kDemoEngineChecksum) return false;
        if (cachedModule != module || cachedEngine == nullptr) {
            const auto factory = GetCreateInterface(module);
            if (factory == nullptr) return false;
            cachedEngine = factory("Source2EngineToClient001", nullptr);
            cachedModule = module;
        }
        void* engine = cachedEngine;
        if (engine == nullptr) return false;
        const auto engineVtable = *reinterpret_cast<void***>(engine);
        if (engineVtable[69] != base + kDemoGetFileRva) return false;
        using GetFile = void* (__fastcall*)(void*);
        void* demo = reinterpret_cast<GetFile>(engineVtable[69])(engine);
        if (demo == nullptr) return false;
        const auto vtable = *reinterpret_cast<void***>(demo);
        if (vtable[3] != base + kDemoGetTickRva || vtable[12] != base + kPlayerDemoPauseRva) return false;
        using GetTick = int (__fastcall*)(void*);
        using IsPaused = bool (__fastcall*)(void*);
        clock.tick = reinterpret_cast<GetTick>(vtable[3])(demo);
        clock.paused = reinterpret_cast<IsPaused>(vtable[12])(demo);
        return clock.tick >= 0 && clock.tick <= 640000000;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { cachedEngine = nullptr; return false; }
}
