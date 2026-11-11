// Exact installed build: after native OverrideView, FOV/aspect work and cleanup,
// immediately before final origin/angle history copies and the epilogue.
constexpr std::uint32_t kPlayerTailPatchRva = 0x00B9EA60u;
constexpr std::array<std::uint8_t, hook::kPlayerTailPatchSize> kPlayerTailReplay{
    0xF2,0x0F,0x10,0x06, 0x4C,0x8D,0x9C,0x24,0xC8,0,0,0,
    0x8B,0x46,0x08, 0x41,0x0F,0x28,0x73,0xE8};
std::mutex g_playerTailInstallMutex;
void* g_playerTailStub{};

bool EnsurePlayerTailHookInstalled() {
    if (g_playerTailHookInstalled.load(std::memory_order_acquire)) return true;
    std::lock_guard lock(g_playerTailInstallMutex);
    if (g_playerTailHookInstalled.load(std::memory_order_acquire)) return true;
    if (!g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire) || g_clientModule == nullptr) {
        SetInitialized(false, FormatIncompatibleBuildReason("player final-view build is not verified")); return false;
    }
    auto* patch = reinterpret_cast<std::uint8_t*>(g_clientModule) + kPlayerTailPatchRva;
    if (!IsAddressExecutableInModule(g_clientModule, patch)
        || std::memcmp(patch, kPlayerTailReplay.data(), kPlayerTailReplay.size()) != 0) {
        SetInitialized(false, FormatIncompatibleBuildReason("player final-view instructions changed")); return false;
    }
    auto* stub = static_cast<std::uint8_t*>(AllocateExecutableMemoryNearAddress(patch, 512));
    if (!stub) { SetInitialized(false, "player final-view allocation failed"); return false; }
    std::vector<std::uint8_t> code;
    std::string failure;
    if (!hook::BuildPlayerTailDetourStub(reinterpret_cast<std::uintptr_t>(stub),
        reinterpret_cast<std::uintptr_t>(patch + kPlayerTailReplay.size()),
        reinterpret_cast<std::uintptr_t>(&SelfieStickSetUpViewTailBridge), kPlayerTailReplay, code, &failure)) {
        VirtualFree(stub, 0, MEM_RELEASE); SetInitialized(false, failure); return false;
    }
    std::memcpy(stub, code.data(), code.size()); FlushInstructionCache(GetCurrentProcess(), stub, code.size());
    std::array<std::uint8_t, hook::kSetUpViewPatchSize> entry{};
    if (!hook::BuildSetUpViewEntryPatch(reinterpret_cast<std::uintptr_t>(patch),
        reinterpret_cast<std::uintptr_t>(stub), entry, &failure)) {
        VirtualFree(stub, 0, MEM_RELEASE); SetInitialized(false, failure); return false;
    }
    std::array<std::uint8_t, hook::kPlayerTailPatchSize> bytes{}; bytes.fill(0x90);
    std::copy(entry.begin(), entry.end(), bytes.begin());
    if (!WriteHotCodePatchSafely(patch, bytes.data(), bytes.size())) {
        VirtualFree(stub, 0, MEM_RELEASE); SetInitialized(false, "player final-view patch failed"); return false;
    }
    g_playerTailStub = stub;
    g_playerTailHookInstalled.store(true, std::memory_order_release);
    TracePrintf("player-final-view installed patch=%p stub=%p size=%u", patch, stub, static_cast<unsigned>(bytes.size()));
    return true;
}
