#pragma once

namespace player_turn_checks {
inline selfiestick::PlayerMotionClock clock{};
inline bool ReadClock(selfiestick::PlayerMotionClock& output) { output = clock; return true; }
inline bool Run() {
    using namespace selfiestick;
    const auto oldOffsets = g_offsets;
    const auto oldModule = g_clientModule;
    const auto oldGetter = g_getEntityAbsOrigin;
    const auto oldReader = g_playerMotionClockReader;
    const auto oldFrame = g_playerFrame;
    const auto oldKnown = g_knownClientSchemaFallbackAllowed.load();
    const auto oldGeneration = g_playerCameraGeneration.load();
    hand_checks::C_CSPlayerPawn pawn;
    unsigned char identity[64]{}, scene[512]{};
    hand_checks::Store(&pawn, 0x10, static_cast<void*>(identity));
    hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
    hand_checks::Store(identity, 0x10, EntityHandle{(850u << 15u) | 42u});
    g_offsets.entityInstanceToIdentity = 0x10; g_offsets.sceneNode = 0x330;
    g_offsets.sceneAbsOrigin = 0xC8; g_offsets.pawnEyeAngles = 0x3600;
    g_offsets.entitySimulationTime = 0;
    g_clientModule = GetModuleHandleW(nullptr);
    g_getEntityAbsOrigin = prop_origin_checks::EngineGetAbsOrigin;
    g_knownClientSchemaFallbackAllowed.store(true);
    g_playerMotionClockReader = ReadClock;
    std::string failure;
    const bool passed = [&]() {
        for (const double rate : {0.01, 0.1, 1.0, 8.0}) {
            for (const double fps : {30.0, 60.0, 144.0, 500.0}) {
                g_playerFrame = {};
                double previousYaw{}, maxStep{}, maxLag{};
                const int frames = static_cast<int>(std::ceil(0.75 / rate * fps));
                for (int frame = 0; frame <= frames; ++frame) {
                    const double wall = frame / fps, demo = wall * rate;
                    clock = {1000 + wall, static_cast<int>(std::floor(demo * 64)), false};
                    const float rawYaw = static_cast<float>(clock.tick);
                    const Vec3 origin{100 + static_cast<float>(demo) * 10, 200, 0};
                    hand_checks::Store(scene, 0xC8, origin);
                    hand_checks::Store(&pawn, 0x3600, Vec3{30, rawYaw, 0});
                    WeaponFrameBasis basis{};
                    if (!TryBuildHumanFrameBasis(&pawn, HeldItemKind::Unknown, nullptr, nullptr, basis, failure)) return false;
                    const double yaw = std::atan2(basis.forward.y, basis.forward.x) * 180 / 3.141592653589793;
                    if (frame > 0 && demo > 0.15) {
                        maxStep = std::max(maxStep, std::fabs(std::remainder(yaw - previousYaw, 360.0)));
                        maxLag = std::max(maxLag, std::fabs(std::remainder(rawYaw - yaw, 360.0)));
                    }
                    previousYaw = yaw;
                    const Vec3 relative = Subtract(basis.anchorOrigin, basis.bodyOrigin);
                    if (Length(Subtract(basis.bodyOrigin, origin)) > 0.001f
                        || std::fabs(Length(relative) - std::sqrt(10.0f*10+12.0f*12+46.0f*46)) > 0.001f) return false;
                }
                const double idealStep = 64 * rate / fps;
                if (maxStep > std::max(0.55, idealStep * 1.8) || maxLag > 2.5 + idealStep) {
                    std::printf("player slow-turn FAILED %.2fx / %.0f FPS: max step %.3f deg, lag %.3f deg\n", rate, fps, maxStep, maxLag);
                    return false;
                }
                if (rate == 0.1 && fps == 144)
                    std::printf("player 0.1x turn: max render step %.4f deg (raw tick step 1 deg), max yaw lag %.3f deg\n", maxStep, maxLag);
            }
        }
        // Changing pawn lifetime or re-enabling the camera must not inherit
        // the previous subject's filtered heading.
        hand_checks::Store(identity, 0x10, EntityHandle{(851u << 15u) | 42u});
        hand_checks::Store(&pawn, 0x3600, Vec3{0, 90, 0});
        WeaponFrameBasis changed{};
        if (!TryBuildHumanFrameBasis(&pawn, HeldItemKind::Unknown, nullptr, nullptr, changed, failure)
            || changed.forward.y < 0.9999f) return false;
        g_playerCameraGeneration.fetch_add(1u);
        hand_checks::Store(&pawn, 0x3600, Vec3{0, -90, 0});
        if (!TryBuildHumanFrameBasis(&pawn, HeldItemKind::Unknown, nullptr, nullptr, changed, failure)
            || changed.forward.y > -0.9999f) return false;
        // Shortest-path wrap, pause/no drift, resume, duplicate callbacks and seeks.
        PlayerTurnFilter state{};
        PlayerMotionClock sample{0, 100, false};
        FilterPlayerTurn(state, 179, sample);
        sample = {0.016, 101, false};
        const float wrapped = FilterPlayerTurn(state, -179, sample);
        if (std::fabs(std::remainder(wrapped - 179.0f, 360.0f)) > 2.01f) return false;
        sample.paused = true;
        const float held = FilterPlayerTurn(state, -179, sample);
        for (int i = 0; i < 100; ++i) {
            sample.wall += 0.1;
            if (FilterPlayerTurn(state, 20, sample) != held) return false;
        }
        sample.paused = false; sample.wall += 0.016;
        if (FilterPlayerTurn(state, -179, sample) != held) return false;
        sample.wall += 0.016; ++sample.tick;
        const float resumed = FilterPlayerTurn(state, -179, sample);
        if (FilterPlayerTurn(state, 90, sample) != resumed) return false;
        sample.paused = true;
        FilterPlayerTurn(state, -179, sample);
        sample.wall += 0.1; sample.tick += 1000;
        if (FilterPlayerTurn(state, 90, sample) != 90) return false;
        sample.paused = false; sample.wall += 0.1; sample.tick = 5;
        if (FilterPlayerTurn(state, 25, sample) != 25) return false;
        // Fast alternating sub-degree yaw noise after the rate estimator warms.
        state = {};
        double maxNoise{};
        for (int i = 0; i < 1000; ++i) {
            const double wall = i / 144.0;
            sample = {wall, static_cast<int>(std::floor(wall * 0.1 * 64)), false};
            const float yaw = FilterPlayerTurn(state, i % 2 ? 0.2f : -0.2f, sample);
            if (wall > 2.0) maxNoise = std::max(maxNoise, std::fabs(static_cast<double>(yaw)));
        }
        if (maxNoise > 0.025) return false;
        std::printf("player 0.1x alternating yaw noise: +/-0.2 deg input -> %.4f deg max residual\n", maxNoise);
        g_playerMotionClockReader = nullptr;
        if (SampleStablePlayerYaw(state, 45) != 45 || state.valid) return false;
        return true;
    }();
    g_offsets = oldOffsets; g_clientModule = oldModule; g_getEntityAbsOrigin = oldGetter;
    g_playerMotionClockReader = oldReader; g_playerFrame = oldFrame;
    g_knownClientSchemaFallbackAllowed.store(oldKnown);
    g_playerCameraGeneration.store(oldGeneration);
    if (passed) puts("player turn regression passed: 0.01/0.1/1/8x at 30/60/144/500 FPS, unchanged body position/radius, angle wrap, pause, resume, repeat callbacks and seek reset");
    return passed;
}
}
