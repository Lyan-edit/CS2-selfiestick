// Fixed-size, nonblocking producer. Explicit export alone writes to disk.
struct PlayerMotionSample {
    std::uint64_t sequence{};
    double time{};
    std::uint32_t expected{}, target{}, weapon{};
    Vec3 body{}, anchor{}, aim{}, camera{}, angles{};
    float fov{};
    int result{}, look{};
    bool recovering{}, suppressed{};
    double yawInput{}, yawOutput{}, demoRate{};
    int demoTick{};
    bool paused{}, turnFiltered{};
    float sourceFov{};
    DWORD threadId{};
    int callbackStage{};
    double renderDemoSeconds{};
    int renderFrame{};
    bool renderTimeValid{};
    float renderInterpolation{};
    Vec3 nativeOrigin{}, nativeAngles{};
    float duckAmount{}, bodyHeight{};
};
thread_local Vec3 g_playerNativeViewOrigin{}, g_playerNativeViewAngles{};
std::mutex g_playerMotionMutex;
// At 1200 FPS, 8192 early/final records cover about 3.4 seconds. Camera
// callbacks still perform bounded memory writes only; export alone writes disk.
std::array<PlayerMotionSample, 8192> g_playerMotionSamples{};
std::size_t g_playerMotionHead{}, g_playerMotionCount{};
std::uint64_t g_playerMotionSequence{};
std::atomic<bool> g_playerMotionExportBusy{false};
std::wstring g_playerMotionExportMessage;

std::size_t PlayerMotionSampleCount() {
    std::lock_guard lock(g_playerMotionMutex); return g_playerMotionCount;
}
std::wstring PlayerMotionExportMessage() {
    std::lock_guard lock(g_playerMotionMutex); return g_playerMotionExportMessage;
}
void RecordPlayerMotionFrame(void* viewSetup, const OverrideConfig& config, EntityHandle target,
    const WeaponFrameBasis* basis, int result, bool recovering) {
    PlayerMotionSample sample{};
    // Keep the large backing array zero-initialized (.bss), while individual
    // records retain the explicit unavailable-clock sentinels in the CSV.
    sample.demoRate = 1.0; sample.demoTick = sample.renderFrame = -1;
    {
        std::unique_lock suppressionLock(g_viewModelSuppressionMutex, std::try_to_lock);
        if (!suppressionLock.owns_lock()) return;
        sample.suppressed = g_viewModelSuppression.active;
    }
    sample.time = HighResolutionSeconds();
    sample.expected = (config.targetMode == TargetMode::Locked ? config.lockedHandle : config.followHandle).raw;
    sample.target = target.raw; sample.result = result; sample.recovering = recovering;
    sample.look = static_cast<int>(config.lookMode);
    sample.threadId = GetCurrentThreadId();
    sample.callbackStage = g_playerCallbackStage;
    if (result == 2) {
        sample.renderDemoSeconds = g_lastPlayerMotionClock.renderDemoSeconds;
        sample.renderFrame = g_lastPlayerMotionClock.renderFrame;
        sample.renderTimeValid = g_lastPlayerMotionClock.renderTimeValid;
        sample.renderInterpolation = g_lastPlayerMotionClock.interpolation;
        sample.nativeOrigin = g_playerNativeViewOrigin; sample.nativeAngles = g_playerNativeViewAngles;
        sample.bodyHeight = g_playerFrame.bodyHeight;
        sample.duckAmount = std::numeric_limits<float>::quiet_NaN();
        std::ptrdiff_t movementOffset{}, duckOffset{}; void* movement = nullptr;
        if (g_knownClientSchemaFallbackAllowed.load(std::memory_order_acquire)
            && schema::TryGetKnownClientSchemaFieldOffsetFallback("C_BasePlayerPawn", "m_pMovementServices", movementOffset)
            && schema::TryGetKnownClientSchemaFieldOffsetFallback("CCSPlayer_MovementServices", "m_flDuckAmount", duckOffset)
            && TryReadOffsetPointer(g_playerView.entity, movementOffset, movement))
            TryReadOffsetValue(movement, duckOffset, sample.duckAmount);
    }
    sample.yawInput = g_playerFrame.angles.y;
    sample.yawOutput = g_playerFrame.turn.valid ? g_playerFrame.turn.yaw : g_playerFrame.angles.y;
    sample.turnFiltered = g_playerFrame.turn.valid;
    if (sample.turnFiltered) {
        sample.demoTick = g_playerFrame.turn.tick;
        sample.demoRate = g_playerFrame.turn.rate;
        sample.paused = g_playerFrame.turn.paused;
    }
    if (basis != nullptr) {
        sample.body = basis->bodyOrigin; sample.anchor = basis->anchorOrigin;
        sample.aim = basis->upperBodyTarget; sample.weapon = basis->weaponHandle.raw;
    }
    TryReadViewSetupOrigin(viewSetup, sample.camera);
    TryReadViewSetupAngles(viewSetup, sample.angles);
    TryReadOffsetValue(viewSetup, kViewSetupFovOffset, sample.fov);
    sample.sourceFov = result == 2 ? g_playerView.sourceFov : sample.fov;
    std::unique_lock lock(g_playerMotionMutex, std::try_to_lock);
    if (!lock.owns_lock()) return;
    sample.sequence = ++g_playerMotionSequence;
    g_playerMotionSamples[g_playerMotionHead] = sample;
    g_playerMotionHead = (g_playerMotionHead + 1u) % g_playerMotionSamples.size();
    g_playerMotionCount = std::min(g_playerMotionCount + 1u, g_playerMotionSamples.size());
}
std::wstring PlayerMotionCsvPath() {
    auto path = GetTraceFilePath();
    const auto slash = path.find_last_of(L"\\/");
    return (slash == std::wstring::npos ? L"" : path.substr(0, slash + 1)) + L"selfiestick_player_motion.csv";
}
DWORD WINAPI ExportPlayerMotionWorker(LPVOID) {
    std::vector<PlayerMotionSample> samples;
    samples.reserve(g_playerMotionSamples.size());
    {
        std::lock_guard lock(g_playerMotionMutex);
        const auto start = (g_playerMotionHead + g_playerMotionSamples.size() - g_playerMotionCount) % g_playerMotionSamples.size();
        for (std::size_t i = 0; i < g_playerMotionCount; ++i)
            samples.push_back(g_playerMotionSamples[(start + i) % g_playerMotionSamples.size()]);
    }
    std::ostringstream csv;
    csv.imbue(std::locale::classic()); csv.precision(17);
    csv << "# build=" << kBuildTag << "\n# result:0=no_target,1=no_pose,2=overridden,3=pre_native; stage:0=direct_test,1=early,2=final\n"
        << "sequence,time,expected,target,weapon,result,look,recovering,suppressed,body_x,body_y,body_z,anchor_x,anchor_y,anchor_z,"
        << "aim_x,aim_y,aim_z,camera_x,camera_y,camera_z,pitch,yaw,roll,fov,yaw_input,yaw_output,estimated_demo_rate,demo_tick,demo_paused,turn_filtered,source_fov,thread_id,callback_stage,"
        << "render_demo_seconds,render_frame,render_time_valid,render_interpolation,native_camera_x,native_camera_y,native_camera_z,native_pitch,native_yaw,native_roll,duck_amount,body_height\n";
    for (const auto& s : samples) {
        csv << s.sequence << ',' << s.time << ',' << s.expected << ',' << s.target << ',' << s.weapon << ','
            << s.result << ',' << s.look << ',' << s.recovering << ',' << s.suppressed << ','
            << s.body.x << ',' << s.body.y << ',' << s.body.z << ',' << s.anchor.x << ',' << s.anchor.y << ',' << s.anchor.z << ','
            << s.aim.x << ',' << s.aim.y << ',' << s.aim.z << ',' << s.camera.x << ',' << s.camera.y << ',' << s.camera.z << ','
            << s.angles.x << ',' << s.angles.y << ',' << s.angles.z << ',' << s.fov << ','
            << s.yawInput << ',' << s.yawOutput << ',' << s.demoRate << ',' << s.demoTick << ',' << s.paused << ',' << s.turnFiltered << ','
            << s.sourceFov << ',' << s.threadId << ',' << s.callbackStage << ','
            << s.renderDemoSeconds << ',' << s.renderFrame << ',' << s.renderTimeValid << ',' << s.renderInterpolation << ','
            << s.nativeOrigin.x << ',' << s.nativeOrigin.y << ',' << s.nativeOrigin.z << ','
            << s.nativeAngles.x << ',' << s.nativeAngles.y << ',' << s.nativeAngles.z << ',' << s.duckAmount << ',' << s.bodyHeight << '\n';
    }
    const auto bytes = csv.str(); const auto path = PlayerMotionCsvPath();
    const HANDLE file = CreateFileW(path.c_str(), GENERIC_WRITE, FILE_SHARE_READ, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    DWORD written{};
    const bool saved = file != INVALID_HANDLE_VALUE && WriteFile(file, bytes.data(), static_cast<DWORD>(bytes.size()), &written, nullptr)
        && written == bytes.size();
    if (file != INVALID_HANDLE_VALUE) CloseHandle(file);
    {
        std::lock_guard lock(g_playerMotionMutex);
#if defined(SELFIESTICK_LANG_ZH_CN)
        g_playerMotionExportMessage = saved ? L"人物诊断已导出：" + path : L"人物诊断导出失败";
#else
        g_playerMotionExportMessage = saved ? L"Player diagnostics exported: " + path : L"Player export failed";
#endif
    }
    g_playerMotionExportBusy.store(false, std::memory_order_release); MarkOverlayDirty();
    return saved ? 0u : 1u;
}
void StartPlayerMotionExport() {
    if (g_playerMotionExportBusy.exchange(true, std::memory_order_acq_rel)) return;
    const HANDLE thread = CreateThread(nullptr, 0, ExportPlayerMotionWorker, nullptr, 0, nullptr);
    if (thread != nullptr) CloseHandle(thread); else g_playerMotionExportBusy.store(false, std::memory_order_release);
}
