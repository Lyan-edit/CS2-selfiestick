// Included inside namespace selfiestick after the projectile accessors.
void UpdatePropCandidatesForTarget(void* targetPawn, EntityHandle targetHandle,
    double nowSeconds = HighResolutionSeconds()) {
    std::unique_lock scanLock(g_propCandidateScanMutex, std::try_to_lock);
    if (!scanLock.owns_lock()) return;
    auto& scan = g_propCandidateScan;
    if (targetPawn == nullptr || !targetHandle.IsValid() || g_entityListStorage == nullptr
        || *g_entityListStorage == nullptr) {
        scan.entityList = nullptr;
        return;
    }
    void* entityList = *g_entityListStorage;
    // The function extracted by the old signature returns ENTITY COUNT
    // (this+0x2120/+0x2124), not the largest occupied handle index. Sparse
    // high slots must remain searchable regardless of that count.
    constexpr int highestIndex = static_cast<int>(kMaxPropEntitySlots) - 1;
    const EntityHandle controllerHandle = PawnGetControllerHandle(targetPawn);
    const bool explicitReset = g_propDiscoveryResetRequested.exchange(false, std::memory_order_acq_rel);
    const bool sameTarget = !explicitReset && scan.entityList == entityList && scan.targetHandle.raw == targetHandle.raw;
    float simulationTime{};
    const bool simulationTimeValid = TryReadOffsetValue(targetPawn, g_offsets.entitySimulationTime, simulationTime)
        && std::isfinite(simulationTime) && simulationTime >= 0.0f && simulationTime < 10000000.0f;
    const bool timelineRewound = sameTarget && simulationTimeValid && scan.simulationTimeValid
        && simulationTime < scan.lastSimulationTime - 0.25f;
    const bool targetChanged = !sameTarget || timelineRewound;
    if (targetChanged) {
        scan.entityList = entityList;
        scan.targetHandle = targetHandle;
        scan.controllerHandle = controllerHandle;
        scan.nextEntityIndex = 0;
        scan.baselineHighestIndex = highestIndex;
        scan.nextEligibleSequence = 0u;
        scan.lastAcquiredSequence = 0u;
        scan.lastAcquiredHandle = {};
        scan.lastAcquiredSpawnTime = -1.0f;
        g_propAutoReleasedHandle.store(0xFFFFFFFFu, std::memory_order_release);
        scan.slots.fill({});
        // Keep known projectile slot indices across target changes. Recycled
        // and not-yet-networked projectile slots get priority every callback.
    }
    scan.lastSimulationTime = simulationTime;
    scan.simulationTimeValid = simulationTimeValid;
    scan.controllerHandle = controllerHandle;

    PropSelection previous{};
    std::array<PropCandidate, kMaxPropCandidates> candidates{};
    {
        std::lock_guard lock(g_stateMutex);
        if (!g_state.enabled || g_state.cameraMode != CameraMode::PropSelfie) return;
        previous = {g_state.lockedPropHandle, g_state.lockedPropThrowerHandle,
            g_state.lockedPropKind, g_state.manualPropLock};
        if (!targetChanged) candidates = g_state.propCandidates;
    }
    void* lockedEntity = compat::ShouldAcceptPropOwnerCandidate(targetHandle.raw, controllerHandle.raw,
        previous.throwerHandle.raw) ? ResolveHandleToEntity(previous.handle) : nullptr;
    PropCandidate liveLocked{};
    const bool lockedLive = !timelineRewound && lockedEntity != nullptr
        && IsLockedPropProjectileStillOwnedByThrower(lockedEntity, previous.throwerHandle, liveLocked, true);
    const auto retainedView = SnapshotPropView();
    const bool sameViewShot = retainedView.handle.raw == previous.handle.raw;
    const bool pendingHandoff = previous.handle.raw == scan.lastAcquiredHandle.raw
        && retainedView.targetHandle.raw == targetHandle.raw;
    const bool hasRetainedView = !timelineRewound && retainedView.valid
        && ((sameViewShot && ((sameTarget && retainedView.targetHandle.raw == targetHandle.raw) || previous.manual))
            || (sameTarget && pendingHandoff));
    const float previousSpawnTime = lockedLive ? ReadPropSpawnTime(lockedEntity) : -1.0f;
    const auto& previousSlot = scan.slots[static_cast<std::size_t>(previous.handle.Index())];
    const auto previousSequence = previousSlot.handle.raw == previous.handle.raw ? previousSlot.eligibleSequence : 0u;

    for (auto& candidate : candidates) {
        if (!candidate.handle.IsValid()) continue;
        PropCandidate refreshed{};
        if (IsPropProjectileForTarget(ResolveHandleToEntity(candidate.handle), targetHandle, controllerHandle, refreshed)) {
            candidate = refreshed;
        } else if (candidate.valid) {
            candidate.valid = false;
            ++candidate.ageFrames;
        }
    }
    const auto remember = [&](const PropCandidate& candidate) {
        auto found = std::find_if(candidates.begin(), candidates.end(), [&](const PropCandidate& item) {
            return item.handle.raw == candidate.handle.raw;
        });
        if (found != candidates.end()) *found = candidate;
        else {
            std::move_backward(candidates.begin(), candidates.end() - 1, candidates.end());
            candidates[0] = candidate;
        }
    };
    PropCandidate available{};
    float availableSpawnTime = -1.0f;
    std::uint64_t availableSequence = 0u;
    const auto newer = [](float time, std::uint64_t sequence, float oldTime, std::uint64_t oldSequence) {
        // Lifetime order must not expire after a 100 ms wall-clock window.
        // Spawn time also prevents a late-scanned OLD projectile stealing back.
        if (time > 0.0f && oldTime > 0.0f && time != oldTime) return time > oldTime;
        return sequence > oldSequence;
    };
    const double deadline = HighResolutionSeconds() + kPropDiscoveryBudgetSeconds;
    unsigned int newClassCount = 0u;
    const auto observe = [&](int index, bool priority, bool birthEvent = false) {
        void* entity = nullptr;
        void* vtable = nullptr;
        PropClassKind type{};
        if (TryInvokeGetEntityFromIndex(entityList, index, entity) && entity != nullptr
            && TryReadMemoryValue(reinterpret_cast<void* const*>(entity), vtable) && vtable != nullptr) {
            const auto cached = scan.kindsByVtable.find(vtable);
            if (cached != scan.kindsByVtable.end()) type = cached->second;
            else {
                if (newClassCount >= (birthEvent ? 32u : 8u)) return false;
                type = ReadPropClassKind(entity);
                scan.kindsByVtable.emplace(vtable, type);
                ++newClassCount;
            }
        }
        // A base RTTI vtable can represent different grenade kinds. Resolve
        // m_designerName per entity rather than poisoning a vtable-kind cache.
        const auto kind = entity != nullptr ? ResolvePropClassKind(entity, type) : PropProjectileKind::Unknown;
        if ((!priority || birthEvent) && (kind != PropProjectileKind::Unknown || type.needsDesignerName)
            && std::find(scan.projectileSlots.begin(), scan.projectileSlots.end(), index) == scan.projectileSlots.end()) {
            if (scan.projectileSlots.size() >= 128u) scan.projectileSlots.erase(scan.projectileSlots.begin());
            scan.projectileSlots.push_back(index);
        }
        auto& slot = scan.slots[static_cast<std::size_t>(index)];
        const EntityHandle handle = kind != PropProjectileKind::Unknown ? EntityGetHandle(entity) : EntityHandle{};
        if (handle.raw != slot.handle.raw) {
            slot.freshUntilSeconds = handle.IsValid() && (birthEvent || slot.observed || index > scan.baselineHighestIndex)
                ? nowSeconds + kFreshPropInitialVelocityGraceSeconds : 0.0;
            slot.handle = handle;
            slot.eligibleSequence = 0u;
            slot.originValid = false;
        }
        slot.observed = true;
        if (!handle.IsValid() || handle.Index() != index || HasPropProjectileEnded(entity, kind)) return true;

        Vec3 origin{};
        std::string originFailure;
        const bool originValid = TryGetResolvedPropOrigin(entity, origin, originFailure);
        const Vec3 delta = Subtract(origin, slot.lastOrigin);
        const bool moved = slot.originValid && originValid && Dot(delta, delta) > 0.0001f;
        slot.lastOrigin = origin;
        slot.originValid = originValid;
        const float spawnTime = ReadPropSpawnTime(entity);
        const bool fresh = slot.freshUntilSeconds > 0.0 && nowSeconds <= slot.freshUntilSeconds;
        const bool newerSpawn = lockedLive && spawnTime > 0.0f && previousSpawnTime > 0.0f
            && spawnTime > previousSpawnTime;
        PropCandidate candidate{};
        if (!IsPropProjectileForTarget(entity, targetHandle, controllerHandle, candidate,
                fresh || newerSpawn, kind, moved)) return true;
        if (slot.eligibleSequence == 0u) slot.eligibleSequence = ++scan.nextEligibleSequence;
        remember(candidate);
        // Manual row selection remains usable; suppression only prevents
        // automatic recapture of shots that a clear/expiry already released.
        if (slot.suppressedHandle.raw == handle.raw
            || g_propAutoReleasedHandle.load(std::memory_order_acquire) == handle.raw) return true;
        if (candidate.handle.raw != previous.handle.raw
            && (newer(spawnTime, slot.eligibleSequence, scan.lastAcquiredSpawnTime, scan.lastAcquiredSequence)
                || (!previous.handle.IsValid() && candidate.handle.raw == scan.lastAcquiredHandle.raw))
            && (!available.valid
            || newer(spawnTime, slot.eligibleSequence, availableSpawnTime, availableSequence))) {
            available = candidate;
            availableSpawnTime = spawnTime;
            availableSequence = slot.eligibleSequence;
        }
        return true;
    };

    std::array<EntityHandle, 64> births{};
    const auto birthCount = DrainPropEntityBirths(births);
    for (std::size_t birth = 0; birth < birthCount; ++birth) {
        // A delayed notification must never bind the replacement serial.
        if (ResolveHandleToEntity(births[birth]) != nullptr && !observe(births[birth].Index(), true, true))
            RecordPropEntityBirth(births[birth]);
    }
    for (const int index : scan.projectileSlots) {
        if (index >= 0 && index <= highestIndex) observe(index, true);
    }
    unsigned int visited = 0u;
    const int startIndex = std::clamp(scan.nextEntityIndex, 0, highestIndex);
    int index = startIndex;
    do {
        if (!observe(index, false)) break;
        ++visited;
        index = index >= highestIndex ? 0 : index + 1;
        if ((visited & 63u) == 0u && HighResolutionSeconds() >= deadline) break;
    } while (index != startIndex);
    scan.nextEntityIndex = index;

    // Retained effect/recovery views never block automatic handoff. Explicit
    // pinning protects both a live shot and its final view until unpinned.
    PropSelection selected = lockedLive || hasRetainedView ? previous : PropSelection{};
    if (available.valid && !(previous.manual && (lockedLive || hasRetainedView))
        && (!lockedLive || newer(availableSpawnTime, availableSequence, previousSpawnTime, previousSequence))) {
        selected = {available.handle, available.throwerHandle, available.kind, false};
    }
    if (lockedLive && selected.handle.raw == previous.handle.raw) {
        selected.throwerHandle = liveLocked.throwerHandle;
        selected.kind = liveLocked.kind;
    }
    std::lock_guard lock(g_stateMutex);
    if (!g_state.enabled || g_state.cameraMode != CameraMode::PropSelfie
        || g_state.lockedPropHandle.raw != previous.handle.raw || g_state.manualPropLock != previous.manual
        || (g_state.targetMode == TargetMode::Locked && g_state.lockedHandle.raw != targetHandle.raw)
        || (g_state.targetMode == TargetMode::Follow && g_state.currentFollowHandle.IsValid()
            && g_state.currentFollowHandle.raw != targetHandle.raw)) return;
    const bool selectionChanged = selected.handle.raw != previous.handle.raw;
    const bool candidatesChanged = !PropCandidatesEqual(g_state.propCandidates, candidates);
    g_state.lockedPropHandle = selected.handle;
    g_state.lockedPropThrowerHandle = selected.throwerHandle;
    g_state.lockedPropKind = selected.kind;
    g_state.manualPropLock = selected.manual;
    g_state.propCandidates = candidates;
    if (selectionChanged) {
        if (selected.handle.IsValid()) {
            scan.lastAcquiredSequence = availableSequence;
            scan.lastAcquiredHandle = selected.handle;
            if (availableSpawnTime > 0.0f) scan.lastAcquiredSpawnTime = availableSpawnTime;
        }
        ResetPropTrackingState(selected.handle.IsValid() && sameTarget && !timelineRewound && retainedView.valid);
        g_state.propViewPhase = PropViewPhase::Waiting;
        TracePrintf("prop-chain-lock handle=%u kind=%s thrower=%u manual=%d spawn=%.3f",
            selected.handle.raw, PropProjectileKindToString(selected.kind), selected.throwerHandle.raw,
            selected.manual ? 1 : 0, selected.handle.raw == previous.handle.raw ? previousSpawnTime : availableSpawnTime);
    }
    if (selectionChanged || candidatesChanged) {
        RefreshStatusTextLocked();
        MarkOverlayDirty();
    }
}
