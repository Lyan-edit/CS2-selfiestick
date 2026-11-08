#pragma once

// Actual discovery, exact-handle access and camera code, with fake entities in
// this hidden test EXE. No game process, injected hook or live demo is needed.
namespace prop_checks {
class C_FlashbangProjectile { public: virtual ~C_FlashbangProjectile() = default; unsigned char data[0x2000]{}; };
class C_HEGrenadeProjectile { public: virtual ~C_HEGrenadeProjectile() = default; unsigned char data[0x2000]{}; };
class C_MolotovProjectile { public: virtual ~C_MolotovProjectile() = default; unsigned char data[0x2000]{}; };
class C_SmokeGrenadeProjectile { public: virtual ~C_SmokeGrenadeProjectile() = default; unsigned char data[0x2000]{}; };
class C_DecoyProjectile { public: virtual ~C_DecoyProjectile() = default; unsigned char data[0x2000]{}; };
class C_BaseCSGrenadeProjectile { public: virtual ~C_BaseCSGrenadeProjectile() = default; unsigned char data[0x2000]{}; };
class WorldEntity { public: virtual ~WorldEntity() = default; };
inline std::array<void*, selfiestick::kMaxPropEntitySlots> entities{};
inline int highestIndex = 8191;
inline int reportedCountOverride = -1;
inline double lookupDelaySeconds{};
inline unsigned int lookupCount{};
inline void* __fastcall EntityAtIndex(void*, int index) {
    ++lookupCount;
    if (lookupDelaySeconds > 0) {
        const double finish = selfiestick::HighResolutionSeconds() + lookupDelaySeconds;
        while (selfiestick::HighResolutionSeconds() < finish) {}
    }
    return index >= 0 && index <= highestIndex ? entities[static_cast<std::size_t>(index)] : nullptr;
}
inline int __fastcall HighestIndex(void*, bool) { return reportedCountOverride >= 0 ? reportedCountOverride : highestIndex; }
template<class T> struct FakeEntity {
    T object;
    unsigned char identity[64]{}, scene[512]{};
    selfiestick::EntityHandle handle{};
    void SetHandle(int index, unsigned int serial) {
        handle = {(serial << 15u) | static_cast<unsigned int>(index)};
        hand_checks::Store(identity, 0x10, handle.raw);
    }
    void Init(int index, unsigned int serial, selfiestick::EntityHandle owner = {}) {
        using namespace selfiestick;
        hand_checks::Store(&object, 0x10, static_cast<void*>(identity));
        hand_checks::Store(&object, 0x330, static_cast<void*>(scene));
        hand_checks::Store(scene, 0xC8, Vec3{150, 200, 70});
        SetHandle(index, serial);
        hand_checks::Store(&object, 0x400, Vec3{500, 0, 100});
        hand_checks::Store(&object, 0x410, Vec3{500, 0, 100});
        hand_checks::Store(&object, 0x420, owner);
        hand_checks::Store(&object, 0x424, EntityHandle{});
        hand_checks::Store(&object, 0x428, EntityHandle{});
        hand_checks::Store(&object, 0x440, 100.0f);
        hand_checks::Store(&object, 0x450, 0);
        hand_checks::Store(&object, 0x454, std::uint8_t{});
        hand_checks::Store(&object, 0x460, 0);
        hand_checks::Store(&object, 0x464, std::uint8_t{});
        hand_checks::Store(&object, 0x468, 0);
    }
    void Velocity(selfiestick::Vec3 value) { hand_checks::Store(&object, 0x400, value); }
    void SpawnTime(float value) { hand_checks::Store(&object, 0x440, value); }
    void Spawn() { entities[static_cast<std::size_t>(handle.Index())] = &object; }
    void End() { entities[static_cast<std::size_t>(handle.Index())] = nullptr; }
};

inline bool Run() {
    using namespace selfiestick;
    const auto oldOffsets = g_offsets;
    const auto oldModule = g_clientModule;
    const auto oldList = g_entityListStorage;
    const auto oldGet = g_getEntityFromIndex;
    const auto oldHighest = g_getHighestEntityIndex;
    const auto oldState = g_state;
    auto oldScan = std::make_unique<PropCandidateScanState>(std::move(g_propCandidateScan));
    g_propCandidateScan.entityList = nullptr;
    g_propCandidateScan.slots.fill({});
    g_clientModule = GetModuleHandleW(nullptr);
    void* list = reinterpret_cast<void*>(2);
    g_entityListStorage = &list;
    g_getEntityFromIndex = EntityAtIndex;
    g_getHighestEntityIndex = HighestIndex;
    g_offsets = {};
    g_offsets.entityInstanceToIdentity = 0x10;
    g_offsets.sceneNode = 0x330;
    g_offsets.sceneAbsOrigin = 0xC8;
    g_offsets.pawnControllerHandle = 0x800;
    g_offsets.projectileVelocity = 0x400;
    g_offsets.projectileInitialVelocity = 0x410;
    g_offsets.projectileThrower = 0x420;
    g_offsets.projectileOriginalThrower = 0x424;
    g_offsets.projectileOwnerEntity = 0x428;
    g_offsets.projectileSpawnTime = 0x440;
    g_offsets.projectileExplodeTick = 0x450;
    g_offsets.projectileExplodeBegan = 0x454;
    g_offsets.smokeEffectTick = 0x460;
    g_offsets.smokeEffectBegan = 0x464;
    g_offsets.decoyShotTick = 0x468;
    g_offsets.identityDesignerName = 0x20;
    g_offsets.entitySimulationTime = 0x470;
    FakeEntity<hand_checks::C_CSPlayerPawn> pawn;
    pawn.Init(42, 6);
    const EntityHandle controller{(7u << 15u) | 43u};
    hand_checks::Store(&pawn.object, 0x800, controller);
    // The eye path safely falls back to a valid scene origin in this test.
    void** originalVtable = *reinterpret_cast<void***>(&pawn.object);
    void* table[176]{};
    table[0] = originalVtable[-1];
    table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&hand_checks::Eye);
    *reinterpret_cast<void***>(&pawn.object) = table + 1;
    hand_checks::eyeMode = hand_checks::EyeMode::NoWrite;
    entities.fill(nullptr);
    pawn.Spawn();
    g_state = {};
    g_state.enabled = true;
    g_state.cameraMode = CameraMode::PropSelfie;
    g_state.currentFollowHandle = pawn.handle;
    FakeEntity<C_FlashbangProjectile> flash;
    FakeEntity<C_HEGrenadeProjectile> he;
    FakeEntity<C_MolotovProjectile> fire;
    FakeEntity<C_SmokeGrenadeProjectile> smoke;
    FakeEntity<C_DecoyProjectile> decoy;
    flash.Init(3, 1, pawn.handle);
    he.Init(5, 1, pawn.handle);
    fire.Init(7, 1, pawn.handle);
    smoke.Init(9, 1, pawn.handle);
    decoy.Init(11, 1, pawn.handle);
    smoke.Velocity({}); smoke.Spawn();
    decoy.Velocity({}); decoy.Spawn();
    int stage = 1;
    std::string failure;
    Vec3 position{}, angles{};
    const auto update = [&](double now = HighResolutionSeconds()) {
        UpdatePropCandidatesForTarget(&pawn.object, pawn.handle, now);
    };
    const auto pose = [&](float followSpeed = kDefaultPropFollowSpeed) {
        const auto selected = GetPropSelection();
        const Vec3 eye{150, 200, 134};
        return TryBuildPropCameraPose(&pawn.object, pawn.handle, selected.handle,
            selected.throwerHandle, selected.kind, &eye, {0, -13.5f, 2.25f}, {},
            followSpeed, position, angles, failure);
    };
    const bool passed = [&]() {
        // Baseline: lingering stationary smoke/decoy with nonzero INITIAL
        // velocity must not capture the camera. Warm the slot cache fairly.
        for (int i = 0; i < 128 && !g_propCandidateScan.slots[8191].observed; ++i) update();
        if (!g_propCandidateScan.slots[8191].observed || GetPropSelection().handle.IsValid()) return false;

        stage = 2;
        // One-frame flash, low index (far outside the old high-128 tail),
        // current velocity not networked yet. Discover + build pose NOW.
        flash.Velocity({}); flash.Spawn();
        g_propCandidateScan.nextEntityIndex = 0;
        update();
        auto selected = GetPropSelection();
        if (selected.handle.raw != flash.handle.raw || selected.kind != PropProjectileKind::Flash
            || selected.throwerHandle.raw != pawn.handle.raw || !pose()) return false;
        flash.End();
        update();
        if (GetPropSelection().handle.IsValid() || pose() || g_propSmoothing.valid
            || g_propOriginHistory.valid || g_propCameraReference.valid || g_propLookTarget.valid) return false;

        stage = 3;
        // Released shot must yield to a short HE, with owner+kind updated
        // together in the same frame (the prior config held Flash metadata).
        he.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update();
        selected = GetPropSelection();
        if (selected.handle.raw != he.handle.raw || selected.kind != PropProjectileKind::Explosive
            || selected.manual || !pose()) return false;

        stage = 4;
        PropCandidate manual{};
        if (!IsPropProjectileForTarget(&he.object, pawn.handle, controller, manual)) return false;
        SetLockedProp(manual);
        fire.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.raw != he.handle.raw || !GetPropSelection().manual) return false;
        he.Velocity({}); update();
        if (GetPropSelection().handle.raw != he.handle.raw || !pose()) return false;
        he.End(); g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.raw != fire.handle.raw || GetPropSelection().manual || !pose()) return false;

        stage = 5;
        // Recycle the exact same address/vtable/index. Only serial changes.
        const EntityHandle stale = fire.handle;
        fire.SetHandle(7, 2);
        g_propCandidateScan.nextEntityIndex = 0; update();
        if (ResolveHandleToEntity(stale) != nullptr || GetPropSelection().handle.raw != fire.handle.raw
            || !pose() || g_propSmoothing.propHandle.raw != fire.handle.raw) return false;

        stage = 6;
        // Owner arrives one frame late via controller/original-thrower.
        flash.SetHandle(3, 2);
        hand_checks::Store(&flash.object, 0x420, EntityHandle{123456u});
        flash.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.raw != fire.handle.raw) return false;
        hand_checks::Store(&flash.object, 0x424, controller);
        g_propCandidateScan.nextEntityIndex = 0; update();
        selected = GetPropSelection();
        if (selected.handle.raw != flash.handle.raw || selected.throwerHandle.raw != controller.raw
            || selected.kind != PropProjectileKind::Flash || !pose()) return false;
        flash.End(); fire.End(); ClearLockedProp();

        stage = 7;
        // Initial-velocity grace is bounded; a late owner must not turn an
        // already stationary projectile into an indefinite moving candidate.
        const double now = HighResolutionSeconds();
        he.SetHandle(5, 2); he.Velocity({});
        hand_checks::Store(&he.object, 0x420, EntityHandle{});
        he.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update(now);
        hand_checks::Store(&he.object, 0x428, pawn.handle);
        g_propCandidateScan.nextEntityIndex = 0; update(now + 0.2);
        if (GetPropSelection().handle.IsValid()) return false;
        he.Velocity({500, 0, 0});
        g_propCandidateScan.nextEntityIndex = 0; update(now + 0.21);
        if (GetPropSelection().handle.raw != he.handle.raw || !pose()) return false;
        he.End();

        stage = 8;
        update();
        if (GetPropSelection().handle.IsValid()) return false;
        // A clear intentionally suppresses existing shots. This is a NEW throw.
        smoke.SetHandle(9, 2); smoke.Velocity({500, 0, 0}); g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().kind != PropProjectileKind::Smoke || !pose()) return false;
        decoy.SetHandle(11, 2); decoy.Velocity({500, 0, 0});
        g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().kind != PropProjectileKind::Decoy || !pose()) return false;
        smoke.End(); decoy.End(); ClearLockedProp();
        // Player mode and disabled mode must never acquire any prop.
        flash.SetHandle(3, 3); flash.Velocity({500, 0, 0}); flash.Spawn();
        g_state.cameraMode = CameraMode::PlayerSelfie; update();
        if (GetPropSelection().handle.IsValid()) return false;
        g_state.cameraMode = CameraMode::PropSelfie; g_state.enabled = false; update();
        if (GetPropSelection().handle.IsValid()) return false;
        flash.End(); g_state.enabled = true;

        stage = 11;
        // Regression: late ownership outside the 100 ms birth window must
        // still replace the previous AUTOMATIC live lock.
        fire.SetHandle(7, 3); fire.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update();
        he.SetHandle(5, 3); he.Velocity({500, 0, 0});
        for (const auto offset : {0x420, 0x424, 0x428}) hand_checks::Store(&he.object, offset, EntityHandle{});
        he.Spawn();
        const double delayedOwnerTime = HighResolutionSeconds();
        g_propCandidateScan.nextEntityIndex = 0; update(delayedOwnerTime);
        hand_checks::Store(&he.object, 0x428, pawn.handle);
        g_propCandidateScan.nextEntityIndex = 0; update(delayedOwnerTime + 0.2);
        if (GetPropSelection().handle.raw != he.handle.raw) return false;
        he.End(); fire.End(); ClearLockedProp();

        stage = 12;
        // Priority slots must capture a one-frame recycled flash even while
        // the resumable full sweep is thousands of slots away and slow.
        fire.Init(7, 50, pawn.handle); fire.SpawnTime(500); fire.Spawn();
        g_propCandidateScan.nextEntityIndex = 0; update();
        flash.Init(3, 50, pawn.handle); flash.SpawnTime(501); flash.Spawn();
        g_propCandidateScan.nextEntityIndex = 4096;
        lookupDelaySeconds = 0.00001;
        update();
        lookupDelaySeconds = 0;
        if (GetPropSelection().handle.raw != flash.handle.raw || !pose()) return false;
        // The older projectile remains alive, including a nonzero velocity.
        // Ending the newer shot must NOT select the older one again.
        hand_checks::Store(&flash.object, 0x450, 100);
        hand_checks::Store(&flash.object, 0x454, std::uint8_t{1});
        flash.End();
        update();
        if (GetPropSelection().handle.IsValid() || g_propSmoothing.valid) return false;
        update();
        if (GetPropSelection().handle.IsValid()) return false;
        flash.End(); fire.End();

        stage = 13;
        int terminalCase = 0;
        const auto terminal = [&](auto& item, int index, int offset, bool byte) {
            item.Init(index, 70u + ++terminalCase, pawn.handle);
            item.SpawnTime(520.0f + terminalCase); item.Spawn();
            g_propCandidateScan.nextEntityIndex = 0; update();
            if (GetPropSelection().handle.raw != item.handle.raw || !pose()) return false;
            PropCandidate pin{};
            if (!IsPropProjectileForTarget(&item.object, pawn.handle, controller, pin)) return false;
            SetLockedProp(pin);
            if (!pose()) return false;
            if (byte) hand_checks::Store(&item.object, offset, std::uint8_t{1});
            else hand_checks::Store(&item.object, offset, 100);
            // Entity still resolves at the same address with speed 500.
            if (ResolveHandleToEntity(item.handle) != &item.object) return false;
            const bool actualEffect = byte || offset == 0x468;
            if (HasPropProjectileEnded(&item.object, GetPropSelection().kind) != actualEffect) return false;
            // Without a retained view, removing the entity still clears a lock.
            item.End();
            update();
            const auto cleared = GetPropSelection();
            if (cleared.handle.IsValid() || cleared.throwerHandle.IsValid() || cleared.manual
                || cleared.kind != PropProjectileKind::Unknown || g_propSmoothing.valid
                || g_propOriginHistory.valid || g_propCameraReference.valid || g_propLookTarget.valid) return false;
            update();
            if (GetPropSelection().handle.IsValid()) return false;
            item.End();
            return true;
        };
        if (!terminal(flash, 3, 0x450, false) || !terminal(he, 5, 0x454, true)
            || !terminal(fire, 7, 0x454, true) || !terminal(smoke, 9, 0x460, false)
            || !terminal(smoke, 9, 0x464, true) || !terminal(decoy, 11, 0x468, false)) {
            std::printf("terminal subcase %d failed\n", terminalCase);
            return false;
        }

        stage = 14;
        // Shared base RTTI: designer name can arrive late and differs between
        // entities sharing one vtable. Never cache its concrete kind by vtable.
        FakeEntity<C_BaseCSGrenadeProjectile> genericFlash, genericHe;
        genericFlash.Init(13, 90, pawn.handle); genericFlash.SpawnTime(600); genericFlash.Spawn();
        g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.IsValid()) return false;
        hand_checks::Store(genericFlash.identity, 0x20, static_cast<const char*>("flashbang_projectile"));
        g_propCandidateScan.nextEntityIndex = 4096; update();
        if (GetPropSelection().handle.raw != genericFlash.handle.raw
            || GetPropSelection().kind != PropProjectileKind::Flash || !pose()) return false;
        genericHe.Init(15, 90, pawn.handle); genericHe.SpawnTime(601);
        hand_checks::Store(genericHe.identity, 0x20, static_cast<const char*>("hegrenade_projectile"));
        genericHe.Spawn(); g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.raw != genericHe.handle.raw
            || GetPropSelection().kind != PropProjectileKind::Explosive || !pose()) return false;
        genericHe.SetHandle(15, 91); genericHe.SpawnTime(602);
        hand_checks::Store(genericHe.identity, 0x20, static_cast<const char*>("decoy_projectile"));
        update();
        if (GetPropSelection().kind != PropProjectileKind::Decoy || !pose()) return false;
        genericFlash.End(); genericHe.End(); update();

        stage = 15;
        // No network velocity at all: observed scene movement is sufficient.
        he.Init(5, 100, pawn.handle); he.SpawnTime(610); he.Velocity({});
        hand_checks::Store(&he.object, 0x410, Vec3{});
        he.Spawn(); update();
        if (GetPropSelection().handle.IsValid()) return false;
        hand_checks::Store(he.scene, 0xC8, Vec3{160, 200, 70});
        g_propCandidateScan.nextEntityIndex = 4096; update();
        if (GetPropSelection().handle.raw != he.handle.raw || !pose()) return false;
        he.End(); update();

        stage = 16;
        // A demo seek back must reset chronology, so lower spawn timestamps
        // in the replay can still be acquired. Pause alone must not reset it.
        hand_checks::Store(&pawn.object, 0x470, 700.0f); update();
        hand_checks::Store(&pawn.object, 0x470, 10.0f);
        he.Init(5, 101, pawn.handle); he.SpawnTime(20); he.Spawn();
        update();
        if (GetPropSelection().handle.raw != he.handle.raw || !pose()) return false;
        fire.Init(7, 101, pawn.handle); fire.SpawnTime(19); fire.Spawn();
        update();
        if (GetPropSelection().handle.raw != he.handle.raw) return false;
        he.End(); fire.End(); ClearLockedProp();

        stage = 17;
        // Pose failure releases the exact current lock, but a transient first
        // frame failure must not permanently consume this throw's eligibility.
        flash.Init(3, 110, pawn.handle); flash.SpawnTime(22); flash.Spawn();
        update();
        if (GetPropSelection().handle.raw != flash.handle.raw || !pose()) return false;
        ClearLockedProp(flash.handle, true); update();
        if (GetPropSelection().handle.raw != flash.handle.raw || !pose()) return false;
        ClearLockedProp(he.handle, true); // stale failure must not clear a new lock
        if (GetPropSelection().handle.raw != flash.handle.raw) return false;
        flash.End(); update();

        stage = 18;
        highestIndex = 32767; reportedCountOverride = 32;
        he.Init(25000, 200, pawn.handle); he.SpawnTime(24); he.Spawn();
        g_propCandidateScan.nextEntityIndex = 25000;
        update();
        if (GetPropSelection().handle.raw != he.handle.raw || !pose()) return false;
        he.End(); update();

        stage = 19;
        // Newly allocated high slot, not in priority history and far from the
        // sweep: a one-frame birth event must capture it despite a count of 32.
        flash.Init(28000, 201, pawn.handle); flash.SpawnTime(25); flash.Velocity({}); flash.Spawn();
        g_propCandidateScan.nextEntityIndex = 0;
        RecordPropEntityBirth(flash.handle);
        lookupDelaySeconds = 0.00001;
        update(); lookupDelaySeconds = 0;
        if (GetPropSelection().handle.raw != flash.handle.raw || !pose()) return false;
        flash.End(); update();
        flash.Init(31000, 202, pawn.handle); flash.SpawnTime(26); flash.Spawn();
        const EntityHandle staleBirth{(201u << 15u) | 31000u};
        RecordPropEntityBirth(staleBirth); g_propCandidateScan.nextEntityIndex = 0;
        lookupDelaySeconds = 0.00001;
        update(); lookupDelaySeconds = 0;
        if (GetPropSelection().handle.IsValid()) return false;
        RecordPropEntityBirth(flash.handle);
        update();
        if (GetPropSelection().handle.raw != flash.handle.raw || !pose()) return false;
        flash.End(); update();
        reportedCountOverride = -1;

        stage = 20;
        // Production camera math: constant prop-relative distance and center
        // through fast throws, bounce metadata, packet repeats, pause/resume,
        // legacy follow speeds and a later mutation of initial-velocity data.
        he.Init(5, 210, pawn.handle); he.SpawnTime(30); he.Spawn();
        g_propCandidateScan.nextEntityIndex = 0; update();
        const float expectedDistance = std::sqrt(13.5f * 13.5f + 2.25f * 2.25f);
        for (const float speed : {1.0f, 640.0f, 100000.0f}) {
            ResetPropTrackingState();
            hand_checks::Store(&he.object, 0x410, Vec3{500, 0, 100});
            Vec3 expectedRelative{};
            for (int frame = 0; frame < 600; ++frame) {
                const Vec3 raw{150.0f + frame * 3.0f, 200.0f + 25.0f * std::sin(frame * 0.03f),
                    70.0f + 10.0f * std::cos(frame * 0.05f)};
                hand_checks::Store(he.scene, 0xC8, raw);
                if (frame > 100) hand_checks::Store(&he.object, 0x410, Vec3{0, -500, 100});
                if (!pose(speed)) return false;
                const Vec3 relative = Subtract(position, raw);
                if (frame == 0) expectedRelative = relative;
                Vec3 toward = Scale(relative, -1.0f);
                if (!TryNormalize(toward) || std::fabs(Length(relative) - expectedDistance) > 0.002f
                    || Length(Subtract(relative, expectedRelative)) > 0.002f) return false;
                constexpr float radians = 3.14159265358979323846f / 180.0f;
                const Vec3 direction{std::cos(angles.x * radians) * std::cos(angles.y * radians),
                    std::cos(angles.x * radians) * std::sin(angles.y * radians), -std::sin(angles.x * radians)};
                if (Dot(direction, toward) < 0.99999f) return false;
                if ((frame % 40) == 0) {
                    const Vec3 pausedPosition = position;
                    for (int repeat = 0; repeat < 8; ++repeat)
                        if (!pose(speed) || Length(Subtract(position, pausedPosition)) > 0.0001f) return false;
                }
            }
        }
        he.End(); update();

        stage = 21;
        double demoSeconds = -1.0;
        const auto stablePose = [&](double now = HighResolutionSeconds(), bool detached = false) {
            const auto shot = GetPropSelection();
            const Vec3 eye{150, 200, 134};
            return TryBuildStablePropView(detached ? nullptr : &pawn.object, pawn.handle,
                shot, &eye, {0, -13.5f, 2.25f}, {}, kDefaultPropFollowSpeed,
                position, angles, failure, now, demoSeconds);
        };
        fire.Init(7, 300, pawn.handle); fire.SpawnTime(40); fire.Spawn();
        g_propCandidateScan.nextEntityIndex = 0; update();
        if (!stablePose(100) || g_state.propViewPhase != PropViewPhase::Tracking) return false;
        // Landing and a scheduled future effect tick are not detonation.
        fire.Velocity({}); hand_checks::Store(&fire.object, 0x450, 99999);
        hand_checks::Store(fire.scene, 0xC8, Vec3{170, 200, 70});
        update();
        if (!stablePose(100.016) || g_state.propViewPhase != PropViewPhase::Tracking
            || HasPropEffectStarted(&fire.object, PropProjectileKind::Fire)) return false;
        const Vec3 landedPosition = position;
        // A missing scene pointer holds the view instead of observer/prop flicker.
        hand_checks::Store(&fire.object, 0x330, static_cast<void*>(nullptr));
        update();
        if (!stablePose(100.032) || g_state.propViewPhase != PropViewPhase::Recovering
            || Length(Subtract(position, landedPosition)) > 0.001f) return false;
        hand_checks::Store(&fire.object, 0x330, static_cast<void*>(fire.scene));
        update();
        if (!stablePose(100.048) || g_state.propViewPhase != PropViewPhase::Tracking) return false;
        for (const auto offset : {0x420, 0x424, 0x428}) hand_checks::Store(&fire.object, offset, EntityHandle{});
        update();
        if (!stablePose(100.064) || g_state.propViewPhase != PropViewPhase::Recovering) return false;
        // Even a longer data gap must be allowed to recover, not end the shot.
        if (!stablePose(101) || SnapshotPropView().terminal) return false;
        hand_checks::Store(&fire.object, 0x420, pawn.handle); update();
        if (!stablePose(101.016) || g_state.propViewPhase != PropViewPhase::Tracking) return false;
        for (int sample = 0; sample < 40; ++sample) {
            hand_checks::Store(&pawn.object, 0x470, sample % 2 ? 9.984f : 10.0f);
            update();
            if (!stablePose(101.032 + sample * 0.016) || !SnapshotPropView().valid
                || Length(Subtract(position, landedPosition)) > 0.001f) return false;
        }

        stage = 22;
        unsigned char viewSetup[0x500]{};
        hand_checks::Store(viewSetup, kViewSetupFovOffset, 90.0f);
        if (!ApplyPropViewFov(viewSetup, fire.handle)) return false;
        for (const float hostFov : {30.0f, 110.0f, NAN, 0.0f}) {
            if (!stablePose()) return false;
            hand_checks::Store(viewSetup, kViewSetupFovOffset, hostFov);
            if (!ApplyPropViewFov(viewSetup, fire.handle)) return false;
            float fov{}; TryReadOffsetValue(viewSetup, kViewSetupFovOffset, fov);
            if (fov != 90.0f) return false;
        }
        hand_checks::Store(&fire.object, 0x454, std::uint8_t{1});
        update();
        demoSeconds = 100.0;
        if (!stablePose(102) || g_state.propViewPhase != PropViewPhase::EffectHold) return false;
        const Vec3 fireEffectView = position;
        fire.End();
        for (int frame = 0; frame < 60; ++frame) {
            // Long wall-clock pauses cannot consume any of the 2 demo seconds.
            update();
            if (!stablePose(102.016 + frame) || g_state.propViewPhase != PropViewPhase::EffectHold
                || Length(Subtract(position, fireEffectView)) > 0.001f) return false;
        }
        // An automatic retained effect view never blocks the next short throw.
        he.Init(5, 301, pawn.handle); he.SpawnTime(41); he.Spawn();
        hand_checks::Store(&he.object, 0x330, static_cast<void*>(nullptr));
        demoSeconds = 101.0; // next shot preempts before the 2-second deadline
        RecordPropEntityBirth(he.handle); update();
        if (GetPropSelection().handle.raw != he.handle.raw || !stablePose(170)) return false;
        if (g_state.propViewPhase != PropViewPhase::Recovering
            || Length(Subtract(position, fireEffectView)) > 0.001f) return false;
        hand_checks::Store(viewSetup, kViewSetupFovOffset, 30.0f);
        if (!ApplyPropViewFov(viewSetup, he.handle)) return false;
        float pendingFov{}; TryReadOffsetValue(viewSetup, kViewSetupFovOffset, pendingFov);
        if (pendingFov != 90.0f) return false;
        hand_checks::Store(&he.object, 0x330, static_cast<void*>(he.scene));
        update();
        if (!stablePose(170.008) || g_state.propViewPhase != PropViewPhase::Tracking) return false;
        hand_checks::Store(viewSetup, kViewSetupFovOffset, 80.0f);
        if (!ApplyPropViewFov(viewSetup, he.handle)) return false;
        float freshFov{}; TryReadOffsetValue(viewSetup, kViewSetupFovOffset, freshFov);
        if (freshFov != 80.0f) return false;

        stage = 23;
        const Vec3 beforePin = position;
        SetPropPinned(true);
        if (!GetPropSelection().manual || !stablePose(170.016)
            || Length(Subtract(position, beforePin)) > 0.001f) return false;
        // Clicking the already selected list row also must not reset the rig.
        PropCandidate same{};
        if (!IsPropProjectileForTarget(&he.object, pawn.handle, controller, same)) return false;
        SetLockedProp(same);
        if (!SnapshotPropView().valid || !stablePose(170.032)) return false;
        hand_checks::Store(&he.object, 0x454, std::uint8_t{1});
        if (!stablePose(170.048)) return false;
        const Vec3 pinnedEffect = position;
        he.End();
        smoke.Init(9, 302, pawn.handle); smoke.SpawnTime(42); smoke.Spawn();
        RecordPropEntityBirth(smoke.handle);
        for (int frame = 0; frame < 60; ++frame) {
            demoSeconds = 102.0 + frame;
            update();
            if (GetPropSelection().handle.raw != he.handle.raw || !GetPropSelection().manual
                || !stablePose(171 + frame) || Length(Subtract(position, pinnedEffect)) > 0.001f) return false;
        }
        SetPropPinned(false); update();
        if (GetPropSelection().handle.raw != smoke.handle.raw || GetPropSelection().manual
            || !stablePose(240) || g_state.propViewPhase != PropViewPhase::Tracking) return false;

        stage = 24;
        SetPropPinned(true); pawn.End();
        hand_checks::Store(smoke.scene, 0xC8, Vec3{180, 200, 70});
        if (!stablePose(240.016, true)) return false;
        hand_checks::Store(&smoke.object, 0x464, std::uint8_t{1});
        if (!stablePose(240.032, true) || g_state.propViewPhase != PropViewPhase::EffectHold) return false;
        smoke.End();
        if (!stablePose(300, true)) return false;
        pawn.Spawn();
        ClearLockedProp();
        if (SnapshotPropView().valid || g_state.propViewPhase != PropViewPhase::Waiting) return false;

        stage = 25;
        // AUTO returns exactly after 128 demo ticks, including slow playback.
        // The production caller must return before touching native pose/FOV.
        flash.Init(3, 400, pawn.handle); flash.SpawnTime(43); flash.Spawn();
        RecordPropEntityBirth(flash.handle); update();
        demoSeconds = 200.0;
        if (!stablePose(400)) return false;
        hand_checks::Store(&flash.object, 0x454, std::uint8_t{1});
        if (!stablePose(400)) return false;
        const Vec3 finalView = position;
        flash.End();
        for (int tick = 0; tick < 128; ++tick) {
            demoSeconds = 200.0 + tick / 64.0;
            update();
            if (!stablePose(400 + tick * (10.0 / 64.0))
                || Length(Subtract(position, finalView)) > 0.001f) return false;
        }
        demoSeconds = 202.0;
        if (stablePose(420) || GetPropSelection().handle.IsValid() || SnapshotPropView().valid
            || !g_state.enabled || g_state.cameraMode != CameraMode::PropSelfie) return false;
        hand_checks::Store(viewSetup, kViewSetupFovOffset, 72.0f);
        if (ApplyPropViewFov(viewSetup, flash.handle)) return false;
        float nativeFov{}; TryReadOffsetValue(viewSetup, kViewSetupFovOffset, nativeFov);
        if (nativeFov != 72.0f) return false;
        // Even stale live metadata for the released handle cannot reacquire it.
        hand_checks::Store(&flash.object, 0x454, std::uint8_t{}); flash.Spawn();
        update();
        if (GetPropSelection().handle.IsValid()) return false;
        flash.End();

        stage = 26;
        he.Init(5, 401, pawn.handle); he.SpawnTime(44); he.Spawn();
        RecordPropEntityBirth(he.handle); update();
        if (GetPropSelection().handle.raw != he.handle.raw || !stablePose(421)) return false;
        SetPropPinned(true);
        smoke.Init(9, 401); smoke.SpawnTime(45); smoke.Spawn();
        RecordPropEntityBirth(smoke.handle); update();
        if (GetPropSelection().handle.raw != he.handle.raw) return false;
        // The lower target-clear button also releases the prop camera.
        g_state.targetMode = TargetMode::Locked; g_state.lockedHandle = pawn.handle;
        ClearLockedTarget();
        // Ownership arriving after clear must not resurrect an existing shot.
        hand_checks::Store(&smoke.object, 0x420, pawn.handle);
        for (int repeat = 0; repeat < 8; ++repeat) {
            update();
            if (GetPropSelection().handle.IsValid() || SnapshotPropView().valid
                || g_state.targetMode != TargetMode::Follow) return false;
        }
        // Row selection explicitly reopens a cleared live prop and pins it.
        PropCandidate row{};
        if (!IsPropProjectileForTarget(&smoke.object, pawn.handle, controller, row)) return false;
        SetLockedProp(row); update();
        if (GetPropSelection().handle.raw != smoke.handle.raw || !GetPropSelection().manual || !stablePose(422)) return false;
        ClearLockedProp(); update();
        if (GetPropSelection().handle.IsValid()) return false;
        flash.Init(3, 402, pawn.handle); flash.SpawnTime(46); flash.Spawn();
        RecordPropEntityBirth(flash.handle); update();
        if (GetPropSelection().handle.raw != flash.handle.raw || !stablePose(423)) return false;
        // Losing the clock never silently falls back to wall time.
        demoSeconds = -1;
        hand_checks::Store(&flash.object, 0x454, std::uint8_t{1});
        if (!stablePose(424) || !stablePose(9000)) return false;
        demoSeconds = 1000;
        if (!stablePose(9001)) return false;
        demoSeconds = 1001.5;
        if (!stablePose(9002)) return false;
        demoSeconds = 500; // a seek rebaselines the hold rather than expiring it
        if (!stablePose(9003)) return false;
        demoSeconds = 501.984375;
        if (!stablePose(9004)) return false;
        SetPropPinned(true); demoSeconds = 510;
        if (!stablePose(9005)) return false;
        SetPropPinned(false);
        if (stablePose(9006) || GetPropSelection().handle.IsValid()) return false;
        flash.End(); he.End(); smoke.End(); ClearLockedProp();

        // A target pawn/controller replacement must not retain the old lock.
        fire.Init(7, 102, pawn.handle); fire.SpawnTime(47); fire.Spawn();
        g_propCandidateScan.nextEntityIndex = 0; update();
        if (GetPropSelection().handle.raw != fire.handle.raw) return false;
        pawn.SetHandle(42, 7);
        hand_checks::Store(&pawn.object, 0x800, EntityHandle{(8u << 15u) | 43u});
        g_state.currentFollowHandle = pawn.handle;
        update();
        if (GetPropSelection().handle.IsValid()) return false;
        fire.End();

        stage = 9;
        // A large populated list still has a strict work boundary. Inject a
        // deliberately slow entity lookup, then verify resume makes progress.
        WorldEntity world;
        entities.fill(&world); entities[42] = &pawn.object;
        highestIndex = 32767;
        g_propCandidateScan.nextEntityIndex = 0;
        update(); // warm World RTTI
        lookupDelaySeconds = 0.00001;
        lookupCount = 0;
        const int before = g_propCandidateScan.nextEntityIndex;
        update();
        const int after = g_propCandidateScan.nextEntityIndex;
        lookupDelaySeconds = 0;
        if (after == before || lookupCount > 128u) return false;

        stage = 10;
        // Record offline warm-cache timings, not in-game FPS. Max slot count.
        std::vector<double> timings;
        for (int i = 0; i < 500; ++i) {
            const double start = HighResolutionSeconds(); update();
            timings.push_back((HighResolutionSeconds() - start) * 1000.0);
        }
        std::sort(timings.begin(), timings.end());
        std::printf("prop discovery offline 32768 populated slots: median %.3f ms, p95 %.3f ms, max %.3f ms (500 callbacks)\n",
            timings[250], timings[475], timings.back());
        return true;
    }();
    lookupDelaySeconds = 0;
    reportedCountOverride = -1;
    *reinterpret_cast<void***>(&pawn.object) = originalVtable;
    entities.fill(nullptr);
    ResetPropTrackingState();
    g_offsets = oldOffsets;
    g_clientModule = oldModule;
    g_entityListStorage = oldList;
    g_getEntityFromIndex = oldGet;
    g_getHighestEntityIndex = oldHighest;
    g_state = oldState;
    g_propCandidateScan = std::move(*oldScan);
    if (passed) puts("prop follow regression passed: short-birth/sparse-slot capture, 1800 fixed-distance poses, scene/owner recovery, stable FOV, landing continues, auto quick handoff, 128-demo-tick effect hold, pause/slow playback, native return, no canceled-shot reacquire, row select+pin, indefinite pins and detached-target retention");
    else std::printf("prop follow regression FAILED stage %d: %s\n", stage, failure.c_str());
    return passed;
}
}
