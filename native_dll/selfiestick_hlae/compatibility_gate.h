#pragma once

#include <cstddef>
#include <string_view>

namespace selfiestick::compat {

enum class ClientEntityKind {
    Unknown,
    PlayerPawn,
    PlayerController
};

struct RuntimeCompatibility {
    bool entityAccessReady{};
    bool schemaOffsetsReady{};
    bool splitScreenAccessorReady{};
    bool setUpViewPatchReady{};
};

struct CameraOffset {
    float right{};
    float back{};
    float up{};
};

struct CameraVector {
    float x{};
    float y{};
    float z{};
};

struct OffsetCameraVector {
    std::ptrdiff_t offset{};
    CameraVector value{};
};

enum class PropProjectileKind {
    Unknown,
    Smoke,
    Fire,
    Explosive,
    Flash,
    Decoy
};

ClientEntityKind ClassifyEntityClassName(std::string_view className);
PropProjectileKind ClassifyPropProjectileClassName(std::string_view className);
bool CanResolveFollowTarget(const RuntimeCompatibility& compatibility) noexcept;
bool CanProbeSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept;
bool CanInstallSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept;
bool ShouldAcceptPropOwnerCandidate(
    unsigned int targetPawnHandle,
    unsigned int targetControllerHandle,
    unsigned int candidateOwnerHandle
) noexcept;
bool ShouldKeepManualPropLock(
    bool hasLockedProp,
    bool lockedPropStillValid,
    unsigned int lockedPropHandle,
    unsigned int newestCandidateHandle
) noexcept;
bool ShouldAutoLockSinglePropCandidate(
    bool hasLockedProp,
    unsigned int validCandidateCount
) noexcept;
bool ShouldUseObserverPawnAsFollowTarget(
    ClientEntityKind splitScreenEntityKind,
    bool observerTargetValid,
    bool observerPawnResolved,
    bool observerPawnHandleValid
) noexcept;
bool ShouldUseSceneOriginEyeFallback(bool renderEyeOriginSucceeded, bool sceneOriginAvailable) noexcept;
bool ShouldUseViewSetupEyeFallback(
    bool renderEyeOriginSucceeded,
    bool sceneOriginAvailable,
    bool viewSetupOriginAvailable
) noexcept;
bool ShouldUseViewSetupAnglesFallback(
    bool pawnEyeAnglesAvailable,
    bool viewSetupAnglesAvailable
) noexcept;
bool ShouldUseSyntheticFallbackHandAnchor(
    bool attachmentAccessAvailable,
    bool heldItemCanUseSyntheticHandAnchor
) noexcept;
bool ShouldSuppressRenderWithViewModels(
    bool overrideSucceeded,
    bool weaponHandleValid,
    bool renderWithViewModelsOffsetAvailable,
    bool heldItemShouldSuppressViewModel
) noexcept;
bool ShouldEmitSampledTrace(
    unsigned long long index,
    unsigned long long initialCount,
    unsigned long long interval
) noexcept;
CameraOffset ApplyLeftSelfieCameraOffsetAdjustment(CameraOffset offset, bool isLeftSelfieLookMode) noexcept;
CameraVector MoveTowards(CameraVector current, CameraVector target, float maxDistance) noexcept;
CameraVector ExtrapolateProjectileOrigin(
    CameraVector origin,
    CameraVector velocity,
    float elapsedSeconds,
    float maxElapsedSeconds
) noexcept;
CameraVector ApplyAngleTrim(CameraVector baseAngles, CameraVector trim) noexcept;
CameraVector ApplyPlayerSelfieAngleTrim(CameraVector baseAngles, CameraVector playerRotation) noexcept;
bool TryBuildPropCenteredViewDirection(
    CameraVector cameraPosition,
    CameraVector propOrigin,
    CameraVector fallbackDirection,
    CameraVector& direction
) noexcept;
bool TrySelectNearestPlausibleVector(
    const CameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    CameraVector& selected
) noexcept;
bool TrySelectTrustedProjectileSceneOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept;
bool TrySelectDynamicProjectileOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept;
bool TrySelectLockedProjectileOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    std::ptrdiff_t lockedOffset,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept;
bool TrySelectMovingProjectileOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    const OffsetCameraVector* previousCandidates,
    unsigned int previousCandidateCount,
    CameraVector reference,
    float maxDistance,
    float minMotionDistance,
    float maxMotionDistance,
    OffsetCameraVector& selected
) noexcept;

} // namespace selfiestick::compat
