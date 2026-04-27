#include "compatibility_gate.h"

namespace selfiestick::compat {
namespace {

constexpr float kLeftSelfieAdditionalRightOffset = 15.0f;
constexpr float kLeftSelfieAdditionalBackOffset = 10.0f;

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

CameraOffset ApplyLeftSelfieCameraOffsetAdjustment(CameraOffset offset, bool isLeftSelfieLookMode) noexcept {
    if (!isLeftSelfieLookMode) {
        return offset;
    }

    offset.right += kLeftSelfieAdditionalRightOffset;
    offset.back += kLeftSelfieAdditionalBackOffset;
    return offset;
}

} // namespace selfiestick::compat
