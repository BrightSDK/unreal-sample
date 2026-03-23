// LICENSE_CODE ZON

#include "BrightSDKSubsystem.h"
#include <stdio.h>
#include "Async/Async.h"

#if PLATFORM_IOS || PLATFORM_TVOS
#define APPLE_MOBILE
#endif

#ifdef APPLE_MOBILE
#ifdef __cplusplus
extern "C" {
#endif
void brdSDK_SetOnChoiceChangeCallback(void (*ChoiceChangedCallback)(int64_t));

char* brdSDK_Version(void);
char* brdSDK_GetUUID(void);
int64_t brdSDK_AuthorizeDevice(void);
int64_t brdSDK_CurrentChoice(void);
void brdSDK_ExternalOptIn(int64_t);
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

static EBrightSDKChoice _MapChoice(int64_t raw)
{
    switch (raw) {
        case 0: return EBrightSDKChoice::None;
        case 1: return EBrightSDKChoice::Peer;
        case 2: return EBrightSDKChoice::NotPeer;
        default: return EBrightSDKChoice::None;
    }
}

static TWeakObjectPtr<UBrightSDKSubsystem> GBrightSDKSubsystem;
static void _OnChoiceChangeRawHandler(int64_t raw)
{
    EBrightSDKChoice choice = _MapChoice(raw);
    AsyncTask(ENamedThreads::GameThread, [choice](){
        if (UBrightSDKSubsystem* S = GBrightSDKSubsystem.Get())
        {
            S->OnChoiceChanged.Broadcast(choice);
        }
    });
}
#endif //APPLE_MOBILE

// MARK: -----

void UBrightSDKSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

#ifdef APPLE_MOBILE
    GBrightSDKSubsystem = this;
#endif
}

void UBrightSDKSubsystem::Deinitialize()
{
#ifdef APPLE_MOBILE
    GBrightSDKSubsystem.Reset();
#endif

    Super::Deinitialize();
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
#ifdef APPLE_MOBILE
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
#else
    return INT32_MIN;
#endif
}

FString UBrightSDKSubsystem::GetVersion() const
{
#ifdef APPLE_MOBILE
    char* val = brdSDK_Version();
    FString result = val ? FString(UTF8_TO_TCHAR(val)) : FString();
    if (val) free(val);
    return result;
#else
    return FString();
#endif
}

FString UBrightSDKSubsystem::GetUUIDOrEmpty() const
{
#ifdef APPLE_MOBILE
    char* val = brdSDK_GetUUID();
    FString result = val ? FString(UTF8_TO_TCHAR(val)) : FString();
    if (val) free(val);
    return result;
#else
    return FString();
#endif
}

EBrightSDKAuthorizationStatus UBrightSDKSubsystem::AuthorizeDevice() const
{
#ifdef APPLE_MOBILE
    int64_t raw = brdSDK_AuthorizeDevice();
    switch (raw) {
        case 0: return EBrightSDKAuthorizationStatus::Authorized;
        case -1: return EBrightSDKAuthorizationStatus::SdkNotInitialized;
        case -2: return EBrightSDKAuthorizationStatus::ParentControlEnabled;
        default: return EBrightSDKAuthorizationStatus::Unknown;
    }
#else
    return EBrightSDKAuthorizationStatus::Unknown;
#endif
}

EBrightSDKChoice UBrightSDKSubsystem::GetCurrentChoice() const
{
#ifdef APPLE_MOBILE
    int64_t raw = brdSDK_CurrentChoice();
    return _MapChoice(raw);
#else
    return EBrightSDKChoice::None;
#endif
}

void UBrightSDKSubsystem::ExternalOptIn(EBrightSDKChoiceTriggerType Trigger) const
{
#ifdef APPLE_MOBILE
    brdSDK_ExternalOptIn((int64_t)Trigger);
#endif
}

void UBrightSDKSubsystem::OptOut(EBrightSDKChoiceTriggerType Trigger) const
{
#ifdef APPLE_MOBILE
    brdSDK_OptOut((int64_t)Trigger);
#endif
}

void UBrightSDKSubsystem::NotifyConsentShown() const
{
#ifdef APPLE_MOBILE
    brdSDK_NotifyConsentShown();
#endif
}

bool UBrightSDKSubsystem::ShowConsent() const
{
#ifdef APPLE_MOBILE
    __block bool Result = false;
    _RunOnIOSMainThreadSync(^{
        Result = brdSDK_ShowConsent(nullptr, nullptr, nullptr, nullptr, 0);
    });
    return Result;
#else
    return false;
#endif
}

void UBrightSDKSubsystem::SetTrackingId(const FString& TrackingId) const
{
#ifdef APPLE_MOBILE
    if (TrackingId.IsEmpty())
    {
        brdSDK_SetTrackingId(nullptr);
        return;
    }

    FTCHARToUTF8 Utf8(*TrackingId);
    brdSDK_SetTrackingId(Utf8.Get());
#endif
}
