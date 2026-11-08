// A shot owns its last valid view separately from its live entity. Auto shots
// remain replaceable; explicit pins also protect the final effect view.
bool CommitPropView(const PropViewCache& view, PropViewPhase phase) {
    std::lock_guard stateLock(g_stateMutex);
    if (g_state.lockedPropHandle.raw != view.handle.raw) return false;
    if (view.terminal && !g_state.manualPropLock && view.effectDemoTime >= 0.0
        && view.sampledDemoTime - view.effectDemoTime >= 2.0) {
        g_propAutoReleasedHandle.store(view.handle.raw, std::memory_order_release);
        g_state.lockedPropHandle = {};
        g_state.lockedPropThrowerHandle = {};
        g_state.lockedPropKind = PropProjectileKind::Unknown;
        g_state.manualPropLock = false;
        g_state.propViewPhase = PropViewPhase::Waiting;
        ResetPropTrackingState();
        RefreshStatusTextLocked();
        MarkOverlayDirty();
        // The caller returns before writing origin, angles or FOV. Leave the
        // engine's native agent camera intact and keep auto capture armed.
        return false;
    }
    {
        std::lock_guard viewLock(g_propViewMutex);
        g_propView = view;
    }
    if (g_state.propViewPhase != phase) {
        g_state.propViewPhase = phase;
        RefreshStatusTextLocked();
        MarkOverlayDirty();
    }
    return true;
}

void SamplePropEffectClock(PropViewCache& view, double demoSeconds) {
    if (!view.terminal) return;
    if (demoSeconds < 0.0 && !TryReadPropDemoSeconds(demoSeconds)) return;
    if (!std::isfinite(demoSeconds) || demoSeconds < 0.0) return;
    if (view.effectDemoTime < 0.0 || (view.sampledDemoTime >= 0.0 && demoSeconds < view.sampledDemoTime))
        view.effectDemoTime = demoSeconds;
    view.sampledDemoTime = demoSeconds;
}

bool TryBuildStablePropView(void* targetPawn, EntityHandle targetHandle,
    const PropSelection& selected, const Vec3* eye, const Vec3& offset,
    const Vec3& rotation, float legacySpeed, Vec3& position, Vec3& angles,
    std::string& failure, double now = HighResolutionSeconds(), double demoSeconds = -1.0) {
    auto view = SnapshotPropView();
    const bool cached = view.valid && view.handle.raw == selected.handle.raw;
    void* entity = cached && view.terminal ? nullptr : ResolveHandleToEntity(selected.handle);
    const bool effectStarted = entity == nullptr || HasPropEffectStarted(entity, selected.kind);
    bool built = false;
    if (!cached || !view.terminal) {
        built = TryBuildPropCameraPose(targetPawn, targetHandle, selected.handle,
            selected.throwerHandle, selected.kind, eye, offset, rotation,
            legacySpeed, position, angles, failure);
    }
    if (built) {
        if (!cached) view = {};
        view.handle = selected.handle;
        view.targetHandle = targetHandle;
        {
            std::lock_guard lock(g_propSmoothingMutex);
            view.origin = g_propSmoothing.position;
        }
        if (!TryGetCachedPropForward(selected.handle, view.forward)) return false;
        view.position = position;
        view.angles = angles;
        view.valid = true;
        view.failureSince = -1.0;
        view.terminal = effectStarted;
        SamplePropEffectClock(view, demoSeconds);
        failure.clear();
        return CommitPropView(view, view.terminal ? PropViewPhase::EffectHold : PropViewPhase::Tracking);
    }
    if (!cached) {
        // Selection can precede the new entity's first usable scene sample.
        // Keep the displayed shot until the new one has a complete pose.
        if (!view.valid || !ComposePropRigPose(view.origin, view.forward,
                offset, rotation, position, angles, failure)) return false;
        std::lock_guard lock(g_stateMutex);
        if (g_state.lockedPropHandle.raw != selected.handle.raw) return false;
        if (g_state.propViewPhase != PropViewPhase::Recovering) {
            g_state.propViewPhase = PropViewPhase::Recovering;
            RefreshStatusTextLocked();
            MarkOverlayDirty();
        }
        failure.clear();
        return true;
    }

    if (!view.terminal) {
        if (view.failureSince < 0.0) view.failureSince = now;
        // One bad scene/owner sample must not cut to the observer camera.
        // There is no extrapolation during the recovery window.
        view.terminal = effectStarted;
    }
    if (!ComposePropRigPose(view.origin, view.forward, offset, rotation, position, angles, failure)) return false;
    view.position = position;
    view.angles = angles;
    SamplePropEffectClock(view, demoSeconds);
    failure.clear();
    return CommitPropView(view, view.terminal ? PropViewPhase::EffectHold : PropViewPhase::Recovering);
}

bool ApplyPropViewFov(void* viewSetup, EntityHandle handle) {
    std::lock_guard stateLock(g_stateMutex);
    if (g_state.lockedPropHandle.raw != handle.raw) return false;
    std::lock_guard lock(g_propViewMutex);
    if (!g_propView.valid) return false;
    if (!g_propView.fovValid) {
        float fov{};
        if (!TryReadOffsetValue(viewSetup, kViewSetupFovOffset, fov)
            || !std::isfinite(fov) || fov < 1.0f || fov > 179.0f) return false;
        g_propView.fov = fov;
        g_propView.fovValid = true;
    }
    return TryWriteOffsetValue(viewSetup, kViewSetupFovOffset, g_propView.fov);
}
