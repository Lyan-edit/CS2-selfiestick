// Entity/pose scratch stays thread-local. Only yaw history is shared so a
// callback migrating to another render thread cannot restart the turn filter.
struct SharedPlayerYawHistory {
    EntityHandle pawn{};
    void* entity{};
    void* entityList{};
    std::uint64_t generation{};
    float simulationTime{};
    bool hasSimulationTime{};
    PlayerTurnFilter turn{};
};
std::mutex g_playerYawMutex;
SharedPlayerYawHistory g_playerYawHistory;

float SampleSharedPlayerYaw(PlayerFrameCache& cache, float rawYaw) {
    PlayerMotionClock clock{};
    const bool timed = g_playerMotionClockReader != nullptr && g_playerMotionClockReader(clock)
        && clock.renderTimeValid;
    g_lastPlayerMotionClock = timed ? clock : PlayerMotionClock{};
    // No entity reads, engine calls, allocations, tracing or file writes under
    // this lock; it covers just the small history and the filter arithmetic.
    std::lock_guard lock(g_playerYawMutex);
    auto& shared = g_playerYawHistory;
    const bool rewound = cache.hasSimulationTime && shared.hasSimulationTime
        && cache.simulationTime < shared.simulationTime - 0.25f;
    if (shared.pawn.raw != cache.pawn.raw || shared.entity != cache.entity
        || shared.entityList != cache.entityList || shared.generation != cache.generation || rewound) {
        shared = {};
        shared.pawn = cache.pawn; shared.entity = cache.entity;
        shared.entityList = cache.entityList; shared.generation = cache.generation;
    }
    shared.simulationTime = cache.simulationTime; shared.hasSimulationTime = cache.hasSimulationTime;
    if (!timed) shared.turn = {};
    const float result = timed ? FilterPlayerTurn(shared.turn, rawYaw, clock) : rawYaw;
    cache.turn = shared.turn; // keep per-callback diagnostics coherent
    return result;
}
