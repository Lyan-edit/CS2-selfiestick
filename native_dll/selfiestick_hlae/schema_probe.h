#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>

namespace selfiestick::schema {

struct SchemaRawScanLayout {
    std::ptrdiff_t schemaSystemScopeArrayOffset{};
    std::ptrdiff_t typeScopeDeclaredClassCountOffset{};
    std::ptrdiff_t typeScopeBucketTableOffset{};
    std::ptrdiff_t typeScopeBucketStride{};
    std::ptrdiff_t typeScopeBucketHeadOffset{};
    std::ptrdiff_t typeScopeBucketNodeNextOffset{};
    std::ptrdiff_t typeScopeBucketNodePayloadOffset{};
    std::size_t typeScopeBucketCount{};
};

struct GenericRawFieldArrayLayoutCandidate {
    std::size_t wrapperOffset{};
    std::size_t arrayOffset{};
    std::size_t fieldStride{};
    std::size_t fieldNameOffset{};
    std::size_t fieldOffsetOffset{};
};

std::uint32_t DetermineDeclaredClassScanLimit(std::uint32_t rawCount) noexcept;
std::size_t DetermineTypeScopeLookupProbeSlotLimit() noexcept;
const char* DetermineExpectedProbeField(const char* className) noexcept;
bool IsUniqueRawSchemaCandidateCount(std::size_t candidateCount) noexcept;
bool ShouldStartBackgroundSchemaResolve(
    bool schemaResolved,
    bool schemaResolveInProgress,
    bool hasRecentFailure,
    unsigned long long millisSinceLastAttempt,
    unsigned long long retryIntervalMs
) noexcept;
bool ShouldUseSchemaInterfaceFallback(bool rawScopeFound, bool rawPayloadsCollected, bool rawOffsetsComplete) noexcept;
bool BuildQualifiedClassName(const char* moduleName, const char* className, std::string& qualifiedName);
std::size_t BuildSchemaClassLookupCandidates(
    const char* moduleName,
    const char* className,
    std::array<std::string, 2>& candidates
);
bool TryGetKnownClientSchemaFieldOffsetFallback(
    const char* className,
    const char* fieldName,
    std::ptrdiff_t& target
) noexcept;
std::size_t BuildDeclaredClassSchemaClassPointerOffsets(std::array<std::size_t, 8>& offsets) noexcept;
std::size_t BuildFocusedGenericRawFieldArrayLayoutCandidates(
    std::array<GenericRawFieldArrayLayoutCandidate, 12>& candidates
) noexcept;
bool TryGetSchemaClassFromDeclaredClassPayload(const void* payload, void*& schemaClass) noexcept;
SchemaRawScanLayout GetSchemaRawScanLayout() noexcept;

} // namespace selfiestick::schema
