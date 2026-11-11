#pragma once

// Uses the actual production functions with fake entities in this EXE only.
namespace hand_checks {
class C_CSPlayerPawn {
public:
    virtual ~C_CSPlayerPawn() = default;
    unsigned char storage[0x4000]{};
};
inline void* pawnPointer{};
inline bool attachmentLive{true};
inline selfiestick::Vec3 attachment{110, 188, 46};
enum class EyeMode { NoWrite, Zero, FarAway, PartialWrite, Valid, Crouched };
inline EyeMode eyeMode{EyeMode::NoWrite};
inline int oldEyeSlotCalls{};
inline void __fastcall OldEyeSlot(void*, float*) { ++oldEyeSlotCalls; }
inline void __fastcall Eye(void*, float* output) {
    if (eyeMode == EyeMode::NoWrite) return;
    if (eyeMode == EyeMode::PartialWrite) { output[0] = 100; return; }
    const selfiestick::Vec3 value = eyeMode == EyeMode::Zero ? selfiestick::Vec3{}
        : eyeMode == EyeMode::FarAway ? selfiestick::Vec3{4096, 8192, -1024}
        : selfiestick::Vec3{100, 200, eyeMode == EyeMode::Crouched ? 46.0f : 64.0f};
    std::memcpy(output, &value, sizeof(value));
}
inline void* __fastcall EntityAtIndex(void*, int index) { return index == 42 ? pawnPointer : nullptr; }
inline void __fastcall Lookup(void*, std::uint8_t& index, const char*) { index = attachmentLive ? 0u : 0xFFu; }
inline bool __fastcall Attachment(void*, std::uint8_t, void* transform) {
    if (!attachmentLive) return false;
    std::memcpy(transform, &attachment, sizeof(attachment));
    return true;
}
template<class T> void Store(void* base, std::size_t offset, const T& value) {
    std::memcpy(static_cast<unsigned char*>(base) + offset, &value, sizeof(value));
}
inline bool Run() {
    using namespace selfiestick;
    const auto oldOffsets = g_offsets;
    const auto oldModule = g_clientModule;
    const auto oldList = g_entityListStorage;
    const auto oldGet = g_getEntityFromIndex;
    const auto oldLookup = g_lookupAttachment;
    const auto oldAttachment = g_getAttachment;
    const auto oldSuppression = g_viewModelSuppression;
    C_CSPlayerPawn pawn;
    void** originalVtable = *reinterpret_cast<void***>(&pawn);
    void* table[176]{};
    table[0] = originalVtable[-1]; // real EXE RTTI, controlled eye virtual methods
    table[166 + 1] = reinterpret_cast<void*>(&OldEyeSlot);
    table[kRenderEyeOriginVtableIndex + 1] = reinterpret_cast<void*>(&Eye);
    *reinterpret_cast<void***>(&pawn) = table + 1;
    unsigned char identity[64]{}, scene[256]{}, services[128]{};
    pawnPointer = &pawn;
    void* list = reinterpret_cast<void*>(1);
    g_clientModule = GetModuleHandleW(nullptr);
    g_entityListStorage = &list;
    g_getEntityFromIndex = EntityAtIndex;
    g_lookupAttachment = Lookup;
    g_getAttachment = Attachment;
    g_offsets.entityInstanceToIdentity = 0x10;
    g_offsets.sceneNode = 0x330;
    g_offsets.sceneAbsOrigin = 0xC8;
    g_offsets.pawnEyeAngles = 0x3600;
    g_offsets.pawnWeaponServices = 0x12F0;
    g_offsets.activeWeapon = 0x60;
    g_offsets.renderWithViewModels = 0x56A;
    Store(&pawn, 0x10, static_cast<void*>(identity));
    Store(&pawn, 0x330, static_cast<void*>(scene));
    Store(&pawn, 0x12F0, static_cast<void*>(services));
    const EntityHandle exact{(6u << 15u) | 42u};
    const EntityHandle stale{(5u << 15u) | 42u};
    Store(identity, 0x10, exact.raw);
    Store(scene, 0xC8, Vec3{100, 200, 0});
    Store(&pawn, 0x3600, Vec3{0, 0, 0});
    Store(services, 0x60, EntityHandle{});
    int stage = 1;
    const bool passed = [&]() {
        if (ResolveHandleToEntity(stale) != nullptr || ResolveHandleToEntity(exact) != &pawn) return false;
        stage = 2;
        g_viewModelSuppression = {};
        g_viewModelSuppression.weaponHandle = stale;
        g_viewModelSuppression.weaponEntity = &pawn;
        g_viewModelSuppression.previousRenderWithViewModels = true;
        g_viewModelSuppression.active = true;
        RestoreSuppressedRenderWithViewModels();
        if (reinterpret_cast<unsigned char*>(&pawn)[0x56A] != 0u) return false;

        stage = 3;
        std::string failure;
        WeaponFrameBasis live{}, noHand{}, turned{};
        attachmentLive = true;
        if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, live, failure)
            || !live.pawnHandAnchor || !live.syntheticFallbackHandAnchor
            || Length(Subtract(live.anchorOrigin, Vec3{112, 190, 46})) > 0.01f) {
            std::printf("live-hand failure: %s anchor=(%.2f,%.2f,%.2f) class=%s\n", failure.c_str(), live.anchorOrigin.x, live.anchorOrigin.y, live.anchorOrigin.z, TryGetEntityRttiClassName(&pawn).c_str());
            return false;
        }

        // A newly selected weapon has not appeared in the entity list yet.
        stage = 4;
        Store(services, 0x60, EntityHandle{(3u << 15u) | 7u});
        attachmentLive = false;
        Store(scene, 0xC8, Vec3{200, 200, 0});
        const Vec3 unrelatedCamera{-5000, 3000, 1500};
        const Vec3 unrelatedAngles{0, 90, 0};
        if (!TryBuildWeaponFrameBasis(&pawn, &unrelatedCamera, &unrelatedAngles, noHand, failure)
            || !noHand.syntheticFallbackHandAnchor
            || Length(Subtract(noHand.anchorOrigin, Add(live.anchorOrigin, Vec3{100, 0, 0}))) > 0.01f) return false;

        stage = 5;
        Store(&pawn, 0x3600, Vec3{89.99f, 180, 0});
        if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, turned, failure)
            || turned.up.z != 1.0f || turned.forward.x > -0.99f) return false;
        Vec3 expected = ComposeBasisPoint({200, 200, 64}, turned.right, turned.forward, turned.up, 10, 12, -18);
        if (Length(Subtract(turned.anchorOrigin, expected)) > 0.01f) return false;

        // Weapon kind must not change the player distance/profile on transition.
        stage = 6;
        noHand.heldItemKind = HeldItemKind::Gun;
        const auto gun = GetBaseOffsetForBasis(noHand, LookMode::SelfieRight);
        noHand.heldItemKind = HeldItemKind::KnifeOrUtility;
        const auto knife = GetBaseOffsetForBasis(noHand, LookMode::SelfieRight);
        if (gun.x != knife.x || gun.y != knife.y || gun.z != knife.z) return false;

        stage = 7;
        Store(scene, 0xC8, Vec3{100, 200, 0});
        float eye[3]{};
        eyeMode = EyeMode::NoWrite;
        if (TryEntityGetRenderEyeOrigin(&pawn, eye, failure)) return false;
        for (const EyeMode mode : {EyeMode::NoWrite, EyeMode::Zero, EyeMode::FarAway, EyeMode::PartialWrite, EyeMode::Valid, EyeMode::Crouched}) {
            eyeMode = mode;
            if (!TryGetEntityEyeOrigin(&pawn, nullptr, false, eye, failure)) return false;
            const float expectedHeight = mode == EyeMode::Crouched ? 46.0f : 64.0f;
            if (eye[0] != 100 || eye[1] != 200 || eye[2] != expectedHeight) return false;
        }
        if (oldEyeSlotCalls != 0) return false;

        stage = 8;
        eyeMode = EyeMode::NoWrite;
        Store(scene, 0xC8, Vec3{1000, 2000, 100});
        WeaponFrameBasis worldFrame{};
        if (!TryBuildWeaponFrameBasis(&pawn, nullptr, nullptr, worldFrame, failure)
            || Length(Subtract(worldFrame.upperBodyTarget, Vec3{1000, 2000, 158})) > 0.01f
            || Length(Subtract(worldFrame.anchorOrigin, Vec3{1000, 2000, 164})) > 128.0f) return false;
        Store(scene, 0xC8, Vec3{});
        if (TryGetEntityEyeOrigin(&pawn, nullptr, false, eye, failure)) return false;
        float originalPosition[3]{100, 200, 64}, originalAngles[3]{};
        if (TryWriteViewSetupPose(originalPosition, originalAngles, {0, 0, NAN}, 0, 0)
            || originalPosition[0] != 100 || originalPosition[1] != 200 || originalPosition[2] != 64) return false;
        return true;
    }();
    *reinterpret_cast<void***>(&pawn) = originalVtable;
    g_offsets = oldOffsets;
    g_clientModule = oldModule;
    g_entityListStorage = oldList;
    g_getEntityFromIndex = oldGet;
    g_lookupAttachment = oldLookup;
    g_getAttachment = oldAttachment;
    g_viewModelSuppression = oldSuppression;
    if (passed) puts("player hand/eye regression passed: handle reuse, missing weapon/hand, yaw/up, eye slot 173, unwritten/partial/zero/far eye rejection, crouch, world placement and invalid-write guard");
    else std::printf("player hand regression failed at stage %d\n", stage);
    return passed;
}
}
