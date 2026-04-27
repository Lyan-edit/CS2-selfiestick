#include "setup_view_hook.h"

#include <cstring>
#include <limits>

namespace selfiestick::hook {
namespace {

void SetError(std::string* errorMessage, const char* message) {
    if (errorMessage != nullptr) {
        *errorMessage = message;
    }
}

bool TryEncodeRel32(std::uintptr_t instructionEndAddress, std::uintptr_t targetAddress, std::int32_t& displacement) {
    const auto delta = static_cast<std::int64_t>(targetAddress) - static_cast<std::int64_t>(instructionEndAddress);
    if (delta < std::numeric_limits<std::int32_t>::min() || delta > std::numeric_limits<std::int32_t>::max()) {
        return false;
    }

    displacement = static_cast<std::int32_t>(delta);
    return true;
}

bool PatchRel32(
    std::vector<std::uint8_t>& bytes,
    std::size_t displacementOffset,
    std::uintptr_t instructionEndAddress,
    std::uintptr_t targetAddress,
    std::string* errorMessage,
    const char* label
) {
    std::int32_t displacement{};
    if (!TryEncodeRel32(instructionEndAddress, targetAddress, displacement)) {
        SetError(errorMessage, label);
        return false;
    }

    std::memcpy(bytes.data() + displacementOffset, &displacement, sizeof(displacement));
    return true;
}

} // namespace

bool DecodeSetUpViewPatchSite(const void* patchAddress, SetUpViewPatchSite& site, std::string* errorMessage) {
    if (patchAddress == nullptr) {
        SetError(errorMessage, "SetUpView patch address was null");
        return false;
    }

    auto address = reinterpret_cast<std::uintptr_t>(patchAddress);
    site = {};
    site.patchAddress = address;
    std::memcpy(site.originalBytes.data(), patchAddress, site.originalBytes.size());

    if (site.originalBytes[0] != 0xBA
        || site.originalBytes[5] != 0x48
        || site.originalBytes[6] != 0x8D
        || site.originalBytes[7] != 0x0D
        || site.originalBytes[12] != 0xE8) {
        SetError(errorMessage, "SetUpView patch site instructions changed");
        return false;
    }

    const auto helperStringDisplacement = *reinterpret_cast<const std::int32_t*>(site.originalBytes.data() + 8);
    const auto helperCallDisplacement = *reinterpret_cast<const std::int32_t*>(site.originalBytes.data() + 13);
    site.helperStringAddress = address + 12u + static_cast<std::intptr_t>(helperStringDisplacement);
    site.helperCallTarget = address + 17u + static_cast<std::intptr_t>(helperCallDisplacement);
    site.returnAddress = address + kSetUpViewPatchSize;
    return true;
}

bool BuildSetUpViewDetourStub(
    std::uintptr_t detourStubAddress,
    const SetUpViewPatchSite& site,
    std::uintptr_t callbackAddress,
    std::vector<std::uint8_t>& outBytes,
    std::string* errorMessage
) {
    if (detourStubAddress == 0u) {
        SetError(errorMessage, "detour stub address was null");
        return false;
    }

    if (site.patchAddress == 0u || site.returnAddress == 0u) {
        SetError(errorMessage, "SetUpView patch metadata was incomplete");
        return false;
    }

    outBytes.clear();
    outBytes.reserve(128);

    auto appendByte = [&outBytes](std::uint8_t value) {
        outBytes.push_back(value);
    };

    auto appendImmediate64 = [&outBytes](std::uintptr_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index) {
            outBytes.push_back(static_cast<std::uint8_t>((value >> (index * 8)) & 0xFFu));
        }
    };

    // ENDBR64 keeps the stub friendly to CET/indirect-branch hardened builds.
    appendByte(0xF3);
    appendByte(0x0F);
    appendByte(0x1E);
    appendByte(0xFA);

    const std::size_t relocatedSetupOffset = outBytes.size();
    outBytes.insert(outBytes.end(), site.originalBytes.begin(), site.originalBytes.end());
    if (!PatchRel32(
            outBytes,
            relocatedSetupOffset + 8u,
            detourStubAddress + relocatedSetupOffset + 12u,
            site.helperStringAddress,
            errorMessage,
            "SetUpView helper string was out of rel32 range")) {
        return false;
    }

    if (!PatchRel32(
            outBytes,
            relocatedSetupOffset + 13u,
            detourStubAddress + relocatedSetupOffset + 17u,
            site.helperCallTarget,
            errorMessage,
            "SetUpView helper call target was out of rel32 range")) {
        return false;
    }

    appendByte(0x48);
    appendByte(0x83);
    appendByte(0xEC);
    appendByte(0x60);

    appendByte(0x48);
    appendByte(0x89);
    appendByte(0x44);
    appendByte(0x24);
    appendByte(0x20);
    appendByte(0x48);
    appendByte(0x89);
    appendByte(0x4C);
    appendByte(0x24);
    appendByte(0x28);
    appendByte(0x48);
    appendByte(0x89);
    appendByte(0x54);
    appendByte(0x24);
    appendByte(0x30);
    appendByte(0x4C);
    appendByte(0x89);
    appendByte(0x44);
    appendByte(0x24);
    appendByte(0x38);
    appendByte(0x4C);
    appendByte(0x89);
    appendByte(0x4C);
    appendByte(0x24);
    appendByte(0x40);
    appendByte(0x4C);
    appendByte(0x89);
    appendByte(0x54);
    appendByte(0x24);
    appendByte(0x48);
    appendByte(0x4C);
    appendByte(0x89);
    appendByte(0x5C);
    appendByte(0x24);
    appendByte(0x50);

    appendByte(0x48);
    appendByte(0x8B);
    appendByte(0xCE);

    const auto callbackCallOffset = outBytes.size();
    std::int32_t callbackDisplacement{};
    if (TryEncodeRel32(detourStubAddress + callbackCallOffset + 5u, callbackAddress, callbackDisplacement)) {
        appendByte(0xE8);
        for (std::size_t index = 0; index < sizeof(callbackDisplacement); ++index) {
            appendByte(static_cast<std::uint8_t>((static_cast<std::uint32_t>(callbackDisplacement) >> (index * 8)) & 0xFFu));
        }
    }
    else {
        appendByte(0x48);
        appendByte(0xB8);
        appendImmediate64(callbackAddress);
        appendByte(0xFF);
        appendByte(0xD0);
    }

    appendByte(0x4C);
    appendByte(0x8B);
    appendByte(0x5C);
    appendByte(0x24);
    appendByte(0x50);
    appendByte(0x4C);
    appendByte(0x8B);
    appendByte(0x54);
    appendByte(0x24);
    appendByte(0x48);
    appendByte(0x4C);
    appendByte(0x8B);
    appendByte(0x4C);
    appendByte(0x24);
    appendByte(0x40);
    appendByte(0x4C);
    appendByte(0x8B);
    appendByte(0x44);
    appendByte(0x24);
    appendByte(0x38);
    appendByte(0x48);
    appendByte(0x8B);
    appendByte(0x54);
    appendByte(0x24);
    appendByte(0x30);
    appendByte(0x48);
    appendByte(0x8B);
    appendByte(0x4C);
    appendByte(0x24);
    appendByte(0x28);
    appendByte(0x48);
    appendByte(0x8B);
    appendByte(0x44);
    appendByte(0x24);
    appendByte(0x20);

    appendByte(0x48);
    appendByte(0x83);
    appendByte(0xC4);
    appendByte(0x60);

    const auto returnJumpOffset = outBytes.size();
    appendByte(0xE9);
    appendByte(0x00);
    appendByte(0x00);
    appendByte(0x00);
    appendByte(0x00);
    if (!PatchRel32(
            outBytes,
            returnJumpOffset + 1u,
            detourStubAddress + returnJumpOffset + 5u,
            site.returnAddress,
            errorMessage,
            "SetUpView return address was out of rel32 range")) {
        return false;
    }

    return true;
}

bool BuildSetUpViewEntryPatch(
    std::uintptr_t patchAddress,
    std::uintptr_t detourStubAddress,
    std::array<std::uint8_t, kSetUpViewPatchSize>& patchBytes,
    std::string* errorMessage
) {
    if (patchAddress == 0u || detourStubAddress == 0u) {
        SetError(errorMessage, "SetUpView entry patch inputs were null");
        return false;
    }

    patchBytes.fill(0x90);
    patchBytes[0] = 0xE9;

    std::int32_t displacement{};
    if (!TryEncodeRel32(patchAddress + 5u, detourStubAddress, displacement)) {
        SetError(errorMessage, "SetUpView detour stub was out of rel32 range");
        return false;
    }

    std::memcpy(patchBytes.data() + 1u, &displacement, sizeof(displacement));
    return true;
}

} // namespace selfiestick::hook
