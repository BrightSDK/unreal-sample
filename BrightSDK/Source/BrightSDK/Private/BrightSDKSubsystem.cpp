// LICENSE_CODE ZON

#include "BrightSDKSubsystem.h"

#include <stdio.h>

#include "Async/Async.h"
#include "HAL/PlatformProcess.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/Paths.h"

#if PLATFORM_IOS || PLATFORM_TVOS
#define BRIGHTSDK_APPLE_MOBILE 1
#else
#define BRIGHTSDK_APPLE_MOBILE 0
#endif

#if PLATFORM_WINDOWS && PLATFORM_64BITS
#define BRIGHTSDK_WIN64 1
#else
#define BRIGHTSDK_WIN64 0
#endif

static EBrightSDKChoice _MapChoice(int64_t raw)
{
    switch (raw)
    {
        case 0: return EBrightSDKChoice::None;
        case 1: return EBrightSDKChoice::Peer;
        case 2: return EBrightSDKChoice::NotPeer;
        default: return EBrightSDKChoice::None;
    }
}

static TWeakObjectPtr<UBrightSDKSubsystem> GBrightSDKSubsystem;

static void _BroadcastChoiceChanged(int64_t raw)
{
    EBrightSDKChoice choice = _MapChoice(raw);
    AsyncTask(ENamedThreads::GameThread, [choice]()
    {
        if (UBrightSDKSubsystem* Subsystem = GBrightSDKSubsystem.Get())
        {
            Subsystem->OnChoiceChanged.Broadcast(choice);
        }
    });
}

#if BRIGHTSDK_APPLE_MOBILE
#ifdef __cplusplus
extern "C" {
#endif
void brdSDK_SetOnChoiceChangeCallback(void (*ChoiceChangedCallback)(int64_t));

char* brdSDK_Version(void);
char* brdSDK_GetUUID(void);
int64_t brdSDK_AuthorizeDevice(void);
int64_t brdSDK_CurrentChoice(void);
int64_t brdSDK_ExternalOptIn(int64_t);
void brdSDK_OptOut(int64_t);
void brdSDK_NotifyConsentShown();
bool brdSDK_ShowConsent(const char *, const char *, const char *, const char *, int64_t);
void brdSDK_SetTrackingId(const char *);
int64_t brdSDK_StaticInit(const char * /*benefit*/,
                          const char * /*agreeTitle*/,
                          const char * /*disagreeTitle*/,
                          const char * /*optInInstructions*/,
                          const char * /*appIcon*/,
                          const char * /*language*/,
                          bool /*skipConsent*/,
                          const void * /*consentColors*/,
                          const void * /*backgroundImage*/,
                          const void * /*optInInfo*/,
                          const void * /*optOutInfo*/,
                          const void * /*fonts*/,
                          const char * /*campaign*/);

extern void _RunOnIOSMainThreadSync(void (^block)(void));
#ifdef __cplusplus
}
#endif

static void _OnChoiceChangeRawHandler(int64_t raw)
{
    _BroadcastChoiceChanged(raw);
}
#endif

#if BRIGHTSDK_WIN64
namespace
{
    using FBrdSdkChoiceChangeCallback = void (__stdcall *)(int);
    using FBrdSdkIsSupportedFunction = int (__stdcall *)(void);
    using FBrdSdkInitFunction = void (__stdcall *)(void);
    using FBrdSdkShowConsentFunction = void (__stdcall *)(void);
    using FBrdSdkOptOutFunction = void (__stdcall *)(void);
    using FBrdSdkOptInFunction = void (__stdcall *)(void);
    using FBrdSdkGetConsentChoiceFunction = int (__stdcall *)(void);
    using FBrdSdkCloseFunction = void (__stdcall *)(void);
    using FBrdSdkSetChoiceChangeCbFunction = void (__stdcall *)(FBrdSdkChoiceChangeCallback);
    using FBrdSdkSetSkipConsentOnInitFunction = void (__stdcall *)(unsigned char);
    using FBrdSdkSetStringFunction = void (__stdcall *)(char*);
    using FBrdSdkGetStringFunction = char* (__stdcall *)(void);

    struct FWin64BrightSdkApi
    {
        void* DllHandle = nullptr;
        FString DllPath;
        FBrdSdkIsSupportedFunction IsSupported = nullptr;
        FBrdSdkInitFunction Init = nullptr;
        FBrdSdkShowConsentFunction ShowConsent = nullptr;
        FBrdSdkOptOutFunction OptOut = nullptr;
        FBrdSdkOptInFunction OptIn = nullptr;
        FBrdSdkGetConsentChoiceFunction GetConsentChoice = nullptr;
        FBrdSdkCloseFunction Close = nullptr;
        FBrdSdkSetChoiceChangeCbFunction SetChoiceChangeCb = nullptr;
        FBrdSdkSetSkipConsentOnInitFunction SetSkipConsentOnInit = nullptr;
        FBrdSdkSetStringFunction SetAppId = nullptr;
        FBrdSdkSetStringFunction SetAppName = nullptr;
        FBrdSdkSetStringFunction SetLogoLink = nullptr;
        FBrdSdkSetStringFunction SetBenefit = nullptr;
        FBrdSdkSetStringFunction SetBenefitText = nullptr;
        FBrdSdkSetStringFunction SetAgreeButton = nullptr;
        FBrdSdkSetStringFunction SetDisagreeButton = nullptr;
        FBrdSdkSetStringFunction SetCampaign = nullptr;
        FBrdSdkGetStringFunction GetVersion = nullptr;
        FBrdSdkSetStringFunction SetOptOutText = nullptr;
        FBrdSdkGetStringFunction GetUuid = nullptr;
        FBrdSdkSetStringFunction SetTrackingId = nullptr;
    };

    static FWin64BrightSdkApi GWin64BrightSdkApi;
    static bool GWin64Initialized = false;

    static FString GetWin64DllPath()
    {
        const TArray<FString> CandidatePaths = {
            // 1. Plugin ThirdParty bundle (self-contained package — preferred)
            FPaths::ConvertRelativePathToFull(FPaths::Combine(
                IPluginManager::Get().FindPlugin(TEXT("BrightSDK"))->GetBaseDir(),
                TEXT("ThirdParty/Win64/lum_sdk64.dll"))),
            // 2. Staged binary dir (packaged game)
            FPaths::Combine(FPlatformProcess::BaseDir(), TEXT("lum_sdk64.dll")),
            // 3. Legacy external ThirdParty location (backward compat)
            FPaths::ConvertRelativePathToFull(FPaths::Combine(FPaths::ProjectDir(), TEXT("ThirdParty"), TEXT("BrightSdk"), TEXT("lum_sdk64.dll"))),
        };

        for (const FString& CandidatePath : CandidatePaths)
        {
            if (FPaths::FileExists(CandidatePath))
            {
                return CandidatePath;
            }
        }

        return CandidatePaths[0];
    }

    template <typename T>
    static bool LoadRequiredExport(T& Function, const TCHAR* ExportName)
    {
        Function = reinterpret_cast<T>(FPlatformProcess::GetDllExport(GWin64BrightSdkApi.DllHandle, ExportName));
        if (!Function)
        {
            UE_LOG(LogTemp, Error, TEXT("BrightSDK Win64 DLL is missing export %s"), ExportName);
            return false;
        }

        return true;
    }

    template <typename T>
    static void LoadOptionalExport(T& Function, const TCHAR* ExportName)
    {
        Function = reinterpret_cast<T>(FPlatformProcess::GetDllExport(GWin64BrightSdkApi.DllHandle, ExportName));
    }

    static void UnloadWin64Bridge()
    {
        if (GWin64BrightSdkApi.DllHandle)
        {
            FPlatformProcess::FreeDllHandle(GWin64BrightSdkApi.DllHandle);
        }

        GWin64BrightSdkApi = FWin64BrightSdkApi();
        GWin64Initialized = false;
    }

    static bool EnsureWin64BridgeLoaded()
    {
        if (GWin64BrightSdkApi.DllHandle)
        {
            return true;
        }

        GWin64BrightSdkApi.DllPath = GetWin64DllPath();
        if (!FPaths::FileExists(GWin64BrightSdkApi.DllPath))
        {
            UE_LOG(LogTemp, Error, TEXT("BrightSDK Win64 DLL was not found: %s"), *GWin64BrightSdkApi.DllPath);
            return false;
        }

        GWin64BrightSdkApi.DllHandle = FPlatformProcess::GetDllHandle(*GWin64BrightSdkApi.DllPath);
        if (!GWin64BrightSdkApi.DllHandle)
        {
            UE_LOG(LogTemp, Error, TEXT("BrightSDK failed to load Win64 DLL: %s"), *GWin64BrightSdkApi.DllPath);
            UnloadWin64Bridge();
            return false;
        }

        bool bLoadedAllRequiredExports =
            LoadRequiredExport(GWin64BrightSdkApi.IsSupported, TEXT("brd_sdk_is_supported_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.Init, TEXT("brd_sdk_init_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.ShowConsent, TEXT("brd_sdk_show_consent_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.OptOut, TEXT("brd_sdk_opt_out_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.OptIn, TEXT("brd_sdk_opt_in_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.GetConsentChoice, TEXT("brd_sdk_get_consent_choice_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.Close, TEXT("brd_sdk_close_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetChoiceChangeCb, TEXT("brd_sdk_set_choice_change_cb_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetSkipConsentOnInit, TEXT("brd_sdk_set_skip_consent_on_init_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetAppId, TEXT("brd_sdk_set_appid_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetAppName, TEXT("brd_sdk_set_app_name_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetAgreeButton, TEXT("brd_sdk_set_agree_btn_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetDisagreeButton, TEXT("brd_sdk_set_disagree_btn_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetCampaign, TEXT("brd_sdk_set_campaign_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.GetUuid, TEXT("brd_sdk_get_uuid_c")) &&
            LoadRequiredExport(GWin64BrightSdkApi.SetTrackingId, TEXT("brd_sdk_set_tracking_id_c"));

        if (!bLoadedAllRequiredExports)
        {
            UnloadWin64Bridge();
            return false;
        }

        LoadOptionalExport(GWin64BrightSdkApi.SetLogoLink, TEXT("brd_sdk_set_logo_link_c"));
        LoadOptionalExport(GWin64BrightSdkApi.SetBenefit, TEXT("brd_sdk_set_benefit_c"));
        LoadOptionalExport(GWin64BrightSdkApi.SetBenefitText, TEXT("brd_sdk_set_benefit_txt_c"));
        LoadOptionalExport(GWin64BrightSdkApi.GetVersion, TEXT("brd_sdk_get_version_c"));
        LoadOptionalExport(GWin64BrightSdkApi.SetOptOutText, TEXT("brd_sdk_set_opt_out_txt_c"));

        return true;
    }

    static void __stdcall OnChoiceChangeRawHandlerWin64(int raw)
    {
        _BroadcastChoiceChanged(static_cast<int64_t>(raw));
    }

    static void ApplyStringSetting(const FString& Value, FBrdSdkSetStringFunction Setter)
    {
        if (!Setter)
        {
            return;
        }

        if (Value.IsEmpty())
        {
            Setter(nullptr);
            return;
        }

        FTCHARToUTF8 Utf8(*Value);
        Setter(const_cast<char*>(Utf8.Get()));
    }

    static void ApplyStringSetting(const FTCHARToUTF8& ValueUtf8, FBrdSdkSetStringFunction Setter)
    {
        if (Setter)
        {
            Setter(const_cast<char*>(ValueUtf8.Get()));
        }
    }
}
#endif

// MARK: -----

void UBrightSDKSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

#if BRIGHTSDK_APPLE_MOBILE || BRIGHTSDK_WIN64
    GBrightSDKSubsystem = this;
#endif
}

void UBrightSDKSubsystem::Deinitialize()
{
#if BRIGHTSDK_APPLE_MOBILE
    GBrightSDKSubsystem.Reset();
#endif

#if BRIGHTSDK_WIN64
    if (GWin64BrightSdkApi.DllHandle && GWin64BrightSdkApi.SetChoiceChangeCb)
    {
        GWin64BrightSdkApi.SetChoiceChangeCb(nullptr);
    }

    if (GWin64Initialized && GWin64BrightSdkApi.Close)
    {
        GWin64BrightSdkApi.Close();
    }

    UnloadWin64Bridge();
    GBrightSDKSubsystem.Reset();
#endif

    Super::Deinitialize();
}

// MARK: -----

void UBrightSDKSubsystem::WindowsPreInit(const FString& AppId, const FString& AppName) const
{
#if BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return;
    }

    ApplyStringSetting(AppId, GWin64BrightSdkApi.SetAppId);
    ApplyStringSetting(AppName, GWin64BrightSdkApi.SetAppName);
#endif
}

// MARK: -----

int64 UBrightSDKSubsystem::TryInitialize(const FString& Benefit,
                                     const FString& AgreeButton,
                                     const FString& DisagreeButton,
                                     const FString& OptOutInstructions,
                                     const FString& AppIconNameOrPath,
                                     bool SkipConsent,
                                     const FString& Campaign) const
{
#if BRIGHTSDK_APPLE_MOBILE
    FTCHARToUTF8 BenefitUtf8(*Benefit);
    FTCHARToUTF8 AgreeUtf8(*AgreeButton);
    FTCHARToUTF8 DisagreeUtf8(*DisagreeButton);
    FTCHARToUTF8 OptOutUtf8(*OptOutInstructions);
    FTCHARToUTF8 IconUtf8(*AppIconNameOrPath);
    FTCHARToUTF8 CampaignUtf8(*Campaign);

    const char* BenefitPtr = Benefit.IsEmpty() ? nullptr : BenefitUtf8.Get();
    const char* AgreePtr = AgreeButton.IsEmpty() ? nullptr : AgreeUtf8.Get();
    const char* DisagreePtr = DisagreeButton.IsEmpty() ? nullptr : DisagreeUtf8.Get();
    const char* OptOutPtr = OptOutInstructions.IsEmpty() ? nullptr : OptOutUtf8.Get();
    const char* IconPtr = AppIconNameOrPath.IsEmpty() ? nullptr : IconUtf8.Get();
    const char* CampaignPtr = Campaign.IsEmpty() ? nullptr : CampaignUtf8.Get();

    brdSDK_SetOnChoiceChangeCallback(&_OnChoiceChangeRawHandler);
    __block int64 Result = 0;
    _RunOnIOSMainThreadSync(^{
        Result = brdSDK_StaticInit(BenefitPtr,
                               AgreePtr,
                               DisagreePtr,
                               OptOutPtr,
                               IconPtr,
                               /*language*/ nullptr,
                               /*skipConsent*/ SkipConsent,
                               /*consentColors*/ nullptr,
                               /*backgroundImage*/ nullptr,
                               /*optInInfo*/ nullptr,
                               /*optOutInfo*/ nullptr,
                               /*fonts*/ nullptr,
                               CampaignPtr);
    });
    return Result;
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return 0;
    }

    ApplyStringSetting(Benefit, GWin64BrightSdkApi.SetBenefit);
    ApplyStringSetting(AgreeButton, GWin64BrightSdkApi.SetAgreeButton);
    ApplyStringSetting(DisagreeButton, GWin64BrightSdkApi.SetDisagreeButton);
    ApplyStringSetting(OptOutInstructions, GWin64BrightSdkApi.SetOptOutText);
    ApplyStringSetting(AppIconNameOrPath, GWin64BrightSdkApi.SetLogoLink);
    ApplyStringSetting(Campaign, GWin64BrightSdkApi.SetCampaign);

    GWin64BrightSdkApi.SetSkipConsentOnInit(SkipConsent ? 1 : 0);
    GWin64BrightSdkApi.SetChoiceChangeCb(&OnChoiceChangeRawHandlerWin64);
    GWin64BrightSdkApi.Init();
    GWin64Initialized = true;

    return 1;
#else
    return 0;
#endif
}

FString UBrightSDKSubsystem::GetVersion() const
{
#if BRIGHTSDK_APPLE_MOBILE
    char* val = brdSDK_Version();
    FString result = val ? FString(UTF8_TO_TCHAR(val)) : FString();
    if (val)
    {
        free(val);
    }
    return result;
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded() || !GWin64BrightSdkApi.GetVersion)
    {
        return FString();
    }

    char* value = GWin64BrightSdkApi.GetVersion();
    return value ? FString(UTF8_TO_TCHAR(value)) : FString();
#else
    return FString();
#endif
}

FString UBrightSDKSubsystem::GetUUIDOrEmpty() const
{
#if BRIGHTSDK_APPLE_MOBILE
    char* val = brdSDK_GetUUID();
    FString result = val ? FString(UTF8_TO_TCHAR(val)) : FString();
    if (val)
    {
        free(val);
    }
    return result;
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return FString();
    }

    char* value = GWin64BrightSdkApi.GetUuid();
    return value ? FString(UTF8_TO_TCHAR(value)) : FString();
#else
    return FString();
#endif
}

EBrightSDKAuthorizationStatus UBrightSDKSubsystem::AuthorizeDevice() const
{
#if BRIGHTSDK_APPLE_MOBILE
    int64_t raw = brdSDK_AuthorizeDevice();
    switch (raw)
    {
        case 0: return EBrightSDKAuthorizationStatus::Authorized;
        case -1: return EBrightSDKAuthorizationStatus::SdkNotInitialized;
        case -2: return EBrightSDKAuthorizationStatus::ParentControlEnabled;
        default: return EBrightSDKAuthorizationStatus::Unknown;
    }
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded() || !GWin64Initialized)
    {
        return EBrightSDKAuthorizationStatus::SdkNotInitialized;
    }

    return GWin64BrightSdkApi.IsSupported() != 0
        ? EBrightSDKAuthorizationStatus::Authorized
        : EBrightSDKAuthorizationStatus::Unknown;
#else
    return EBrightSDKAuthorizationStatus::Unknown;
#endif
}

EBrightSDKChoice UBrightSDKSubsystem::GetCurrentChoice() const
{
#if BRIGHTSDK_APPLE_MOBILE
    int64_t raw = brdSDK_CurrentChoice();
    return _MapChoice(raw);
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return EBrightSDKChoice::None;
    }

    return _MapChoice(static_cast<int64_t>(GWin64BrightSdkApi.GetConsentChoice()));
#else
    return EBrightSDKChoice::None;
#endif
}

int64 UBrightSDKSubsystem::ExternalOptIn(EBrightSDKChoiceTriggerType Trigger) const
{
    static_cast<void>(Trigger);

#if BRIGHTSDK_APPLE_MOBILE
    return (int64)brdSDK_ExternalOptIn((int64_t)Trigger);
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return 0;
    }

    GWin64BrightSdkApi.OptIn();
    return 1;
#else
    return 0;
#endif
}

void UBrightSDKSubsystem::OptOut(EBrightSDKChoiceTriggerType Trigger) const
{
    static_cast<void>(Trigger);

#if BRIGHTSDK_APPLE_MOBILE
    brdSDK_OptOut((int64_t)Trigger);
#elif BRIGHTSDK_WIN64
    if (EnsureWin64BridgeLoaded())
    {
        GWin64BrightSdkApi.OptOut();
    }
#endif
}

void UBrightSDKSubsystem::NotifyConsentShown() const
{
#if BRIGHTSDK_APPLE_MOBILE
    brdSDK_NotifyConsentShown();
#endif
}

bool UBrightSDKSubsystem::ShowConsent() const
{
#if BRIGHTSDK_APPLE_MOBILE
    __block bool Result = false;
    _RunOnIOSMainThreadSync(^{
        Result = brdSDK_ShowConsent(nullptr, nullptr, nullptr, nullptr, 0);
    });
    return Result;
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return false;
    }

    GWin64BrightSdkApi.ShowConsent();
    return true;
#else
    return false;
#endif
}

void UBrightSDKSubsystem::SetTrackingId(const FString& TrackingId) const
{
#if BRIGHTSDK_APPLE_MOBILE
    if (TrackingId.IsEmpty())
    {
        brdSDK_SetTrackingId(nullptr);
        return;
    }

    FTCHARToUTF8 Utf8(*TrackingId);
    brdSDK_SetTrackingId(Utf8.Get());
#elif BRIGHTSDK_WIN64
    if (!EnsureWin64BridgeLoaded())
    {
        return;
    }

    if (TrackingId.IsEmpty())
    {
        GWin64BrightSdkApi.SetTrackingId(nullptr);
        return;
    }

    FTCHARToUTF8 Utf8(*TrackingId);
    GWin64BrightSdkApi.SetTrackingId(const_cast<char*>(Utf8.Get()));
#endif
}
