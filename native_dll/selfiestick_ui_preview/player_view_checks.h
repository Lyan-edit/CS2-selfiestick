#pragma once

namespace player_view_checks {
class C_WeaponM4A1 { public: virtual ~C_WeaponM4A1() = default; unsigned char data[0x2000]{}; };
class C_Knife { public: virtual ~C_Knife() = default; unsigned char data[0x2000]{}; };
inline std::array<void*, 128> entities{};
inline void* observer{};
inline void* __fastcall EntityAtIndex(void*, int index) { return index >= 0 && index < 128 ? entities[index] : nullptr; }
inline void* __fastcall SplitScreen(int) { return observer; }
inline bool __fastcall UnwrittenAttachment(void*, std::uint8_t, void*) { return true; }
inline bool __fastcall PartialAttachment(void*, std::uint8_t, void* data) { static_cast<float*>(data)[0] = 100; return true; }
inline selfiestick::Vec3 animatedEye{100, 200, 64};
inline int animatedEyeCalls{}, animatedHandCalls{};
inline void __fastcall AnimationEye(void*, float* output) { ++animatedEyeCalls; std::memcpy(output, &animatedEye, sizeof(animatedEye)); }
inline bool __fastcall AnimationAttachment(void* entity, std::uint8_t index, void* data) {
    ++animatedHandCalls; return hand_checks::Attachment(entity, index, data);
}
inline bool Run() {
    using namespace selfiestick;
    const auto oldOffsets = g_offsets;
    const auto oldModule = g_clientModule;
    const auto oldGetter = g_getEntityAbsOrigin;
    const auto oldLookup = g_lookupAttachment;
    const auto oldAttachment = g_getAttachment;
    const auto oldState = g_state;
    const auto oldList = g_entityListStorage;
    const auto oldGet = g_getEntityFromIndex;
    const auto oldSplit = g_getSplitScreenPlayer;
    const auto oldCandidates = g_getSplitScreenPlayerCandidates;
    const auto oldSuppression = g_viewModelSuppression;
    const auto oldView = g_playerView;
    const auto oldFrame = g_playerFrame;
    const bool oldSchema = g_schemaOffsetsResolved.load();
    const auto oldRefresh = g_lastFollowTargetRefreshTickMs.load();
    const auto oldGeneration = g_playerCameraGeneration.load();
    const auto oldClockReader = g_playerMotionClockReader;
    const auto oldTail = g_playerTailHookInstalled.load();
    const auto oldFovLatch = g_playerFovLatch;
    const bool oldKnown = g_knownClientSchemaFallbackAllowed.load();
    hand_checks::C_CSPlayerPawn pawn;
    void** originalVtable = *reinterpret_cast<void***>(&pawn);
    void* table[176]{};
    table[0] = originalVtable[-1];
    table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&hand_checks::Eye);
    *reinterpret_cast<void***>(&pawn) = table + 1;
    unsigned char identity[64]{}, scene[512]{}, movement[0x500]{};
    hand_checks::Store(&pawn, 0x1330, static_cast<void*>(movement));
    hand_checks::Store(&pawn, 0x10, static_cast<void*>(identity));
    hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
    hand_checks::Store(identity, 0x10, EntityHandle{(600u << 15u) | 42u});
    hand_checks::Store(&pawn, 0x3630, Vec3{});
    g_offsets.entityInstanceToIdentity = 0x10;
    g_offsets.sceneNode = 0x330;
    g_offsets.sceneAbsOrigin = 0xC8;
    g_offsets.pawnEyeAngles = 0x3630;
    g_clientModule = GetModuleHandleW(nullptr);
    g_getEntityAbsOrigin = prop_origin_checks::EngineGetAbsOrigin;
    g_knownClientSchemaFallbackAllowed.store(true);
    g_lookupAttachment = hand_checks::Lookup;
    g_getAttachment = hand_checks::Attachment;
    hand_checks::attachmentLive = true;
    hand_checks::attachment = {110, 188, 46};
    hand_checks::eyeMode = hand_checks::EyeMode::Valid;
    int stage = 1;
    std::string failure;
    const bool passed = [&]() {
        // Eye and hand are current, but the pawn's cached origin is dirty.
        // Production must resolve it BEFORE validation/fallback composition.
        for (int frame = 0; frame < 480; ++frame) {
            hand_checks::Store(scene, 0xC8, Vec3{100.0f + (frame % 2 ? 80.0f : -80.0f), 200, 0});
            hand_checks::Store(scene, 0x140, Vec3{100, 200, 0});
            scene[kSceneAbsTransformDirtyOffset] = 1;
            WeaponFrameBasis basis{};
            if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, basis, failure)
                || Length(Subtract(basis.upperBodyTarget, Vec3{100, 200, 58})) > 0.001f) {
                std::printf("player dirty-scene FAILED frame %d: body target error %.3f\n", frame,
                    Length(Subtract(basis.upperBodyTarget, Vec3{100, 200, 58})));
                return false;
            }
        }
        stage = 6;
        table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&AnimationEye);
        hand_checks::Store(scene, 0xC8, Vec3{100, 200, 0});
        WeaponFrameBasis neutral{};
        if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, neutral, failure)) return false;
        for (int sample = 1; sample <= 600; ++sample) {
            hand_checks::attachment = {110.0f + 16 * std::sin(sample * 0.2f),
                188.0f + 12 * std::sin(sample * 0.3f), 46.0f + 20 * std::sin(sample * 0.1f)};
            animatedEye = {100.0f + 4 * std::sin(sample * 0.2f),
                200.0f + 4 * std::cos(sample * 0.2f), 64.0f + 4 * std::sin(sample * 0.1f)};
            WeaponFrameBasis animated{};
            if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, animated, failure)
                || Length(Subtract(animated.anchorOrigin, neutral.anchorOrigin)) > 0.001f
                || Length(Subtract(animated.upperBodyTarget, neutral.upperBodyTarget)) > 0.001f) {
                std::printf("animated player anchor FAILED sample %d: hand/eye animation moved camera anchor %.3f units\n",
                    sample, Length(Subtract(animated.anchorOrigin, neutral.anchorOrigin)));
                return false;
            }
        }
        table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&hand_checks::Eye);
        hand_checks::attachment = {110, 188, 46};
        // Body lock must still follow real locomotion, turning and stance.
        for (int step = 0; step < 20; ++step) {
            const Vec3 root{100 + step * 10.0f, 200 + step * 2.0f, step * 1.0f};
            const float duck = (step % 5) * 0.25f;
            hand_checks::Store(scene, 0xC8, root);
            hand_checks::Store(movement, 0x40C, duck);
            hand_checks::Store(&pawn, 0x3630, Vec3{80, step * 90.0f, 0});
            WeaponFrameBasis moving{};
            if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, moving, failure)
                || Length(Subtract(moving.bodyOrigin, root)) > 0.001f
                || std::fabs(moving.upperBodyTarget.z - (root.z + 58 - 18 * duck)) > 0.001f) return false;
        }
        hand_checks::Store(scene, 0xC8, Vec3{100, 200, 0});
        hand_checks::Store(movement, 0x40C, 0.0f);
        hand_checks::Store(&pawn, 0x3630, Vec3{});
        stage = 2;
        for (const auto getter : {GetAttachmentFn{UnwrittenAttachment}, GetAttachmentFn{PartialAttachment}}) {
            g_getAttachment = getter;
            Vec3 unwritten{};
            if (TryGetAttachmentOrigin(&pawn, kAnchorAttachmentName, unwritten, failure)) return false;
        }
        g_getAttachment = hand_checks::Attachment;
        // Exercise the ENTIRE production callback, including target resolution,
        // pose writes, FOV and viewmodel suppression rather than pose math only.
        prop_checks::FakeEntity<C_WeaponM4A1> weapon;
        prop_checks::FakeEntity<C_Knife> knife;
        prop_checks::FakeEntity<hand_checks::C_CSPlayerPawn> observerPawn;
        weapon.Init(7, 900); knife.Init(8, 900); observerPawn.Init(43, 900);
        unsigned char weaponServices[128]{}, observerServices[128]{};
        const EntityHandle handle{(600u << 15u) | 42u};
        entities.fill(nullptr);
        entities[42] = &pawn; entities[7] = &weapon.object; entities[8] = &knife.object;
        entities[43] = &observerPawn.object;
        observer = &observerPawn.object;
        void* list = reinterpret_cast<void*>(100);
        g_entityListStorage = &list; g_getEntityFromIndex = EntityAtIndex;
        g_getSplitScreenPlayer = SplitScreen; g_getSplitScreenPlayerCandidates = {SplitScreen};
        g_offsets.pawnWeaponServices = 0x12F0; g_offsets.activeWeapon = 0x60;
        g_offsets.pawnObserverServices = 0x1308; g_offsets.observerTarget = 0x4C; g_offsets.observerMode = 0x48;
        g_offsets.renderWithViewModels = 0x56A;
        hand_checks::Store(&pawn, 0x12F0, static_cast<void*>(weaponServices));
        hand_checks::Store(&observerPawn.object, 0x1308, static_cast<void*>(observerServices));
        hand_checks::Store(observerServices, 0x4C, handle);
        hand_checks::Store(observerServices, 0x48, 4);
        hand_checks::Store(weaponServices, 0x60, weapon.handle);
        hand_checks::Store(&weapon.object, 0x56A, std::uint8_t{1});
        hand_checks::Store(&knife.object, 0x56A, std::uint8_t{1});
        hand_checks::Store(scene, 0xC8, Vec3{100, 200, 0});
        g_state = {}; g_state.enabled = true; g_state.cameraMode = CameraMode::PlayerSelfie;
        g_state.targetMode = TargetMode::Locked; g_state.lockedHandle = handle;
        g_schemaOffsetsResolved.store(true); g_viewModelSuppression = {};
        g_playerView = {}; g_playerFrame = {};
        hand_checks::eyeMode = hand_checks::EyeMode::Crouched;
        hand_checks::Store(movement, 0x40C, 1.0f);
        hand_checks::attachment = {110, 188, 28};
        table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&AnimationEye);
        g_getAttachment = AnimationAttachment;
        animatedEyeCalls = 0; animatedHandCalls = 0;
        const Vec3 native{-1000, 2000, -300};
        Vec3 position{}, angles{};
        float fov{};
        unsigned char view[0x900]{};
        const auto frame = [&](float hostFov = 90.0f) {
            hand_checks::Store(view, kViewSetupOriginOffset, native);
            hand_checks::Store(view, kViewSetupAnglesOffset, Vec3{1, 2, 3});
            hand_checks::Store(view, kViewSetupFovOffset, hostFov);
            SelfieStickSetUpView(view);
            TryReadViewSetupOrigin(view, position); TryReadViewSetupAngles(view, angles);
            TryReadOffsetValue(view, kViewSetupFovOffset, fov);
        };
        frame();
        if (!g_state.lastOverrideSucceeded || !g_viewModelSuppression.active || Length(Subtract(position, native)) < 1) return false;
        const Vec3 expectedPosition = position, expectedAngles = angles;
        std::vector<double> times;
        for (int sample = 0; sample < 1200; ++sample) {
            hand_checks::attachment = {110 + 16.0f * std::sin(sample * 0.2f),
                188 + 12.0f * std::cos(sample * 0.1f), 28 + 20.0f * std::sin(sample * 0.3f)};
            animatedEye = {100 + 4.0f * std::sin(sample * 0.2f),
                200 + 4.0f * std::cos(sample * 0.2f), 46 + 4.0f * std::sin(sample * 0.1f)};
            const bool noScene = sample % 3 == 1;
            const bool noMetadata = sample % 5 == 1;
            hand_checks::Store(&pawn, 0x330, noScene ? nullptr : static_cast<void*>(scene));
            hand_checks::Store(weaponServices, 0x60, noMetadata ? EntityHandle{} : weapon.handle);
            hand_checks::Store(&pawn, 0x3630, sample % 7 ? Vec3{} : Vec3{NAN, 0, 0});
            hand_checks::eyeMode = sample % 2 ? hand_checks::EyeMode::NoWrite : hand_checks::EyeMode::Crouched;
            hand_checks::attachmentLive = sample % 11 != 0;
            const double begin = HighResolutionSeconds();
            frame(noScene || noMetadata ? 30.0f : 90.0f);
            times.push_back((HighResolutionSeconds() - begin) * 1000);
            std::uint8_t flag{}; TryReadOffsetValue(&weapon.object, 0x56A, flag);
            if (!g_state.lastOverrideSucceeded || flag != 0 || fov != 90
                || Length(Subtract(position, expectedPosition)) > 0.001f
                || Length(Subtract(angles, expectedAngles)) > 0.001f) {
                std::printf("player callback FAILED sample %d: pose error %.3f fov %.1f flag %u\n",
                    sample, Length(Subtract(position, expectedPosition)), fov, flag);
                return false;
            }
        }
        std::sort(times.begin(), times.end());
        if (animatedEyeCalls != 0 || animatedHandCalls != 0) return false;
        std::printf("player callback offline: median %.3f ms, p95 %.3f ms, max %.3f ms (1200 callbacks)\n",
            times[600], times[1140], times.back());
        stage = 8;
        player_tail_checks::Harness engineHarness;
        if (!engineHarness.ready) return false;
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
        hand_checks::Store(&pawn, 0x3630, Vec3{});
        hand_checks::Store(weaponServices, 0x60, weapon.handle);
        g_state.setUpViewHooked = true;
        g_playerTailHookInstalled.store(false);
        if (!engineHarness.Run(view, 65)) return false;
        TryReadViewSetupOrigin(view, position);
        if (Length(Subtract(position, native)) > 0.001f
            || Length(Subtract(player_tail_checks::beforeNative,expectedPosition))>0.001f) return false;
        g_playerTailHookInstalled.store(true);
        for (int sample=0; sample<120; ++sample) {
            hand_checks::Store(weaponServices,0x60,sample%2?weapon.handle:knife.handle);
            if (!engineHarness.Run(view, sample%2?110.0f:55.0f)) return false;
            TryReadViewSetupOrigin(view,position);TryReadViewSetupAngles(view,angles);
            TryReadOffsetValue(view,kViewSetupFovOffset,fov);
            Vec3 historyPosition{},historyAngles{};
            TryReadOffsetValue(view,0x554,historyPosition);TryReadOffsetValue(view,0x560,historyAngles);
            if (Length(Subtract(position,expectedPosition))>0.001f
                || Length(Subtract(historyPosition,position))>0.001f || Length(Subtract(historyAngles,angles))>0.001f
                || fov!=90) {
                std::printf("native post-view FAILED sample %d: final pose error %.3f, fov %.1f\n",sample,Length(Subtract(position,expectedPosition)),fov);
                return false;
            }
        }
        bool workerPassed = false;
        std::thread worker([&] {
            unsigned char workerView[0x900]{};
            if (!engineHarness.Run(workerView,77)) return;
            float workerFov{};TryReadOffsetValue(workerView,kViewSetupFovOffset,workerFov);
            workerPassed = workerFov == 90;
        });
        worker.join();
        if (!workerPassed) return false;
        // Final callback must not execute a second prop camera or alter a native
        // view while prop mode has no selected projectile.
        g_state.cameraMode = CameraMode::PropSelfie;
        if (!engineHarness.Run(view,82)) return false;
        TryReadViewSetupOrigin(view,position);TryReadOffsetValue(view,kViewSetupFovOffset,fov);
        if (Length(Subtract(position,native))>0.001f || fov!=82) return false;
        g_state.cameraMode = CameraMode::PlayerSelfie;
        puts("player final-view native execution passed: post-early mutation replaced, 120 valid gun/knife/FOV transitions, final history copies, volatile GPR/XMM preservation, cross-thread FOV and prop bypass");
        // Full callback with quantized turn + alternating firing-sized yaw
        // perturbations, using the verified-clock seam in the hidden EXE.
        g_playerMotionClockReader = player_turn_checks::ReadClock;
        g_playerFrame.turn = {};
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
        for (int step = 0; step < 300; ++step) {
            const double wall = step / 144.0;
            const int ticks = static_cast<int>(std::floor(wall * 0.1 * 64));
            player_turn_checks::clock = {2000 + wall, 1000 + ticks, false};
            hand_checks::Store(&pawn, 0x3630, Vec3{30, ticks + (step % 2 ? 0.2f : -0.2f), 0});
            hand_checks::Store(weaponServices, 0x60, step % 2 ? weapon.handle : knife.handle);
            if (!engineHarness.Run(view, step % 2 ? 110.0f : 55.0f)) return false;
            TryReadViewSetupOrigin(view, position);
            TryReadOffsetValue(view, kViewSetupFovOffset, fov);
            if (!g_state.lastOverrideSucceeded || !g_playerFrame.turn.valid
                || fov != 90 || std::hypot(position.x - 100, position.y - 200) < 31.999f) return false;
        }
        g_state.enabled = false;
        if (!engineHarness.Run(view, 70)) return false;
        TryReadViewSetupOrigin(view, position); TryReadOffsetValue(view, kViewSetupFovOffset, fov);
        if (Length(Subtract(position, native)) > 0.001f || fov != 70) return false;
        g_state.enabled = true; g_playerCameraGeneration.fetch_add(1);
        if (!engineHarness.Run(view, 95)) return false;
        TryReadOffsetValue(view, kViewSetupFovOffset, fov);
        if (fov != 95) return false; // new session may deliberately choose a new FOV
        g_playerCameraGeneration.fetch_add(1);
        g_playerTailHookInstalled.store(false);
        puts("player final-view slow-turn passed: 300 late callbacks with 0.1x quantized yaw, alternating guns/knives and native FOV; disabled native return and new-session FOV capture");
        g_playerMotionClockReader = oldClockReader;
        g_playerFrame.turn = {};
        // Extreme trims must not place a fixed camera inside the body.
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
        hand_checks::Store(&pawn, 0x3630, Vec3{});
        g_state.lookMode = LookMode::Forward; g_state.offset = {-17, -18, 0}; frame();
        if (std::hypot(position.x - 100, position.y - 200) < 23.999f) return false;
        g_state.lookMode = LookMode::SelfieRight; g_state.offset = {-15, 100, 0}; frame();
        if (std::hypot(position.x - 100, position.y - 200) < 31.999f) return false;
        g_state.offset = {}; g_state.lookMode = LookMode::SelfieLeft; frame();
        if (!g_state.lastOverrideSucceeded) return false;
        g_state.lookMode = LookMode::SelfieRight;

        stage = 3;
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene));
        hand_checks::Store(&pawn, 0x3630, Vec3{});
        hand_checks::Store(weaponServices, 0x60, knife.handle); frame();
        std::uint8_t flag{}; TryReadOffsetValue(&weapon.object, 0x56A, flag);
        if (flag != 1 || g_viewModelSuppression.weaponHandle.raw != knife.handle.raw) return false;
        entities[8] = nullptr;
        frame(30); // missing weapon must not erase its original restore value
        entities[8] = &knife.object; frame();
        g_state.enabled = false; frame(35);
        TryReadOffsetValue(&knife.object, 0x56A, flag);
        if (flag != 1 || g_viewModelSuppression.active || fov != 35 || Length(Subtract(position, native)) > 0.001f) return false;

        stage = 4;
        g_state.enabled = true; g_playerCameraGeneration.fetch_add(1);
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(nullptr)); frame();
        if (Length(Subtract(position, native)) > 0.001f) return false; // cold failure cannot borrow old pose
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene)); frame();
        hand_checks::Store(identity, 0x10, EntityHandle{(601u << 15u) | 42u});
        frame();
        if (Length(Subtract(position, native)) > 0.001f) return false; // old serial cannot hold replacement
        hand_checks::Store(identity, 0x10, handle); frame();
        // A seek back with the same pawn serial must invalidate old world pose.
        g_offsets.entitySimulationTime = 0x470;
        hand_checks::Store(&pawn, 0x470, 100.0f); frame();
        hand_checks::Store(&pawn, 0x470, 10.0f);
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(nullptr)); frame();
        if (Length(Subtract(position, native)) > 0.001f) return false;
        hand_checks::Store(&pawn, 0x330, static_cast<void*>(scene)); frame();
        if (!g_state.lastOverrideSucceeded) return false;

        stage = 5;
        g_state.targetMode = TargetMode::Follow; g_state.currentFollowHandle = handle;
        const auto config = GetOverrideConfig();
        if (ResolvePlayerTargetForView(config, true, 100).handle.raw != handle.raw) return false;
        hand_checks::Store(observerServices, 0x4C, EntityHandle{});
        if (ResolvePlayerTargetForView(config, true, 100.01).handle.raw != handle.raw) return false;
        // A positively observed new target overrides the old snapshot immediately.
        hand_checks::Store(observerServices, 0x4C, observerPawn.handle);
        if (ResolvePlayerTargetForView(config, true, 100.02).handle.raw != observerPawn.handle.raw) return false;
        hand_checks::Store(observerServices, 0x4C, handle);
        if (ResolvePlayerTargetForView(config, true, 101).handle.raw != handle.raw) return false;
        hand_checks::Store(observerServices, 0x4C, EntityHandle{});
        if (ResolvePlayerTargetForView(config, true, 101.251).entity != nullptr) return false;
        g_state.enabled = false; frame();
        if (PlayerMotionSampleCount() != 1024u || ExportPlayerMotionWorker(nullptr) != 0u) return false;
        return true;
    }();
    *reinterpret_cast<void***>(&pawn) = originalVtable;
    g_offsets = oldOffsets;
    g_clientModule = oldModule;
    g_getEntityAbsOrigin = oldGetter;
    g_lookupAttachment = oldLookup;
    g_getAttachment = oldAttachment;
    g_state = oldState;
    g_entityListStorage = oldList; g_getEntityFromIndex = oldGet;
    g_getSplitScreenPlayer = oldSplit; g_getSplitScreenPlayerCandidates = oldCandidates;
    g_viewModelSuppression = oldSuppression; g_playerView = oldView; g_playerFrame = oldFrame;
    g_schemaOffsetsResolved.store(oldSchema); g_lastFollowTargetRefreshTickMs.store(oldRefresh);
    g_playerCameraGeneration.store(oldGeneration);
    g_playerMotionClockReader = oldClockReader;
    g_playerTailHookInstalled.store(oldTail);
    g_playerFovLatch = oldFovLatch;
    entities.fill(nullptr); observer = nullptr;
    g_knownClientSchemaFallbackAllowed.store(oldKnown);
    if (passed) puts("player view regression passed: 480 dirty-scene samples, 600 valid animated hands/eyes ignored, locomotion/yaw/crouch, 1200 full callbacks with zero bone/eye queries, body clearance, metadata/FOV recovery, safe lifecycle and bounded player CSV export");
    else std::printf("player view regression FAILED stage %d: %s\n", stage, failure.c_str());
    return passed;
}
}
