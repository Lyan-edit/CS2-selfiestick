// Exact CS2 26598758 CSource2Client::SetGlobals[11]. The original method stores
// the Globals pointer here; HLAE's vtable detour still calls the original.
// This code only reads that pointer. It installs no additional hook.
constexpr std::uint32_t kPlayerSetGlobalsRva = 0x00B75C50u;
constexpr std::uint32_t kPlayerGlobalsStorageRva = 0x02221AB8u;
constexpr std::uint32_t kPlayerDefaultGlobalsRva = 0x02221AE0u;

bool DecodePlayerRenderTime(int tick, int beforeFrame, int afterFrame, float interpolation,
    PlayerMotionClock& clock) {
    if (tick < 0 || beforeFrame < 0 || beforeFrame != afterFrame
        || !std::isfinite(interpolation) || interpolation < 0 || interpolation > 1) return false;
    clock.renderDemoSeconds = (static_cast<double>(tick) + interpolation) / 64.0;
    clock.renderFrame = beforeFrame; clock.interpolation = interpolation;
    clock.renderTimeValid = true;
    return true;
}

bool TryReadPlayerRenderTime(PlayerMotionClock& clock) {
    if (!g_clientModule || !g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)) return false;
    __try {
        const auto* base = reinterpret_cast<const std::uint8_t*>(g_clientModule);
        const auto* code = base + kPlayerSetGlobalsRva;
        // Both the previous-pointer load and new-pointer store must still
        // reference the same exact-build storage.
        if (code[0] != 0x48 || code[1] != 0x8B || code[2] != 0x0D
            || code[28] != 0x48 || code[29] != 0x89 || code[30] != 0x15
            || code + 7 + *reinterpret_cast<const std::int32_t*>(code + 3) != base + kPlayerGlobalsStorageRva
            || code + 35 + *reinterpret_cast<const std::int32_t*>(code + 31) != base + kPlayerGlobalsStorageRva) return false;
        const auto* globals = *reinterpret_cast<const std::uint8_t* const volatile*>(base + kPlayerGlobalsStorageRva);
        if (!globals || globals == base + kPlayerDefaultGlobalsRva) return false;
        const int before = *reinterpret_cast<const volatile int*>(globals + 4);
        const float fraction = *reinterpret_cast<const volatile float*>(globals + 14 * 4);
        const int after = *reinterpret_cast<const volatile int*>(globals + 4);
        return DecodePlayerRenderTime(clock.tick, before, after, fraction, clock);
    }
    __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
