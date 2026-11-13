// HLAE's ISource2EngineToClient001::GetDemoFile (69), IDemoFile::GetDemoTick
// (3), and 64 Hz demo time. Never substitute wall time or a dormant pawn clock.
// Guards verified against the installed engine2.dll; no additional hook.
constexpr std::uint32_t kDemoEngineTimestamp = 0x6AF4A60Du;
constexpr std::uint32_t kDemoEngineImageSize = 0x00A0A000u;
constexpr std::uint32_t kDemoEngineChecksum = 0x00721DBDu;
constexpr std::uint32_t kDemoGetFileRva = 0x00074380u;
constexpr std::uint32_t kDemoGetTickRva = 0x00035970u;
bool TryReadPropDemoSeconds(double& seconds) {
    seconds = -1.0;
    if (!g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)) return false;
    const HMODULE module = GetModuleHandleW(L"engine2.dll");
    if (module == nullptr) return false;
    __try {
        const auto* base = reinterpret_cast<const std::uint8_t*>(module);
        const auto* dos = reinterpret_cast<const IMAGE_DOS_HEADER*>(base);
        const auto* nt = reinterpret_cast<const IMAGE_NT_HEADERS64*>(base + dos->e_lfanew);
        if (nt->FileHeader.TimeDateStamp != kDemoEngineTimestamp
            || nt->OptionalHeader.SizeOfImage != kDemoEngineImageSize
            || nt->OptionalHeader.CheckSum != kDemoEngineChecksum) return false;
        const auto factory = GetCreateInterface(module);
        if (!factory) return false;
        void* engine = factory("Source2EngineToClient001", nullptr);
        if (!engine) return false;
        const auto vtable = *reinterpret_cast<void***>(engine);
        if (vtable[69] != base + kDemoGetFileRva) return false;
        using GetFile = void* (__fastcall*)(void*);
        void* demo = reinterpret_cast<GetFile>(vtable[69])(engine);
        if (!demo) return false;
        const auto demoVtable = *reinterpret_cast<void***>(demo);
        if (demoVtable[3] != base + kDemoGetTickRva) return false;
        using GetTick = int (__fastcall*)(void*);
        const int tick = reinterpret_cast<GetTick>(demoVtable[3])(demo);
        if (tick < 0 || tick > 640000000) return false;
        seconds = static_cast<double>(tick) / 64.0;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
