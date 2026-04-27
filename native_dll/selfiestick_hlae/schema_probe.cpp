#include "schema_probe.h"

#include <cstring>

namespace selfiestick::schema {
namespace {

constexpr std::uint32_t kFallbackDeclaredClassScanLimit = 32768u;
constexpr std::uint32_t kMinReliableDeclaredClassCount = 512u;
constexpr std::size_t kTypeScopeLookupProbeSlotLimit = 64u;
constexpr std::size_t kDeclaredClassSchemaClassPointerOffset = 0x20u;
constexpr std::array<std::size_t, 8> kDeclaredClassSchemaClassPointerOffsets{
    0x20u,
    0x18u,
    0x28u,
    0x30u,
    0x38u,
    0x40u,
    0x48u,
    0x50u
};
constexpr std::array<GenericRawFieldArrayLayoutCandidate, 12> kFocusedGenericRawFieldArrayLayoutCandidates{ {
    { 0x40u, 0x0u, 0x20u, 0x08u, 0x10u },
    { 0x40u, 0x0u, 0x20u, 0x10u, 0x10u },
    { 0x40u, 0x0u, 0x20u, 0x00u, 0x20u },
    { 0x40u, 0x0u, 0x20u, 0x08u, 0x20u },
    { 0x40u, 0x0u, 0x20u, 0x00u, 0x10u },
    { 0x38u, 0x10u, 0x20u, 0x08u, 0x10u },
    { 0x38u, 0x68u, 0x20u, 0x08u, 0x10u },
    { 0x30u, 0x08u, 0x20u, 0x08u, 0x10u },
    { 0x00u, 0x58u, 0x20u, 0x08u, 0x10u },
    { 0x00u, 0x70u, 0x20u, 0x08u, 0x10u },
    { 0x40u, 0xC8u, 0x20u, 0x08u, 0x10u },
    { 0x40u, 0x0u, 0x28u, 0x08u, 0x10u }
} };

} // namespace

std::uint32_t DetermineDeclaredClassScanLimit(std::uint32_t rawCount) noexcept {
    if (rawCount < kMinReliableDeclaredClassCount) {
        return kFallbackDeclaredClassScanLimit;
    }

    return rawCount;
}

std::size_t DetermineTypeScopeLookupProbeSlotLimit() noexcept {
    return kTypeScopeLookupProbeSlotLimit;
}

const char* DetermineExpectedProbeField(const char* className) noexcept {
    if (className == nullptr) {
        return nullptr;
    }

    if (0 == std::strcmp(className, "CEntityInstance")) {
        return "m_pEntity";
    }

    if (0 == std::strcmp(className, "C_BaseEntity")) {
        return "m_pGameSceneNode";
    }

    if (0 == std::strcmp(className, "C_BasePlayerPawn")) {
        return "m_pObserverServices";
    }

    if (0 == std::strcmp(className, "C_CSPlayerPawn")) {
        return "m_angEyeAngles";
    }

    if (0 == std::strcmp(className, "CBasePlayerController")
        || 0 == std::strcmp(className, "CCSPlayerController")) {
        return "m_hPawn";
    }

    return nullptr;
}

bool IsUniqueRawSchemaCandidateCount(std::size_t candidateCount) noexcept {
    return candidateCount == 1u;
}

bool ShouldStartBackgroundSchemaResolve(
    bool schemaResolved,
    bool schemaResolveInProgress,
    bool hasRecentFailure,
    unsigned long long millisSinceLastAttempt,
    unsigned long long retryIntervalMs
) noexcept {
    if (schemaResolved || schemaResolveInProgress) {
        return false;
    }

    if (!hasRecentFailure) {
        return true;
    }

    return millisSinceLastAttempt >= retryIntervalMs;
}

bool ShouldUseSchemaInterfaceFallback(bool rawScopeFound, bool rawPayloadsCollected, bool rawOffsetsComplete) noexcept {
    if (rawOffsetsComplete) {
        return false;
    }

    return !(rawScopeFound && rawPayloadsCollected);
}

bool BuildQualifiedClassName(const char* moduleName, const char* className, std::string& qualifiedName) {
    qualifiedName.clear();
    if (moduleName == nullptr || className == nullptr || *moduleName == '\0' || *className == '\0') {
        return false;
    }

    qualifiedName.reserve(std::strlen(moduleName) + 1u + std::strlen(className));
    qualifiedName.append(moduleName);
    qualifiedName.push_back('!');
    qualifiedName.append(className);
    return true;
}

std::size_t BuildSchemaClassLookupCandidates(
    const char* moduleName,
    const char* className,
    std::array<std::string, 2>& candidates
) {
    candidates[0].clear();
    candidates[1].clear();
    if (className == nullptr || *className == '\0') {
        return 0u;
    }

    candidates[0] = className;
    if (BuildQualifiedClassName(moduleName, className, candidates[1])) {
        return 2u;
    }

    return 1u;
}

bool TryGetKnownClientSchemaFieldOffsetFallback(
    const char* className,
    const char* fieldName,
    std::ptrdiff_t& target
) noexcept {
    target = 0;
    if (className == nullptr || fieldName == nullptr) {
        return false;
    }

    auto matches = [className, fieldName](const char* expectedClass, const char* expectedField) noexcept {
        return 0 == std::strcmp(className, expectedClass) && 0 == std::strcmp(fieldName, expectedField);
    };

    if (matches("CEntityInstance", "m_pEntity")) {
        target = 0x10;
    }
    else if (matches("CBasePlayerController", "m_hPawn") || matches("CCSPlayerController", "m_hPlayerPawn")) {
        target = 0x6C4;
    }
    else if (matches("C_BasePlayerPawn", "m_pWeaponServices")) {
        target = 0x11E0;
    }
    else if (matches("C_BasePlayerPawn", "m_pObserverServices")) {
        target = 0x11F8;
    }
    else if (matches("C_BasePlayerPawn", "m_pCameraServices")) {
        target = 0x1218;
    }
    else if (matches("C_BasePlayerPawn", "m_hController")) {
        target = 0x13A8;
    }
    else if (matches("CPlayer_ObserverServices", "m_iObserverMode")) {
        target = 0x48;
    }
    else if (matches("CPlayer_ObserverServices", "m_hObserverTarget")) {
        target = 0x4C;
    }
    else if (matches("CPlayer_WeaponServices", "m_hActiveWeapon")) {
        target = 0x60;
    }
    else if (matches("C_BaseEntity", "m_pGameSceneNode")) {
        target = 0x338;
    }

    return target != 0;
}

std::size_t BuildDeclaredClassSchemaClassPointerOffsets(std::array<std::size_t, 8>& offsets) noexcept {
    offsets = kDeclaredClassSchemaClassPointerOffsets;
    return kDeclaredClassSchemaClassPointerOffsets.size();
}

std::size_t BuildFocusedGenericRawFieldArrayLayoutCandidates(
    std::array<GenericRawFieldArrayLayoutCandidate, 12>& candidates
) noexcept {
    candidates = kFocusedGenericRawFieldArrayLayoutCandidates;
    return kFocusedGenericRawFieldArrayLayoutCandidates.size();
}

bool TryGetSchemaClassFromDeclaredClassPayload(const void* payload, void*& schemaClass) noexcept {
    schemaClass = nullptr;
    if (payload == nullptr) {
        return false;
    }

    const auto* payloadBytes = static_cast<const std::byte*>(payload);
    schemaClass = *reinterpret_cast<void* const*>(payloadBytes + kDeclaredClassSchemaClassPointerOffset);
    return schemaClass != nullptr;
}

SchemaRawScanLayout GetSchemaRawScanLayout() noexcept {
    return SchemaRawScanLayout{
        .schemaSystemScopeArrayOffset = 0x198,
        .typeScopeDeclaredClassCountOffset = 0x56C,
        .typeScopeBucketTableOffset = 0x5C0,
        .typeScopeBucketStride = 0x18,
        .typeScopeBucketHeadOffset = 0x10,
        .typeScopeBucketNodeNextOffset = 0x08,
        .typeScopeBucketNodePayloadOffset = 0x10,
        .typeScopeBucketCount = 0x100u
    };
}

} // namespace selfiestick::schema
