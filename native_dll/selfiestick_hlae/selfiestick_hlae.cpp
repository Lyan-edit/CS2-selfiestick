#include <windows.h>
#include <commctrl.h>

#include <array>
#include <atomic>
#include <cctype>
#include <cmath>
#include <cstdarg>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <cwctype>
#include <memory>
#include <mutex>
#include <limits>
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <unordered_set>
#include <utility>
#include <vector>

#include "compatibility_gate.h"
#include "schema_probe.h"
#include "setup_view_hook.h"

#pragma comment(lib, "Comctl32.lib")

namespace selfiestick {

constexpr wchar_t kClientModuleName[] = L"client.dll";
constexpr wchar_t kSchemaModuleName[] = L"schemasystem.dll";
constexpr wchar_t kTier0ModuleName[] = L"tier0.dll";
constexpr char kClientSchemaScopeName[] = "client.dll";
constexpr wchar_t kOverlayControllerClassName[] = L"SelfiestickControllerWindow";
constexpr wchar_t kOverlayWindowClassName[] = L"SelfiestickOverlayWindow";
constexpr char kBuildTag[] = __DATE__ " " __TIME__;
constexpr char kSchemaProbeTraceVersion[] = "schema-probe-v20-left-selfie-offset-tune";

constexpr char kAnchorAttachmentName[] = "weapon_hand_r";
constexpr char kGunSideAttachmentName[] = "weapon_hand_l";
constexpr char kGunAnchorAttachmentName[] = "muzzle_flash";
constexpr char kSource2ClientVersion[] = "Source2Client002";
constexpr char kSchemaSystemVersion[] = "SchemaSystem_001";

constexpr UINT_PTR kOverlayTimerId = 1;
constexpr UINT kOverlayTimerIntervalMs = 33;
constexpr UINT kOverlayShutdownMessage = WM_APP + 1;
constexpr int kFollowRefreshFrameStage = 5;
constexpr int kOverlayWidth = 540;
constexpr int kOverlayHeight = 700;
constexpr int kOverlayMinWidth = 500;
constexpr int kOverlayMinHeight = 660;
constexpr int kOverlayMargin = 24;
constexpr int kOverlayHotkeyVirtualKey = VK_INSERT;
constexpr int kToggleEnabledHotkeyVirtualKey = VK_F8;
constexpr int kLockCurrentHotkeyVirtualKey = VK_F9;
constexpr int kFollowTargetHotkeyVirtualKey = VK_F10;
constexpr DWORD kOverlayWindowStyle = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_THICKFRAME | WS_CLIPCHILDREN | WS_CLIPSIBLINGS;
constexpr DWORD kOverlayWindowExStyle = WS_EX_CONTROLPARENT;
constexpr wchar_t kSettingsFileName[] = L"selfiestick.ini";
constexpr wchar_t kSettingsSectionHotkeys[] = L"Hotkeys";
constexpr wchar_t kSettingsKeyPanelToggleVirtualKey[] = L"PanelToggleVirtualKey";
constexpr wchar_t kSettingsSectionDebug[] = L"Debug";
constexpr wchar_t kSettingsKeyTraceEnabled[] = L"TraceEnabled";
constexpr wchar_t kSettingsKeyDisableScopedFovOverride[] = L"DisableScopedFovOverride";
constexpr wchar_t kSettingsKeyDisableGunStabilization[] = L"DisableGunStabilization";
constexpr wchar_t kTraceFileName[] = L"selfiestick_trace.log";
constexpr unsigned long long kSchemaResolveRetryIntervalMs = 500ull;
constexpr unsigned long long kSchemaResolveStartupWaitTimeoutMs = 10000ull;
constexpr unsigned long kSchemaResolveStartupWaitSleepMs = 25ul;
constexpr std::uint16_t kMaxReasonableSchemaFieldCount = 2048u;
constexpr std::size_t kMaxSchemaNameLength = 256u;
constexpr std::size_t kSchemaDiagnosticStringSlotLimit = 0x80u;
constexpr std::size_t kSchemaDiagnosticPayloadSampleLimit = 16u;

constexpr COLORREF kColorPanel = RGB(13, 17, 23);
constexpr COLORREF kColorPanelStrong = RGB(18, 25, 35);
constexpr COLORREF kColorPanelElevated = RGB(22, 29, 41);
constexpr COLORREF kColorPanelInset = RGB(9, 13, 20);
constexpr COLORREF kColorInput = RGB(10, 16, 23);
constexpr COLORREF kColorLine = RGB(39, 50, 70);
constexpr COLORREF kColorLineStrong = RGB(68, 84, 112);
constexpr COLORREF kColorText = RGB(243, 246, 251);
constexpr COLORREF kColorTextMuted = RGB(149, 163, 182);
constexpr COLORREF kColorAccent = RGB(87, 164, 255);
constexpr COLORREF kColorAccentStrong = RGB(146, 202, 255);
constexpr COLORREF kColorAccentSoft = RGB(34, 62, 95);
constexpr COLORREF kColorSuccess = RGB(98, 214, 145);
constexpr COLORREF kColorSuccessSoft = RGB(24, 56, 43);
constexpr COLORREF kColorDanger = RGB(255, 101, 124);
constexpr COLORREF kColorDangerSoft = RGB(65, 27, 37);
constexpr COLORREF kColorWarn = RGB(255, 159, 67);

constexpr char kPatternEntityList[] = "40 55 53 48 8d ac 24 ?? ?? ?? ?? 48 81 ec ?? ?? ?? ?? 48 8b 0d ?? ?? ?? ?? 33 d2 e8 ?? ?? ?? ??";
constexpr char kPatternGetEntityFromIndex[] = "4c 8d 49 10 81 fa fe 7f 00 00";
constexpr char kPatternGetSplitScreenPlayer[] = "48 83 EC ?? 83 F9 ?? 75 ?? 48 8B 0D ?? ?? ?? ?? 48 8D 54 24 ?? 48 8B 01 FF 90 ?? ?? ?? ?? 8B 08 48 63 C1 48 8D 0D ?? ?? ?? ?? 48 8B 04 C1 48 83 C4 ?? C3";
constexpr char kPatternAttachmentAccess[] = "E8 ?? ?? ?? ?? 80 BD ?? ?? ?? ?? 00 0F 84 ?? ?? ?? ?? 0F B6 95 ?? ?? ?? ?? 84 D2 0F 84 ?? ?? ?? ?? 4C 8D 45 ?? 48 8B CF E8 ?? ?? ?? ??";
constexpr char kPatternSchemaSystem[] = "48 89 05 ?? ?? ?? ?? 4C 8D 0D ?? ?? ?? ?? 33 C0 48 C7 05";
constexpr char kPatternSetupViewPatch[] = "BA FF FF FF FF 48 8D 0D ?? ?? ?? ?? E8 ?? ?? ?? ?? 48 85 C0 75 0B 48 8B 05 ?? ?? ?? ?? 48 8B 40 08 4C 8D AE A0 04 00 00 48 8D 9E B8 04 00 00 44 38 20 75 2F";
constexpr char kPatternSchemaQualifiedClassLookup[] = "48 89 5C 24 08 48 89 6C 24 10 48 89 74 24 18 57 48 83 EC 20 48 8B DA 48 8B F1 BA 21 00 00 00 49 8B C8 49 8B F8 E8 ?? ?? ?? ?? 48 8B E8 48 8B CE 48 8B 06 48 85 ED 75 15 FF 50 58 4C 8B C7 48 8B D3 48 8B C8 4C 8B 08 41 FF 51 18 EB 23 45 33 C0 48 8B D7 FF 50 68 48 8B C8 48 85 C0 75 05 48 89 03 EB 0D 48 8B 00 4C 8D 45 01 48 8B D3 FF 50 18";

constexpr std::uint32_t kInvalidHandleValue = 0xFFFFFFFFu;
constexpr int kEntityHandleIndexMask = 0x7FFF;

constexpr std::ptrdiff_t kViewSetupFovOffset = 0x498;
constexpr std::ptrdiff_t kViewSetupOriginOffset = 0x4A0;
constexpr std::ptrdiff_t kViewSetupAnglesOffset = 0x4B8;

constexpr float kUpperBodyTargetBlendFactor = 0.30f;
constexpr float kGunAnchorStabilizationFactor = 0.18f;
constexpr float kScopedFovThreshold = 60.0f;
constexpr float kFallbackUnscopedFov = 90.0f;

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

enum class HeldItemKind {
    Unknown,
    KnifeOrUtility,
    Gun
};

struct RttiCompleteObjectLocator {
    std::uint32_t signature{};
    std::uint32_t offset{};
    std::uint32_t constructorDisplacementOffset{};
    std::int32_t typeDescriptorOffset{};
    std::int32_t classDescriptorOffset{};
    std::int32_t selfOffset{};
};

struct RttiTypeDescriptor {
    void* vtable{};
    void* spare{};
    char name[1];
};

enum class SupportedWeaponId {
    None,
    AK47,
    M4A4,
    M4A1S,
    AWP
};

struct WeaponAttachmentSpec {
    SupportedWeaponId id{SupportedWeaponId::None};
    const char* rttiClassName{};
    const char* debugName{};
    const char* barrelAttachmentName{};
};

constexpr WeaponAttachmentSpec kSupportedWeaponSpecs[] = {
    { SupportedWeaponId::AK47, "C_AK47", "weapon_ak47", kAnchorAttachmentName },
    { SupportedWeaponId::M4A4, "C_WeaponM4A1", "weapon_m4a1", kAnchorAttachmentName },
    { SupportedWeaponId::M4A1S, "C_WeaponM4A1Silencer", "weapon_m4a1_silencer", kAnchorAttachmentName },
    { SupportedWeaponId::AWP, "C_WeaponAWP", "weapon_awp", kAnchorAttachmentName }
};

constexpr Vec3 kCombatBaseOffset{ 8.0f, 18.0f, 6.0f };
constexpr Vec3 kKnifeFallbackHandAnchorOffset{ 10.0f, 12.0f, -18.0f };
constexpr Vec3 kSyntheticFallbackSelfieBaseOffset{ 6.0f, 12.0f, 10.0f };
constexpr Vec3 kGunSelfieBaseOffset{ 10.5f, 12.5f, 6.75f };
constexpr Vec3 kGunForwardBaseOffset{ 6.5f, 16.25f, -2.5f };
constexpr Vec3 kWorldUp{ 0.0f, 0.0f, 1.0f };
constexpr float kGunUpperBodyTargetBlendFactor = 0.34f;
constexpr float kGunSelfieTargetCrossBias = 3.75f;
constexpr float kGunSelfieTargetUpBias = 0.15f;
constexpr float kFallbackEyeOriginHeight = 64.0f;

struct EntityHandle {
    std::uint32_t raw{kInvalidHandleValue};

    bool IsValid() const {
        return raw != kInvalidHandleValue;
    }

    int Index() const {
        return static_cast<int>(raw & kEntityHandleIndexMask);
    }
};

struct WeaponFrameBasis {
    Vec3 anchorOrigin{};
    Vec3 upperBodyTarget{};
    Vec3 right{};
    Vec3 forward{};
    Vec3 up{};
    HeldItemKind heldItemKind{HeldItemKind::Unknown};
    EntityHandle weaponHandle{};
    SupportedWeaponId supportedWeaponId{SupportedWeaponId::None};
    bool syntheticFallbackHandAnchor{false};
};

enum class LookMode {
    SelfieLeft,
    SelfieRight,
    Forward
};

enum class TargetMode {
    Follow,
    Locked
};

#if defined(SELFIESTICK_LANG_ZH_CN) && defined(SELFIESTICK_LANG_EN_US)
#error selfiestick language macros are mutually exclusive
#endif

struct LocalizedText {
    const char* lookSelfieLeft;
    const char* lookSelfieRight;
    const char* lookForward;
    const char* targetFollow;
    const char* targetLocked;
    const char* valueNone;
    const char* reasonClientModuleMissing;
    const char* reasonSchemaModuleMissing;
    const char* reasonCreateInterfaceMissing;
    const char* reasonSource2ClientUnavailable;
    const char* reasonSchemaPatternMissing;
    const char* reasonSchemaSystemUnavailable;
    const char* reasonRequiredSchemaOffsetsMissing;
    const char* reasonEntityListPatternMissing;
    const char* reasonGetEntityFromIndexPatternMissing;
    const char* reasonGetSplitScreenPlayerPatternMissing;
    const char* reasonAttachmentAccessPatternMissing;
    const char* reasonSource2ClientInterfaceMissing;
    const char* reasonSource2ClientVtableMissing;
    const char* reasonFrameStageNotifyMissing;
    const char* reasonFrameStageNotifyHookFailed;
    const char* reasonSetUpViewPatchSiteMissing;
    const char* reasonSetUpViewDetourAllocationFailed;
    const char* reasonSetUpViewPatchFailed;
    const char* reasonOverlayWindowClassRegistrationFailed;
    const char* reasonOverlayControllerCreationFailed;
    const char* reasonOverlayUiThreadCreationFailed;
    const char* reasonWeaponEntityMissing;
    const char* reasonWeaponSceneNodeMissing;
    const char* reasonWeaponOriginInvalid;
    const char* reasonSplitScreenPlayerAccessUnavailable;
    const char* reasonSplitScreenPlayerMissing;
    const char* reasonSplitScreenEntityWrongType;
    const char* reasonObserverPawnMissing;
    const char* reasonNoObserverTarget;
    const char* reasonObserverTargetUnresolved;
    const char* reasonLockedTargetMissing;
    const char* reasonLockedTargetUnresolved;
    const char* reasonFollowTargetMissing;
    const char* reasonFollowTargetUnresolved;
    const char* reasonTargetEntityMissing;
    const char* reasonAttachmentAccessUnavailable;
    const char* reasonInvalidEyeOrigin;
    const char* reasonUpperBodyTargetInvalid;
    const char* reasonCombatRightAxisDegenerate;
    const char* reasonCombatForwardAxisDegenerate;
    const char* reasonCombatUpAxisDegenerate;
    const char* reasonCameraDirectionDegenerate;
    const char* reasonAttachmentMissingFormat;
    const char* logEnabled;
    const char* logLook;
    const char* logTargetMode;
    const char* logTargetCleared;
    const char* logBlocked;
    const char* logLive;
    const char* logSplitScreenMatches;
    const char* logSelectedCandidate;
    const char* logSetUpViewActive;
    const char* logEnabledCallbackEntered;
    const char* logSetUpViewHookInstalled;
    const char* logLockedTarget;
    const char* logOverlayHostMissing;
    const char* logOverlayResourcesUnavailable;
    const char* logOverlayWindowCreationFailed;
    const char* logOverlayEnableClicked;
    const char* logToggleViaF8;
    const char* logTargetFollowViaF10;
    const char* logOverlayHotkeyChanged;
    const char* logOverlayClassRegistrationFailed;
    const char* logOverlayControllerCreationFailed;
    const char* logOverlayUiThreadCreationFailed;
    const char* logInterfaceInitializationFailed;
    const char* logSchemaInitializationFailed;
    const char* logEntityInitializationFailed;
    const char* logHookInstallationFailed;
    const char* logBuild;
    const char* logReady;
    const wchar_t* uiNone;
    const wchar_t* uiEnabledLabel;
    const wchar_t* uiLookLabel;
    const wchar_t* uiTargetModeLabel;
    const wchar_t* uiLockedHandleLabel;
    const wchar_t* uiCurrentTargetLabel;
    const wchar_t* uiAnchorAttachLabel;
    const wchar_t* uiTrimLabel;
    const wchar_t* uiLastOverrideLabel;
    const wchar_t* uiReasonLabel;
    const wchar_t* uiStatusOn;
    const wchar_t* uiStatusOff;
    const wchar_t* uiStatusReady;
    const wchar_t* uiStatusMissing;
    const wchar_t* uiStatusSuccess;
    const wchar_t* uiStatusFailed;
    const wchar_t* uiReasonEmpty;
    const wchar_t* uiButtonEnableOn;
    const wchar_t* uiButtonEnableOff;
    const wchar_t* uiButtonLookSelfieLeft;
    const wchar_t* uiButtonLookSelfieRight;
    const wchar_t* uiButtonLookForward;
    const wchar_t* uiButtonTargetFollow;
    const wchar_t* uiButtonTargetLockCurrent;
    const wchar_t* uiButtonTargetClear;
    const wchar_t* uiButtonPanelHotkey;
    const wchar_t* uiButtonPanelHotkeyCapture;
    const wchar_t* uiLifecycleBoot;
    const wchar_t* uiLifecycleIdle;
    const wchar_t* uiLifecycleLive;
    const wchar_t* uiLifecycleBlocked;
    const wchar_t* uiTitle;
    const wchar_t* uiSubtitle;
    const wchar_t* uiHotkeyChip;
    const wchar_t* uiPanelHotkeyLabel;
    const wchar_t* uiEnableSection;
    const wchar_t* uiEnableChipOn;
    const wchar_t* uiEnableChipOff;
    const wchar_t* uiOffsetSection;
    const wchar_t* uiOffsetHint;
    const wchar_t* uiDirectionSection;
    const wchar_t* uiDirectionChipSelfieLeft;
    const wchar_t* uiDirectionChipSelfieRight;
    const wchar_t* uiDirectionChipForward;
    const wchar_t* uiTargetSection;
    const wchar_t* uiTargetChipLocked;
    const wchar_t* uiTargetChipFollow;
    const wchar_t* uiRuntimeSection;
    const wchar_t* uiRuntimeSubsection;
    const wchar_t* uiFontName;
    const wchar_t* uiMonoFontName;
};

constexpr LocalizedText kLocalizedTextEnUs{
    .lookSelfieLeft = "selfie left",
    .lookSelfieRight = "selfie right",
    .lookForward = "forward",
    .targetFollow = "follow",
    .targetLocked = "locked",
    .valueNone = "none",
    .reasonClientModuleMissing = "client.dll missing",
    .reasonSchemaModuleMissing = "schemasystem.dll missing",
    .reasonCreateInterfaceMissing = "CreateInterface missing",
    .reasonSource2ClientUnavailable = "Source2Client002 unavailable",
    .reasonSchemaPatternMissing = "schema pattern missing",
    .reasonSchemaSystemUnavailable = "schema system unavailable",
    .reasonRequiredSchemaOffsetsMissing = "required schema offsets missing",
    .reasonEntityListPatternMissing = "entity list pattern missing",
    .reasonGetEntityFromIndexPatternMissing = "GetEntityFromIndex pattern missing",
    .reasonGetSplitScreenPlayerPatternMissing = "GetSplitScreenPlayer pattern missing",
    .reasonAttachmentAccessPatternMissing = "attachment access pattern missing",
    .reasonSource2ClientInterfaceMissing = "Source2Client interface missing",
    .reasonSource2ClientVtableMissing = "Source2Client vtable missing",
    .reasonFrameStageNotifyMissing = "FrameStageNotify missing",
    .reasonFrameStageNotifyHookFailed = "FrameStageNotify hook failed",
    .reasonSetUpViewPatchSiteMissing = "SetUpView patch site missing",
    .reasonSetUpViewDetourAllocationFailed = "SetUpView detour allocation failed",
    .reasonSetUpViewPatchFailed = "SetUpView patch failed",
    .reasonOverlayWindowClassRegistrationFailed = "overlay window class registration failed",
    .reasonOverlayControllerCreationFailed = "overlay controller window creation failed",
    .reasonOverlayUiThreadCreationFailed = "overlay ui thread creation failed",
    .reasonWeaponEntityMissing = "weapon entity missing",
    .reasonWeaponSceneNodeMissing = "weapon scene node missing",
    .reasonWeaponOriginInvalid = "weapon origin invalid",
    .reasonSplitScreenPlayerAccessUnavailable = "split-screen player access unavailable",
    .reasonSplitScreenPlayerMissing = "split-screen player missing",
    .reasonSplitScreenEntityWrongType = "split-screen entity is neither controller nor pawn",
    .reasonObserverPawnMissing = "observer pawn missing",
    .reasonNoObserverTarget = "no observer target",
    .reasonObserverTargetUnresolved = "observer target unresolved",
    .reasonLockedTargetMissing = "locked target missing",
    .reasonLockedTargetUnresolved = "locked target unresolved",
    .reasonFollowTargetMissing = "follow target missing",
    .reasonFollowTargetUnresolved = "follow target unresolved",
    .reasonTargetEntityMissing = "target entity missing",
    .reasonAttachmentAccessUnavailable = "attachment access unavailable",
    .reasonInvalidEyeOrigin = "invalid eye origin",
    .reasonUpperBodyTargetInvalid = "upper body target invalid",
    .reasonCombatRightAxisDegenerate = "combat right axis degenerate",
    .reasonCombatForwardAxisDegenerate = "combat forward axis degenerate",
    .reasonCombatUpAxisDegenerate = "combat up axis degenerate",
    .reasonCameraDirectionDegenerate = "camera direction degenerate",
    .reasonAttachmentMissingFormat = "%s missing",
    .logEnabled = "[selfiestick] enabled=%d\n",
    .logLook = "[selfiestick] look=%s\n",
    .logTargetMode = "[selfiestick] target mode=%s\n",
    .logTargetCleared = "[selfiestick] target cleared\n",
    .logBlocked = "[selfiestick] blocked: %s\n",
    .logLive = "[selfiestick] live: target=%u attachment=%s\n",
    .logSplitScreenMatches = "[selfiestick] split-screen player pattern matches=%zu\n",
    .logSelectedCandidate = "[selfiestick] selected split-screen player candidate %zu\n",
    .logSetUpViewActive = "[selfiestick] SetUpView callback active\n",
    .logEnabledCallbackEntered = "[selfiestick] enabled callback entered: targetMode=%s follow=%u locked=%u\n",
    .logSetUpViewHookInstalled = "[selfiestick] SetUpView continuation hook installed at 0x%llX\n",
    .logLockedTarget = "[selfiestick] locked target %u\n",
    .logOverlayHostMissing = "[selfiestick] overlay host window missing\n",
    .logOverlayResourcesUnavailable = "[selfiestick] overlay resources unavailable\n",
    .logOverlayWindowCreationFailed = "[selfiestick] overlay window creation failed: %lu\n",
    .logOverlayEnableClicked = "[selfiestick] overlay enable button clicked\n",
    .logToggleViaF8 = "[selfiestick] toggle via F8\n",
    .logTargetFollowViaF10 = "[selfiestick] target follow via F10\n",
    .logOverlayHotkeyChanged = "[selfiestick] panel hotkey=%s\n",
    .logOverlayClassRegistrationFailed = "[selfiestick] overlay class registration failed\n",
    .logOverlayControllerCreationFailed = "[selfiestick] overlay controller creation failed: %lu\n",
    .logOverlayUiThreadCreationFailed = "[selfiestick] overlay ui thread creation failed\n",
    .logInterfaceInitializationFailed = "[selfiestick] interface initialization failed: %s\n",
    .logSchemaInitializationFailed = "[selfiestick] schema initialization failed: %s\n",
    .logEntityInitializationFailed = "[selfiestick] entity initialization failed: %s\n",
    .logHookInstallationFailed = "[selfiestick] hook installation failed: %s\n",
    .logBuild = "[selfiestick] build %s\n",
    .logReady = "[selfiestick] ready - press Ins for panel, F8 toggle, F9 lock, F10 follow\n",
    .uiNone = L"none",
    .uiEnabledLabel = L"Enabled: ",
    .uiLookLabel = L"Look: ",
    .uiTargetModeLabel = L"Target mode: ",
    .uiLockedHandleLabel = L"Locked handle: ",
    .uiCurrentTargetLabel = L"Current target: ",
    .uiAnchorAttachLabel = L"Anchor attach: ",
    .uiTrimLabel = L"Trim R/B/U: ",
    .uiLastOverrideLabel = L"Last override: ",
    .uiReasonLabel = L"Reason: ",
    .uiStatusOn = L"on",
    .uiStatusOff = L"off",
    .uiStatusReady = L"ready",
    .uiStatusMissing = L"missing",
    .uiStatusSuccess = L"success",
    .uiStatusFailed = L"failed",
    .uiReasonEmpty = L"-",
    .uiButtonEnableOn = L"SELFIESTICK ON",
    .uiButtonEnableOff = L"SELFIESTICK OFF",
    .uiButtonLookSelfieLeft = L"SELFIE LEFT",
    .uiButtonLookSelfieRight = L"SELFIE RIGHT",
    .uiButtonLookForward = L"FORWARD",
    .uiButtonTargetFollow = L"FOLLOW",
    .uiButtonTargetLockCurrent = L"LOCK CURRENT",
    .uiButtonTargetClear = L"CLEAR",
    .uiButtonPanelHotkey = L"CHANGE PANEL KEY",
    .uiButtonPanelHotkeyCapture = L"PRESS A KEY...",
    .uiLifecycleBoot = L"BOOT",
    .uiLifecycleIdle = L"IDLE",
    .uiLifecycleLive = L"LIVE",
    .uiLifecycleBlocked = L"BLOCKED",
    .uiTitle = L"SELFIESTICK",
    .uiSubtitle = L"HAND FRAME / LOCAL TRIM",
    .uiHotkeyChip = L"PANEL",
    .uiPanelHotkeyLabel = L"Panel key: ",
    .uiEnableSection = L"ENABLE",
    .uiEnableChipOn = L"ON",
    .uiEnableChipOff = L"OFF",
    .uiOffsetSection = L"LOCAL TRIM",
    .uiOffsetHint = L"ANCHOR + BASE FRAME + R / B / U",
    .uiDirectionSection = L"DIRECTION",
    .uiDirectionChipSelfieLeft = L"LEFT",
    .uiDirectionChipSelfieRight = L"RIGHT",
    .uiDirectionChipForward = L"FORWARD",
    .uiTargetSection = L"TARGET",
    .uiTargetChipLocked = L"LOCKED",
    .uiTargetChipFollow = L"FOLLOW",
    .uiRuntimeSection = L"RUNTIME STATUS",
    .uiRuntimeSubsection = L"RAW STATE | NO FALLBACK",
    .uiFontName = L"Segoe UI",
    .uiMonoFontName = L"Consolas"
};

constexpr LocalizedText kLocalizedTextZhCn{
    .lookSelfieLeft = "左侧自拍",
    .lookSelfieRight = "右侧自拍",
    .lookForward = "前视",
    .targetFollow = "跟随",
    .targetLocked = "锁定",
    .valueNone = "无",
    .reasonClientModuleMissing = "缺少 client.dll",
    .reasonSchemaModuleMissing = "缺少 schemasystem.dll",
    .reasonCreateInterfaceMissing = "缺少 CreateInterface",
    .reasonSource2ClientUnavailable = "无法获取 Source2Client002",
    .reasonSchemaPatternMissing = "未找到 schema 特征码",
    .reasonSchemaSystemUnavailable = "schema system 不可用",
    .reasonRequiredSchemaOffsetsMissing = "缺少必需的 schema 偏移",
    .reasonEntityListPatternMissing = "未找到 entity list 特征码",
    .reasonGetEntityFromIndexPatternMissing = "未找到 GetEntityFromIndex 特征码",
    .reasonGetSplitScreenPlayerPatternMissing = "未找到 GetSplitScreenPlayer 特征码",
    .reasonAttachmentAccessPatternMissing = "未找到 attachment access 特征码",
    .reasonSource2ClientInterfaceMissing = "Source2Client 接口缺失",
    .reasonSource2ClientVtableMissing = "Source2Client vtable 缺失",
    .reasonFrameStageNotifyMissing = "缺少 FrameStageNotify",
    .reasonFrameStageNotifyHookFailed = "FrameStageNotify hook 失败",
    .reasonSetUpViewPatchSiteMissing = "未找到 SetUpView 补丁位点",
    .reasonSetUpViewDetourAllocationFailed = "SetUpView detour 分配失败",
    .reasonSetUpViewPatchFailed = "SetUpView 补丁失败",
    .reasonOverlayWindowClassRegistrationFailed = "覆盖面板窗口类注册失败",
    .reasonOverlayControllerCreationFailed = "覆盖面板控制窗口创建失败",
    .reasonOverlayUiThreadCreationFailed = "覆盖面板 UI 线程创建失败",
    .reasonWeaponEntityMissing = "武器实体缺失",
    .reasonWeaponSceneNodeMissing = "武器 scene node 缺失",
    .reasonWeaponOriginInvalid = "武器原点无效",
    .reasonSplitScreenPlayerAccessUnavailable = "split-screen player 访问不可用",
    .reasonSplitScreenPlayerMissing = "缺少 split-screen player",
    .reasonSplitScreenEntityWrongType = "split-screen 实体既不是 controller 也不是 pawn",
    .reasonObserverPawnMissing = "缺少 observer pawn",
    .reasonNoObserverTarget = "没有观战目标",
    .reasonObserverTargetUnresolved = "观战目标解析失败",
    .reasonLockedTargetMissing = "缺少锁定目标",
    .reasonLockedTargetUnresolved = "锁定目标解析失败",
    .reasonFollowTargetMissing = "缺少跟随目标",
    .reasonFollowTargetUnresolved = "跟随目标解析失败",
    .reasonTargetEntityMissing = "目标实体缺失",
    .reasonAttachmentAccessUnavailable = "attachment 访问不可用",
    .reasonInvalidEyeOrigin = "眼位原点无效",
    .reasonUpperBodyTargetInvalid = "上半身构图目标无效",
    .reasonCombatRightAxisDegenerate = "战斗右轴退化",
    .reasonCombatForwardAxisDegenerate = "战斗前轴退化",
    .reasonCombatUpAxisDegenerate = "战斗上轴退化",
    .reasonCameraDirectionDegenerate = "相机方向退化",
    .reasonAttachmentMissingFormat = "attachment 缺失：%s",
    .logEnabled = "[selfiestick] 启用=%d\n",
    .logLook = "[selfiestick] 朝向=%s\n",
    .logTargetMode = "[selfiestick] 目标模式=%s\n",
    .logTargetCleared = "[selfiestick] 已清除目标锁定\n",
    .logBlocked = "[selfiestick] 已阻塞：%s\n",
    .logLive = "[selfiestick] 生效中：target=%u attachment=%s\n",
    .logSplitScreenMatches = "[selfiestick] split-screen player 特征匹配数=%zu\n",
    .logSelectedCandidate = "[selfiestick] 已选择 split-screen player candidate %zu\n",
    .logSetUpViewActive = "[selfiestick] SetUpView 回调已激活\n",
    .logEnabledCallbackEntered = "[selfiestick] 启用后进入回调：targetMode=%s follow=%u locked=%u\n",
    .logSetUpViewHookInstalled = "[selfiestick] SetUpView continuation hook 已安装，偏移 0x%llX\n",
    .logLockedTarget = "[selfiestick] 已锁定目标 %u\n",
    .logOverlayHostMissing = "[selfiestick] 缺少覆盖面板宿主窗口\n",
    .logOverlayResourcesUnavailable = "[selfiestick] 覆盖面板资源不可用\n",
    .logOverlayWindowCreationFailed = "[selfiestick] 覆盖面板窗口创建失败：%lu\n",
    .logOverlayEnableClicked = "[selfiestick] 已点击覆盖面板启用按钮\n",
    .logToggleViaF8 = "[selfiestick] 已通过 F8 切换启用\n",
    .logTargetFollowViaF10 = "[selfiestick] 已通过 F10 切回跟随\n",
    .logOverlayHotkeyChanged = "[selfiestick] 面板按键=%s\n",
    .logOverlayClassRegistrationFailed = "[selfiestick] 覆盖面板窗口类注册失败\n",
    .logOverlayControllerCreationFailed = "[selfiestick] 覆盖面板控制窗口创建失败：%lu\n",
    .logOverlayUiThreadCreationFailed = "[selfiestick] 覆盖面板 UI 线程创建失败\n",
    .logInterfaceInitializationFailed = "[selfiestick] 接口初始化失败：%s\n",
    .logSchemaInitializationFailed = "[selfiestick] schema 初始化失败：%s\n",
    .logEntityInitializationFailed = "[selfiestick] entity 初始化失败：%s\n",
    .logHookInstallationFailed = "[selfiestick] hook 安装失败：%s\n",
    .logBuild = "[selfiestick] 构建版本 %s\n",
    .logReady = "[selfiestick] 已就绪 - 按 Ins 打开面板，F8 开关，F9 锁定，F10 跟随\n",
    .uiNone = L"无",
    .uiEnabledLabel = L"启用: ",
    .uiLookLabel = L"朝向: ",
    .uiTargetModeLabel = L"目标模式: ",
    .uiLockedHandleLabel = L"锁定句柄: ",
    .uiCurrentTargetLabel = L"当前目标: ",
    .uiAnchorAttachLabel = L"锚点 attachment: ",
    .uiTrimLabel = L"微调 R/B/U: ",
    .uiLastOverrideLabel = L"最近覆写: ",
    .uiReasonLabel = L"原因: ",
    .uiStatusOn = L"开启",
    .uiStatusOff = L"关闭",
    .uiStatusReady = L"可用",
    .uiStatusMissing = L"缺失",
    .uiStatusSuccess = L"成功",
    .uiStatusFailed = L"失败",
    .uiReasonEmpty = L"-",
    .uiButtonEnableOn = L"自拍杆 已开启",
    .uiButtonEnableOff = L"自拍杆 已关闭",
    .uiButtonLookSelfieLeft = L"左侧自拍",
    .uiButtonLookSelfieRight = L"右侧自拍",
    .uiButtonLookForward = L"向前看",
    .uiButtonTargetFollow = L"跟随",
    .uiButtonTargetLockCurrent = L"锁定当前",
    .uiButtonTargetClear = L"清除锁定",
    .uiButtonPanelHotkey = L"修改面板按键",
    .uiButtonPanelHotkeyCapture = L"请按一个键...",
    .uiLifecycleBoot = L"启动中",
    .uiLifecycleIdle = L"待命",
    .uiLifecycleLive = L"生效",
    .uiLifecycleBlocked = L"阻塞",
    .uiTitle = L"自拍杆",
    .uiSubtitle = L"右手锚点 / 局部微调",
    .uiHotkeyChip = L"面板",
    .uiPanelHotkeyLabel = L"面板按键: ",
    .uiEnableSection = L"启用",
    .uiEnableChipOn = L"开",
    .uiEnableChipOff = L"关",
    .uiOffsetSection = L"局部微调",
    .uiOffsetHint = L"锚点 + 基准构图 + R / B / U",
    .uiDirectionSection = L"朝向",
    .uiDirectionChipSelfieLeft = L"左侧",
    .uiDirectionChipSelfieRight = L"右侧",
    .uiDirectionChipForward = L"前视",
    .uiTargetSection = L"目标",
    .uiTargetChipLocked = L"锁定",
    .uiTargetChipFollow = L"跟随",
    .uiRuntimeSection = L"运行状态",
    .uiRuntimeSubsection = L"真实状态 | 无兜底",
    .uiFontName = L"Microsoft YaHei UI",
    .uiMonoFontName = L"Microsoft YaHei UI"
};

const LocalizedText& Texts() {
#if defined(SELFIESTICK_LANG_ZH_CN)
    return kLocalizedTextZhCn;
#else
    return kLocalizedTextEnUs;
#endif
}

struct ClientOffsets {
    std::ptrdiff_t entityInstanceToIdentity{};
    std::ptrdiff_t controllerPawnHandle{};
    std::ptrdiff_t pawnObserverServices{};
    std::ptrdiff_t pawnCameraServices{};
    std::ptrdiff_t pawnWeaponServices{};
    std::ptrdiff_t pawnControllerHandle{};
    std::ptrdiff_t pawnEyeAngles{};
    std::ptrdiff_t observerMode{};
    std::ptrdiff_t observerTarget{};
    std::ptrdiff_t cameraViewEntity{};
    std::ptrdiff_t activeWeapon{};
    std::ptrdiff_t sceneNode{};
    std::ptrdiff_t sceneAbsOrigin{};
    std::ptrdiff_t renderWithViewModels{};
};

struct SchemaField {
    const char* name;
    void* type;
    std::uint32_t offset;
    std::uint32_t metadataSize;
    void* metadata;
};

template <typename T>
struct SchemaArray {
    T* elements;
    std::uint32_t count;
    std::uint32_t reserved;
};

struct SchemaFieldModern {
    void* type;
    const char* name;
    std::uint32_t offset;
    std::byte padding0[0xC];
};

struct SchemaClassInfoModern {
    std::byte nameStorage[0x18];
    std::uint32_t sizeOf;
    std::uint32_t alignOf;
    SchemaArray<SchemaFieldModern> fields;
};

struct SchemaClassInfoLegacy {
    std::byte padding0[0x1C];
    std::uint16_t fieldCount;
    std::byte padding1[0xA];
    SchemaFieldModern* fields;
};

struct RuntimeState {
    bool initialized{false};
    bool enabled{false};
    LookMode lookMode{LookMode::SelfieRight};
    TargetMode targetMode{TargetMode::Follow};
    int overlayHotkeyVirtualKey{kOverlayHotkeyVirtualKey};
    Vec3 offset{0.0f, 0.0f, 0.0f};
    EntityHandle lockedHandle{};
    EntityHandle currentFollowHandle{};
    EntityHandle currentTargetHandle{};
    bool attachmentAvailable{false};
    bool lastOverrideSucceeded{false};
    bool setUpViewHooked{false};
    bool frameStageHooked{false};
    int lastFrameStage{-1};
    std::string attachmentName{kAnchorAttachmentName};
    std::string lastFailureReason{"dll loaded, waiting for initialization"};
    std::string lastStatusText{};
};

struct OverrideConfig {
    bool enabled{false};
    LookMode lookMode{LookMode::SelfieRight};
    TargetMode targetMode{TargetMode::Follow};
    Vec3 offset{};
    EntityHandle lockedHandle{};
    EntityHandle followHandle{};
};

struct GunBasisContinuityState {
    EntityHandle targetHandle{};
    Vec3 right{};
    bool valid{false};
};

struct GunAnchorStabilityState {
    EntityHandle targetHandle{};
    EntityHandle weaponHandle{};
    float rightDistance{};
    float forwardDistance{};
    float upDistance{};
    bool valid{false};
};

struct ScopedFovOverrideState {
    EntityHandle targetHandle{};
    EntityHandle weaponHandle{};
    float unzoomedFov{kFallbackUnscopedFov};
    bool valid{false};
};

struct ViewModelSuppressionState {
    EntityHandle weaponHandle{};
    void* weaponEntity{};
    bool previousRenderWithViewModels{};
    bool active{false};
};

enum OverlayControlId : int {
    kControlButtonEnable = 1001,
    kControlEditOffsetX = 1002,
    kControlEditOffsetY = 1003,
    kControlEditOffsetZ = 1004,
    kControlButtonLookSelfieLeft = 1005,
    kControlButtonLookSelfieRight = 1006,
    kControlButtonLookForward = 1007,
    kControlButtonTargetFollow = 1008,
    kControlButtonTargetLock = 1009,
    kControlButtonTargetClear = 1010,
    kControlEditStatus = 1011,
    kControlButtonPanelHotkey = 1012
};

struct OverlayUiState {
    HWND controllerWindow{};
    HWND hostWindow{};
    HWND overlayWindow{};
    HWND enableButton{};
    HWND offsetXEdit{};
    HWND offsetYEdit{};
    HWND offsetZEdit{};
    HWND lookSelfieLeftButton{};
    HWND lookSelfieRightButton{};
    HWND lookForwardButton{};
    HWND targetFollowButton{};
    HWND targetLockButton{};
    HWND targetClearButton{};
    HWND overlayHotkeyButton{};
    HWND statusEdit{};
    HFONT titleFont{};
    HFONT bodyFont{};
    HFONT monoFont{};
    HBRUSH panelBrush{};
    HBRUSH editBrush{};
    bool overlayCreated{false};
    bool overlayVisible{false};
    bool suppressOffsetSync{false};
    bool captureOverlayHotkey{false};
    bool overlayHotkeyWasDown{false};
    bool toggleHotkeyWasDown{false};
    bool lockHotkeyWasDown{false};
    bool followHotkeyWasDown{false};
    bool overlayPlacementInitialized{false};
    bool hasLastHostRect{false};
    RECT lastHostRect{};
    bool hasLastRenderedState{false};
    RuntimeState lastRenderedState{};
    std::wstring lastRenderedStatusText{};
    std::wstring lastRenderedHotkeyChipText{};
    std::atomic<bool> dirty{true};
};

struct DebugOptions {
    bool traceEnabled{true};
    bool disableScopedFovOverride{false};
    bool disableGunStabilization{false};
};

using CreateInterfaceFn = void* (*)(const char* name, int* returnCode);
using GetHighestEntityIndexFn = int(__fastcall*)(void* entityList, bool includeUnknown);
using GetEntityFromIndexFn = void* (__fastcall*)(void* entityList, int index);
using LookupAttachmentFn = void(__fastcall*)(void* entity, std::uint8_t& outIndex, const char* attachmentName);
using GetAttachmentFn = bool(__fastcall*)(void* entity, std::uint8_t attachmentIndex, void* outputTransform);
using GetSplitScreenPlayerFn = void* (__fastcall*)(int slot);
using FrameStageNotifyFn = void(__fastcall*)(void* thisPointer, int stage);
using Tier0MsgFn = void(__cdecl*)(const char*, ...);
using FindTypeScopeForModuleFn = void* (__fastcall*)(void* schemaSystem, const char* moduleName, void* unused);
using FindDeclaredClassInTypeScopeFn = void* (__fastcall*)(void* typeScope, const char* className);
using FindDeclaredClassOutParamFn = void(__fastcall*)(void* typeScope, void** outClassInfo, const char* className);
using FindDeclaredClassByQualifiedNameFn = void* (__fastcall*)(void* schemaSystem, void** outClassInfo, const char* qualifiedClassName);

HMODULE g_module{};
HMODULE g_clientModule{};
HMODULE g_schemaModule{};
HMODULE g_tier0Module{};

void* g_source2Client{};
void** g_entityListStorage{};
GetHighestEntityIndexFn g_getHighestEntityIndex{};
GetEntityFromIndexFn g_getEntityFromIndex{};
LookupAttachmentFn g_lookupAttachment{};
GetAttachmentFn g_getAttachment{};
GetSplitScreenPlayerFn g_getSplitScreenPlayer{};
std::vector<GetSplitScreenPlayerFn> g_getSplitScreenPlayerCandidates{};
FrameStageNotifyFn g_originalFrameStageNotify{};
void* g_setupViewPatchAddress{};
void* g_setupViewDetourStub{};
std::array<std::uint8_t, hook::kSetUpViewPatchSize> g_setupViewOriginalBytes{};
Tier0MsgFn g_tier0Msg{};
FindDeclaredClassByQualifiedNameFn g_findDeclaredClassByQualifiedName{};
std::uintptr_t g_schemaQualifiedClassLookupAddress{};
ClientOffsets g_offsets{};
compat::RuntimeCompatibility g_runtimeCompatibility{};

std::mutex g_stateMutex;
std::mutex g_gunBasisContinuityMutex;
std::mutex g_gunAnchorStabilityMutex;
std::mutex g_scopedFovOverrideMutex;
std::mutex g_viewModelSuppressionMutex;
std::mutex g_solidBrushCacheMutex;
RuntimeState g_state;
OverlayUiState g_ui;
GunBasisContinuityState g_gunBasisContinuity;
GunAnchorStabilityState g_gunAnchorStability;
ScopedFovOverrideState g_scopedFovOverride;
ViewModelSuppressionState g_viewModelSuppression;
DebugOptions g_debugOptions;
std::atomic<bool> g_uiThreadStarted{false};
std::atomic<bool> g_setupViewCallbackObserved{false};
std::atomic<bool> g_enabledSetUpViewObserved{false};
std::atomic<unsigned long long> g_setupViewBridgeCounter{0};
std::atomic<unsigned long long> g_traceFrameCounter{0};
std::atomic<unsigned long long> g_followResolutionTraceCounter{0};
std::atomic<unsigned long long> g_controllerPawnFallbackTraceCounter{0};
std::atomic<unsigned long long> g_knifeFallbackAnchorTraceCounter{0};
std::atomic<int> g_initStage{0};
std::atomic<bool> g_schemaOffsetsResolved{false};
std::atomic<bool> g_schemaResolveInProgress{false};
std::atomic<unsigned long long> g_lastSchemaResolveAttemptTickMs{0};
std::unordered_map<COLORREF, HBRUSH> g_solidBrushCache;
std::mutex g_traceMutex;
std::mutex g_schemaResolveMutex;
std::string g_schemaResolveFailureReason{};

void RefreshStatusTextLocked();
RuntimeState SnapshotState();
void ConsolePrintf(const char* format, ...);
void TracePrintf(const char* format, ...);
void MarkOverlayDirty();
std::string TryGetEntityRttiClassName(void* entity);
bool TryGetEntityAbsOrigin(void* entity, Vec3& origin, std::string& failure);
bool TryReadViewSetupOrigin(void* viewSetup, Vec3& origin);
bool TryReadViewSetupAngles(void* viewSetup, Vec3& angles);
bool IsFiniteVec3(const Vec3& value);
const char* InitStageToString(int stage) noexcept;
bool StartBackgroundSchemaResolve(const char* caller, bool traceAttempt);
DWORD WINAPI SchemaResolveWorkerThreadMain(LPVOID);

std::string FormatIncompatibleBuildReason(const char* detail) {
#if defined(SELFIESTICK_LANG_ZH_CN)
    return std::string("当前 CS2 版本不兼容: ") + detail;
#else
    return std::string("incompatible CS2 build: ") + detail;
#endif
}

bool IsExecutableProtection(DWORD protection) {
    const DWORD normalized = protection & 0xFFu;
    return normalized == PAGE_EXECUTE
        || normalized == PAGE_EXECUTE_READ
        || normalized == PAGE_EXECUTE_READWRITE
        || normalized == PAGE_EXECUTE_WRITECOPY;
}

bool IsReadableProtection(DWORD protection) {
    const DWORD normalized = protection & 0xFFu;
    return normalized == PAGE_READONLY
        || normalized == PAGE_READWRITE
        || normalized == PAGE_WRITECOPY
        || normalized == PAGE_EXECUTE_READ
        || normalized == PAGE_EXECUTE_READWRITE
        || normalized == PAGE_EXECUTE_WRITECOPY;
}

bool IsAddressInModuleWithProtection(HMODULE module, const void* address, bool requireExecutable) {
    if (module == nullptr || address == nullptr) {
        return false;
    }

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(address, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || memoryInfo.AllocationBase != module) {
        return false;
    }

    if ((memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return false;
    }

    return requireExecutable ? IsExecutableProtection(memoryInfo.Protect) : IsReadableProtection(memoryInfo.Protect);
}

bool IsAddressExecutableInModule(HMODULE module, const void* address) {
    return IsAddressInModuleWithProtection(module, address, true);
}

bool IsAddressReadableInModule(HMODULE module, const void* address) {
    return IsAddressInModuleWithProtection(module, address, false);
}

bool TryInvokeSplitScreenPlayer(GetSplitScreenPlayerFn function, int slot, void*& entity) {
    entity = nullptr;
    if (function == nullptr) {
        return false;
    }

    __try {
        entity = function(slot);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        entity = nullptr;
        return false;
    }
}

bool TryInvokeGetEntityFromIndex(void* entityList, int index, void*& entity) {
    entity = nullptr;
    if (entityList == nullptr || g_getEntityFromIndex == nullptr) {
        return false;
    }

    __try {
        entity = g_getEntityFromIndex(entityList, index);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        entity = nullptr;
        return false;
    }
}

bool TryInvokeRenderEyeOrigin(void* entity, void* functionAddress, float output[3]) {
    if (entity == nullptr || functionAddress == nullptr || output == nullptr) {
        return false;
    }

    auto function = reinterpret_cast<void(__fastcall*)(void*, float output[3])>(functionAddress);
    __try {
        function(entity, output);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryInvokeLookupAttachment(void* entity, LookupAttachmentFn function, std::uint8_t& attachmentIndex, const char* attachmentName) {
    if (entity == nullptr || function == nullptr || attachmentName == nullptr) {
        return false;
    }

    __try {
        function(entity, attachmentIndex, attachmentName);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        attachmentIndex = 0xFFu;
        return false;
    }
}

bool TryInvokeGetAttachment(void* entity, GetAttachmentFn function, std::uint8_t attachmentIndex, void* outputTransform, bool& result) {
    result = false;
    if (entity == nullptr || function == nullptr || outputTransform == nullptr) {
        return false;
    }

    __try {
        result = function(entity, attachmentIndex, outputTransform);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadFloatPointer(const float* address, float& value) {
    value = 0.0f;
    if (address == nullptr) {
        return false;
    }

    __try {
        value = *address;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        value = 0.0f;
        return false;
    }
}

bool TryReadViewSetupOrigin(void* viewSetup, Vec3& origin) {
    origin = {};
    if (viewSetup == nullptr) {
        return false;
    }

    auto* viewOrigin = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupOriginOffset);
    if (!TryReadFloatPointer(viewOrigin + 0, origin.x)
        || !TryReadFloatPointer(viewOrigin + 1, origin.y)
        || !TryReadFloatPointer(viewOrigin + 2, origin.z)
        || !IsFiniteVec3(origin)) {
        origin = {};
        return false;
    }

    return true;
}

bool TryReadViewSetupAngles(void* viewSetup, Vec3& angles) {
    angles = {};
    if (viewSetup == nullptr) {
        return false;
    }

    auto* viewAngles = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupAnglesOffset);
    if (!TryReadFloatPointer(viewAngles + 0, angles.x)
        || !TryReadFloatPointer(viewAngles + 1, angles.y)
        || !TryReadFloatPointer(viewAngles + 2, angles.z)
        || !IsFiniteVec3(angles)) {
        angles = {};
        return false;
    }

    return true;
}

template <typename T>
bool TryReadMemoryValue(const T* address, T& value) {
    value = T{};
    if (address == nullptr) {
        return false;
    }

    __try {
        value = *address;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        value = T{};
        return false;
    }
}

bool TryCopyCString(const char* source, std::string& value, std::size_t maxLength = kMaxSchemaNameLength) {
    value.clear();
    if (source == nullptr) {
        return false;
    }

    __try {
        for (std::size_t index = 0; index < maxLength; ++index) {
            const char character = source[index];
            if ('\0' == character) {
                return true;
            }
            value.push_back(character);
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        value.clear();
        return false;
    }

    value.clear();
    return false;
}

bool TryWriteViewSetupPose(float* origin, float* angles, const Vec3& cameraPosition, float pitch, float yaw) {
    if (origin == nullptr || angles == nullptr) {
        return false;
    }

    __try {
        origin[0] = cameraPosition.x;
        origin[1] = cameraPosition.y;
        origin[2] = cameraPosition.z;
        angles[0] = pitch;
        angles[1] = yaw;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool IsFiniteVec3(const Vec3& value) {
    return std::isfinite(value.x) && std::isfinite(value.y) && std::isfinite(value.z);
}

Vec3 Add(const Vec3& left, const Vec3& right) {
    return Vec3{ left.x + right.x, left.y + right.y, left.z + right.z };
}

Vec3 Subtract(const Vec3& left, const Vec3& right) {
    return Vec3{ left.x - right.x, left.y - right.y, left.z - right.z };
}

Vec3 Scale(const Vec3& value, float scale) {
    return Vec3{ value.x * scale, value.y * scale, value.z * scale };
}

float Dot(const Vec3& left, const Vec3& right) {
    return left.x * right.x + left.y * right.y + left.z * right.z;
}

Vec3 Cross(const Vec3& left, const Vec3& right) {
    return Vec3{
        left.y * right.z - left.z * right.y,
        left.z * right.x - left.x * right.z,
        left.x * right.y - left.y * right.x
    };
}

float Length(const Vec3& value) {
    return std::sqrt(Dot(value, value));
}

bool TryNormalize(Vec3& value) {
    const float length = Length(value);
    if (length <= 0.0001f || !std::isfinite(length)) {
        return false;
    }

    const float inverse = 1.0f / length;
    value.x *= inverse;
    value.y *= inverse;
    value.z *= inverse;
    return IsFiniteVec3(value);
}

Vec3 Lerp(const Vec3& start, const Vec3& end, float t) {
    return Vec3{
        start.x + (end.x - start.x) * t,
        start.y + (end.y - start.y) * t,
        start.z + (end.z - start.z) * t
    };
}

Vec3 ProjectOntoPlane(const Vec3& value, const Vec3& planeNormal) {
    return Subtract(value, Scale(planeNormal, Dot(value, planeNormal)));
}

float LerpFloat(float start, float end, float t) {
    return start + (end - start) * t;
}

Vec3 ComposeBasisPoint(const Vec3& origin, const Vec3& right, const Vec3& forward, const Vec3& up, float rightDistance, float forwardDistance, float upDistance) {
    return Add(
        Add(
            Add(origin, Scale(right, rightDistance)),
            Scale(forward, forwardDistance)
        ),
        Scale(up, upDistance)
    );
}

void StabilizeGunBasisRightAxis(EntityHandle targetHandle, WeaponFrameBasis& basis) {
    if (g_debugOptions.disableGunStabilization) {
        return;
    }

    if (!targetHandle.IsValid()) {
        return;
    }

    std::lock_guard lock(g_gunBasisContinuityMutex);
    if (g_gunBasisContinuity.valid
        && g_gunBasisContinuity.targetHandle.raw == targetHandle.raw
        && Dot(basis.right, g_gunBasisContinuity.right) < 0.0f) {
        basis.right = Scale(basis.right, -1.0f);
        basis.up = Scale(basis.up, -1.0f);
    }

    g_gunBasisContinuity.targetHandle = targetHandle;
    g_gunBasisContinuity.right = basis.right;
    g_gunBasisContinuity.valid = true;
}

Vec3 StabilizeGunAnchorOrigin(EntityHandle targetHandle, EntityHandle weaponHandle, const Vec3& handMidpoint, const WeaponFrameBasis& basis, const Vec3& rawAnchorOrigin) {
    if (g_debugOptions.disableGunStabilization) {
        return rawAnchorOrigin;
    }

    if (!targetHandle.IsValid() || !weaponHandle.IsValid()) {
        return rawAnchorOrigin;
    }

    const Vec3 rawRelative = Subtract(rawAnchorOrigin, handMidpoint);
    const float rawRightDistance = Dot(rawRelative, basis.right);
    const float rawForwardDistance = Dot(rawRelative, basis.forward);
    const float rawUpDistance = Dot(rawRelative, basis.up);
    if (!std::isfinite(rawRightDistance) || !std::isfinite(rawForwardDistance) || !std::isfinite(rawUpDistance)) {
        return rawAnchorOrigin;
    }

    std::lock_guard lock(g_gunAnchorStabilityMutex);
    if (!g_gunAnchorStability.valid
        || g_gunAnchorStability.targetHandle.raw != targetHandle.raw
        || g_gunAnchorStability.weaponHandle.raw != weaponHandle.raw) {
        g_gunAnchorStability.targetHandle = targetHandle;
        g_gunAnchorStability.weaponHandle = weaponHandle;
        g_gunAnchorStability.rightDistance = rawRightDistance;
        g_gunAnchorStability.forwardDistance = rawForwardDistance;
        g_gunAnchorStability.upDistance = rawUpDistance;
        g_gunAnchorStability.valid = true;
    }
    else {
        g_gunAnchorStability.rightDistance = LerpFloat(g_gunAnchorStability.rightDistance, rawRightDistance, kGunAnchorStabilizationFactor);
        g_gunAnchorStability.forwardDistance = LerpFloat(g_gunAnchorStability.forwardDistance, rawForwardDistance, kGunAnchorStabilizationFactor);
        g_gunAnchorStability.upDistance = LerpFloat(g_gunAnchorStability.upDistance, rawUpDistance, kGunAnchorStabilizationFactor);
    }

    return ComposeBasisPoint(
        handMidpoint,
        basis.right,
        basis.forward,
        basis.up,
        g_gunAnchorStability.rightDistance,
        g_gunAnchorStability.forwardDistance,
        g_gunAnchorStability.upDistance
    );
}

std::string FormatFloat(float value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    return std::string(buffer);
}

std::string FormatText(const char* format, ...) {
    char buffer[1024]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);
    return std::string(buffer);
}

const char* LookModeToString(LookMode mode) {
    switch (mode) {
    case LookMode::SelfieLeft:
        return Texts().lookSelfieLeft;
    case LookMode::SelfieRight:
        return Texts().lookSelfieRight;
    case LookMode::Forward:
    default:
        return Texts().lookForward;
    }
}

float GetLookModeBaseRightOffset(LookMode mode, float magnitude) {
    switch (mode) {
    case LookMode::SelfieLeft:
        return -magnitude;
    case LookMode::SelfieRight:
        return magnitude;
    case LookMode::Forward:
    default:
        return magnitude;
    }
}

bool IsSelfieLookMode(LookMode mode) {
    return mode == LookMode::SelfieLeft || mode == LookMode::SelfieRight;
}

bool UsesLookTarget(LookMode mode) {
    return IsSelfieLookMode(mode);
}

const wchar_t* GetOverlayDirectionChipText(LookMode mode) {
    switch (mode) {
    case LookMode::SelfieLeft:
        return Texts().uiDirectionChipSelfieLeft;
    case LookMode::SelfieRight:
        return Texts().uiDirectionChipSelfieRight;
    case LookMode::Forward:
    default:
        return Texts().uiDirectionChipForward;
    }
}

const char* TargetModeToString(TargetMode mode) {
    return mode == TargetMode::Follow ? Texts().targetFollow : Texts().targetLocked;
}

RuntimeState SnapshotState() {
    std::lock_guard lock(g_stateMutex);
    return g_state;
}

void RefreshStatusTextLocked() {
    std::ostringstream stream;
    stream
        << "initialized=" << (g_state.initialized ? "1" : "0")
        << " enabled=" << (g_state.enabled ? "1" : "0")
        << " look=" << LookModeToString(g_state.lookMode)
        << " targetMode=" << TargetModeToString(g_state.targetMode)
        << " trimRBU=(" << FormatFloat(g_state.offset.x) << "," << FormatFloat(g_state.offset.y) << "," << FormatFloat(g_state.offset.z) << ")"
        << " lockedHandle=";

    if (g_state.lockedHandle.IsValid()) {
        stream << g_state.lockedHandle.raw;
    }
    else {
        stream << Texts().valueNone;
    }

    stream << " currentTarget=";
    if (g_state.currentTargetHandle.IsValid()) {
        stream << g_state.currentTargetHandle.raw;
    }
    else {
        stream << Texts().valueNone;
    }

    stream
        << " attachment=" << g_state.attachmentName
        << " attachmentAvailable=" << (g_state.attachmentAvailable ? "1" : "0")
        << " lastOverrideSucceeded=" << (g_state.lastOverrideSucceeded ? "1" : "0");

    if (!g_state.lastFailureReason.empty()) {
        stream << " reason=" << g_state.lastFailureReason;
    }

    g_state.lastStatusText = stream.str();
}

void ConsolePrintf(const char* format, ...) {
    char buffer[2048]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    if (g_tier0Msg != nullptr) {
        g_tier0Msg("%s", buffer);
    }

    OutputDebugStringA(buffer);
}

void MarkOverlayDirty() {
    g_ui.dirty.store(true, std::memory_order_relaxed);
}

void SetInitialized(bool initialized, const std::string& reason) {
    std::lock_guard lock(g_stateMutex);
    g_state.initialized = initialized;
    g_state.lastFailureReason = reason;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
}

void SetHookFlags(bool setupViewHooked, bool frameStageHooked) {
    std::lock_guard lock(g_stateMutex);
    g_state.setUpViewHooked = setupViewHooked;
    g_state.frameStageHooked = frameStageHooked;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
}

void SetEnabled(bool enabled) {
    bool changed = false;
    std::lock_guard lock(g_stateMutex);
    changed = g_state.enabled != enabled;
    g_state.enabled = enabled;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
    if (changed) {
        if (enabled) {
            g_enabledSetUpViewObserved.store(false, std::memory_order_relaxed);
        }
        ConsolePrintf(Texts().logEnabled, enabled ? 1 : 0);
    }
}

void ToggleEnabled() {
    bool enabled = false;
    std::lock_guard lock(g_stateMutex);
    g_state.enabled = !g_state.enabled;
    enabled = g_state.enabled;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
    if (enabled) {
        g_enabledSetUpViewObserved.store(false, std::memory_order_relaxed);
    }
    ConsolePrintf(Texts().logEnabled, enabled ? 1 : 0);
}

void SetLookMode(LookMode lookMode) {
    bool changed = false;
    std::lock_guard lock(g_stateMutex);
    changed = g_state.lookMode != lookMode;
    g_state.lookMode = lookMode;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
    if (changed) {
        ConsolePrintf(Texts().logLook, LookModeToString(lookMode));
    }
}

void SetOffset(const Vec3& offset) {
    std::lock_guard lock(g_stateMutex);
    g_state.offset = offset;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
}

void SetTargetMode(TargetMode targetMode) {
    bool changed = false;
    std::lock_guard lock(g_stateMutex);
    changed = g_state.targetMode != targetMode;
    g_state.targetMode = targetMode;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
    if (changed) {
        ConsolePrintf(Texts().logTargetMode, TargetModeToString(targetMode));
    }
}

void ClearLockedTarget() {
    std::lock_guard lock(g_stateMutex);
    g_state.lockedHandle = EntityHandle{};
    g_state.targetMode = TargetMode::Follow;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
    ConsolePrintf(Texts().logTargetCleared);
}

void SetLockedTarget(EntityHandle handle) {
    std::lock_guard lock(g_stateMutex);
    g_state.lockedHandle = handle;
    g_state.targetMode = TargetMode::Locked;
    RefreshStatusTextLocked();
    MarkOverlayDirty();
}

void SetCurrentFollowHandle(EntityHandle handle) {
    bool shouldRefresh = false;
    std::lock_guard lock(g_stateMutex);
    shouldRefresh = g_state.currentFollowHandle.raw != handle.raw;
    g_state.currentFollowHandle = handle;
    if (g_state.targetMode == TargetMode::Follow) {
        shouldRefresh = shouldRefresh || g_state.currentTargetHandle.raw != handle.raw;
        g_state.currentTargetHandle = handle;
    }
    if (shouldRefresh) {
        RefreshStatusTextLocked();
        MarkOverlayDirty();
    }
}

void SetLastFrameStage(int stage) {
    std::lock_guard lock(g_stateMutex);
    g_state.lastFrameStage = stage;
}

void UpdateFailureReason(const std::string& reason, bool attachmentAvailable) {
    bool shouldLog = false;
    bool shouldRefresh = false;
    std::lock_guard lock(g_stateMutex);
    shouldLog = g_state.lastOverrideSucceeded
        || g_state.attachmentAvailable != attachmentAvailable
        || g_state.lastFailureReason != reason;
    shouldRefresh = shouldLog;
    g_state.lastOverrideSucceeded = false;
    g_state.attachmentAvailable = attachmentAvailable;
    g_state.lastFailureReason = reason;
    if (shouldRefresh) {
        RefreshStatusTextLocked();
        MarkOverlayDirty();
    }
    if (shouldLog) {
        ConsolePrintf(Texts().logBlocked, reason.c_str());
    }
}

void UpdateOverrideSuccess(EntityHandle handle) {
    bool shouldLog = false;
    bool shouldRefresh = false;
    std::lock_guard lock(g_stateMutex);
    shouldLog = !g_state.lastOverrideSucceeded || g_state.currentTargetHandle.raw != handle.raw;
    shouldRefresh = shouldLog || !g_state.attachmentAvailable || !g_state.lastFailureReason.empty();
    g_state.currentTargetHandle = handle;
    g_state.lastOverrideSucceeded = true;
    g_state.attachmentAvailable = true;
    g_state.lastFailureReason.clear();
    if (shouldRefresh) {
        RefreshStatusTextLocked();
        MarkOverlayDirty();
    }
    if (shouldLog) {
        ConsolePrintf(Texts().logLive, handle.raw, kAnchorAttachmentName);
    }
}

bool WriteProcessMemoryProtected(void* address, const void* bytes, std::size_t size) {
    DWORD oldProtect{};
    if (!VirtualProtect(address, size, PAGE_EXECUTE_READWRITE, &oldProtect)) {
        return false;
    }

    std::memcpy(address, bytes, size);
    FlushInstructionCache(GetCurrentProcess(), address, size);

    DWORD discard{};
    VirtualProtect(address, size, oldProtect, &discard);
    return true;
}

std::uintptr_t AlignDown(std::uintptr_t value, std::uintptr_t alignment) {
    return value & ~(alignment - 1u);
}

void* AllocateExecutableMemoryNearAddress(const void* address, std::size_t size) {
    if (address == nullptr || size == 0u) {
        return nullptr;
    }

    SYSTEM_INFO systemInfo{};
    GetSystemInfo(&systemInfo);
    const std::uintptr_t granularity = static_cast<std::uintptr_t>(systemInfo.dwAllocationGranularity);
    if (granularity == 0u) {
        return nullptr;
    }

    constexpr std::uintptr_t kMaxRel32Distance = 0x7FFF0000ull;
    const auto target = reinterpret_cast<std::uintptr_t>(address);
    const auto minAddress = target > kMaxRel32Distance ? target - kMaxRel32Distance : granularity;
    const auto maxAddress = target < (std::numeric_limits<std::uintptr_t>::max() - kMaxRel32Distance)
        ? target + kMaxRel32Distance
        : std::numeric_limits<std::uintptr_t>::max() - granularity;

    auto tryAllocate = [size](std::uintptr_t candidate) -> void* {
        return VirtualAlloc(reinterpret_cast<void*>(candidate), size, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE);
    };

    const auto alignedTarget = AlignDown(target, granularity);
    for (std::uintptr_t distance = 0u; distance <= kMaxRel32Distance; distance += granularity) {
        const auto upCandidate = alignedTarget + distance;
        if (upCandidate >= alignedTarget && upCandidate <= maxAddress) {
            if (void* allocation = tryAllocate(upCandidate)) {
                return allocation;
            }
        }

        if (distance == 0u || alignedTarget <= minAddress + distance) {
            continue;
        }

        const auto downCandidate = alignedTarget - distance;
        if (downCandidate >= minAddress) {
            if (void* allocation = tryAllocate(downCandidate)) {
                return allocation;
            }
        }
    }

    return nullptr;
}

std::vector<int> ParsePattern(const char* pattern) {
    std::vector<int> bytes;
    const char* cursor = pattern;

    while (*cursor != '\0') {
        if (*cursor == ' ') {
            ++cursor;
            continue;
        }

        if (*cursor == '?') {
            ++cursor;
            if (*cursor == '?') {
                ++cursor;
            }
            bytes.push_back(-1);
            continue;
        }

        char token[3]{ cursor[0], cursor[1], '\0' };
        bytes.push_back(std::strtoul(token, nullptr, 16));
        cursor += 2;
    }

    return bytes;
}

std::uintptr_t FindPattern(HMODULE module, const char* pattern) {
    if (module == nullptr) {
        return 0;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);
    const std::size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    const auto signature = ParsePattern(pattern);
    if (signature.empty() || imageSize < signature.size()) {
        return 0;
    }

    auto* imageBytes = reinterpret_cast<std::uint8_t*>(module);
    for (std::size_t offset = 0; offset <= imageSize - signature.size(); ++offset) {
        bool matches = true;
        for (std::size_t i = 0; i < signature.size(); ++i) {
            if (signature[i] != -1 && imageBytes[offset + i] != static_cast<std::uint8_t>(signature[i])) {
                matches = false;
                break;
            }
        }

        if (matches) {
            return reinterpret_cast<std::uintptr_t>(imageBytes + offset);
        }
    }

    return 0;
}

std::vector<std::uintptr_t> FindPatterns(HMODULE module, const char* pattern, std::size_t maxMatches = 8) {
    std::vector<std::uintptr_t> matches;
    if (module == nullptr) {
        return matches;
    }

    auto* dosHeader = reinterpret_cast<IMAGE_DOS_HEADER*>(module);
    auto* ntHeaders = reinterpret_cast<IMAGE_NT_HEADERS*>(reinterpret_cast<std::uint8_t*>(module) + dosHeader->e_lfanew);
    const std::size_t imageSize = ntHeaders->OptionalHeader.SizeOfImage;
    const auto signature = ParsePattern(pattern);
    if (signature.empty() || imageSize < signature.size()) {
        return matches;
    }

    auto* imageBytes = reinterpret_cast<std::uint8_t*>(module);
    for (std::size_t offset = 0; offset <= imageSize - signature.size(); ++offset) {
        bool matchesPattern = true;
        for (std::size_t i = 0; i < signature.size(); ++i) {
            if (signature[i] != -1 && imageBytes[offset + i] != static_cast<std::uint8_t>(signature[i])) {
                matchesPattern = false;
                break;
            }
        }

        if (matchesPattern) {
            matches.push_back(reinterpret_cast<std::uintptr_t>(imageBytes + offset));
            if (matches.size() >= maxMatches) {
                break;
            }
        }
    }

    return matches;
}

HMODULE WaitForModule(const wchar_t* moduleName, int timeoutMilliseconds) {
    const int sleepStep = 100;
    int elapsed = 0;
    while (elapsed <= timeoutMilliseconds) {
        if (HMODULE module = GetModuleHandleW(moduleName)) {
            return module;
        }

        Sleep(sleepStep);
        elapsed += sleepStep;
    }

    return nullptr;
}

CreateInterfaceFn GetCreateInterface(HMODULE module) {
    if (module == nullptr) {
        return nullptr;
    }
    return reinterpret_cast<CreateInterfaceFn>(GetProcAddress(module, "CreateInterface"));
}

bool LoadInterfaces() {
    g_clientModule = WaitForModule(kClientModuleName, 10000);
    g_schemaModule = WaitForModule(kSchemaModuleName, 10000);
    g_tier0Module = WaitForModule(kTier0ModuleName, 1000);
    if (g_tier0Module != nullptr) {
        g_tier0Msg = reinterpret_cast<Tier0MsgFn>(GetProcAddress(g_tier0Module, "Msg"));
    }

    if (g_clientModule == nullptr) {
        SetInitialized(false, Texts().reasonClientModuleMissing);
        return false;
    }

    if (g_schemaModule == nullptr) {
        SetInitialized(false, Texts().reasonSchemaModuleMissing);
        return false;
    }

    auto clientFactory = GetCreateInterface(g_clientModule);
    if (clientFactory == nullptr) {
        SetInitialized(false, Texts().reasonCreateInterfaceMissing);
        return false;
    }

    g_source2Client = clientFactory(kSource2ClientVersion, nullptr);
    if (g_source2Client == nullptr) {
        SetInitialized(false, Texts().reasonSource2ClientUnavailable);
        return false;
    }

    return true;
}

bool TryInvokeFindTypeScopeForModule(FindTypeScopeForModuleFn function, void* schemaSystem, const char* moduleName, void*& scope) {
    scope = nullptr;
    if (function == nullptr || schemaSystem == nullptr || moduleName == nullptr) {
        return false;
    }

    __try {
        scope = function(schemaSystem, moduleName, nullptr);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        scope = nullptr;
        return false;
    }
}

bool TryInvokeFindDeclaredClassByQualifiedName(
    FindDeclaredClassByQualifiedNameFn function,
    void* schemaSystem,
    const char* qualifiedClassName,
    void*& classInfo
) {
    classInfo = nullptr;
    if (function == nullptr || schemaSystem == nullptr || qualifiedClassName == nullptr || *qualifiedClassName == '\0') {
        return false;
    }

    __try {
        function(schemaSystem, &classInfo, qualifiedClassName);
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        classInfo = nullptr;
        return false;
    }
}

bool ResolveSchemaQualifiedClassLookupEntry(std::string& failure) {
    failure.clear();
    if (g_findDeclaredClassByQualifiedName != nullptr && g_schemaQualifiedClassLookupAddress != 0u) {
        return true;
    }

    const auto matches = FindPatterns(g_schemaModule, kPatternSchemaQualifiedClassLookup, 4);
    TracePrintf(
        "schema-entry scan name=qualified-class matches=%u traceVersion=%s",
        static_cast<unsigned>(matches.size()),
        kSchemaProbeTraceVersion
    );
    if (matches.empty()) {
        failure = Texts().reasonSchemaPatternMissing;
        return false;
    }

    std::uintptr_t selectedAddress = 0u;
    for (const auto match : matches) {
        TracePrintf("schema-entry candidate name=qualified-class address=%p", reinterpret_cast<void*>(match));
        if (0u == selectedAddress && IsAddressExecutableInModule(g_schemaModule, reinterpret_cast<void*>(match))) {
            selectedAddress = match;
        }
    }

    if (0u == selectedAddress) {
        failure = FormatIncompatibleBuildReason("schema qualified class entry is outside schemasystem.dll");
        return false;
    }

    g_schemaQualifiedClassLookupAddress = selectedAddress;
    g_findDeclaredClassByQualifiedName = reinterpret_cast<FindDeclaredClassByQualifiedNameFn>(selectedAddress);
    TracePrintf(
        "schema-entry pattern-hit name=qualified-class address=%p matches=%u",
        reinterpret_cast<void*>(g_schemaQualifiedClassLookupAddress),
        static_cast<unsigned>(matches.size())
    );
    return true;
}

enum class TypeScopeClassLookupMode {
    Unknown,
    ReturnValue,
    OutParam
};

const char* TypeScopeClassLookupModeToString(TypeScopeClassLookupMode mode) noexcept {
    switch (mode) {
    case TypeScopeClassLookupMode::ReturnValue:
        return "return";
    case TypeScopeClassLookupMode::OutParam:
        return "outparam";
    default:
        return "unknown";
    }
}

int g_typeScopeClassLookupVtableIndex = -1;
TypeScopeClassLookupMode g_typeScopeClassLookupMode = TypeScopeClassLookupMode::Unknown;

bool TryInvokeTypeScopeClassLookupSlot(
    void* scope,
    std::size_t slotIndex,
    TypeScopeClassLookupMode mode,
    const char* className,
    void*& classInfo
);
bool ResolveTypeScopeClassLookup(void* scope);

bool TryInvokeFindDeclaredClassInTypeScope(void* scope, const char* className, void*& classInfo) {
    if (!ResolveTypeScopeClassLookup(scope)) {
        TracePrintf(
            "schema-lookup resolve-failed class=%s scope=%p cachedSlot=%d cachedMode=%s",
            className != nullptr ? className : "<null>",
            scope,
            g_typeScopeClassLookupVtableIndex,
            TypeScopeClassLookupModeToString(g_typeScopeClassLookupMode)
        );
        return false;
    }

    const bool invoked = TryInvokeTypeScopeClassLookupSlot(
        scope,
        static_cast<std::size_t>(g_typeScopeClassLookupVtableIndex),
        g_typeScopeClassLookupMode,
        className,
        classInfo
    );
    if (!invoked || classInfo == nullptr) {
        TracePrintf(
            "schema-lookup call-miss class=%s scope=%p slot=%d mode=%s invoked=%d classInfo=%p",
            className != nullptr ? className : "<null>",
            scope,
            g_typeScopeClassLookupVtableIndex,
            TypeScopeClassLookupModeToString(g_typeScopeClassLookupMode),
            invoked ? 1 : 0,
            classInfo
        );
    }

    return invoked;
}

bool TryReadSchemaFieldSpanModern(const void* classInfo, SchemaFieldModern*& fields, std::uint32_t& count) {
    fields = nullptr;
    count = 0u;
    if (classInfo == nullptr) {
        return false;
    }

    SchemaClassInfoModern info{};
    if (!TryReadMemoryValue(reinterpret_cast<const SchemaClassInfoModern*>(classInfo), info)) {
        return false;
    }

    if (info.fields.elements == nullptr || info.fields.count == 0u || info.fields.count > kMaxReasonableSchemaFieldCount) {
        return false;
    }

    fields = info.fields.elements;
    count = info.fields.count;
    return true;
}

bool TryReadSchemaFieldSpanLegacy(const void* classInfo, SchemaFieldModern*& fields, std::uint32_t& count) {
    fields = nullptr;
    count = 0u;
    if (classInfo == nullptr) {
        return false;
    }

    SchemaClassInfoLegacy info{};
    if (!TryReadMemoryValue(reinterpret_cast<const SchemaClassInfoLegacy*>(classInfo), info)) {
        return false;
    }

    if (info.fields == nullptr || info.fieldCount == 0u || info.fieldCount > kMaxReasonableSchemaFieldCount) {
        return false;
    }

    fields = info.fields;
    count = static_cast<std::uint32_t>(info.fieldCount);
    return true;
}

bool TryReadSchemaFieldArrayInfo(const void* fieldArrayInfo, SchemaFieldModern*& fields, std::uint32_t& count) {
    fields = nullptr;
    count = 0u;
    if (fieldArrayInfo == nullptr) {
        return false;
    }

    SchemaArray<SchemaFieldModern> fieldArray{};
    if (!TryReadMemoryValue(reinterpret_cast<const SchemaArray<SchemaFieldModern>*>(fieldArrayInfo), fieldArray)) {
        return false;
    }

    if (fieldArray.elements == nullptr || fieldArray.count == 0u || fieldArray.count > kMaxReasonableSchemaFieldCount) {
        return false;
    }

    SchemaFieldModern firstField{};
    if (!TryReadMemoryValue(fieldArray.elements, firstField) || firstField.name == nullptr) {
        return false;
    }

    std::string firstFieldName;
    if (!TryCopyCString(firstField.name, firstFieldName, 128u) || firstFieldName.empty()) {
        return false;
    }

    fields = fieldArray.elements;
    count = fieldArray.count;
    return true;
}

bool TryReadSchemaFieldSpanDeclaredPayload(const void* payload, SchemaFieldModern*& fields, std::uint32_t& count, const char*& layoutName) {
    fields = nullptr;
    count = 0u;
    layoutName = nullptr;
    if (payload == nullptr) {
        return false;
    }

    void* schemaClass = nullptr;
    __try {
        if (!schema::TryGetSchemaClassFromDeclaredClassPayload(payload, schemaClass) || schemaClass == nullptr) {
            return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }

    if (TryReadSchemaFieldArrayInfo(schemaClass, fields, count)) {
        layoutName = "declared-field-array";
        return true;
    }

    if (TryReadSchemaFieldSpanModern(schemaClass, fields, count)) {
        layoutName = "declared-modern";
        return true;
    }

    return false;
}

bool TryReadSchemaFieldSpan(const void* classInfo, SchemaFieldModern*& fields, std::uint32_t& count, const char*& layoutName) {
    if (TryReadSchemaFieldArrayInfo(classInfo, fields, count)) {
        layoutName = "field-array";
        return true;
    }

    if (TryReadSchemaFieldSpanModern(classInfo, fields, count)) {
        layoutName = "modern";
        return true;
    }

    if (TryReadSchemaFieldSpanDeclaredPayload(classInfo, fields, count, layoutName)) {
        return true;
    }

    layoutName = "unknown";
    fields = nullptr;
    count = 0u;
    return false;
}

bool ContainsSchemaFieldName(SchemaFieldModern* fields, std::uint32_t fieldCount, const char* expectedFieldName) {
    if (fields == nullptr || fieldCount == 0u || expectedFieldName == nullptr || *expectedFieldName == '\0') {
        return false;
    }

    for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
        SchemaFieldModern field{};
        if (!TryReadMemoryValue(fields + fieldIndex, field) || field.name == nullptr) {
            continue;
        }

        std::string resolvedFieldName;
        if (!TryCopyCString(field.name, resolvedFieldName) || resolvedFieldName.empty()) {
            continue;
        }

        if (resolvedFieldName == expectedFieldName) {
            return true;
        }
    }

    return false;
}

bool ValidateTypeScopeLookupCandidate(
    void* scope,
    std::size_t slotIndex,
    TypeScopeClassLookupMode mode,
    const char*& acceptedProbeClass,
    void*& acceptedClassInfo,
    const char*& acceptedLayoutName,
    std::uint32_t& acceptedFieldCount
) {
    struct ProbeRequirement {
        const char* className;
        const char* expectedFieldName;
    };

    constexpr ProbeRequirement kRequirements[] = {
        { "C_CSPlayerPawn", "m_angEyeAngles" },
        { "C_BaseEntity", "m_pGameSceneNode" },
        { "C_BasePlayerPawn", "m_pObserverServices" },
        { "CBasePlayerController", "m_hPawn" }
    };

    acceptedProbeClass = nullptr;
    acceptedClassInfo = nullptr;
    acceptedLayoutName = nullptr;
    acceptedFieldCount = 0u;

    unsigned matchedCount = 0u;
    for (const auto& requirement : kRequirements) {
        void* classInfoA = nullptr;
        if (!TryInvokeTypeScopeClassLookupSlot(scope, slotIndex, mode, requirement.className, classInfoA) || classInfoA == nullptr) {
            TracePrintf(
                "schema-lookup reject slot=%u mode=%s probeClass=%s reason=lookup-miss",
                static_cast<unsigned>(slotIndex),
                TypeScopeClassLookupModeToString(mode),
                requirement.className
            );
            return false;
        }

        void* classInfoB = nullptr;
        if (!TryInvokeTypeScopeClassLookupSlot(scope, slotIndex, mode, requirement.className, classInfoB) || classInfoB == nullptr) {
            TracePrintf(
                "schema-lookup reject slot=%u mode=%s probeClass=%s reason=lookup-unstable-second-call classInfoA=%p",
                static_cast<unsigned>(slotIndex),
                TypeScopeClassLookupModeToString(mode),
                requirement.className,
                classInfoA
            );
            return false;
        }

        if (classInfoA != classInfoB) {
            TracePrintf(
                "schema-lookup reject slot=%u mode=%s probeClass=%s reason=lookup-unstable-pointer classInfoA=%p classInfoB=%p",
                static_cast<unsigned>(slotIndex),
                TypeScopeClassLookupModeToString(mode),
                requirement.className,
                classInfoA,
                classInfoB
            );
            return false;
        }

        SchemaFieldModern* fields = nullptr;
        std::uint32_t fieldCount = 0u;
        const char* layoutName = nullptr;
        if (!TryReadSchemaFieldSpan(classInfoA, fields, fieldCount, layoutName)) {
            TracePrintf(
                "schema-lookup reject slot=%u mode=%s probeClass=%s reason=field-span-unreadable classInfo=%p",
                static_cast<unsigned>(slotIndex),
                TypeScopeClassLookupModeToString(mode),
                requirement.className,
                classInfoA
            );
            return false;
        }

        if (!ContainsSchemaFieldName(fields, fieldCount, requirement.expectedFieldName)) {
            TracePrintf(
                "schema-lookup reject slot=%u mode=%s probeClass=%s classInfo=%p layout=%s fields=%u expectedField=%s reason=missing-expected-field",
                static_cast<unsigned>(slotIndex),
                TypeScopeClassLookupModeToString(mode),
                requirement.className,
                classInfoA,
                layoutName,
                static_cast<unsigned>(fieldCount),
                requirement.expectedFieldName
            );
            return false;
        }

        if (acceptedProbeClass == nullptr) {
            acceptedProbeClass = requirement.className;
            acceptedClassInfo = classInfoA;
            acceptedLayoutName = layoutName;
            acceptedFieldCount = fieldCount;
        }

        ++matchedCount;
    }

    return matchedCount == std::size(kRequirements);
}

bool TryInvokeTypeScopeClassLookupSlot(
    void* scope,
    std::size_t slotIndex,
    TypeScopeClassLookupMode mode,
    const char* className,
    void*& classInfo
) {
    classInfo = nullptr;
    if (scope == nullptr || className == nullptr || *className == '\0') {
        return false;
    }

    void** vtable = nullptr;
    if (!TryReadMemoryValue(reinterpret_cast<void***>(scope), vtable) || vtable == nullptr) {
        return false;
    }

    void* functionAddress = nullptr;
    if (!TryReadMemoryValue(vtable + slotIndex, functionAddress) || functionAddress == nullptr) {
        return false;
    }

    if (!IsAddressExecutableInModule(g_schemaModule, functionAddress)) {
        return false;
    }

    __try {
        switch (mode) {
        case TypeScopeClassLookupMode::ReturnValue: {
            auto function = reinterpret_cast<FindDeclaredClassInTypeScopeFn>(functionAddress);
            classInfo = function(scope, className);
            return true;
        }
        case TypeScopeClassLookupMode::OutParam: {
            auto function = reinterpret_cast<FindDeclaredClassOutParamFn>(functionAddress);
            function(scope, &classInfo, className);
            return true;
        }
        default:
            return false;
        }
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        classInfo = nullptr;
        return false;
    }
}

bool ResolveTypeScopeClassLookup(void* scope) {
    if (g_typeScopeClassLookupVtableIndex >= 0 && g_typeScopeClassLookupMode != TypeScopeClassLookupMode::Unknown) {
        TracePrintf(
            "schema-lookup cached scope=%p slot=%d mode=%s",
            scope,
            g_typeScopeClassLookupVtableIndex,
            TypeScopeClassLookupModeToString(g_typeScopeClassLookupMode)
        );
        return true;
    }

    constexpr const char* kProbeClasses[] = {
        "C_CSPlayerPawn",
        "C_BaseEntity",
        "C_BasePlayerPawn",
        "CCSPlayerController"
    };
    const std::size_t slotLimit = schema::DetermineTypeScopeLookupProbeSlotLimit();
    TracePrintf(
        "schema-lookup enter scope=%p probeSlots=%u traceVersion=%s",
        scope,
        static_cast<unsigned>(slotLimit),
        kSchemaProbeTraceVersion
    );

    for (std::size_t slotIndex = 0; slotIndex < slotLimit; ++slotIndex) {
        if (slotIndex < 8u || 0u == (slotIndex % 8u)) {
            TracePrintf(
                "schema-lookup probe slot=%u modes=return,outparam",
                static_cast<unsigned>(slotIndex)
            );
        }
        for (const auto mode : { TypeScopeClassLookupMode::ReturnValue, TypeScopeClassLookupMode::OutParam }) {
            const char* probeClass = nullptr;
            void* classInfo = nullptr;
            const char* layoutName = nullptr;
            std::uint32_t fieldCount = 0u;
            if (ValidateTypeScopeLookupCandidate(
                scope,
                slotIndex,
                mode,
                probeClass,
                classInfo,
                layoutName,
                fieldCount
            )) {
                g_typeScopeClassLookupVtableIndex = static_cast<int>(slotIndex);
                g_typeScopeClassLookupMode = mode;
                TracePrintf(
                    "schema-lookup resolved slot=%u mode=%s probeClass=%s classInfo=%p layout=%s fields=%u",
                    static_cast<unsigned>(slotIndex),
                    TypeScopeClassLookupModeToString(mode),
                    probeClass,
                    classInfo,
                    layoutName,
                    static_cast<unsigned>(fieldCount)
                );
                return true;
            }
        }
    }

    TracePrintf("schema-lookup unresolved scope=%p", scope);
    return false;
}

bool TryResolveSchemaSystemFromPattern(void*& schemaSystem, std::string& failure) {
    schemaSystem = nullptr;
    failure.clear();

    const auto schemaInstruction = FindPattern(g_schemaModule, kPatternSchemaSystem);
    if (schemaInstruction == 0u) {
        failure = Texts().reasonSchemaPatternMissing;
        TracePrintf("schema-entry pattern-miss name=raw-system");
        return false;
    }

    std::int32_t displacement = 0;
    if (!TryReadMemoryValue(reinterpret_cast<const std::int32_t*>(schemaInstruction + 3u), displacement)) {
        failure = Texts().reasonSchemaSystemUnavailable;
        TracePrintf("schema-entry pattern-hit name=raw-system instruction=%p reason=displacement-unreadable", reinterpret_cast<void*>(schemaInstruction));
        return false;
    }

    schemaSystem = reinterpret_cast<void*>(schemaInstruction + static_cast<std::intptr_t>(displacement) + 7u);
    TracePrintf(
        "schema-entry pattern-hit name=raw-system instruction=%p schemaSystem=%p",
        reinterpret_cast<void*>(schemaInstruction),
        schemaSystem
    );
    if (schemaSystem == nullptr || !IsAddressReadableInModule(g_schemaModule, schemaSystem)) {
        schemaSystem = nullptr;
        failure = Texts().reasonSchemaSystemUnavailable;
        return false;
    }

    return true;
}

bool TryReadSchemaScopeName(void* scope, std::string& scopeName) {
    scopeName.clear();
    if (scope == nullptr) {
        return false;
    }

    const auto* scopeNameAddress = reinterpret_cast<const char*>(reinterpret_cast<const std::byte*>(scope) + sizeof(void*));
    return TryCopyCString(scopeNameAddress, scopeName);
}

bool TryResolveClientSchemaTypeScopeRaw(void* schemaSystem, void*& scope) {
    scope = nullptr;
    if (schemaSystem == nullptr) {
        return false;
    }

    const auto layout = schema::GetSchemaRawScanLayout();
    if (layout.schemaSystemScopeArrayOffset < static_cast<std::ptrdiff_t>(sizeof(std::uint64_t))) {
        return false;
    }

    const auto* schemaBytes = reinterpret_cast<const std::byte*>(schemaSystem);
    const auto scopeCountOffset = layout.schemaSystemScopeArrayOffset - static_cast<std::ptrdiff_t>(sizeof(std::uint64_t));

    std::uint64_t scopeCount = 0u;
    void** scopeArray = nullptr;
    if (!TryReadMemoryValue(reinterpret_cast<const std::uint64_t*>(schemaBytes + scopeCountOffset), scopeCount)
        || !TryReadMemoryValue(reinterpret_cast<void***>(const_cast<std::byte*>(schemaBytes) + layout.schemaSystemScopeArrayOffset), scopeArray)
        || scopeArray == nullptr
        || scopeCount == 0u
        || scopeCount > 0x400u) {
        TracePrintf(
            "schema-scope raw-system-invalid schemaSystem=%p scopeCount=%llu scopeArray=%p",
            schemaSystem,
            static_cast<unsigned long long>(scopeCount),
            scopeArray
        );
        return false;
    }

    for (std::uint64_t scopeIndex = 0; scopeIndex < scopeCount; ++scopeIndex) {
        void* candidateScope = nullptr;
        if (!TryReadMemoryValue(scopeArray + scopeIndex, candidateScope) || candidateScope == nullptr) {
            continue;
        }

        std::string scopeName;
        if (!TryReadSchemaScopeName(candidateScope, scopeName)) {
            continue;
        }

        if (scopeName != kClientSchemaScopeName) {
            continue;
        }

        scope = candidateScope;
        TracePrintf(
            "schema-scope raw-hit schemaSystem=%p scope=%p scopeIndex=%llu name=%s",
            schemaSystem,
            scope,
            static_cast<unsigned long long>(scopeIndex),
            scopeName.c_str()
        );
        return true;
    }

    TracePrintf(
        "schema-scope raw-miss schemaSystem=%p scopeCount=%llu target=%s",
        schemaSystem,
        static_cast<unsigned long long>(scopeCount),
        kClientSchemaScopeName
    );
    return false;
}

bool TryCollectTypeScopeClassPayloads(void* scope, std::vector<void*>& payloads) {
    payloads.clear();
    if (scope == nullptr) {
        return false;
    }

    const auto layout = schema::GetSchemaRawScanLayout();
    const auto* scopeBytes = reinterpret_cast<const std::byte*>(scope);
    std::int32_t declaredClassCountSigned = 0;
    if (!TryReadMemoryValue(
        reinterpret_cast<const std::int32_t*>(scopeBytes + layout.typeScopeDeclaredClassCountOffset),
        declaredClassCountSigned
    )) {
        TracePrintf("schema-bucket unreadable-count scope=%p", scope);
        return false;
    }

    const std::uint32_t declaredClassCount = declaredClassCountSigned > 0
        ? static_cast<std::uint32_t>(declaredClassCountSigned)
        : 0u;
    const std::size_t payloadLimit = static_cast<std::size_t>(schema::DetermineDeclaredClassScanLimit(declaredClassCount));
    const auto* bucketBase = scopeBytes + layout.typeScopeBucketTableOffset;

    std::unordered_set<void*> seenNodes;
    std::unordered_set<void*> seenPayloads;
    std::size_t traversedNodes = 0u;
    const std::size_t nodeLimit = payloadLimit * 4u;

    for (std::size_t bucketIndex = 0; bucketIndex < layout.typeScopeBucketCount; ++bucketIndex) {
        const auto* bucket = bucketBase + (bucketIndex * static_cast<std::size_t>(layout.typeScopeBucketStride));
        void* node = nullptr;
        if (!TryReadMemoryValue(reinterpret_cast<void* const*>(bucket + layout.typeScopeBucketHeadOffset), node) || node == nullptr) {
            continue;
        }

        while (node != nullptr && traversedNodes < nodeLimit) {
            if (!seenNodes.insert(node).second) {
                break;
            }

            ++traversedNodes;
            const auto* nodeBytes = reinterpret_cast<const std::byte*>(node);

            void* payload = nullptr;
            if (TryReadMemoryValue(reinterpret_cast<void* const*>(nodeBytes + layout.typeScopeBucketNodePayloadOffset), payload)
                && payload != nullptr
                && seenPayloads.insert(payload).second) {
                payloads.push_back(payload);
                if (payloads.size() >= payloadLimit) {
                    break;
                }
            }

            void* next = nullptr;
            if (!TryReadMemoryValue(reinterpret_cast<void* const*>(nodeBytes + layout.typeScopeBucketNodeNextOffset), next)) {
                break;
            }

            node = next;
        }

        if (payloads.size() >= payloadLimit || traversedNodes >= nodeLimit) {
            break;
        }
    }

    TracePrintf(
        "schema-bucket collect scope=%p declaredCount=%u payloadLimit=%u payloads=%u traversedNodes=%u",
        scope,
        static_cast<unsigned>(declaredClassCount),
        static_cast<unsigned>(payloadLimit),
        static_cast<unsigned>(payloads.size()),
        static_cast<unsigned>(traversedNodes)
    );
    return !payloads.empty();
}

bool TryGetSchemaFieldOffsetFromClassInfo(
    void* classInfo,
    const char* fieldName,
    std::ptrdiff_t& target,
    const char*& layoutName,
    std::uint32_t& fieldCount
) {
    target = 0;
    layoutName = nullptr;
    fieldCount = 0u;
    if (classInfo == nullptr || fieldName == nullptr || *fieldName == '\0') {
        return false;
    }

    SchemaFieldModern* fields = nullptr;
    if (!TryReadSchemaFieldSpan(classInfo, fields, fieldCount, layoutName)) {
        return false;
    }

    for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
        SchemaFieldModern field{};
        if (!TryReadMemoryValue(fields + fieldIndex, field) || field.name == nullptr) {
            continue;
        }

        std::string resolvedFieldName;
        if (!TryCopyCString(field.name, resolvedFieldName) || resolvedFieldName.empty()) {
            continue;
        }

        if (resolvedFieldName != fieldName) {
            continue;
        }

        target = static_cast<std::ptrdiff_t>(field.offset);
        return true;
    }

    return false;
}

bool TryGetSchemaFieldOffsetFromSpan(
    SchemaFieldModern* fields,
    std::uint32_t fieldCount,
    const char* fieldName,
    std::ptrdiff_t& target
) {
    target = 0;
    if (fields == nullptr || fieldCount == 0u || fieldName == nullptr || *fieldName == '\0') {
        return false;
    }

    for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
        SchemaFieldModern field{};
        if (!TryReadMemoryValue(fields + fieldIndex, field) || field.name == nullptr) {
            continue;
        }

        std::string resolvedFieldName;
        if (!TryCopyCString(field.name, resolvedFieldName) || resolvedFieldName.empty()) {
            continue;
        }

        if (resolvedFieldName != fieldName) {
            continue;
        }

        target = static_cast<std::ptrdiff_t>(field.offset);
        return true;
    }

    return false;
}

bool RawFieldCandidateMatches(
    SchemaFieldModern* fields,
    std::uint32_t fieldCount,
    const char* fieldName,
    std::initializer_list<const char*> requiredFields,
    std::ptrdiff_t& target
) {
    if (fields == nullptr || fieldCount == 0u) {
        return false;
    }

    for (const char* requiredField : requiredFields) {
        if (!ContainsSchemaFieldName(fields, fieldCount, requiredField)) {
            return false;
        }
    }

    return TryGetSchemaFieldOffsetFromSpan(fields, fieldCount, fieldName, target);
}

bool TryReadSchemaFieldSpanAtPayloadPointerOffset(
    void* payload,
    std::size_t pointerOffset,
    SchemaFieldModern*& fields,
    std::uint32_t& fieldCount,
    const char*& layoutName
) {
    fields = nullptr;
    fieldCount = 0u;
    layoutName = nullptr;
    if (payload == nullptr) {
        return false;
    }

    const auto* payloadBytes = reinterpret_cast<const std::byte*>(payload);
    void* schemaClass = nullptr;
    if (!TryReadMemoryValue(reinterpret_cast<void* const*>(payloadBytes + pointerOffset), schemaClass)
        || schemaClass == nullptr
        || schemaClass == payload) {
        return false;
    }

    if (TryReadSchemaFieldArrayInfo(schemaClass, fields, fieldCount)) {
        layoutName = "declared-field-array";
        return true;
    }

    if (TryReadSchemaFieldSpanModern(schemaClass, fields, fieldCount)) {
        layoutName = "declared-modern";
        return true;
    }

    return false;
}

bool IsLikelySchemaName(const std::string& value) {
    if (value.empty() || value.size() > kMaxSchemaNameLength) {
        return false;
    }

    for (const char character : value) {
        const auto byte = static_cast<unsigned char>(character);
        if (byte < 0x20u || byte > 0x7Eu) {
            return false;
        }
    }

    return true;
}

bool TryReadStringPointerAtOffset(const void* base, std::size_t pointerOffset, std::string& value, const char*& stringPointer) {
    value.clear();
    stringPointer = nullptr;
    if (base == nullptr) {
        return false;
    }

    const auto* bytes = static_cast<const std::byte*>(base);
    const char* candidate = nullptr;
    if (!TryReadMemoryValue(reinterpret_cast<const char* const*>(bytes + pointerOffset), candidate) || candidate == nullptr) {
        return false;
    }

    std::string resolved;
    if (!TryCopyCString(candidate, resolved, 128u) || !IsLikelySchemaName(resolved)) {
        return false;
    }

    value = resolved;
    stringPointer = candidate;
    return true;
}

bool PayloadStringMatchesClassName(void* payload, const char* className, std::size_t& stringPointerOffset, std::string& matchedValue) {
    stringPointerOffset = 0u;
    matchedValue.clear();
    if (payload == nullptr || className == nullptr || *className == '\0') {
        return false;
    }

    for (std::size_t pointerOffset = 0u; pointerOffset <= kSchemaDiagnosticStringSlotLimit; pointerOffset += sizeof(void*)) {
        const char* stringPointer = nullptr;
        std::string value;
        if (!TryReadStringPointerAtOffset(payload, pointerOffset, value, stringPointer)) {
            continue;
        }

        if (value == className || value.find(className) != std::string::npos) {
            stringPointerOffset = pointerOffset;
            matchedValue = value;
            return true;
        }
    }

    return false;
}

bool PayloadStringEqualsClassName(void* payload, const char* className) {
    if (payload == nullptr || className == nullptr || *className == '\0') {
        return false;
    }

    for (std::size_t pointerOffset = 0u; pointerOffset <= kSchemaDiagnosticStringSlotLimit; pointerOffset += sizeof(void*)) {
        const char* stringPointer = nullptr;
        std::string value;
        if (!TryReadStringPointerAtOffset(payload, pointerOffset, value, stringPointer)) {
            continue;
        }

        if (value == className) {
            return true;
        }
    }

    return false;
}

bool TryReadPointerAtOffset(const void* base, std::size_t pointerOffset, void*& value) {
    value = nullptr;
    if (base == nullptr) {
        return false;
    }

    const auto* bytes = static_cast<const std::byte*>(base);
    return TryReadMemoryValue(reinterpret_cast<void* const*>(bytes + pointerOffset), value) && value != nullptr;
}

std::string FormatSchemaFieldSampleByStride(
    void* fieldBase,
    std::uint32_t fieldCount,
    std::size_t fieldStride,
    std::size_t fieldNameOffset,
    std::size_t fieldOffsetOffset
) {
    std::ostringstream stream;
    std::uint32_t emitted = 0u;
    const auto sampleCount = fieldCount < 8u ? fieldCount : 8u;
    const auto* fieldBytes = static_cast<const std::byte*>(fieldBase);

    for (std::uint32_t fieldIndex = 0u; fieldIndex < sampleCount; ++fieldIndex) {
        const auto* entry = fieldBytes + (static_cast<std::size_t>(fieldIndex) * fieldStride);
        const char* fieldNamePointer = nullptr;
        if (!TryReadMemoryValue(reinterpret_cast<const char* const*>(entry + fieldNameOffset), fieldNamePointer)
            || fieldNamePointer == nullptr) {
            continue;
        }

        std::string fieldName;
        if (!TryCopyCString(fieldNamePointer, fieldName, 128u) || !IsLikelySchemaName(fieldName)) {
            continue;
        }

        std::uint32_t fieldOffset = 0u;
        if (!TryReadMemoryValue(reinterpret_cast<const std::uint32_t*>(entry + fieldOffsetOffset), fieldOffset)) {
            fieldOffset = 0u;
        }

        if (emitted > 0u) {
            stream << ",";
        }
        stream << fieldName << "@0x" << std::hex << std::uppercase << fieldOffset << std::dec;
        ++emitted;
    }

    if (emitted == 0u) {
        return {};
    }

    return stream.str();
}

std::string FormatSchemaFieldSampleModern(SchemaFieldModern* fields, std::uint32_t fieldCount) {
    std::ostringstream stream;
    std::uint32_t emitted = 0u;
    const auto sampleCount = fieldCount < 8u ? fieldCount : 8u;
    for (std::uint32_t fieldIndex = 0u; fieldIndex < sampleCount; ++fieldIndex) {
        SchemaFieldModern field{};
        if (!TryReadMemoryValue(fields + fieldIndex, field) || field.name == nullptr) {
            continue;
        }

        std::string fieldName;
        if (!TryCopyCString(field.name, fieldName, 128u) || !IsLikelySchemaName(fieldName)) {
            continue;
        }

        if (emitted > 0u) {
            stream << ",";
        }
        stream << fieldName << "@0x" << std::hex << std::uppercase << field.offset << std::dec;
        ++emitted;
    }

    return stream.str();
}

void TraceGenericSchemaFieldArrayCandidates(void* payload, const char* className, void* classInfo, const char* classInfoSource) {
    if (classInfo == nullptr) {
        return;
    }

    constexpr std::array<std::size_t, 5> kFieldStrides{ 0x20u, 0x28u, 0x30u, 0x38u, 0x40u };
    constexpr std::array<std::size_t, 4> kFieldNameOffsets{ 0x00u, 0x08u, 0x10u, 0x18u };
    constexpr std::array<std::size_t, 5> kFieldOffsetOffsets{ 0x0Cu, 0x10u, 0x14u, 0x18u, 0x20u };

    std::uint32_t emittedCandidates = 0u;
    const auto* classBytes = static_cast<const std::byte*>(classInfo);
    for (std::size_t arrayOffset = 0u; arrayOffset <= 0x120u && emittedCandidates < 6u; arrayOffset += 4u) {
        void* fieldBase = nullptr;
        std::uint32_t fieldCount = 0u;
        if (!TryReadMemoryValue(reinterpret_cast<void* const*>(classBytes + arrayOffset), fieldBase)
            || !TryReadMemoryValue(reinterpret_cast<const std::uint32_t*>(classBytes + arrayOffset + sizeof(void*)), fieldCount)
            || fieldBase == nullptr
            || fieldCount == 0u
            || fieldCount > kMaxReasonableSchemaFieldCount) {
            continue;
        }

        for (const std::size_t fieldStride : kFieldStrides) {
            for (const std::size_t fieldNameOffset : kFieldNameOffsets) {
                for (const std::size_t fieldOffsetOffset : kFieldOffsetOffsets) {
                    const auto sample = FormatSchemaFieldSampleByStride(
                        fieldBase,
                        fieldCount,
                        fieldStride,
                        fieldNameOffset,
                        fieldOffsetOffset
                    );
                    if (sample.empty()) {
                        continue;
                    }

                    TracePrintf(
                        "schema-diag array class=%s payload=%p classInfo=%p source=%s arrayOffset=0x%llX fields=%u stride=0x%llX nameOff=0x%llX offsetOff=0x%llX sample=%s",
                        className != nullptr ? className : "<unknown>",
                        payload,
                        classInfo,
                        classInfoSource != nullptr ? classInfoSource : "<unknown>",
                        static_cast<unsigned long long>(arrayOffset),
                        static_cast<unsigned>(fieldCount),
                        static_cast<unsigned long long>(fieldStride),
                        static_cast<unsigned long long>(fieldNameOffset),
                        static_cast<unsigned long long>(fieldOffsetOffset),
                        sample.c_str()
                    );

                    ++emittedCandidates;
                    if (emittedCandidates >= 6u) {
                        return;
                    }
                }
            }
        }
    }
}

bool TryReadSchemaFieldArrayAtOffset(
    const void* classInfo,
    std::size_t arrayOffset,
    void*& fieldBase,
    std::uint32_t& fieldCount
) {
    fieldBase = nullptr;
    fieldCount = 0u;
    if (classInfo == nullptr) {
        return false;
    }

    const auto* classBytes = static_cast<const std::byte*>(classInfo);
    if (!TryReadMemoryValue(reinterpret_cast<void* const*>(classBytes + arrayOffset), fieldBase)
        || !TryReadMemoryValue(reinterpret_cast<const std::uint32_t*>(classBytes + arrayOffset + sizeof(void*)), fieldCount)
        || fieldBase == nullptr
        || fieldCount == 0u
        || fieldCount > kMaxReasonableSchemaFieldCount) {
        return false;
    }

    return true;
}

bool TryReadGenericSchemaFieldName(
    void* fieldBase,
    std::uint32_t fieldCount,
    std::size_t fieldStride,
    std::size_t fieldNameOffset,
    std::uint32_t fieldIndex,
    std::string& fieldName
) {
    fieldName.clear();
    if (fieldBase == nullptr || fieldIndex >= fieldCount) {
        return false;
    }

    const auto* fieldBytes = static_cast<const std::byte*>(fieldBase);
    const auto* entry = fieldBytes + (static_cast<std::size_t>(fieldIndex) * fieldStride);
    const char* fieldNamePointer = nullptr;
    if (!TryReadMemoryValue(reinterpret_cast<const char* const*>(entry + fieldNameOffset), fieldNamePointer)
        || fieldNamePointer == nullptr) {
        return false;
    }

    return TryCopyCString(fieldNamePointer, fieldName, 128u) && IsLikelySchemaName(fieldName);
}

bool ContainsGenericSchemaFieldName(
    void* fieldBase,
    std::uint32_t fieldCount,
    std::size_t fieldStride,
    std::size_t fieldNameOffset,
    const char* expectedFieldName
) {
    if (expectedFieldName == nullptr || *expectedFieldName == '\0') {
        return false;
    }

    for (std::uint32_t fieldIndex = 0u; fieldIndex < fieldCount; ++fieldIndex) {
        std::string fieldName;
        if (!TryReadGenericSchemaFieldName(fieldBase, fieldCount, fieldStride, fieldNameOffset, fieldIndex, fieldName)) {
            continue;
        }

        if (fieldName == expectedFieldName) {
            return true;
        }
    }

    return false;
}

bool TryGetGenericSchemaFieldOffset(
    void* fieldBase,
    std::uint32_t fieldCount,
    std::size_t fieldStride,
    std::size_t fieldNameOffset,
    std::size_t fieldOffsetOffset,
    const char* fieldName,
    std::initializer_list<const char*> requiredFields,
    std::ptrdiff_t& target
) {
    target = 0;
    if (fieldBase == nullptr || fieldName == nullptr || *fieldName == '\0') {
        return false;
    }

    for (const char* requiredField : requiredFields) {
        if (!ContainsGenericSchemaFieldName(fieldBase, fieldCount, fieldStride, fieldNameOffset, requiredField)) {
            return false;
        }
    }

    const auto* fieldBytes = static_cast<const std::byte*>(fieldBase);
    for (std::uint32_t fieldIndex = 0u; fieldIndex < fieldCount; ++fieldIndex) {
        std::string resolvedFieldName;
        if (!TryReadGenericSchemaFieldName(fieldBase, fieldCount, fieldStride, fieldNameOffset, fieldIndex, resolvedFieldName)
            || resolvedFieldName != fieldName) {
            continue;
        }

        const auto* entry = fieldBytes + (static_cast<std::size_t>(fieldIndex) * fieldStride);
        std::uint32_t resolvedOffset = 0u;
        if (!TryReadMemoryValue(reinterpret_cast<const std::uint32_t*>(entry + fieldOffsetOffset), resolvedOffset)) {
            continue;
        }

        if (resolvedOffset == 0u || resolvedOffset > 0x5000u) {
            continue;
        }

        target = static_cast<std::ptrdiff_t>(resolvedOffset);
        return true;
    }

    return false;
}

bool TryGetSchemaFieldOffsetFromGenericRawPayloads(
    const std::vector<void*>& payloads,
    const char* traceClassName,
    const char* fieldName,
    std::initializer_list<const char*> requiredFields,
    std::ptrdiff_t& target
) {
    struct GenericMatch {
        void* payload{};
        void* classInfo{};
        std::ptrdiff_t offset{};
        std::uint32_t fieldCount{};
        std::size_t wrapperOffset{};
        std::size_t arrayOffset{};
        std::size_t fieldStride{};
        std::size_t fieldNameOffset{};
        std::size_t fieldOffsetOffset{};
    };

    if (traceClassName == nullptr || *traceClassName == '\0' || fieldName == nullptr || *fieldName == '\0') {
        return false;
    }

    std::array<schema::GenericRawFieldArrayLayoutCandidate, 12> layouts{};
    const std::size_t layoutCount = schema::BuildFocusedGenericRawFieldArrayLayoutCandidates(layouts);
    GenericMatch match{};
    std::size_t matchCount = 0u;
    bool ambiguousDifferentOffset = false;

    for (void* payload : payloads) {
        if (!PayloadStringEqualsClassName(payload, traceClassName)) {
            continue;
        }

        for (std::size_t layoutIndex = 0u; layoutIndex < layoutCount; ++layoutIndex) {
            const auto& layout = layouts[layoutIndex];
            void* classInfo = payload;
            if (layout.wrapperOffset != 0u) {
                void* wrappedClassInfo = nullptr;
                if (!TryReadPointerAtOffset(payload, layout.wrapperOffset, wrappedClassInfo) || wrappedClassInfo == payload) {
                    continue;
                }
                classInfo = wrappedClassInfo;
            }

            void* fieldBase = nullptr;
            std::uint32_t fieldCount = 0u;
            if (!TryReadSchemaFieldArrayAtOffset(classInfo, layout.arrayOffset, fieldBase, fieldCount)) {
                continue;
            }

            std::ptrdiff_t resolvedOffset = 0;
            const bool resolvedWithRequiredFields = TryGetGenericSchemaFieldOffset(
                fieldBase,
                fieldCount,
                layout.fieldStride,
                layout.fieldNameOffset,
                layout.fieldOffsetOffset,
                fieldName,
                requiredFields,
                resolvedOffset
            );
            const bool resolvedWithoutRequiredFields = !resolvedWithRequiredFields
                && requiredFields.size() > 1u
                && TryGetGenericSchemaFieldOffset(
                    fieldBase,
                    fieldCount,
                    layout.fieldStride,
                    layout.fieldNameOffset,
                    layout.fieldOffsetOffset,
                    fieldName,
                    {},
                    resolvedOffset
                );
            if (!resolvedWithRequiredFields && !resolvedWithoutRequiredFields) {
                continue;
            }

            if (matchCount > 0u && match.offset != resolvedOffset) {
                ambiguousDifferentOffset = true;
                break;
            }

            ++matchCount;
            match = GenericMatch{
                .payload = payload,
                .classInfo = classInfo,
                .offset = resolvedOffset,
                .fieldCount = fieldCount,
                .wrapperOffset = layout.wrapperOffset,
                .arrayOffset = layout.arrayOffset,
                .fieldStride = layout.fieldStride,
                .fieldNameOffset = layout.fieldNameOffset,
                .fieldOffsetOffset = layout.fieldOffsetOffset
            };
        }

        if (ambiguousDifferentOffset) {
            break;
        }
    }

    if (matchCount > 0u && !ambiguousDifferentOffset) {
        target = match.offset;
        TracePrintf(
            "schema-raw field=%s traceClass=%s payload=%p classInfo=%p layout=focused-generic-field-array fields=%u target=0x%llX matches=%u wrapperOffset=0x%llX arrayOffset=0x%llX stride=0x%llX nameOff=0x%llX offsetOff=0x%llX",
            fieldName,
            traceClassName,
            match.payload,
            match.classInfo,
            static_cast<unsigned>(match.fieldCount),
            static_cast<unsigned long long>(target),
            static_cast<unsigned>(matchCount),
            static_cast<unsigned long long>(match.wrapperOffset),
            static_cast<unsigned long long>(match.arrayOffset),
            static_cast<unsigned long long>(match.fieldStride),
            static_cast<unsigned long long>(match.fieldNameOffset),
            static_cast<unsigned long long>(match.fieldOffsetOffset)
        );
        return true;
    }

    if (matchCount > 0u || ambiguousDifferentOffset) {
        TracePrintf(
            "schema-raw generic-ambiguous traceClass=%s field=%s matches=%u",
            traceClassName,
            fieldName,
            static_cast<unsigned>(matchCount)
        );
    }

    return false;
}

void TraceSchemaClassCandidateDiagnostics(void* payload, const char* className, void* classInfo, const char* sourceName) {
    if (classInfo == nullptr) {
        return;
    }

    SchemaFieldModern* fields = nullptr;
    std::uint32_t fieldCount = 0u;
    if (TryReadSchemaFieldSpanModern(classInfo, fields, fieldCount)) {
        const auto sample = FormatSchemaFieldSampleModern(fields, fieldCount);
        TracePrintf(
            "schema-diag span class=%s payload=%p classInfo=%p source=%s layout=modern fields=%u sample=%s",
            className != nullptr ? className : "<unknown>",
            payload,
            classInfo,
            sourceName != nullptr ? sourceName : "<unknown>",
            static_cast<unsigned>(fieldCount),
            sample.empty() ? "<none>" : sample.c_str()
        );
    }

    if (TryReadSchemaFieldSpanLegacy(classInfo, fields, fieldCount)) {
        const auto sample = FormatSchemaFieldSampleModern(fields, fieldCount);
        TracePrintf(
            "schema-diag span class=%s payload=%p classInfo=%p source=%s layout=legacy fields=%u sample=%s",
            className != nullptr ? className : "<unknown>",
            payload,
            classInfo,
            sourceName != nullptr ? sourceName : "<unknown>",
            static_cast<unsigned>(fieldCount),
            sample.empty() ? "<none>" : sample.c_str()
        );
    }

    TraceGenericSchemaFieldArrayCandidates(payload, className, classInfo, sourceName);
}

void TraceSchemaRawPayloadDiagnostics(const std::vector<void*>& payloads) {
    constexpr std::array<const char*, 9> kTargetClasses{
        "CEntityInstance",
        "CBasePlayerController",
        "C_BasePlayerPawn",
        "C_CSPlayerPawn",
        "CPlayer_ObserverServices",
        "CPlayer_CameraServices",
        "CPlayer_WeaponServices",
        "C_BaseEntity",
        "CGameSceneNode"
    };

    const std::size_t sampleLimit = payloads.size() < kSchemaDiagnosticPayloadSampleLimit
        ? payloads.size()
        : kSchemaDiagnosticPayloadSampleLimit;
    for (std::size_t payloadIndex = 0u; payloadIndex < sampleLimit; ++payloadIndex) {
        void* payload = payloads[payloadIndex];
        std::ostringstream slots;
        std::uint32_t emittedSlots = 0u;
        for (std::size_t pointerOffset = 0u; pointerOffset <= kSchemaDiagnosticStringSlotLimit; pointerOffset += sizeof(void*)) {
            const char* stringPointer = nullptr;
            std::string value;
            if (!TryReadStringPointerAtOffset(payload, pointerOffset, value, stringPointer)) {
                continue;
            }

            if (emittedSlots > 0u) {
                slots << ",";
            }
            slots << "0x" << std::hex << std::uppercase << pointerOffset << std::dec << "=" << value;
            ++emittedSlots;
            if (emittedSlots >= 8u) {
                break;
            }
        }

        TracePrintf(
            "schema-diag sample payloadIndex=%u payload=%p stringSlots=%s",
            static_cast<unsigned>(payloadIndex),
            payload,
            emittedSlots == 0u ? "<none>" : slots.str().c_str()
        );
    }

    std::array<std::size_t, 8> wrapperOffsets{};
    const std::size_t wrapperOffsetCount = schema::BuildDeclaredClassSchemaClassPointerOffsets(wrapperOffsets);

    for (const char* targetClassName : kTargetClasses) {
        std::uint32_t classHits = 0u;
        for (void* payload : payloads) {
            std::size_t stringPointerOffset = 0u;
            std::string matchedValue;
            if (!PayloadStringMatchesClassName(payload, targetClassName, stringPointerOffset, matchedValue)) {
                continue;
            }

            ++classHits;
            TracePrintf(
                "schema-diag target-hit class=%s payload=%p nameSlot=0x%llX value=%s",
                targetClassName,
                payload,
                static_cast<unsigned long long>(stringPointerOffset),
                matchedValue.c_str()
            );

            TraceSchemaClassCandidateDiagnostics(payload, targetClassName, payload, "payload-direct");
            for (std::size_t offsetIndex = 0u; offsetIndex < wrapperOffsetCount; ++offsetIndex) {
                void* classInfo = nullptr;
                const std::size_t wrapperOffset = wrapperOffsets[offsetIndex];
                if (!TryReadPointerAtOffset(payload, wrapperOffset, classInfo) || classInfo == payload) {
                    continue;
                }

                char sourceName[32]{};
                std::snprintf(sourceName, sizeof(sourceName), "payload+0x%llX", static_cast<unsigned long long>(wrapperOffset));
                TracePrintf(
                    "schema-diag classptr class=%s payload=%p wrapperOffset=0x%llX classInfo=%p",
                    targetClassName,
                    payload,
                    static_cast<unsigned long long>(wrapperOffset),
                    classInfo
                );
                TraceSchemaClassCandidateDiagnostics(payload, targetClassName, classInfo, sourceName);
            }

            if (classHits >= 3u) {
                break;
            }
        }

        TracePrintf(
            "schema-diag target-summary class=%s hits=%u payloads=%u",
            targetClassName,
            static_cast<unsigned>(classHits),
            static_cast<unsigned>(payloads.size())
        );
    }
}

bool TryGetSchemaFieldOffsetFromRawPayloads(
    const std::vector<void*>& payloads,
    const char* traceClassName,
    const char* fieldName,
    std::initializer_list<const char*> requiredFields,
    std::ptrdiff_t& target
) {
    std::size_t matchCount = 0u;
    std::ptrdiff_t resolvedTarget = 0;
    const char* resolvedLayoutName = nullptr;
    void* resolvedPayload = nullptr;
    std::uint32_t resolvedFieldCount = 0u;
    std::size_t resolvedPointerOffset = 0u;

    for (void* payload : payloads) {
        SchemaFieldModern* fields = nullptr;
        std::uint32_t fieldCount = 0u;
        const char* layoutName = nullptr;

        auto acceptCandidate = [&](const char* candidateLayoutName, std::size_t pointerOffset) -> bool {
            ++matchCount;
            resolvedTarget = target;
            resolvedLayoutName = candidateLayoutName;
            resolvedPayload = payload;
            resolvedFieldCount = fieldCount;
            resolvedPointerOffset = pointerOffset;
            return true;
        };

        if (TryReadSchemaFieldSpanModern(payload, fields, fieldCount)
            && RawFieldCandidateMatches(fields, fieldCount, fieldName, requiredFields, target)
            && acceptCandidate("modern", 0u)) {
            continue;
        }

        std::array<std::size_t, 8> pointerOffsets{};
        const std::size_t pointerOffsetCount = schema::BuildDeclaredClassSchemaClassPointerOffsets(pointerOffsets);
        for (std::size_t pointerOffsetIndex = 0u; pointerOffsetIndex < pointerOffsetCount; ++pointerOffsetIndex) {
            const std::size_t pointerOffset = pointerOffsets[pointerOffsetIndex];
            if (!TryReadSchemaFieldSpanAtPayloadPointerOffset(payload, pointerOffset, fields, fieldCount, layoutName)) {
                continue;
            }

            if (!RawFieldCandidateMatches(fields, fieldCount, fieldName, requiredFields, target)) {
                continue;
            }

            acceptCandidate(layoutName, pointerOffset);
            break;
        }
    }

    if (schema::IsUniqueRawSchemaCandidateCount(matchCount)) {
        target = resolvedTarget;
        TracePrintf(
            "schema-raw field=%s traceClass=%s classInfo=%p layout=%s fields=%u matches=%u wrapperOffset=0x%llX",
            fieldName,
            traceClassName != nullptr ? traceClassName : "<unknown>",
            resolvedPayload,
            resolvedLayoutName != nullptr ? resolvedLayoutName : "unknown",
            static_cast<unsigned>(resolvedFieldCount),
            static_cast<unsigned>(matchCount),
            static_cast<unsigned long long>(resolvedPointerOffset)
        );
        return true;
    }

    if (matchCount > 1u) {
        TracePrintf(
            "schema-raw ambiguous traceClass=%s field=%s matches=%u requiredCount=%u",
            traceClassName != nullptr ? traceClassName : "<unknown>",
            fieldName != nullptr ? fieldName : "<null>",
            static_cast<unsigned>(matchCount),
            static_cast<unsigned>(requiredFields.size())
        );
        return false;
    }

    if (TryGetSchemaFieldOffsetFromGenericRawPayloads(payloads, traceClassName, fieldName, requiredFields, target)) {
        return true;
    }

    if (schema::TryGetKnownClientSchemaFieldOffsetFallback(traceClassName, fieldName, target)) {
        TracePrintf(
            "schema-raw field=%s traceClass=%s layout=known-cs2-client-fallback target=0x%llX reason=schema-field-omitted-or-unresolved",
            fieldName,
            traceClassName,
            static_cast<unsigned long long>(target)
        );
        return true;
    }

    TracePrintf(
        "schema-raw missing traceClass=%s field=%s requiredCount=%u",
        traceClassName != nullptr ? traceClassName : "<unknown>",
        fieldName != nullptr ? fieldName : "<null>",
        static_cast<unsigned>(requiredFields.size())
    );
    return false;
}

template <typename T>
bool TryReadOffsetValue(const void* base, std::ptrdiff_t offset, T& value) {
    if (base == nullptr || offset == 0) {
        value = T{};
        return false;
    }

    const auto* address = reinterpret_cast<const T*>(reinterpret_cast<const std::uint8_t*>(base) + offset);
    return TryReadMemoryValue(address, value);
}

template <typename T>
bool TryWriteOffsetValue(void* base, std::ptrdiff_t offset, const T& value) {
    if (base == nullptr || offset == 0) {
        return false;
    }

    auto* address = reinterpret_cast<T*>(reinterpret_cast<std::uint8_t*>(base) + offset);
    __try {
        *address = value;
        return true;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return false;
    }
}

bool TryReadOffsetPointer(const void* base, std::ptrdiff_t offset, void*& value) {
    value = nullptr;
    return TryReadOffsetValue(base, offset, value);
}

bool TryReadOffsetHandle(const void* base, std::ptrdiff_t offset, EntityHandle& handle) {
    std::uint32_t rawHandle = kInvalidHandleValue;
    if (!TryReadOffsetValue(base, offset, rawHandle)) {
        handle = {};
        return false;
    }

    handle = EntityHandle{ rawHandle };
    return true;
}

bool ResolveSchemaOffsetsNow(std::string& failure) {
    failure.clear();

    void* rawSchemaSystem = nullptr;
    std::string rawSchemaFailure;
    bool rawScopeFound = false;
    bool rawPayloadsCollected = false;
    if (TryResolveSchemaSystemFromPattern(rawSchemaSystem, rawSchemaFailure) && rawSchemaSystem != nullptr) {
        void* rawScope = nullptr;
        if (TryResolveClientSchemaTypeScopeRaw(rawSchemaSystem, rawScope) && rawScope != nullptr) {
            rawScopeFound = true;
            std::vector<void*> rawPayloads;
            if (TryCollectTypeScopeClassPayloads(rawScope, rawPayloads)) {
                rawPayloadsCollected = true;
                TraceSchemaRawPayloadDiagnostics(rawPayloads);
                ClientOffsets rawResolvedOffsets{};
                bool rawOk = true;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CEntityInstance",
                    "m_pEntity",
                    { "m_pEntity" },
                    rawResolvedOffsets.entityInstanceToIdentity
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CBasePlayerController",
                    "m_hPawn",
                    { "m_hPawn" },
                    rawResolvedOffsets.controllerPawnHandle
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BasePlayerPawn",
                    "m_pObserverServices",
                    { "m_pObserverServices", "m_pCameraServices", "m_pWeaponServices", "m_hController" },
                    rawResolvedOffsets.pawnObserverServices
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BasePlayerPawn",
                    "m_pCameraServices",
                    { "m_pObserverServices", "m_pCameraServices", "m_pWeaponServices", "m_hController" },
                    rawResolvedOffsets.pawnCameraServices
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BasePlayerPawn",
                    "m_pWeaponServices",
                    { "m_pObserverServices", "m_pCameraServices", "m_pWeaponServices", "m_hController" },
                    rawResolvedOffsets.pawnWeaponServices
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BasePlayerPawn",
                    "m_hController",
                    { "m_pObserverServices", "m_pCameraServices", "m_pWeaponServices", "m_hController" },
                    rawResolvedOffsets.pawnControllerHandle
                ) && rawOk;
                TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_CSPlayerPawn",
                    "m_angEyeAngles",
                    { "m_angEyeAngles" },
                    rawResolvedOffsets.pawnEyeAngles
                );
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CPlayer_ObserverServices",
                    "m_iObserverMode",
                    { "m_iObserverMode", "m_hObserverTarget" },
                    rawResolvedOffsets.observerMode
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CPlayer_ObserverServices",
                    "m_hObserverTarget",
                    { "m_iObserverMode", "m_hObserverTarget" },
                    rawResolvedOffsets.observerTarget
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CPlayer_CameraServices",
                    "m_hViewEntity",
                    { "m_hViewEntity" },
                    rawResolvedOffsets.cameraViewEntity
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CPlayer_WeaponServices",
                    "m_hActiveWeapon",
                    { "m_hActiveWeapon" },
                    rawResolvedOffsets.activeWeapon
                ) && rawOk;
                rawOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BaseEntity",
                    "m_pGameSceneNode",
                    { "m_pGameSceneNode" },
                    rawResolvedOffsets.sceneNode
                ) && rawOk;
                bool sceneOriginOk = TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "CGameSceneNode",
                    "m_vecAbsOrigin",
                    { "m_vecAbsOrigin" },
                    rawResolvedOffsets.sceneAbsOrigin
                );
                if (!sceneOriginOk) {
                    sceneOriginOk = TryGetSchemaFieldOffsetFromRawPayloads(
                        rawPayloads,
                        "CGameSceneNode",
                        "m_vecOrigin",
                        { "m_vecOrigin" },
                        rawResolvedOffsets.sceneAbsOrigin
                    );
                }
                if (!sceneOriginOk) {
                    sceneOriginOk = TryGetSchemaFieldOffsetFromRawPayloads(
                        rawPayloads,
                        "CGameSceneNode",
                        "local.origin",
                        { "local.origin" },
                        rawResolvedOffsets.sceneAbsOrigin
                    );
                }
                rawOk = sceneOriginOk && rawOk;
                TryGetSchemaFieldOffsetFromRawPayloads(
                    rawPayloads,
                    "C_BaseModelEntity",
                    "m_bRenderWithViewModels",
                    { "m_bRenderWithViewModels" },
                    rawResolvedOffsets.renderWithViewModels
                );

                if (rawOk) {
                    g_offsets = rawResolvedOffsets;
                    TracePrintf(
                        "schema-resolve raw-success schemaSystem=%p scope=%p payloads=%u",
                        rawSchemaSystem,
                        rawScope,
                        static_cast<unsigned>(rawPayloads.size())
                    );
                    return true;
                }

                TracePrintf(
                    "schema-resolve raw-incomplete schemaSystem=%p scope=%p payloads=%u",
                    rawSchemaSystem,
                    rawScope,
                    static_cast<unsigned>(rawPayloads.size())
                );
            }
        }
    }
    else if (!rawSchemaFailure.empty()) {
        TracePrintf("schema-resolve raw-unavailable reason=%s", rawSchemaFailure.c_str());
    }

    if (!schema::ShouldUseSchemaInterfaceFallback(rawScopeFound, rawPayloadsCollected, false)) {
        failure = Texts().reasonRequiredSchemaOffsetsMissing;
        TracePrintf(
            "schema-resolve fallback-skipped rawScope=%d rawPayloads=%d reason=raw-schema-incomplete",
            rawScopeFound ? 1 : 0,
            rawPayloadsCollected ? 1 : 0
        );
        return false;
    }

    auto schemaFactory = GetCreateInterface(g_schemaModule);
    TracePrintf("schema-resolve factory=%p", reinterpret_cast<void*>(schemaFactory));
    if (schemaFactory == nullptr) {
        failure = Texts().reasonCreateInterfaceMissing;
        return false;
    }

    void* schemaSystem = schemaFactory(kSchemaSystemVersion, nullptr);
    TracePrintf("schema-resolve interface=%p version=%s", schemaSystem, kSchemaSystemVersion);
    if (schemaSystem == nullptr) {
        failure = Texts().reasonSchemaSystemUnavailable;
        return false;
    }

    std::string qualifiedLookupFailure;
    const bool qualifiedLookupReady = ResolveSchemaQualifiedClassLookupEntry(qualifiedLookupFailure);
    if (!qualifiedLookupReady) {
        TracePrintf(
            "schema-entry pattern-miss name=qualified-class reason=%s",
            qualifiedLookupFailure.empty() ? "<unknown>" : qualifiedLookupFailure.c_str()
        );
    }

    void** vtable = nullptr;
    void* findTypeScopeAddress = nullptr;
    void* scope = nullptr;
    bool scopeReady = false;
    if (TryReadMemoryValue(reinterpret_cast<void***>(schemaSystem), vtable) && vtable != nullptr) {
        if (TryReadMemoryValue(vtable + 13, findTypeScopeAddress) && findTypeScopeAddress != nullptr) {
            TracePrintf("schema-resolve vtable=%p slot13=%p", vtable, findTypeScopeAddress);
            if (IsAddressExecutableInModule(g_schemaModule, findTypeScopeAddress)) {
                auto findTypeScopeForModule = reinterpret_cast<FindTypeScopeForModuleFn>(findTypeScopeAddress);
                if (TryInvokeFindTypeScopeForModule(findTypeScopeForModule, schemaSystem, kClientSchemaScopeName, scope) && scope != nullptr) {
                    scopeReady = true;
                    TracePrintf(
                        "schema-resolve scope=%p traceVersion=%s cachedSlot=%d cachedMode=%s probeSlots=%u",
                        scope,
                        kSchemaProbeTraceVersion,
                        g_typeScopeClassLookupVtableIndex,
                        TypeScopeClassLookupModeToString(g_typeScopeClassLookupMode),
                        static_cast<unsigned>(schema::DetermineTypeScopeLookupProbeSlotLimit())
                    );
                }
                else {
                    TracePrintf("schema-resolve scope-fallback faulted slot13=%p", findTypeScopeAddress);
                }
            }
            else {
                TracePrintf("schema-resolve scope-fallback untrusted slot13=%p", findTypeScopeAddress);
            }
        }
        else {
            TracePrintf("schema-resolve scope-fallback missing-slot13 vtable=%p", vtable);
        }
    }
    else {
        TracePrintf("schema-resolve scope-fallback unreadable-vtable interface=%p", schemaSystem);
    }

    if (!qualifiedLookupReady && !scopeReady) {
        failure = !qualifiedLookupFailure.empty()
            ? qualifiedLookupFailure
            : FormatIncompatibleBuildReason("schema class lookup entry missing");
        return false;
    }

    ClientOffsets resolvedOffsets{};
    auto findClassInfo = [schemaSystem, scope, scopeReady, qualifiedLookupReady](const char* className, void*& classInfo) -> bool {
        classInfo = nullptr;

        std::array<std::string, 2> lookupCandidates;
        const auto lookupCandidateCount = schema::BuildSchemaClassLookupCandidates(
            kClientSchemaScopeName,
            className,
            lookupCandidates
        );
        if (qualifiedLookupReady) {
            for (std::size_t candidateIndex = 0; candidateIndex < lookupCandidateCount; ++candidateIndex) {
                const auto& lookupName = lookupCandidates[candidateIndex];
                if (lookupName.empty()) {
                    continue;
                }

                if (TryInvokeFindDeclaredClassByQualifiedName(
                    g_findDeclaredClassByQualifiedName,
                    schemaSystem,
                    lookupName.c_str(),
                    classInfo
                ) && classInfo != nullptr) {
                    TracePrintf(
                        "schema-qualified class=%s query=%s classInfo=%p entry=%p candidate=%u",
                        className,
                        lookupName.c_str(),
                        classInfo,
                        reinterpret_cast<void*>(g_schemaQualifiedClassLookupAddress),
                        static_cast<unsigned>(candidateIndex)
                    );
                    return true;
                }

                TracePrintf(
                    "schema-qualified miss class=%s query=%s classInfo=%p entry=%p candidate=%u",
                    className,
                    lookupName.c_str(),
                    classInfo,
                    reinterpret_cast<void*>(g_schemaQualifiedClassLookupAddress),
                    static_cast<unsigned>(candidateIndex)
                );
            }
        }

        if (scopeReady && TryInvokeFindDeclaredClassInTypeScope(scope, className, classInfo) && classInfo != nullptr) {
            TracePrintf(
                "schema-class fallback class=%s classInfo=%p scope=%p slot=%d mode=%s",
                className,
                classInfo,
                scope,
                g_typeScopeClassLookupVtableIndex,
                TypeScopeClassLookupModeToString(g_typeScopeClassLookupMode)
            );
            return true;
        }

        TracePrintf("schema-class missing class=%s", className);
        return false;
    };

    auto getOffset = [&findClassInfo](const char* className, const char* fieldName, std::ptrdiff_t& target) -> bool {
        void* classInfo = nullptr;
        if (!findClassInfo(className, classInfo) || classInfo == nullptr) {
            return false;
        }

        SchemaFieldModern* fields = nullptr;
        std::uint32_t fieldCount = 0u;
        const char* layoutName = nullptr;
        if (!TryReadSchemaFieldSpan(classInfo, fields, fieldCount, layoutName)) {
            TracePrintf("schema-class unreadable class=%s classInfo=%p", className, classInfo);
            return false;
        }

        TracePrintf(
            "schema-class class=%s classInfo=%p layout=%s fields=%u fieldData=%p",
            className,
            classInfo,
            layoutName,
            static_cast<unsigned>(fieldCount),
            fields
        );

        for (std::uint32_t fieldIndex = 0; fieldIndex < fieldCount; ++fieldIndex) {
            SchemaFieldModern field{};
            if (!TryReadMemoryValue(fields + fieldIndex, field) || field.name == nullptr) {
                continue;
            }

            std::string resolvedFieldName;
            if (!TryCopyCString(field.name, resolvedFieldName) || resolvedFieldName.empty()) {
                continue;
            }

            if (resolvedFieldName != fieldName) {
                continue;
            }

            target = static_cast<std::ptrdiff_t>(field.offset);
            TracePrintf(
                "schema-field class=%s field=%s offset=0x%llX layout=%s",
                className,
                fieldName,
                static_cast<unsigned long long>(static_cast<std::uint64_t>(field.offset)),
                layoutName
            );
            return true;
        }

        TracePrintf("schema-field missing class=%s field=%s", className, fieldName);
        return false;
    };

    bool ok = true;
    ok = ok && getOffset("CEntityInstance", "m_pEntity", resolvedOffsets.entityInstanceToIdentity);
    ok = ok && getOffset("CBasePlayerController", "m_hPawn", resolvedOffsets.controllerPawnHandle);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pObserverServices", resolvedOffsets.pawnObserverServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pCameraServices", resolvedOffsets.pawnCameraServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pWeaponServices", resolvedOffsets.pawnWeaponServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_hController", resolvedOffsets.pawnControllerHandle);
    getOffset("C_CSPlayerPawn", "m_angEyeAngles", resolvedOffsets.pawnEyeAngles);
    ok = ok && getOffset("CPlayer_ObserverServices", "m_iObserverMode", resolvedOffsets.observerMode);
    ok = ok && getOffset("CPlayer_ObserverServices", "m_hObserverTarget", resolvedOffsets.observerTarget);
    ok = ok && getOffset("CPlayer_CameraServices", "m_hViewEntity", resolvedOffsets.cameraViewEntity);
    ok = ok && getOffset("CPlayer_WeaponServices", "m_hActiveWeapon", resolvedOffsets.activeWeapon);
    ok = ok && getOffset("C_BaseEntity", "m_pGameSceneNode", resolvedOffsets.sceneNode);
    bool sceneOriginOk = getOffset("CGameSceneNode", "m_vecAbsOrigin", resolvedOffsets.sceneAbsOrigin);
    if (!sceneOriginOk) {
        sceneOriginOk = getOffset("CGameSceneNode", "m_vecOrigin", resolvedOffsets.sceneAbsOrigin);
    }
    if (!sceneOriginOk) {
        sceneOriginOk = getOffset("CGameSceneNode", "local.origin", resolvedOffsets.sceneAbsOrigin);
    }
    ok = ok && sceneOriginOk;
    getOffset("C_BaseModelEntity", "m_bRenderWithViewModels", resolvedOffsets.renderWithViewModels);

    if (!ok) {
        failure = Texts().reasonRequiredSchemaOffsetsMissing;
        return false;
    }

    g_offsets = resolvedOffsets;
    return true;
}

DWORD WINAPI SchemaResolveWorkerThreadMain(LPVOID) {
    TracePrintf("schema-async worker enter");

    std::string resolveFailure;
    if (!ResolveSchemaOffsetsNow(resolveFailure)) {
        g_runtimeCompatibility.schemaOffsetsReady = false;
        {
            std::lock_guard lock(g_schemaResolveMutex);
            g_schemaResolveFailureReason = resolveFailure;
        }
        TracePrintf(
            "schema-async worker failed reason=%s",
            resolveFailure.empty() ? "<unknown>" : resolveFailure.c_str()
        );
        g_schemaResolveInProgress.store(false, std::memory_order_release);
        return 0;
    }

    g_runtimeCompatibility.schemaOffsetsReady = true;
    {
        std::lock_guard lock(g_schemaResolveMutex);
        g_schemaResolveFailureReason.clear();
    }
    g_schemaOffsetsResolved.store(true, std::memory_order_release);
    TracePrintf("schema-async worker ok");
    g_schemaResolveInProgress.store(false, std::memory_order_release);
    return 0;
}

bool StartBackgroundSchemaResolve(const char* caller, bool traceAttempt) {
    if (g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
        return true;
    }

    const unsigned long long now = GetTickCount64();
    bool shouldStart = false;
    {
        std::lock_guard lock(g_schemaResolveMutex);
        if (g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
            return true;
        }

        const bool inProgress = g_schemaResolveInProgress.load(std::memory_order_acquire);
        const bool hasRecentFailure = !g_schemaResolveFailureReason.empty();
        const unsigned long long lastAttempt = g_lastSchemaResolveAttemptTickMs.load(std::memory_order_relaxed);
        const unsigned long long millisSinceLastAttempt =
            (lastAttempt != 0ull && now >= lastAttempt) ? (now - lastAttempt) : kSchemaResolveRetryIntervalMs;

        shouldStart = schema::ShouldStartBackgroundSchemaResolve(
            false,
            inProgress,
            hasRecentFailure,
            millisSinceLastAttempt,
            kSchemaResolveRetryIntervalMs
        );
        if (!shouldStart) {
            return inProgress || !hasRecentFailure;
        }

        g_schemaResolveInProgress.store(true, std::memory_order_release);
        g_lastSchemaResolveAttemptTickMs.store(now, std::memory_order_relaxed);
        g_schemaResolveFailureReason.clear();
    }

    HANDLE thread = CreateThread(nullptr, 0, &SchemaResolveWorkerThreadMain, nullptr, 0, nullptr);
    if (thread == nullptr) {
        g_schemaResolveInProgress.store(false, std::memory_order_release);
        {
            std::lock_guard lock(g_schemaResolveMutex);
            g_schemaResolveFailureReason = FormatIncompatibleBuildReason("schema resolve worker creation failed");
        }
        return false;
    }

    CloseHandle(thread);
    if (traceAttempt) {
        TracePrintf("schema-async caller=%s started", caller != nullptr ? caller : "<null>");
    }
    return true;
}

bool WaitForStartupSchemaResolve(const char* caller) {
    const unsigned long long start = GetTickCount64();
    while (!g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
        if (!g_schemaResolveInProgress.load(std::memory_order_acquire)) {
            std::string failure;
            {
                std::lock_guard lock(g_schemaResolveMutex);
                failure = g_schemaResolveFailureReason;
            }
            TracePrintf(
                "schema-wait caller=%s result=not-ready elapsed=%llu reason=%s",
                caller != nullptr ? caller : "<unknown>",
                static_cast<unsigned long long>(GetTickCount64() - start),
                failure.empty() ? "<none>" : failure.c_str()
            );
            return false;
        }

        const unsigned long long now = GetTickCount64();
        if (now - start >= kSchemaResolveStartupWaitTimeoutMs) {
            TracePrintf(
                "schema-wait caller=%s result=timeout elapsed=%llu",
                caller != nullptr ? caller : "<unknown>",
                static_cast<unsigned long long>(now - start)
            );
            return false;
        }

        Sleep(kSchemaResolveStartupWaitSleepMs);
    }

    TracePrintf(
        "schema-wait caller=%s result=ready elapsed=%llu",
        caller != nullptr ? caller : "<unknown>",
        static_cast<unsigned long long>(GetTickCount64() - start)
    );
    return true;
}

bool EnsureSchemaOffsetsResolved(const char* caller, bool traceAttempt, std::string& failure) {
    failure.clear();
    if (g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
        return true;
    }

    const bool startIssued = StartBackgroundSchemaResolve(caller, traceAttempt);
    if (g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
        return true;
    }

    {
        std::lock_guard lock(g_schemaResolveMutex);
        if (g_schemaOffsetsResolved.load(std::memory_order_acquire)) {
            return true;
        }

        if (!g_schemaResolveFailureReason.empty()) {
            failure = g_schemaResolveFailureReason;
        }
        else if (g_schemaResolveInProgress.load(std::memory_order_acquire) || startIssued) {
            failure = FormatIncompatibleBuildReason("schema offsets still resolving");
        }
        else {
            failure = FormatIncompatibleBuildReason("schema offsets unavailable");
        }
    }

    if (traceAttempt) {
        TracePrintf(
            "schema-lazy caller=%s pending inProgress=%d reason=%s",
            caller != nullptr ? caller : "<null>",
            g_schemaResolveInProgress.load(std::memory_order_acquire) ? 1 : 0,
            failure.empty() ? "<none>" : failure.c_str()
        );
    }
    return false;
}

bool ResolveEntityAccess() {
    const auto entityListPattern = FindPattern(g_clientModule, kPatternEntityList);
    if (entityListPattern == 0) {
        SetInitialized(false, Texts().reasonEntityListPatternMissing);
        return false;
    }

    g_entityListStorage = reinterpret_cast<void**>(entityListPattern + 18 + 7 + *reinterpret_cast<std::int32_t*>(entityListPattern + 18 + 3));
    g_getHighestEntityIndex = reinterpret_cast<GetHighestEntityIndexFn>(entityListPattern + 27 + 5 + *reinterpret_cast<std::int32_t*>(entityListPattern + 27 + 1));

    const auto getEntityFromIndex = FindPattern(g_clientModule, kPatternGetEntityFromIndex);
    if (getEntityFromIndex == 0) {
        SetInitialized(false, Texts().reasonGetEntityFromIndexPatternMissing);
        return false;
    }

    g_getEntityFromIndex = reinterpret_cast<GetEntityFromIndexFn>(getEntityFromIndex);
    if (!IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(g_getEntityFromIndex))) {
        SetInitialized(false, FormatIncompatibleBuildReason("GetEntityFromIndex pointer is outside client.dll"));
        return false;
    }

    const auto splitScreenPlayers = FindPatterns(g_clientModule, kPatternGetSplitScreenPlayer);
    if (splitScreenPlayers.empty()) {
        SetInitialized(false, Texts().reasonGetSplitScreenPlayerPatternMissing);
        return false;
    }

    g_getSplitScreenPlayerCandidates.clear();
    for (const auto splitScreenPlayer : splitScreenPlayers) {
        auto candidate = reinterpret_cast<GetSplitScreenPlayerFn>(splitScreenPlayer);
        if (IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(candidate))) {
            g_getSplitScreenPlayerCandidates.push_back(candidate);
        }
    }
    if (g_getSplitScreenPlayerCandidates.empty()) {
        SetInitialized(false, FormatIncompatibleBuildReason("GetSplitScreenPlayer candidates are not trusted"));
        return false;
    }
    g_getSplitScreenPlayer = g_getSplitScreenPlayerCandidates.front();
    if (g_getSplitScreenPlayerCandidates.size() > 1) {
        ConsolePrintf(Texts().logSplitScreenMatches, g_getSplitScreenPlayerCandidates.size());
    }

    g_lookupAttachment = nullptr;
    g_getAttachment = nullptr;
    const auto attachmentPattern = FindPattern(g_clientModule, kPatternAttachmentAccess);
    if (attachmentPattern == 0) {
        TracePrintf("attachment-pattern missing, enabling eye-angle fallback");
    }
    else {
        auto lookupAttachment = reinterpret_cast<LookupAttachmentFn>(attachmentPattern + 5 + *reinterpret_cast<std::int32_t*>(attachmentPattern + 1));
        auto getAttachment = reinterpret_cast<GetAttachmentFn>(attachmentPattern + 40 + 5 + *reinterpret_cast<std::int32_t*>(attachmentPattern + 40 + 1));
        if (!IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(lookupAttachment))
            || !IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(getAttachment))) {
            TracePrintf("attachment-accessors untrusted, enabling eye-angle fallback");
        }
        else {
            g_lookupAttachment = lookupAttachment;
            g_getAttachment = getAttachment;
        }
    }

    if (!IsAddressReadableInModule(g_clientModule, g_entityListStorage)) {
        SetInitialized(false, FormatIncompatibleBuildReason("entity list storage is outside client.dll"));
        return false;
    }

    g_runtimeCompatibility.entityAccessReady = g_entityListStorage != nullptr
        && g_getEntityFromIndex != nullptr
        && g_getHighestEntityIndex != nullptr;
    g_runtimeCompatibility.schemaOffsetsReady = g_schemaOffsetsResolved.load(std::memory_order_acquire);
    g_runtimeCompatibility.splitScreenAccessorReady = g_getSplitScreenPlayer != nullptr;

    return g_runtimeCompatibility.entityAccessReady && g_runtimeCompatibility.splitScreenAccessorReady;
}

bool EntityIsPlayerPawn(void* entity) {
    return compat::ClassifyEntityClassName(TryGetEntityRttiClassName(entity)) == compat::ClientEntityKind::PlayerPawn;
}

bool EntityIsPlayerController(void* entity) {
    return compat::ClassifyEntityClassName(TryGetEntityRttiClassName(entity)) == compat::ClientEntityKind::PlayerController;
}

const char* ClientEntityKindToTraceString(compat::ClientEntityKind kind) {
    switch (kind) {
    case compat::ClientEntityKind::PlayerPawn:
        return "pawn";
    case compat::ClientEntityKind::PlayerController:
        return "controller";
    default:
        return "unknown";
    }
}

bool TryEntityGetRenderEyeOrigin(void* entity, float output[3], std::string& failure) {
    output[0] = 0.0f;
    output[1] = 0.0f;
    output[2] = 0.0f;
    if (entity == nullptr) {
        failure = Texts().reasonInvalidEyeOrigin;
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(entity);
    if (vtable == nullptr || !IsAddressExecutableInModule(g_clientModule, vtable[166])) {
        failure = FormatIncompatibleBuildReason("RenderEyeOrigin vtable slot changed");
        TracePrintf("eye-origin-slot-invalid entity=%p slot=%p", entity, vtable == nullptr ? nullptr : vtable[166]);
        return false;
    }

    if (!TryInvokeRenderEyeOrigin(entity, vtable[166], output)) {
        failure = FormatIncompatibleBuildReason("RenderEyeOrigin call faulted");
        TracePrintf("eye-origin-call-fault entity=%p slot=%p", entity, vtable[166]);
        return false;
    }
    if (!IsFiniteVec3(Vec3{ output[0], output[1], output[2] })) {
        failure = Texts().reasonInvalidEyeOrigin;
        return false;
    }

    return true;
}

bool TryGetEntityEyeOrigin(void* entity, const Vec3* viewSetupEyeOrigin, float output[3], std::string& failure) {
    if (TryEntityGetRenderEyeOrigin(entity, output, failure)) {
        return true;
    }

    const std::string renderFailure = failure;
    Vec3 sceneOrigin{};
    std::string sceneFailure;
    const bool sceneOriginAvailable = TryGetEntityAbsOrigin(entity, sceneOrigin, sceneFailure);
    if (compat::ShouldUseSceneOriginEyeFallback(false, sceneOriginAvailable)) {
        output[0] = sceneOrigin.x;
        output[1] = sceneOrigin.y;
        output[2] = sceneOrigin.z + kFallbackEyeOriginHeight;
        if (!IsFiniteVec3(Vec3{ output[0], output[1], output[2] })) {
            failure = Texts().reasonInvalidEyeOrigin;
            return false;
        }

        TracePrintf(
            "eye-origin-fallback entity=%p renderFailure=%s scene=(%.3f,%.3f,%.3f) eye=(%.3f,%.3f,%.3f)",
            entity,
            renderFailure.c_str(),
            sceneOrigin.x,
            sceneOrigin.y,
            sceneOrigin.z,
            output[0],
            output[1],
            output[2]
        );
        failure.clear();
        return true;
    }

    if (!sceneFailure.empty()) {
        TracePrintf("eye-origin-scene-failed entity=%p renderFailure=%s sceneFailure=%s", entity, renderFailure.c_str(), sceneFailure.c_str());
    }

    const bool viewSetupOriginAvailable = viewSetupEyeOrigin != nullptr && IsFiniteVec3(*viewSetupEyeOrigin);
    if (!compat::ShouldUseViewSetupEyeFallback(false, sceneOriginAvailable, viewSetupOriginAvailable)) {
        return false;
    }

    output[0] = viewSetupEyeOrigin->x;
    output[1] = viewSetupEyeOrigin->y;
    output[2] = viewSetupEyeOrigin->z;
    if (!IsFiniteVec3(Vec3{ output[0], output[1], output[2] })) {
        failure = Texts().reasonInvalidEyeOrigin;
        return false;
    }

    TracePrintf(
        "eye-origin-viewsetup-fallback entity=%p renderFailure=%s sceneFailure=%s eye=(%.3f,%.3f,%.3f)",
        entity,
        renderFailure.c_str(),
        sceneFailure.c_str(),
        output[0],
        output[1],
        output[2]
    );
    failure.clear();
    return true;
}

EntityHandle EntityGetHandle(void* entity) {
    if (entity == nullptr) {
        return {};
    }

    void* identityVoid = nullptr;
    if (!TryReadOffsetPointer(entity, g_offsets.entityInstanceToIdentity, identityVoid)) {
        return {};
    }

    auto* identity = reinterpret_cast<std::uint8_t*>(identityVoid);
    if (identity == nullptr) {
        return {};
    }

    EntityHandle handle{};
    if (!TryReadOffsetHandle(identity, 0x10, handle)) {
        return {};
    }

    return handle;
}

EntityHandle ControllerGetPawnHandle(void* controller) {
    if (controller == nullptr || !EntityIsPlayerController(controller)) {
        return {};
    }

    EntityHandle handle{};
    if (!TryReadOffsetHandle(controller, g_offsets.controllerPawnHandle, handle)) {
        return {};
    }

    return handle;
}

EntityHandle ControllerGetPawnHandleAtOffset(void* controller, std::ptrdiff_t offset) {
    if (controller == nullptr || !EntityIsPlayerController(controller)) {
        return {};
    }

    EntityHandle handle{};
    if (!TryReadOffsetHandle(controller, offset, handle)) {
        return {};
    }

    return handle;
}

bool TryGetEntityAbsOrigin(void* entity, Vec3& origin, std::string& failure) {
    static std::atomic<unsigned long long> traceCounter{0};
    const auto shouldTraceFailure = [&]() {
        const unsigned long long traceIndex = traceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
        return traceIndex <= 20u || 0u == (traceIndex % 60u);
    };

    if (entity == nullptr) {
        failure = Texts().reasonWeaponEntityMissing;
        if (shouldTraceFailure()) {
            TracePrintf("abs-origin-failed entity=%p reason=null-entity", entity);
        }
        return false;
    }

    void* sceneNode = nullptr;
    if (!TryReadOffsetPointer(entity, g_offsets.sceneNode, sceneNode)) {
        failure = FormatIncompatibleBuildReason("scene node read faulted");
        if (shouldTraceFailure()) {
            TracePrintf("abs-origin-failed entity=%p sceneNodeOffset=0x%llX reason=scene-node-read-fault", entity, static_cast<unsigned long long>(g_offsets.sceneNode));
        }
        return false;
    }

    if (sceneNode == nullptr) {
        failure = Texts().reasonWeaponSceneNodeMissing;
        if (shouldTraceFailure()) {
            TracePrintf("abs-origin-failed entity=%p sceneNodeOffset=0x%llX sceneNode=%p reason=scene-node-null", entity, static_cast<unsigned long long>(g_offsets.sceneNode), sceneNode);
        }
        return false;
    }

    auto* originBase = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(sceneNode) + g_offsets.sceneAbsOrigin);
    const bool readX = TryReadFloatPointer(originBase + 0, origin.x);
    const bool readY = TryReadFloatPointer(originBase + 1, origin.y);
    const bool readZ = TryReadFloatPointer(originBase + 2, origin.z);
    if (!readX || !readY || !readZ || !IsFiniteVec3(origin)) {
        failure = Texts().reasonWeaponOriginInvalid;
        if (shouldTraceFailure()) {
            TracePrintf(
                "abs-origin-failed entity=%p sceneNodeOffset=0x%llX sceneNode=%p originOffset=0x%llX read=(%d,%d,%d) origin=(%.3f,%.3f,%.3f) reason=origin-invalid",
                entity,
                static_cast<unsigned long long>(g_offsets.sceneNode),
                sceneNode,
                static_cast<unsigned long long>(g_offsets.sceneAbsOrigin),
                readX ? 1 : 0,
                readY ? 1 : 0,
                readZ ? 1 : 0,
                origin.x,
                origin.y,
                origin.z
            );
        }
        return false;
    }

    return true;
}

bool TryGetPawnEyeAngles(void* pawn, Vec3& angles, std::string& failure) {
    angles = {};
    if (pawn == nullptr || !EntityIsPlayerPawn(pawn) || g_offsets.pawnEyeAngles == 0) {
        failure = FormatIncompatibleBuildReason("eye angles offset unavailable");
        return false;
    }

    auto* eyeAngles = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(pawn) + g_offsets.pawnEyeAngles);
    if (!TryReadFloatPointer(eyeAngles + 0, angles.x)
        || !TryReadFloatPointer(eyeAngles + 1, angles.y)
        || !TryReadFloatPointer(eyeAngles + 2, angles.z)
        || !IsFiniteVec3(angles)) {
        failure = FormatIncompatibleBuildReason("eye angles read faulted");
        return false;
    }

    return true;
}

EntityHandle PawnGetActiveWeaponHandle(void* pawn) {
    if (pawn == nullptr || !EntityIsPlayerPawn(pawn)) {
        return {};
    }

    void* weaponServices = nullptr;
    if (!TryReadOffsetPointer(pawn, g_offsets.pawnWeaponServices, weaponServices)) {
        return {};
    }

    if (weaponServices == nullptr) {
        return {};
    }

    EntityHandle handle{};
    if (!TryReadOffsetHandle(weaponServices, g_offsets.activeWeapon, handle)) {
        return {};
    }

    return handle;
}

std::string TryGetEntityRttiClassName(void* entity) {
    if (entity == nullptr) {
        return {};
    }

    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(entity, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return {};
    }

    auto** vtable = *reinterpret_cast<void***>(entity);
    if (vtable == nullptr) {
        return {};
    }

    auto* locator = reinterpret_cast<RttiCompleteObjectLocator*>(vtable[-1]);
    if (locator == nullptr) {
        return {};
    }

    if (VirtualQuery(locator, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return {};
    }

    const auto moduleBase = reinterpret_cast<std::uintptr_t>(locator) - static_cast<std::uintptr_t>(locator->selfOffset);
    if (moduleBase == 0 || locator->typeDescriptorOffset <= 0) {
        return {};
    }

    auto* typeDescriptor = reinterpret_cast<RttiTypeDescriptor*>(moduleBase + static_cast<std::uintptr_t>(locator->typeDescriptorOffset));
    if (typeDescriptor == nullptr) {
        return {};
    }

    if (VirtualQuery(typeDescriptor, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return {};
    }

    const char* rawName = typeDescriptor->name;
    if (rawName == nullptr) {
        return {};
    }

    if (std::strncmp(rawName, ".?AV", 4) == 0) {
        rawName += 4;
    }

    std::string className(rawName);
    const std::size_t terminator = className.find("@@");
    if (terminator != std::string::npos) {
        className.erase(terminator);
    }

    return className;
}

std::string DescribeWeaponEntity(void* weaponEntity) {
    const std::string className = TryGetEntityRttiClassName(weaponEntity);
    if (!className.empty()) {
        return className;
    }

    const EntityHandle handle = EntityGetHandle(weaponEntity);
    if (handle.IsValid()) {
        return "handle=" + std::to_string(handle.raw);
    }

    return "unknown";
}

SupportedWeaponId TryGetSupportedWeaponId(void* weaponEntity) {
    const std::string className = TryGetEntityRttiClassName(weaponEntity);
    if (className.empty()) {
        return SupportedWeaponId::None;
    }

    for (const auto& spec : kSupportedWeaponSpecs) {
        if (className == spec.rttiClassName) {
            return spec.id;
        }
    }

    return SupportedWeaponId::None;
}

bool ContainsIgnoreCaseAscii(std::string_view haystack, std::string_view needle) {
    if (needle.empty() || haystack.size() < needle.size()) {
        return false;
    }

    auto toLower = [](unsigned char value) -> unsigned char {
        if (value >= 'A' && value <= 'Z') {
            return static_cast<unsigned char>(value - 'A' + 'a');
        }
        return value;
    };

    for (std::size_t start = 0; start + needle.size() <= haystack.size(); ++start) {
        bool matched = true;
        for (std::size_t index = 0; index < needle.size(); ++index) {
            if (toLower(static_cast<unsigned char>(haystack[start + index])) != toLower(static_cast<unsigned char>(needle[index]))) {
                matched = false;
                break;
            }
        }

        if (matched) {
            return true;
        }
    }

    return false;
}

HeldItemKind ClassifyHeldItemEntity(void* weaponEntity) {
    const std::string className = TryGetEntityRttiClassName(weaponEntity);
    if (className.empty()) {
        return HeldItemKind::Unknown;
    }

    if (ContainsIgnoreCaseAscii(className, "knife")
        || ContainsIgnoreCaseAscii(className, "grenade")
        || ContainsIgnoreCaseAscii(className, "flashbang")
        || ContainsIgnoreCaseAscii(className, "smoke")
        || ContainsIgnoreCaseAscii(className, "molotov")
        || ContainsIgnoreCaseAscii(className, "incendiary")
        || ContainsIgnoreCaseAscii(className, "decoy")
        || ContainsIgnoreCaseAscii(className, "c4")) {
        return HeldItemKind::KnifeOrUtility;
    }

    return HeldItemKind::Gun;
}

EntityHandle PawnGetObserverTarget(void* pawn) {
    if (pawn == nullptr || !EntityIsPlayerPawn(pawn)) {
        return {};
    }
    void* observerServices = nullptr;
    if (!TryReadOffsetPointer(pawn, g_offsets.pawnObserverServices, observerServices)) {
        return {};
    }

    if (observerServices == nullptr) {
        return {};
    }

    EntityHandle handle{};
    if (!TryReadOffsetHandle(observerServices, g_offsets.observerTarget, handle)) {
        return {};
    }

    return handle;
}

bool TryPawnGetObserverMode(void* pawn, int& observerMode) {
    observerMode = -1;
    if (pawn == nullptr || !EntityIsPlayerPawn(pawn)) {
        return false;
    }

    void* observerServices = nullptr;
    if (!TryReadOffsetPointer(pawn, g_offsets.pawnObserverServices, observerServices)) {
        return false;
    }

    if (observerServices == nullptr) {
        return false;
    }

    return TryReadOffsetValue(observerServices, g_offsets.observerMode, observerMode);
}

void* ResolveHandleToEntity(EntityHandle handle) {
    if (!handle.IsValid() || g_entityListStorage == nullptr || *g_entityListStorage == nullptr || g_getEntityFromIndex == nullptr) {
        return nullptr;
    }

    void* entity = nullptr;
    if (!TryInvokeGetEntityFromIndex(*g_entityListStorage, handle.Index(), entity)) {
        return nullptr;
    }

    return entity;
}

void RestoreSuppressedRenderWithViewModels() {
    std::lock_guard lock(g_viewModelSuppressionMutex);
    if (!g_viewModelSuppression.active) {
        return;
    }

    void* weaponEntity = ResolveHandleToEntity(g_viewModelSuppression.weaponHandle);
    if (weaponEntity == nullptr) {
        weaponEntity = g_viewModelSuppression.weaponEntity;
    }

    const std::uint8_t restoreValue = g_viewModelSuppression.previousRenderWithViewModels ? 1u : 0u;
    const bool restored = TryWriteOffsetValue(weaponEntity, g_offsets.renderWithViewModels, restoreValue);
    TracePrintf(
        "viewmodel-restore weapon=%u entity=%p offset=0x%llX value=%u restored=%d",
        g_viewModelSuppression.weaponHandle.raw,
        weaponEntity,
        static_cast<unsigned long long>(g_offsets.renderWithViewModels),
        static_cast<unsigned>(restoreValue),
        restored ? 1 : 0
    );

    g_viewModelSuppression = {};
}

void UpdateRenderWithViewModelsSuppression(
    const WeaponFrameBasis& basis,
    bool overrideSucceeded,
    bool traceFrame,
    unsigned long long frameIndex
) {
    const bool shouldSuppress = compat::ShouldSuppressRenderWithViewModels(
        overrideSucceeded,
        basis.weaponHandle.IsValid(),
        g_offsets.renderWithViewModels != 0,
        basis.heldItemKind != HeldItemKind::Unknown
    );
    if (!shouldSuppress) {
        RestoreSuppressedRenderWithViewModels();
        return;
    }

    void* weaponEntity = ResolveHandleToEntity(basis.weaponHandle);
    if (weaponEntity == nullptr) {
        if (traceFrame) {
            TracePrintf("viewmodel-suppress-skip idx=%llu reason=weapon-unresolved weapon=%u", frameIndex, basis.weaponHandle.raw);
        }
        RestoreSuppressedRenderWithViewModels();
        return;
    }

    std::uint8_t currentValue = 0u;
    if (!TryReadOffsetValue(weaponEntity, g_offsets.renderWithViewModels, currentValue)) {
        if (traceFrame) {
            TracePrintf(
                "viewmodel-suppress-skip idx=%llu reason=read-failed weapon=%u entity=%p offset=0x%llX",
                frameIndex,
                basis.weaponHandle.raw,
                weaponEntity,
                static_cast<unsigned long long>(g_offsets.renderWithViewModels)
            );
        }
        RestoreSuppressedRenderWithViewModels();
        return;
    }

    bool alreadyActiveForWeapon = false;
    {
        std::lock_guard lock(g_viewModelSuppressionMutex);
        alreadyActiveForWeapon = g_viewModelSuppression.active
            && g_viewModelSuppression.weaponHandle.raw == basis.weaponHandle.raw;
    }
    if (!alreadyActiveForWeapon) {
        RestoreSuppressedRenderWithViewModels();
    }

    const std::uint8_t suppressedValue = 0u;
    if (!TryWriteOffsetValue(weaponEntity, g_offsets.renderWithViewModels, suppressedValue)) {
        if (traceFrame) {
            TracePrintf(
                "viewmodel-suppress-failed idx=%llu weapon=%u entity=%p offset=0x%llX",
                frameIndex,
                basis.weaponHandle.raw,
                weaponEntity,
                static_cast<unsigned long long>(g_offsets.renderWithViewModels)
            );
        }
        return;
    }

    if (!alreadyActiveForWeapon) {
        std::lock_guard lock(g_viewModelSuppressionMutex);
        g_viewModelSuppression.weaponHandle = basis.weaponHandle;
        g_viewModelSuppression.weaponEntity = weaponEntity;
        g_viewModelSuppression.previousRenderWithViewModels = currentValue != 0u;
        g_viewModelSuppression.active = true;
    }

    if (traceFrame) {
        TracePrintf(
            "viewmodel-suppress-ok idx=%llu weapon=%u entity=%p offset=0x%llX previous=%u",
            frameIndex,
            basis.weaponHandle.raw,
            weaponEntity,
            static_cast<unsigned long long>(g_offsets.renderWithViewModels),
            static_cast<unsigned>(currentValue)
        );
    }
}

void* ResolveControllerPawnEntity(void* controller) {
    const EntityHandle primaryHandle = ControllerGetPawnHandle(controller);
    void* primaryEntity = ResolveHandleToEntity(primaryHandle);
    if (EntityIsPlayerPawn(primaryEntity)) {
        if (g_debugOptions.traceEnabled) {
            const unsigned long long fallbackIndex = g_controllerPawnFallbackTraceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
            if (fallbackIndex <= 20u || 0u == (fallbackIndex % 30u)) {
                TracePrintf(
                    "follow-controller-pawn-primary idx=%llu controller=%p offset=0x%llX handle=%u pawn=%p class=%s",
                    fallbackIndex,
                    controller,
                    static_cast<unsigned long long>(g_offsets.controllerPawnHandle),
                    primaryHandle.raw,
                    primaryEntity,
                    TryGetEntityRttiClassName(primaryEntity).c_str()
                );
            }
        }
        return primaryEntity;
    }

    std::ptrdiff_t fallbackOffset = 0;
    if (schema::TryGetKnownClientSchemaFieldOffsetFallback("CBasePlayerController", "m_hPawn", fallbackOffset)
        && fallbackOffset != 0
        && fallbackOffset != g_offsets.controllerPawnHandle) {
        const EntityHandle fallbackHandle = ControllerGetPawnHandleAtOffset(controller, fallbackOffset);
        void* fallbackEntity = ResolveHandleToEntity(fallbackHandle);
        if (EntityIsPlayerPawn(fallbackEntity)) {
            if (g_debugOptions.traceEnabled) {
                const unsigned long long fallbackIndex = g_controllerPawnFallbackTraceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
                if (fallbackIndex <= 20u || 0u == (fallbackIndex % 30u)) {
                    TracePrintf(
                        "follow-controller-pawn-fallback idx=%llu controller=%p primaryOffset=0x%llX primaryHandle=%u fallbackOffset=0x%llX fallbackHandle=%u pawn=%p",
                        fallbackIndex,
                        controller,
                        static_cast<unsigned long long>(g_offsets.controllerPawnHandle),
                        primaryHandle.raw,
                        static_cast<unsigned long long>(fallbackOffset),
                        fallbackHandle.raw,
                        fallbackEntity
                    );
                }
            }
            return fallbackEntity;
        }
    }

    if (g_debugOptions.traceEnabled) {
        const unsigned long long fallbackIndex = g_controllerPawnFallbackTraceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
        if (fallbackIndex <= 20u || 0u == (fallbackIndex % 30u)) {
            TracePrintf(
                "follow-controller-pawn-missing idx=%llu controller=%p primaryOffset=0x%llX primaryHandle=%u primaryEntity=%p primaryClass=%s",
                fallbackIndex,
                controller,
                static_cast<unsigned long long>(g_offsets.controllerPawnHandle),
                primaryHandle.raw,
                primaryEntity,
                primaryEntity != nullptr ? TryGetEntityRttiClassName(primaryEntity).c_str() : "<null>"
            );
        }
    }

    return nullptr;
}

HeldItemKind GetHeldItemKindForPawn(void* pawn) {
    const EntityHandle weaponHandle = PawnGetActiveWeaponHandle(pawn);
    if (!weaponHandle.IsValid()) {
        return HeldItemKind::Unknown;
    }

    return ClassifyHeldItemEntity(ResolveHandleToEntity(weaponHandle));
}

Vec3 GetBaseOffsetForHeldItem(HeldItemKind kind, LookMode lookMode) {
    if (kind == HeldItemKind::Gun) {
        if (lookMode == LookMode::Forward) {
            return kGunForwardBaseOffset;
        }

        return kGunSelfieBaseOffset;
    }

    return kCombatBaseOffset;
}

Vec3 GetBaseOffsetForBasis(const WeaponFrameBasis& basis, LookMode lookMode) {
    if (basis.syntheticFallbackHandAnchor && IsSelfieLookMode(lookMode)) {
        return kSyntheticFallbackSelfieBaseOffset;
    }

    return GetBaseOffsetForHeldItem(basis.heldItemKind, lookMode);
}

float GetUpperBodyTargetBlendForHeldItem(HeldItemKind kind) {
    if (kind == HeldItemKind::Gun) {
        return kGunUpperBodyTargetBlendFactor;
    }

    return kUpperBodyTargetBlendFactor;
}

Vec3 GetLookTargetForBasis(const WeaponFrameBasis& basis, LookMode lookMode) {
    if (basis.heldItemKind == HeldItemKind::Gun) {
        if (lookMode == LookMode::SelfieLeft) {
            return Add(
                Add(basis.upperBodyTarget, Scale(basis.right, -kGunSelfieTargetCrossBias)),
                Scale(basis.up, kGunSelfieTargetUpBias)
            );
        }

        if (lookMode == LookMode::SelfieRight) {
            return Add(
                Add(basis.upperBodyTarget, Scale(basis.right, kGunSelfieTargetCrossBias)),
                Scale(basis.up, kGunSelfieTargetUpBias)
            );
        }
    }

    return basis.upperBodyTarget;
}

float GetCameraRightOffsetForBasis(const WeaponFrameBasis& basis, LookMode lookMode, float magnitude) {
    if (basis.heldItemKind == HeldItemKind::Gun) {
        if (lookMode == LookMode::SelfieRight) {
            return magnitude;
        }

        if (lookMode == LookMode::SelfieLeft) {
            return -magnitude;
        }
    }

    return GetLookModeBaseRightOffset(lookMode, magnitude);
}

bool IsLikelyClientEntity(void* entity) {
    if (entity == nullptr) {
        return false;
    }

    const EntityHandle handle = EntityGetHandle(entity);
    if (!handle.IsValid()) {
        return false;
    }

    return ResolveHandleToEntity(handle) == entity;
}

void* ResolveSplitScreenPlayerEntity() {
    if (g_getSplitScreenPlayerCandidates.empty()) {
        return nullptr;
    }

    if (g_getSplitScreenPlayer != nullptr) {
        void* entity = nullptr;
        if (!TryInvokeSplitScreenPlayer(g_getSplitScreenPlayer, 0, entity)) {
            entity = nullptr;
        }
        if (entity != nullptr && (EntityIsPlayerController(entity) || EntityIsPlayerPawn(entity))) {
            return entity;
        }
    }

    for (std::size_t index = 0; index < g_getSplitScreenPlayerCandidates.size(); ++index) {
        auto candidate = g_getSplitScreenPlayerCandidates[index];
        if (candidate == nullptr) {
            continue;
        }

        void* entity = nullptr;
        if (!TryInvokeSplitScreenPlayer(candidate, 0, entity)) {
            continue;
        }
        if (!IsLikelyClientEntity(entity)) {
            continue;
        }

        if (!EntityIsPlayerController(entity) && !EntityIsPlayerPawn(entity)) {
            continue;
        }

        if (g_getSplitScreenPlayer != candidate) {
            g_getSplitScreenPlayer = candidate;
            ConsolePrintf(Texts().logSelectedCandidate, index);
        }

        return entity;
    }

    return nullptr;
}

void* ResolveEntityToPawn(void* entity) {
    if (entity == nullptr) {
        return nullptr;
    }

    if (EntityIsPlayerPawn(entity)) {
        return entity;
    }

    if (EntityIsPlayerController(entity)) {
        return ResolveControllerPawnEntity(entity);
    }

    return nullptr;
}

struct TargetResolution {
    void* entity{};
    EntityHandle handle{};
    std::string failure;
};

TargetResolution ResolveFollowTarget() {
    std::string schemaFailure;
    if (!EnsureSchemaOffsetsResolved("ResolveFollowTarget", g_debugOptions.traceEnabled, schemaFailure)) {
        return { nullptr, {}, schemaFailure };
    }

    if (g_getSplitScreenPlayerCandidates.empty()) {
        return { nullptr, {}, Texts().reasonSplitScreenPlayerAccessUnavailable };
    }

    void* splitScreenEntity = ResolveSplitScreenPlayerEntity();
    if (splitScreenEntity == nullptr) {
        return { nullptr, {}, Texts().reasonSplitScreenPlayerMissing };
    }

    compat::ClientEntityKind splitScreenKind = compat::ClientEntityKind::Unknown;
    void* observerPawn = nullptr;
    if (EntityIsPlayerController(splitScreenEntity)) {
        splitScreenKind = compat::ClientEntityKind::PlayerController;
        observerPawn = ResolveControllerPawnEntity(splitScreenEntity);
    }
    else if (EntityIsPlayerPawn(splitScreenEntity)) {
        splitScreenKind = compat::ClientEntityKind::PlayerPawn;
        observerPawn = splitScreenEntity;
    }
    else {
        return { nullptr, {}, Texts().reasonSplitScreenEntityWrongType };
    }

    if (observerPawn == nullptr) {
        return { nullptr, {}, Texts().reasonObserverPawnMissing };
    }

    const auto observerTarget = PawnGetObserverTarget(observerPawn);
    if (!observerTarget.IsValid()) {
        const EntityHandle observerPawnHandle = EntityGetHandle(observerPawn);
        int observerMode = -1;
        const bool observerModeRead = TryPawnGetObserverMode(observerPawn, observerMode);
        const bool useObserverPawn = compat::ShouldUseObserverPawnAsFollowTarget(
            splitScreenKind,
            observerTarget.IsValid(),
            observerPawn != nullptr,
            EntityIsPlayerPawn(observerPawn) && observerPawnHandle.IsValid()
        );
        if (g_debugOptions.traceEnabled) {
            const unsigned long long resolveIndex = g_followResolutionTraceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
            if (resolveIndex <= 20u || 0u == (resolveIndex % 30u)) {
                TracePrintf(
                    "follow-resolve-chain idx=%llu split=%p splitKind=%s observerPawn=%p pawnHandle=%u observerModeRead=%d observerMode=%d observerTarget=%u fallbackPawn=%d",
                    resolveIndex,
                    splitScreenEntity,
                    ClientEntityKindToTraceString(splitScreenKind),
                    observerPawn,
                    observerPawnHandle.raw,
                    observerModeRead ? 1 : 0,
                    observerMode,
                    observerTarget.raw,
                    useObserverPawn ? 1 : 0
                );
            }
        }
        if (useObserverPawn) {
            return { observerPawn, observerPawnHandle, {} };
        }
        return { nullptr, {}, Texts().reasonNoObserverTarget };
    }

    void* targetEntity = ResolveEntityToPawn(ResolveHandleToEntity(observerTarget));
    if (targetEntity == nullptr) {
        return { nullptr, {}, Texts().reasonObserverTargetUnresolved };
    }

    return { targetEntity, EntityGetHandle(targetEntity), {} };
}

TargetResolution ResolveLockedTarget(EntityHandle handle) {
    std::string schemaFailure;
    if (!EnsureSchemaOffsetsResolved("ResolveLockedTarget", g_debugOptions.traceEnabled, schemaFailure)) {
        return { nullptr, {}, schemaFailure };
    }

    if (!handle.IsValid()) {
        return { nullptr, {}, Texts().reasonLockedTargetMissing };
    }

    void* entity = ResolveEntityToPawn(ResolveHandleToEntity(handle));
    if (entity == nullptr) {
        return { nullptr, {}, Texts().reasonLockedTargetUnresolved };
    }

    return { entity, EntityGetHandle(entity), {} };
}

TargetResolution ResolveFollowSnapshotTarget(EntityHandle handle) {
    std::string schemaFailure;
    if (!EnsureSchemaOffsetsResolved("ResolveFollowSnapshotTarget", g_debugOptions.traceEnabled, schemaFailure)) {
        return { nullptr, {}, schemaFailure };
    }

    if (!handle.IsValid()) {
        return { nullptr, {}, Texts().reasonFollowTargetMissing };
    }

    void* entity = ResolveEntityToPawn(ResolveHandleToEntity(handle));
    if (entity == nullptr) {
        return { nullptr, {}, Texts().reasonFollowTargetUnresolved };
    }

    return { entity, EntityGetHandle(entity), {} };
}

bool TryGetAttachmentOrigin(void* entity, const char* attachmentName, Vec3& origin, std::string& failure) {
    if (entity == nullptr) {
        failure = Texts().reasonTargetEntityMissing;
        return false;
    }

    if (g_lookupAttachment == nullptr || g_getAttachment == nullptr) {
        failure = Texts().reasonAttachmentAccessUnavailable;
        return false;
    }

    std::uint8_t attachmentIndex{ 0xFFu };
    if (!TryInvokeLookupAttachment(entity, g_lookupAttachment, attachmentIndex, attachmentName)) {
        failure = FormatIncompatibleBuildReason("attachment lookup faulted");
        TracePrintf("attachment-lookup-fault entity=%p name=%s", entity, attachmentName);
        return false;
    }
    if (attachmentIndex == 0xFFu) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, attachmentName);
        return false;
    }

    alignas(16) float data[8]{};
    bool gotAttachment = false;
    if (!TryInvokeGetAttachment(entity, g_getAttachment, attachmentIndex, data, gotAttachment)) {
        failure = FormatIncompatibleBuildReason("attachment transform faulted");
        TracePrintf("attachment-transform-fault entity=%p name=%s index=%u", entity, attachmentName, static_cast<unsigned>(attachmentIndex));
        return false;
    }
    if (!gotAttachment) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, attachmentName);
        return false;
    }

    origin.x = data[0];
    origin.y = data[1];
    origin.z = data[2];
    return true;
}

bool TryBuildHumanFrameBasis(void* targetPawn, HeldItemKind heldItemKind, const Vec3* viewSetupEyeOrigin, WeaponFrameBasis& basis, std::string& failure) {
    Vec3 handOrigin{};
    if (!TryGetAttachmentOrigin(targetPawn, kAnchorAttachmentName, handOrigin, failure)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, kAnchorAttachmentName);
        return false;
    }

    float eyeOriginRaw[3]{};
    if (!TryGetEntityEyeOrigin(targetPawn, viewSetupEyeOrigin, eyeOriginRaw, failure)) {
        return false;
    }
    const Vec3 eyeOrigin{ eyeOriginRaw[0], eyeOriginRaw[1], eyeOriginRaw[2] };

    const Vec3 upperBodyTarget = Lerp(eyeOrigin, handOrigin, GetUpperBodyTargetBlendForHeldItem(heldItemKind));
    if (!IsFiniteVec3(upperBodyTarget)) {
        failure = Texts().reasonUpperBodyTargetInvalid;
        return false;
    }

    Vec3 right = Subtract(handOrigin, eyeOrigin);
    right = ProjectOntoPlane(right, kWorldUp);
    if (!TryNormalize(right)) {
        failure = Texts().reasonCombatRightAxisDegenerate;
        return false;
    }

    Vec3 forward = Cross(right, kWorldUp);
    if (!TryNormalize(forward)) {
        failure = Texts().reasonCombatForwardAxisDegenerate;
        return false;
    }

    Vec3 up = Cross(forward, right);
    if (!TryNormalize(up)) {
        failure = Texts().reasonCombatUpAxisDegenerate;
        return false;
    }

    basis.anchorOrigin = handOrigin;
    basis.upperBodyTarget = upperBodyTarget;
    basis.right = right;
    basis.forward = forward;
    basis.up = up;
    basis.heldItemKind = heldItemKind;
    basis.weaponHandle = {};
    basis.supportedWeaponId = SupportedWeaponId::None;
    basis.syntheticFallbackHandAnchor = false;
    return true;
}

bool TryBuildGunFrameBasis(void* targetPawn, const Vec3* viewSetupEyeOrigin, WeaponFrameBasis& basis, std::string& failure) {
    const EntityHandle targetHandle = EntityGetHandle(targetPawn);
    const EntityHandle weaponHandle = PawnGetActiveWeaponHandle(targetPawn);
    if (!weaponHandle.IsValid()) {
        failure = Texts().reasonWeaponEntityMissing;
        return false;
    }

    void* weaponEntity = ResolveHandleToEntity(weaponHandle);
    if (weaponEntity == nullptr) {
        failure = Texts().reasonWeaponEntityMissing;
        return false;
    }

    Vec3 muzzleOrigin{};
    if (!TryGetAttachmentOrigin(weaponEntity, kGunAnchorAttachmentName, muzzleOrigin, failure)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, kGunAnchorAttachmentName);
        return false;
    }

    Vec3 rightHandOrigin{};
    if (!TryGetAttachmentOrigin(targetPawn, kAnchorAttachmentName, rightHandOrigin, failure)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, kAnchorAttachmentName);
        return false;
    }

    Vec3 leftHandOrigin{};
    if (!TryGetAttachmentOrigin(targetPawn, kGunSideAttachmentName, leftHandOrigin, failure)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, kGunSideAttachmentName);
        return false;
    }

    float eyeOriginRaw[3]{};
    if (!TryGetEntityEyeOrigin(targetPawn, viewSetupEyeOrigin, eyeOriginRaw, failure)) {
        return false;
    }
    const Vec3 eyeOrigin{ eyeOriginRaw[0], eyeOriginRaw[1], eyeOriginRaw[2] };

    const Vec3 handMidpoint = Lerp(leftHandOrigin, rightHandOrigin, 0.5f);
    const Vec3 upperBodyTarget = Lerp(eyeOrigin, handMidpoint, GetUpperBodyTargetBlendForHeldItem(HeldItemKind::Gun));
    if (!IsFiniteVec3(upperBodyTarget)) {
        failure = Texts().reasonUpperBodyTargetInvalid;
        return false;
    }

    Vec3 forward = Subtract(muzzleOrigin, handMidpoint);
    forward = ProjectOntoPlane(forward, kWorldUp);
    if (!TryNormalize(forward)) {
        failure = Texts().reasonCombatForwardAxisDegenerate;
        return false;
    }

    Vec3 referenceRight = Subtract(rightHandOrigin, leftHandOrigin);
    referenceRight = ProjectOntoPlane(referenceRight, kWorldUp);
    if (!TryNormalize(referenceRight)) {
        referenceRight = Subtract(rightHandOrigin, eyeOrigin);
        referenceRight = ProjectOntoPlane(referenceRight, kWorldUp);
        if (!TryNormalize(referenceRight)) {
            failure = Texts().reasonCombatRightAxisDegenerate;
            return false;
        }
    }

    Vec3 right = ProjectOntoPlane(referenceRight, forward);
    if (!TryNormalize(right)) {
        right = Cross(kWorldUp, forward);
        if (!TryNormalize(right)) {
            failure = Texts().reasonCombatRightAxisDegenerate;
            return false;
        }

        if (Dot(right, referenceRight) < 0.0f) {
            right = Scale(right, -1.0f);
        }
    }

    Vec3 up = Cross(right, forward);
    if (!TryNormalize(up)) {
        failure = Texts().reasonCombatUpAxisDegenerate;
        return false;
    }

    basis.anchorOrigin = muzzleOrigin;
    basis.upperBodyTarget = upperBodyTarget;
    basis.right = right;
    basis.forward = forward;
    basis.up = up;
    basis.heldItemKind = HeldItemKind::Gun;
    basis.weaponHandle = weaponHandle;
    basis.supportedWeaponId = TryGetSupportedWeaponId(weaponEntity);
    basis.syntheticFallbackHandAnchor = false;
    StabilizeGunBasisRightAxis(targetHandle, basis);
    basis.anchorOrigin = StabilizeGunAnchorOrigin(targetHandle, weaponHandle, handMidpoint, basis, muzzleOrigin);
    return true;
}

Vec3 AnglesToForward(const Vec3& angles) {
    const float pitchRadians = angles.x * 3.14159265358979323846f / 180.0f;
    const float yawRadians = angles.y * 3.14159265358979323846f / 180.0f;
    const float cosPitch = std::cos(pitchRadians);
    return Vec3{
        cosPitch * std::cos(yawRadians),
        cosPitch * std::sin(yawRadians),
        -std::sin(pitchRadians)
    };
}

bool TryBuildFallbackFrameBasis(void* targetPawn, HeldItemKind heldItemKind, const Vec3* viewSetupEyeOrigin, const Vec3* viewSetupEyeAngles, WeaponFrameBasis& basis, std::string& failure) {
    float eyeOriginRaw[3]{};
    if (!TryGetEntityEyeOrigin(targetPawn, viewSetupEyeOrigin, eyeOriginRaw, failure)) {
        return false;
    }

    Vec3 eyeAngles{};
    if (!TryGetPawnEyeAngles(targetPawn, eyeAngles, failure)) {
        const std::string pawnAnglesFailure = failure;
        const bool viewSetupAnglesAvailable = viewSetupEyeAngles != nullptr && IsFiniteVec3(*viewSetupEyeAngles);
        if (!compat::ShouldUseViewSetupAnglesFallback(false, viewSetupAnglesAvailable)) {
            return false;
        }

        eyeAngles = *viewSetupEyeAngles;
        TracePrintf(
            "eye-angles-viewsetup-fallback entity=%p pawnFailure=%s angles=(%.3f,%.3f,%.3f)",
            targetPawn,
            pawnAnglesFailure.c_str(),
            eyeAngles.x,
            eyeAngles.y,
            eyeAngles.z
        );
        failure.clear();
    }

    Vec3 forward = AnglesToForward(eyeAngles);
    if (!TryNormalize(forward)) {
        failure = Texts().reasonCombatForwardAxisDegenerate;
        return false;
    }

    Vec3 right = Cross(forward, kWorldUp);
    if (!TryNormalize(right)) {
        const float yawRadians = eyeAngles.y * 3.14159265358979323846f / 180.0f;
        right = Vec3{ std::sin(yawRadians), -std::cos(yawRadians), 0.0f };
        if (!TryNormalize(right)) {
            failure = Texts().reasonCombatRightAxisDegenerate;
            return false;
        }
    }

    Vec3 up = Cross(right, forward);
    if (!TryNormalize(up)) {
        failure = Texts().reasonCombatUpAxisDegenerate;
        return false;
    }

    const Vec3 eyeOrigin{ eyeOriginRaw[0], eyeOriginRaw[1], eyeOriginRaw[2] };
    const bool useSyntheticHandAnchor = compat::ShouldUseSyntheticFallbackHandAnchor(
        g_lookupAttachment != nullptr && g_getAttachment != nullptr,
        heldItemKind != HeldItemKind::Unknown
    );
    if (useSyntheticHandAnchor) {
        const Vec3 handAnchor = ComposeBasisPoint(
            eyeOrigin,
            right,
            forward,
            up,
            kKnifeFallbackHandAnchorOffset.x,
            kKnifeFallbackHandAnchorOffset.y,
            kKnifeFallbackHandAnchorOffset.z
        );
        Vec3 syntheticRight = Subtract(handAnchor, eyeOrigin);
        syntheticRight = ProjectOntoPlane(syntheticRight, kWorldUp);
        if (!TryNormalize(syntheticRight)) {
            failure = Texts().reasonCombatRightAxisDegenerate;
            return false;
        }

        Vec3 syntheticForward = Cross(syntheticRight, kWorldUp);
        if (!TryNormalize(syntheticForward)) {
            failure = Texts().reasonCombatForwardAxisDegenerate;
            return false;
        }

        Vec3 syntheticUp = Cross(syntheticForward, syntheticRight);
        if (!TryNormalize(syntheticUp)) {
            failure = Texts().reasonCombatUpAxisDegenerate;
            return false;
        }

        basis.anchorOrigin = handAnchor;
        basis.upperBodyTarget = Lerp(eyeOrigin, handAnchor, GetUpperBodyTargetBlendForHeldItem(heldItemKind));
        basis.right = syntheticRight;
        basis.forward = syntheticForward;
        basis.up = syntheticUp;
        basis.syntheticFallbackHandAnchor = true;
        if (g_debugOptions.traceEnabled) {
            const unsigned long long fallbackIndex = g_knifeFallbackAnchorTraceCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
            if (fallbackIndex <= 20u || 0u == (fallbackIndex % 30u)) {
                TracePrintf(
                    "synthetic-fallback-anchor idx=%llu held=%d eye=(%.3f,%.3f,%.3f) anchor=(%.3f,%.3f,%.3f) right=(%.3f,%.3f,%.3f) forward=(%.3f,%.3f,%.3f)",
                    fallbackIndex,
                    static_cast<int>(heldItemKind),
                    eyeOrigin.x,
                    eyeOrigin.y,
                    eyeOrigin.z,
                    basis.anchorOrigin.x,
                    basis.anchorOrigin.y,
                    basis.anchorOrigin.z,
                    basis.right.x,
                    basis.right.y,
                    basis.right.z,
                    basis.forward.x,
                    basis.forward.y,
                    basis.forward.z
                );
            }
        }
    }
    else {
        basis.anchorOrigin = eyeOrigin;
        basis.upperBodyTarget = Add(eyeOrigin, Scale(forward, 24.0f));
        basis.right = right;
        basis.forward = forward;
        basis.up = up;
        basis.syntheticFallbackHandAnchor = false;
    }
    basis.heldItemKind = heldItemKind;

    const EntityHandle weaponHandle = PawnGetActiveWeaponHandle(targetPawn);
    basis.weaponHandle = weaponHandle;
    basis.supportedWeaponId = SupportedWeaponId::None;
    if (heldItemKind == HeldItemKind::Gun && weaponHandle.IsValid()) {
        basis.supportedWeaponId = TryGetSupportedWeaponId(ResolveHandleToEntity(weaponHandle));
    }

    return true;
}

bool TryBuildWeaponFrameBasis(void* targetPawn, const Vec3* viewSetupEyeOrigin, const Vec3* viewSetupEyeAngles, WeaponFrameBasis& basis, std::string& failure) {
    const HeldItemKind heldItemKind = GetHeldItemKindForPawn(targetPawn);
    if (g_lookupAttachment == nullptr || g_getAttachment == nullptr) {
        return TryBuildFallbackFrameBasis(targetPawn, heldItemKind, viewSetupEyeOrigin, viewSetupEyeAngles, basis, failure);
    }

    if (heldItemKind == HeldItemKind::Gun) {
        return TryBuildGunFrameBasis(targetPawn, viewSetupEyeOrigin, basis, failure);
    }

    return TryBuildHumanFrameBasis(targetPawn, heldItemKind, viewSetupEyeOrigin, basis, failure);
}

void VectorToAngles(const Vec3& direction, float& pitch, float& yaw) {
    const float horizontalLength = std::sqrt(direction.x * direction.x + direction.y * direction.y);
    pitch = std::atan2(-direction.z, horizontalLength) * 180.0f / 3.14159265358979323846f;
    yaw = std::atan2(direction.y, direction.x) * 180.0f / 3.14159265358979323846f;
}

void OverrideScopedAwpFov(void* viewSetup, EntityHandle targetHandle, const WeaponFrameBasis& basis) {
    if (viewSetup == nullptr || !targetHandle.IsValid() || !basis.weaponHandle.IsValid()) {
        return;
    }

    if (g_debugOptions.disableScopedFovOverride) {
        return;
    }

    auto* viewFov = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupFovOffset);
    float currentFov = 0.0f;
    if (!TryReadFloatPointer(viewFov, currentFov)) {
        TracePrintf("fov-read-fault viewSetup=%p fovPtr=%p", viewSetup, viewFov);
        return;
    }
    if (!std::isfinite(currentFov) || currentFov <= 1.0f) {
        TracePrintf("fov-read-invalid viewSetup=%p fovPtr=%p fov=%.3f", viewSetup, viewFov, currentFov);
        return;
    }

    std::lock_guard lock(g_scopedFovOverrideMutex);
    if (basis.supportedWeaponId != SupportedWeaponId::AWP) {
        if (g_scopedFovOverride.valid
            && g_scopedFovOverride.targetHandle.raw == targetHandle.raw
            && g_scopedFovOverride.weaponHandle.raw == basis.weaponHandle.raw) {
            g_scopedFovOverride.valid = false;
        }
        return;
    }

    if (!g_scopedFovOverride.valid
        || g_scopedFovOverride.targetHandle.raw != targetHandle.raw
        || g_scopedFovOverride.weaponHandle.raw != basis.weaponHandle.raw) {
        g_scopedFovOverride.targetHandle = targetHandle;
        g_scopedFovOverride.weaponHandle = basis.weaponHandle;
        g_scopedFovOverride.unzoomedFov = currentFov >= kScopedFovThreshold ? currentFov : kFallbackUnscopedFov;
        g_scopedFovOverride.valid = true;
    }

    if (currentFov >= kScopedFovThreshold) {
        g_scopedFovOverride.unzoomedFov = currentFov;
        return;
    }

    *viewFov = g_scopedFovOverride.unzoomedFov;
}

void RefreshFollowSnapshot() {
    const auto follow = ResolveFollowTarget();
    if (follow.entity != nullptr) {
        SetCurrentFollowHandle(follow.handle);
        return;
    }

    SetCurrentFollowHandle(EntityHandle{});
}

void __fastcall HookedFrameStageNotify(void* thisPointer, int stage) {
    SetLastFrameStage(stage);
    if (stage == kFollowRefreshFrameStage) {
        RefreshFollowSnapshot();
    }
    if (g_originalFrameStageNotify != nullptr) {
        g_originalFrameStageNotify(thisPointer, stage);
    }
}

bool HookFrameStageNotify() {
    if (g_source2Client == nullptr) {
        SetInitialized(false, Texts().reasonSource2ClientInterfaceMissing);
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(g_source2Client);
    if (vtable == nullptr) {
        SetInitialized(false, Texts().reasonSource2ClientVtableMissing);
        return false;
    }

    g_originalFrameStageNotify = reinterpret_cast<FrameStageNotifyFn>(vtable[36]);
    if (g_originalFrameStageNotify == nullptr) {
        SetInitialized(false, Texts().reasonFrameStageNotifyMissing);
        return false;
    }

    void* replacement = reinterpret_cast<void*>(&HookedFrameStageNotify);
    if (!WriteProcessMemoryProtected(&vtable[36], &replacement, sizeof(replacement))) {
        SetInitialized(false, Texts().reasonFrameStageNotifyHookFailed);
        return false;
    }

    return true;
}

RuntimeState GetStateForOverride() {
    std::lock_guard lock(g_stateMutex);
    return g_state;
}

OverrideConfig GetOverrideConfig() {
    std::lock_guard lock(g_stateMutex);
    return OverrideConfig{
        g_state.enabled,
        g_state.lookMode,
        g_state.targetMode,
        g_state.offset,
        g_state.lockedHandle,
        g_state.currentFollowHandle
    };
}

void __fastcall SelfieStickSetUpView(void* viewSetup);

DWORD __fastcall RunSelfieStickSetUpViewGuarded(void* viewSetup) {
    __try {
        SelfieStickSetUpView(viewSetup);
        return 0u;
    }
    __except (EXCEPTION_EXECUTE_HANDLER) {
        return GetExceptionCode();
    }
}

void __fastcall SelfieStickSetUpViewBridge(void* viewSetup) {
    const unsigned long long bridgeIndex = g_setupViewBridgeCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
    const bool traceBridge = g_debugOptions.traceEnabled && (bridgeIndex <= 20u || 0u == (bridgeIndex % 30u));
    if (traceBridge) {
        TracePrintf("bridge-enter idx=%llu viewSetup=%p", bridgeIndex, viewSetup);
    }

    const DWORD exceptionCode = RunSelfieStickSetUpViewGuarded(viewSetup);
    if (exceptionCode != 0u) {
        TracePrintf("bridge-exception idx=%llu code=0x%08lX viewSetup=%p", bridgeIndex, exceptionCode, viewSetup);
        RestoreSuppressedRenderWithViewModels();
        UpdateFailureReason(FormatIncompatibleBuildReason("SetUpView callback faulted"), true);
        SetEnabled(false);
    }
}

void __fastcall SelfieStickSetUpView(void* viewSetup) {
    if (viewSetup == nullptr) {
        return;
    }

    if (!g_setupViewCallbackObserved.exchange(true)) {
        TracePrintf("callback-active viewSetup=%p", viewSetup);
    }

    const OverrideConfig config = GetOverrideConfig();
    if (!config.enabled) {
        RestoreSuppressedRenderWithViewModels();
        return;
    }

    const unsigned long long frameIndex = g_traceFrameCounter.fetch_add(1, std::memory_order_relaxed) + 1u;
    const bool traceFrame = g_debugOptions.traceEnabled && (frameIndex <= 20u || 0u == (frameIndex % 30u));
    if (traceFrame) {
        TracePrintf(
            "frame-enter idx=%llu viewSetup=%p mode=%s locked=%u follow=%u",
            frameIndex,
            viewSetup,
            TargetModeToString(config.targetMode),
            config.lockedHandle.raw,
            config.followHandle.raw
        );
    }

    if (!g_enabledSetUpViewObserved.exchange(true)) {
        TracePrintf(
            "enabled-callback mode=%s follow=%u locked=%u",
            TargetModeToString(config.targetMode),
            config.followHandle.raw,
            config.lockedHandle.raw
        );
    }

    TargetResolution resolution{};
    if (config.targetMode == TargetMode::Locked) {
        resolution = ResolveLockedTarget(config.lockedHandle);
    }
    else {
        resolution = ResolveFollowTarget();
        SetCurrentFollowHandle(resolution.handle);
    }

    if (resolution.entity == nullptr) {
        if (traceFrame) {
            TracePrintf("frame-resolve-failed idx=%llu reason=%s", frameIndex, resolution.failure.c_str());
        }
        if (config.targetMode == TargetMode::Follow) {
            SetCurrentFollowHandle(EntityHandle{});
        }
        RestoreSuppressedRenderWithViewModels();
        UpdateFailureReason(resolution.failure, false);
        return;
    }

    if (traceFrame) {
        TracePrintf("frame-resolved idx=%llu entity=%p handle=%u", frameIndex, resolution.entity, resolution.handle.raw);
    }

    Vec3 viewSetupEyeOrigin{};
    const bool viewSetupEyeOriginAvailable = TryReadViewSetupOrigin(viewSetup, viewSetupEyeOrigin);
    Vec3 viewSetupEyeAngles{};
    const bool viewSetupEyeAnglesAvailable = TryReadViewSetupAngles(viewSetup, viewSetupEyeAngles);
    if (traceFrame) {
        TracePrintf(
            "frame-viewsetup-pose idx=%llu originAvailable=%d anglesAvailable=%d origin=(%.3f,%.3f,%.3f) angles=(%.3f,%.3f,%.3f)",
            frameIndex,
            viewSetupEyeOriginAvailable ? 1 : 0,
            viewSetupEyeAnglesAvailable ? 1 : 0,
            viewSetupEyeOrigin.x,
            viewSetupEyeOrigin.y,
            viewSetupEyeOrigin.z,
            viewSetupEyeAngles.x,
            viewSetupEyeAngles.y,
            viewSetupEyeAngles.z
        );
    }

    WeaponFrameBasis basis{};
    std::string failure;
    if (!TryBuildWeaponFrameBasis(
            resolution.entity,
            viewSetupEyeOriginAvailable ? &viewSetupEyeOrigin : nullptr,
            viewSetupEyeAnglesAvailable ? &viewSetupEyeAngles : nullptr,
            basis,
            failure)) {
        if (traceFrame) {
            TracePrintf("frame-basis-failed idx=%llu reason=%s", frameIndex, failure.c_str());
        }
        RestoreSuppressedRenderWithViewModels();
        UpdateFailureReason(failure, false);
        return;
    }

    if (traceFrame) {
        TracePrintf(
            "frame-basis-ok idx=%llu held=%d weapon=%u supported=%d synthetic=%d anchor=(%.3f,%.3f,%.3f)",
            frameIndex,
            static_cast<int>(basis.heldItemKind),
            basis.weaponHandle.raw,
            static_cast<int>(basis.supportedWeaponId),
            basis.syntheticFallbackHandAnchor ? 1 : 0,
            basis.anchorOrigin.x,
            basis.anchorOrigin.y,
            basis.anchorOrigin.z
        );
    }

    const Vec3 baseOffset = GetBaseOffsetForBasis(basis, config.lookMode);

    const compat::CameraOffset adjustedTrim = compat::ApplyLeftSelfieCameraOffsetAdjustment(
        compat::CameraOffset{
            GetCameraRightOffsetForBasis(basis, config.lookMode, baseOffset.x) + config.offset.x,
            baseOffset.y + config.offset.y,
            baseOffset.z + config.offset.z
        },
        config.lookMode == LookMode::SelfieLeft
    );

    const Vec3 composedTrim{
        adjustedTrim.right,
        adjustedTrim.back,
        adjustedTrim.up
    };

    const Vec3 cameraPosition = Add(
        Add(
            Add(basis.anchorOrigin, Scale(basis.right, composedTrim.x)),
            Scale(basis.forward, -composedTrim.y)
        ),
        Scale(basis.up, composedTrim.z)
    );

    Vec3 direction{};
    if (UsesLookTarget(config.lookMode)) {
        direction = Subtract(GetLookTargetForBasis(basis, config.lookMode), cameraPosition);
    }
    else {
        direction = basis.forward;
    }

    if (!TryNormalize(direction)) {
        RestoreSuppressedRenderWithViewModels();
        UpdateFailureReason(Texts().reasonCameraDirectionDegenerate, true);
        return;
    }

    float pitch{};
    float yaw{};
    VectorToAngles(direction, pitch, yaw);

    if (traceFrame) {
        TracePrintf("frame-pre-fov idx=%llu pitch=%.3f yaw=%.3f", frameIndex, pitch, yaw);
    }
    OverrideScopedAwpFov(viewSetup, resolution.handle, basis);

    auto* viewOrigin = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupOriginOffset);
    auto* viewAngles = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupAnglesOffset);
    if (traceFrame) {
        TracePrintf(
            "frame-pre-write idx=%llu originPtr=%p anglesPtr=%p pos=(%.3f,%.3f,%.3f) ang=(%.3f,%.3f)",
            frameIndex,
            viewOrigin,
            viewAngles,
            cameraPosition.x,
            cameraPosition.y,
            cameraPosition.z,
            pitch,
            yaw
        );
    }
    if (!TryWriteViewSetupPose(viewOrigin, viewAngles, cameraPosition, pitch, yaw)) {
        const std::string reason = FormatIncompatibleBuildReason("view setup write faulted");
        TracePrintf("frame-write-fault idx=%llu originPtr=%p anglesPtr=%p", frameIndex, viewOrigin, viewAngles);
        RestoreSuppressedRenderWithViewModels();
        UpdateFailureReason(reason, true);
        SetEnabled(false);
        return;
    }

    if (traceFrame) {
        TracePrintf("frame-write-ok idx=%llu", frameIndex);
    }

    UpdateRenderWithViewModelsSuppression(basis, true, traceFrame, frameIndex);
    UpdateOverrideSuccess(resolution.handle);
}

bool HookSetUpView() {
    g_runtimeCompatibility.setUpViewPatchReady = false;
    TracePrintf(
        "setupview-enter entity=%d split=%d schema=%d patch=%d",
        g_runtimeCompatibility.entityAccessReady ? 1 : 0,
        g_runtimeCompatibility.splitScreenAccessorReady ? 1 : 0,
        g_runtimeCompatibility.schemaOffsetsReady ? 1 : 0,
        g_runtimeCompatibility.setUpViewPatchReady ? 1 : 0
    );
    if (!compat::CanProbeSetUpViewHook(g_runtimeCompatibility)) {
        TracePrintf(
            "setupview-fail reason=precheck entity=%d split=%d",
            g_runtimeCompatibility.entityAccessReady ? 1 : 0,
            g_runtimeCompatibility.splitScreenAccessorReady ? 1 : 0
        );
        SetInitialized(false, FormatIncompatibleBuildReason("follow target entry points are not trusted"));
        return false;
    }

    const auto patchAddress = FindPattern(g_clientModule, kPatternSetupViewPatch);
    if (patchAddress == 0) {
        TracePrintf("setupview-fail reason=pattern-missing");
        SetInitialized(false, Texts().reasonSetUpViewPatchSiteMissing);
        return false;
    }
    TracePrintf("setupview-scan patch=%p", reinterpret_cast<void*>(patchAddress));

    hook::SetUpViewPatchSite patchSite{};
    std::string hookError;
    if (!hook::DecodeSetUpViewPatchSite(reinterpret_cast<void*>(patchAddress), patchSite, &hookError)) {
        TracePrintf("setupview-fail reason=decode error=%s", hookError.c_str());
        SetInitialized(false, FormatIncompatibleBuildReason(hookError.c_str()));
        return false;
    }

    g_setupViewPatchAddress = reinterpret_cast<void*>(patchSite.patchAddress);
    g_setupViewOriginalBytes = patchSite.originalBytes;
    if (!IsAddressReadableInModule(g_clientModule, reinterpret_cast<void*>(patchSite.helperStringAddress))
        || !IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(patchSite.helperCallTarget))
        || !IsAddressExecutableInModule(g_clientModule, reinterpret_cast<void*>(patchSite.returnAddress))) {
        TracePrintf(
            "setupview-fail reason=site-changed helperString=%p helperCall=%p return=%p",
            reinterpret_cast<void*>(patchSite.helperStringAddress),
            reinterpret_cast<void*>(patchSite.helperCallTarget),
            reinterpret_cast<void*>(patchSite.returnAddress)
        );
        SetInitialized(false, FormatIncompatibleBuildReason("SetUpView patch site changed"));
        return false;
    }

    g_runtimeCompatibility.setUpViewPatchReady = true;
    if (!compat::CanInstallSetUpViewHook(g_runtimeCompatibility)) {
        TracePrintf(
            "setupview-fail reason=compat-after-scan entity=%d split=%d patch=%d",
            g_runtimeCompatibility.entityAccessReady ? 1 : 0,
            g_runtimeCompatibility.splitScreenAccessorReady ? 1 : 0,
            g_runtimeCompatibility.setUpViewPatchReady ? 1 : 0
        );
        SetInitialized(false, FormatIncompatibleBuildReason("SetUpView hook blocked by compatibility gate"));
        return false;
    }

    auto* detourStub = reinterpret_cast<std::uint8_t*>(AllocateExecutableMemoryNearAddress(g_setupViewPatchAddress, 192));
    if (detourStub == nullptr) {
        TracePrintf("setupview-fail reason=near-alloc patch=%p", g_setupViewPatchAddress);
        g_runtimeCompatibility.setUpViewPatchReady = false;
        SetInitialized(false, Texts().reasonSetUpViewDetourAllocationFailed);
        return false;
    }

    g_setupViewDetourStub = detourStub;

    std::vector<std::uint8_t> stub;
    if (!hook::BuildSetUpViewDetourStub(
            reinterpret_cast<std::uintptr_t>(detourStub),
            patchSite,
            reinterpret_cast<std::uintptr_t>(&SelfieStickSetUpViewBridge),
            stub,
            &hookError)) {
        TracePrintf("setupview-fail reason=build-detour error=%s", hookError.c_str());
        VirtualFree(detourStub, 0, MEM_RELEASE);
        g_setupViewDetourStub = nullptr;
        g_runtimeCompatibility.setUpViewPatchReady = false;
        SetInitialized(false, FormatIncompatibleBuildReason(hookError.c_str()));
        return false;
    }

    std::memcpy(detourStub, stub.data(), stub.size());
    FlushInstructionCache(GetCurrentProcess(), detourStub, stub.size());

    std::array<std::uint8_t, hook::kSetUpViewPatchSize> patch{};
    if (!hook::BuildSetUpViewEntryPatch(
            patchSite.patchAddress,
            reinterpret_cast<std::uintptr_t>(g_setupViewDetourStub),
            patch,
            &hookError)) {
        TracePrintf("setupview-fail reason=build-entry error=%s", hookError.c_str());
        VirtualFree(detourStub, 0, MEM_RELEASE);
        g_setupViewDetourStub = nullptr;
        g_runtimeCompatibility.setUpViewPatchReady = false;
        SetInitialized(false, FormatIncompatibleBuildReason(hookError.c_str()));
        return false;
    }

    if (!WriteProcessMemoryProtected(g_setupViewPatchAddress, patch.data(), patch.size())) {
        TracePrintf("setupview-fail reason=patch-write gle=%lu", GetLastError());
        VirtualFree(detourStub, 0, MEM_RELEASE);
        g_setupViewDetourStub = nullptr;
        g_runtimeCompatibility.setUpViewPatchReady = false;
        SetInitialized(false, Texts().reasonSetUpViewPatchFailed);
        return false;
    }

    TracePrintf(
        "setupview-ok patch=%p helperString=%p helperCall=%p return=%p stub=%p",
        reinterpret_cast<void*>(patchSite.patchAddress),
        reinterpret_cast<void*>(patchSite.helperStringAddress),
        reinterpret_cast<void*>(patchSite.helperCallTarget),
        reinterpret_cast<void*>(patchSite.returnAddress),
        detourStub
    );
    ConsolePrintf(Texts().logSetUpViewHookInstalled, static_cast<unsigned long long>(patchAddress - reinterpret_cast<std::uintptr_t>(g_clientModule)));
    return true;
}

void HandleLockCurrent() {
    const auto follow = ResolveFollowTarget();
    if (follow.entity == nullptr) {
        UpdateFailureReason(follow.failure, false);
        ConsolePrintf(Texts().logBlocked, follow.failure.c_str());
        return;
    }

    SetLockedTarget(follow.handle);
    ConsolePrintf(Texts().logLockedTarget, follow.handle.raw);
}

LRESULT CALLBACK OverlayControllerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);

std::wstring ToWide(const std::string& text) {
    if (text.empty()) {
        return {};
    }

    const int length = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
    if (length <= 0) {
        return std::wstring(text.begin(), text.end());
    }

    std::wstring result(static_cast<std::size_t>(length), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, result.data(), length);
    result.resize(static_cast<std::size_t>(length) - 1u);
    return result;
}

std::string ToUtf8(const std::wstring& text) {
    if (text.empty()) {
        return {};
    }

    const int length = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
    if (length <= 0) {
        std::string fallback;
        fallback.reserve(text.size());
        for (const wchar_t character : text) {
            fallback.push_back(character >= 0 && character <= 0x7F ? static_cast<char>(character) : '?');
        }
        return fallback;
    }

    std::string result(static_cast<std::size_t>(length), '\0');
    WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), length, nullptr, nullptr);
    result.resize(static_cast<std::size_t>(length) - 1u);
    return result;
}

bool IsMouseVirtualKey(int virtualKey);
bool IsModifierVirtualKey(int virtualKey);

std::wstring GetSettingsFilePath() {
    std::array<wchar_t, MAX_PATH> modulePath{};
    const DWORD length = GetModuleFileNameW(g_module, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length == 0 || length >= modulePath.size()) {
        return kSettingsFileName;
    }

    std::wstring path(modulePath.data(), length);
    const std::size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash == std::wstring::npos) {
        return kSettingsFileName;
    }

    path.resize(lastSlash + 1u);
    path += kSettingsFileName;
    return path;
}

std::wstring GetTraceFilePath() {
    std::array<wchar_t, MAX_PATH> modulePath{};
    const DWORD length = GetModuleFileNameW(g_module, modulePath.data(), static_cast<DWORD>(modulePath.size()));
    if (length == 0 || length >= modulePath.size()) {
        return kTraceFileName;
    }

    std::wstring path(modulePath.data(), length);
    const std::size_t lastSlash = path.find_last_of(L"\\/");
    if (lastSlash == std::wstring::npos) {
        return kTraceFileName;
    }

    path.resize(lastSlash + 1u);
    path += kTraceFileName;
    return path;
}

void TracePrintf(const char* format, ...) {
    if (!g_debugOptions.traceEnabled) {
        return;
    }

    char payload[2048]{};
    va_list args;
    va_start(args, format);
    std::vsnprintf(payload, sizeof(payload), format, args);
    va_end(args);

    SYSTEMTIME systemTime{};
    GetLocalTime(&systemTime);

    char line[2304]{};
    std::snprintf(
        line,
        sizeof(line),
        "%04u-%02u-%02u %02u:%02u:%02u.%03u %s\r\n",
        systemTime.wYear,
        systemTime.wMonth,
        systemTime.wDay,
        systemTime.wHour,
        systemTime.wMinute,
        systemTime.wSecond,
        systemTime.wMilliseconds,
        payload
    );

    const std::wstring tracePath = GetTraceFilePath();
    std::lock_guard lock(g_traceMutex);
    HANDLE file = CreateFileW(tracePath.c_str(), FILE_APPEND_DATA, FILE_SHARE_READ | FILE_SHARE_WRITE, nullptr, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
    if (file == INVALID_HANDLE_VALUE) {
        return;
    }

    DWORD written{};
    WriteFile(file, line, static_cast<DWORD>(std::strlen(line)), &written, nullptr);
    FlushFileBuffers(file);
    CloseHandle(file);
}

void PersistOverlayHotkeyVirtualKey(int virtualKey) {
    wchar_t value[16]{};
    _itow_s(virtualKey, value, 10);
    const std::wstring settingsPath = GetSettingsFilePath();
    WritePrivateProfileStringW(kSettingsSectionHotkeys, kSettingsKeyPanelToggleVirtualKey, value, settingsPath.c_str());
}

void LoadPersistedOverlayHotkeySetting() {
    const std::wstring settingsPath = GetSettingsFilePath();
    const UINT persistedVirtualKey = GetPrivateProfileIntW(
        kSettingsSectionHotkeys,
        kSettingsKeyPanelToggleVirtualKey,
        kOverlayHotkeyVirtualKey,
        settingsPath.c_str()
    );

    if (persistedVirtualKey <= 0 || persistedVirtualKey > 255) {
        return;
    }

    if (IsMouseVirtualKey(static_cast<int>(persistedVirtualKey)) || IsModifierVirtualKey(static_cast<int>(persistedVirtualKey))) {
        return;
    }

    std::lock_guard lock(g_stateMutex);
    g_state.overlayHotkeyVirtualKey = static_cast<int>(persistedVirtualKey);
    RefreshStatusTextLocked();
}

void LoadDebugOptions() {
    const std::wstring settingsPath = GetSettingsFilePath();
    g_debugOptions.traceEnabled = 0 != GetPrivateProfileIntW(kSettingsSectionDebug, kSettingsKeyTraceEnabled, 1, settingsPath.c_str());
    g_debugOptions.disableScopedFovOverride = 0 != GetPrivateProfileIntW(kSettingsSectionDebug, kSettingsKeyDisableScopedFovOverride, 0, settingsPath.c_str());
    g_debugOptions.disableGunStabilization = 0 != GetPrivateProfileIntW(kSettingsSectionDebug, kSettingsKeyDisableGunStabilization, 0, settingsPath.c_str());
}

const char* InitStageToString(int stage) noexcept {
    switch (stage) {
    case 1:
        return "overlay";
    case 2:
        return "interfaces";
    case 3:
        return "schema";
    case 4:
        return "entity";
    case 5:
        return "setupview-hook";
    case 6:
        return "ready";
    default:
        return "boot";
    }
}

int GetConfiguredOverlayHotkeyVirtualKey() {
    std::lock_guard lock(g_stateMutex);
    return g_state.overlayHotkeyVirtualKey;
}

bool IsMouseVirtualKey(int virtualKey) {
    return virtualKey >= VK_LBUTTON && virtualKey <= VK_XBUTTON2;
}

bool IsModifierVirtualKey(int virtualKey) {
    switch (virtualKey) {
    case VK_SHIFT:
    case VK_LSHIFT:
    case VK_RSHIFT:
    case VK_CONTROL:
    case VK_LCONTROL:
    case VK_RCONTROL:
    case VK_MENU:
    case VK_LMENU:
    case VK_RMENU:
    case VK_LWIN:
    case VK_RWIN:
        return true;
    default:
        return false;
    }
}

bool IsExtendedVirtualKey(int virtualKey) {
    switch (virtualKey) {
    case VK_INSERT:
    case VK_DELETE:
    case VK_HOME:
    case VK_END:
    case VK_PRIOR:
    case VK_NEXT:
    case VK_LEFT:
    case VK_RIGHT:
    case VK_UP:
    case VK_DOWN:
    case VK_NUMLOCK:
    case VK_DIVIDE:
    case VK_RCONTROL:
    case VK_RMENU:
        return true;
    default:
        return false;
    }
}

std::wstring GetVirtualKeyDisplayName(int virtualKey) {
    if (virtualKey >= '0' && virtualKey <= '9') {
        return std::wstring(1, static_cast<wchar_t>(virtualKey));
    }

    if (virtualKey >= 'A' && virtualKey <= 'Z') {
        return std::wstring(1, static_cast<wchar_t>(virtualKey));
    }

    if (virtualKey >= VK_F1 && virtualKey <= VK_F24) {
        return L"F" + std::to_wstring(virtualKey - VK_F1 + 1);
    }

    UINT scanCode = MapVirtualKeyW(static_cast<UINT>(virtualKey), MAPVK_VK_TO_VSC);
    LONG keyNameParam = static_cast<LONG>(scanCode << 16);
    if (IsExtendedVirtualKey(virtualKey)) {
        keyNameParam |= 0x01000000;
    }

    wchar_t nameBuffer[96]{};
    if (GetKeyNameTextW(keyNameParam, nameBuffer, static_cast<int>(std::size(nameBuffer))) > 0) {
        return nameBuffer;
    }

    return L"VK " + std::to_wstring(virtualKey);
}

std::wstring BuildOverlayHotkeyChipText(int virtualKey) {
    return std::wstring(Texts().uiHotkeyChip) + L" " + GetVirtualKeyDisplayName(virtualKey);
}

std::wstring BuildOverlayHotkeyButtonText() {
    return g_ui.captureOverlayHotkey ? Texts().uiButtonPanelHotkeyCapture : Texts().uiButtonPanelHotkey;
}

HBRUSH GetCachedSolidBrush(COLORREF color) {
    std::lock_guard lock(g_solidBrushCacheMutex);
    const auto existing = g_solidBrushCache.find(color);
    if (existing != g_solidBrushCache.end()) {
        return existing->second;
    }

    HBRUSH brush = CreateSolidBrush(color);
    if (brush != nullptr) {
        g_solidBrushCache.emplace(color, brush);
    }
    return brush;
}

void SetOverlayHotkeyVirtualKey(int virtualKey) {
    {
        std::lock_guard lock(g_stateMutex);
        g_state.overlayHotkeyVirtualKey = virtualKey;
        RefreshStatusTextLocked();
    }

    g_ui.overlayHotkeyWasDown = true;
    MarkOverlayDirty();
    PersistOverlayHotkeyVirtualKey(virtualKey);
    ConsolePrintf(Texts().logOverlayHotkeyChanged, ToUtf8(GetVirtualKeyDisplayName(virtualKey)).c_str());
}

std::wstring ReadWindowText(HWND window) {
    const int length = GetWindowTextLengthW(window);
    std::vector<wchar_t> buffer(static_cast<std::size_t>(length) + 1u, L'\0');
    if (length > 0) {
        GetWindowTextW(window, buffer.data(), length + 1);
    }
    return std::wstring(buffer.data());
}

void SetControlText(HWND window, const std::wstring& text) {
    if (window == nullptr) {
        return;
    }

    SetWindowTextW(window, text.c_str());
}

bool TryParseWideFloat(const std::wstring& text, float& output) {
    if (text.empty()) {
        return false;
    }

    wchar_t* end = nullptr;
    const float parsed = std::wcstof(text.c_str(), &end);
    if (end == text.c_str()) {
        return false;
    }

    while (*end != L'\0' && std::iswspace(*end)) {
        ++end;
    }

    if (*end != L'\0') {
        return false;
    }

    output = parsed;
    return true;
}

bool TryReadFloatFromEdit(HWND edit, float& output) {
    if (edit == nullptr) {
        return false;
    }

    return TryParseWideFloat(ReadWindowText(edit), output);
}

std::wstring GetOverlayButtonText(int controlId, HWND controlWindow, const RuntimeState& state) {
    switch (controlId) {
    case kControlButtonEnable:
        return state.enabled ? Texts().uiButtonEnableOn : Texts().uiButtonEnableOff;
    case kControlButtonLookSelfieLeft:
        return Texts().uiButtonLookSelfieLeft;
    case kControlButtonLookSelfieRight:
        return Texts().uiButtonLookSelfieRight;
    case kControlButtonLookForward:
        return Texts().uiButtonLookForward;
    case kControlButtonTargetFollow:
        return Texts().uiButtonTargetFollow;
    case kControlButtonTargetLock:
        return Texts().uiButtonTargetLockCurrent;
    case kControlButtonTargetClear:
        return Texts().uiButtonTargetClear;
    case kControlButtonPanelHotkey:
        return BuildOverlayHotkeyButtonText();
    default:
        return ReadWindowText(controlWindow);
    }
}

void SetEditFloatText(HWND edit, float value) {
    if (edit == nullptr) {
        return;
    }

    wchar_t buffer[64]{};
    swprintf_s(buffer, L"%.3f", value);
    SetControlText(edit, buffer);
}

std::wstring HandleToWide(EntityHandle handle) {
    if (!handle.IsValid()) {
        return Texts().uiNone;
    }

    return std::to_wstring(handle.raw);
}

void AppendAsciiToWide(std::wstring& out, const char* text) {
    if (text == nullptr) {
        return;
    }

    while (*text != '\0') {
        out.push_back(static_cast<wchar_t>(static_cast<unsigned char>(*text)));
        ++text;
    }
}

void AppendWideToWide(std::wstring& out, const std::wstring& text) {
    out.append(text);
}

void AppendUtf8ToWide(std::wstring& out, const std::string& text) {
    if (text.empty()) {
        return;
    }

    out.append(ToWide(text));
}

void AppendFormattedFloatToWide(std::wstring& out, float value) {
    char buffer[32]{};
    std::snprintf(buffer, sizeof(buffer), "%.3f", value);
    AppendAsciiToWide(out, buffer);
}

std::wstring BuildOverlayStatusText(const RuntimeState& state) {
    std::wstring result;
    result.reserve(256);

    AppendWideToWide(result, Texts().uiEnabledLabel);
    AppendWideToWide(result, state.enabled ? Texts().uiStatusOn : Texts().uiStatusOff);
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiLookLabel);
    AppendAsciiToWide(result, LookModeToString(state.lookMode));
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiPanelHotkeyLabel);
    AppendWideToWide(result, GetVirtualKeyDisplayName(state.overlayHotkeyVirtualKey));
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiTargetModeLabel);
    AppendAsciiToWide(result, TargetModeToString(state.targetMode));
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiLockedHandleLabel);
    AppendWideToWide(result, HandleToWide(state.lockedHandle));
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiCurrentTargetLabel);
    AppendWideToWide(result, HandleToWide(state.currentTargetHandle));
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiAnchorAttachLabel);
    AppendAsciiToWide(result, state.attachmentName.c_str());
    AppendWideToWide(result, L" / ");
    AppendWideToWide(result, state.attachmentAvailable ? Texts().uiStatusReady : Texts().uiStatusMissing);
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiTrimLabel);
    AppendFormattedFloatToWide(result, state.offset.x);
    AppendWideToWide(result, L" / ");
    AppendFormattedFloatToWide(result, state.offset.y);
    AppendWideToWide(result, L" / ");
    AppendFormattedFloatToWide(result, state.offset.z);
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiLastOverrideLabel);
    AppendWideToWide(result, state.lastOverrideSucceeded ? Texts().uiStatusSuccess : Texts().uiStatusFailed);
    AppendWideToWide(result, L"\r\n");

    AppendWideToWide(result, Texts().uiReasonLabel);
    if (state.lastFailureReason.empty()) {
        AppendWideToWide(result, Texts().uiReasonEmpty);
    }
    else {
        AppendUtf8ToWide(result, state.lastFailureReason);
    }

    return result;
}

bool OverlayOffsetEditHasFocus() {
    const HWND focused = GetFocus();
    return focused == g_ui.offsetXEdit || focused == g_ui.offsetYEdit || focused == g_ui.offsetZEdit;
}

bool OverlayStateAffectsView(const RuntimeState& left, const RuntimeState& right) {
    return left.initialized == right.initialized
        && left.enabled == right.enabled
        && left.lookMode == right.lookMode
        && left.targetMode == right.targetMode
        && left.overlayHotkeyVirtualKey == right.overlayHotkeyVirtualKey
        && left.offset.x == right.offset.x
        && left.offset.y == right.offset.y
        && left.offset.z == right.offset.z
        && left.lockedHandle.raw == right.lockedHandle.raw
        && left.currentTargetHandle.raw == right.currentTargetHandle.raw
        && left.attachmentAvailable == right.attachmentAvailable
        && left.lastOverrideSucceeded == right.lastOverrideSucceeded
        && left.attachmentName == right.attachmentName
        && left.lastFailureReason == right.lastFailureReason;
}

void SyncOverlayFromState(bool force = false) {
    if (!g_ui.overlayCreated || !IsWindow(g_ui.overlayWindow)) {
        return;
    }

    const RuntimeState state = SnapshotState();
    const bool hasLastRenderedState = g_ui.hasLastRenderedState;
    const bool stateChanged = !hasLastRenderedState || !OverlayStateAffectsView(g_ui.lastRenderedState, state);
    if (!force && !stateChanged) {
        return;
    }

    const bool enabledChanged = force || !hasLastRenderedState || g_ui.lastRenderedState.enabled != state.enabled;
    const bool overlayHotkeyChanged = force || !hasLastRenderedState || g_ui.lastRenderedState.overlayHotkeyVirtualKey != state.overlayHotkeyVirtualKey;
    const bool offsetChanged = force || !hasLastRenderedState
        || g_ui.lastRenderedState.offset.x != state.offset.x
        || g_ui.lastRenderedState.offset.y != state.offset.y
        || g_ui.lastRenderedState.offset.z != state.offset.z;
    const bool statusChanged = force || !hasLastRenderedState || g_ui.lastRenderedStatusText.empty() || !OverlayStateAffectsView(g_ui.lastRenderedState, state);

    if (force || !hasLastRenderedState) {
        SetControlText(g_ui.lookSelfieLeftButton, Texts().uiButtonLookSelfieLeft);
        SetControlText(g_ui.lookSelfieRightButton, Texts().uiButtonLookSelfieRight);
        SetControlText(g_ui.lookForwardButton, Texts().uiButtonLookForward);
        SetControlText(g_ui.targetFollowButton, Texts().uiButtonTargetFollow);
        SetControlText(g_ui.targetLockButton, Texts().uiButtonTargetLockCurrent);
        SetControlText(g_ui.targetClearButton, Texts().uiButtonTargetClear);
    }

    if (enabledChanged) {
        SetControlText(g_ui.enableButton, state.enabled ? Texts().uiButtonEnableOn : Texts().uiButtonEnableOff);
    }

    if (overlayHotkeyChanged) {
        g_ui.lastRenderedHotkeyChipText = BuildOverlayHotkeyChipText(state.overlayHotkeyVirtualKey);
        SetControlText(g_ui.overlayHotkeyButton, BuildOverlayHotkeyButtonText());
    }

    if (statusChanged) {
        const std::wstring statusText = BuildOverlayStatusText(state);
        SetControlText(g_ui.statusEdit, statusText);
        g_ui.lastRenderedStatusText = statusText;
    }

    if (offsetChanged && !OverlayOffsetEditHasFocus()) {
        g_ui.suppressOffsetSync = true;
        SetEditFloatText(g_ui.offsetXEdit, state.offset.x);
        SetEditFloatText(g_ui.offsetYEdit, state.offset.y);
        SetEditFloatText(g_ui.offsetZEdit, state.offset.z);
        g_ui.suppressOffsetSync = false;
    }

    g_ui.lastRenderedState = state;
    g_ui.hasLastRenderedState = true;
    InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
}

void DestroyOverlayResources() {
    if (g_ui.panelBrush != nullptr) {
        DeleteObject(g_ui.panelBrush);
        g_ui.panelBrush = nullptr;
    }

    if (g_ui.editBrush != nullptr) {
        DeleteObject(g_ui.editBrush);
        g_ui.editBrush = nullptr;
    }

    if (g_ui.titleFont != nullptr) {
        DeleteObject(g_ui.titleFont);
        g_ui.titleFont = nullptr;
    }

    if (g_ui.bodyFont != nullptr) {
        DeleteObject(g_ui.bodyFont);
        g_ui.bodyFont = nullptr;
    }

    if (g_ui.monoFont != nullptr) {
        DeleteObject(g_ui.monoFont);
        g_ui.monoFont = nullptr;
    }

    {
        std::lock_guard lock(g_solidBrushCacheMutex);
        for (auto& entry : g_solidBrushCache) {
            if (entry.second != nullptr) {
                DeleteObject(entry.second);
            }
        }
        g_solidBrushCache.clear();
    }
}

bool EnsureOverlayResources() {
    if (g_ui.panelBrush != nullptr && g_ui.editBrush != nullptr && g_ui.titleFont != nullptr && g_ui.bodyFont != nullptr && g_ui.monoFont != nullptr) {
        return true;
    }

    DestroyOverlayResources();

    g_ui.panelBrush = CreateSolidBrush(kColorPanel);
    g_ui.editBrush = CreateSolidBrush(kColorInput);
    g_ui.titleFont = CreateFontW(-24, 0, 0, 0, FW_BOLD, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, Texts().uiFontName);
    g_ui.bodyFont = CreateFontW(-15, 0, 0, 0, FW_MEDIUM, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, Texts().uiFontName);
    g_ui.monoFont = CreateFontW(-14, 0, 0, 0, FW_NORMAL, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_DEFAULT_PRECIS, CLIP_DEFAULT_PRECIS, CLEARTYPE_QUALITY, DEFAULT_PITCH | FF_SWISS, Texts().uiMonoFontName);

    if (g_ui.panelBrush == nullptr || g_ui.editBrush == nullptr || g_ui.titleFont == nullptr || g_ui.bodyFont == nullptr || g_ui.monoFont == nullptr) {
        DestroyOverlayResources();
        return false;
    }

    return true;
}

bool RegisterOverlayWindowClasses() {
    WNDCLASSEXW controllerClass{};
    controllerClass.cbSize = sizeof(controllerClass);
    controllerClass.lpfnWndProc = &OverlayControllerWindowProc;
    controllerClass.hInstance = g_module;
    controllerClass.lpszClassName = kOverlayControllerClassName;
    if (!RegisterClassExW(&controllerClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    WNDCLASSEXW overlayClass{};
    overlayClass.cbSize = sizeof(overlayClass);
    overlayClass.style = CS_HREDRAW | CS_VREDRAW;
    overlayClass.lpfnWndProc = &OverlayWindowProc;
    overlayClass.hInstance = g_module;
    overlayClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);
    overlayClass.lpszClassName = kOverlayWindowClassName;
    if (!RegisterClassExW(&overlayClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS) {
        return false;
    }

    return true;
}

HWND FindHostWindow() {
    struct FindHostWindowContext {
        DWORD processId;
        HWND bestWindow;
        long long bestArea;
    } context{ GetCurrentProcessId(), nullptr, 0 };

    EnumWindows(
        [](HWND window, LPARAM parameter) -> BOOL {
            auto* context = reinterpret_cast<FindHostWindowContext*>(parameter);

            DWORD processId{};
            GetWindowThreadProcessId(window, &processId);
            if (processId != context->processId) {
                return TRUE;
            }

            if (!IsWindowVisible(window) || GetWindow(window, GW_OWNER) != nullptr) {
                return TRUE;
            }

            const LONG_PTR style = GetWindowLongPtrW(window, GWL_STYLE);
            const LONG_PTR exStyle = GetWindowLongPtrW(window, GWL_EXSTYLE);
            if ((style & WS_CHILD) != 0 || (exStyle & WS_EX_TOOLWINDOW) != 0) {
                return TRUE;
            }

            RECT bounds{};
            if (!GetWindowRect(window, &bounds)) {
                return TRUE;
            }

            const long long width = static_cast<long long>(bounds.right) - static_cast<long long>(bounds.left);
            const long long height = static_cast<long long>(bounds.bottom) - static_cast<long long>(bounds.top);
            const long long area = width * height;
            if (area <= context->bestArea) {
                return TRUE;
            }

            context->bestArea = area;
            context->bestWindow = window;
            return TRUE;
        },
        reinterpret_cast<LPARAM>(&context)
    );

    return context.bestWindow;
}

bool IsCurrentProcessForeground() {
    const HWND foreground = GetForegroundWindow();
    if (foreground == nullptr) {
        return false;
    }

    DWORD processId{};
    GetWindowThreadProcessId(foreground, &processId);
    return processId == GetCurrentProcessId();
}

bool IsOverlayHotkeyContextActive() {
    if (IsWindow(g_ui.overlayWindow) && GetForegroundWindow() == g_ui.overlayWindow) {
        return true;
    }

    return IsCurrentProcessForeground();
}

bool IsOverlayToggleKeyMessage(const MSG& message) {
    switch (message.message) {
    case WM_KEYDOWN:
    case WM_KEYUP:
    case WM_SYSKEYDOWN:
    case WM_SYSKEYUP:
        return static_cast<int>(message.wParam) == GetConfiguredOverlayHotkeyVirtualKey();
    default:
        return false;
    }
}

SIZE GetOverlayWindowSizeForClient(int clientWidth, int clientHeight) {
    RECT windowRect{ 0, 0, clientWidth, clientHeight };
    AdjustWindowRectEx(&windowRect, kOverlayWindowStyle, FALSE, kOverlayWindowExStyle);
    return SIZE{
        windowRect.right - windowRect.left,
        windowRect.bottom - windowRect.top
    };
}

RECT GetOverlayWorkArea() {
    HMONITOR monitor = nullptr;
    if (IsWindow(g_ui.hostWindow)) {
        monitor = MonitorFromWindow(g_ui.hostWindow, MONITOR_DEFAULTTONEAREST);
    }
    else if (IsWindow(g_ui.overlayWindow)) {
        monitor = MonitorFromWindow(g_ui.overlayWindow, MONITOR_DEFAULTTONEAREST);
    }

    MONITORINFO monitorInfo{};
    monitorInfo.cbSize = sizeof(monitorInfo);
    if (monitor != nullptr && GetMonitorInfoW(monitor, &monitorInfo)) {
        return monitorInfo.rcWork;
    }

    RECT workArea{};
    SystemParametersInfoW(SPI_GETWORKAREA, 0, &workArea, 0);
    return workArea;
}

bool PositionOverlayWindow(bool activate) {
    if (!IsWindow(g_ui.overlayWindow)) {
        return false;
    }

    if (g_ui.overlayPlacementInitialized) {
        return false;
    }

    if (!IsWindow(g_ui.hostWindow)) {
        g_ui.hostWindow = FindHostWindow();
    }

    RECT targetRect = GetOverlayWorkArea();
    if (IsWindow(g_ui.hostWindow)) {
        GetWindowRect(g_ui.hostWindow, &targetRect);
    }

    const RECT workArea = GetOverlayWorkArea();
    const SIZE windowSize = GetOverlayWindowSizeForClient(kOverlayWidth, kOverlayHeight);
    int x = targetRect.right - windowSize.cx - kOverlayMargin;
    int y = targetRect.top + kOverlayMargin;
    if (x < workArea.left + kOverlayMargin) {
        x = workArea.left + kOverlayMargin;
    }
    if (y < workArea.top + kOverlayMargin) {
        y = workArea.top + kOverlayMargin;
    }
    if (x + windowSize.cx > workArea.right - kOverlayMargin) {
        x = workArea.right - windowSize.cx - kOverlayMargin;
    }
    if (y + windowSize.cy > workArea.bottom - kOverlayMargin) {
        y = workArea.bottom - windowSize.cy - kOverlayMargin;
    }

    UINT flags = SWP_NOOWNERZORDER | SWP_NOZORDER;
    if (!activate) {
        flags |= SWP_NOACTIVATE;
    }

    SetWindowPos(g_ui.overlayWindow, nullptr, x, y, windowSize.cx, windowSize.cy, flags);
    g_ui.overlayPlacementInitialized = true;
    return true;
}

int RectWidth(const RECT& rect) {
    return rect.right - rect.left;
}

int RectHeight(const RECT& rect) {
    return rect.bottom - rect.top;
}

RECT MakeRect(int left, int top, int width, int height) {
    return RECT{ left, top, left + width, top + height };
}

struct OverlayLayout {
    RECT clientRect{};
    RECT headerRect{};
    RECT enableCardRect{};
    RECT offsetCardRect{};
    RECT lookCardRect{};
    RECT targetCardRect{};
    RECT statusCardRect{};

    RECT titleRect{};
    RECT subtitleRect{};
    RECT statusChipRect{};
    RECT hotkeyChipRect{};
    RECT hotkeyButtonRect{};

    RECT enableLabelRect{};
    RECT enableChipRect{};
    RECT enableButtonRect{};

    RECT offsetLabelRect{};
    RECT offsetHintRect{};
    RECT offsetXLabelRect{};
    RECT offsetYLabelRect{};
    RECT offsetZLabelRect{};
    RECT offsetXEditRect{};
    RECT offsetYEditRect{};
    RECT offsetZEditRect{};

    RECT lookLabelRect{};
    RECT lookChipRect{};
    RECT lookSelfieLeftButtonRect{};
    RECT lookSelfieRightButtonRect{};
    RECT lookForwardButtonRect{};

    RECT targetLabelRect{};
    RECT targetChipRect{};
    RECT targetFollowButtonRect{};
    RECT targetLockButtonRect{};
    RECT targetClearButtonRect{};

    RECT statusLabelRect{};
    RECT statusSubRect{};
    RECT statusEditRect{};
};

const OverlayLayout& GetOverlayLayout(HWND window) {
    static OverlayLayout cachedLayout{};
    static int cachedClientWidth = -1;
    static int cachedClientHeight = -1;

    RECT clientRect{};
    GetClientRect(window, &clientRect);
    const int clientWidth = RectWidth(clientRect);
    const int clientHeight = RectHeight(clientRect);
    if (clientWidth == cachedClientWidth && clientHeight == cachedClientHeight) {
        return cachedLayout;
    }

    OverlayLayout layout{};
    layout.clientRect = clientRect;

    const int sideMargin = 16;
    const int topMargin = 10;
    const int cardGap = 10;
    const int sectionPadding = 12;
    const int buttonGap = 12;
    const int cardWidth = RectWidth(layout.clientRect) - sideMargin * 2;
    const int headerHeight = 110;
    const int enableHeight = 66;
    const int offsetHeight = 100;
    const int lookHeight = 86;
    const int targetHeight = 72;

    int currentTop = topMargin;
    layout.headerRect = MakeRect(sideMargin, currentTop, cardWidth, headerHeight);
    currentTop = layout.headerRect.bottom + cardGap;
    layout.enableCardRect = MakeRect(sideMargin, currentTop, cardWidth, enableHeight);
    currentTop = layout.enableCardRect.bottom + cardGap;
    layout.offsetCardRect = MakeRect(sideMargin, currentTop, cardWidth, offsetHeight);
    currentTop = layout.offsetCardRect.bottom + cardGap;
    layout.lookCardRect = MakeRect(sideMargin, currentTop, cardWidth, lookHeight);
    currentTop = layout.lookCardRect.bottom + cardGap;
    layout.targetCardRect = MakeRect(sideMargin, currentTop, cardWidth, targetHeight);
    currentTop = layout.targetCardRect.bottom + cardGap;
    layout.statusCardRect = MakeRect(sideMargin, currentTop, cardWidth, layout.clientRect.bottom - sideMargin - currentTop);

    layout.titleRect = MakeRect(layout.headerRect.left + sectionPadding, layout.headerRect.top + 10, cardWidth - 220, 24);
    layout.subtitleRect = MakeRect(layout.headerRect.left + sectionPadding, layout.headerRect.top + 36, cardWidth - 220, 18);
    layout.statusChipRect = MakeRect(layout.headerRect.right - 124, layout.headerRect.top + 12, 112, 22);
    layout.hotkeyChipRect = MakeRect(layout.headerRect.right - 172, layout.headerRect.top + 42, 160, 22);
    layout.hotkeyButtonRect = MakeRect(layout.headerRect.left + sectionPadding, layout.headerRect.top + 72, cardWidth - sectionPadding * 2, 28);

    layout.enableLabelRect = MakeRect(layout.enableCardRect.left + sectionPadding, layout.enableCardRect.top + 12, 160, 16);
    layout.enableChipRect = MakeRect(layout.enableCardRect.right - 92, layout.enableCardRect.top + 12, 80, 18);
    layout.enableButtonRect = MakeRect(layout.enableCardRect.left + sectionPadding, layout.enableCardRect.top + 28, cardWidth - sectionPadding * 2, 28);

    layout.offsetLabelRect = MakeRect(layout.offsetCardRect.left + sectionPadding, layout.offsetCardRect.top + 12, 180, 16);
    layout.offsetXLabelRect = MakeRect(layout.offsetCardRect.left + sectionPadding, layout.offsetCardRect.top + 34, 80, 14);
    layout.offsetYLabelRect = MakeRect(layout.offsetCardRect.left + sectionPadding + (cardWidth - sectionPadding * 2 - buttonGap * 2) / 3 + buttonGap, layout.offsetCardRect.top + 34, 80, 14);
    layout.offsetZLabelRect = MakeRect(layout.offsetCardRect.left + sectionPadding + ((cardWidth - sectionPadding * 2 - buttonGap * 2) / 3 + buttonGap) * 2, layout.offsetCardRect.top + 34, 80, 14);
    const int offsetEditWidth = (cardWidth - sectionPadding * 2 - buttonGap * 2) / 3;
    layout.offsetXEditRect = MakeRect(layout.offsetCardRect.left + sectionPadding, layout.offsetCardRect.top + 50, offsetEditWidth, 30);
    layout.offsetYEditRect = MakeRect(layout.offsetXEditRect.right + buttonGap, layout.offsetCardRect.top + 50, offsetEditWidth, 30);
    layout.offsetZEditRect = MakeRect(layout.offsetYEditRect.right + buttonGap, layout.offsetCardRect.top + 50, offsetEditWidth, 30);
    layout.offsetHintRect = MakeRect(layout.offsetCardRect.left + sectionPadding, layout.offsetCardRect.bottom - 20, cardWidth - sectionPadding * 2, 14);

    layout.lookLabelRect = MakeRect(layout.lookCardRect.left + sectionPadding, layout.lookCardRect.top + 12, 180, 16);
    layout.lookChipRect = MakeRect(layout.lookCardRect.right - 116, layout.lookCardRect.top + 12, 104, 18);
    const int lookButtonWidth = (cardWidth - sectionPadding * 2 - buttonGap * 2) / 3;
    layout.lookSelfieLeftButtonRect = MakeRect(layout.lookCardRect.left + sectionPadding, layout.lookCardRect.top + 40, lookButtonWidth, 30);
    layout.lookSelfieRightButtonRect = MakeRect(layout.lookSelfieLeftButtonRect.right + buttonGap, layout.lookCardRect.top + 40, lookButtonWidth, 30);
    layout.lookForwardButtonRect = MakeRect(layout.lookSelfieRightButtonRect.right + buttonGap, layout.lookCardRect.top + 40, lookButtonWidth, 30);

    layout.targetLabelRect = MakeRect(layout.targetCardRect.left + sectionPadding, layout.targetCardRect.top + 12, 180, 16);
    layout.targetChipRect = MakeRect(layout.targetCardRect.right - 116, layout.targetCardRect.top + 12, 104, 18);
    const int targetButtonWidth = (cardWidth - sectionPadding * 2 - buttonGap * 2) / 3;
    layout.targetFollowButtonRect = MakeRect(layout.targetCardRect.left + sectionPadding, layout.targetCardRect.top + 30, targetButtonWidth, 30);
    layout.targetLockButtonRect = MakeRect(layout.targetFollowButtonRect.right + buttonGap, layout.targetCardRect.top + 30, targetButtonWidth, 30);
    layout.targetClearButtonRect = MakeRect(layout.targetLockButtonRect.right + buttonGap, layout.targetCardRect.top + 30, targetButtonWidth, 30);

    layout.statusLabelRect = MakeRect(layout.statusCardRect.left + sectionPadding, layout.statusCardRect.top + 12, 180, 16);
    layout.statusSubRect = MakeRect(layout.statusCardRect.right - 220, layout.statusCardRect.top + 12, 208, 16);
    layout.statusEditRect = MakeRect(layout.statusCardRect.left + sectionPadding, layout.statusCardRect.top + 34, cardWidth - sectionPadding * 2, RectHeight(layout.statusCardRect) - 46);

    cachedLayout = layout;
    cachedClientWidth = clientWidth;
    cachedClientHeight = clientHeight;
    return cachedLayout;
}

void MoveOverlayControl(HWND control, const RECT& rect) {
    if (control == nullptr) {
        return;
    }

    MoveWindow(control, rect.left, rect.top, RectWidth(rect), RectHeight(rect), TRUE);
}

void LayoutOverlayControls(HWND overlayWindow) {
    if (!IsWindow(overlayWindow)) {
        return;
    }

    const OverlayLayout& layout = GetOverlayLayout(overlayWindow);
    MoveOverlayControl(g_ui.enableButton, layout.enableButtonRect);
    MoveOverlayControl(g_ui.offsetXEdit, layout.offsetXEditRect);
    MoveOverlayControl(g_ui.offsetYEdit, layout.offsetYEditRect);
    MoveOverlayControl(g_ui.offsetZEdit, layout.offsetZEditRect);
    MoveOverlayControl(g_ui.lookSelfieLeftButton, layout.lookSelfieLeftButtonRect);
    MoveOverlayControl(g_ui.lookSelfieRightButton, layout.lookSelfieRightButtonRect);
    MoveOverlayControl(g_ui.lookForwardButton, layout.lookForwardButtonRect);
    MoveOverlayControl(g_ui.targetFollowButton, layout.targetFollowButtonRect);
    MoveOverlayControl(g_ui.targetLockButton, layout.targetLockButtonRect);
    MoveOverlayControl(g_ui.targetClearButton, layout.targetClearButtonRect);
    MoveOverlayControl(g_ui.overlayHotkeyButton, layout.hotkeyButtonRect);
    MoveOverlayControl(g_ui.statusEdit, layout.statusEditRect);
}

HWND CreateOverlayButton(HWND parent, int controlId, const wchar_t* text, int x, int y, int width, int height) {
    HWND control = CreateWindowExW(
        0,
        L"BUTTON",
        text,
        WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_OWNERDRAW,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        g_module,
        nullptr
    );

    if (control != nullptr) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(g_ui.bodyFont), TRUE);
    }

    return control;
}

HWND CreateOverlayEdit(HWND parent, int controlId, int x, int y, int width, int height, bool readOnly) {
    const DWORD style = readOnly
        ? (WS_CHILD | WS_VISIBLE | ES_LEFT | ES_MULTILINE | ES_AUTOVSCROLL | ES_READONLY | WS_VSCROLL)
        : (WS_CHILD | WS_VISIBLE | WS_TABSTOP | ES_CENTER | ES_AUTOHSCROLL);

    HWND control = CreateWindowExW(
        0,
        L"EDIT",
        L"",
        style,
        x,
        y,
        width,
        height,
        parent,
        reinterpret_cast<HMENU>(static_cast<INT_PTR>(controlId)),
        g_module,
        nullptr
    );

    if (control != nullptr) {
        SendMessageW(control, WM_SETFONT, reinterpret_cast<WPARAM>(readOnly ? g_ui.monoFont : g_ui.bodyFont), TRUE);
        SendMessageW(control, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, MAKELPARAM(readOnly ? 8 : 10, readOnly ? 8 : 10));
        if (!readOnly) {
            SendMessageW(control, EM_LIMITTEXT, 16, 0);
        }
    }

    return control;
}

bool CreateOverlayControls(HWND overlayWindow) {
    g_ui.enableButton = CreateOverlayButton(overlayWindow, kControlButtonEnable, Texts().uiButtonEnableOff, 0, 0, 1, 1);
    g_ui.offsetXEdit = CreateOverlayEdit(overlayWindow, kControlEditOffsetX, 0, 0, 1, 1, false);
    g_ui.offsetYEdit = CreateOverlayEdit(overlayWindow, kControlEditOffsetY, 0, 0, 1, 1, false);
    g_ui.offsetZEdit = CreateOverlayEdit(overlayWindow, kControlEditOffsetZ, 0, 0, 1, 1, false);
    g_ui.lookSelfieLeftButton = CreateOverlayButton(overlayWindow, kControlButtonLookSelfieLeft, Texts().uiButtonLookSelfieLeft, 0, 0, 1, 1);
    g_ui.lookSelfieRightButton = CreateOverlayButton(overlayWindow, kControlButtonLookSelfieRight, Texts().uiButtonLookSelfieRight, 0, 0, 1, 1);
    g_ui.lookForwardButton = CreateOverlayButton(overlayWindow, kControlButtonLookForward, Texts().uiButtonLookForward, 0, 0, 1, 1);
    g_ui.targetFollowButton = CreateOverlayButton(overlayWindow, kControlButtonTargetFollow, Texts().uiButtonTargetFollow, 0, 0, 1, 1);
    g_ui.targetLockButton = CreateOverlayButton(overlayWindow, kControlButtonTargetLock, Texts().uiButtonTargetLockCurrent, 0, 0, 1, 1);
    g_ui.targetClearButton = CreateOverlayButton(overlayWindow, kControlButtonTargetClear, Texts().uiButtonTargetClear, 0, 0, 1, 1);
    g_ui.overlayHotkeyButton = CreateOverlayButton(overlayWindow, kControlButtonPanelHotkey, BuildOverlayHotkeyButtonText().c_str(), 0, 0, 1, 1);
    g_ui.statusEdit = CreateOverlayEdit(overlayWindow, kControlEditStatus, 0, 0, 1, 1, true);

    const bool created = g_ui.enableButton != nullptr
        && g_ui.offsetXEdit != nullptr
        && g_ui.offsetYEdit != nullptr
        && g_ui.offsetZEdit != nullptr
        && g_ui.lookSelfieLeftButton != nullptr
        && g_ui.lookSelfieRightButton != nullptr
        && g_ui.lookForwardButton != nullptr
        && g_ui.targetFollowButton != nullptr
        && g_ui.targetLockButton != nullptr
        && g_ui.targetClearButton != nullptr
        && g_ui.overlayHotkeyButton != nullptr
        && g_ui.statusEdit != nullptr;

    if (created) {
        LayoutOverlayControls(overlayWindow);
    }

    return created;
}

bool EnsureOverlayWindow() {
    if (g_ui.overlayCreated && IsWindow(g_ui.overlayWindow)) {
        return true;
    }

    g_ui.overlayCreated = false;
    g_ui.overlayWindow = nullptr;
    g_ui.hostWindow = FindHostWindow();

    if (!IsWindow(g_ui.hostWindow)) {
        ConsolePrintf(Texts().logOverlayHostMissing);
        return false;
    }

    if (!EnsureOverlayResources()) {
        ConsolePrintf(Texts().logOverlayResourcesUnavailable);
        return false;
    }

    HWND overlayWindow = CreateWindowExW(
        kOverlayWindowExStyle,
        kOverlayWindowClassName,
        Texts().uiTitle,
        kOverlayWindowStyle,
        0,
        0,
        0,
        0,
        nullptr,
        nullptr,
        g_module,
        nullptr
    );

    if (overlayWindow == nullptr) {
        ConsolePrintf(Texts().logOverlayWindowCreationFailed, GetLastError());
        return false;
    }

    g_ui.overlayWindow = overlayWindow;
    g_ui.overlayCreated = true;
    g_ui.overlayVisible = false;
    g_ui.captureOverlayHotkey = false;
    g_ui.overlayPlacementInitialized = false;
    PositionOverlayWindow(false);
    ShowWindow(g_ui.overlayWindow, SW_HIDE);
    g_ui.hasLastRenderedState = false;
    MarkOverlayDirty();
    SyncOverlayFromState(true);
    return true;
}

void ShowOverlayWindow(bool visible) {
    if (visible) {
        if (!EnsureOverlayWindow()) {
            return;
        }

        MarkOverlayDirty();
        SyncOverlayFromState(true);
        PositionOverlayWindow(true);
        g_ui.overlayVisible = true;
        ShowWindow(g_ui.overlayWindow, SW_SHOW);
        SetActiveWindow(g_ui.overlayWindow);
        SetForegroundWindow(g_ui.overlayWindow);
        SetFocus(g_ui.overlayWindow);
        return;
    }

    g_ui.overlayVisible = false;
    g_ui.captureOverlayHotkey = false;
    if (IsWindow(g_ui.overlayWindow)) {
        ShowWindow(g_ui.overlayWindow, SW_HIDE);
    }

    if (IsWindow(g_ui.hostWindow)) {
        SetActiveWindow(g_ui.hostWindow);
        SetForegroundWindow(g_ui.hostWindow);
    }
}

void ToggleOverlayVisibility() {
    ShowOverlayWindow(!g_ui.overlayVisible);
}

void ApplyOffsetFromEdits() {
    if (g_ui.suppressOffsetSync) {
        return;
    }

    float x{};
    float y{};
    float z{};
    if (!TryReadFloatFromEdit(g_ui.offsetXEdit, x) || !TryReadFloatFromEdit(g_ui.offsetYEdit, y) || !TryReadFloatFromEdit(g_ui.offsetZEdit, z)) {
        return;
    }

    SetOffset(Vec3{ x, y, z });
}

void ResetOffsetEditsFromState() {
    const RuntimeState state = SnapshotState();
    g_ui.suppressOffsetSync = true;
    SetEditFloatText(g_ui.offsetXEdit, state.offset.x);
    SetEditFloatText(g_ui.offsetYEdit, state.offset.y);
    SetEditFloatText(g_ui.offsetZEdit, state.offset.z);
    g_ui.suppressOffsetSync = false;
}

void StartOverlayHotkeyCapture() {
    g_ui.captureOverlayHotkey = true;
    g_ui.overlayHotkeyWasDown = true;
    SetFocus(g_ui.overlayWindow);
    SyncOverlayFromState(true);
    InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
}

void StopOverlayHotkeyCapture(bool refresh) {
    g_ui.captureOverlayHotkey = false;
    if (refresh && IsWindow(g_ui.overlayWindow)) {
        SyncOverlayFromState(true);
        InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
    }
}

bool HandleOverlayHotkeyCaptureKeyDown(WPARAM wParam) {
    if (!g_ui.captureOverlayHotkey) {
        return false;
    }

    const int virtualKey = static_cast<int>(wParam);
    if (virtualKey == VK_ESCAPE) {
        StopOverlayHotkeyCapture(true);
        return true;
    }

    if (IsMouseVirtualKey(virtualKey) || IsModifierVirtualKey(virtualKey)) {
        return true;
    }

    SetOverlayHotkeyVirtualKey(virtualKey);
    StopOverlayHotkeyCapture(true);
    return true;
}

void HandleOverlayCommand(WPARAM wParam) {
    const int controlId = LOWORD(wParam);
    const int notifyCode = HIWORD(wParam);

    if (notifyCode == BN_CLICKED) {
        switch (controlId) {
        case kControlButtonEnable:
            ConsolePrintf(Texts().logOverlayEnableClicked);
            ToggleEnabled();
            break;
        case kControlButtonLookSelfieLeft:
            SetLookMode(LookMode::SelfieLeft);
            break;
        case kControlButtonLookSelfieRight:
            SetLookMode(LookMode::SelfieRight);
            break;
        case kControlButtonLookForward:
            SetLookMode(LookMode::Forward);
            break;
        case kControlButtonTargetFollow:
            SetTargetMode(TargetMode::Follow);
            break;
        case kControlButtonTargetLock:
            HandleLockCurrent();
            break;
        case kControlButtonTargetClear:
            ClearLockedTarget();
            break;
        case kControlButtonPanelHotkey:
            StartOverlayHotkeyCapture();
            break;
        default:
            break;
        }

        SyncOverlayFromState(true);
        return;
    }

    const bool isOffsetEdit = controlId == kControlEditOffsetX || controlId == kControlEditOffsetY || controlId == kControlEditOffsetZ;
    if (!isOffsetEdit) {
        return;
    }

    if (notifyCode == EN_CHANGE) {
        ApplyOffsetFromEdits();
        return;
    }

    if (notifyCode == EN_SETFOCUS) {
        InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
        return;
    }

    if (notifyCode == EN_KILLFOCUS) {
        float x{};
        float y{};
        float z{};
        if (!TryReadFloatFromEdit(g_ui.offsetXEdit, x) || !TryReadFloatFromEdit(g_ui.offsetYEdit, y) || !TryReadFloatFromEdit(g_ui.offsetZEdit, z)) {
            ResetOffsetEditsFromState();
            InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
            return;
        }

        ApplyOffsetFromEdits();
        InvalidateRect(g_ui.overlayWindow, nullptr, FALSE);
    }
}

struct OverlayButtonVisual {
    COLORREF background;
    COLORREF border;
    COLORREF text;
};

OverlayButtonVisual GetOverlayButtonVisual(int controlId, const RuntimeState& state, UINT itemState) {
    OverlayButtonVisual visual{ kColorPanelElevated, kColorLine, kColorTextMuted };

    bool active = false;
    bool warning = false;

    switch (controlId) {
    case kControlButtonEnable:
        active = state.enabled;
        break;
    case kControlButtonLookSelfieLeft:
        active = state.lookMode == LookMode::SelfieLeft;
        break;
    case kControlButtonLookSelfieRight:
        active = state.lookMode == LookMode::SelfieRight;
        break;
    case kControlButtonLookForward:
        active = state.lookMode == LookMode::Forward;
        break;
    case kControlButtonTargetFollow:
        active = state.targetMode == TargetMode::Follow;
        break;
    case kControlButtonTargetLock:
        active = state.targetMode == TargetMode::Locked && state.lockedHandle.IsValid();
        break;
    case kControlButtonTargetClear:
        warning = state.lockedHandle.IsValid();
        break;
    case kControlButtonPanelHotkey:
        active = g_ui.captureOverlayHotkey;
        break;
    default:
        break;
    }

    if ((itemState & ODS_DISABLED) != 0) {
        return visual;
    }

    if (warning) {
        visual.background = RGB(53, 32, 24);
        visual.border = kColorWarn;
        visual.text = kColorText;
    }

    if (active) {
        visual.background = kColorAccentSoft;
        visual.border = kColorAccentStrong;
        visual.text = kColorText;
    }

    if ((itemState & ODS_SELECTED) != 0) {
        if (active) {
            visual.background = kColorAccent;
        }
        else if (warning) {
            visual.background = RGB(80, 48, 33);
        }
        else {
            visual.background = RGB(28, 38, 53);
            visual.border = kColorAccentStrong;
            visual.text = kColorText;
        }
    }

    return visual;
}

LRESULT HandleOverlayDrawItem(const DRAWITEMSTRUCT* drawItem) {
    if (drawItem == nullptr || drawItem->CtlType != ODT_BUTTON) {
        return FALSE;
    }

    const RuntimeState state = SnapshotState();
    const OverlayButtonVisual visual = GetOverlayButtonVisual(static_cast<int>(drawItem->CtlID), state, drawItem->itemState);

    RECT frameRect = drawItem->rcItem;
    const bool focused = (drawItem->itemState & ODS_FOCUS) != 0;
    const COLORREF borderColor = focused ? kColorAccentStrong : visual.border;
    const COLORREF innerBorderColor = focused ? kColorAccent : kColorLine;

    if (HBRUSH frameBrush = GetCachedSolidBrush(kColorPanelInset)) {
        FillRect(drawItem->hDC, &frameRect, frameBrush);
    }

    RECT innerRect = frameRect;
    InflateRect(&innerRect, -1, -1);
    if (HBRUSH innerBrush = GetCachedSolidBrush(visual.background)) {
        FillRect(drawItem->hDC, &innerRect, innerBrush);
    }

    RECT accentRect = innerRect;
    accentRect.bottom = accentRect.top + 3;
    if (HBRUSH accentBrush = GetCachedSolidBrush(borderColor)) {
        FillRect(drawItem->hDC, &accentRect, accentBrush);
    }

    RECT sideAccentRect = innerRect;
    sideAccentRect.right = sideAccentRect.left + 3;
    if (HBRUSH sideAccentBrush = GetCachedSolidBrush(borderColor)) {
        FillRect(drawItem->hDC, &sideAccentRect, sideAccentBrush);
    }

    if (HBRUSH borderBrush = GetCachedSolidBrush(borderColor)) {
        FrameRect(drawItem->hDC, &frameRect, borderBrush);
    }

    if (HBRUSH innerBorderBrush = GetCachedSolidBrush(innerBorderColor)) {
        FrameRect(drawItem->hDC, &innerRect, innerBorderBrush);
    }

    RECT textRect = innerRect;
    InflateRect(&textRect, -12, -4);
    textRect.left += 4;
    SetBkMode(drawItem->hDC, TRANSPARENT);
    SetTextColor(drawItem->hDC, visual.text);
    SelectObject(drawItem->hDC, g_ui.bodyFont);

    const std::wstring text = GetOverlayButtonText(static_cast<int>(drawItem->CtlID), drawItem->hwndItem, state);
    DrawTextW(drawItem->hDC, text.c_str(), -1, &textRect, DT_CENTER | DT_VCENTER | DT_SINGLELINE);

    return TRUE;
}

void FillSolidRect(HDC dc, const RECT& rect, COLORREF color) {
    if (HBRUSH brush = GetCachedSolidBrush(color)) {
        FillRect(dc, &rect, brush);
    }
}

void DrawRectOutline(HDC dc, const RECT& rect, COLORREF color) {
    if (HBRUSH brush = GetCachedSolidBrush(color)) {
        FrameRect(dc, &rect, brush);
    }
}

RECT GetChildRectInParent(HWND parent, HWND child) {
    RECT rect{};
    if (parent == nullptr || child == nullptr) {
        return rect;
    }

    GetWindowRect(child, &rect);
    MapWindowPoints(HWND_DESKTOP, parent, reinterpret_cast<LPPOINT>(&rect), 2);
    return rect;
}

bool ControlHasFocus(HWND control) {
    return control != nullptr && GetFocus() == control;
}

void DrawSectionCard(HDC dc, const RECT& rect, COLORREF fillColor, COLORREF borderColor) {
    FillSolidRect(dc, rect, fillColor);
    DrawRectOutline(dc, rect, borderColor);

    RECT innerRect = rect;
    InflateRect(&innerRect, -1, -1);
    DrawRectOutline(dc, innerRect, kColorPanelInset);

    RECT accentRect = innerRect;
    accentRect.bottom = accentRect.top + 3;
    FillSolidRect(dc, accentRect, borderColor);
}

void DrawOverlayText(HDC dc, const wchar_t* text, const RECT& rect, HFONT font, COLORREF color, UINT format);

void DrawChip(HDC dc, const RECT& rect, const wchar_t* text, HFONT font, COLORREF fillColor, COLORREF borderColor, COLORREF textColor) {
    FillSolidRect(dc, rect, fillColor);
    DrawRectOutline(dc, rect, borderColor);
    RECT textRect = rect;
    InflateRect(&textRect, -10, -2);
    DrawOverlayText(dc, text, textRect, font, textColor, DT_CENTER | DT_VCENTER | DT_SINGLELINE);
}

void DrawInputChrome(HDC dc, HWND parent, HWND control, COLORREF borderColor) {
    RECT rect = GetChildRectInParent(parent, control);
    if (rect.left == rect.right || rect.top == rect.bottom) {
        return;
    }

    RECT outerRect = rect;
    InflateRect(&outerRect, 2, 2);
    FillSolidRect(dc, outerRect, kColorPanelInset);
    DrawRectOutline(dc, outerRect, borderColor);

    RECT innerRect = outerRect;
    InflateRect(&innerRect, -1, -1);
    DrawRectOutline(dc, innerRect, kColorLine);

    RECT accentRect = innerRect;
    accentRect.bottom = accentRect.top + 3;
    FillSolidRect(dc, accentRect, borderColor);
}

const wchar_t* GetOverlayLifecycleText(const RuntimeState& state) {
    if (!state.initialized) {
        return Texts().uiLifecycleBoot;
    }

    if (!state.enabled) {
        return Texts().uiLifecycleIdle;
    }

    if (state.lastOverrideSucceeded) {
        return Texts().uiLifecycleLive;
    }

    return Texts().uiLifecycleBlocked;
}

COLORREF GetOverlayLifecycleFill(const RuntimeState& state) {
    if (!state.initialized) {
        return kColorPanelElevated;
    }

    if (!state.enabled) {
        return kColorAccentSoft;
    }

    if (state.lastOverrideSucceeded) {
        return kColorSuccessSoft;
    }

    return kColorDangerSoft;
}

COLORREF GetOverlayLifecycleBorder(const RuntimeState& state) {
    if (!state.initialized) {
        return kColorLineStrong;
    }

    if (!state.enabled) {
        return kColorAccent;
    }

    if (state.lastOverrideSucceeded) {
        return kColorSuccess;
    }

    return kColorDanger;
}

void DrawOverlayText(HDC dc, const wchar_t* text, const RECT& rect, HFONT font, COLORREF color, UINT format) {
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, color);
    SelectObject(dc, font);
    RECT drawRect = rect;
    DrawTextW(dc, text, -1, &drawRect, format);
}

void DrawOverlayWindow(HDC dc, HWND window) {
    const RuntimeState state = SnapshotState();
    const OverlayLayout& layout = GetOverlayLayout(window);
    const RECT& clientRect = layout.clientRect;
    std::wstring fallbackHotkeyChipText;
    const wchar_t* hotkeyChipText = g_ui.lastRenderedHotkeyChipText.c_str();
    if (g_ui.lastRenderedHotkeyChipText.empty()) {
        fallbackHotkeyChipText = BuildOverlayHotkeyChipText(state.overlayHotkeyVirtualKey);
        hotkeyChipText = fallbackHotkeyChipText.c_str();
    }
    FillSolidRect(dc, clientRect, kColorPanel);
    DrawRectOutline(dc, clientRect, kColorLineStrong);

    RECT innerPanel = clientRect;
    InflateRect(&innerPanel, -1, -1);
    DrawRectOutline(dc, innerPanel, kColorPanelInset);

    RECT accentBar{ 1, 1, clientRect.right - 1, 4 };
    FillSolidRect(dc, accentBar, kColorAccent);

    DrawSectionCard(dc, layout.headerRect, kColorPanelStrong, kColorLineStrong);
    DrawSectionCard(dc, layout.enableCardRect, state.enabled ? kColorPanelElevated : kColorPanelStrong, state.enabled ? kColorAccentStrong : kColorLineStrong);
    DrawSectionCard(dc, layout.offsetCardRect, kColorPanelStrong, kColorLineStrong);
    DrawSectionCard(dc, layout.lookCardRect, kColorPanelStrong, kColorAccentStrong);
    DrawSectionCard(dc, layout.targetCardRect, kColorPanelStrong, state.targetMode == TargetMode::Locked ? kColorWarn : kColorLineStrong);
    DrawSectionCard(dc, layout.statusCardRect, kColorPanelStrong, state.lastOverrideSucceeded ? kColorSuccess : (state.enabled ? kColorDanger : kColorLineStrong));

    DrawOverlayText(dc, Texts().uiTitle, layout.titleRect, g_ui.titleFont, kColorAccentStrong, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, Texts().uiSubtitle, layout.subtitleRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawChip(dc, layout.statusChipRect, GetOverlayLifecycleText(state), g_ui.bodyFont, GetOverlayLifecycleFill(state), GetOverlayLifecycleBorder(state), kColorText);
    DrawChip(dc, layout.hotkeyChipRect, hotkeyChipText, g_ui.bodyFont, g_ui.captureOverlayHotkey ? kColorAccentSoft : kColorPanelElevated, g_ui.captureOverlayHotkey ? kColorAccentStrong : kColorLineStrong, kColorText);

    DrawOverlayText(dc, Texts().uiEnableSection, layout.enableLabelRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawChip(dc, layout.enableChipRect, state.enabled ? Texts().uiEnableChipOn : Texts().uiEnableChipOff, g_ui.bodyFont, state.enabled ? kColorAccentSoft : kColorPanelElevated, state.enabled ? kColorAccentStrong : kColorLineStrong, kColorText);

    DrawOverlayText(dc, Texts().uiOffsetSection, layout.offsetLabelRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, L"R", layout.offsetXLabelRect, g_ui.bodyFont, kColorText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, L"B", layout.offsetYLabelRect, g_ui.bodyFont, kColorText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, L"U", layout.offsetZLabelRect, g_ui.bodyFont, kColorText, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, Texts().uiOffsetHint, layout.offsetHintRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);

    DrawOverlayText(dc, Texts().uiDirectionSection, layout.lookLabelRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawChip(dc, layout.lookChipRect, GetOverlayDirectionChipText(state.lookMode), g_ui.bodyFont, kColorPanelElevated, kColorAccentStrong, kColorText);

    DrawOverlayText(dc, Texts().uiTargetSection, layout.targetLabelRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawChip(dc, layout.targetChipRect, state.targetMode == TargetMode::Locked ? Texts().uiTargetChipLocked : Texts().uiTargetChipFollow, g_ui.bodyFont, state.targetMode == TargetMode::Locked ? RGB(53, 32, 24) : kColorPanelElevated, state.targetMode == TargetMode::Locked ? kColorWarn : kColorLineStrong, kColorText);

    DrawOverlayText(dc, Texts().uiRuntimeSection, layout.statusLabelRect, g_ui.bodyFont, kColorTextMuted, DT_LEFT | DT_VCENTER | DT_SINGLELINE);
    DrawOverlayText(dc, Texts().uiRuntimeSubsection, layout.statusSubRect, g_ui.bodyFont, kColorTextMuted, DT_RIGHT | DT_VCENTER | DT_SINGLELINE);

    DrawInputChrome(dc, window, g_ui.offsetXEdit, ControlHasFocus(g_ui.offsetXEdit) ? kColorAccentStrong : kColorLineStrong);
    DrawInputChrome(dc, window, g_ui.offsetYEdit, ControlHasFocus(g_ui.offsetYEdit) ? kColorAccentStrong : kColorLineStrong);
    DrawInputChrome(dc, window, g_ui.offsetZEdit, ControlHasFocus(g_ui.offsetZEdit) ? kColorAccentStrong : kColorLineStrong);
    DrawInputChrome(dc, window, g_ui.statusEdit, state.lastOverrideSucceeded ? kColorSuccess : (state.enabled ? kColorDanger : kColorLineStrong));
}

void HandleBuiltInActionHotkeys() {
    if (g_ui.overlayVisible || !IsCurrentProcessForeground()) {
        return;
    }

    const bool toggleDown = (GetAsyncKeyState(kToggleEnabledHotkeyVirtualKey) & 0x8000) != 0;
    if (toggleDown && !g_ui.toggleHotkeyWasDown) {
        ToggleEnabled();
        ConsolePrintf(Texts().logToggleViaF8);
    }
    g_ui.toggleHotkeyWasDown = toggleDown;

    const bool lockDown = (GetAsyncKeyState(kLockCurrentHotkeyVirtualKey) & 0x8000) != 0;
    if (lockDown && !g_ui.lockHotkeyWasDown) {
        HandleLockCurrent();
    }
    g_ui.lockHotkeyWasDown = lockDown;

    const bool followDown = (GetAsyncKeyState(kFollowTargetHotkeyVirtualKey) & 0x8000) != 0;
    if (followDown && !g_ui.followHotkeyWasDown) {
        SetTargetMode(TargetMode::Follow);
        ConsolePrintf(Texts().logTargetFollowViaF10);
    }
    g_ui.followHotkeyWasDown = followDown;
}

void HandleOverlayTimer() {
    if (!g_ui.captureOverlayHotkey) {
        const int overlayHotkeyVirtualKey = GetConfiguredOverlayHotkeyVirtualKey();
        const bool hotkeyDown = (GetAsyncKeyState(overlayHotkeyVirtualKey) & 0x8000) != 0;
        if (hotkeyDown && !g_ui.overlayHotkeyWasDown && IsOverlayHotkeyContextActive()) {
            ToggleOverlayVisibility();
        }
        g_ui.overlayHotkeyWasDown = hotkeyDown;
    }

    HandleBuiltInActionHotkeys();

    if (!g_ui.overlayCreated || !IsWindow(g_ui.overlayWindow)) {
        return;
    }

    if (!g_ui.overlayVisible) {
        return;
    }

    if (g_ui.dirty.exchange(false, std::memory_order_relaxed)) {
        SyncOverlayFromState();
    }
}

DWORD WINAPI OverlayUiThreadMain(LPVOID) {
    INITCOMMONCONTROLSEX commonControls{};
    commonControls.dwSize = sizeof(commonControls);
    commonControls.dwICC = ICC_STANDARD_CLASSES;
    InitCommonControlsEx(&commonControls);

    if (!RegisterOverlayWindowClasses()) {
        SetInitialized(false, Texts().reasonOverlayWindowClassRegistrationFailed);
        ConsolePrintf(Texts().logOverlayClassRegistrationFailed);
        g_uiThreadStarted = false;
        return 0;
    }

    g_ui.controllerWindow = CreateWindowExW(
        0,
        kOverlayControllerClassName,
        L"SelfiestickController",
        0,
        0,
        0,
        0,
        0,
        HWND_MESSAGE,
        nullptr,
        g_module,
        nullptr
    );

    if (g_ui.controllerWindow == nullptr) {
        SetInitialized(false, Texts().reasonOverlayControllerCreationFailed);
        ConsolePrintf(Texts().logOverlayControllerCreationFailed, GetLastError());
        g_uiThreadStarted = false;
        return 0;
    }

    SetTimer(g_ui.controllerWindow, kOverlayTimerId, kOverlayTimerIntervalMs, nullptr);

    MSG message{};
    while (GetMessageW(&message, nullptr, 0, 0) > 0) {
        if (g_ui.overlayVisible && !g_ui.captureOverlayHotkey && IsOverlayToggleKeyMessage(message)) {
            continue;
        }

        if (g_ui.overlayVisible && IsWindow(g_ui.overlayWindow) && IsDialogMessageW(g_ui.overlayWindow, &message)) {
            continue;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    DestroyOverlayResources();
    g_ui.controllerWindow = nullptr;
    g_ui.hostWindow = nullptr;
    g_ui.overlayWindow = nullptr;
    g_ui.enableButton = nullptr;
    g_ui.offsetXEdit = nullptr;
    g_ui.offsetYEdit = nullptr;
    g_ui.offsetZEdit = nullptr;
    g_ui.lookSelfieLeftButton = nullptr;
    g_ui.lookSelfieRightButton = nullptr;
    g_ui.lookForwardButton = nullptr;
    g_ui.targetFollowButton = nullptr;
    g_ui.targetLockButton = nullptr;
    g_ui.targetClearButton = nullptr;
    g_ui.overlayHotkeyButton = nullptr;
    g_ui.statusEdit = nullptr;
    g_ui.titleFont = nullptr;
    g_ui.bodyFont = nullptr;
    g_ui.monoFont = nullptr;
    g_ui.panelBrush = nullptr;
    g_ui.editBrush = nullptr;
    g_ui.overlayCreated = false;
    g_ui.overlayVisible = false;
    g_ui.suppressOffsetSync = false;
    g_ui.captureOverlayHotkey = false;
    g_ui.overlayHotkeyWasDown = false;
    g_ui.toggleHotkeyWasDown = false;
    g_ui.lockHotkeyWasDown = false;
    g_ui.followHotkeyWasDown = false;
    g_ui.overlayPlacementInitialized = false;
    g_ui.hasLastHostRect = false;
    g_ui.lastHostRect = RECT{};
    g_ui.hasLastRenderedState = false;
    g_ui.lastRenderedState = RuntimeState{};
    g_ui.lastRenderedStatusText.clear();
    g_ui.dirty.store(true, std::memory_order_relaxed);
    g_uiThreadStarted = false;
    return 0;
}

bool StartOverlayUiThread() {
    bool expected = false;
    if (!g_uiThreadStarted.compare_exchange_strong(expected, true)) {
        return true;
    }

    HANDLE thread = CreateThread(nullptr, 0, &OverlayUiThreadMain, nullptr, 0, nullptr);
    if (thread == nullptr) {
        g_uiThreadStarted = false;
        return false;
    }

    CloseHandle(thread);
    return true;
}

LRESULT CALLBACK OverlayControllerWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_TIMER:
        if (wParam == kOverlayTimerId) {
            HandleOverlayTimer();
            return 0;
        }
        break;
    case kOverlayShutdownMessage:
        DestroyWindow(window);
        return 0;
    case WM_DESTROY:
        KillTimer(window, kOverlayTimerId);
        if (IsWindow(g_ui.overlayWindow)) {
            DestroyWindow(g_ui.overlayWindow);
        }
        g_ui.controllerWindow = nullptr;
        PostQuitMessage(0);
        return 0;
    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

LRESULT CALLBACK OverlayWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam) {
    switch (message) {
    case WM_CREATE:
        if (!CreateOverlayControls(window)) {
            return -1;
        }
        MarkOverlayDirty();
        SyncOverlayFromState(true);
        return 0;
    case WM_GETMINMAXINFO: {
        auto* minMaxInfo = reinterpret_cast<MINMAXINFO*>(lParam);
        if (minMaxInfo != nullptr) {
            const SIZE minSize = GetOverlayWindowSizeForClient(kOverlayMinWidth, kOverlayMinHeight);
            minMaxInfo->ptMinTrackSize.x = minSize.cx;
            minMaxInfo->ptMinTrackSize.y = minSize.cy;
        }
        return 0;
    }
    case WM_SIZE:
        LayoutOverlayControls(window);
        InvalidateRect(window, nullptr, FALSE);
        return 0;
    case WM_COMMAND:
        HandleOverlayCommand(wParam);
        return 0;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (HandleOverlayHotkeyCaptureKeyDown(wParam)) {
            return 0;
        }
        break;
    case WM_DRAWITEM:
        return HandleOverlayDrawItem(reinterpret_cast<DRAWITEMSTRUCT*>(lParam));
    case WM_CTLCOLOREDIT:
        SetTextColor(reinterpret_cast<HDC>(wParam), kColorText);
        SetBkColor(reinterpret_cast<HDC>(wParam), kColorInput);
        return reinterpret_cast<LRESULT>(g_ui.editBrush);
    case WM_CTLCOLORSTATIC:
        SetTextColor(reinterpret_cast<HDC>(wParam), kColorText);
        SetBkMode(reinterpret_cast<HDC>(wParam), TRANSPARENT);
        if (reinterpret_cast<HWND>(lParam) == g_ui.statusEdit) {
            SetBkColor(reinterpret_cast<HDC>(wParam), kColorInput);
            return reinterpret_cast<LRESULT>(g_ui.editBrush);
        }
        return reinterpret_cast<LRESULT>(g_ui.panelBrush);
    case WM_ERASEBKGND:
        return 1;
    case WM_PAINT: {
        PAINTSTRUCT paint{};
        HDC dc = BeginPaint(window, &paint);
        DrawOverlayWindow(dc, window);
        EndPaint(window, &paint);
        return 0;
    }
    case WM_CLOSE:
        ShowOverlayWindow(false);
        return 0;
    case WM_DESTROY:
        g_ui.overlayCreated = false;
        g_ui.overlayVisible = false;
        g_ui.hasLastHostRect = false;
        g_ui.hasLastRenderedState = false;
        g_ui.overlayWindow = nullptr;
        g_ui.enableButton = nullptr;
        g_ui.offsetXEdit = nullptr;
        g_ui.offsetYEdit = nullptr;
        g_ui.offsetZEdit = nullptr;
        g_ui.lookSelfieLeftButton = nullptr;
        g_ui.lookSelfieRightButton = nullptr;
        g_ui.lookForwardButton = nullptr;
        g_ui.targetFollowButton = nullptr;
        g_ui.targetLockButton = nullptr;
        g_ui.targetClearButton = nullptr;
        g_ui.overlayHotkeyButton = nullptr;
        g_ui.statusEdit = nullptr;
        g_ui.captureOverlayHotkey = false;
        g_ui.overlayPlacementInitialized = false;
        return 0;
    default:
        break;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}

DWORD InitializeSelfieStickImpl() {
    {
        std::lock_guard lock(g_stateMutex);
        RefreshStatusTextLocked();
    }

    LoadPersistedOverlayHotkeySetting();
    LoadDebugOptions();
    TracePrintf(
        "init trace=%d disableScopedFov=%d disableGunStabilization=%d",
        g_debugOptions.traceEnabled ? 1 : 0,
        g_debugOptions.disableScopedFovOverride ? 1 : 0,
        g_debugOptions.disableGunStabilization ? 1 : 0
    );
    TracePrintf("trace-version schema=%s build=%s", kSchemaProbeTraceVersion, kBuildTag);

    g_initStage.store(1, std::memory_order_relaxed);
    TracePrintf("init-stage stage=%d name=%s enter", 1, InitStageToString(1));
    const bool overlayUiStarted = StartOverlayUiThread();
    TracePrintf("init-stage stage=%d name=%s result=%d", 1, InitStageToString(1), overlayUiStarted ? 1 : 0);
    if (!overlayUiStarted) {
        SetInitialized(false, Texts().reasonOverlayUiThreadCreationFailed);
        ConsolePrintf(Texts().logOverlayUiThreadCreationFailed);
    }

    g_initStage.store(2, std::memory_order_relaxed);
    TracePrintf("init-stage stage=%d name=%s enter", 2, InitStageToString(2));
    if (!LoadInterfaces()) {
        TracePrintf("init-stage stage=%d name=%s failed reason=%s", 2, InitStageToString(2), SnapshotState().lastFailureReason.c_str());
        ConsolePrintf(Texts().logInterfaceInitializationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }
    TracePrintf("init-stage stage=%d name=%s ok", 2, InitStageToString(2));

    g_initStage.store(3, std::memory_order_relaxed);
    g_schemaOffsetsResolved.store(false, std::memory_order_release);
    g_runtimeCompatibility.schemaOffsetsReady = false;
    g_typeScopeClassLookupVtableIndex = -1;
    g_typeScopeClassLookupMode = TypeScopeClassLookupMode::Unknown;
    g_findDeclaredClassByQualifiedName = nullptr;
    g_schemaQualifiedClassLookupAddress = 0u;
    {
        std::lock_guard lock(g_schemaResolveMutex);
        g_schemaResolveFailureReason.clear();
    }
    g_lastSchemaResolveAttemptTickMs.store(0ull, std::memory_order_relaxed);
    g_schemaResolveInProgress.store(false, std::memory_order_release);
    const bool schemaResolveStarted = StartBackgroundSchemaResolve("InitializeSelfieStick", true);
    TracePrintf(
        "init-stage stage=%d name=%s deferred backgroundStarted=%d",
        3,
        InitStageToString(3),
        schemaResolveStarted ? 1 : 0
    );

    g_initStage.store(4, std::memory_order_relaxed);
    TracePrintf("init-stage stage=%d name=%s enter", 4, InitStageToString(4));
    if (!ResolveEntityAccess()) {
        TracePrintf("init-stage stage=%d name=%s failed reason=%s", 4, InitStageToString(4), SnapshotState().lastFailureReason.c_str());
        ConsolePrintf(Texts().logEntityInitializationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }
    TracePrintf("init-stage stage=%d name=%s ok", 4, InitStageToString(4));

    WaitForStartupSchemaResolve("InitializeSelfieStick");

    g_initStage.store(5, std::memory_order_relaxed);
    TracePrintf("init-stage stage=%d name=%s enter", 5, InitStageToString(5));
    const bool setupViewHooked = HookSetUpView();
    constexpr bool frameStageHooked = false;
    SetHookFlags(setupViewHooked, frameStageHooked);
    TracePrintf("init-stage stage=%d name=%s result=%d", 5, InitStageToString(5), setupViewHooked ? 1 : 0);

    if (!setupViewHooked) {
        ConsolePrintf(Texts().logHookInstallationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }

    if (!overlayUiStarted) {
        return 0;
    }

    g_initStage.store(6, std::memory_order_relaxed);
    TracePrintf("init-stage stage=%d name=%s ok", 6, InitStageToString(6));
    SetInitialized(true, "");
    ConsolePrintf(Texts().logBuild, kBuildTag);
    ConsolePrintf(Texts().logReady);
    return 0;
}

DWORD WINAPI InitializeSelfieStick(LPVOID) {
    return InitializeSelfieStickImpl();
}

}

BOOL APIENTRY DllMain(HMODULE module, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        selfiestick::g_module = module;
        DisableThreadLibraryCalls(module);
        CreateThread(nullptr, 0, &selfiestick::InitializeSelfieStick, nullptr, 0, nullptr);
        break;
    case DLL_PROCESS_DETACH:
    default:
        break;
    }

    return TRUE;
}
