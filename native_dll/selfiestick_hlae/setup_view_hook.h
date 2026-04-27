#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace selfiestick::hook {

constexpr std::size_t kSetUpViewPatchSize = 17;

struct SetUpViewPatchSite {
    std::uintptr_t patchAddress{};
    std::uintptr_t helperStringAddress{};
    std::uintptr_t helperCallTarget{};
    std::uintptr_t returnAddress{};
    std::array<std::uint8_t, kSetUpViewPatchSize> originalBytes{};
};

bool DecodeSetUpViewPatchSite(const void* patchAddress, SetUpViewPatchSite& site, std::string* errorMessage = nullptr);
bool BuildSetUpViewDetourStub(
    std::uintptr_t detourStubAddress,
    const SetUpViewPatchSite& site,
    std::uintptr_t callbackAddress,
    std::vector<std::uint8_t>& outBytes,
    std::string* errorMessage = nullptr
);
bool BuildSetUpViewEntryPatch(
    std::uintptr_t patchAddress,
    std::uintptr_t detourStubAddress,
    std::array<std::uint8_t, kSetUpViewPatchSize>& patchBytes,
    std::string* errorMessage = nullptr
);

} // namespace selfiestick::hook
