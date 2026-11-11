// Player-only sampling; keep the confirmed projectile origin/rig code intact.
#include "player_turn_filter.inl"
struct PlayerFrameCache {
    EntityHandle pawn{};
    void* entity{};
    void* entityList{};
    std::uint64_t generation{};
    Vec3 angles{}, bodyOrigin{};
    float bodyHeight{kFallbackEyeOriginHeight};
    float simulationTime{};
    bool hasAngles{};
    bool hasSimulationTime{};
    PlayerTurnFilter turn{};
};
thread_local PlayerFrameCache g_playerFrame;

bool ConstrainPlayerBodyCamera(const WeaponFrameBasis& basis, bool selfie, Vec3& camera) {
    const float minimum = selfie ? 32.0f : 24.0f;
    Vec3 delta = Subtract(camera, basis.bodyOrigin);
    delta.z = 0;
    const float distance = Length(delta);
    if (distance >= minimum) return false;
    if (!TryNormalize(delta)) delta = Scale(basis.forward, selfie ? 1.0f : -1.0f);
    camera.x = basis.bodyOrigin.x + delta.x * minimum;
    camera.y = basis.bodyOrigin.y + delta.y * minimum;
    return true;
}

PlayerFrameCache& PreparePlayerFrameCache(void* pawn, EntityHandle handle) {
    const auto generation = g_playerCameraGeneration.load(std::memory_order_relaxed);
    void* list = g_entityListStorage != nullptr ? *g_entityListStorage : nullptr;
    float simulationTime{};
    const bool timed = g_offsets.entitySimulationTime > 0
        && TryReadOffsetValue(pawn, g_offsets.entitySimulationTime, simulationTime)
        && std::isfinite(simulationTime) && simulationTime >= 0;
    const bool rewound = timed && g_playerFrame.hasSimulationTime
        && simulationTime < g_playerFrame.simulationTime - 0.25f;
    if (g_playerFrame.pawn.raw != handle.raw || g_playerFrame.entity != pawn
        || g_playerFrame.entityList != list || g_playerFrame.generation != generation || rewound) {
        g_playerFrame = {};
        g_playerFrame.pawn = handle;
        g_playerFrame.entity = pawn;
        g_playerFrame.entityList = list;
        g_playerFrame.generation = generation;
    }
    g_playerFrame.simulationTime = simulationTime;
    g_playerFrame.hasSimulationTime = timed;
    return g_playerFrame;
}

bool TryGetPlayerSceneOrigin(void* pawn, Vec3& origin, std::string& failure) {
    if (g_getEntityAbsOrigin != nullptr) {
        if (TryInvokeEntityAbsOrigin(g_getEntityAbsOrigin, pawn, origin)) return true;
        failure = "player absolute-origin resolver failed";
        return false;
    }
    if (g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)) {
        failure = "player absolute-origin resolver is not bound";
        return false;
    }
    return TryGetEntityAbsOrigin(pawn, origin, failure); // standalone fake entities
}

bool TryGetStablePlayerBodyOrigin(void* pawn, PlayerFrameCache& cache, Vec3& eye, std::string& failure) {
    Vec3 origin{};
    if (!TryGetPlayerSceneOrigin(pawn, origin, failure)) return false;
    if (Length(origin) <= 0.001f) { failure = Texts().reasonInvalidEyeOrigin; return false; }
    // Stance belongs to movement services, not the animated hand/render eye.
    // Only read exact-build class-qualified fields; no runtime schema probing.
    std::ptrdiff_t movementOffset{}, duckOffset{};
    void* movement = nullptr;
    float duck{};
    if (g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)
        && schema::TryGetKnownClientSchemaFieldOffsetFallback("C_BasePlayerPawn", "m_pMovementServices", movementOffset)
        && schema::TryGetKnownClientSchemaFieldOffsetFallback("CCSPlayer_MovementServices", "m_flDuckAmount", duckOffset)
        && TryReadOffsetPointer(pawn, movementOffset, movement) && movement != nullptr
        && TryReadOffsetValue(movement, duckOffset, duck) && std::isfinite(duck) && duck >= 0 && duck <= 1) {
        cache.bodyHeight = kFallbackEyeOriginHeight - 18.0f * duck;
    }
    cache.bodyOrigin = origin;
    eye = Add(origin, Vec3{0, 0, cache.bodyHeight});
    failure.clear();
    return true;
}
