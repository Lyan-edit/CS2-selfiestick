// Startup/enable-time scanning only. Never dereference unreadable image pages;
// a protection change during scanning invalidates the complete search result.
std::vector<int> ParsePattern(const char* pattern) {
    std::vector<int> bytes;
    if (pattern == nullptr) return bytes;
    const auto hex = [](unsigned char c) -> int {
        if (c >= '0' && c <= '9') return c - '0';
        if (c >= 'a' && c <= 'f') return c - 'a' + 10;
        if (c >= 'A' && c <= 'F') return c - 'A' + 10;
        return -1;
    };
    while (*pattern) {
        if (std::isspace(static_cast<unsigned char>(*pattern))) { ++pattern; continue; }
        if (*pattern == '?') {
            ++pattern;
            if (*pattern == '?') ++pattern;
            bytes.push_back(-1);
        } else {
            const int high = hex(static_cast<unsigned char>(pattern[0]));
            if (high < 0 || pattern[1] == '\0') return {};
            const int low = hex(static_cast<unsigned char>(pattern[1]));
            if (low < 0) return {};
            bytes.push_back(high * 16 + low); pattern += 2;
        }
        if (*pattern && !std::isspace(static_cast<unsigned char>(*pattern))) return {};
    }
    return bytes;
}

bool ScanReadablePatternRange(std::uintptr_t begin, std::uintptr_t end,
    const int* signature, std::size_t length, std::uintptr_t* matches,
    std::size_t maxMatches, std::size_t& count) {
    if (end - begin < length) return true;
    __try {
        for (auto address = begin; address <= end - length; ++address) {
            bool found = true;
            const auto* bytes = reinterpret_cast<const std::uint8_t*>(address);
            for (std::size_t i = 0; i < length; ++i) {
                if (signature[i] >= 0 && bytes[i] != static_cast<std::uint8_t>(signature[i])) {
                    found = false; break;
                }
            }
            if (found) {
                matches[count++] = address;
                if (count == maxMatches) return true;
            }
        }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}

std::vector<std::uintptr_t> FindPatterns(HMODULE module, const char* pattern, std::size_t maxMatches = 8) {
    if (module == nullptr || maxMatches == 0 || !IsAddressReadableInModule(module, module)) return {};
    IMAGE_DOS_HEADER dos{};
    IMAGE_NT_HEADERS nt{};
    const auto base = reinterpret_cast<std::uintptr_t>(module);
    if (!TryReadMemoryValue(reinterpret_cast<const IMAGE_DOS_HEADER*>(base), dos)
        || dos.e_magic != IMAGE_DOS_SIGNATURE || dos.e_lfanew < static_cast<LONG>(sizeof(dos)) || dos.e_lfanew > 0x100000
        || !TryReadMemoryValue(reinterpret_cast<const IMAGE_NT_HEADERS*>(base + dos.e_lfanew), nt)
        || nt.Signature != IMAGE_NT_SIGNATURE || nt.OptionalHeader.Magic != IMAGE_NT_OPTIONAL_HDR_MAGIC) return {};
    const std::size_t imageSize = nt.OptionalHeader.SizeOfImage;
    const auto signature = ParsePattern(pattern);
    if (signature.empty() || imageSize < signature.size() || imageSize > 0x40000000u
        || base > std::numeric_limits<std::uintptr_t>::max() - imageSize) return {};
    const auto end = base + imageSize;
    std::vector<std::uintptr_t> matches(std::min(maxMatches, imageSize - signature.size() + 1u));
    std::size_t count = 0;
    std::uintptr_t run = 0;
    for (auto cursor = base; cursor < end;) {
        MEMORY_BASIC_INFORMATION region{};
        if (VirtualQuery(reinterpret_cast<const void*>(cursor), &region, sizeof(region)) == 0) return {};
        const auto next = std::min(end, reinterpret_cast<std::uintptr_t>(region.BaseAddress) + region.RegionSize);
        if (next <= cursor || region.AllocationBase != module) return {};
        const bool readable = region.State == MEM_COMMIT && !(region.Protect & PAGE_GUARD)
            && IsReadableProtection(region.Protect);
        if (readable) {
            if (run == 0) run = cursor;
        } else if (run != 0) {
            if (!ScanReadablePatternRange(run, cursor, signature.data(), signature.size(), matches.data(), matches.size(), count)) return {};
            if (count == matches.size()) return matches;
            run = 0;
        }
        cursor = next;
    }
    if (run != 0 && !ScanReadablePatternRange(run, end, signature.data(), signature.size(), matches.data(), matches.size(), count)) return {};
    matches.resize(count);
    return matches;
}

std::uintptr_t FindPattern(HMODULE module, const char* pattern) {
    const auto matches = FindPatterns(module, pattern, 1);
    return matches.empty() ? 0u : matches.front();
}
