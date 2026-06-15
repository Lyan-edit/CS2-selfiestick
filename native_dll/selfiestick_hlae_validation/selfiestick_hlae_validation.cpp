#include <iostream>
#include <string_view>

#include "..\selfiestick_hlae\compatibility_gate.h"
#include "..\selfiestick_hlae\schema_probe.h"
#include "..\selfiestick_hlae\setup_view_hook.h"

namespace {

int ReportFailure(const char* message) {
    std::cerr << message << std::endl;
    return 1;
}

struct DeclaredClassProbe {
    void* vtable{};
    const char* name{};
    const char* moduleName{};
    const char* unknown{};
    void* schemaClass{};
};

} // namespace

int main() {
    using selfiestick::compat::CanProbeSetUpViewHook;
    using selfiestick::compat::CanInstallSetUpViewHook;
    using selfiestick::compat::CanResolveFollowTarget;
    using selfiestick::compat::ClassifyEntityClassName;
    using selfiestick::compat::ClientEntityKind;
    using selfiestick::compat::RuntimeCompatibility;
    using selfiestick::compat::ShouldUseObserverPawnAsFollowTarget;
    using selfiestick::compat::ShouldUseSceneOriginEyeFallback;
    using selfiestick::compat::ShouldUseSyntheticFallbackHandAnchor;
    using selfiestick::compat::ShouldUseViewSetupAnglesFallback;
    using selfiestick::compat::ShouldUseViewSetupEyeFallback;
    using selfiestick::compat::ShouldSuppressRenderWithViewModels;
    using selfiestick::compat::ShouldEmitSampledTrace;
    using selfiestick::compat::ApplyLeftSelfieCameraOffsetAdjustment;
    using selfiestick::compat::CameraOffset;
    using selfiestick::compat::CameraVector;
    using selfiestick::compat::ClassifyPropProjectileClassName;
    using selfiestick::compat::PropProjectileKind;
    using selfiestick::compat::ShouldAcceptPropOwnerCandidate;
    using selfiestick::compat::ShouldAutoLockSinglePropCandidate;
    using selfiestick::compat::ShouldKeepManualPropLock;
    using selfiestick::compat::ShouldUseActualThrowerDirection;
    using selfiestick::compat::ShouldUseSmokePostFlightHold;
    using selfiestick::compat::TrySelectNearestPlausibleVector;
    using selfiestick::compat::TrySelectTrustedProjectileSceneOrigin;
    using selfiestick::compat::TrySelectDynamicProjectileOrigin;
    using selfiestick::compat::TryBuildPropCenteredViewDirection;
    using selfiestick::compat::TrySelectLockedProjectileOrigin;
    using selfiestick::compat::TrySelectMovingProjectileOrigin;
    using selfiestick::compat::MoveTowards;
    using selfiestick::compat::ExtrapolateProjectileOrigin;
    using selfiestick::compat::ApplyAngleTrim;
    using selfiestick::compat::ApplyPlayerSelfieAngleTrim;
    using selfiestick::compat::GetImageStylePropCameraPreset;
    using selfiestick::compat::OffsetCameraVector;
    using selfiestick::schema::DetermineDeclaredClassScanLimit;
    using selfiestick::schema::DetermineExpectedProbeField;
    using selfiestick::schema::DetermineTypeScopeLookupProbeSlotLimit;
    using selfiestick::schema::IsUniqueRawSchemaCandidateCount;
    using selfiestick::schema::ShouldStartBackgroundSchemaResolve;
    using selfiestick::schema::ShouldUseSchemaInterfaceFallback;
    using selfiestick::schema::BuildQualifiedClassName;
    using selfiestick::schema::BuildSchemaClassLookupCandidates;
    using selfiestick::schema::TryGetKnownClientSchemaFieldOffsetFallback;
    using selfiestick::schema::BuildDeclaredClassSchemaClassPointerOffsets;
    using selfiestick::schema::BuildFocusedGenericRawFieldArrayLayoutCandidates;
    using selfiestick::schema::GetSchemaRawScanLayout;
    using selfiestick::schema::TryGetSchemaClassFromDeclaredClassPayload;
    using selfiestick::hook::BuildSetUpViewDetourStub;
    using selfiestick::hook::BuildSetUpViewEntryPatch;
    using selfiestick::hook::DecodeSetUpViewPatchSite;
    using selfiestick::hook::kSetUpViewPatchSize;
    using selfiestick::hook::SetUpViewPatchSite;

    if (ClassifyEntityClassName("C_CSPlayerPawn") != ClientEntityKind::PlayerPawn) {
        return ReportFailure("expected C_CSPlayerPawn to classify as PlayerPawn");
    }

    if (ClassifyEntityClassName("C_CSObserverPawn") != ClientEntityKind::PlayerPawn) {
        return ReportFailure("expected C_CSObserverPawn to classify as PlayerPawn");
    }

    if (ClassifyEntityClassName("CCSPlayerController") != ClientEntityKind::PlayerController) {
        return ReportFailure("expected CCSPlayerController to classify as PlayerController");
    }

    if (ClassifyEntityClassName("C_WeaponAWP") != ClientEntityKind::Unknown) {
        return ReportFailure("expected weapon class to classify as Unknown");
    }

    if (ClassifyPropProjectileClassName("C_SmokeGrenadeProjectile") != PropProjectileKind::Smoke) {
        return ReportFailure("expected smoke projectile to classify as Smoke");
    }

    if (ClassifyPropProjectileClassName("C_MolotovProjectile") != PropProjectileKind::Fire) {
        return ReportFailure("expected molotov projectile to classify as Fire");
    }

    if (ClassifyPropProjectileClassName("C_IncendiaryGrenadeProjectile") != PropProjectileKind::Fire) {
        return ReportFailure("expected incendiary projectile to classify as Fire");
    }

    if (ClassifyPropProjectileClassName("C_HEGrenadeProjectile") != PropProjectileKind::Explosive) {
        return ReportFailure("expected HE projectile to classify as Explosive");
    }

    if (ClassifyPropProjectileClassName("C_FlashbangProjectile") != PropProjectileKind::Flash) {
        return ReportFailure("expected flashbang projectile to classify as Flash");
    }

    if (ClassifyPropProjectileClassName("C_DecoyProjectile") != PropProjectileKind::Decoy) {
        return ReportFailure("expected decoy projectile to classify as Decoy");
    }

    if (ClassifyPropProjectileClassName("C_AK47") != PropProjectileKind::Unknown) {
        return ReportFailure("expected weapon class to reject prop projectile classification");
    }

    if (!ShouldAcceptPropOwnerCandidate(100u, 200u, 100u)) {
        return ReportFailure("prop owner matching should accept the target pawn handle");
    }

    if (!ShouldAcceptPropOwnerCandidate(100u, 200u, 200u)) {
        return ReportFailure("prop owner matching should accept the target controller handle");
    }

    if (ShouldAcceptPropOwnerCandidate(100u, 200u, 300u)) {
        return ReportFailure("prop owner matching should reject unrelated handles");
    }

    if (ShouldAcceptPropOwnerCandidate(100u, 200u, 0xFFFFFFFFu)) {
        return ReportFailure("prop owner matching should reject invalid handles");
    }

    if (!ShouldKeepManualPropLock(true, true, 500u, 600u)) {
        return ReportFailure("manual prop lock should keep the existing valid lock when a new candidate appears");
    }

    if (ShouldKeepManualPropLock(false, true, 500u, 600u)) {
        return ReportFailure("manual prop lock should not keep an unlocked state");
    }

    if (ShouldKeepManualPropLock(true, false, 500u, 600u)) {
        return ReportFailure("manual prop lock should clear when the locked prop is no longer valid");
    }

    if (!ShouldKeepManualPropLock(true, true, 500u, 500u)) {
        return ReportFailure("manual prop lock should keep the same locked candidate");
    }

    if (!ShouldAutoLockSinglePropCandidate(false, 1u)) {
        return ReportFailure("prop auto-lock should select the only valid candidate when no prop is locked");
    }

    if (ShouldAutoLockSinglePropCandidate(false, 0u)) {
        return ReportFailure("prop auto-lock should not select when no valid candidates exist");
    }

    if (ShouldAutoLockSinglePropCandidate(false, 2u)) {
        return ReportFailure("prop auto-lock should not select when multiple valid candidates exist");
    }

    if (ShouldAutoLockSinglePropCandidate(true, 1u)) {
        return ReportFailure("prop auto-lock should not replace an existing prop lock");
    }

    if (!ShouldUseActualThrowerDirection(true, true)) {
        return ReportFailure("prop camera should prefer the actual projectile thrower over the current target");
    }

    if (!ShouldUseActualThrowerDirection(true, false)) {
        return ReportFailure("prop camera should use the actual projectile thrower even when the current target is unavailable");
    }

    if (ShouldUseActualThrowerDirection(false, true)) {
        return ReportFailure("prop camera should fall back to the current target only when actual thrower cannot resolve");
    }

    if (!ShouldUseSmokePostFlightHold(PropProjectileKind::Smoke, false, 0.6, 1.2)) {
        return ReportFailure("smoke projectile should keep a short post-flight camera hold after the entity ends");
    }

    if (ShouldUseSmokePostFlightHold(PropProjectileKind::Smoke, false, 1.3, 1.2)) {
        return ReportFailure("smoke post-flight camera hold should expire after the configured duration");
    }

    if (ShouldUseSmokePostFlightHold(PropProjectileKind::Fire, false, 0.6, 1.2)) {
        return ReportFailure("non-smoke projectiles should not use the long smoke post-flight hold");
    }

    if (ShouldUseSmokePostFlightHold(PropProjectileKind::Smoke, true, 0.0, 1.2)) {
        return ReportFailure("live smoke projectiles should not be treated as post-flight hold frames");
    }

    const auto propImageStylePreset = GetImageStylePropCameraPreset();
    if (propImageStylePreset.offset.x != -8.0f
        || propImageStylePreset.offset.y != -22.0f
        || propImageStylePreset.offset.z != 4.0f
        || propImageStylePreset.rotation.x != 1.0f
        || propImageStylePreset.rotation.y != 0.0f
        || propImageStylePreset.rotation.z != 0.0f) {
        return ReportFailure("image-style prop camera preset should match the shared screenshot framing values");
    }

    const selfiestick::compat::CameraVector originCandidates[] = {
        { 9999.0f, 9999.0f, 9999.0f },
        { 30.0f, 0.0f, 0.0f },
        { 10.0f, 0.0f, 0.0f }
    };
    selfiestick::compat::CameraVector selectedOrigin{};
    if (!TrySelectNearestPlausibleVector(originCandidates, 3u, { 0.0f, 0.0f, 0.0f }, 128.0f, selectedOrigin)) {
        return ReportFailure("projectile origin fallback should select a plausible nearby vector");
    }
    if (selectedOrigin.x != 10.0f || selectedOrigin.y != 0.0f || selectedOrigin.z != 0.0f) {
        return ReportFailure("projectile origin fallback should select the nearest plausible vector");
    }

    const selfiestick::compat::CameraVector invalidOriginCandidates[] = {
        { 9999.0f, 9999.0f, 9999.0f },
        { NAN, 0.0f, 0.0f }
    };
    if (TrySelectNearestPlausibleVector(invalidOriginCandidates, 2u, { 0.0f, 0.0f, 0.0f }, 128.0f, selectedOrigin)) {
        return ReportFailure("projectile origin fallback should reject non-finite or distant vectors");
    }

    const OffsetCameraVector sceneNodeOriginCandidates[] = {
        { 0x30, { 149.248f, 0.0f, 1.0f } },
        { 0x88, { 151.0f, -1960.0f, 56.0f } },
        { 0xD0, { 152.0f, -1961.0f, 57.0f } }
    };
    OffsetCameraVector selectedSceneNodeOrigin{};
    if (!TrySelectTrustedProjectileSceneOrigin(sceneNodeOriginCandidates, 3u, { 151.965f, -1962.597f, 56.0f }, 4096.0f, selectedSceneNodeOrigin)) {
        return ReportFailure("projectile scene origin should select a trusted scene-node offset");
    }
    if (selectedSceneNodeOrigin.offset != 0xD0) {
        return ReportFailure("projectile scene origin should prefer trusted abs-origin offset over nearer local-looking values");
    }

    const OffsetCameraVector invalidSceneNodeOriginCandidates[] = {
        { 0x30, { 149.248f, 0.0f, 1.0f } },
        { 0xD0, { NAN, 0.0f, 0.0f } }
    };
    if (TrySelectTrustedProjectileSceneOrigin(invalidSceneNodeOriginCandidates, 2u, { 151.965f, -1962.597f, 56.0f }, 4096.0f, selectedSceneNodeOrigin)) {
        return ReportFailure("projectile scene origin should reject untrusted local-origin-only values");
    }

    const OffsetCameraVector zeroSceneNodeOriginCandidates[] = {
        { 0xD0, { 0.0f, -0.0f, -0.0f } },
        { 0x88, { 151.0f, -1960.0f, 56.0f } }
    };
    if (!TrySelectTrustedProjectileSceneOrigin(zeroSceneNodeOriginCandidates, 2u, { 151.965f, -1962.597f, 56.0f }, 4096.0f, selectedSceneNodeOrigin)) {
        return ReportFailure("projectile scene origin should continue past a trusted-but-zero candidate");
    }
    if (selectedSceneNodeOrigin.offset != 0x88) {
        return ReportFailure("projectile scene origin should reject a static map-origin value when the target is elsewhere");
    }

    const OffsetCameraVector onlyZeroSceneNodeOriginCandidates[] = {
        { 0xD0, { 0.0f, 0.0f, 0.0f } }
    };
    if (TrySelectTrustedProjectileSceneOrigin(onlyZeroSceneNodeOriginCandidates, 1u, { 151.965f, -1962.597f, 56.0f }, 4096.0f, selectedSceneNodeOrigin)) {
        return ReportFailure("projectile scene origin should not accept a static map-origin value as live projectile origin");
    }

    const OffsetCameraVector dynamicOriginCandidates[] = {
        { 0x40, { 12.420f, -111.355f, 0.0f } },
        { 0x88, { 0.0f, 0.0f, -1752469063335936.0f } },
        { 0x180, { -884.0f, -2334.0f, -105.0f } },
        { 0x220, { -2000.0f, -4000.0f, -500.0f } }
    };
    OffsetCameraVector selectedDynamicOrigin{};
    if (!TrySelectDynamicProjectileOrigin(dynamicOriginCandidates, 4u, { -887.0f, -2336.0f, -105.0f }, 1536.0f, selectedDynamicOrigin)) {
        return ReportFailure("dynamic projectile origin scan should select a plausible entity-space world vector");
    }
    if (selectedDynamicOrigin.offset != 0x180) {
        return ReportFailure("dynamic projectile origin scan should prefer the nearest plausible world vector");
    }

    const OffsetCameraVector invalidDynamicOriginCandidates[] = {
        { 0x40, { 12.420f, -111.355f, 0.0f } },
        { 0x80, { 0.0f, 0.0f, 0.0f } },
        { 0x88, { 0.0f, 0.0f, -1752469063335936.0f } }
    };
    if (TrySelectDynamicProjectileOrigin(invalidDynamicOriginCandidates, 3u, { -887.0f, -2336.0f, -105.0f }, 1536.0f, selectedDynamicOrigin)) {
        return ReportFailure("dynamic projectile origin scan should reject angle, zero, and non-world candidates");
    }

    const OffsetCameraVector movingOriginCandidates[] = {
        { 0x11A0, { -882.869f, -2332.305f, -98.567f } },
        { 0x1200, { -840.0f, -2318.0f, -105.0f } }
    };
    const OffsetCameraVector previousMovingOriginCandidates[] = {
        { 0x11A0, { -882.869f, -2332.305f, -98.567f } },
        { 0x1200, { -870.0f, -2330.0f, -102.0f } }
    };
    if (!TrySelectMovingProjectileOrigin(
            movingOriginCandidates,
            2u,
            previousMovingOriginCandidates,
            2u,
            { -835.0f, -2316.0f, -108.0f },
            1536.0f,
            0.5f,
            512.0f,
            selectedDynamicOrigin)) {
        return ReportFailure("moving projectile origin scan should select a moving world vector");
    }
    if (selectedDynamicOrigin.offset != 0x1200) {
        return ReportFailure("moving projectile origin scan should prefer moving current-position fields over static initial-position fields");
    }

    const OffsetCameraVector lockedOriginCandidates[] = {
        { 0x11A0, { -884.0f, -2334.0f, -105.0f } },
        { 0x11DC, { -840.0f, -2318.0f, -101.0f } }
    };
    if (!TrySelectLockedProjectileOrigin(
            lockedOriginCandidates,
            2u,
            0x11DC,
            { -835.0f, -2316.0f, -108.0f },
            1536.0f,
            selectedDynamicOrigin)) {
        return ReportFailure("locked projectile origin scan should keep using the locked live offset");
    }
    if (selectedDynamicOrigin.offset != 0x11DC) {
        return ReportFailure("locked projectile origin scan should not switch to another plausible offset");
    }

    const auto moved = MoveTowards({ 0.0f, 0.0f, 0.0f }, { 10.0f, 0.0f, 0.0f }, 3.0f);
    if (moved.x != 3.0f || moved.y != 0.0f || moved.z != 0.0f) {
        return ReportFailure("MoveTowards should advance by the capped distance");
    }

    const auto snappedMove = MoveTowards({ 0.0f, 0.0f, 0.0f }, { 2.0f, 0.0f, 0.0f }, 3.0f);
    if (snappedMove.x != 2.0f || snappedMove.y != 0.0f || snappedMove.z != 0.0f) {
        return ReportFailure("MoveTowards should snap when the target is within the cap");
    }

    const auto extrapolatedOrigin = ExtrapolateProjectileOrigin({ 10.0f, 0.0f, 0.0f }, { 100.0f, 0.0f, -20.0f }, 0.05f, 0.10f);
    if (extrapolatedOrigin.x != 15.0f || extrapolatedOrigin.y != 0.0f || extrapolatedOrigin.z != -1.0f) {
        return ReportFailure("projectile origin extrapolation should advance by velocity over elapsed time");
    }

    const auto clampedExtrapolatedOrigin = ExtrapolateProjectileOrigin({ 10.0f, 0.0f, 0.0f }, { 100.0f, 0.0f, 0.0f }, 0.25f, 0.10f);
    if (clampedExtrapolatedOrigin.x != 20.0f || clampedExtrapolatedOrigin.y != 0.0f || clampedExtrapolatedOrigin.z != 0.0f) {
        return ReportFailure("projectile origin extrapolation should clamp excessive elapsed time");
    }

    if (!ShouldEmitSampledTrace(1u, 3u, 10u) || !ShouldEmitSampledTrace(3u, 3u, 10u)) {
        return ReportFailure("sampled trace should emit initial frames");
    }
    if (ShouldEmitSampledTrace(4u, 3u, 10u)) {
        return ReportFailure("sampled trace should suppress ordinary frames after the initial window");
    }
    if (!ShouldEmitSampledTrace(10u, 3u, 10u) || ShouldEmitSampledTrace(11u, 3u, 10u)) {
        return ReportFailure("sampled trace should emit only interval frames after the initial window");
    }

    const auto angleTrim = ApplyAngleTrim({ 10.0f, 20.0f, 30.0f }, { 1.0f, -2.0f, 3.0f });
    if (angleTrim.x != 11.0f || angleTrim.y != 18.0f || angleTrim.z != 33.0f) {
        return ReportFailure("ApplyAngleTrim should add local pitch/yaw/roll trim");
    }

    const auto playerAngleTrim = ApplyPlayerSelfieAngleTrim({ -5.0f, 135.0f, 0.0f }, { 2.5f, -10.0f, 1.5f });
    if (playerAngleTrim.x != -2.5f || playerAngleTrim.y != 125.0f || playerAngleTrim.z != 1.5f) {
        return ReportFailure("player selfie angle trim should add independent local pitch/yaw/roll trim");
    }

    CameraVector propCenteredDirection{};
    if (!TryBuildPropCenteredViewDirection({ 0.0f, -18.0f, 6.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, propCenteredDirection)) {
        return ReportFailure("prop centered camera direction should point from camera to projectile center");
    }
    if (propCenteredDirection.y <= 0.9f || propCenteredDirection.z >= -0.2f) {
        return ReportFailure("prop centered camera direction should look at the projectile instead of straight back to the thrower");
    }

    if (!TryBuildPropCenteredViewDirection({ 0.0f, 0.0f, 0.0f }, { 0.0f, 0.0f, 0.0f }, { 0.0f, 1.0f, 0.0f }, propCenteredDirection)) {
        return ReportFailure("prop centered camera direction should use fallback when camera sits on projectile center");
    }
    if (propCenteredDirection.x != 0.0f || propCenteredDirection.y != 1.0f || propCenteredDirection.z != 0.0f) {
        return ReportFailure("prop centered camera direction should normalize the fallback direction");
    }

    if (!ShouldUseObserverPawnAsFollowTarget(ClientEntityKind::PlayerPawn, false, true, true)) {
        return ReportFailure("follow target resolution should fall back to a valid split-screen pawn when observer target is empty");
    }

    if (ShouldUseObserverPawnAsFollowTarget(ClientEntityKind::PlayerPawn, true, true, true)) {
        return ReportFailure("follow target resolution should prefer an explicit observer target when one exists");
    }

    if (ShouldUseObserverPawnAsFollowTarget(ClientEntityKind::PlayerController, false, true, false)) {
        return ReportFailure("follow target resolution should reject observer-pawn fallback without a valid pawn handle");
    }

    if (!ShouldUseSceneOriginEyeFallback(false, true)) {
        return ReportFailure("eye origin resolution should fall back to scene origin after RenderEyeOrigin faults");
    }

    if (ShouldUseSceneOriginEyeFallback(true, true)) {
        return ReportFailure("eye origin resolution should not use scene fallback when RenderEyeOrigin succeeds");
    }

    if (ShouldUseSceneOriginEyeFallback(false, false)) {
        return ReportFailure("eye origin resolution should reject scene fallback when scene origin is unavailable");
    }

    if (!ShouldUseViewSetupEyeFallback(false, false, true)) {
        return ReportFailure("eye origin resolution should fall back to view setup origin after RenderEyeOrigin and scene origin fail");
    }

    if (ShouldUseViewSetupEyeFallback(false, true, true)) {
        return ReportFailure("eye origin resolution should prefer scene origin over view setup origin when scene origin is available");
    }

    if (ShouldUseViewSetupEyeFallback(true, false, true)) {
        return ReportFailure("eye origin resolution should not use view setup fallback when RenderEyeOrigin succeeds");
    }

    if (ShouldUseViewSetupEyeFallback(false, false, false)) {
        return ReportFailure("eye origin resolution should reject view setup fallback when no valid view setup origin is available");
    }

    if (!ShouldUseViewSetupAnglesFallback(false, true)) {
        return ReportFailure("fallback frame basis should use view setup angles when pawn eye angles are unavailable");
    }

    if (ShouldUseViewSetupAnglesFallback(true, true)) {
        return ReportFailure("fallback frame basis should prefer pawn eye angles when they are available");
    }

    if (ShouldUseViewSetupAnglesFallback(false, false)) {
        return ReportFailure("fallback frame basis should reject view setup angles when they are unavailable");
    }

    if (!ShouldUseSyntheticFallbackHandAnchor(false, true)) {
        return ReportFailure("knife fallback should synthesize a hand anchor when attachment access is unavailable");
    }

    if (ShouldUseSyntheticFallbackHandAnchor(true, true)) {
        return ReportFailure("knife fallback should prefer real attachments when attachment access is available");
    }

    if (ShouldUseSyntheticFallbackHandAnchor(false, false)) {
        return ReportFailure("non-knife fallback should not use the knife hand anchor");
    }

    if (!ShouldSuppressRenderWithViewModels(true, true, true, true)) {
        return ReportFailure("knife override should suppress render-with-viewmodels when the schema field and weapon are available");
    }

    if (ShouldSuppressRenderWithViewModels(false, true, true, true)) {
        return ReportFailure("viewmodel suppression should not run when the camera override did not succeed");
    }

    if (ShouldSuppressRenderWithViewModels(true, false, true, true)) {
        return ReportFailure("viewmodel suppression should require a valid weapon handle");
    }

    if (ShouldSuppressRenderWithViewModels(true, true, false, true)) {
        return ReportFailure("viewmodel suppression should require the schema field offset");
    }

    if (ShouldSuppressRenderWithViewModels(true, true, true, false)) {
        return ReportFailure("viewmodel suppression should require a held item that renders as a viewmodel");
    }

    const CameraOffset originalCameraOffset{ 1.0f, 2.0f, 3.0f };
    const CameraOffset leftSelfieCameraOffset = ApplyLeftSelfieCameraOffsetAdjustment(originalCameraOffset, true);
    if (leftSelfieCameraOffset.right != 16.0f
        || leftSelfieCameraOffset.back != 12.0f
        || leftSelfieCameraOffset.up != 3.0f) {
        return ReportFailure("left selfie offset adjustment should add R +15 and B +10 while preserving U");
    }

    const CameraOffset unchangedCameraOffset = ApplyLeftSelfieCameraOffsetAdjustment(originalCameraOffset, false);
    if (unchangedCameraOffset.right != originalCameraOffset.right
        || unchangedCameraOffset.back != originalCameraOffset.back
        || unchangedCameraOffset.up != originalCameraOffset.up) {
        return ReportFailure("non-left-selfie offset adjustment should leave R/B/U unchanged");
    }

    RuntimeCompatibility blocked{};
    blocked.entityAccessReady = true;
    blocked.schemaOffsetsReady = false;
    blocked.splitScreenAccessorReady = false;
    blocked.setUpViewPatchReady = true;
    if (CanResolveFollowTarget(blocked)) {
        return ReportFailure("follow target resolution must be blocked when split-screen access is untrusted");
    }

    if (CanInstallSetUpViewHook(blocked)) {
        return ReportFailure("SetUpView hook must not install when follow target resolution is blocked");
    }

    RuntimeCompatibility ready{};
    ready.entityAccessReady = true;
    ready.schemaOffsetsReady = false;
    ready.splitScreenAccessorReady = true;
    ready.setUpViewPatchReady = true;
    if (CanResolveFollowTarget(ready)) {
        return ReportFailure("follow target resolution must be blocked when schema offsets are unresolved");
    }

    if (!CanProbeSetUpViewHook(ready)) {
        return ReportFailure("SetUpView hook probing should not require schema offsets");
    }

    if (!CanInstallSetUpViewHook(ready)) {
        return ReportFailure("SetUpView hook should install while schema offsets are still resolving");
    }

    ready.schemaOffsetsReady = true;
    if (!CanResolveFollowTarget(ready)) {
        return ReportFailure("follow target resolution should be available when entity access and schema offsets are trusted");
    }

    ready.setUpViewPatchReady = false;
    if (!CanProbeSetUpViewHook(ready)) {
        return ReportFailure("SetUpView hook probing should start before patch metadata is marked ready");
    }

    ready.setUpViewPatchReady = true;
    if (!CanInstallSetUpViewHook(ready)) {
        return ReportFailure("SetUpView hook should install when all compatibility gates are satisfied");
    }

    if (DetermineDeclaredClassScanLimit(0u) != 32768u) {
        return ReportFailure("zero declared class count should use the fallback scan limit");
    }

    if (DetermineDeclaredClassScanLimit(42u) != 32768u) {
        return ReportFailure("small declared class counts should use the fallback scan limit");
    }

    if (DetermineDeclaredClassScanLimit(12000u) != 12000u) {
        return ReportFailure("plausible declared class counts should be preserved");
    }

    if (DetermineTypeScopeLookupProbeSlotLimit() < 64u) {
        return ReportFailure("type scope class lookup probing should cover the widened post-update vtable range");
    }

    if (std::string_view(DetermineExpectedProbeField("C_CSPlayerPawn")) != "m_angEyeAngles") {
        return ReportFailure("C_CSPlayerPawn probe should validate against m_angEyeAngles");
    }

    if (std::string_view(DetermineExpectedProbeField("C_BaseEntity")) != "m_pGameSceneNode") {
        return ReportFailure("C_BaseEntity probe should validate against m_pGameSceneNode");
    }

    if (DetermineExpectedProbeField("UnknownSchemaClass") != nullptr) {
        return ReportFailure("unknown schema probe classes should not force an expected field");
    }

    std::string qualifiedClassName;
    if (!BuildQualifiedClassName("client.dll", "C_CSPlayerPawn", qualifiedClassName)) {
        return ReportFailure("qualified schema class name should build for client.dll classes");
    }

    if (qualifiedClassName != "client.dll!C_CSPlayerPawn") {
        return ReportFailure("qualified schema class name should use module!class format");
    }

    if (BuildQualifiedClassName(nullptr, "C_CSPlayerPawn", qualifiedClassName)) {
        return ReportFailure("qualified schema class name should reject null module names");
    }

    if (BuildQualifiedClassName("client.dll", "", qualifiedClassName)) {
        return ReportFailure("qualified schema class name should reject empty class names");
    }

    std::array<std::string, 2> lookupCandidates;
    const auto lookupCandidateCount = BuildSchemaClassLookupCandidates("client.dll", "C_CSPlayerPawn", lookupCandidates);
    if (lookupCandidateCount != 2u) {
        return ReportFailure("schema class lookup candidates should include both default and qualified queries");
    }

    if (lookupCandidates[0] != "C_CSPlayerPawn") {
        return ReportFailure("schema class lookup should try the default scope name first");
    }

    if (lookupCandidates[1] != "client.dll!C_CSPlayerPawn") {
        return ReportFailure("schema class lookup should fall back to the qualified module!class form");
    }

    std::ptrdiff_t fallbackOffset = 0;
    if (!TryGetKnownClientSchemaFieldOffsetFallback("C_BasePlayerPawn", "m_pWeaponServices", fallbackOffset)
        || fallbackOffset != 0x11E0) {
        return ReportFailure("known schema fallback should cover the observed C_BasePlayerPawn weapon-services offset");
    }

    if (!TryGetKnownClientSchemaFieldOffsetFallback("CPlayer_ObserverServices", "m_iObserverMode", fallbackOffset)
        || fallbackOffset != 0x48) {
        return ReportFailure("known schema fallback should cover the observed observer mode offset");
    }

    if (TryGetKnownClientSchemaFieldOffsetFallback("C_BasePlayerPawn", "m_unknownField", fallbackOffset)) {
        return ReportFailure("known schema fallback should reject unknown fields");
    }

    if (IsUniqueRawSchemaCandidateCount(0u)) {
        return ReportFailure("raw schema matching should reject empty candidate sets");
    }

    if (!IsUniqueRawSchemaCandidateCount(1u)) {
        return ReportFailure("raw schema matching should accept exactly one candidate");
    }

    if (IsUniqueRawSchemaCandidateCount(2u)) {
        return ReportFailure("raw schema matching should reject ambiguous candidate sets");
    }

    int schemaClassSentinel = 0;
    DeclaredClassProbe declaredClassProbe{};
    declaredClassProbe.schemaClass = &schemaClassSentinel;
    void* unwrappedSchemaClass = nullptr;
    if (!TryGetSchemaClassFromDeclaredClassPayload(&declaredClassProbe, unwrappedSchemaClass)) {
        return ReportFailure("declared class payloads should unwrap to their schema class pointer");
    }

    if (unwrappedSchemaClass != &schemaClassSentinel) {
        return ReportFailure("declared class payload unwrap returned the wrong schema class pointer");
    }

    std::array<std::size_t, 8> declaredClassSchemaClassOffsets{};
    const auto declaredClassSchemaClassOffsetCount = BuildDeclaredClassSchemaClassPointerOffsets(declaredClassSchemaClassOffsets);
    if (declaredClassSchemaClassOffsetCount < 3u) {
        return ReportFailure("declared class schema class pointer probing should cover multiple wrapper layouts");
    }

    if (declaredClassSchemaClassOffsets[0] != 0x20u) {
        return ReportFailure("declared class schema class pointer probing should prefer the known legacy offset first");
    }

    bool probesUpdatedWrapperOffset = false;
    for (std::size_t offsetIndex = 0; offsetIndex < declaredClassSchemaClassOffsetCount; ++offsetIndex) {
        probesUpdatedWrapperOffset = probesUpdatedWrapperOffset || declaredClassSchemaClassOffsets[offsetIndex] == 0x28u;
    }

    if (!probesUpdatedWrapperOffset) {
        return ReportFailure("declared class schema class pointer probing should include the next pointer slot");
    }

    if (ShouldStartBackgroundSchemaResolve(true, false, false, 0ull, 500ull)) {
        return ReportFailure("background schema resolve should not start when offsets are already resolved");
    }

    if (ShouldStartBackgroundSchemaResolve(false, true, false, 0ull, 500ull)) {
        return ReportFailure("background schema resolve should not start while a resolve is already in progress");
    }

    if (!ShouldStartBackgroundSchemaResolve(false, false, false, 0ull, 500ull)) {
        return ReportFailure("background schema resolve should start immediately when no attempt has run yet");
    }

    if (ShouldStartBackgroundSchemaResolve(false, false, true, 250ull, 500ull)) {
        return ReportFailure("background schema resolve should back off briefly after a recent failure");
    }

    if (!ShouldStartBackgroundSchemaResolve(false, false, true, 500ull, 500ull)) {
        return ReportFailure("background schema resolve should retry once the backoff interval expires");
    }

    if (ShouldUseSchemaInterfaceFallback(true, true, false)) {
        return ReportFailure("schema interface fallback should stay disabled after raw schema payloads are found but incomplete");
    }

    if (!ShouldUseSchemaInterfaceFallback(false, false, false)) {
        return ReportFailure("schema interface fallback should remain available when raw schema cannot be inspected");
    }

    const auto rawScanLayout = GetSchemaRawScanLayout();
    if (rawScanLayout.schemaSystemScopeArrayOffset != 0x198) {
        return ReportFailure("schema raw scan should use the recovered schemaSystem->scopeArray offset");
    }

    if (rawScanLayout.typeScopeDeclaredClassCountOffset != 0x56C) {
        return ReportFailure("schema raw scan should use the recovered type scope count offset");
    }

    if (rawScanLayout.typeScopeBucketTableOffset != 0x5C0) {
        return ReportFailure("schema raw scan should use the recovered type scope bucket table offset");
    }

    if (rawScanLayout.typeScopeBucketStride != 0x18) {
        return ReportFailure("schema raw scan should use the recovered bucket stride");
    }

    if (rawScanLayout.typeScopeBucketHeadOffset != 0x10) {
        return ReportFailure("schema raw scan should use the recovered bucket head offset");
    }

    if (rawScanLayout.typeScopeBucketNodeNextOffset != 0x08) {
        return ReportFailure("schema raw scan should use the recovered bucket next offset");
    }

    if (rawScanLayout.typeScopeBucketNodePayloadOffset != 0x10) {
        return ReportFailure("schema raw scan should use the recovered bucket payload offset");
    }

    if (rawScanLayout.typeScopeBucketCount != 0x100u) {
        return ReportFailure("schema raw scan should enumerate all recovered type scope buckets");
    }

    std::array<selfiestick::schema::GenericRawFieldArrayLayoutCandidate, 12> focusedRawFieldLayouts{};
    const auto focusedRawFieldLayoutCount = BuildFocusedGenericRawFieldArrayLayoutCandidates(focusedRawFieldLayouts);
    if (focusedRawFieldLayoutCount == 0u || focusedRawFieldLayoutCount > focusedRawFieldLayouts.size()) {
        return ReportFailure("focused raw field array scan should expose a bounded candidate list");
    }

    if (focusedRawFieldLayouts[0].wrapperOffset != 0x40u
        || focusedRawFieldLayouts[0].arrayOffset != 0x0u
        || focusedRawFieldLayouts[0].fieldStride != 0x20u
        || focusedRawFieldLayouts[0].fieldNameOffset != 0x8u
        || focusedRawFieldLayouts[0].fieldOffsetOffset != 0x10u) {
        return ReportFailure("focused raw field array scan should prefer the current CS2 payload+0x40 field array layout");
    }

    std::array<std::uint8_t, kSetUpViewPatchSize> patchBytes{
        0xBA, 0xFF, 0xFF, 0xFF, 0xFF,
        0x48, 0x8D, 0x0D, 0x00, 0x00, 0x00, 0x00,
        0xE8, 0x00, 0x00, 0x00, 0x00
    };
    const auto patchAddress = reinterpret_cast<std::uintptr_t>(patchBytes.data());
    const auto helperStringAddress = patchAddress + 0x1234;
    const auto helperCallTarget = patchAddress + 0x4321;
    *reinterpret_cast<std::int32_t*>(patchBytes.data() + 8) = static_cast<std::int32_t>(helperStringAddress - (patchAddress + 12u));
    *reinterpret_cast<std::int32_t*>(patchBytes.data() + 13) = static_cast<std::int32_t>(helperCallTarget - (patchAddress + 17u));

    SetUpViewPatchSite decodedSite{};
    std::string error;
    if (!DecodeSetUpViewPatchSite(patchBytes.data(), decodedSite, &error)) {
        return ReportFailure(("failed to decode SetUpView patch site: " + error).c_str());
    }

    if (decodedSite.patchAddress != patchAddress) {
        return ReportFailure("decoded patch address mismatch");
    }

    if (decodedSite.helperStringAddress != helperStringAddress) {
        return ReportFailure("decoded helper string address mismatch");
    }

    if (decodedSite.helperCallTarget != helperCallTarget) {
        return ReportFailure("decoded helper call target mismatch");
    }

    if (decodedSite.returnAddress != patchAddress + kSetUpViewPatchSize) {
        return ReportFailure("decoded return address mismatch");
    }

    const std::uintptr_t detourStubAddress = patchAddress + 0x200;
    const std::uintptr_t callbackAddress = 0x7FFF000012340000ull;
    std::vector<std::uint8_t> stubBytes;
    if (!BuildSetUpViewDetourStub(detourStubAddress, decodedSite, callbackAddress, stubBytes, &error)) {
        return ReportFailure(("failed to build SetUpView detour stub: " + error).c_str());
    }

    if (stubBytes.size() < 32u) {
        return ReportFailure("detour stub should contain relocated setup and callback bridge");
    }

    if (!(stubBytes[0] == 0xF3 && stubBytes[1] == 0x0F && stubBytes[2] == 0x1E && stubBytes[3] == 0xFA)) {
        return ReportFailure("detour stub should begin with ENDBR64");
    }

    const auto relocatedStringDisplacement = *reinterpret_cast<const std::int32_t*>(stubBytes.data() + 12);
    const auto relocatedCallDisplacement = *reinterpret_cast<const std::int32_t*>(stubBytes.data() + 17);
    const auto relocatedStringAddress = (detourStubAddress + 16u) + static_cast<std::intptr_t>(relocatedStringDisplacement);
    const auto relocatedCallAddress = (detourStubAddress + 21u) + static_cast<std::intptr_t>(relocatedCallDisplacement);
    if (relocatedStringAddress != helperStringAddress) {
        return ReportFailure("relocated helper string address mismatch");
    }

    if (relocatedCallAddress != helperCallTarget) {
        return ReportFailure("relocated helper call target mismatch");
    }

    std::array<std::uint8_t, kSetUpViewPatchSize> entryPatch{};
    if (!BuildSetUpViewEntryPatch(patchAddress, detourStubAddress, entryPatch, &error)) {
        return ReportFailure(("failed to build SetUpView entry patch: " + error).c_str());
    }

    if (entryPatch[0] != 0xE9) {
        return ReportFailure("entry patch should start with a relative JMP");
    }

    const auto entryJumpDisplacement = *reinterpret_cast<const std::int32_t*>(entryPatch.data() + 1);
    const auto entryJumpTarget = (patchAddress + 5u) + static_cast<std::intptr_t>(entryJumpDisplacement);
    if (entryJumpTarget != detourStubAddress) {
        return ReportFailure("entry patch jump target mismatch");
    }

    for (std::size_t index = 5; index < entryPatch.size(); ++index) {
        if (entryPatch[index] != 0x90) {
            return ReportFailure("entry patch tail should be padded with NOPs");
        }
    }

    std::cout << "compatibility gate validation passed" << std::endl;
    return 0;
}
