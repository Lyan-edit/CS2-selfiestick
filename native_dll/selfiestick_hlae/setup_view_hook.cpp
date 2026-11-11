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

    const bool isEngineClientCall = site.originalBytes[0] == 0x48
        && site.originalBytes[1] == 0x8B
        && site.originalBytes[2] == 0x0D
        && site.originalBytes[7] == 0x48
        && site.originalBytes[8] == 0x8B
        && site.originalBytes[9] == 0x01
        && site.originalBytes[10] == 0xFF
        && site.originalBytes[11] == 0x90
        && site.originalBytes[12] == 0x50
        && site.originalBytes[13] == 0x01
        && site.originalBytes[14] == 0x00
        && site.originalBytes[15] == 0x00;
    const bool isHlaeTrampoline = site.originalBytes[0] == 0x48
        && site.originalBytes[1] == 0x89
        && site.originalBytes[2] == 0xD9
        && site.originalBytes[3] == 0x48
        && site.originalBytes[4] == 0xB8
        && site.originalBytes[13] == 0xFF
        && site.originalBytes[14] == 0x10
        && site.originalBytes[15] == 0x90;

    if (!isEngineClientCall && !isHlaeTrampoline) {
        SetError(errorMessage, "SetUpView patch site instructions changed");
        return false;
    }

    if (isEngineClientCall) {
        const auto engineClientStorageDisplacement = *reinterpret_cast<const std::int32_t*>(site.originalBytes.data() + 3);
        site.engineClientStorageAddress = address + 7u + static_cast<std::intptr_t>(engineClientStorageDisplacement);
        site.replayKind = SetUpViewReplayKind::EngineClientIsPlayingDemo;
    }
    else {
        std::memcpy(
            &site.chainedHookPointerStorageAddress,
            site.originalBytes.data() + 5u,
            sizeof(site.chainedHookPointerStorageAddress)
        );
        if (site.chainedHookPointerStorageAddress == 0u) {
            SetError(errorMessage, "HLAE SetUpView trampoline pointer storage was null");
            return false;
        }
        site.replayKind = SetUpViewReplayKind::HlaeTrampoline;
    }
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
    switch (site.replayKind) {
    case SetUpViewReplayKind::EngineClientIsPlayingDemo:
        if (!PatchRel32(
                outBytes,
                relocatedSetupOffset + 3u,
                detourStubAddress + relocatedSetupOffset + 7u,
                site.engineClientStorageAddress,
                errorMessage,
                "SetUpView engine client storage was out of rel32 range")) {
            return false;
        }
        break;
    case SetUpViewReplayKind::HlaeTrampoline:
        if (site.chainedHookPointerStorageAddress == 0u) {
            SetError(errorMessage, "HLAE SetUpView trampoline metadata was incomplete");
            return false;
        }
        break;
    default:
        SetError(errorMessage, "SetUpView replay kind was invalid");
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

    switch (site.viewSetupRegister) {
    case ViewSetupRegister::Rsi:
        // mov rcx, rsi
        appendByte(0x48);
        appendByte(0x8B);
        appendByte(0xCE);
        break;
    case ViewSetupRegister::R14:
        // mov rcx, r14
        appendByte(0x49);
        appendByte(0x8B);
        appendByte(0xCE);
        break;
    case ViewSetupRegister::Rbx:
        // mov rcx, rbx
        appendByte(0x48);
        appendByte(0x89);
        appendByte(0xD9);
        break;
    default:
        SetError(errorMessage, "SetUpView register selection was invalid");
        return false;
    }

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

bool BuildPlayerTailDetourStub(std::uintptr_t stubAddress, std::uintptr_t returnAddress,
    std::uintptr_t callbackAddress, const std::array<std::uint8_t, kPlayerTailPatchSize>& replay,
    std::vector<std::uint8_t>& out, std::string* errorMessage) {
    constexpr std::array<std::uint8_t, kPlayerTailPatchSize> expected{
        0xF2,0x0F,0x10,0x06, 0x4C,0x8D,0x9C,0x24,0xC8,0,0,0,
        0x8B,0x46,0x08, 0x41,0x0F,0x28,0x73,0xE8};
    if (!stubAddress || !returnAddress || !callbackAddress || replay != expected) {
        SetError(errorMessage, "Player tail replay changed or address missing"); return false;
    }
    out.clear();
    const auto bytes = [&](std::initializer_list<std::uint8_t> values) { out.insert(out.end(), values); };
    const auto value32 = [&](std::uint32_t value) {
        for (unsigned i = 0; i < 4; ++i) out.push_back(static_cast<std::uint8_t>(value >> (i*8)));
    };
    // At this verified site RSP is 16-byte aligned. Reserve shadow space plus
    // seven volatile GPRs and XMM0..5; all nonvolatile registers follow Win64 ABI.
    bytes({0x48,0x81,0xEC}); value32(0xC0);
    constexpr unsigned registers[]{0,1,2,8,9,10,11};
    for (unsigned i = 0; i < 7; ++i) {
        const auto reg = registers[i];
        bytes({static_cast<std::uint8_t>(reg >= 8 ? 0x4C : 0x48),0x89,
            static_cast<std::uint8_t>(0x44 | ((reg & 7) << 3)),0x24,static_cast<std::uint8_t>(0x20+i*8)});
    }
    for (unsigned reg = 0; reg < 6; ++reg) {
        bytes({0xF3,0x0F,0x7F,static_cast<std::uint8_t>(0x84 | (reg << 3)),0x24}); value32(0x60+reg*16);
    }
    bytes({0x48,0x89,0xD9,0x48,0xB8}); // RCX = RBX = CViewSetup
    for (unsigned i = 0; i < 8; ++i) out.push_back(static_cast<std::uint8_t>(callbackAddress >> (i*8)));
    bytes({0xFF,0xD0});
    for (unsigned reg = 0; reg < 6; ++reg) {
        bytes({0xF3,0x0F,0x6F,static_cast<std::uint8_t>(0x84 | (reg << 3)),0x24}); value32(0x60+reg*16);
    }
    for (unsigned i = 0; i < 7; ++i) {
        const auto reg = registers[i];
        bytes({static_cast<std::uint8_t>(reg >= 8 ? 0x4C : 0x48),0x8B,
            static_cast<std::uint8_t>(0x44 | ((reg & 7) << 3)),0x24,static_cast<std::uint8_t>(0x20+i*8)});
    }
    bytes({0x48,0x81,0xC4}); value32(0xC0);
    out.insert(out.end(), replay.begin(), replay.end());
    const auto jump = out.size(); bytes({0xE9,0,0,0,0});
    return PatchRel32(out, jump+1, stubAddress+jump+5, returnAddress,
        errorMessage, "Player tail return out of rel32 range");
}

} // namespace selfiestick::hook
