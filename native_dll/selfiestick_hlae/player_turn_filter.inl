// Only the pawn-local yaw is filtered. The rendered body origin, height and
// camera radius remain authoritative and are never integrated or delayed.
struct PlayerMotionClock {
    double wall{};
    int tick{};
    bool paused{};
};
bool TryReadPlayerMotionClock(PlayerMotionClock& clock);
using PlayerMotionClockReader = bool (*)(PlayerMotionClock&);
PlayerMotionClockReader g_playerMotionClockReader = TryReadPlayerMotionClock;

struct PlayerTurnFilter {
    struct Interval { double wall{}, demo{}; };
    std::array<Interval, 8> intervals{};
    std::size_t intervalHead{}, intervalCount{};
    double yaw{}, rawYaw{}, wall{}, tickWall{}, rate{1.0};
    int tick{}, rateTick{};
    bool valid{}, paused{}, fullInterval{}, rateValid{};
};

float FilterPlayerTurn(PlayerTurnFilter& state, float rawYaw, const PlayerMotionClock& clock) {
    const auto reset = [&]() {
        state = {};
        state.valid = true; state.yaw = std::remainder(static_cast<double>(rawYaw), 360.0);
        state.rawYaw = rawYaw; state.wall = state.tickWall = clock.wall;
        state.tick = state.rateTick = clock.tick; state.paused = clock.paused;
        return static_cast<float>(state.yaw);
    };
    if (!std::isfinite(rawYaw) || !std::isfinite(clock.wall))
        return state.valid ? static_cast<float>(state.yaw) : 0.0f;
    if (!state.valid || clock.wall < state.wall || clock.tick < state.tick
        || clock.tick - state.tick > 64) return reset();
    state.rawYaw = rawYaw;
    if (clock.paused) {
        if (state.paused && clock.tick != state.tick) return reset(); // paused seek/frame step
        state.wall = state.tickWall = clock.wall;
        state.tick = state.rateTick = clock.tick;
        state.intervalHead = state.intervalCount = 0u;
        state.fullInterval = false; state.paused = true;
        return static_cast<float>(state.yaw);
    }
    if (state.paused) {
        state.paused = false; state.wall = state.tickWall = clock.wall;
        state.tick = state.rateTick = clock.tick;
        return static_cast<float>(state.yaw);
    }
    const double elapsed = clock.wall - state.wall;
    if (elapsed <= 0.0) return static_cast<float>(state.yaw); // repeated subview callback
    if (elapsed > 0.25) return reset(); // callback suspension; do not orbit through a stale pose
    if (clock.tick > state.rateTick) {
        const double span = clock.wall - state.tickWall;
        if (state.fullInterval && span > 0.0) {
            state.intervals[state.intervalHead] = {span, (clock.tick - state.rateTick) / 64.0};
            state.intervalHead = (state.intervalHead + 1u) % state.intervals.size();
            state.intervalCount = std::min(state.intervalCount + 1u, state.intervals.size());
            double wallSum{}, demoSum{};
            for (std::size_t i = 0; i < state.intervalCount; ++i) {
                wallSum += state.intervals[i].wall; demoSum += state.intervals[i].demo;
            }
            state.rate = std::clamp(demoSum / wallSum, 0.0001, 64.0);
            state.rateValid = true;
        }
        state.fullInterval = true; state.rateTick = clock.tick; state.tickWall = clock.wall;
    }
    // Twelve milliseconds of demo time; about 120 ms real time at 0.1x.
    // Estimating rate over full tick intervals makes the filter progress on
    // every render callback rather than in integer-tick jumps.
    const double alpha = -std::expm1(-elapsed * state.rate / 0.012);
    const double delta = std::remainder(static_cast<double>(rawYaw) - state.yaw, 360.0);
    state.yaw = std::remainder(state.yaw + delta * alpha, 360.0);
    if (std::fabs(delta) < 0.002) state.yaw = std::remainder(static_cast<double>(rawYaw), 360.0);
    state.wall = clock.wall; state.tick = clock.tick;
    return static_cast<float>(state.yaw);
}

float SampleStablePlayerYaw(PlayerTurnFilter& state, float rawYaw) {
    PlayerMotionClock clock{};
    if (g_playerMotionClockReader == nullptr || !g_playerMotionClockReader(clock)) {
        state = {};
        return rawYaw; // no wall-clock-only substitute for an unverified demo clock
    }
    return FilterPlayerTurn(state, rawYaw, clock);
}
