// Only the pawn-local yaw is filtered. The rendered body origin, height and
// camera radius remain authoritative and are never integrated or delayed.
struct PlayerMotionClock {
    double wall{};
    int tick{};
    bool paused{};
    double renderDemoSeconds{};
    int renderFrame{-1};
    bool renderTimeValid{};
    float interpolation{};
};
bool TryReadPlayerMotionClock(PlayerMotionClock& clock);
using PlayerMotionClockReader = bool (*)(PlayerMotionClock&);
PlayerMotionClockReader g_playerMotionClockReader = TryReadPlayerMotionClock;
thread_local PlayerMotionClock g_lastPlayerMotionClock;

struct PlayerTurnFilter {
    double yaw{}, rawYaw{}, wall{}, rate{1.0};
    int tick{};
    bool valid{}, paused{}, rateValid{};
    double renderTime{};
    int renderFrame{-1};
    bool renderClock{};
};

float FilterPlayerTurn(PlayerTurnFilter& state, float rawYaw, const PlayerMotionClock& clock) {
    if (!clock.renderTimeValid) { state = {}; return std::isfinite(rawYaw) ? rawYaw : 0.0f; }
    const auto reset = [&]() {
        state = {};
        state.valid = state.renderClock = true;
        state.yaw = std::remainder(static_cast<double>(rawYaw), 360.0);
        state.rawYaw = rawYaw; state.wall = clock.wall; state.tick = clock.tick;
        state.renderTime = clock.renderDemoSeconds; state.renderFrame = clock.renderFrame;
        state.paused = clock.paused;
        return static_cast<float>(state.yaw);
    };
    if (!std::isfinite(rawYaw) || !std::isfinite(clock.renderDemoSeconds))
        return state.valid ? static_cast<float>(state.yaw) : 0.0f;
    if (!state.valid || !state.renderClock || clock.renderDemoSeconds < state.renderTime
        || clock.renderDemoSeconds - state.renderTime > 1.0) return reset();
    state.rawYaw = rawYaw;
    if (clock.paused) {
        if (state.paused && clock.renderDemoSeconds != state.renderTime) return reset();
        state.renderTime = clock.renderDemoSeconds; state.renderFrame = clock.renderFrame;
        state.wall = clock.wall; state.tick = clock.tick; state.paused = true;
        return static_cast<float>(state.yaw);
    }
    if (state.paused) {
        state.paused = false; state.renderTime = clock.renderDemoSeconds;
        state.renderFrame = clock.renderFrame; state.wall = clock.wall; state.tick = clock.tick;
        return static_cast<float>(state.yaw);
    }
    const double elapsedDemo = clock.renderDemoSeconds - state.renderTime;
    if (elapsedDemo <= 0) return static_cast<float>(state.yaw);
    const double elapsedWall = clock.wall - state.wall;
    if (elapsedWall > 0) { state.rate = elapsedDemo / elapsedWall; state.rateValid = true; }
    const double delta = std::remainder(static_cast<double>(rawYaw) - state.yaw, 360.0);
    const double alpha = -std::expm1(-elapsedDemo / 0.012);
    state.yaw = std::remainder(state.yaw + delta * alpha, 360.0);
    if (std::fabs(delta) < 0.002) state.yaw = std::remainder(static_cast<double>(rawYaw), 360.0);
    state.renderTime = clock.renderDemoSeconds; state.renderFrame = clock.renderFrame;
    state.wall = clock.wall; state.tick = clock.tick;
    return static_cast<float>(state.yaw);
}



float SampleStablePlayerYaw(PlayerTurnFilter& state, float rawYaw) {
    PlayerMotionClock clock{};
    if (g_playerMotionClockReader == nullptr || !g_playerMotionClockReader(clock) || !clock.renderTimeValid) {
        g_lastPlayerMotionClock = {};
        state = {};
        return rawYaw; // no wall-clock-only substitute for an unverified demo clock
    }
    g_lastPlayerMotionClock = clock;
    return FilterPlayerTurn(state, rawYaw, clock);
}
