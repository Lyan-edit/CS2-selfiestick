#pragma once

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

ClientEntityKind ClassifyEntityClassName(std::string_view className);
bool CanResolveFollowTarget(const RuntimeCompatibility& compatibility) noexcept;
bool CanProbeSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept;
bool CanInstallSetUpViewHook(const RuntimeCompatibility& compatibility) noexcept;
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
CameraOffset ApplyLeftSelfieCameraOffsetAdjustment(CameraOffset offset, bool isLeftSelfieLookMode) noexcept;

} // namespace selfiestick::compat
