#pragma once

namespace prop_rig_checks {
inline bool Run() {
    using namespace selfiestick;
    ResetPropTrackingState();
    const EntityHandle handle{(1u << 15u) | 10u};
    ProjectileKinematicSample motion{};
    motion.velocityValid = true;
    motion.velocity = {500, 0, 0};
    Vec3 raw{100, 200, 70};
    SmoothPropAnchor(handle, raw, motion, 640, 1.0);
    raw.x += 500.0f / 60.0f;
    for (int frame = 1; frame <= 12; ++frame) {
        const auto anchor = SmoothPropAnchor(handle, raw, motion, 640, 1.0 + frame / 60.0);
        if (Length(Subtract(anchor, raw)) > 0.001f) {
            std::printf("prop rigid-anchor FAILED: stationary render sample drifts %.3f units at frame %d\n",
                Length(Subtract(anchor, raw)), frame);
            ResetPropTrackingState();
            return false;
        }
    }
    for (const double fps : {30.0, 60.0, 144.0, 500.0}) {
        for (const float playback : {0.1f, 1.0f, 8.0f}) {
            for (const float legacySpeed : {1.0f, 640.0f, 100000.0f}) {
                ResetPropTrackingState();
                double wallTime = 10.0;
                for (int frame = 0; frame < 360; ++frame) {
                    // Repeat packets, pause, accelerate, drop a frame and seek
                    // back. The render sample is the only positional clock.
                    wallTime += frame % 80 == 0 ? 0.25 : 1.0 / fps;
                    const float t = frame * playback / static_cast<float>(fps);
                    const Vec3 sample{100 + 500 * t, 200 + 25 * std::sin(t), 70 + 10 * std::cos(t)};
                    motion.velocity = frame % 3 == 0 ? Vec3{1000, -300, 200} : Vec3{};
                    motion.bounceCountValid = true;
                    motion.bounceCount = frame / 80;
                    const auto result = SmoothPropAnchor(handle, sample, motion, legacySpeed, wallTime);
                    if (Length(Subtract(result, sample)) > 0.001f) return false;
                    for (int repeat = 0; repeat < 3; ++repeat) {
                        if (Length(Subtract(SmoothPropAnchor(handle, sample, motion, legacySpeed,
                                wallTime + repeat * 0.04), sample)) > 0.001f) return false;
                    }
                }
            }
        }
    }
    ResetPropTrackingState();
    puts("prop render-rig regression passed: no prediction drift at 30/60/144/500 FPS, 0.1/1/8x playback, repeated packets, pause, frame gaps, bounces and legacy follow-speed values");
    return true;
}
}
