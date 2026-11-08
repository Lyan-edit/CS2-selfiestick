#pragma once

namespace prop_origin_checks {
inline int engineCalls{};
inline const selfiestick::Vec3* __fastcall EngineGetAbsOrigin(void* entity) {
    using namespace selfiestick;
    void* scene = *reinterpret_cast<void**>(static_cast<unsigned char*>(entity) + 0x330);
    auto* bytes = static_cast<unsigned char*>(scene);
    ++engineCalls;
    if (bytes[kSceneAbsTransformDirtyOffset] != 0) {
        const Vec3 pending = *reinterpret_cast<Vec3*>(bytes + 0x140);
        hand_checks::Store(scene, 0xC8, pending);
        bytes[kSceneAbsTransformDirtyOffset] = 0;
    }
    return reinterpret_cast<Vec3*>(bytes + 0xC8);
}
inline const selfiestick::Vec3* __fastcall NullOrigin(void*) { return nullptr; }
inline const selfiestick::Vec3* __fastcall InvalidOrigin(void*) {
    static const selfiestick::Vec3 invalid{NAN, 100, 200};
    return &invalid;
}
inline bool Run() {
    using namespace selfiestick;
    const auto oldOffsets = g_offsets;
    const auto oldGetter = g_getEntityAbsOrigin;
    const bool oldKnown = g_knownClientSchemaFallbackAllowed.load();
    g_offsets.sceneNode = 0x330;
    g_offsets.sceneAbsOrigin = 0xC8;
    g_getEntityAbsOrigin = EngineGetAbsOrigin;
    prop_checks::FakeEntity<prop_checks::C_HEGrenadeProjectile> prop;
    prop.Init(12, 1);
    bool passed = true;
    std::string failure;
    for (int frame = 0; frame < 480; ++frame) {
        const Vec3 rendered{100 + frame * 0.5f, 200 + 5 * std::sin(frame * 0.03f), 70};
        const Vec3 stale = Add(rendered, Vec3{frame % 2 ? 15.0f : -15.0f, 0, 0});
        hand_checks::Store(prop.scene, 0xC8, stale);
        hand_checks::Store(prop.scene, 0x140, rendered);
        prop.scene[kSceneAbsTransformDirtyOffset] = 1;
        Vec3 origin{};
        if (!TryGetProjectileOriginWithFallback(prop.handle, &prop.object, {100, 200, 100}, origin, failure)
            || Length(Subtract(origin, rendered)) > 0.001f) {
            std::printf("dirty-scene origin FAILED at frame %d: cache error %.3f, engine calls %d\n",
                frame, Length(Subtract(origin, rendered)), engineCalls);
            passed = false;
            break;
        }
    }
    if (passed) {
        Vec3 result{};
        for (const auto getter : {EntityAbsOriginFn{NullOrigin}, EntityAbsOriginFn{InvalidOrigin}}) {
            g_getEntityAbsOrigin = getter;
            if (TryGetResolvedPropOrigin(&prop.object, result, failure)) passed = false;
        }
        g_getEntityAbsOrigin = nullptr;
        g_knownClientSchemaFallbackAllowed.store(true);
        if (TryGetResolvedPropOrigin(&prop.object, result, failure)) passed = false;
    }
    g_offsets = oldOffsets;
    g_getEntityAbsOrigin = oldGetter;
    g_knownClientSchemaFallbackAllowed.store(oldKnown);
    ResetPropTrackingState();
    if (passed) puts("dirty-scene origin regression passed: alternating stale caches resolved by engine getter across 480 samples");
    return passed;
}
}
