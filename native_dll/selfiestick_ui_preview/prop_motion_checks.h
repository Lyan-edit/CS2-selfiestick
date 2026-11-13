#pragma once

namespace prop_motion_checks {
inline bool Run() {
    using namespace selfiestick;
    for (unsigned int index = 0; index < 1100u; ++index) {
        PropMotionSample sample{};
        sample.selected = index;
        sample.camera = {static_cast<float>(index), 200, 100};
        PushPropMotionSample(sample);
    }
    if (PropMotionSampleCount() != 1024u) return false;
    if (ExportPropMotionWorker(nullptr) != 0u) return false;
    const auto path = PropMotionCsvPath();
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_READ, FILE_SHARE_READ, nullptr, OPEN_EXISTING, 0, nullptr);
    if (file == INVALID_HANDLE_VALUE) return false;
    const DWORD size = GetFileSize(file, nullptr);
    std::vector<char> bytes(size);
    DWORD read{};
    const bool loaded = ReadFile(file, bytes.data(), size, &read, nullptr) && read == size;
    CloseHandle(file);
    const std::string csv(bytes.begin(), bytes.end());
    const auto newlineCount = std::count(csv.begin(), csv.end(), '\n');
    const bool passed = loaded && newlineCount == 1027
        && csv.find("schema-probe-v43-prop-origin-cs2-26598758") != std::string::npos
        && csv.find("engine_resolved,dirty_before,total_dirty_resolves") != std::string::npos;
    if (passed) puts("prop motion diagnostics regression passed: bounded 1024-sample ring and explicit CSV export outside camera callbacks");
    return passed;
}
}
