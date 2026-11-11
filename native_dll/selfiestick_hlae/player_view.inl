struct PlayerViewCache {
    void* entityList{};
    void* entity{};
    std::uint64_t generation{};
    EntityHandle pawn{}, confirmedFollow{};
    double followConfirmedAt{-1.0};
    WeaponFrameBasis basis{};
    float fov{};
    float sourceFov{};
    float simulationTime{};
    bool valid{}, fovValid{};
    bool hasSimulationTime{};
};
thread_local PlayerViewCache g_playerView;
struct PlayerFovLatch {
    void* entityList{};
    EntityHandle target{};
    std::uint64_t generation{};
    float fov{};
    bool valid{};
};
std::mutex g_playerFovMutex;
PlayerFovLatch g_playerFovLatch;

void PreparePlayerViewCache() {
    void* list = g_entityListStorage != nullptr ? *g_entityListStorage : nullptr;
    const auto generation = g_playerCameraGeneration.load(std::memory_order_relaxed);
    if (g_playerView.entityList != list || g_playerView.generation != generation) {
        g_playerView = {};
        g_playerView.entityList = list;
        g_playerView.generation = generation;
    }
}

TargetResolution ResolvePlayerTargetForView(const OverrideConfig& config, bool refresh,
    double now = HighResolutionSeconds()) {
    PreparePlayerViewCache();
    if (config.targetMode == TargetMode::Locked) return ResolveLockedTarget(config.lockedHandle);
    TargetResolution result{};
    if (!refresh && config.followHandle.IsValid()) result = ResolveFollowSnapshotTarget(config.followHandle);
    if (result.entity == nullptr) {
        result = ResolveFollowTarget();
        if (result.entity != nullptr) {
            g_playerView.confirmedFollow = result.handle;
            g_playerView.followConfirmedAt = now;
        } else if (g_playerView.valid && config.followHandle.raw == g_playerView.pawn.raw
            && config.followHandle.raw == g_playerView.confirmedFollow.raw
            && now >= g_playerView.followConfirmedAt && now - g_playerView.followConfirmedAt <= 0.25) {
            // A missing observer-services sample is not a spectator switch.
            // A positive new target always wins; never follow a reused serial.
            auto retained = ResolveFollowSnapshotTarget(config.followHandle);
            if (retained.entity != nullptr) result = retained;
        }
    }
    if (result.entity == nullptr) { g_playerView.valid = false; g_playerView.fovValid = false; }
    return result;
}

bool TryBuildStablePlayerFrame(void* pawn, EntityHandle handle, const Vec3* eye, const Vec3* angles,
    WeaponFrameBasis& basis, bool& recovering, std::string& failure) {
    PreparePlayerViewCache();
    recovering = false;
    float simulationTime{};
    const bool timed = g_offsets.entitySimulationTime > 0
        && TryReadOffsetValue(pawn, g_offsets.entitySimulationTime, simulationTime)
        && std::isfinite(simulationTime) && simulationTime >= 0;
    if (g_playerView.pawn.raw == handle.raw && timed && g_playerView.hasSimulationTime
        && simulationTime < g_playerView.simulationTime - 0.25f) {
        g_playerView.valid = false;
        g_playerView.fovValid = false;
        g_playerFrame = {};
    }
    g_playerView.simulationTime = simulationTime;
    g_playerView.hasSimulationTime = timed;
    const bool same = g_playerView.valid && g_playerView.pawn.raw == handle.raw && g_playerView.entity == pawn;
    if (!same) { g_playerView.valid = false; g_playerView.fovValid = false; }
    if (TryBuildWeaponFrameBasis(pawn, eye, angles, basis, failure)) {
        g_playerView.pawn = handle;
        g_playerView.entity = pawn;
        g_playerView.basis = basis;
        g_playerView.valid = true;
        return true;
    }
    if (!same || !handle.IsValid() || ResolveHandleToEntity(handle) != pawn) return false;
    // No position integration or sampling-rate reduction. A complete actor
    // frame owns the camera through temporary scene/eye/animation data gaps.
    basis = g_playerView.basis;
    recovering = true;
    failure.clear();
    return true;
}

void ApplyPlayerViewFov(void* viewSetup, EntityHandle target, const WeaponFrameBasis& basis, bool /*recovering*/) {
    g_playerView.sourceFov = std::numeric_limits<float>::quiet_NaN();
    TryReadOffsetValue(viewSetup, kViewSetupFovOffset, g_playerView.sourceFov);
    std::lock_guard lock(g_playerFovMutex);
    const auto generation = g_playerCameraGeneration.load(std::memory_order_relaxed);
    void* list = g_entityListStorage ? *g_entityListStorage : nullptr;
    if (g_playerFovLatch.entityList != list || g_playerFovLatch.target.raw != target.raw
        || g_playerFovLatch.generation != generation) {
        g_playerFovLatch = {list, target, generation};
    }
    if (!g_playerFovLatch.valid) {
        OverrideScopedAwpFov(viewSetup, target, basis);
        float fov{};
        if (!TryReadOffsetValue(viewSetup, kViewSetupFovOffset, fov)
            || !std::isfinite(fov) || fov < 1 || fov > 179) fov = kFallbackUnscopedFov;
        g_playerFovLatch.fov = fov;
        g_playerFovLatch.valid = true;
    }
    g_playerView.fov = g_playerFovLatch.fov;
    g_playerView.fovValid = true;
    TryWriteOffsetValue(viewSetup, kViewSetupFovOffset, g_playerView.fov);
}
