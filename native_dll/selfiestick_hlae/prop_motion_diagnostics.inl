// Bounded in-memory samples only. CSV writes run on an explicit export worker,
// never from camera callbacks, and contain no entity pointers or player names.
struct PropMotionSample {
    double time{};
    std::uint64_t sequence{}, originSequence{}, dirtyResolves{};
    std::uint32_t selected{}, focus{}, displayed{}, target{};
    Vec3 cachedOrigin{}, resolvedOrigin{}, camera{}, angles{};
    float fov{};
    int phase{};
    bool pinned{}, rawValid{}, engineResolved{};
    std::uint8_t dirty{};
};
std::mutex g_propMotionMutex;
std::array<PropMotionSample, 1024> g_propMotionSamples{};
std::size_t g_propMotionHead{}, g_propMotionCount{};
std::uint64_t g_propMotionSequence{};
std::atomic<bool> g_propMotionExportBusy{false};
std::wstring g_propMotionExportMessage;

void PushPropMotionSample(PropMotionSample sample) {
    std::unique_lock lock(g_propMotionMutex, std::try_to_lock);
    if (!lock.owns_lock()) return;
    sample.sequence = ++g_propMotionSequence;
    g_propMotionSamples[g_propMotionHead] = sample;
    g_propMotionHead = (g_propMotionHead + 1u) % g_propMotionSamples.size();
    g_propMotionCount = std::min(g_propMotionCount + 1u, g_propMotionSamples.size());
}

std::size_t PropMotionSampleCount() {
    std::lock_guard lock(g_propMotionMutex);
    return g_propMotionCount;
}

std::wstring PropMotionExportMessage() {
    std::lock_guard lock(g_propMotionMutex);
    return g_propMotionExportMessage;
}

void RecordPropMotionFrame(const PropSelection& selected, const Vec3& camera, const Vec3& angles) {
    PropMotionSample sample{};
    const auto view = SnapshotPropView();
    sample.time = HighResolutionSeconds();
    sample.selected = selected.handle.raw;
    sample.displayed = view.handle.raw;
    sample.target = view.targetHandle.raw;
    sample.camera = camera;
    sample.angles = angles;
    sample.fov = view.fov;
    sample.dirtyResolves = g_propDirtyOriginResolves.load(std::memory_order_relaxed);
    {
        std::lock_guard lock(g_stateMutex);
        sample.focus = g_state.lockedPropHandle.raw;
        sample.phase = static_cast<int>(g_state.propViewPhase);
        sample.pinned = g_state.manualPropLock;
    }
    {
        std::lock_guard lock(g_propOriginHistoryMutex);
        if (g_propOriginHistory.valid && g_propOriginHistory.propHandle.raw == view.handle.raw) {
            sample.rawValid = true;
            sample.cachedOrigin = g_propOriginHistory.cachedBeforeResolve;
            sample.resolvedOrigin = g_propOriginHistory.lastOrigin;
            sample.engineResolved = g_propOriginHistory.engineResolved;
            sample.dirty = g_propOriginHistory.dirtyBeforeResolve;
            sample.originSequence = g_propOriginHistory.readSequence;
        }
    }
    PushPropMotionSample(sample);
}

std::wstring PropMotionCsvPath() {
    std::wstring path = GetTraceFilePath();
    const auto slash = path.find_last_of(L"\\/");
    path = slash == std::wstring::npos ? L"" : path.substr(0, slash + 1);
    return path + L"selfiestick_prop_motion.csv";
}

DWORD WINAPI ExportPropMotionWorker(LPVOID) {
    std::vector<PropMotionSample> snapshot;
    snapshot.reserve(g_propMotionSamples.size());
    {
        std::lock_guard lock(g_propMotionMutex);
        const auto start = (g_propMotionHead + g_propMotionSamples.size() - g_propMotionCount) % g_propMotionSamples.size();
        for (std::size_t i = 0; i < g_propMotionCount; ++i)
            snapshot.push_back(g_propMotionSamples[(start + i) % g_propMotionSamples.size()]);
    }
    std::ostringstream csv;
    csv.imbue(std::locale::classic());
    csv.precision(9);
    csv << "# build=" << kBuildTag << "\n# marker=" << kSchemaProbeTraceVersion
        << "\nsequence,time,selected,focus,displayed,target,phase,pinned,raw_valid,origin_sequence,engine_resolved,dirty_before,total_dirty_resolves,"
        << "cached_x,cached_y,cached_z,resolved_x,resolved_y,resolved_z,camera_x,camera_y,camera_z,pitch,yaw,roll,fov\n";
    for (const auto& sample : snapshot) {
        csv << sample.sequence << ',' << sample.time << ',' << sample.selected << ',' << sample.focus << ','
            << sample.displayed << ',' << sample.target << ',' << sample.phase << ',' << sample.pinned << ','
            << sample.rawValid << ',' << sample.originSequence << ',' << sample.engineResolved << ',' << static_cast<unsigned int>(sample.dirty) << ','
            << sample.dirtyResolves << ',' << sample.cachedOrigin.x << ',' << sample.cachedOrigin.y << ',' << sample.cachedOrigin.z << ','
            << sample.resolvedOrigin.x << ',' << sample.resolvedOrigin.y << ',' << sample.resolvedOrigin.z << ','
            << sample.camera.x << ',' << sample.camera.y << ',' << sample.camera.z << ','
            << sample.angles.x << ',' << sample.angles.y << ',' << sample.angles.z << ',' << sample.fov << '\n';
    }
    const auto path = PropMotionCsvPath();
    const auto bytes = csv.str();
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written{};
    const bool saved = file != INVALID_HANDLE_VALUE
        && WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
        && written == bytes.size();
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    {
        std::lock_guard lock(g_propMotionMutex);
#if defined(SELFIESTICK_LANG_ZH_CN)
        g_propMotionExportMessage = saved ? L"已导出：" + path : L"导出失败，请检查 DLL 目录是否可写";
#else
        g_propMotionExportMessage = saved ? L"Exported: " + path : L"Export failed; check DLL folder write access";
#endif
    }
    g_propMotionExportBusy.store(false, std::memory_order_release);
    MarkOverlayDirty();
    return saved ? 0u : 1u;
}

void StartPropMotionExport() {
    if (g_propMotionExportBusy.exchange(true, std::memory_order_acq_rel)) return;
    const HANDLE thread = CreateThread(nullptr, 0, ExportPropMotionWorker, nullptr, 0, nullptr);
    if (thread != nullptr) CloseHandle(thread);
    else g_propMotionExportBusy.store(false, std::memory_order_release);
}
