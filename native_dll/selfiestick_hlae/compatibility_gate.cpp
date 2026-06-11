#include "compatibility_gate.h"

#include <cmath>

namespace selfiestick::compat {
namespace {

constexpr float kLeftSelfieAdditionalRightOffset = 15.0f;
constexpr float kLeftSelfieAdditionalBackOffset = 10.0f;

int ProjectileSceneOriginOffsetPriority(std::ptrdiff_t offset) noexcept {
    switch (offset) {
    case 0xD0:
        return 0;
    case 0x88:
        return 1;
    default:
        return -1;
    }
}

float LengthSquared(CameraVector value) noexcept {
    return value.x * value.x + value.y * value.y + value.z * value.z;
}

bool TryNormalizeCameraVector(CameraVector value, CameraVector& normalized) noexcept {
    const float lengthSquared = LengthSquared(value);
    if (!std::isfinite(lengthSquared) || lengthSquared <= 1.0e-8f) {
        return false;
    }

    const float inverseLength = 1.0f / std::sqrt(lengthSquared);
    normalized = CameraVector{
        value.x * inverseLength,
        value.y * inverseLength,
        value.z * inverseLength
    };
    return std::isfinite(normalized.x) && std::isfinite(normalized.y) && std::isfinite(normalized.z);
}

bool IsStaticMapOriginCandidate(CameraVector candidate, CameraVector reference) noexcept {
    constexpr float kNearMapOriginDistance = 64.0f;
    constexpr float kReferenceAwayFromOriginDistance = 512.0f;
    return LengthSquared(candidate) <= kNearMapOriginDistance * kNearMapOriginDistance
        && LengthSquared(reference) >= kReferenceAwayFromOriginDistance * kReferenceAwayFromOriginDistance;
}

bool IsAngleLikeVector(CameraVector candidate, CameraVector reference) noexcept {
    constexpr float kAngleComponentLimit = 360.0f;
    constexpr float kReferenceAwayFromOriginDistance = 512.0f;
    const float maxAbsComponent = std::fmax(std::fmax(std::fabs(candidate.x), std::fabs(candidate.y)), std::fabs(candidate.z));
    return LengthSquared(reference) >= kReferenceAwayFromOriginDistance * kReferenceAwayFromOriginDistance
        && maxAbsComponent <= kAngleComponentLimit;
}

bool IsReasonableWorldVector(CameraVector candidate) noexcept {
    constexpr float kWorldComponentLimit = 32768.0f;
    return std::fabs(candidate.x) <= kWorldComponentLimit
        && std::fabs(candidate.y) <= kWorldComponentLimit
        && std::fabs(candidate.z) <= kWorldComponentLimit;
}

bool IsPlausibleDynamicProjectileOrigin(CameraVector candidate, CameraVector reference, float maxDistance) noexcept {
    if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) {
        return false;
    }

    if (!IsReasonableWorldVector(candidate)
        || IsStaticMapOriginCandidate(candidate, reference)
        || IsAngleLikeVector(candidate, reference)) {
        return false;
    }

    const CameraVector delta{
        candidate.x - reference.x,
        candidate.y - reference.y,
        candidate.z - reference.z
    };
    const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    return std::isfinite(distanceSquared) && distanceSquared <= maxDistance * maxDistance;
}

bool Contains(std::string_view haystack, std::string_view needle) noexcept {
    return !needle.empty() && haystack.find(needle) != std::string_view::npos;
}

} // namespace

ClientEntityKind ClassifyEntityClassName(std::string_view className) {
    if (Contains(className, "PlayerPawn") || Contains(className, "ObserverPawn")) {
        return ClientEntityKind::PlayerPawn;
    }

    if (Contains(className, "PlayerController")) {
        return ClientEntityKind::PlayerController;
    }

    return ClientEntityKind::Unknown;
}

PropProjectileKind ClassifyPropProjectileClassName(std::string_view className) {
    if (!Contains(className, "Projectile")) {
        return PropProjectileKind::Unknown;
    }

    if (Contains(className, "Smoke")) {
        return PropProjectileKind::Smoke;
    }

    if (Contains(className, "Molotov") || Contains(className, "Incendiary")) {
        return PropProjectileKind::Fire;
    }

    if (Contains(className, "HEGrenade") || Contains(className, "HighExplosive")) {
        return PropProjectileKind::Explosive;
    }

    if (Contains(className, "Flashbang")) {
        return PropProjectileKind::Flash;
    }

    if (Contains(className, "Decoy")) {
        return PropProjectileKind::Decoy;
    }

    return PropProjectileKind::Unknown;
}

bool CanResolveFollowTarget(const RuntimeCompatibility& compatibility) noexcept {
    return compatibility.entityAccessReady
        && compatibility.schemaOffsetsReady
        && compatibility.splitScreenAccessorReady;
}

bool CanProbeSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept {
    return compatibility.entityAccessReady
        && compatibility.splitScreenAccessorReady;
}

bool CanInstallSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept {
    return compatibility.setUpViewPatchReady
        && compatibility.entityAccessReady
        && compatibility.schemaOffsetsReady
        && compatibility.splitScreenAccessorReady;
}

bool ShouldAcceptPropOwnerCandidate(
    unsigned int targetPawnHandle,
    unsigned int targetControllerHandle,
    unsigned int candidateOwnerHandle
) noexcept {
    constexpr unsigned int invalidHandle = 0xFFFFFFFFu;
    if (candidateOwnerHandle == invalidHandle) {
        return false;
    }

    return candidateOwnerHandle == targetPawnHandle
        || candidateOwnerHandle == targetControllerHandle;
}

bool ShouldKeepManualPropLock(
    bool hasLockedProp,
    bool lockedPropStillValid,
    unsigned int lockedPropHandle,
    unsigned int newestCandidateHandle
) noexcept {
    if (!hasLockedProp || !lockedPropStillValid) {
        return false;
    }

    return lockedPropHandle == newestCandidateHandle || newestCandidateHandle != 0xFFFFFFFFu;
}

bool ShouldAutoLockSinglePropCandidate(
    bool hasLockedProp,
    unsigned int validCandidateCount
) noexcept {
    return !hasLockedProp && validCandidateCount == 1u;
}

bool ShouldUseObserverPawnAsFollowTarget(
    ClientEntityKind splitScreenEntityKind,
    bool observerTargetValid,
    bool observerPawnResolved,
    bool observerPawnHandleValid
) noexcept {
    if (observerTargetValid || !observerPawnResolved || !observerPawnHandleValid) {
        return false;
    }

    return splitScreenEntityKind == ClientEntityKind::PlayerPawn
        || splitScreenEntityKind == ClientEntityKind::PlayerController;
}

bool ShouldUseSceneOriginEyeFallback(bool renderEyeOriginSucceeded, bool sceneOriginAvailable) noexcept {
    return !renderEyeOriginSucceeded && sceneOriginAvailable;
}

bool ShouldUseViewSetupEyeFallback(
    bool renderEyeOriginSucceeded,
    bool sceneOriginAvailable,
    bool viewSetupOriginAvailable
) noexcept {
    return !renderEyeOriginSucceeded && !sceneOriginAvailable && viewSetupOriginAvailable;
}

bool ShouldUseViewSetupAnglesFallback(
    bool pawnEyeAnglesAvailable,
    bool viewSetupAnglesAvailable
) noexcept {
    return !pawnEyeAnglesAvailable && viewSetupAnglesAvailable;
}

bool ShouldUseSyntheticFallbackHandAnchor(
    bool attachmentAccessAvailable,
    bool heldItemCanUseSyntheticHandAnchor
) noexcept {
    return !attachmentAccessAvailable && heldItemCanUseSyntheticHandAnchor;
}

bool ShouldSuppressRenderWithViewModels(
    bool overrideSucceeded,
    bool weaponHandleValid,
    bool renderWithViewModelsOffsetAvailable,
    bool heldItemShouldSuppressViewModel
) noexcept {
    return overrideSucceeded
        && weaponHandleValid
        && renderWithViewModelsOffsetAvailable
        && heldItemShouldSuppressViewModel;
}

bool ShouldEmitSampledTrace(
    unsigned long long index,
    unsigned long long initialCount,
    unsigned long long interval
) noexcept {
    if (index == 0u) {
        return false;
    }

    return index <= initialCount || (interval > 0u && 0u == (index % interval));
}

CameraOffset ApplyLeftSelfieCameraOffsetAdjustment(CameraOffset offset, bool isLeftSelfieLookMode) noexcept {
    if (!isLeftSelfieLookMode) {
        return offset;
    }

    offset.right += kLeftSelfieAdditionalRightOffset;
    offset.back += kLeftSelfieAdditionalBackOffset;
    return offset;
}

CameraVector MoveTowards(CameraVector current, CameraVector target, float maxDistance) noexcept {
    if (maxDistance <= 0.0f || !std::isfinite(maxDistance)) {
        return current;
    }

    const CameraVector delta{
        target.x - current.x,
        target.y - current.y,
        target.z - current.z
    };
    const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
    if (distanceSquared <= maxDistance * maxDistance) {
        return target;
    }

    const float distance = std::sqrt(distanceSquared);
    if (distance <= 0.0001f || !std::isfinite(distance)) {
        return current;
    }

    const float scale = maxDistance / distance;
    return CameraVector{
        current.x + delta.x * scale,
        current.y + delta.y * scale,
        current.z + delta.z * scale
    };
}

CameraVector ExtrapolateProjectileOrigin(
    CameraVector origin,
    CameraVector velocity,
    float elapsedSeconds,
    float maxElapsedSeconds
) noexcept {
    if (!std::isfinite(elapsedSeconds) || elapsedSeconds < 0.0f) {
        elapsedSeconds = 0.0f;
    }
    if (!std::isfinite(maxElapsedSeconds) || maxElapsedSeconds < 0.0f) {
        maxElapsedSeconds = 0.0f;
    }

    const float clampedElapsed = std::fmin(elapsedSeconds, maxElapsedSeconds);
    return CameraVector{
        origin.x + velocity.x * clampedElapsed,
        origin.y + velocity.y * clampedElapsed,
        origin.z + velocity.z * clampedElapsed
    };
}

CameraVector ApplyAngleTrim(CameraVector baseAngles, CameraVector trim) noexcept {
    return CameraVector{
        baseAngles.x + trim.x,
        baseAngles.y + trim.y,
        baseAngles.z + trim.z
    };
}

bool TryBuildPropCenteredViewDirection(
    CameraVector cameraPosition,
    CameraVector propOrigin,
    CameraVector fallbackDirection,
    CameraVector& direction
) noexcept {
    const CameraVector toProp{
        propOrigin.x - cameraPosition.x,
        propOrigin.y - cameraPosition.y,
        propOrigin.z - cameraPosition.z
    };
    if (TryNormalizeCameraVector(toProp, direction)) {
        return true;
    }

    return TryNormalizeCameraVector(fallbackDirection, direction);
}

bool TrySelectNearestPlausibleVector(
    const CameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    CameraVector& selected
) noexcept {
    if (candidates == nullptr || candidateCount == 0u || maxDistance <= 0.0f || !std::isfinite(maxDistance)) {
        return false;
    }

    const float maxDistanceSquared = maxDistance * maxDistance;
    float bestDistanceSquared = 0.0f;
    bool found = false;

    for (unsigned int index = 0u; index < candidateCount; ++index) {
        const CameraVector candidate = candidates[index];
        if (!std::isfinite(candidate.x) || !std::isfinite(candidate.y) || !std::isfinite(candidate.z)) {
            continue;
        }

        const CameraVector delta{
            candidate.x - reference.x,
            candidate.y - reference.y,
            candidate.z - reference.z
        };
        const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (!std::isfinite(distanceSquared) || distanceSquared > maxDistanceSquared) {
            continue;
        }

        if (!found || distanceSquared < bestDistanceSquared) {
            selected = candidate;
            bestDistanceSquared = distanceSquared;
            found = true;
        }
    }

    return found;
}

bool TrySelectTrustedProjectileSceneOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept {
    if (candidates == nullptr || candidateCount == 0u || maxDistance <= 0.0f || !std::isfinite(maxDistance)) {
        return false;
    }

    const float maxDistanceSquared = maxDistance * maxDistance;
    float bestDistanceSquared = 0.0f;
    int bestPriority = 0;
    bool found = false;

    for (unsigned int index = 0u; index < candidateCount; ++index) {
        const OffsetCameraVector candidate = candidates[index];
        const int priority = ProjectileSceneOriginOffsetPriority(candidate.offset);
        if (priority < 0) {
            continue;
        }

        if (!std::isfinite(candidate.value.x) || !std::isfinite(candidate.value.y) || !std::isfinite(candidate.value.z)) {
            continue;
        }

        if (IsStaticMapOriginCandidate(candidate.value, reference)) {
            continue;
        }

        const CameraVector delta{
            candidate.value.x - reference.x,
            candidate.value.y - reference.y,
            candidate.value.z - reference.z
        };
        const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (!std::isfinite(distanceSquared) || distanceSquared > maxDistanceSquared) {
            continue;
        }

        if (!found || priority < bestPriority || (priority == bestPriority && distanceSquared < bestDistanceSquared)) {
            selected = candidate;
            bestDistanceSquared = distanceSquared;
            bestPriority = priority;
            found = true;
        }
    }

    return found;
}

bool TrySelectDynamicProjectileOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept {
    if (candidates == nullptr || candidateCount == 0u || maxDistance <= 0.0f || !std::isfinite(maxDistance)) {
        return false;
    }

    const float maxDistanceSquared = maxDistance * maxDistance;
    float bestDistanceSquared = 0.0f;
    bool found = false;

    for (unsigned int index = 0u; index < candidateCount; ++index) {
        const OffsetCameraVector candidate = candidates[index];
        if (!IsPlausibleDynamicProjectileOrigin(candidate.value, reference, maxDistance)) {
            continue;
        }

        const CameraVector delta{
            candidate.value.x - reference.x,
            candidate.value.y - reference.y,
            candidate.value.z - reference.z
        };
        const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (!found || distanceSquared < bestDistanceSquared) {
            selected = candidate;
            bestDistanceSquared = distanceSquared;
            found = true;
        }
    }

    return found;
}

bool TrySelectLockedProjectileOrigin(
    const OffsetCameraVector* candidates,
    unsigned int candidateCount,
    std::ptrdiff_t lockedOffset,
    CameraVector reference,
    float maxDistance,
    OffsetCameraVector& selected
) noexcept {
    if (candidates == nullptr
        || candidateCount == 0u
        || lockedOffset < 0
        || maxDistance <= 0.0f
        || !std::isfinite(maxDistance)) {
        return false;
    }

    for (unsigned int index = 0u; index < candidateCount; ++index) {
        const OffsetCameraVector candidate = candidates[index];
        if (candidate.offset != lockedOffset) {
            continue;
        }

        if (!IsPlausibleDynamicProjectileOrigin(candidate.value, reference, maxDistance)) {
            return false;
        }

        selected = candidate;
        return true;
    }

    return false;
}

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
) noexcept {
    if (candidates == nullptr
        || previousCandidates == nullptr
        || candidateCount == 0u
        || previousCandidateCount == 0u
        || maxDistance <= 0.0f
        || minMotionDistance < 0.0f
        || maxMotionDistance <= 0.0f
        || !std::isfinite(maxDistance)
        || !std::isfinite(minMotionDistance)
        || !std::isfinite(maxMotionDistance)) {
        return false;
    }

    const float minMotionSquared = minMotionDistance * minMotionDistance;
    const float maxMotionSquared = maxMotionDistance * maxMotionDistance;
    float bestDistanceSquared = 0.0f;
    bool found = false;

    for (unsigned int index = 0u; index < candidateCount; ++index) {
        const OffsetCameraVector candidate = candidates[index];
        if (!IsPlausibleDynamicProjectileOrigin(candidate.value, reference, maxDistance)) {
            continue;
        }

        bool hasMatchingPrevious = false;
        CameraVector previous{};
        for (unsigned int previousIndex = 0u; previousIndex < previousCandidateCount; ++previousIndex) {
            if (previousCandidates[previousIndex].offset != candidate.offset) {
                continue;
            }
            previous = previousCandidates[previousIndex].value;
            hasMatchingPrevious = true;
            break;
        }

        if (!hasMatchingPrevious || !IsPlausibleDynamicProjectileOrigin(previous, reference, maxDistance)) {
            continue;
        }

        const CameraVector motion{
            candidate.value.x - previous.x,
            candidate.value.y - previous.y,
            candidate.value.z - previous.z
        };
        const float motionSquared = motion.x * motion.x + motion.y * motion.y + motion.z * motion.z;
        if (!std::isfinite(motionSquared) || motionSquared < minMotionSquared || motionSquared > maxMotionSquared) {
            continue;
        }

        const CameraVector delta{
            candidate.value.x - reference.x,
            candidate.value.y - reference.y,
            candidate.value.z - reference.z
        };
        const float distanceSquared = delta.x * delta.x + delta.y * delta.y + delta.z * delta.z;
        if (!std::isfinite(distanceSquared)) {
            continue;
        }

        if (!found || distanceSquared < bestDistanceSquared) {
            selected = candidate;
            bestDistanceSquared = distanceSquared;
            found = true;
        }
    }

    return found;
}

} // namespace selfiestick::compat
