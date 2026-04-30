// LICENSE_CODE ZON

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "BrightSDKSubsystem.generated.h"

UENUM(BlueprintType)
enum class EBrightSDKAuthorizationStatus : uint8
{
    Authorized = 0 UMETA(DisplayName="Authorized"),
    SdkNotInitialized = 1 UMETA(DisplayName="SDK Not Initialized"),
    ParentControlEnabled = 2 UMETA(DisplayName="Parent Control Enabled"),
    Unknown = (uint8)-1 UMETA(DisplayName="Unknown or unsupported status"),
};
UENUM(BlueprintType)
enum class EBrightSDKChoice : uint8
{
    None = 0 UMETA(DisplayName="None"),
    Peer = 1 UMETA(DisplayName="Peer"),
    NotPeer = 2 UMETA(DisplayName="Not peer"),
};
UENUM(BlueprintType)
enum class EBrightSDKChoiceTriggerType : uint8
{
    Consent = 0 UMETA(DisplayName="Trigger from consent screen"),
    Manual = 1 UMETA(DisplayName="Trigger from code"),
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FBrightSDKChoiceChanged, EBrightSDKChoice, NewChoice);

UCLASS()
class BRIGHTSDK_API UBrightSDKSubsystem : public UGameInstanceSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;

public:
    UPROPERTY(BlueprintAssignable, Category="BrightSDK")
    FBrightSDKChoiceChanged OnChoiceChanged;

public:
    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    void WindowsPreInit(const FString& AppId, const FString& AppName) const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    int64 TryInitialize(const FString& Benefit,
                        const FString& AgreeButton,
                        const FString& DisagreeButton,
                        const FString& OptOutInstructions,
                        const FString& AppIconNameOrPath,
                        bool SkipConsent,
                        const FString& Campaign) const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    FString GetVersion() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    FString GetUUIDOrEmpty() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    EBrightSDKAuthorizationStatus AuthorizeDevice() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    EBrightSDKChoice GetCurrentChoice() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    int64 ExternalOptIn(EBrightSDKChoiceTriggerType Trigger) const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    void OptOut(EBrightSDKChoiceTriggerType Trigger) const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    void NotifyConsentShown() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    bool ShowConsent() const;

    UFUNCTION(BlueprintCallable, Category="BrightSDK")
    void SetTrackingId(const FString& TrackingId) const;
};
