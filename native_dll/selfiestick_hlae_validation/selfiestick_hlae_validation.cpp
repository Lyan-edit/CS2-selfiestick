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
    using selfiestick::compat::ApplyLeftSelfieCameraOffsetAdjustment;
    using selfiestick::compat::CameraOffset;
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

    if (CanInstallSetUpViewHook(ready)) {
        return ReportFailure("SetUpView hook must not install while schema offsets are unresolved");
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
