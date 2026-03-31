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
#include <sstream>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

#pragma comment(lib, "Comctl32.lib")

namespace selfiestick {

constexpr wchar_t kClientModuleName[] = L"client.dll";
constexpr wchar_t kSchemaModuleName[] = L"schemasystem.dll";
constexpr wchar_t kTier0ModuleName[] = L"tier0.dll";
constexpr wchar_t kOverlayControllerClassName[] = L"SelfiestickControllerWindow";
constexpr wchar_t kOverlayWindowClassName[] = L"SelfiestickOverlayWindow";
constexpr char kBuildTag[] = __DATE__ " " __TIME__;

constexpr char kAnchorAttachmentName[] = "weapon_hand_r";
constexpr char kGunSideAttachmentName[] = "weapon_hand_l";
constexpr char kGunAnchorAttachmentName[] = "muzzle_flash";
constexpr char kSource2ClientVersion[] = "Source2Client002";

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
constexpr Vec3 kGunSelfieBaseOffset{ 10.5f, 12.5f, 6.75f };
constexpr Vec3 kGunForwardBaseOffset{ 6.5f, 16.25f, -2.5f };
constexpr Vec3 kWorldUp{ 0.0f, 0.0f, 1.0f };
constexpr float kGunUpperBodyTargetBlendFactor = 0.34f;
constexpr float kGunSelfieTargetCrossBias = 3.75f;
constexpr float kGunSelfieTargetUpBias = 0.15f;

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
    std::ptrdiff_t observerMode{};
    std::ptrdiff_t observerTarget{};
    std::ptrdiff_t cameraViewEntity{};
    std::ptrdiff_t activeWeapon{};
    std::ptrdiff_t sceneNode{};
    std::ptrdiff_t sceneAbsOrigin{};
};

struct SchemaField {
    const char* name;
    void* type;
    std::uint32_t offset;
    std::uint32_t metadataSize;
    void* metadata;
};

struct SchemaClass {
    void* vtable;
    const char* name;
    const char* moduleName;
    std::uint32_t size;
    std::uint16_t numFields;
    std::byte padding0[2];
    std::uint16_t staticSize;
    std::uint16_t metadataSize;
    std::byte padding1[4];
    SchemaField* fields;
};

struct SchemaDeclaredClass {
    void* vtable;
    const char* name;
    const char* moduleName;
    const char* unknown;
    SchemaClass* schemaClass;
};

struct SchemaDeclaredClassEntry {
    std::uint64_t hash[2];
    SchemaDeclaredClass* declaredClass;
};

struct SchemaTypeScope {
    void* vtable;
    char name[256];
    std::byte padding[0x368];
    std::uint16_t numDeclaredClasses;
    std::byte padding1[6];
    SchemaDeclaredClassEntry* declaredClasses;
};

struct SchemaSystem {
    std::byte padding[0x190];
    std::uint64_t scopeSize;
    SchemaTypeScope** scopeArray;
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

using CreateInterfaceFn = void* (*)(const char* name, int* returnCode);
using GetHighestEntityIndexFn = int(__fastcall*)(void* entityList, bool includeUnknown);
using GetEntityFromIndexFn = void* (__fastcall*)(void* entityList, int index);
using LookupAttachmentFn = void(__fastcall*)(void* entity, std::uint8_t& outIndex, const char* attachmentName);
using GetAttachmentFn = bool(__fastcall*)(void* entity, std::uint8_t attachmentIndex, void* outputTransform);
using GetSplitScreenPlayerFn = void* (__fastcall*)(int slot);
using FrameStageNotifyFn = void(__fastcall*)(void* thisPointer, int stage);
using Tier0MsgFn = void(__cdecl*)(const char*, ...);

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
std::array<std::uint8_t, 17> g_setupViewOriginalBytes{};
Tier0MsgFn g_tier0Msg{};
ClientOffsets g_offsets{};

std::mutex g_stateMutex;
std::mutex g_gunBasisContinuityMutex;
std::mutex g_gunAnchorStabilityMutex;
std::mutex g_scopedFovOverrideMutex;
std::mutex g_solidBrushCacheMutex;
RuntimeState g_state;
OverlayUiState g_ui;
GunBasisContinuityState g_gunBasisContinuity;
GunAnchorStabilityState g_gunAnchorStability;
ScopedFovOverrideState g_scopedFovOverride;
std::atomic<bool> g_uiThreadStarted{false};
std::atomic<bool> g_setupViewCallbackObserved{false};
std::atomic<bool> g_enabledSetUpViewObserved{false};
std::unordered_map<COLORREF, HBRUSH> g_solidBrushCache;

void RefreshStatusTextLocked();
RuntimeState SnapshotState();
void ConsolePrintf(const char* format, ...);
void MarkOverlayDirty();

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

bool ResolveSchemaOffsets() {
    const auto schemaInstruction = FindPattern(g_schemaModule, kPatternSchemaSystem);
    if (schemaInstruction == 0) {
        SetInitialized(false, Texts().reasonSchemaPatternMissing);
        return false;
    }

    auto* schemaSystem = reinterpret_cast<SchemaSystem*>(schemaInstruction + *reinterpret_cast<std::int32_t*>(schemaInstruction + 3) + 7);
    if (schemaSystem == nullptr || schemaSystem->scopeArray == nullptr) {
        SetInitialized(false, Texts().reasonSchemaSystemUnavailable);
        return false;
    }

    std::unordered_map<std::string, std::unordered_map<std::string, std::ptrdiff_t>> fields;

    for (std::uint64_t scopeIndex = 0; scopeIndex < schemaSystem->scopeSize; ++scopeIndex) {
        auto* scope = schemaSystem->scopeArray[scopeIndex];
        if (scope == nullptr || scope->declaredClasses == nullptr) {
            continue;
        }

        if (std::strcmp(scope->name, "client.dll") != 0) {
            continue;
        }

        for (std::uint16_t classIndex = 0; classIndex < scope->numDeclaredClasses; ++classIndex) {
            auto* declaredClass = scope->declaredClasses[classIndex].declaredClass;
            if (declaredClass == nullptr || declaredClass->schemaClass == nullptr || declaredClass->schemaClass->fields == nullptr) {
                continue;
            }

            auto* schemaClass = declaredClass->schemaClass;
            if (schemaClass->name == nullptr) {
                continue;
            }

            auto& classFields = fields[schemaClass->name];
            for (std::uint16_t fieldIndex = 0; fieldIndex < schemaClass->numFields; ++fieldIndex) {
                const auto& field = schemaClass->fields[fieldIndex];
                if (field.name == nullptr) {
                    continue;
                }
                classFields[field.name] = field.offset;
            }
        }
    }

    auto getOffset = [&fields](const char* className, const char* fieldName, std::ptrdiff_t& target) -> bool {
        const auto classIt = fields.find(className);
        if (classIt == fields.end()) {
            return false;
        }

        const auto fieldIt = classIt->second.find(fieldName);
        if (fieldIt == classIt->second.end()) {
            return false;
        }

        target = fieldIt->second;
        return true;
    };

    bool ok = true;
    ok = ok && getOffset("CEntityInstance", "m_pEntity", g_offsets.entityInstanceToIdentity);
    ok = ok && getOffset("CBasePlayerController", "m_hPawn", g_offsets.controllerPawnHandle);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pObserverServices", g_offsets.pawnObserverServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pCameraServices", g_offsets.pawnCameraServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_pWeaponServices", g_offsets.pawnWeaponServices);
    ok = ok && getOffset("C_BasePlayerPawn", "m_hController", g_offsets.pawnControllerHandle);
    ok = ok && getOffset("CPlayer_ObserverServices", "m_iObserverMode", g_offsets.observerMode);
    ok = ok && getOffset("CPlayer_ObserverServices", "m_hObserverTarget", g_offsets.observerTarget);
    ok = ok && getOffset("CPlayer_CameraServices", "m_hViewEntity", g_offsets.cameraViewEntity);
    ok = ok && getOffset("CPlayer_WeaponServices", "m_hActiveWeapon", g_offsets.activeWeapon);
    ok = ok && getOffset("C_BaseEntity", "m_pGameSceneNode", g_offsets.sceneNode);
    ok = ok && getOffset("CGameSceneNode", "m_vecAbsOrigin", g_offsets.sceneAbsOrigin);

    if (!ok) {
        SetInitialized(false, Texts().reasonRequiredSchemaOffsetsMissing);
        return false;
    }

    return true;
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

    const auto splitScreenPlayers = FindPatterns(g_clientModule, kPatternGetSplitScreenPlayer);
    if (splitScreenPlayers.empty()) {
        SetInitialized(false, Texts().reasonGetSplitScreenPlayerPatternMissing);
        return false;
    }

    g_getSplitScreenPlayerCandidates.clear();
    for (const auto splitScreenPlayer : splitScreenPlayers) {
        g_getSplitScreenPlayerCandidates.push_back(reinterpret_cast<GetSplitScreenPlayerFn>(splitScreenPlayer));
    }
    g_getSplitScreenPlayer = g_getSplitScreenPlayerCandidates.front();
    if (g_getSplitScreenPlayerCandidates.size() > 1) {
        ConsolePrintf(Texts().logSplitScreenMatches, g_getSplitScreenPlayerCandidates.size());
    }

    const auto attachmentPattern = FindPattern(g_clientModule, kPatternAttachmentAccess);
    if (attachmentPattern == 0) {
        SetInitialized(false, Texts().reasonAttachmentAccessPatternMissing);
        return false;
    }

    g_lookupAttachment = reinterpret_cast<LookupAttachmentFn>(attachmentPattern + 5 + *reinterpret_cast<std::int32_t*>(attachmentPattern + 1));
    g_getAttachment = reinterpret_cast<GetAttachmentFn>(attachmentPattern + 40 + 5 + *reinterpret_cast<std::int32_t*>(attachmentPattern + 40 + 1));

    return g_entityListStorage != nullptr && g_getHighestEntityIndex != nullptr && g_getEntityFromIndex != nullptr && g_lookupAttachment != nullptr && g_getAttachment != nullptr && g_getSplitScreenPlayer != nullptr;
}

bool EntityIsPlayerPawn(void* entity) {
    if (entity == nullptr) {
        return false;
    }
    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(entity, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(entity);
    if (vtable == nullptr) {
        return false;
    }
    auto function = reinterpret_cast<bool(__fastcall*)(void*)>(vtable[151]);
    return function(entity);
}

bool EntityIsPlayerController(void* entity) {
    if (entity == nullptr) {
        return false;
    }
    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(entity, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return false;
    }

    auto** vtable = *reinterpret_cast<void***>(entity);
    if (vtable == nullptr) {
        return false;
    }
    auto function = reinterpret_cast<bool(__fastcall*)(void*)>(vtable[152]);
    return function(entity);
}

void EntityGetRenderEyeOrigin(void* entity, float output[3]) {
    if (entity == nullptr) {
        output[0] = 0.0f;
        output[1] = 0.0f;
        output[2] = 0.0f;
        return;
    }
    auto** vtable = *reinterpret_cast<void***>(entity);
    auto function = reinterpret_cast<void(__fastcall*)(void*, float output[3])>(vtable[166]);
    function(entity, output);
}

EntityHandle EntityGetHandle(void* entity) {
    if (entity == nullptr) {
        return {};
    }

    auto* identity = *reinterpret_cast<std::uint8_t**>(reinterpret_cast<std::uint8_t*>(entity) + g_offsets.entityInstanceToIdentity);
    if (identity == nullptr) {
        return {};
    }

    return EntityHandle{ *reinterpret_cast<std::uint32_t*>(identity + 0x10) };
}

EntityHandle ControllerGetPawnHandle(void* controller) {
    if (controller == nullptr || !EntityIsPlayerController(controller)) {
        return {};
    }
    return EntityHandle{ *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(controller) + g_offsets.controllerPawnHandle) };
}

bool TryGetEntityAbsOrigin(void* entity, Vec3& origin, std::string& failure) {
    if (entity == nullptr) {
        failure = Texts().reasonWeaponEntityMissing;
        return false;
    }

    auto* sceneNode = *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(entity) + g_offsets.sceneNode);
    if (sceneNode == nullptr) {
        failure = Texts().reasonWeaponSceneNodeMissing;
        return false;
    }

    auto* sceneOrigin = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(sceneNode) + g_offsets.sceneAbsOrigin);
    origin = Vec3{ sceneOrigin[0], sceneOrigin[1], sceneOrigin[2] };
    if (!IsFiniteVec3(origin)) {
        failure = Texts().reasonWeaponOriginInvalid;
        return false;
    }

    return true;
}

EntityHandle PawnGetActiveWeaponHandle(void* pawn) {
    if (pawn == nullptr || !EntityIsPlayerPawn(pawn)) {
        return {};
    }

    auto* weaponServices = *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(pawn) + g_offsets.pawnWeaponServices);
    if (weaponServices == nullptr) {
        return {};
    }

    return EntityHandle{ *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(weaponServices) + g_offsets.activeWeapon) };
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
    auto* observerServices = *reinterpret_cast<void**>(reinterpret_cast<std::uint8_t*>(pawn) + g_offsets.pawnObserverServices);
    if (observerServices == nullptr) {
        return {};
    }
    return EntityHandle{ *reinterpret_cast<std::uint32_t*>(reinterpret_cast<std::uint8_t*>(observerServices) + g_offsets.observerTarget) };
}

void* ResolveHandleToEntity(EntityHandle handle) {
    if (!handle.IsValid() || g_entityListStorage == nullptr || *g_entityListStorage == nullptr || g_getEntityFromIndex == nullptr) {
        return nullptr;
    }
    return g_getEntityFromIndex(*g_entityListStorage, handle.Index());
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

    auto* fieldAddress = reinterpret_cast<std::uint8_t*>(entity) + g_offsets.entityInstanceToIdentity;
    MEMORY_BASIC_INFORMATION memoryInfo{};
    if (VirtualQuery(fieldAddress, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return false;
    }

    auto* identity = *reinterpret_cast<std::uint8_t**>(fieldAddress);
    if (identity == nullptr) {
        return false;
    }

    if (VirtualQuery(identity + 0x10, &memoryInfo, sizeof(memoryInfo)) == 0 || memoryInfo.State != MEM_COMMIT || (memoryInfo.Protect & PAGE_GUARD) != 0 || memoryInfo.Protect == PAGE_NOACCESS) {
        return false;
    }

    const EntityHandle handle{ *reinterpret_cast<std::uint32_t*>(identity + 0x10) };
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
        void* entity = g_getSplitScreenPlayer(0);
        if (entity != nullptr && (EntityIsPlayerController(entity) || EntityIsPlayerPawn(entity))) {
            return entity;
        }
    }

    for (std::size_t index = 0; index < g_getSplitScreenPlayerCandidates.size(); ++index) {
        auto candidate = g_getSplitScreenPlayerCandidates[index];
        if (candidate == nullptr) {
            continue;
        }

        void* entity = candidate(0);
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
        return ResolveHandleToEntity(ControllerGetPawnHandle(entity));
    }

    return nullptr;
}

struct TargetResolution {
    void* entity{};
    EntityHandle handle{};
    std::string failure;
};

TargetResolution ResolveFollowTarget() {
    if (g_getSplitScreenPlayerCandidates.empty()) {
        return { nullptr, {}, Texts().reasonSplitScreenPlayerAccessUnavailable };
    }

    void* splitScreenEntity = ResolveSplitScreenPlayerEntity();
    if (splitScreenEntity == nullptr) {
        return { nullptr, {}, Texts().reasonSplitScreenPlayerMissing };
    }

    void* observerPawn = nullptr;
    if (EntityIsPlayerController(splitScreenEntity)) {
        observerPawn = ResolveHandleToEntity(ControllerGetPawnHandle(splitScreenEntity));
    }
    else if (EntityIsPlayerPawn(splitScreenEntity)) {
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
        return { nullptr, {}, Texts().reasonNoObserverTarget };
    }

    void* targetEntity = ResolveEntityToPawn(ResolveHandleToEntity(observerTarget));
    if (targetEntity == nullptr) {
        return { nullptr, {}, Texts().reasonObserverTargetUnresolved };
    }

    return { targetEntity, EntityGetHandle(targetEntity), {} };
}

TargetResolution ResolveLockedTarget(EntityHandle handle) {
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
    g_lookupAttachment(entity, attachmentIndex, attachmentName);
    if (attachmentIndex == 0xFFu) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, attachmentName);
        return false;
    }

    alignas(16) float data[8]{};
    if (!g_getAttachment(entity, attachmentIndex, data)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, attachmentName);
        return false;
    }

    origin.x = data[0];
    origin.y = data[1];
    origin.z = data[2];
    return true;
}

bool TryBuildHumanFrameBasis(void* targetPawn, HeldItemKind heldItemKind, WeaponFrameBasis& basis, std::string& failure) {
    Vec3 handOrigin{};
    if (!TryGetAttachmentOrigin(targetPawn, kAnchorAttachmentName, handOrigin, failure)) {
        failure = FormatText(Texts().reasonAttachmentMissingFormat, kAnchorAttachmentName);
        return false;
    }

    float eyeOriginRaw[3]{};
    EntityGetRenderEyeOrigin(targetPawn, eyeOriginRaw);
    const Vec3 eyeOrigin{ eyeOriginRaw[0], eyeOriginRaw[1], eyeOriginRaw[2] };
    if (!IsFiniteVec3(eyeOrigin)) {
        failure = Texts().reasonInvalidEyeOrigin;
        return false;
    }

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
    return true;
}

bool TryBuildGunFrameBasis(void* targetPawn, WeaponFrameBasis& basis, std::string& failure) {
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
    EntityGetRenderEyeOrigin(targetPawn, eyeOriginRaw);
    const Vec3 eyeOrigin{ eyeOriginRaw[0], eyeOriginRaw[1], eyeOriginRaw[2] };
    if (!IsFiniteVec3(eyeOrigin)) {
        failure = Texts().reasonInvalidEyeOrigin;
        return false;
    }

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
    StabilizeGunBasisRightAxis(targetHandle, basis);
    basis.anchorOrigin = StabilizeGunAnchorOrigin(targetHandle, weaponHandle, handMidpoint, basis, muzzleOrigin);
    return true;
}

bool TryBuildWeaponFrameBasis(void* targetPawn, WeaponFrameBasis& basis, std::string& failure) {
    const HeldItemKind heldItemKind = GetHeldItemKindForPawn(targetPawn);
    if (heldItemKind == HeldItemKind::Gun) {
        return TryBuildGunFrameBasis(targetPawn, basis, failure);
    }

    return TryBuildHumanFrameBasis(targetPawn, heldItemKind, basis, failure);
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

    auto* viewFov = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupFovOffset);
    const float currentFov = *viewFov;
    if (!std::isfinite(currentFov) || currentFov <= 1.0f) {
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

void __fastcall SelfieStickSetUpView(void* viewSetup) {
    if (viewSetup == nullptr) {
        return;
    }

    if (!g_setupViewCallbackObserved.exchange(true)) {
        ConsolePrintf(Texts().logSetUpViewActive);
    }

    const OverrideConfig config = GetOverrideConfig();
    if (!config.enabled) {
        return;
    }

    if (!g_enabledSetUpViewObserved.exchange(true)) {
        ConsolePrintf(Texts().logEnabledCallbackEntered, TargetModeToString(config.targetMode), config.followHandle.raw, config.lockedHandle.raw);
    }

    TargetResolution resolution{};
    if (config.targetMode == TargetMode::Locked) {
        resolution = ResolveLockedTarget(config.lockedHandle);
    }
    else {
        resolution = ResolveFollowSnapshotTarget(config.followHandle);
    }

    if (resolution.entity == nullptr) {
        UpdateFailureReason(resolution.failure, false);
        return;
    }

    WeaponFrameBasis basis{};
    std::string failure;
    if (!TryBuildWeaponFrameBasis(resolution.entity, basis, failure)) {
        UpdateFailureReason(failure, false);
        return;
    }

    const Vec3 baseOffset = GetBaseOffsetForHeldItem(basis.heldItemKind, config.lookMode);

    const Vec3 composedTrim{
        GetCameraRightOffsetForBasis(basis, config.lookMode, baseOffset.x) + config.offset.x,
        baseOffset.y + config.offset.y,
        baseOffset.z + config.offset.z
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
        UpdateFailureReason(Texts().reasonCameraDirectionDegenerate, true);
        return;
    }

    float pitch{};
    float yaw{};
    VectorToAngles(direction, pitch, yaw);

    OverrideScopedAwpFov(viewSetup, resolution.handle, basis);

    auto* viewOrigin = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupOriginOffset);
    auto* viewAngles = reinterpret_cast<float*>(reinterpret_cast<std::uint8_t*>(viewSetup) + kViewSetupAnglesOffset);
    viewOrigin[0] = cameraPosition.x;
    viewOrigin[1] = cameraPosition.y;
    viewOrigin[2] = cameraPosition.z;
    viewAngles[0] = pitch;
    viewAngles[1] = yaw;

    UpdateOverrideSuccess(resolution.handle);
}

bool HookSetUpView() {
    const auto patchAddress = FindPattern(g_clientModule, kPatternSetupViewPatch);
    if (patchAddress == 0) {
        SetInitialized(false, Texts().reasonSetUpViewPatchSiteMissing);
        return false;
    }

    g_setupViewPatchAddress = reinterpret_cast<void*>(patchAddress);
    std::memcpy(g_setupViewOriginalBytes.data(), g_setupViewPatchAddress, g_setupViewOriginalBytes.size());

    const auto helperStringAddress = patchAddress + 12 + *reinterpret_cast<std::int32_t*>(patchAddress + 8);
    const auto helperCallTarget = patchAddress + 17 + *reinterpret_cast<std::int32_t*>(patchAddress + 13);
    const auto returnAddress = patchAddress + g_setupViewOriginalBytes.size();
    auto* detourStub = reinterpret_cast<std::uint8_t*>(VirtualAlloc(nullptr, 96, MEM_COMMIT | MEM_RESERVE, PAGE_EXECUTE_READWRITE));
    if (detourStub == nullptr) {
        SetInitialized(false, Texts().reasonSetUpViewDetourAllocationFailed);
        return false;
    }

    g_setupViewDetourStub = detourStub;

    std::vector<std::uint8_t> stub;
    stub.reserve(96);

    auto appendByte = [&stub](std::uint8_t value) {
        stub.push_back(value);
    };

    auto appendImmediate64 = [&stub](std::uintptr_t value) {
        for (std::size_t index = 0; index < sizeof(value); ++index) {
            stub.push_back(static_cast<std::uint8_t>((value >> (index * 8)) & 0xFFu));
        }
    };

    appendByte(0xBA);
    appendByte(0xFF);
    appendByte(0xFF);
    appendByte(0xFF);
    appendByte(0xFF);

    appendByte(0x48);
    appendByte(0xB9);
    appendImmediate64(helperStringAddress);

    appendByte(0x48);
    appendByte(0xB8);
    appendImmediate64(helperCallTarget);
    appendByte(0xFF);
    appendByte(0xD0);

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
    appendByte(0x48);
    appendByte(0xB8);
    appendImmediate64(reinterpret_cast<std::uintptr_t>(&SelfieStickSetUpView));
    appendByte(0xFF);
    appendByte(0xD0);

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

    appendByte(0x49);
    appendByte(0xBB);
    appendImmediate64(returnAddress);
    appendByte(0x41);
    appendByte(0xFF);
    appendByte(0xE3);

    std::memcpy(detourStub, stub.data(), stub.size());
    FlushInstructionCache(GetCurrentProcess(), detourStub, stub.size());

    std::array<std::uint8_t, 17> patch{
        0x49, 0xBB,
        0, 0, 0, 0, 0, 0, 0, 0,
        0x41, 0xFF, 0xE3,
        0x90, 0x90, 0x90, 0x90
    };
    std::memcpy(patch.data() + 2, &g_setupViewDetourStub, sizeof(g_setupViewDetourStub));

    if (!WriteProcessMemoryProtected(g_setupViewPatchAddress, patch.data(), patch.size())) {
        VirtualFree(detourStub, 0, MEM_RELEASE);
        g_setupViewDetourStub = nullptr;
        SetInitialized(false, Texts().reasonSetUpViewPatchFailed);
        return false;
    }

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


DWORD WINAPI InitializeSelfieStick(LPVOID) {
    {
        std::lock_guard lock(g_stateMutex);
        RefreshStatusTextLocked();
    }

    LoadPersistedOverlayHotkeySetting();

    const bool overlayUiStarted = StartOverlayUiThread();
    if (!overlayUiStarted) {
        SetInitialized(false, Texts().reasonOverlayUiThreadCreationFailed);
        ConsolePrintf(Texts().logOverlayUiThreadCreationFailed);
    }

    if (!LoadInterfaces()) {
        ConsolePrintf(Texts().logInterfaceInitializationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }

    if (!ResolveSchemaOffsets()) {
        ConsolePrintf(Texts().logSchemaInitializationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }

    if (!ResolveEntityAccess()) {
        ConsolePrintf(Texts().logEntityInitializationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }

    const bool setupViewHooked = HookSetUpView();
    const bool frameStageHooked = HookFrameStageNotify();
    SetHookFlags(setupViewHooked, frameStageHooked);

    if (!setupViewHooked || !frameStageHooked) {
        ConsolePrintf(Texts().logHookInstallationFailed, SnapshotState().lastFailureReason.c_str());
        return 0;
    }

    if (!overlayUiStarted) {
        return 0;
    }

    SetInitialized(true, "");
    ConsolePrintf(Texts().logBuild, kBuildTag);
    ConsolePrintf(Texts().logReady);
    return 0;
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
